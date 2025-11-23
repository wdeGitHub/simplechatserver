#include "simplechat.h"

SimpleChat::SimpleChat()
{
    m_pool = nullptr;
    m_sqlconnpool = nullptr;
}

void SimpleChat::init(std::string ip,std::string user,std::string passwd,std::string db,int Port,int con_num,int thread_count)
{
    m_ip=ip;
    m_user=user;
    m_passwd=passwd;
    m_db=db;
    m_port=Port;
    m_con_num=con_num;
    m_thread_count=thread_count;
}

void SimpleChat::thread_pool()
{
    m_pool = new ThreadPool<tcp_conn>(m_thread_count);
}

void SimpleChat::sql_pool()
{
    m_sqlconnpool = SqlConnectionPool::getInstance();
    m_sqlconnpool->init(m_ip, m_user, m_passwd, m_db, m_port, m_con_num);
}

void SimpleChat::eventListen()
{
    int lfd=socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1)
    {
        std::runtime_error("cfd init defeat");
    }
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct  sockaddr_in addr;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(m_port);
    int tem=bind(lfd,(sockaddr*)&addr,sizeof(addr));
    if(tem==-1)
    {
        std::runtime_error("bind error");
    }
    tem=listen(lfd,64);
    m_lfd=lfd;


    int epfd=epoll_create(100);
    struct epoll_event ev;
    ev.events = EPOLLIN;    // 检测lfd读读缓冲区是否有数据
    ev.data.fd = lfd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if(ret == -1)
    {
        perror("epoll_ctl     ");
        exit(0);
    }
    ep_size = sizeof(events) / sizeof(struct epoll_event);
}

void SimpleChat::eventLoop()
{
    while(1)
    {
        int num=epoll_wait(epoll_fd,events,ep_size,-1);
        for(int i=0;i<num;i++)
        {
            int now_fd=events[i].data.fd;
            if(now_fd==m_lfd)
            {
                struct sockaddr_in user_addr;
                socklen_t count;
                int userfd=accept(m_lfd,(sockaddr*)&user_addr,&count);
                struct epoll_event ev;
                ev.events = EPOLLIN;    // 读缓冲区是否有数据
                ev.data.fd = userfd;
                int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, userfd, &ev);
                if(ret == -1)
                {
                    perror("epoll_ctl-accept");
                    exit(0);
                }
                if(userfd==-1)
                {
                    exit(1);
                }
            }
            else{
                 // 处理通信的文件描述符
                // 接收数据
                //检测要发送用户在不在线，若不在线存入request队列，关闭时存入log文件
                char buf[1024];
                memset(buf, 0, sizeof(buf));
                int len = recv(now_fd, buf, sizeof(buf), 0);
                if(len == 0)
                {
                    printf("客户端已经断开了连接\n");
                    // 将这个文件描述符从epoll模型中删除
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, now_fd, NULL);
                    close(now_fd);
                }
                else if(len > 0)
                {
                    printf("客户端say: %s\n", buf);
                    send(now_fd, buf, len, 0);
                }
                else
                {
                    perror("recv");
                    exit(0);
                } 
            }
        }
    }
}
