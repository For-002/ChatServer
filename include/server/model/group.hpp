#ifndef GROUP_H
#define GROUP_H

#include<string>
#include<vector>
#include"groupuser.hpp"

// AllGroup 表的ORM类
class Group
{
public:
    Group(int id = -1, std::string groupname = "", std::string groupdesc = "") 
                : id(id), groupname(groupname), groupdesc(groupdesc) {}

    void setId(int id) { this->id = id; }
    void setName(std::string name) { this->groupname = name; }
    void setDesc(std::string desc) { this->groupdesc = desc; }

    int getId() { return this->id; }
    std::string getName() { return this->groupname; }
    std::string getDesc() { return this->groupdesc; }
    std::vector<GroupUser>& getUsers() { return this->users; }
private:
    int id;
    std::string groupname;
    std::string groupdesc;
    std::vector<GroupUser> users;
};

#endif