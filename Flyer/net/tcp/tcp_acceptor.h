#ifndef FLYER_NET_TCP_TCP_ACCEPTOR_H
#define FLYER_NET_TCP_TCP_ACCEPTOR_H

#include <memory>
#include "Flyer/net/tcp/net_addr.h"

namespace Flyer {

class TcpAcceptor {
public:
    typedef std::shared_ptr<TcpAcceptor> s_ptr;

    TcpAcceptor(NetAddr::s_ptr local_addr);

    ~TcpAcceptor();

    std::pair<int, NetAddr::s_ptr> accept();

    int getListenFd() {
        return m_listenfd;
    }

private:
    // server listen addr -- ip :port
    NetAddr::s_ptr m_local_addr;
    
    int m_family {-1};

    // listen socket
    int m_listenfd {-1};  
};

}

#endif