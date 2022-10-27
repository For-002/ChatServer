#include"usermodel.hpp"
#include"db.hpp"
#include<iostream>

bool UserModel::insert(User &user)
{
    // 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO User(name, password, state) VALUES('%s', '%s', '%s');",
                user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取刚刚插入数据的主键 id
            user.setId(mysql_insert_id(mysql.getConnection()));     // 返回由前面的INSERT或UPDATE语句为AUTO_INCREMENT列生成的值。
            return true;
        }
    }
    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT * FROM User WHERE id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "UPDATE User SET state = '%s' WHERE id = %d", user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

bool UserModel::resetState()
{
    char sql[1024] = "UPDATE User SET state = 'offline' WHERE state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}