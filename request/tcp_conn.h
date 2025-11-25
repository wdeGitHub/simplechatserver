#ifndef TCP_CONN
#define TCP_CONN
class tcp_conn
{
public:
    tcp_conn() : m_fd(-1) {}
    tcp_conn(int fd) : m_fd(fd) {}
    int recvData();
    int sendData();
    void setFd(int fd) { m_fd = fd; }
    int getFd() const { return m_fd; }
private:
    int m_fd;
};
#endif