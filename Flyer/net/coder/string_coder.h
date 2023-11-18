#ifndef FLYER_NET_STRING_CODER_H
#define FLYER_NET_STRING_CODER_H

#include "Flyer/net/coder/abstract_coder.h"
#include "Flyer/net/coder/abstract_protocol.h"

namespace Flyer {


class StringProtocol : public AbstractProtocol{
public:
    std::string info;
};

class StringCoder : public AbstractCoder {

    // message obj to stream, then write to buffer
    void encode(std::vector<AbstractProtocol::s_ptr>& messsages, TcpBuffer::s_ptr out_buffer) {
        for(size_t i = 0; i < messsages.size(); i++) {
            std::shared_ptr<StringProtocol> msg = std::dynamic_pointer_cast<StringProtocol>(messsages[i]);
            out_buffer->writeToBuffer(msg->info.c_str(), msg->info.length());
        }
    }

    // stream in buffer to message obj
    void decode(std::vector<AbstractProtocol::s_ptr>& out_messsage, TcpBuffer::s_ptr in_buffer)  {
        std::vector<char> re;
        in_buffer->readFromBuffer(re, in_buffer->readAble());

        std::string info;
        for(size_t i = 0; i < re.size(); i++) {
            info += re[i];
        }

        std::shared_ptr<StringProtocol> msg  = std::make_shared<StringProtocol>();
        msg->info = info;
        msg->m_msg_id = "123456";
        out_messsage.push_back(msg);
    }
};

}

#endif