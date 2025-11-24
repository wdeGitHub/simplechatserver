#ifndef SIMPLECHAT
#define SIMPLECHAT

#include<iostream>
#include<string>
#include"sql/sqlconnectionpool.h"
#include"thread/threadpool.h"
#include"json/json.hpp"
#include"request/requestparse.h"
#include"request/tcp_conn.h"
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
class SimpleChat
{
public:
    SimpleChat();
    ~SimpleChat();
    void init(std::string ip,std::string user,std::string passwd,std::string db,int Port,int con_num,int thread_count);//初始化
    void thread_pool();//初始化线程池，将创建连接的任务函数放入
    void sql_pool();//在解析需求之前，先初始化，在request中还要用到
    void eventListen();//设置监听
    void eventLoop();//事件循环
    int recvData(int sockfd);
    void parseData(int sockfd,char *,int);
private:
    void handleRead(int sockfd);
    void handleWrite(int sockfd);
    void process();//处理函数
    ThreadPool<tcp_conn> *m_pool;//处理客户端请求的
    SqlConnectionPool *m_sqlconnpool;
    //数据库类参数
    std::string m_user;
    std::string m_passwd;
    std::string m_db;
    int m_con_num;
    //socket参数
    std::string m_ip;
    
    int m_port;
    int m_lfd;
    //epoll参数
    int epoll_fd;
    struct epoll_event events[1024];
    int ep_size;
    tcp_conn *users;
    //线程类
    int m_thread_count;
};

#endif