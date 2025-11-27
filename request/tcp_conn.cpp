#include "tcp_conn.h"
#include <errno.h>

int tcp_conn::cstate = tcp_conn::ET;
int tcp_conn::epfd = -1;

tcp_conn::tcp_conn() : m_fd(-1), alllen(0) {
  memset(m_buf, 0, sizeof(m_buf));
  memset(m_writebuf, 0, sizeof(m_writebuf));
}

bool tcp_conn::read_once() {
  // 使用proactor模式
  char sempbuf[100];
  if (alllen >=MAXREADLEN) {
    return false; // 认怂，读不了那么多数据了
  }
  if (cstate == LT) {
    if (mode == 0) // 一次读完
    {
      while (1) {
        int len = read(m_fd, sempbuf, MAXREADLEN - alllen);
        if (len == 0) {
          memset(m_buf, 0, sizeof(m_buf));
          alllen = 0;
          std::cout << "对方断开连接"<<std::endl;
          struct epoll_event ev;
          ev.events = EPOLLIN;
          ev.data.fd = m_fd;
          epoll_ctl(tcp_conn::epfd, EPOLL_CTL_DEL, m_fd, &ev);
          return false;
        } else if (len == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            parseData(m_buf, len);
            memset(m_buf, 0, sizeof(m_buf));
            alllen = 0;
            return true;
          }
          return false;
        }
        // 使用 memcpy 更安全，因为 read 可能不会在末尾添加 null 终止符
        memcpy(m_buf + alllen, sempbuf, len);
        alllen += len;
        m_buf[alllen] = '\0';  // 确保字符串以 null 结尾
      }
    } else if (mode == 1) // 一次读一些,若没有读完返回false继续读
    {
      int len = read(m_fd, sempbuf, MAXREADLEN - alllen);
      if (len == 0) {
        memset(m_buf, 0, sizeof(m_buf));
        alllen = 0;
        std::cout << "对方断开连接"<<std::endl;
        return false;
      } else if (len == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) // 读完了
        {
          m_buf[alllen] = '\0';
          parseData(m_buf, alllen);
          memset(m_buf, 0, sizeof(m_buf));
          alllen = 0;
          return true;
        }
        return false;
      }
      memcpy(m_buf + alllen, sempbuf, len);
      alllen += len;
      update_events(READ_EVENT);
      return true;
    }

  } else if (cstate == ET) // 一次读完
  {
    while (1) {
      int len = read(m_fd, sempbuf, MAXREADLEN - alllen);
      if (len == 0) {
        memset(m_buf, 0, sizeof(m_buf));
        alllen = 0;
        std::cout << "对方断开连接"<<std::endl;
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = m_fd;
        epoll_ctl(epfd, EPOLL_CTL_DEL, m_fd, &ev);
        break;
      } else if (len == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          m_buf[alllen] = '\0';
          parseData(m_buf, alllen);
          memset(m_buf, 0, sizeof(m_buf));
          alllen = 0;
          return true;
        }
        return false;
      }
      memcpy(m_buf + alllen, sempbuf, len);
      alllen += len;
    }
    return true;
  }

  return false;
}
bool tcp_conn::sendData() {
    int tep = write(m_fd, m_writebuf, strlen(m_writebuf));
    if (tep < 0) {
      return false;
    }
    memset(m_writebuf, 0, sizeof(m_writebuf));
    update_events(READ_EVENT);
    return true;
  }
void tcp_conn::parseData(char *buf, int len) {
  std::cout << buf<<std::endl;
  if (buf[0] == 'Y') {
    strcpy(m_writebuf, "hello world");
    update_events(WRITE_EVENT);
  } else {
    std::cout << "哦二，难吃" << std::endl;
    update_events(READ_EVENT);
  }
}
void tcp_conn::remove_fd() {
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = m_fd;
  epoll_ctl(tcp_conn::epfd, EPOLL_CTL_DEL, m_fd, &ev);
}

void tcp_conn::update_events(uint32_t events) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.fd = m_fd;
  epoll_ctl(tcp_conn::epfd, EPOLL_CTL_MOD, m_fd, &ev);
}