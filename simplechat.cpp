#include "simplechat.h"
#include <cerrno>
#include <stdexcept>

namespace {
int setNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    return -1;
  }
  return 0;
}
} // namespace

SimpleChat::SimpleChat()
    : m_pool(nullptr), m_sqlconnpool(nullptr), m_user(), m_passwd(), m_db(),
      m_con_num(0), m_ip(), m_port(-1), m_lfd(-1), epoll_fd(-1), ep_size(0),
      users(nullptr), m_thread_count(0) {
  memset(events, 0, sizeof(events));
}

SimpleChat::~SimpleChat() {
  if (epoll_fd != -1) {
    close(epoll_fd);
    epoll_fd = -1;
  }
  if (m_lfd != -1) {
    close(m_lfd);
    m_lfd = -1;
  }
  if (m_pool != nullptr) {
    m_pool->close();
    delete m_pool;
    m_pool = nullptr;
  }
  delete[] users;
  users = nullptr;
}

void SimpleChat::init(std::string ip, std::string user, std::string passwd,
                      std::string db, int Port, int con_num, int thread_count) {
  m_ip = ip;
  m_user = user;
  m_passwd = passwd;
  m_db = db;
  m_port = Port;
  m_con_num = con_num;
  m_thread_count = thread_count;
  tcp_conn::setcstate(tcp_conn::LT);
}

void SimpleChat::thread_pool() {
  if (m_thread_count <= 0) {
    throw std::invalid_argument("thread count must be positive");
  }
  if (m_pool == nullptr) {
    m_pool = new ThreadPool<tcp_conn>(m_thread_count);
    // 加入接受数据函数
    m_pool->addFunction([](tcp_conn *conn) { return conn->read_once(); });
    // 加入发送数据函数
    m_pool->addFunction([](tcp_conn *conn) { return conn->sendData(); });
    m_pool->start();
  }
}

void SimpleChat::sql_pool() {
  m_sqlconnpool = SqlConnectionPool::getInstance();
  m_sqlconnpool->init(m_ip, m_user, m_passwd, m_db, m_port, m_con_num);
}

void SimpleChat::eventListen() {
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  setNonBlock(lfd);
  if (lfd == -1) {
    throw std::runtime_error("socket init defeat");
  }
  int opt = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_port);
  if (bind(lfd, (sockaddr *)&addr, sizeof(addr)) == -1) {
    close(lfd);
    throw std::runtime_error("bind error");
  }
  if (listen(lfd, 64) == -1) {
    close(lfd);
    throw std::runtime_error("listen error");
  }
  if (setNonBlock(lfd) == -1) {
    close(lfd);
    throw std::runtime_error("set nonblock error");
  }
  m_lfd = lfd;

  int epfd = epoll_create(100);
  if (epfd == -1) {
    close(m_lfd);
    m_lfd = -1;
    throw std::runtime_error("epoll_create1 error");
  }
  tcp_conn::setepfd(epfd);
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.fd = lfd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) == -1) {
    close(epfd);
    throw std::runtime_error("epoll_ctl add listen fd error");
  }
  epoll_fd = epfd;
  ep_size = sizeof(events) / sizeof(struct epoll_event);
  if (users == nullptr) {
    users = new tcp_conn[MAX_FD];
  }
}

void SimpleChat::eventLoop() {
  if (epoll_fd == -1 || m_lfd == -1) {
    throw std::logic_error("eventLoop called before eventListen");
  }
  while (true) {
    int num = epoll_wait(epoll_fd, events, ep_size, -1);
    if (num == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("epoll_wait error");
    }
    for (int i = 0; i < num; ++i) {
      int now_fd = events[i].data.fd;
      uint32_t event_mask = events[i].events;
      if (now_fd == m_lfd) {
        while (1) {
          struct sockaddr_in user_addr;
          socklen_t count = sizeof(user_addr);
          int userfd = accept(m_lfd, (sockaddr *)&user_addr, &count);
          if (userfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            }
            perror("accept");
            break;
          }
          setNonBlock(userfd);
          if (userfd >= MAX_FD) {
            close(userfd);
            continue;
          }
          users[userfd].setFd(userfd); // 初始化连接对象
          struct epoll_event ev;
          memset(&ev, 0, sizeof(ev));
          ev.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
          ev.data.fd = userfd;
          if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, userfd, &ev) == -1) {
            perror("epoll_ctl-accept");
            close(userfd);
          }
        }
      } else {
        if (event_mask & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) {
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, now_fd, NULL);
          close(now_fd);
          break;
        } else if (event_mask & EPOLLIN) {
          handleRead(now_fd);
        } else if (event_mask & EPOLLOUT) {
          handleWrite(now_fd);
        }
      }
    }
  }
}

void SimpleChat::handleRead(int sockfd) {
  if (sockfd >= MAX_FD || m_pool == nullptr) {
    return;
  }
  m_pool->append(&users[sockfd], 0); // 0 表示读操作
}

void SimpleChat::handleWrite(int sockfd) {
  if (sockfd >= MAX_FD || m_pool == nullptr) {
    return;
  }
  m_pool->append(&users[sockfd], 1); // 1 表示写操作
}
