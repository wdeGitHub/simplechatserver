#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H
#include<iostream>
#include<semaphore.h>
#include <stdexcept>
#include<list>
//线程安全的队列,即读取，写入，增加，删除等操作是线程安全的
//维护一个队列,阻塞和非阻塞状态不能混着用
template<typename T>
class SafeQueue
{
public:
    SafeQueue();
    SafeQueue(int max_count);
    ~SafeQueue();
    //非阻塞版
    T pop();
    std::list<T> pop(int num);
    void push(const T &value);
    void push(T &&value);
    //阻塞版(空阻塞)
    void setblock();
    T pop_b();
    //std::list<T> pop_b(int num);
    void push_b(const T &value);
    void push_b(T &&value);

    bool isFull();
    T front();
    bool empty();
    size_t size();
private:
    SafeQueue(const SafeQueue&) = delete;
    SafeQueue& operator=(const SafeQueue&) = delete;
    std::list<T> list;
    pthread_mutex_t mutex;
    bool isblock;
    sem_t m_sem;
    int m_max_count;
};

template <typename T>
inline SafeQueue<T>::SafeQueue()
{
    m_max_count=-1;
    pthread_mutex_init(&mutex,NULL);
    this->isblock=false;
}
template <typename T>
SafeQueue<T>::SafeQueue(int max_count) : m_max_count(max_count)
{
    if(max_count<=0)
    {
        std::runtime_error("max_count is small");
    }
    pthread_mutex_init(&mutex,NULL);
    this->isblock=false;
}

template <typename T>
SafeQueue<T>::~SafeQueue()
{
    pthread_mutex_destroy(&mutex);
    // 若信号量已初始化，销毁它
    if (this->isblock) {
        sem_destroy(&m_sem);
    }
}
//返回值是引用和指针都有一定风险
template <typename T>
T SafeQueue<T>::pop()
{
    pthread_mutex_lock(&mutex);
    if(list.empty())
    {
        pthread_mutex_unlock(&mutex);
        return nullptr;
    }
    T value = list.front();
    list.pop_front();
    pthread_mutex_unlock(&mutex);
    return value;
}
    // 实现：用移动语义返回临时list
template <typename T>
std::list<T> SafeQueue<T>::pop(int num) {
    pthread_mutex_lock(&mutex);
    std::list<T> temp; // 局部list

    // ... 往temp中添加元素（如批量弹出队列元素）...
    size_t pop_count = std::min(static_cast<size_t>(num), list.size());
    for (size_t i = 0; i < pop_count; ++i) {
        temp.push_back(std::move(list.front())); // 移动元素到temp
        list.pop_front();
    }
    pthread_mutex_unlock(&mutex);
    return temp; // 自动调用移动构造函数（C++11起返回局部对象会触发移动）
}

template <typename T>
void SafeQueue<T>::push(const T &value)
{
    pthread_mutex_lock(&mutex);
    list.push_back(value);
    pthread_mutex_unlock(&mutex);
}

template <typename T>
void SafeQueue<T>::push(T &&value)
{
    pthread_mutex_lock(&mutex);
    list.push_back(std::move(value));
    pthread_mutex_unlock(&mutex);   
}

template <typename T>
void SafeQueue<T>::setblock() {
        pthread_mutex_lock(&mutex); // 保护isblock的读写
        if (this->isblock) {
            pthread_mutex_unlock(&mutex);
            throw std::runtime_error("Block mode already enabled");
        }
        // 初始化信号量（初始值0，表示队列初始为空）
        if (sem_init(&m_sem, 0, 0) != 0) {
            pthread_mutex_unlock(&mutex);
            throw std::runtime_error("Failed to initialize semaphore");
        }
        this->isblock = true;
        pthread_mutex_unlock(&mutex);
    }

template <typename T>
T SafeQueue<T>::pop_b()
{
    //队列为空时阻塞，等待队列有元素，函数出错直接返回
    if(sem_wait(&m_sem)!=0)
    {
        exit(1);
    }
    pthread_mutex_lock(&mutex);
    if (list.empty()){
        pthread_mutex_unlock(&mutex);
        throw std::runtime_error("list is empty!");
    }
    T value = list.front();
    list.pop_front();
    pthread_mutex_unlock(&mutex);
    return value;
}
// template <typename T>
// std::list<T> pop_b(int num) {
//         check_block_mode();

//         if (num <= 0) {
//             return {}; // 无效参数，返回空列表
//         }

//         std::list<T> temp;
//         // 先获取1个信号量（确保至少有1个元素，避免死锁）
//         if (sem_wait(&m_sem) != 0) {
//             return temp; // 被中断，返回空
//         }

//         pthread_mutex_lock(&mutex);
//         // 此时至少有1个元素（因已获取1个信号量）
//         size_t pop_count = std::min(static_cast<size_t>(num), list.size());
//         for (size_t i = 0; i < pop_count; ++i) {
//             temp.push_back(std::move(list.front()));
//             list.pop_front();
//             // 若还需要更多元素，且队列中仍有，直接取（无需再等信号量）
//             if (i + 1 < pop_count && !list.empty()) {
//                 continue;
//             }
//             // 若还需要，但队列已空，需等待下一个信号量（释放锁后等待）
//             if (i + 1 < pop_count) {
//                 pthread_mutex_unlock(&mutex); // 先释放锁
//                 if (sem_wait(&m_sem) != 0) {
//                     return temp; // 被中断，返回已获取的元素
//                 }
//                 pthread_mutex_lock(&mutex); // 重新加锁
//             }
//         }
//         pthread_mutex_unlock(&mutex);
//         return temp;
//     }

template <typename T>
void SafeQueue<T>::push_b(const T &value)
{
    pthread_mutex_lock(&mutex);
        list.push_back(value);
    pthread_mutex_unlock(&mutex);
    sem_post(&m_sem);
}

template <typename T>
void SafeQueue<T>::push_b(T &&value)
{
    pthread_mutex_lock(&mutex);
        list.push_back(std::move(value));
    pthread_mutex_unlock(&mutex);  
    sem_post(&m_sem);   
}

template <typename T>
inline bool SafeQueue<T>::isFull()
{
    if(m_max_count==-1)
    {
        std::runtime_error("m_max_count is not initual");
    }
    pthread_mutex_lock(&mutex);
        bool isfull = (list.size() >= m_max_count);
    pthread_mutex_unlock(&mutex);
    return isfull;
}

template <typename T>
T SafeQueue<T>::front()
{
    pthread_mutex_lock(&mutex);
        if (list.empty()) {
            pthread_mutex_unlock(&mutex);
            return nullptr;
        }
        T value = list.front();
    pthread_mutex_unlock(&mutex);
    return value;
}

template <typename T>
bool SafeQueue<T>::empty()
{
    pthread_mutex_lock(&mutex);
        bool isEmpty = list.empty();
    pthread_mutex_unlock(&mutex);
    return isEmpty;
}

template <typename T>
size_t SafeQueue<T>::size()
{
    pthread_mutex_lock(&mutex);
        size_t size = list.size();
    pthread_mutex_unlock(&mutex);  
    return size;
}

#endif
