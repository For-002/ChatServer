#include"friendmodel.hpp"

bool FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO Friend VALUES(%d, %d),(%d, %d)", userid, friendid, friendid, userid);

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

std::vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT a.id, a.name, a.state FROM User a INNER JOIN Friend b ON a.id = b.friendid WHERE b.userid = %d", userid);

    MySQL mysql;
    std::vector<User> vec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}