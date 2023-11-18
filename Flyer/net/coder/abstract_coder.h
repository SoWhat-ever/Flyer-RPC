#ifndef FLYER_NET_ABSTRACT_CODER_H
#define FLYER_NET_ABSTRACT_CODER_H

#include "Flyer/net/tcp/tcp_buffer.h"
#include "Flyer/net/coder/abstract_protocol.h"

#include <vector>

namespace Flyer {

class AbstractCoder {
public:
    // message obj to stream, then write to buffer
    virtual void encode(std::vector<AbstractProtocol::s_ptr>& messsage, TcpBuffer::s_ptr out_buffer) = 0;

    // stream in buffer to message obj
    virtual void decode(std::vector<AbstractProtocol::s_ptr>& out_messsage, TcpBuffer::s_ptr in_buffer) = 0;

    virtual ~AbstractCoder() {}
};

}

#endif