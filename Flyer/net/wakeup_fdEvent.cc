#include "Flyer/net/wakeup_fdEvent.h"
#include "Flyer/common/log.h"
#include <unistd.h>

namespace Flyer {

WakeupFdEvent::WakeupFdEvent(int fd) : FdEvent(fd) {
    
}

WakeupFdEvent::~WakeupFdEvent() {

}

void WakeupFdEvent::wakeup() {
    char buf[8] = {'a'};

    int rt = write(m_fd, buf, 8);

    if(rt != 8)
        ERRORLOG("Write to up fd less than 8 bytes, fd[%d]", m_fd);
}   

}