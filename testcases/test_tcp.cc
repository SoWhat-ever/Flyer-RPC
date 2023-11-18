#include "Flyer/common/log.h"
#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/tcp/tcp_server.h"

#include <iostream>
#include <memory>

void test_tcp_server() {
    Flyer::IPNetAddr::s_ptr addr = std::make_shared<Flyer::IPNetAddr>("127.0.0.1", 12345);
    DEBUGLOG("create addr %s",  addr->toString().c_str());

    Flyer::TcpServer tcp_server(addr);

    tcp_server.start();
}

int main() {
    std::cout << "Test Start...\n";

    Flyer::Config::SetGlobalConfig("/home/os/Desktop/project/config/flyer.xml");
    
    Flyer::Logger::InitGlobalLogger();

    test_tcp_server();
        
}