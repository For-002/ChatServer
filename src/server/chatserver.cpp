#include"chatserver.hpp"
#include"json.hpp"
#include"chatservice.hpp"
#include<string>

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 连接断开
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);    // 处理断开的客户端连接
        conn->shutdown();   // 释放资源
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    std::string buf = buffer->retrieveAllAsString();
    // 数据序列化
    json js = json::parse(buf);

    auto msghandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msghandler(conn, js, time);
}