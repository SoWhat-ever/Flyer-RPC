#include "Flyer/common/log.h"
#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/tcp/tcp_server.h"
#include "order.pb.h"
#include "Flyer/net/rpc/rpc_dispatcher.h"

#include <iostream>
#include <memory>
#include <google/protobuf/service.h>
#include <unistd.h>


class OrderServiceImpl : public OrderService {
public:
    void makeOrder(google::protobuf::RpcController* controller,
                       const ::makeOrderRequest* request,
                       ::makeOrderResponse* response,
                       ::google::protobuf::Closure* done) {
        // DEBUGLOG("start sleep 5s");
        // sleep(5);
        // DEBUGLOG("end sleep 5s");
        APPDEBUGLOG("call makeOrder");
        printf("111");
        if(request->price() < 100) {
            response->set_ret_code(-1);
            response->set_res_info("price not enough (< 100)");
            return;
        }
        response->set_order_id("20231314");
        response->set_ret_code(0);
        response->set_res_info("success make order, orderId=20231314");
        APPDEBUGLOG("call makeOrder success");
    }
};



int main(int argc, char* argv[]) {

    if(argc != 2) {
        printf("Start rpc server error, argc is not 2 \n");
        printf("Start like this: \n");
        printf("./rpc_server /home/os/Desktop/project/config/flyer_server.xml \n");
        return 0;
    }


    Flyer::Config::SetGlobalConfig(argv[1]);
    
    Flyer::Logger::InitGlobalLogger();

    std::shared_ptr<OrderServiceImpl> order_service = std::make_shared<OrderServiceImpl>();
    Flyer::RpcDispatcher::GetRpcDispatcher()->registerService(order_service);

    Flyer::IPNetAddr::s_ptr addr = std::make_shared<Flyer::IPNetAddr>("127.0.0.1", Flyer::Config::GetGlobalConfig()->m_port);

    Flyer::TcpServer tcp_server(addr);

    tcp_server.start();
        
}