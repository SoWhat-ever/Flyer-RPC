#include "Flyer/common/log.h"
#include "Flyer/net/tcp/net_addr.h"
#include "Flyer/net/tcp/tcp_server.h"
#include "Flyer/net/tcp/tcp_connection.h"
#include "Flyer/net/tcp/tcp_client.h"
#include "Flyer/net/coder/string_coder.h"
#include "Flyer/net/coder/abstract_protocol.h"
#include "Flyer/net/coder/tinypb_coder.h"
#include "Flyer/net/coder/tinypb_protocol.h"


#include <iostream>
#include <memory>
#include <string.h>
#include <unistd.h>

void test_client() {
    // use connect to server
    // write a string
    // waiting read return

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    DEBUGLOG("client -> server fd = [%d]", fd);

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    int rt = connect(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

    std::string msg = "Hello Flyer!";

    rt = write(fd, msg.c_str(), msg.length());

    DEBUGLOG("succecc write %d bytes, [%s]", rt, msg.c_str());

    char buf[100];
    rt = read(fd, buf, 100);

    DEBUGLOG("succecc write %d bytes, [%s]", rt, std::string(buf).c_str());
}

void test_tcp_client() {
    Flyer::NetAddr::s_ptr addr = std::make_shared<Flyer::IPNetAddr>("127.0.0.1", 12345);
    Flyer::TcpClient client(addr);
    client.connect([addr, &client](){
        DEBUGLOG("connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<Flyer::TinyPBProtocol> message = std::make_shared<Flyer::TinyPBProtocol>();
        
        message->m_msg_id = "123456789";
        message->m_pb_data = "Client send response : Hello to server from client";
        
        client.writeMessage(message, [](Flyer::AbstractProtocol::s_ptr msg_ptr){
            DEBUGLOG("send message success");
        });
        client.readMessage("123456789", [](Flyer::AbstractProtocol::s_ptr msg_ptr){
            std::shared_ptr<Flyer::TinyPBProtocol> message = std::dynamic_pointer_cast<Flyer::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("msg_id[%s], get response [%s]", message->m_msg_id.c_str(),message->m_pb_data.c_str());
        });

    });
}

int main() {
    std::cout << "Test Start...\n";

    Flyer::Config::SetGlobalConfig("/home/os/Desktop/project/config/flyer.xml");
    
    Flyer::Logger::InitGlobalLogger();

    // test_client();

    test_tcp_client();
        
}