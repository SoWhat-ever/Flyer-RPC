#include "Flyer/net/time_event.h"
#include "Flyer/common/log.h"
#include "Flyer/common/util.h"

namespace Flyer {

TimerEvent::TimerEvent(int interval, bool is_repeated, std::function<void()> task) 
    : m_interval(interval), m_is_repeated(is_repeated),  m_task(task) {
    resetArriveTime();
}

void TimerEvent::resetArriveTime() {
    m_arrive_time = getNowMs() + m_interval;
    DEBUGLOG("success create timer event, will excute at [%lld]", m_arrive_time);
}

}