#ifndef FLYER_NET_TCP_TCP_CONNECTION_H
#define FLYER_NET_TCP_TCP_CONNECTION_H

#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/tcp/tcp_buffer.h"
#include "Flyer/net/io_thread.h"
#include "Flyer/net/coder/abstract_protocol.h"
#include "Flyer/net/coder/abstract_coder.h"
#include "Flyer/net/rpc/rpc_dispatcher.h"

#include <memory>
#include <vector>

namespace Flyer {

enum TcpState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
    Closed = 4,
};

enum TcpConnectionType {
    TcpConnectionByServer = 1,
    TcpConnectionByClient = 2,
};


class TcpConnection {
public:
    typedef std::shared_ptr<TcpConnection> s_ptr;

    TcpConnection(Eventloop* event_loop, int fd, int buffer_size, NetAddr::s_ptr local_addr,
        NetAddr::s_ptr peer_addr, TcpConnectionType type = TcpConnectionByServer);

    ~TcpConnection();

    void onRead();

    void excute();

    void onWrite();

    void setState(const TcpState state);

    TcpState getState();

    void clear();

    // server close connection
    void shutdown();

    void setConnectionType(TcpConnectionType type);

    // start to listen readable event
    void listenRead();
    // start to listen writeable event
    void listenWrite();

    void pushSendMassage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);
    
    void pushReadMassage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    NetAddr::s_ptr getLocalAddr();
    NetAddr::s_ptr getPeerAddr();

private:
    // the IOThread which hold this connection 
    // IOThread* m_io_thread {NULL};
    Eventloop* m_event_loop {NULL};

    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    TcpBuffer::s_ptr m_in_buffer;
    TcpBuffer::s_ptr m_out_buffer;

    TcpState m_state;

    FdEvent* m_fd_event;

    int m_fd {0};

    TcpConnectionType m_connection_type {TcpConnectionByServer};

    AbstractCoder* m_coder {NULL};

    // vector <pair <message, write_done>> write_dones
    std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)> >> m_write_dones;
    // map <key = msg_id, read_done>
    std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_dones;

};
}


#endif