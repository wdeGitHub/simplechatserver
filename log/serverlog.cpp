#include "serverlog.h"
#include <string.h>
#include <stdlib.h> // 用于 exit


// 单例模式，双重检查锁定(DCL)实现
ServerLog* ServerLog::getInstance()
{
    static ServerLog m_instance;
    return &m_instance;
}

ServerLog::ServerLog()
    : m_split_lines(5000000), m_log_buf_size(8192), m_count(0), m_today(0),
      m_fp(NULL), m_buf(NULL), m_log_queue(NULL), m_is_async(false), m_close_log(0)
{
    // 初始化互斥锁
    pthread_mutex_init(&m_mutex, NULL);
}

ServerLog::~ServerLog()
{
    // 销毁互斥锁
    pthread_mutex_destroy(&m_mutex);
    
    // 关闭文件
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
    
    // 释放缓冲区
    if (m_buf != NULL)
    {
        delete[] m_buf;
    }
    
    // 释放队列
    if (m_log_queue != NULL)
    {
        delete m_log_queue;
    }
}

bool ServerLog::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    m_close_log = close_log;
    if (m_close_log) return true; // 如果关闭日志，直接返回

    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);

    m_split_lines = split_lines;

    // 获取当前时间
    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 解析日志文件路径和名称
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s",
                 my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                 file_name);
    }
    else
    {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s",
                 dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                 log_name);
    }

    m_today = my_tm.tm_mday;

    // 打开日志文件
    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL)
    {
        perror("fopen");
        return false;
    }

    // 如果设置了队列大小，则启用异步模式
    if (max_queue_size > 0)
    {
        m_is_async = true;
        m_log_queue = new SafeQueue<std::string>(max_queue_size);
        
        // 创建并分离异步写入线程
        pthread_t tid;
        if (pthread_create(&tid, NULL, flush_log_thread, NULL) != 0)
        {
            delete m_log_queue;
            m_log_queue = NULL;
            m_is_async = false;
            return false;
        }
        pthread_detach(tid);
    }

    return true;
}

void ServerLog::write_log(int level, const char *format, ...)
{
    if (m_close_log) return;

    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    // 日志级别字符串
    const char* level_str[] = {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]"};
    if (level < 0 || level >= 4) level = 1; // 默认INFO

    // 加锁，准备写入
    pthread_mutex_lock(&m_mutex);

    m_count++;

    // 检查是否需要切割日志
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};

        snprintf(tail, 15, "%d_%02d_%02d_",
                 my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 1; // 新文件，计数从1开始
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }

        m_fp = fopen(new_log, "a");
        if (m_fp == NULL)
        {
             // 打开失败时，解锁并退出，防止死锁
             pthread_mutex_unlock(&m_mutex);
             return;
        }
    }
    
    // 格式化日志内容
    va_list valst;
    va_start(valst, format);

    int n = snprintf(m_buf, m_log_buf_size - 1, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, level_str[level]);

    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 2, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';

    va_end(valst);

    std::string log_str = m_buf;

    // 根据模式写入
    if (m_is_async && m_log_queue != NULL)
    {
        // 异步模式：加入队列，由后台线程写入
        m_log_queue->push(log_str);
    }
    else
    {
        // 同步模式：直接写入文件
        fputs(log_str.c_str(), m_fp);
    }

    pthread_mutex_unlock(&m_mutex);
}

void ServerLog::flush(void)
{
    if (m_close_log) return;

    pthread_mutex_lock(&m_mutex);
    fflush(m_fp);
    pthread_mutex_unlock(&m_mutex);
}

// 静态线程函数，它会调用实例的非静态方法
void *ServerLog::flush_log_thread(void *args)
{
    ServerLog::getInstance()->async_write_log();
    return NULL;
}

// 异步日志写入的核心逻辑
void ServerLog::async_write_log()
{
    std::string single_log;
    while (true)
    {
        // 从队列中阻塞获取日志
        if (m_log_queue->pop_b(single_log))
        {
            // 写入文件时也需要加锁，因为可能与同步模式的 write_log 或 flush 同时操作 m_fp
            pthread_mutex_lock(&m_mutex);
            fputs(single_log.c_str(), m_fp);
            pthread_mutex_unlock(&m_mutex);
        }
        // 如果队列被关闭或出现异常，可以在这里添加退出逻辑
    }
}