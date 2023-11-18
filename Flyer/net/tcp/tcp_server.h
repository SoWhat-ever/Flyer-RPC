#ifndef FLYER_NET_TCP_SERVER_H
#define FLYER_NET_TCP_SERVER_H

#include "Flyer/net/tcp/tcp_acceptor.h"
#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/eventloop.h"
#include "Flyer/net/io_thread_group.h"
#include "Flyer/net/tcp/tcp_connection.h"

namespace Flyer {

class TcpServer {
public:
    TcpServer(NetAddr::s_ptr local_addr);

    ~TcpServer();

    void start();

private:
    void init();

    // work when new client connect
    void onAccept();

private:
    TcpAcceptor::s_ptr m_acceptor;

    // local listen addr
    NetAddr::s_ptr m_local_addr;

    // mainReactor
    Eventloop* m_main_eventloop {NULL};

    // subReactors
    IOThreadGroup* m_io_thread_group {NULL};

    FdEvent* m_listen_fd_event;

    int m_client_counts {0};

    std::set<TcpConnection::s_ptr> m_clients;
};

}
#endif