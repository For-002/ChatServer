#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include<vector>
#include<string>
#include<cstdio>
#include"group.hpp"

// 维护群组信息的操作接口方法
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    bool joinGroup(int userid, int groupid, std::string role);
    // 获取用户所在群组的详细信息
    std::vector<Group> queryGroups(int userid);
    // 根据指定群组id，查询群组用户列表，除userid自己，用于发送群消息
    std::vector<int> queryGroupUsers(int userid, int groupid);
};

#endif