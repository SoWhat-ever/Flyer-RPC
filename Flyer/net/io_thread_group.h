#ifndef FLYER_NET_IO_THREAD_GROUP_H
#define FLYER_NET_IO_THREAD_GROUP_H

#include <vector>
#include "Flyer/common/log.h"
#include "Flyer/net/io_thread.h"

namespace Flyer {

class IOThreadGroup {
public:
    IOThreadGroup(size_t size);

    ~IOThreadGroup();

    void start();

    void join();

    IOThread* getIOThread();

private:
    size_t m_size {0};

    std::vector<IOThread*> m_io_thread_group;

    size_t m_index {0};
};

}


#endif