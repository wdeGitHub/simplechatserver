//这是测试safequeue类的文件
#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include"safequeue.h"
using namespace std;
void *func(void*arg)
{
    SafeQueue<int*>*qu=static_cast<SafeQueue<int*>*>(arg);
    for(int i=0;i<5;i++)
    {
        int *p=new int(i);
        qu->push_b(p);
        usleep(100);
    }
    return nullptr;
}
int main()
{
    pthread_t id[5];
    SafeQueue<int*>qu;
    qu.setblock();
    for(int i=0;i<5;i++)
    {
        pthread_create(&id[i],NULL,func,&qu);
        pthread_detach(id[i]);
    }   
    sleep(1);
    std::cout<<"队列大小为:"<<qu.size()<<std::endl;
    while (!qu.empty())
    {
        int *x=qu.front();
        if(qu.pop_b())
        {
            cout<<"pop ok";
        }
        else
        {
            cout<<"pop error";
        }
        std::cout<<*x<<" "<<qu.size()<<std::endl;
        delete x;
    }
    qu.pop_b();//可以发现进入阻塞
    return 0;
}