#ifndef FLYER_NET_TCP_TCP_CLIENT
#define FLYER_NET_TCP_TCP_CLIENT

#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/eventloop.h"
#include "Flyer/net/tcp/tcp_connection.h"
#include "Flyer/net/coder/abstract_protocol.h"
#include "Flyer/net/fd_event.h"
#include "Flyer/net/time_event.h"

#include <memory>

namespace Flyer {

class TcpClient {
public:
    typedef std::shared_ptr<TcpClient> s_ptr;

    TcpClient(NetAddr::s_ptr peer_addr);

    ~TcpClient();

    // Asynchronous connect
    // success connet, run done
    void connect(std::function<void()> done);

    // Asynchronously send message
    // success send message, run done which pass in message object
    void writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    // Asynchronously read message
    // success read message, run done which pass in message object
    void readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    void stop();

    int getConnectErrorCode();
    std::string getConnectErrorInfo();

    NetAddr::s_ptr getPeerAddr();
    NetAddr::s_ptr getLocalAddr();

    void initLocalAddr();

    void addTimerEvent(TimerEvent::s_ptr timer_event);

    
private:
    NetAddr::s_ptr m_peer_addr;

    NetAddr::s_ptr m_local_addr;

    Eventloop* m_event_loop {NULL};

    FdEvent* m_fd_event {NULL};

    int m_fd {-1};

    TcpConnection::s_ptr m_connection;

    int m_connect_err_code {0};
    std::string m_connect_err_info;
};

}

#endif