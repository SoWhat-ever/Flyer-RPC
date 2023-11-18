#include "Flyer/net/rpc/rpc_dispatcher.h"
#include "Flyer/common/log.h"
#include "Flyer/common/error_code.h"
#include "Flyer/net/rpc/rpc_controller.h"
#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/tcp/tcp_connection.h"
#include "Flyer/common/run_time.h"

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>


namespace Flyer {

#define DELETE_RESOURCE(XX) \
    if(XX != NULL) {    \
        delete XX;  \
        XX = NULL;  \
    }

static RpcDispatcher* g_rpc_dispatcher = NULL;

RpcDispatcher* RpcDispatcher::GetRpcDispatcher() {
    if(g_rpc_dispatcher != NULL) {
        return g_rpc_dispatcher;
    }
    g_rpc_dispatcher = new RpcDispatcher;
    return g_rpc_dispatcher;
}

void RpcDispatcher::dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection) {
    std::shared_ptr<TinyPBProtocol> req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
    std::shared_ptr<TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);

    std::string method_full_name = req_protocol->m_method_name;
    std::string service_name;
    std::string method_name;

    rsp_protocol->m_msg_id = req_protocol->m_msg_id;
    rsp_protocol->m_method_name = req_protocol->m_method_name;

    if(!parseServiceFullName(method_full_name, service_name, method_name)) {
        setTinyPBError(rsp_protocol, ERROR_PARSE_SERVICE_NAME, "parse service name error");
        return;
    }

    auto it = m_service_map.find(service_name);
    if(it == m_service_map.end()) {
        ERRORLOG("msg_id[%s] | service name[%s] not found", req_protocol->m_msg_id.c_str(), service_name.c_str());
        setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "service not found");
        return;
    }

    service_s_ptr service = (*it).second;
    const google::protobuf::MethodDescriptor* method= service->GetDescriptor()->FindMethodByName(method_name);
    if(method == NULL) {
        ERRORLOG("msg_id[%s] | method name[%s] not found in service [%s]", req_protocol->m_msg_id.c_str(), method_name.c_str(), service_name.c_str());
        setTinyPBError(rsp_protocol, ERROR_METHOD_NOT_FOUND, "method not found");
        return;
    }

    google::protobuf::Message* req_msg = service->GetResponsePrototype(method).New();

    // deserialize pd_data to req_msg
    if(!req_msg->ParseFromString(req_protocol->m_pb_data)) {
        ERRORLOG("msg_id[%s] | deserialize error", req_protocol->m_msg_id.c_str());
        setTinyPBError(rsp_protocol, ERROR_FAILED_DESERIALIZE, "deserialize error");
        DELETE_RESOURCE(req_msg);
        return;
    }
    INFOLOG("msg_id[%s] | get rpc request [%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str());

    google::protobuf::Message* rsp_msg = service->GetResponsePrototype(method).New();

    RpcController rpcController;
    rpcController.setLocalAddr(connection->getLocalAddr());
    rpcController.setPeerAddr(connection->getPeerAddr());
    rpcController.setMsgId(req_protocol->m_msg_id);

    RunTime::GetRunTime()->m_msgid = req_protocol->m_msg_id;
    RunTime::GetRunTime()->m_method_name = method_name;
    
    service->CallMethod(method, &rpcController, req_msg, rsp_msg, NULL);

    if(!rsp_msg->SerializeToString(&rsp_protocol->m_pb_data)) {
        ERRORLOG("msg_id[%s] | serialize error, origin message [%s]", req_protocol->m_msg_id.c_str(), rsp_msg->ShortDebugString().c_str());
        setTinyPBError(rsp_protocol, ERROR_FAILED_SERIALIZE, "serialize error");
        DELETE_RESOURCE(req_msg);
        DELETE_RESOURCE(rsp_msg);
        return;
    }

    rsp_protocol->m_err_code = 0;
    INFOLOG("msg_id[%s] | dispatch success, request[%s], response[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str());
    
    DELETE_RESOURCE(req_msg);
    DELETE_RESOURCE(rsp_msg);
}

void RpcDispatcher::setTinyPBError(std::shared_ptr<TinyPBProtocol> msg, int32_t err_code, const std::string err_info) {
    msg->m_err_code = err_code;
    msg->m_err_info = err_info;
    msg->m_err_info_len = err_info.length();
}


bool RpcDispatcher::parseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name) {
    // parse: OrderService.make_order
    if(full_name.empty()) {
        ERRORLOG("full name is empty")
        return false;
    }
    
    size_t i = full_name.find_first_of(".");
    if(i == full_name.npos) {
        ERRORLOG("not find . in full name [%s]", full_name.c_str());
        return false;
    }
    service_name = full_name.substr(0, i);
    method_name = full_name.substr(i + 1, full_name.length() - i - 1);
    INFOLOG("parse server_name [%s] and method_name [%s] from full_name [%s]", service_name.c_str(), method_name.c_str(), full_name.c_str());
    return true;
}


void RpcDispatcher::registerService(service_s_ptr service) {
    std::string service_name = service->GetDescriptor()->full_name();
    m_service_map[service_name] = service;
}

}