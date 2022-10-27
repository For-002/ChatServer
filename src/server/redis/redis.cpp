#include"redis.hpp"
#include<iostream>

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    // 负责 publish 发布消息的上下文链接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        std::cerr << "Connect redis failed!" << std::endl;
        return false;
    }

    // 负责 subscribe 订阅消息的上下文链接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribe_context)
    {
        std::cerr << "Connect redis failed!" << std::endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息就给业务层进行上报
    std::thread t([&]() {
        observe_channel_message();
    });
    t.detach();

    std::cout << "Connect redis-server success!" << std::endl;

    return true;
}

bool Redis::publish(int channel, std::string message)
{
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        std::cerr << "Publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{

    // subscribe 命令本身会阻塞等待通道里接收到的消息，这里只告诉redis服务器我们要订阅某某某通道，并没有实际开始接收通道消息
    // 通道消息的接收专门在 observer_channel_message 函数中的独立线程中进行
    // 这里只负责发送命令，不阻塞接收 redis 服务器的消息，否则会和 notifyMsg 线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        std::cerr << "Subscrube command failed!" << std::endl;
        return false;
    }
    // redisBufferWrite 可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            std::cerr << "Subscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}

bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        std::cerr << "Unsubscribe command failed!" << std::endl;
        return false;
    }
    // redisBufferWrite 可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            std::cerr << "Unsubscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}

void Redis::observe_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << std::endl;
}

void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->_notify_message_handler = fn;
}