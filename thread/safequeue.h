#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H
#include<iostream>
#include<list>
//线程安全的队列,即读取，写入，增加，删除等操作是线程安全的
//维护一个队列
template<typename T>
class SafeQueue
{
public:
    SafeQueue();
    ~SafeQueue();
    T& pop();
    void push(T value);
    T& front();
    bool empty();
    size_t size();
private:
    std::list<T> list;
    pthread_mutex_t mutex;
};

#include"safequeue.h"
template<typename T>
SafeQueue<T>::SafeQueue()
{
    pthread_mutex_init(&mutex,NULL);
}

template <typename T>
SafeQueue<T>::~SafeQueue()
{
    pthread_mutex_destroy(&mutex);
}

template <typename T>
T &SafeQueue<T>::pop()
{
    pthread_mutex_lock(&mutex);
    T& value = list.front();
    list.pop_front();
    pthread_mutex_unlock(&mutex);
    return value;
}

template <typename T>
void SafeQueue<T>::push(T value)
{
    pthread_mutex_lock(&mutex);
    list.push_back(value);
    pthread_mutex_unlock(&mutex);
}

template <typename T>
T &SafeQueue<T>::front()
{
    pthread_mutex_lock(&mutex);
    T& value = list.front();
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
    return list.size();
}




#endif
