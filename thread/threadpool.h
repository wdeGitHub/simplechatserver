#include<iostream>
#include<fcntl.h>
#include"../safequeue/safequeue.h"
#include<pthread.h>
#include<functional>
#include<vector>
#include<semaphore.h>
#include<unistd.h>
#include<functional>
#include<vector>
template<typename T>
class ThreadPool
{
public:
    ThreadPool(int threadCount=8,int maxrequest=10000);
    bool append(T* reqstruct);
    bool isClosePool();
    void work();
    void start();
    void close();
    int addFunction(std::function<int(T*)>);
    ~ThreadPool();
private:
    //线程安全的任务队列
    SafeQueue<T*> taskQueue;
    int threadCount;//当前线程数量
    int maxrequest;//线程池允许的最大线程数量
    bool isClose;//线程池是否关闭
    std::vector<pthread_t> *threads;//线程池中的线程
    sem_t sem;  //用于监管资源
    std::vector<std::function<int(T*)>> funbox;//往funbox中添加函数,线程池执行,用这个特性，增加池的通用性
};


template<typename T>
ThreadPool<T>::ThreadPool(int threadCount, int maxrequest)
{
    if(threadCount <= 0 || maxrequest <= 0)
    {
        throw std::invalid_argument("ThreadPool constructor invalid argument");
    }
    isClose = true;
    threads = new std::vector<pthread_t>;
    this->threadCount = threadCount;
    this->maxrequest = maxrequest;
    sem_init(&this->sem,0,0);
    
}
template<typename T>
bool ThreadPool<T>::append(T* reqstruct)
{
    if(this->taskQueue.size() < (size_t)maxrequest)
    {
        this->taskQueue.push(reqstruct);
        sem_post(&this->sem);
        return true;
    }
    return false;
}
template<typename T>
void ThreadPool<T>::work()
{
    while(!isClose)
    {
        sem_wait(&this->sem);
        T* task=this->taskQueue.pop();
        if(task!=nullptr)
        {
            auto &f=funbox.at(task->state);
            f(task);
            //task->process();
        }
        else{
            sleep(1);
        }
    }
}
template<typename T>
void ThreadPool<T>::start()
{
    isClose=false;
    for(int i=0;i<threadCount;i++)
    {
        pthread_t id;
        int tmp=pthread_create(&id, nullptr, [](void* arg) -> void* {
            ThreadPool* pool = static_cast<ThreadPool<T>*>(arg);//不在这里转换，就要在函数里转换了
            pool->work();
            return nullptr;
        }, this);
        if(tmp!=0)
        {
            delete threads;
            throw std::runtime_error("ThreadPool start pthread_create error");
        }
        threads->push_back(id);
    }
}
template<typename T>
void ThreadPool<T>::close()
{
    isClose = true;
    // 唤醒所有阻塞在 sem_wait 的线程（假设 maxrequest 是最大线程数）
    for (int i = 0; i < threadCount; ++i) {
        sem_post(&sem);  // 每次 post 唤醒一个线程
    }
}
template <typename T>
inline int ThreadPool<T>::addFunction(std::function<int(T *)>)
{
    return 0;
}
template <typename T>
ThreadPool<T>::~ThreadPool()
{
    close();
    for(size_t i=0;i<threads->size();i++)
    {
        pthread_join((*threads)[i],nullptr);
    }
    delete threads;
    sem_destroy(&this->sem);
}
