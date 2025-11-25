#include <netinet/in.h>
#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include<exception>
using namespace std;
// server
enum{
    ET,
    LT
};
static const int lstate=ET;
static const int cstate=ET;

void dealread(int fd);
void dealwrite(int fd);
int main(int argc, const char* argv[])
{
    int lfd=socket(AF_INET,SOCK_STREAM,0);
    if(lfd<0)
    {
        throw exception();
    }
    //绑定端口
    struct sockaddr_in addr;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(9990);
    addr.sin_family=AF_INET;
    int tmp=bind(lfd,(sockaddr*)&addr,sizeof(addr));
    if(tmp<0)
    {
        std::cout<<"bind error";
    }
    int epfd=epoll_create(100);
    struct sockaddr caddr;
    socklen_t size;
    int cfd=accept(lfd,&caddr,&size);
    struct epoll_event epev;
    epev.events=EPOLLIN;
    epev.data.fd=cfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD,cfd,&epev );
    struct epoll_event evns[1024];
    int maxevns=sizeof(evns)/sizeof(evns[0]);
    while(1)
    {
        int num=epoll_wait(epfd, evns, maxevns, 0);
        for(int i=0;i<num;i++)
        {
            int fd=evns[i].data.fd;
            if(fd==cfd)
            {
                int cfd=accept(lfd,&caddr,&size);
                struct epoll_event epev;
                if(lstate==ET)
                {
                    epev.events=EPOLLIN|EPOLLET;
                }
                else {
                    epev.events=EPOLLIN;
                }
                epev.data.fd=cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD,cfd,&epev );
            }
            else {
                if(evns[i].events&EPOLLIN)
                {
                    dealread(fd);
                }
                else if(evns[i].events&EPOLLOUT)
                {
                    dealwrite(fd);
                }
                else if(evns[i].events&EPOLLERR)
                {
                    cout<<"error";
                }
            }
        }

    }
}
void dealread(int fd)
{
    if(cstate==ET)
    {
        char buf[1024];
        memset(buf, 0, sizeof(buf));
        int alllen=0;
        while(1)
        {
            char subuf[100];
            memset(subuf,0,sizeof(subuf));
            int len=read(fd, subuf, sizeof(buf)); 
            if(len==0)
            {
                break;
            }
            alllen+=len;
            strcpy(buf+alllen, subuf);
        }
    }
    else if(cstate==LT)
    {
        char buf[100];
        memset(buf, 0, sizeof(buf));
        int len=read(fd, buf, sizeof(buf));
        if(len>0)
        {

        }
        else if(len<0)
        {

        }
        else 
        {

        }
    }
    

}
void dealwrite()
{

}