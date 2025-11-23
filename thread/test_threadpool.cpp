#include<iostream>
#include"threadpool.h"
#include <unistd.h>
using namespace std;
class Test
{
public:
    void process();
};
void Test::process()
{
    cout<<"hello threadpool!"<<endl;
}
int main()
{
    ThreadPool<Test> threadpool;
    Test *test=new Test();
    threadpool.append(test);
    threadpool.append(test);
    threadpool.append(test);
    threadpool.start();
    sleep(3);
    threadpool.close();
    delete test;
    return 0;
}