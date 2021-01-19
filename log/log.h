#ifndef WEBSERVER_LOG_LOG_H_
#define WEBSERVER_LOG_LOG_H_

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

// 日志类Log
// 单例模式+懒汉模式
// 获得实例方法Log::get_instance()
// 初始化init()，参数包括文件名称，是否关闭，最大缓冲区长度，最大行数，队列最大规模
// 写日志log()，参数包括level(debug、info...)，以及写入信息
// 强制更新文件写入缓冲区flush()

class Log
{
public:
    // 公有方法获取实例
    // C++11以后静态变量初始化线程安全，可以不加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }
    static void *flush_log_thread(void *arg)
    {
        Log::get_instance()->async_write_log();
    }
    // 日志初始化
    // 参数：日志文件，日志缓冲区大小，最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192,
              int split = 5000000, int max_queue_size = 0);
    void write_log(int level, const char *format, ...);
    void flush(void);

private:
    Log();
    virtual ~Log();
    void *async_write_log()
    {
        string single_log;
        // 从阻塞队列中取出一个日志string，写入文件
        while (log_queue_->pop(single_log))
        {
            mutex_.lock();
            fputs(single_log.c_str(), file_ptr_); //写入日志
            mutex_.unlock();
        }
    }

private:
    char dir_name_[128];             //路径名称
    char log_name_[128];             //log文件名
    int split_lines_;                //日志最大行数
    int log_buf_size_;               //日志缓冲区大小
    long long count_;                //日志行数记录
    int today_;                      //记录今天哪天
    FILE *file_ptr_;                 //打开log的文件指针
    char *buf_;                      //缓冲区
    Block_queue<string> *log_queue_; //阻塞队列
    bool is_async_;                  //是否同步标志位
    Locker mutex_;                   //互斥锁
    int close_log_;                  //关闭日志
};

// 写入debug信息
#define LOG_DEBUG(format, ...)                                   \
    if (0 == close_log)                                          \
    {                                                            \
        Log::get_instance()->write_log(0, format, ##_VA_ARGS__); \
        Log::get_instance()->flush();                            \
    }
// 写入info信息
#define LOG_INFO(format, ...)                                    \
    if (0 == close_log)                                          \
    {                                                            \
        Log::get_instance()->write_log(1, format, ##_VA_ARGS__); \
        Log::get_instance()->flush();                            \
    }
// 写入warn信息
#define LOG_WARN(format, ...)                                    \
    if (0 == close_log)                                          \
    {                                                            \
        Log::get_instance()->write_log(2, format, ##_VA_ARGS__); \
        Log::get_instance()->flush();                            \
    }
// 写入error信息
#define LOG_ERROR(format, ...)                                   \
    if (0 == close_log)                                          \
    {                                                            \
        Log::get_instance()->write_log(3, format, ##_VA_ARGS__); \
        Log::get_instance()->flush();                            \
    }

#endif