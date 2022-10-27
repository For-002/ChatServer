#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include"db.hpp"
#include<cstdio>
#include<string>
#include<vector>

// 存储离线消息表的接口
class OfflineMsgModel
{
public:
    // 插入离线消息
    bool insert(int userid, std::string msg);

    // 删除某用户的所有离线消息
    bool remove(int userid);

    // 获取某用户的所有离线消息
    std::vector<std::string> query(int user);
};

#endif