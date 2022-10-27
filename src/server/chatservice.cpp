#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<iostream>

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    _msghandlermap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({ONE_CHAT_MSG, std::bind(&ChatService::onechat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addfriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::creategroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({JOIN_GROUP_MSG, std::bind(&ChatService::joingroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupchat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msghandlermap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    // 连接 redis server
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto iter = _msghandlermap.find(msgid);
    if (iter == _msghandlermap.end())
    {
        return [=](const muduo::net::TcpConnectionPtr&, json&, muduo::Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << " can't find Handler!";
        };
    }
    else
    {
        return _msghandlermap[msgid];
    }
}

void ChatService::login(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == -1)
    {
        // 该用户不存在 或 查找数据库发生错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "The user does not exist!";
        conn->send(response.dump());
    }
    else
    {
        if (user.getState() == "online")
        {
            // 该用户已登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "This account is using, input another!";
            conn->send(response.dump());
        }
        else if (user.getId() == id && user.getPwd() == password)
        {
            // 登录成功
            // 先记录用户的连接信息
            {//-----------------------------------------------这个加锁学到很多---------------------------------------------
                lock_guard<std::mutex> lock_guard(_connMutex);
                _userConnMap.insert({id, conn});
            }// 这里的大括号保证了 锁的粒度越小越好！

            // 更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);
            
            // 向 redis 订阅 channel(id)
            _redis.subscribe(id);
            
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查看是否有离线消息
            std::vector<std::string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemessage"] = vec;
                // 获取到离线消息后，把离线消息都删掉
                _offlineMsgModel.remove(id);
            }

            // 获取用户好友列表
            std::vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                std::vector<std::string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 获取用户群组信息
            std::vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                std::vector<string> s_groupVec;
                for (Group &group : groupVec)
                {
                    json groupjs;
                    groupjs["groupid"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    std::vector<string> s_groupUserVec;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        s_groupUserVec.push_back(js.dump());
                    }
                    groupjs["users"] = s_groupUserVec;
                    s_groupVec.push_back(groupjs.dump());
                }
                response["groups"] = s_groupVec;
            }

            // 获取完 离线消息列表、好友列表、群组列表后，将登录应答消息发送出去
            conn->send(response.dump());
        }
        else
        {
            // 输入的用户名或密码错误
            json response;
            response["errno"] = 1;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errmsg"] = "Id or password is invalid!";
            conn->send(response.dump());
        }
    }
}

void ChatService::reg(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPwd(password);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Username is already exist!";
        conn->send(response.dump());
    }
}

void ChatService::clientCloseException(const muduo::net::TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<std::mutex> lock(_connMutex);
        for (auto iter = _userConnMap.begin(); iter != _userConnMap.end(); iter++)
        {
            if (iter->second == conn)
            {
                // 从 map 表中删除用户连接信息
                user.setId(iter->first);
                _userConnMap.erase(iter);
                break;
            }
        }
    }

    // 解除异常下线用户订阅的 redis 通道
    _redis.unsubscribe(user.getId());

    // 更新用户状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::onechat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp)
{
    int toid = js["toid"].get<int>();

    {
        lock_guard<std::mutex> lock(_connMutex);
        auto iter = _userConnMap.find(toid);
        if (iter != _userConnMap.end())
        {
            // 目的用户在线，将消息原封不动的转发给他
            iter->second->send(js.dump());
            return ;
        }
    }

    // 查询 toid 是否是登陆在其他服务器上
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return ;
    }

    // 目的用户不在线，存储为离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::addfriend(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

void ChatService::creategroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    std::string groupname = js["groupname"];
    std::string groupdesc = js["groupdesc"];

    // 存储群组信息
    Group group;
    group.setDesc(groupdesc);
    group.setName(groupname);
    if (_groupModel.createGroup(group))
    {
        // 创建群组成功，存储创建人信息
        _groupModel.joinGroup(id, group.getId(), "creator");
    }
    else
    {
        // 创建群组失败，发送错误信息--------------------------------------------------------------待测试!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // 还需增加一个 CREATE_GROUP_MSG_ACK 的msgid，已表明消息类型
        json ackjs;
        ackjs["errno"] = 1;
        ackjs["errmsg"] = "Create failed! The groupname already exist!";
        lock_guard<std::mutex> lock(_connMutex);
        _userConnMap[id]->send(ackjs.dump());
    }
}

void ChatService::joingroup(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int groupid = js["groupid"].get<int>();
    int userid = js["id"].get<int>();

    _groupModel.joinGroup(groupid, userid, "normal");
}

void ChatService::groupchat(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int srcid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    std::vector<int> idVec = _groupModel.queryGroupUsers(srcid, groupid);
    if (!idVec.empty())
    {
        lock_guard<std::mutex> lock(_connMutex);
        for (int userid : idVec)
        {
            auto iter = _userConnMap.find(userid);
            if (iter != _userConnMap.end())
            {
                iter->second->send(js.dump());
            }
            else if (_userModel.query(userid).getState() == "online")   // 查询用户是否登陆在其他服务器
            {
                _redis.publish(userid, js.dump());
            }
            else
            {
                // 如果不在线，发送离线消息
                _offlineMsgModel.insert(userid, js.dump());
            }
        }
    }
}

void ChatService::loginout(const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    
    {
        lock_guard<mutex> lock(_connMutex);
        auto iter = _userConnMap.find(id);
        if (iter != _userConnMap.end())
        {
            _userConnMap.erase(iter);
        }
    }

    // 解除下线用户订阅的 redis 通道
    _redis.unsubscribe(id);

    // 更新用户状态
    User user(id, "", "", "offline");
    _userModel.updateState(user);
}

void ChatService::reset()
{
    // online状态的用户重置为offline
    _userModel.resetState();
}

void ChatService::handlerRedisSubscribeMessage(int userid, std::string msg)
{
    {
        lock_guard<std::mutex> lock(_connMutex);
        auto iter = _userConnMap.find(userid);
        if (iter != _userConnMap.end())
        {
            iter->second->send(msg);
            return ;
        }
    }

    // 虽然刚刚用户还在线，但就在消息在通道中传输的过程中，用户就下线了，还是需要存储离线消息
    _offlineMsgModel.insert(userid, msg);
}