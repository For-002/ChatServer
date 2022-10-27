#ifndef USER_H
#define USER_H

#include<string>

// user表的ORM类
class User
{
public:
    User(int id = -1, std::string name = "", std::string password = "", std::string state = "offline")
            : id(id), name(name), password(password), state(state) {}
    ~User() {}

    void setId(int id) { this->id = id; }
    void setName(std::string name) { this->name = name; }
    void setPwd(std::string pwd) { this->password = pwd; }
    void setState(std::string state) { this->state = state; }

    int getId() { return this->id; }
    std::string getName() { return this->name; }
    std::string getPwd() { return this->password; }
    std::string getState() { return this->state; }
protected:
    int id;
    std::string name;
    std::string password;
    std::string state;
};

#endif