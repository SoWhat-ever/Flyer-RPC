#ifndef FLYER_NET_TIMER_H
#define FLYER_NET_TIMER_H

#include "Flyer/net/fd_event.h"
#include "Flyer/net/time_event.h"
#include "mutex.h"
#include <map>

namespace Flyer {

class Timer : public FdEvent {
public:
    Timer();

    ~Timer();

    void addTimerEvent(TimerEvent::s_ptr event);

    void deleteTimerEvent(TimerEvent::s_ptr event);

    void onTimer();

private:
    void resetArriveTime();

private:
    std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;

    Mutex m_mutex;
};

}

#endif