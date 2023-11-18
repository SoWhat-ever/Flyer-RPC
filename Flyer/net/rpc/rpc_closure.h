#ifndef FLYER_NET_RPC_RPC_CLOSUER_H
#define FLYER_NET_RPC_RPC_CLOSUER_H

#include <google/protobuf/stubs/callback.h>

#include <functional>

namespace Flyer {

class RpcClosure : public google::protobuf::Closure{
public:
    RpcClosure(std::function<void()> cb) : m_cb(cb) {}

    void Run() override {
        if(m_cb != NULL) {
            m_cb();
        }
    }

private:
    std::function<void()> m_cb {nullptr};
};

}

#endif