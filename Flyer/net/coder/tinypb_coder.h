#ifndef FLYER_NET_CODER_TINYPB_CODER_H
#define FLYER_NET_CODER_TINYPB_CODER_H

#include "Flyer/net/coder/abstract_coder.h"
#include "Flyer/net/coder/tinypb_protocol.h"

namespace Flyer {

class TinyPBCoder : public AbstractCoder {
public:
    TinyPBCoder() {}
    ~TinyPBCoder() {}

    // message obj to stream, then write to buffer
    void encode(std::vector<AbstractProtocol::s_ptr>& messsage, TcpBuffer::s_ptr out_buffer);

    // stream in buffer to message obj
    void decode(std::vector<AbstractProtocol::s_ptr>& out_messsage, TcpBuffer::s_ptr in_buffer);
    
private:
    // message to stream (char*)
    const char* encodeTinyPb(std::shared_ptr<TinyPBProtocol> message, int& len);
};

}

#endif