#include "Flyer/common/log.h"
#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/tcp/tcp_server.h"
#include "Flyer/net/tcp/tcp_connection.h"
#include "Flyer/net/tcp/tcp_client.h"
#include "Flyer/net/coder/string_coder.h"
#include "Flyer/net/coder/abstract_protocol.h"
#include "Flyer/net/coder/tinypb_coder.h"
#include "Flyer/net/coder/tinypb_protocol.h"
#include "Flyer/net/rpc/rpc_controller.h"
#include "Flyer/net/rpc/rpc_closure.h"
#include "Flyer/net/rpc/rpc_channel.h"
#include "order.pb.h"


#include <iostream>
#include <memory>
#include <string.h>
#include <unistd.h>

void rpc_client() {
    // Flyer::NetAddr::s_ptr addr = std::make_shared<Flyer::IPNetAddr>("127.0.0.1", 12345);
    // std::shared_ptr<Flyer::RpcChannel> channel = std::make_shared<Flyer::RpcChannel>(addr);
    NEWRPCCHANNEL("127.0.0.1:12345", channel);

    // std::shared_ptr<makeOrderRequest> request = std::make_shared<makeOrderRequest>();
    NEWMESSAGE(makeOrderRequest, request);
    // std::shared_ptr<makeOrderResponse> response = std::make_shared<makeOrderResponse>();
    NEWMESSAGE(makeOrderResponse, response);

    request->set_price(50);
    request->set_goods("IPhone");

    // std::shared_ptr<Flyer::RpcController> controller = std::make_shared<Flyer::RpcController>();
    NEWRPCCONTROLLER(controller);
    controller->setMsgId("99998888");
    // controller->setTimeout(10000);

    std::shared_ptr<Flyer::RpcClosure> closure = std::make_shared<Flyer::RpcClosure>([request, response, channel, controller]() mutable {
        if(controller->getErrorCode() == 0) {
            INFOLOG("call rpc success, req[%s], rsp[%s]", 
            request->ShortDebugString().c_str(), 
            response->ShortDebugString().c_str());

            // 执行业务逻辑
            if (response->order_id() == "xxx") {
                // xx
            }

        }
        else {
            ERRORLOG("call rpc failed, request[%s], error code[%d], error info[%s]", 
            request->ShortDebugString().c_str(), 
            controller->getErrorCode(), 
            controller->getErrorInfo().c_str());
        }

        INFOLOG("now exit eventloop");
        // channel->getTcpClient()->stop();
        channel.reset();
    });
    
    // channel->Init(controller, request, response, closure);

    // OrderService_Stub stub(channel.get());

    // stub.makeOrder(controller.get(), request.get(), response.get(), closure.get());

    CALLRPRC("127.0.0.1:12345", OrderService_Stub, makeOrder, controller, request, response, closure);

}


int main() {
    std::cout << "Test Start...\n";

    Flyer::Config::SetGlobalConfig(NULL);
    
    Flyer::Logger::InitGlobalLogger(0);

    rpc_client();
}