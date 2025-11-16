//这是测试safequeue类的文件
#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include"safequeue.h"
void func(void*arg)
{
    SafeQueue<int>*qu=(SafeQueue<int>*)arg;
    for(int i=0;i<5;i++)
    {
        qu->push(i);
    }
}
int main()
{
    pthread_t id[5];
    SafeQueue<int>qu;
    for(int i=0;i<5;i++)
    {
        pthread_create(&id[i],NULL,(void*(*)(void*))func,&qu);
        pthread_detach(id[i]);
    }   
    sleep(1);
    std::cout<<"队列大小为:"<<qu.size()<<std::endl;
    for(int i=0;i<qu.size();i++)
    {
        int x=qu.pop();
        std::cout<<x<<std::endl;
    }
    return 0;
}