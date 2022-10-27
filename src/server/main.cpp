#include"chatserver.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<signal.h>

// 服务器ctrl+c异常结束，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Connamd invalid! Example: ./ChatServer 127.0.0.1 8000" << endl;
        exit(1);
    }

    // 解析ip和端口号
    char *ip = argv[1];
    int port = atoi(argv[2]);

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}