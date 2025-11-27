#include "simplechat.h"
int main()
{
    SimpleChat chat;
    chat.init("127.0.0.1","root","@Yd66666","SCSdata",9990,100,10);
    chat.thread_pool(); 
    //chat.sql_pool();
    chat.eventListen();
    chat.eventLoop();
    return 0;
}