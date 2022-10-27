#ifndef USERMODEL_H
#define USERMODEL_H

#include"user.hpp"

// user表的数据操作类
class UserModel
{
public:
    // 插入一个user
    bool insert(User &user);
    // 查找id对应的user
    User query(int id);
    // 更新用户状态信息
    bool updateState(User user);
    // 重置用户的状态信息
    bool resetState();
private:

};

#endif