#ifndef FLYER_NET_ABSTRACT_PROTOCOL_H
#define FLYER_NET_ABSTRACT_PROTOCOL_H

#include "Flyer/net/tcp/tcp_buffer.h"

#include <memory>

namespace Flyer {

struct AbstractProtocol : public std::enable_shared_from_this<AbstractProtocol> {
public:
    typedef std::shared_ptr<AbstractProtocol> s_ptr;

    virtual ~AbstractProtocol() {}

public:
    // use id to match request and response
    std::string m_msg_id;

private:

};
}

#endif