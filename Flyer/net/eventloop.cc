#include "Flyer/net/eventloop.h"
#include "Flyer/common/log.h"
#include "Flyer/common/util.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>

#define ADD_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd()); \
    int op = EPOLL_CTL_ADD; \
    if(it != m_listen_fds.end()) \
        op = EPOLL_CTL_MOD; \
    epoll_event tmp = event->getEpollEvent(); \
    INFOLOG("epoll_event.events = %d", (int)tmp.events) \
    int rt = epoll_ctl(m_epool_fd, op, event->getFd(), &tmp); \
    if(rt == -1) \
        ERRORLOG("Failed epol_ctl when add fd %d, errno=%d, error=%s", event->getFd(), errno, strerror(errno)); \
    m_listen_fds.insert(event->getFd()); \
    DEBUGLOG("Add event success, fd[%d]", event->getFd())\

#define DELETE_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd());\
    if(it == m_listen_fds.end()) \
        return; \
    int op = EPOLL_CTL_DEL; \
    epoll_event tmp = event->getEpollEvent(); \
    int rt = epoll_ctl(m_epool_fd, op, event->getFd(), &tmp); \
    if(rt == -1) \
        ERRORLOG("Failed epol_ctl when delete fd %d, errno=%d, error=%s", event->getFd(), errno, strerror(errno)); \
    m_listen_fds.erase(event->getFd()); \
    DEBUGLOG("Delete event success, fd[%d]", event->getFd()) \

namespace Flyer {

static thread_local Eventloop* t_current_eventloop = NULL;

static int g_epoll_max_timeout = 1000;

static int g_epoll_max_events = 10;

Eventloop::Eventloop() {
    if(t_current_eventloop) {
        ERRORLOG("Failed to create event loop : This thread has created event loop");
        exit(0);
    }

    m_thread_id = getThreadId();

    m_epool_fd = epoll_create(10);
    if(m_epool_fd == -1) {
        ERRORLOG("Failed to create event loop : Epool_create error, error info [%d]", errno);
        exit(0);
    }

    initWakeupEvent();

    initTimer();

    INFOLOG("Succce create event loop in thread %d", m_thread_id);
    t_current_eventloop = this;
}

void Eventloop::initWakeupEvent() {
    m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
    if(m_wakeup_fd < 0) {
        ERRORLOG("Failed to create event loop : Eventfd error, error info [%d]", errno);
        exit(0);
    }
    INFOLOG("wake fd = %d", m_wakeup_fd);

    m_wakeup_fd_event = new WakeupFdEvent(m_wakeup_fd);
    m_wakeup_fd_event->listen(FdEvent::IN_EVENT, [this]() {
        char buf[8];
        while(read(m_wakeup_fd, buf, 8) != -1 && errno != EAGAIN) {
        }
        DEBUGLOG("Read full bytes from wakeup fd[%d]", m_wakeup_fd);
    });

    addEpollEvent(m_wakeup_fd_event);
}

void Eventloop::initTimer() {
    m_timer = new Timer();
    addEpollEvent(m_timer);
}

void Eventloop::addTimerEvent(TimerEvent::s_ptr event) {
    m_timer->addTimerEvent(event);
}


Eventloop::~Eventloop() {
    close(m_epool_fd);
    if(m_wakeup_fd_event) {
        delete m_wakeup_fd_event;
        m_wakeup_fd_event = NULL;
    }
    if(m_timer) {
        delete m_timer;
        m_timer = NULL;
    }
}

void Eventloop::loop() {
    m_is_looping = true;
    while(!m_stop_flag) {
        ScopeMutex<Mutex> lock(m_mutex);
        std::queue<std::function<void()>> tmp_tasks;
        m_pending_tasks.swap(tmp_tasks);
        lock.unlock();

        while(!tmp_tasks.empty()) {
            std::function<void()> cb  = tmp_tasks.front();
            tmp_tasks.pop();
            if(cb)
                cb();
        }

        int timeout = g_epoll_max_timeout;
        epoll_event result_events[g_epoll_max_events];
        // DEBUGLOG("now begin to epoll_wait");
        int rt = epoll_wait(m_epool_fd, result_events, g_epoll_max_events, timeout);
        // DEBUGLOG("now end epoll_wait, rt = %d", rt);

        if(rt < 0) {
            ERRORLOG("Epoll_wait error, errno = ", errno);
        }
        else {
            for(int i = 0; i < rt; i++) {
                epoll_event trigger_event = result_events[i];
                FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
                if(fd_event == NULL)
                    continue;
                if(trigger_event.events & EPOLLIN) {
                    DEBUGLOG("fd %d trigger EPOLLIN event", fd_event->getFd());
                    addTask(fd_event->handler(FdEvent::IN_EVENT));
                }
                if(trigger_event.events & EPOLLOUT) {
                    DEBUGLOG("fd %d trigger EPOLLOUT event", fd_event->getFd());
                    addTask(fd_event->handler(FdEvent::OUT_EVENT));
                }
                if(trigger_event.events & EPOLLERR) {
                    DEBUGLOG("fd %d trigger EPOLLERR event", fd_event->getFd());
                    deleteEpollEvent(fd_event);
                    if(fd_event->handler(FdEvent::ERROR_EVENT) != nullptr) {
                        DEBUGLOG("fd %d trigger EPOLLERR event", fd_event->getFd());
                        addTask(fd_event->handler(FdEvent::OUT_EVENT));
                    }
                }
            }
        }
    }
}

void Eventloop::wakeup() {
    m_wakeup_fd_event->wakeup();
}

void Eventloop::stop() {
    m_stop_flag = true;
    wakeup();
}

void Eventloop::dealWakeup() {

}

void Eventloop::addEpollEvent(FdEvent* event) {
    if(isInLoopThread()) {
        // DEBUGLOG("inLoopThread");
        ADD_TO_EPOLL();
    }
    else {
        // DEBUGLOG("NotinLoopThread");
        auto cb = [this, event]() {
            ADD_TO_EPOLL();
        };
        addTask(cb, true);
    }
}

void Eventloop::deleteEpollEvent(FdEvent* event) {
    if(isInLoopThread()) {
        DELETE_TO_EPOLL();
    }
    else {
        auto cb = [this, event]() {
            DELETE_TO_EPOLL();
        };
        addTask(cb, true);
    }
}

void Eventloop::addTask(std::function<void()> cb, bool is_wake_up /*= false*/) {
    ScopeMutex<Mutex> lock(m_mutex);
    m_pending_tasks.push(cb);
    lock.unlock();
    if(is_wake_up)
        wakeup();
}

bool Eventloop::isInLoopThread() {
    return getThreadId() == m_thread_id;
}


Eventloop* Eventloop::GetCurrentEventloop() {
    if(t_current_eventloop) {
        return t_current_eventloop;
    }

    t_current_eventloop = new Eventloop();
    return t_current_eventloop;
}

bool Eventloop::isLooping() {
    return m_is_looping;
}

}