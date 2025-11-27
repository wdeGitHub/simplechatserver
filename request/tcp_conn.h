#ifndef TCP_CONN
#define TCP_CONN
#include <cstdint>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <map>
#include"../sql/sqlconnectionpool.h"
#include"../json/json.hpp"
#include"../global.hpp"
using json = nlohmann::json;
class tcp_conn
{

    const int MAXREADLEN = 1024;
public:
    enum { LT, ET };
    enum Reqstate{
        LOGIN_DEFEAT=1,
        LOGIN_SUCCESS,
        REGISTER_SUCCESS,
        REGISTER_DEFEAT,
        RECVMESSAGE,
    };
    enum ReqInformation{
        LOGIN=0,
        REGISTER,
        SENDMESSAGE, // 新增：消息类型请求
    };
    tcp_conn();
    tcp_conn(int fd) : m_fd(fd) {}
    void init();
    void initAllResult();
    void parseData(char *buf, int len);
    static void setepfd(int epfd) { tcp_conn::epfd = epfd; }
    static void setcstate(int cstate) { tcp_conn::cstate = cstate; }
    bool sendData();
    bool read_once();
    void remove_fd();
    void update_events(uint32_t events);
    static constexpr uint32_t READ_EVENT =
        EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    static constexpr uint32_t WRITE_EVENT =
        EPOLLOUT | EPOLLRDHUP | EPOLLONESHOT;
    void setFd(int fd) { m_fd = fd; }
    int getFd() const { return m_fd; }
    void cleanAllData();
public:
    MYSQL* m_sql;
private:
    static const int mode = 0;
    static  int cstate ;
    static int epfd;
    int m_fd;
    char m_buf[1024];
    char m_writebuf[1024];
    int alllen = 0;
    
};
#endif