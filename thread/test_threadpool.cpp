#include<iostream>
#include"threadpool.h"
#include <unistd.h>
#include<functional>
using namespace std;
class Test
{
public:
    int process(Test* test) {
        cout<<"hello threadpool! state="<<this->state<<endl;
        return 0;
    }
    int testfun() {
        cout<<"hello threadpool! testfun"<<endl;
        return 0;
    }
    int state;
};
int main()
{
    ThreadPool<Test> threadpool;
    // 使用 lambda 表达式替换 std::bind
    int funnum=threadpool.addFunction([](Test* t) { 
        return t->testfun(); 
    });
    int funnum2=threadpool.addFunction([](Test* t) { 
        return t->process(t); 
    });
    Test *test=new Test();
    threadpool.start();
    while(true)
    {
        threadpool.append(test,funnum);
        threadpool.append(test,funnum2);
        threadpool.append(test,funnum);
        sleep(3);
    }
    
    sleep(3);
    //threadpool.close();
    //delete test;
    return 0;
}