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


void test_tcp_client() {
    Flyer::NetAddr::s_ptr addr = std::make_shared<Flyer::IPNetAddr>("127.0.0.1", 12345);
    Flyer::TcpClient client(addr);
    client.connect([addr, &client](){
        DEBUGLOG("connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<Flyer::TinyPBProtocol> message = std::make_shared<Flyer::TinyPBProtocol>();
        
        message->m_msg_id = "9999";
        // message->m_pb_data = "Client send response : Hello to server from client";
        
        makeOrderRequest request;
        request.set_price(200);
        request.set_goods("IPhone");
        
        if(!request.SerializeToString(&(message->m_pb_data))) {
            ERRORLOG("serilize error");
            return;
        }

        message->m_method_name = "OrderService.makeOrder";

        client.writeMessage(message, [request](Flyer::AbstractProtocol::s_ptr msg_ptr){
            DEBUGLOG("send message success, request[%s]", request.ShortDebugString().c_str());
        });
        
        client.readMessage("9999", [](Flyer::AbstractProtocol::s_ptr msg_ptr){
            std::shared_ptr<Flyer::TinyPBProtocol> message = std::dynamic_pointer_cast<Flyer::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("msg_id[%s], get response [%s]", message->m_msg_id.c_str(),message->m_pb_data.c_str());

            makeOrderResponse response;
            if(!response.ParseFromString(message->m_pb_data)) {
                ERRORLOG("deserilize error");
                return;
            }
            DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());
        });

    });
}


void test_tcp_channel() {
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

    // test_tcp_client();

    test_tcp_channel();
}