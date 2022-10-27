#include"json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<ctime>
#include<unordered_map>
#include<functional>

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<semaphore.h>
#include<atomic>

#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前用户登录的群组列表信息
vector<Group> g_currentUserGroupList;
// 控制聊天页面
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t wrsem;
// 记录登陆状态
atomic_bool g_loginSuccess{false};

// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int sockfd);

// 处理登录响应消息
void doLoginResponse(json);
// 处理注册响应消息
void doResResponse(json);

// 聊天客户端程序实现，主线程为发送线程，子线程为接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "cnmmand invalid! Example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建 client 端的 socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写 client 需要连接的 server 信息：ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client 和 server 进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&wrsem, 0, 0);

    // 成功连接服务器后立即启动子线程，专门接收服务器消息
    std::thread readTask(readTaskHandler, clientfd);    // pthread_create
    readTask.detach();      // pthread_detach

    // 主线程接受用户输入，发送出去
    for (;;)
    {
        // 显示首页面菜单，登录，注册，退出
        cout << "=====================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "=====================" << endl;
        cout << "choice" << endl;
        int choice = 0;
        cin >> choice;
        cin.get();  // 读掉缓冲区残留的回车（换行符）

        switch (choice)
        {
            case 1: // login 业务
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "userid: ";
                cin >> id;
                cin.get();  // 读掉缓冲区残留的回车
                cout << "user password: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump(); // 将 json 对象序列化为字符流

                g_loginSuccess = false;

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "Send login msg error!" << request << endl;
                }
                else
                {
                    sem_wait(&wrsem);      // 等待信号量。有子线程处理完登录的响应消息后，通知这里

                    if (g_loginSuccess == true)
                    {
                        // 进入聊天主菜单界面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
            break;
            case 2: // register 业务
            {
                char name[50] = {0};
                char password[50] = {0};
                cout << "username: ";
                cin.getline(name, 50);
                cout << "userpassword: ";
                cin.getline(password, 50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = password;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "Send register msg error!" << request << endl;
                }
                else
                {
                    sem_wait(&wrsem);
                }
            }
            break;
            case 3: // quit 业务
            {
                close(clientfd);
                sem_destroy(&wrsem);
                exit(0);
            }
            default:
                cerr << "invalid input!" << endl;
            break;
        }
    }
    return 0;
}

void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user --> id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------Friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------Group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            if (group.getUsers().empty())
            {
                cout << "   该群组还没有成员!" << endl;
            }
            else
            {
                for (GroupUser &user : group.getUsers())
                {
                    cout << "   " << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
                }
            }
        }
    }
    cout << "======================================================" << endl;
}

void doLoginResponse(json responsejs)
{
    if (0 != responsejs["errno"].get<int>())
    {
        cerr << responsejs["errmsg"] << endl;
        g_loginSuccess = false;
    }
    else    // 登陆成功
    {
        // 记录当前用户 id 和 name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                User user;
                json js = json::parse(str);
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();

            vector<string> s_groupVec = responsejs["groups"];
            for (string &s_group : s_groupVec)
            {
                Group group;
                json groupjs = json::parse(s_group);
                group.setId(groupjs["groupid"].get<int>());
                group.setName(groupjs["groupname"]);
                group.setDesc(groupjs["groupdesc"]);

                vector<string> s_groupUserVec = groupjs["users"];
                for (string &s_user : s_groupUserVec)
                {
                    json js = json::parse(s_user);
                    GroupUser guser;
                    guser.setId(js["id"].get<int>());
                    guser.setName(js["name"]);
                    guser.setState(js["state"]);
                    guser.setRole(js["role"]);
                    group.getUsers().push_back(guser);
                }
                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登陆用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息
        if (responsejs.contains("offlinemessage"))
        {
            vector<string> offlineMsgVec = responsejs["offlinemessage"];
            for (string &str : offlineMsgVec)
            {
                json js = json::parse(str);
                int msgtype = js["msgid"].get<int>();
                // 输出格式：time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == msgtype)
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                    << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"]
                            << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
        g_loginSuccess = true;
    }
}

void doRegResponse(json responsejs)
{
    if (0 != responsejs["errno"].get<int>())
    {
        cerr << responsejs["errmsg"] << endl;
    }
    else    // 注册成功
    {
        cout << "Name register success, userid is " << responsejs["id"] << " ,don't forget it!" << endl;
    }
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[2048] = {0};
        int len = recv(clientfd, buffer, 2048, 0);  // 1. 程序运行到这里会阻塞！！！      2. 第三个参数很重要，不能随便写。
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }
        
        // 接受CharServer转发的数据，反序列化生产json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() 
                                            << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" 
                            << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js);
            sem_post(&wrsem);   // 通知主线程，登陆结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&wrsem);   // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// "help" command handler           help
void help(int fd = 0, string str = "");
// "chat" command handler           chat:friendid:message
void chat(int, string);
// "addfriend" command handler      addfriendid:friendid
void addfriend(int, string);
// "creategroup" command handler    creategroup:groupname:groupdesc
void creategroup(int, string);
// "joingroup" command handler      joingroup:groupid
void joingroup(int, string);
// "groupchat" command handler      groupchat:groupid:message
void groupchat(int, string);
// "loginout" command handler       loginout
void loginout(int, string);

unordered_map<string, string> commandMap = 
{
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriendid:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"joingroup", "加入群组，格式joingroup:groupid"},
    {"groupchat", "群组聊天，格式groupchat:groupid:message"},
    {"loginout", "注销用户，格式loginout"}
};

unordered_map<string, function<void(int, string)>> commandHandlerMap =
{
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"joingroup", joingroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;     // 用于保存命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            help();
            continue;
        }

        // 调用和相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));  // 调用命令处理方法
    }
}

void help(int fd, string s)
{
    cout << "Show Command list >>> " << endl;
    for (pair<string, string> s : commandMap)
    {
        cout << s.first << " : " << s.second << endl;
    }
    cout << endl;
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return ;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "Send chat message error! -> " << buffer << endl;
    }
}

void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "Send addfriend message error! -> " << buffer << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return ;
    }

    string name = str.substr(0, idx);
    string desc = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = name;
    js["groupdesc"] = desc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "Send creategroup message error! -> " << buffer << endl;
    }
}

void joingroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = JOIN_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "Send joingroup message error! -> " << buffer << endl;
    }
}

void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "Groupchat command invalid!" << endl;
        return ;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string msg = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = msg;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "Send groupchat message error! -> " << buffer << endl;
    }
}

void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "Send loginout message error! -> " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}