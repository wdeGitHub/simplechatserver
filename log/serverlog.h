#ifndef SERVERLOG_H
#define SERVERLOG_H

#include <iostream>
#include <string>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>   // 用于 gettimeofday
#include <stdarg.h>      // 用于可变参数
#include <stdio.h>       // 用于 FILE, fputs, fclose 等
#include "../safequeue/safequeue.h"   // 假设你的 SafeQueue 头文件在同一目录下

// 日志级别定义
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

class ServerLog
{
public:
    // 单例模式，获取唯一实例
    static ServerLog* getInstance();

    // 初始化日志系统
    // file_name: 日志文件基础名
    // close_log: 是否关闭日志(1关闭, 0开启)
    // log_buf_size: 日志缓冲区大小
    // split_lines: 单个日志文件最大行数
    // max_queue_size: 异步日志队列大小, 0表示同步模式
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    // 写入日志
    void write_log(int level, const char *format, ...);

    // 刷新日志缓冲区
    void flush(void);

private:
    // 单例模式：私有构造函数和析构函数，防止外部创建和销毁
    ServerLog();
    ~ServerLog();

    // 静态线程函数，用于异步写入日志
    static void *flush_log_thread(void *args);

    // 异步日志写入的实际逻辑
    void async_write_log();

    // 检查并切割日志文件（按日期或大小）
    void check_log_size();

private:
    char dir_name[128];         // 日志文件所在目录名
    char log_name[128];         // 日志文件基础名
    int m_split_lines;          // 单个日志文件最大行数
    int m_log_buf_size;         // 日志缓冲区大小
    long long m_count;          // 日志总行数记录
    int m_today;                // 当前日志文件的日期（用于按天切割）
    FILE *m_fp;                 // 日志文件指针
    char *m_buf;                // 日志缓冲区
    SafeQueue<std::string>* m_log_queue; // 异步日志队列
    bool m_is_async;            // 是否为异步模式
    pthread_mutex_t m_mutex;    // 互斥锁，保证线程安全
    int m_close_log;            // 是否关闭日志
};

// 方便使用的宏定义
#define LOG_DEBUG(format, ...) ServerLog::getInstance()->write_log(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  ServerLog::getInstance()->write_log(LOG_LEVEL_INFO,  format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  ServerLog::getInstance()->write_log(LOG_LEVEL_WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) ServerLog::getInstance()->write_log(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define LOG_FLUSH()            ServerLog::getInstance()->flush()

#endif