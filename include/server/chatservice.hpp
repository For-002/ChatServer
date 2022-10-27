#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<mutex>
#include"json.hpp"
#include"usermodel.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"

using json = nlohmann::json;

// 表示处理消息的事件 的回调方法类型
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr&, json&, muduo::Timestamp)>;

// 聊天服务业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 登录回调函数
    void login(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 注册回调函数
    void reg(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 一对一聊天
    void onechat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 添加好友
    void addfriend(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 创建群组
    void creategroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 加入群组
    void joingroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 群组聊天
    void groupchat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time);
    // 用户注销
    void loginout(const muduo::net::TcpConnectionPtr &connm, json &js, muduo::Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端退出
    void clientCloseException(const muduo::net::TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void reset();

    // Redis
    void handlerRedisSubscribeMessage(int userid, std::string msg);
private:
    ChatService();
    
    // 存储消息id和相应业务的回调函数
    std::unordered_map<int, MsgHandler> _msghandlermap;

    // 存储客户端与服务器的连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证多个线程对 _userConnMap 写操作的线程安全
    std::mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // redis 相关操作的对象
    Redis _redis;
};

#endif