#include "Flyer/net/tcp/tcp_connection.h"
#include "Flyer/common/log.h"
#include "Flyer/net/fd_event_group.h"
#include "Flyer/net/coder/string_coder.h"
#include "Flyer/net/coder/tinypb_protocol.h"
#include "Flyer/net/coder/tinypb_coder.h"

#include <unistd.h>

namespace Flyer {

TcpConnection::TcpConnection(Eventloop* event_loop, int fd, int buffer_size, NetAddr::s_ptr local_addr,  
    NetAddr::s_ptr peer_addr, TcpConnectionType type /*= TcpConnectionByServer*/) 
    : m_event_loop(event_loop), m_local_addr(local_addr), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd), m_connection_type(type) {

    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);    
    m_fd_event->setNonBlock();

    if(m_connection_type == TcpConnectionByServer) {
        listenRead();
    }

    m_coder = new TinyPBCoder();
}

TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection");
    if(m_coder) {
        delete m_coder;
        m_coder = NULL;
    }
}

void TcpConnection::onRead() {
    // 1. From socket buffer, use system read to read stream to in_buffer
    if(m_state != Connected){
        ERRORLOG("onRead error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    bool is_read_all = false;
    bool is_close = false;
    while(!is_read_all) {
        if(m_in_buffer->writeAble() == 0)
            m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());

        int read_count = m_in_buffer->writeAble();
        int write_index = m_in_buffer->writeIndex();

        int rt = read(m_fd_event->getFd(), &(m_in_buffer->m_buffer[write_index]), read_count);
        DEBUGLOG("success read %d bytes from addr[%s], client fd[%d]", rt, m_peer_addr->toString().c_str(), m_fd);
        if(rt > 0) {
            m_in_buffer->moveWriteIndex(rt);

            if(rt == read_count) {
                continue;
            }
            else if(rt < read_count) {
                is_read_all = true;
                break;
            }
        }
        else if(rt == 0) {
            is_close = true;
            break;
        }
        else if(rt == -1 && errno == EAGAIN) {
            is_read_all = true;
            break;
        }
    }

    if(is_close) {
        INFOLOG("peer closed, peer addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        clear();
        return;
    }

    if(!is_read_all) {
        ERRORLOG("not read all data");
    }

    excute();
}

void TcpConnection::excute() {
    if(m_connection_type == TcpConnectionByServer) {
        // std::vector<char> tmp;
        // int size = m_in_buffer->readAble();
        // tmp.resize(size);
        // m_in_buffer->readFromBuffer(tmp, size);

        // std::string msg;
        // for(size_t i = 0; i < tmp.size(); i++) {
        //     msg += tmp[i];
        // }

        // echo 
        // m_out_buffer->writeToBuffer(msg.c_str(), msg.length());
        
        std::vector<AbstractProtocol::s_ptr> get_messages;
        std::vector<AbstractProtocol::s_ptr> reply_messages;
        m_coder->decode(get_messages, m_in_buffer);
        for(size_t i = 0; i < get_messages.size(); i++) {
            INFOLOG("success get request [%s] from client[%s]", get_messages[i]->m_msg_id.c_str(),  m_peer_addr->toString().c_str());

            // 1. for each request, call RPC function and get a message
            // 2. write each message to out_buffer, and listen writeable event to send out 
            std::shared_ptr<TinyPBProtocol> reply_message = std::make_shared<TinyPBProtocol>();
            RpcDispatcher::GetRpcDispatcher()->dispatch(get_messages[i], reply_message, this);
            reply_messages.emplace_back(reply_message);
        }
        m_coder->encode(reply_messages, m_out_buffer);

        listenWrite();
    }
    else {
        // Client: decode from buffer to get message, and run matched callback
        std::vector<AbstractProtocol::s_ptr> result;
        m_coder->decode(result, m_in_buffer);

        for(size_t i = 0; i < result.size(); i++) {
            std::string msg_id = result[i]->m_msg_id;
            auto it = m_read_dones.find(msg_id);
            if(it != m_read_dones.end()) {
                it->second(result[i]);
            }
        }
    }

    
}

void TcpConnection::onWrite() {
    // send all data in out_buffer to client
    if(m_state != Connected) {
        ERRORLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }
    
    if(m_connection_type == TcpConnectionByClient) {
        // 1. encode message
        // 2. write to buffer
        std::vector<AbstractProtocol::s_ptr> messages;
        for(size_t i = 0; i < m_write_dones.size(); i++) {
            messages.push_back(m_write_dones[i].first);
        }
        m_coder->encode(messages, m_out_buffer);
    }

    bool is_write_all = false;
    while(true) {
        if(m_out_buffer->readAble() == 0) {
            is_write_all = true;
            DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
            break;
        }
        
        int write_size = m_out_buffer->readAble();
        int read_index = m_out_buffer->readIndex();
        int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

        if(rt >= write_size) {
            is_write_all = true;
            DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
            break;
        }
        if(rt == -1 && errno == EAGAIN) {
            // send buffer is full, cannot send; wait fd next writeadle
            ERRORLOG("write data error, errno=EAGAIN and rt = -1");
            break;
        }
    }

    if(is_write_all) {
        m_fd_event->cancle(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_fd_event);
    }

    if(m_connection_type == TcpConnectionByClient) {
        DEBUGLOG("write all, start to run done");
        for(size_t i = 0; i < m_write_dones.size(); i++) {
            m_write_dones[i].second(m_write_dones[i].first);
        }
        m_write_dones.clear();
    }
    
}

void TcpConnection::setState(const TcpState state) {
    m_state = Connected;
}

TcpState TcpConnection::getState() {
    return m_state;
}

void TcpConnection::clear() {
    // clear after connection closed
    if(m_state == Closed) {
        return;
    }

    m_fd_event->cancle(FdEvent::IN_EVENT);
    m_fd_event->cancle(FdEvent::OUT_EVENT);

    m_event_loop->deleteEpollEvent(m_fd_event);

    m_state = Closed;
}

void TcpConnection::shutdown() {
    if(m_state == Closed || m_state == NotConnected)
        return;
    
    m_state = HalfClosing;

    // HalfClosing state : send FIN, 1st progess in 4 wave bye
    // Closed: fd trigger readable event, but readable data is 0 which means client send FIN too
    ::shutdown(m_fd, SHUT_RDWR);
}

void TcpConnection::setConnectionType(TcpConnectionType type) {
    m_connection_type = type;
}

void TcpConnection::listenWrite() {
    m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));

    m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::listenRead() {
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));

    m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::pushSendMassage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
    m_write_dones.push_back(std::make_pair(message, done));
}

void TcpConnection::pushReadMassage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    m_read_dones.insert(std::make_pair(msg_id, done));
}

NetAddr::s_ptr TcpConnection::getLocalAddr() {
    return m_local_addr;
}
NetAddr::s_ptr TcpConnection::getPeerAddr() {
    return m_peer_addr;
}


}