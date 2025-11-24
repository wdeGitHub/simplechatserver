#ifndef TCP_CONN
#define TCP_CONN
class tcp_conn
{
public:
    tcp_conn();
    void process() {}//完成
private:
    int m_fd;
};
#endif