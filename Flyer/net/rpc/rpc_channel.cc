#include "Flyer/net/rpc/rpc_channel.h"
#include "Flyer/net/coder/tinypb_protocol.h"
#include "Flyer/common/msg_id_util.h"
#include "Flyer/net/rpc/rpc_controller.h"
#include "Flyer/common/log.h"
#include "Flyer/net/tcp/tcp_client.h"
#include "Flyer/common/error_code.h"
#include "Flyer/net/time_event.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

namespace Flyer {

RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    m_client = std::make_shared<TcpClient>(m_peer_addr);
}

RpcChannel::~RpcChannel() {
    DEBUGLOG("~RpcChannel");
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method, google::protobuf::RpcController* controller, const google::protobuf::Message* request,
        google::protobuf::Message* response, google::protobuf::Closure* done) {
    std::shared_ptr<TinyPBProtocol> req_protocol = std::make_shared<TinyPBProtocol>();
    
    RpcController* my_controller = dynamic_cast<RpcController*>(controller);
    if(my_controller == NULL) {
        ERRORLOG("failed callmethod, RpcController convert error");
        return;
    } 

    if(my_controller->getMsgId().empty()) {
        req_protocol->m_msg_id = MsgIDUtil::GetMsgID();
        my_controller->setMsgId(req_protocol->m_msg_id);
    }
    else {
        req_protocol->m_msg_id = my_controller->getMsgId();
    }

    req_protocol->m_method_name = method->full_name();
    INFOLOG("msgId[%s] | call method name [%s]", req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str());

    if(!m_is_init) {
        std::string err_info = "RpcChannel not call init()";
        my_controller->setError(ERROR_RPC_CHANNEL_INIT, err_info);
        ERRORLOG("msgId[%s] | %s, RpcChannel not init ", req_protocol->m_msg_id.c_str(), err_info.c_str());
        return;
    }

    if(!request->SerializeToString(&(req_protocol->m_pb_data))) {
        std::string err_info = "fail to serialize";
        my_controller->setError(ERROR_FAILED_SERIALIZE, err_info);
        ERRORLOG("msgId[%s] | %s, origin requeset [%s] ", req_protocol->m_msg_id.c_str(), err_info.c_str(), request->ShortDebugString().c_str());
        return;
    }

    s_ptr channel = shared_from_this();

    m_timer_event = std::make_shared<TimerEvent>(my_controller->getTimeout(), false, [my_controller, channel]() mutable {
        my_controller->StartCancel();    
        my_controller->setError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout" + std::to_string(my_controller->getTimeout()));

        if(channel->getClosure()) {
            channel->getClosure()->Run();
        }
        channel.reset();
    });

    m_client->addTimerEvent(m_timer_event);

    m_client->connect([req_protocol, channel]() mutable {
        RpcController* my_controller = dynamic_cast<RpcController*>(channel->getController());

        if(channel->getTcpClient()->getConnectErrorCode()) {
            my_controller->setError(channel->getTcpClient()->getConnectErrorCode(), channel->getTcpClient()->getConnectErrorInfo());

            ERRORLOG("msgId[%s] | connect error, error code[%d], error info[%s], peer addr[%s]",  req_protocol->m_msg_id.c_str(), my_controller->getErrorCode(), 
                my_controller->getErrorInfo().c_str(), channel->getTcpClient()->getPeerAddr()->toString().c_str());

            return;
        }    
    
        channel->getTcpClient()->writeMessage(req_protocol, [req_protocol, channel, my_controller](AbstractProtocol::s_ptr) mutable {
            INFOLOG("msgId[%s] | send rpc request success. call method name[%s], peer addr[%s], local addr[%s]", 
                req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str(),
                channel->getTcpClient()->getPeerAddr()->toString().c_str(), channel->getTcpClient()->getLocalAddr()->toString().c_str());

            channel->getTcpClient()->readMessage(req_protocol->m_msg_id, [channel, my_controller](AbstractProtocol::s_ptr msg_ptr) mutable {
                std::shared_ptr<Flyer::TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<Flyer::TinyPBProtocol>(msg_ptr);
                INFOLOG("msg_id[%s], success get rpc response, call mehthod name[%s], peer addr[%s], local addr[%s]", 
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    channel->getTcpClient()->getPeerAddr()->toString().c_str(), channel->getTcpClient()->getLocalAddr()->toString().c_str());
                
                channel->getTimerEvent()->setCancled(true);

                if(!(channel->getResponse()->ParseFromString(rsp_protocol->m_pb_data))) {
                    my_controller->setError(ERROR_FAILED_DESERIALIZE, "deserialize error");
                    ERRORLOG("deserialize error");
                    return;
                }

                if(rsp_protocol->m_err_code != 0) {
                    ERRORLOG("%s | call rpc methood[%s] failed, error code[%d], error info[%s]", 
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    rsp_protocol->m_err_code, rsp_protocol->m_err_info.c_str());
                    my_controller->setError(rsp_protocol->m_err_code, rsp_protocol->m_err_info);
                    return;
                }

                INFOLOG("msg_id[%s] | call rpc success, call mehthod name[%s], peer addr[%s], local addr[%s]",
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    channel->getTcpClient()->getPeerAddr()->toString().c_str(), channel->getTcpClient()->getLocalAddr()->toString().c_str());

                if(!my_controller->IsCanceled() && channel->getClosure()) {
                    channel->getClosure()->Run();
                }

                channel.reset();
            });
        });
    });
}

void RpcChannel::Init(controller_s_ptr controller, message_s_ptr req_msg, message_s_ptr rsp_msg, closure_s_ptr done) {
    if(m_is_init) {
        return;
    }
    m_controller = controller;
    m_request = req_msg;
    m_response = rsp_msg;
    m_closure = done;
    m_is_init = true;
}

google::protobuf::RpcController* RpcChannel::getController() {
  return m_controller.get();
}

google::protobuf::Message* RpcChannel::getRequest() {
  return m_request.get();
}

google::protobuf::Message* RpcChannel::getResponse() {
  return m_response.get();
}

google::protobuf::Closure* RpcChannel::getClosure() {
  return m_closure.get();
}

TcpClient* RpcChannel::getTcpClient() {
  return m_client.get();
}

TimerEvent::s_ptr RpcChannel::getTimerEvent() {
    return m_timer_event;
}

}