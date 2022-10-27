#include"groupmodel.hpp"
#include"db.hpp"

#include<iostream>

bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO AllGroup(groupname, groupdesc) VALUES('%s', '%s')", 
                                group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

bool GroupModel::joinGroup(int groupid, int userid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO GroupUser VALUES(%d, %d, '%s')", groupid, userid, role.c_str());

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

std::vector<Group> GroupModel::queryGroups(int userid)
{
    /*
    1.  先根据userid找出该用户所属的所有群组（同时能够得到这些群组的详细信息）
    2.  然后根据群组的信息，查询每个群组的所有用户的userid（通过多表联合查询能够得到用户的详细信息）
    */
    char sql[1024] = {0};
    sprintf(sql, "SELECT a.id,a.groupname,a.groupdesc FROM AllGroup a INNER JOIN \
                    GroupUser b ON a.id = b.groupid WHERE b.userid = %d", userid);

    MySQL mysql;
    std::vector<Group> groupVec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
        }
        mysql_free_result(res);
    }

    for (Group &group : groupVec)
    {
        sprintf(sql, "SELECT a.id,a.name,a.state,b.grouprole FROM User a INNER JOIN \
                GroupUser b ON a.id = b.userid WHERE b.groupid = %d", group.getId());
        
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
        }
        mysql_free_result(res);
    }
    return groupVec;
}

std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "SELECT userid FROM GroupUser where groupid = %d and userid != %d", groupid, userid);
    
    MySQL mysql;
    std::vector<int> idVec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
        }
        mysql_free_result(res);
    }
    return idVec;
}