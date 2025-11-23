#include<iostream>
#include<string>
#include<string.h>
#include<unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include<sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#define port 9990
int main()
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
    addr.sin_port=htons(port);
    int tem=bind(lfd,(sockaddr*)&addr,sizeof(addr));
    if(tem==-1)
    {
        std::runtime_error("bind error");
    }
    tem=listen(lfd,64);
    //////////////////////////
    //
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
    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(struct epoll_event);
    //
                char buf[1024];
                memset(buf,0,sizeof(buf));
                strcpy(buf,"right\n\0");
    while(1)
    {
        int num=epoll_wait(epfd,evs,size,-1);
        for(int i=0;i<num;i++)
        {
            int nfd=evs[i].data.fd;
            if(nfd==lfd)
            {
                struct sockaddr_in user_addr;
                socklen_t count;
                int userfd=accept(lfd,(sockaddr*)&user_addr,&count);
                ev.events = EPOLLIN;    // 读缓冲区是否有数据
                ev.data.fd = userfd;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, userfd, &ev);
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
                int len = recv(nfd, buf, sizeof(buf), 0);
                if(len == 0)
                {
                    printf("客户端已经断开了连接\n");
                    // 将这个文件描述符从epoll模型中删除
                    epoll_ctl(epfd, EPOLL_CTL_DEL, nfd, NULL);
                    close(nfd);
                }
                else if(len > 0)
                {
                    printf("客户端say: %s\n", buf);
                    send(nfd, buf, len, 0);
                }
                else
                {
                    perror("recv");
                    exit(0);
                } 
            }
        }
    }
    
    // struct sockaddr_in user_addr;
    // socklen_t count;
    // int userfd=accept(lfd,(sockaddr*)&user_addr,&count);
    // if(userfd==-1)
    // {
    //     exit(1);
    // }
    // std::cout<<"userfd right";
    // char buf[1024];
    // memset(buf,0,sizeof(buf));
    // strcpy(buf,"right\n\0");
    
    // while(1)
    // {
    //    send(userfd,buf,(size_t)sizeof(buf),0);
    //     std::cout<<buf;
    //     char rbuf[1024];
    //     memset(rbuf,0,sizeof(rbuf));
    //     read(userfd,rbuf,sizeof(rbuf));
    //     std::cout<<rbuf;
    // sleep(3); 
    // }
    
    return 0;
}