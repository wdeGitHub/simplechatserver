#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
using namespace std;
// server
enum { ET, LT };
typedef struct data {
  char buf[1024];
  int readlen = 0;
} Data;
Data alldata[1024];
int MAXREADLEN = 1024;
int epfd;
char writebuf[1024];
static const int lstate = LT;
static const int cstate = ET;
bool read_once(int fd);
bool writedata(int fd);
void setNonBlock(int fd);
void parsedata(int fd, char *buf);
int main(int argc, const char *argv[]) {
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  if (lfd < 0) {
    throw runtime_error("haha");
  }
  // 绑定端口
  struct sockaddr_in addr;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(9990);
  addr.sin_family = AF_INET;
  int tmp = bind(lfd, (sockaddr *)&addr, sizeof(addr));
  if (tmp < 0) {
    std::cout << "bind error";
  }
  tmp = listen(lfd, 128);
  epfd = epoll_create(100);

//   int cfd = accept(lfd, &caddr, &size);
  struct epoll_event epev;
  epev.events = EPOLLIN;
  epev.data.fd = lfd;
  // 在epoll中使用时，监听套接字应该设置为非阻塞
  // 这样在while循环中accept时，如果没有连接会立即返回EAGAIN，避免阻塞
  setNonBlock(lfd);
  epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &epev);
  struct epoll_event evns[1024];
  int maxevns = sizeof(evns) / sizeof(evns[0]);
  while (1) {
    int num = epoll_wait(epfd, evns, maxevns, -1);  // -1 表示阻塞等待，直到有事件发生,避免空转
    for (int i = 0; i < num; i++) {
      int fd = evns[i].data.fd;
      if (fd == lfd) {
        if(lstate==LT)
        {
          // LT模式：可以一次accept一个连接，也可以循环accept多个
          // 由于lfd是非阻塞的，accept会立即返回：
          //   - 成功：返回客户端fd
          //   - 失败：返回-1，errno=EAGAIN/EWOULDBLOCK表示没有更多连接
          while(1)
          {
            struct sockaddr_in caddr;
            socklen_t caddr_len = sizeof(caddr);
            int cfd = accept(lfd, (sockaddr*)&caddr, &caddr_len);
            if(cfd<0)
            {
              // 非阻塞accept：没有连接时返回-1，errno=EAGAIN，此时break避免空转
              // 阻塞accept：会一直等待直到有新连接，无法break，会阻塞整个事件循环
              if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接了
              }
              // 其他错误（如EBADF, ECONNABORTED等）也应该break
              break;
            }
            struct epoll_event epev;
            epev.events = EPOLLIN;
            epev.data.fd = cfd;
            setNonBlock(cfd);
            epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &epev); 
          }
        }
        else if(lstate==ET)
        {
            // ET模式：必须一次性accept完所有连接，因为事件只触发一次
            // 非阻塞accept是ET模式的必需，否则无法在循环中accept完所有连接
            while(1)
            {
              struct sockaddr_in caddr;
              socklen_t caddr_len = sizeof(caddr);
              int cfd = accept(lfd, (sockaddr*)&caddr, &caddr_len);
                if(cfd<0)
                {
                    // 非阻塞accept：没有更多连接时返回-1，errno=EAGAIN
                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        break; // 已accept完所有连接
                    }
                    // 其他错误也应该break
                    break;
                }
                struct epoll_event epev;
                epev.events = EPOLLIN | EPOLLET;
                epev.data.fd = cfd;
                setNonBlock(cfd);
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &epev);
            }
        }
      } else {
        if (evns[i].events & EPOLLIN) {
          read_once(fd);
        } else if (evns[i].events & EPOLLOUT) {
          writedata(fd);
        } else if (evns[i].events & EPOLLERR) {
          cout << "error";
        }
      }
    }
  }
}
// EPOLLET以事件触发epoll_wait->必须是一次性读完
// 不使用EPOLLET以读写缓冲区是否被读写空触发epoll_wait->可以一次读完，也可以分几次读
static int mode = 0;
bool read_once(int fd) {
  // 使用proactor模式
  Data &data = alldata[fd];
  char *buf = data.buf;
  char sempbuf[100];
  int &alllen = data.readlen;
  if (alllen >=MAXREADLEN) {
    return false; // 认怂，读不了那么多数据了
  }
  if (cstate == LT) {
    if (mode == 0) // 一次读完
    {
      while (1) {
        int len = read(fd, sempbuf, MAXREADLEN - alllen);
        if (len == 0) {
          memset(buf, 0, sizeof(data.buf));
          data.readlen = 0;
          cout << "对方断开连接";
          struct epoll_event ev;
          ev.events = EPOLLIN;
          ev.data.fd = fd;
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
          return false;
        } else if (len == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            parsedata(fd, buf);
            memset(buf, 0, sizeof(data.buf));
            data.readlen = 0;
            return true;
          }
          return false;
        }
        // 使用 memcpy 更安全，因为 read 可能不会在末尾添加 null 终止符
        memcpy(buf + alllen, sempbuf, len);
        alllen += len;
        buf[alllen] = '\0';  // 确保字符串以 null 结尾
      }
    } else if (mode == 1) // 一次读一些,若没有读完返回false继续读
    {
      int len = read(fd, sempbuf, MAXREADLEN - alllen);
      if (len == 0) {
        memset(buf, 0, sizeof(data.buf));
        data.readlen = 0;
        cout << "对方断开连接";
        return false;
      } else if (len == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) // 读完了
        {
          buf[alllen] = '\0';
          parsedata(fd, buf);
          memset(buf, 0, sizeof(data.buf));
          data.readlen = 0;
          return true;
        }
        return false;
      }
      memcpy(buf + alllen, sempbuf, len);
      alllen += len;
      return true;
    }

  } else if (cstate == ET) // 一次读完
  {
    while (1) {
      int len = read(fd, sempbuf, MAXREADLEN - alllen);
      if (len == 0) {
        memset(buf, 0, sizeof(data.buf));
        data.readlen = 0;
        cout << "对方断开连接"<<endl;
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
        break;
      } else if (len == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          buf[alllen] = '\0';
          parsedata(fd, buf);
          memset(buf, 0, sizeof(data.buf));
          data.readlen = 0;
          return true;
        }
        return false;
      }
      memcpy(buf + alllen, sempbuf, len);
      alllen += len;
    }
    return true;
  }
}
bool writedata(int fd) {
  int tep = write(fd, writebuf, strlen(writebuf));
  if (tep < 0) {
    return false;
  }
  memset(writebuf, 0, sizeof(writebuf));
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = fd;
  epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
  return true;
}
void setNonBlock(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  flag = flag | O_NONBLOCK;
  fcntl(fd, F_SETFL, flag);
}
void parsedata(int fd, char *buf) {
  cout << buf<<endl;
  if (buf[0] == 'Y') {
    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.fd = fd;
    strcpy(writebuf, "hello world");
    // 使用 MOD 而不是 ADD，因为 fd 已经在 epoll 中了
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
  } else {
    cout << "哦二，难吃" << endl;
  }
}