#include "Flyer/net/tcp/tcp_server.h"
#include "Flyer/net/eventloop.h"
#include "Flyer/common/log.h"
#include "Flyer/net/tcp/tcp_connection.h"
#include "Flyer/common/config.h"

namespace Flyer {

TcpServer::TcpServer(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {
   init();

    INFOLOG("flyer TcpServer listen success on [%s]", m_local_addr->toString().c_str());

}

TcpServer::~TcpServer() {
    if(m_main_eventloop) {
        delete m_main_eventloop;
        m_main_eventloop = NULL;
    }
}

void TcpServer::init() {
    m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr);

    m_main_eventloop = Eventloop::GetCurrentEventloop();

    m_io_thread_group = new IOThreadGroup(Config::GetGlobalConfig()->m_io_threads);

    m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
    m_listen_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpServer::onAccept, this));

    m_main_eventloop->addEpollEvent(m_listen_fd_event);
}

void TcpServer::onAccept() {
    auto re = m_acceptor->accept();
    int client_fd = re.first;
    NetAddr::s_ptr peer_addr = re.second;
    
    m_client_counts++;

    // add clienfd to one IOThread of IOThreadGroup
    IOThread* io_thread = m_io_thread_group->getIOThread();
    TcpConnection::s_ptr connection = std::make_shared<TcpConnection>(io_thread->getEventloop(), client_fd, 128, peer_addr, m_local_addr);
    connection->setState(Connected);
    m_clients.insert(connection);
    INFOLOG("TcpServer succ get client, fd=%d", client_fd);
}

void TcpServer::start() {
    m_io_thread_group->start();
    m_main_eventloop->loop();
}

}