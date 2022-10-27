- [ChatServer](#chatserver)
- [环境配置](#环境配置)
  - [nginx配置tcp负载均衡](#nginx配置tcp负载均衡)
  - [安装redis的C++库hiredis](#安装redis的c库hiredis)
- [编译项目](#编译项目)
    - [手动编译方法如下：](#手动编译方法如下)


# ChatServer
可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端源码 基于nuduo库实现 使用了redis发布订阅消息队列 数据库采用MySQL

# 环境配置

## nginx配置tcp负载均衡

安装好nginx后，进入nginx的配置文件所在目录
```bash
cd /usr/local/nginx/conf
```
打开该目录下的`nginx.conf`文件，向其中加入如下配置代码
```
stream 
{
  upstream MyServer
  {
    server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
    server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
  }

  server 
  {
    proxy_connect_timeout 1s;
    listen 8000;
    proxy_pass MyServer;
    tcp_nodelay on;
  }
}
```

保存并退出，完成配置。

## 安装redis的C++库hiredis

系统要安装好redis，安装步骤不赘述。
从github上下载hiredis客户端：
```bash
git clone https://github.com/redis/hiredis
```
进入`hiredis`源码目录：
```bash
cd hiredis
```
进行编译：
```bash
make
```
安装：
```bash
sudo make install
```
拷贝生成的动态库到`/usr/local/lib`目录下：
```bash
sudo ldconfig /usr/local/lib
```
安装完成！

# 编译项目

直接运行项目根目录下的脚本文件`auto_build.sh`即可自动完成编译。

### 手动编译方法如下：
进入项目根目录后，再进入`build`目录：
```bash
cd build
```
```bash
cmake ..
```
```bash
make
```
完成编译！
