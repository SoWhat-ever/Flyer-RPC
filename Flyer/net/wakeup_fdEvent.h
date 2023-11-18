#ifndef FLYER_NET_WAKEUP_EVENT_H
#define FLYER_NET_WAKEUP_EVENT_H

#include "Flyer/net/fd_event.h"

namespace Flyer {

class WakeupFdEvent : public FdEvent {
public:
    WakeupFdEvent(int fd);

    ~WakeupFdEvent();

    void wakeup();

private:

};

}

#endif