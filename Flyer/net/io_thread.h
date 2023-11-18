#ifndef FLYER_NET_IO_THREAD_H
#define FLYER_NET_IO_THREAD_H

#include <semaphore.h>
#include "Flyer/net/eventloop.h"

namespace Flyer {

class IOThread {
public:
    IOThread();

    ~IOThread();

    void start();

    void join();

    Eventloop* getEventloop() {
        return m_event_loop;
    }

public:
    static void* Main(void* arg);

private:
    pid_t m_thread_id {-1};

    pthread_t m_thread {0};

    Eventloop* m_event_loop {NULL};

    sem_t m_init_semaphore;

    sem_t m_start_semaphore;
};

}

#endif