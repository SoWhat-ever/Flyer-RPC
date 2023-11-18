#include <iostream>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

#include "Flyer/common/log.h"
#include "Flyer/common/config.h"
#include "Flyer/net/eventloop.h"
#include "Flyer/net/time_event.h"
#include "Flyer/net/io_thread.h"
#include "Flyer/net/io_thread_group.h"

int main() {
    std::cout << "Test Start...\n";

    Flyer::Config::SetGlobalConfig("/home/os/Desktop/project/config/flyer.xml");
    
    Flyer::Logger::InitGlobalLogger();

    // Flyer::Eventloop* eventloop = new Flyer::Eventloop();

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1) {
        ERRORLOG("listenfd = -1");
        exit(0);
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_port = htons(12345);
    addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &addr.sin_addr);

    int rt = bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if(rt != 0) {
        ERRORLOG("bind error");
        exit(1);
    }

    rt = listen(listenfd, 100);
    if(rt != 0) {
        ERRORLOG("listen error");
        exit(1);
    }

    Flyer::FdEvent event(listenfd);
    event.listen(Flyer::FdEvent::IN_EVENT, [listenfd](){
        sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, sizeof(peer_addr));
        int clientfd = accept(listenfd, reinterpret_cast<sockaddr*>(&peer_addr), &addr_len);
        
        inet_ntoa(peer_addr.sin_addr);
        DEBUGLOG("success get client fd[%d], peer_addr[%s:%d]", clientfd, inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));

    });
    /* # # # # # 
    // TEST eventloop
    // eventloop->addEpollEvent(&event);
    # # # # # */

    int i = 1;
    Flyer::TimerEvent::s_ptr timer_event = std::make_shared<Flyer::TimerEvent> (
        2000, true, [&i]() {
            DEBUGLOG("trigger timer event, count = %d", i++);
        }
    );
    /* # # # # #
    // TEST TimerEvent
    // eventloop->addTimerEvent(timer_event);
    # # # # # */

    // eventloop->loop();

    /* # # # # #
    // TEST IOThread
    Flyer::IOThread io_thread;
    io_thread.getEventloop()->addEpollEvent(&event);
    io_thread.getEventloop()->addTimerEvent(timer_event);

    io_thread.start();
    io_thread.join();
    # # # # # */

    Flyer::IOThreadGroup io_thread_group(2);
    Flyer::IOThread* io_thread = io_thread_group.getIOThread();
    io_thread->getEventloop()->addEpollEvent(&event);
    io_thread->getEventloop()->addTimerEvent(timer_event);

    Flyer::IOThread* io_thread2 = io_thread_group.getIOThread();
    io_thread2->getEventloop()->addTimerEvent(timer_event);

    io_thread_group.start();
    io_thread_group.join();
    return 0;
}