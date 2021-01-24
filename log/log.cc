#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
#include "log.h"

using namespace std;

Log::Log()
{
    count_ = 0;
    is_async_ = false;
}
Log::~Log()
{

    if (file_ptr_ != NULL)
    {
        fclose(file_ptr_);
    }
}

// 异步需要设置阻塞队列长度，同步不需要设置长度
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    if (max_queue_size >= 1)
    {
        is_async_ = true;
        log_queue_ = new Block_queue<string>(max_queue_size);
        pthread_t tid;
        // flush_log_thread为回调函数，这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
    close_log_ = close_log;
    log_buf_size_ = log_buf_size_;
    buf_ = new char[log_buf_size_];
    memset(buf_, '\0', log_buf_size_); //初始化缓冲区
    split_lines_ = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0}; //写入的日志完整路径，在给定路径上添加了时间信息
    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        // filename->dir_name_ log_name
        strcpy(log_name_, p + 1);
        strncpy(dir_name_, file_name, p - file_name + 1); //目录包含末尾/
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name_, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name_);
        // 完整路径：dir_name_+time+log_name
    }
    today_ = my_tm.tm_mday;
    file_ptr_ = fopen(log_full_name, "a");
    if (file_ptr_ == NULL)
    {
        // 打开失败
        return false;
    }
    return true;
}

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
    case 3:
        strcpy(s, "[erro]:");
    default:
        strcpy(s, "[info]:");
        break;
    }
    // 开始log，先上锁
    mutex_.lock();
    count_++;

    if (today_ != my_tm.tm_mday || count_ % split_lines_ == 0)
    {
        //日期发生变化或者写满日志，更新写入文件名称
        char new_log[256] = {0};
        fflush(file_ptr_); //更新缓冲区
        fclose(file_ptr_); //关闭文件
        char tail[16] = {0};
        snprintf(tail, 16, "%d_%02d_%02d", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (today_ != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name_, tail, log_name_);
            today_ = my_tm.tm_mday;
            count_ = 0;
        }
        else
        {
            // 第一次打开在末尾加上标号 count_ / split_lines_，当前行数除最大行数
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name_, tail, log_name_, count_ / split_lines_);
        }
        file_ptr_ = fopen(new_log, "a");
    }
    mutex_.unlock();

    va_list val_lst;
    va_start(val_lst, format);
    string log_str;
    mutex_.lock();

    int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    //例子：2021-01-17 11:58:06.233677 [info]:
    // 返回值为写入字符串长度
    int m = vsnprintf(buf_ + n, log_buf_size_ - 1, format, val_lst);
    buf_[n + m] = '\n';
    buf_[n + m + 1] = '\0';
    log_str = buf_;
    mutex_.unlock();
    if (is_async_ && !log_queue_->full())
    {
        log_queue_->push(log_str);
    }
    else
    {
        mutex_.lock();
        fputs(log_str.c_str(), file_ptr_);
        mutex_.unlock();
    }
    va_end(val_lst);
}

void Log::flush(void)
{
    // 强制刷新写入流缓冲区
    mutex_.lock();
    fflush(file_ptr_);
    mutex_.unlock();
}
