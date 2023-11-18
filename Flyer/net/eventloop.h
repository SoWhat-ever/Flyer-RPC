#ifndef FLYER_NET_EVENTLOOP_H
#define FLYER_NET_EVENTLOOP_H

#include <pthread.h>
#include <set>
#include <functional>
#include <queue>
#include <mutex.h>
#include <fd_event.h>
#include "Flyer/net/wakeup_fdEvent.h"
#include "Flyer/net/timer.h"

namespace Flyer {

class Eventloop {
public:
    Eventloop();

    ~Eventloop();

    void loop();

    void wakeup();

    void stop();

    void addEpollEvent(FdEvent* event);

    void deleteEpollEvent(FdEvent* event);

    bool isInLoopThread();

    void addTask(std::function<void()> cb, bool is_wake_up = false);

    void addTimerEvent(TimerEvent::s_ptr event);

    bool isLooping();

public:
    static Eventloop* GetCurrentEventloop();

private:
    void dealWakeup();

    void initWakeupEvent();

    void initTimer();

private:
    pid_t m_thread_id {0};

    int m_epool_fd {0};

    int m_wakeup_fd {0};

    WakeupFdEvent* m_wakeup_fd_event {NULL};

    bool m_stop_flag {false};

    std::set<int> m_listen_fds;

    std::queue<std::function<void()>> m_pending_tasks;

    Mutex m_mutex;

    Timer* m_timer {NULL}; 

    bool m_is_looping {false};
};

}


#endif