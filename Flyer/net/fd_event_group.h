#ifndef FLYER_NET_FD_EVENT_GROUP_H
#define FLYER_NET_FD_EVENT_GROUP_H

#include <vector>
#include "Flyer/net/fd_event.h"
#include "Flyer/common/mutex.h"

namespace Flyer {

class FdEventGroup {
public:
    FdEventGroup(int size);

    ~FdEventGroup();

    FdEvent* getFdEvent(int fd);

public:
    static FdEventGroup* GetFdEventGroup();
    
private:
    int m_size {0};

    std::vector<FdEvent*> m_fd_group;

    Mutex m_mutex;
};
}

#endif