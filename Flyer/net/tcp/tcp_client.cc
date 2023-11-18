#include "Flyer/net/tcp/tcp_client.h"
#include "Flyer/common/log.h"
#include "Flyer/net/fd_event_group.h"
#include "Flyer/common/error_code.h"
#include "Flyer/net/tcp/net_addr.h"

#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

namespace Flyer {

TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    // DEBUGLOG(" TcpClient::TcpClient, peer_addr[%s]", m_peer_addr->toString().c_str());

    m_event_loop = Eventloop::GetCurrentEventloop();
     
    m_fd = socket(peer_addr->getFamily(), SOCK_STREAM, 0);

    if(m_fd < 0) {
        ERRORLOG("TcpClient::TcpClient() error, failed to create fd");
    }

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlock();

    m_connection = std::make_shared<TcpConnection>(m_event_loop, m_fd, 128, nullptr, peer_addr, TcpConnectionByClient);
    m_connection->setConnectionType(TcpConnectionByClient);
 }

TcpClient::~TcpClient() {
    if(m_fd > 0) {
    close(m_fd);
    }
    DEBUGLOG("TcpClient::~TcpClient");
}

// Asynchronous connect
// success connet, run done
void TcpClient::connect(std::function<void()> done) {
    int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
    // DEBUGLOG("Tcpcient:connect rt = %d", rt);
    if(rt == 0) {
        DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
        m_connection->setState(Connected);
        initLocalAddr();
        if(done)
            done();
    }
    else if(rt == -1) {
        // epoll listen writeable event, then judge error number
        if(errno == EINPROGRESS) {
            m_fd_event->listen(FdEvent::OUT_EVENT, [this, done]() {
                int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
                if ((rt < 0 && errno == EISCONN) || (rt == 0)) {
                    DEBUGLOG("connect [%s] sussess", m_peer_addr->toString().c_str());
                    initLocalAddr();
                    m_connection->setState(Connected);
                } 
                else {
                    if (errno == ECONNREFUSED) {
                        m_connect_err_code = ERROR_PEER_CLOSED;
                        m_connect_err_info = "connect refused, sys error = " + std::string(strerror(errno));
                    } 
                    else {
                        m_connect_err_code = ERROR_FAILED_CONNECT;
                        m_connect_err_info = "connect unkonwn error, sys error = " + std::string(strerror(errno));
                    }
                    ERRORLOG("connect errror, errno=%d, error=%s", errno, strerror(errno));
                    close(m_fd);
                    m_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);
                }

                m_event_loop->deleteEpollEvent(m_fd_event);
                if(done) {
                    done();
                }
            });

            m_event_loop->addEpollEvent(m_fd_event);

            if(!m_event_loop->isLooping()) {
                m_event_loop->loop();
            }
        }
        else {
            m_connect_err_code = ERROR_FAILED_CONNECT;
            m_connect_err_info = "connect error, sys error = " + std::string(strerror(errno));
            ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
            if(done) {
                done();
            }
        }
    }
}

// Asynchronously send message
// success send message, run done which pass in message object
void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
    // 1. write message to connection out buffer, write done to buffer too
    // 2. start connection readable 
    m_connection->pushSendMassage(message, done);
    m_connection->listenWrite();
}

// Asynchronously read message
// success read message, run done which pass in message object
void TcpClient::readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    // 1. listen readabel event
    // 2. decode message object from buffer
    // 3. judge msg_id; if equal, success to read, then run callback
    m_connection->pushReadMassage(msg_id, done);
    m_connection->listenRead();
}

void TcpClient::stop() {
    if(m_event_loop->isLooping()) {
        m_event_loop->stop();
    }
}

int TcpClient::getConnectErrorCode() {
    return m_connect_err_code;
}

std::string TcpClient::getConnectErrorInfo() {
    return m_connect_err_info;
}

NetAddr::s_ptr TcpClient::getPeerAddr() {
    return m_peer_addr;
}

NetAddr::s_ptr TcpClient::getLocalAddr() {
    return m_local_addr;
}

void TcpClient::initLocalAddr() {
    sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);

    int ret = getsockname(m_fd, reinterpret_cast<sockaddr*>(&local_addr), &len);
    if (ret != 0) {
        ERRORLOG("initLocalAddr error, getsockname error. errno=%d, error=%s", errno, strerror(errno));
        return;
    }

    m_local_addr = std::make_shared<IPNetAddr>(local_addr);
}

void TcpClient::addTimerEvent(TimerEvent::s_ptr timer_event) {
    m_event_loop->addTimerEvent(timer_event);
}


}