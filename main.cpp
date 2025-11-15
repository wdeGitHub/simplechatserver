#include <cstdio>
#include<iostream>
#include<fcntl.h>
#include<assert.h>
#include <linux/in.h>
#include <sys/endian.h>
#include <unistd.h>   
#include <sys/types.h>   
#include <sys/socket.h>   
#include <netdb.h>    
#include <stdio.h>    
#include <stdlib.h>   
#include <string.h>    
#include <ctype.h>    
#include <errno.h>   
#include <malloc.h>   
#include <netinet/in.h>    
#include <arpa/inet.h>    
#include <sys/ioctl.h>    
#include <stdarg.h>    
#include <fcntl.h>    


int main()
{
    int cfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(9990);
    int size=sizeof(addr);
    int tmp=bind(cfd, (sockaddr*)&addr, size);
    if(tmp<0)
    {
        perror("wrang");
    }
    int ltmp=listen(cfd, 128);
    if(ltmp<0)
    {
        perror("listen error");
    }
    struct sockaddr cliaddr;
    socklen_t len=sizeof(cliaddr);
    int fd=accept(cfd,(sockaddr*)&addr,&len);
    while (1) {
        read(fd, nullptr, 0);
    }

    return 0;
}