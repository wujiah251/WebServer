#ifndef _LOG_H__
#define _LOG_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>      //access, getpid
#include <sys/time.h>
#include <sys/types.h> //getpid, gettid

enum LOG_LEVEL
{
    FATAL = 1,
    ERROR, //2
    WARN,  //3
    INFO,  //4
    DEBUG, //5
    TRACE, //6
};

#define gettid() syscall(__NR_gettid)

class utc_timer
{
public:
    int year, mon, day, hour, min, sec;
    // char(19+'\0'):2021-06-06 13:12:28
    char utc_fmt[20];

private:
    // 系统时间，单位分钟
    uint64_t sys_min;
    // 系统时间，单位秒
    uint64_t sys_sec;

public:
    // 构造函数
    utc_timer();
    // 析构函数
    ~utc_timer() {}
    // 获取当前时间
    uint64_t get_curr_time(int *p_msec = nullptr);

private:
    // 重置utc时间
    void reset_utc_fmt();

    // 重置utc时间的秒部分
    void reset_utc_fmt_sec();
};

// 缓冲区
class Buffer
{
public:
    enum buffer_status
    {
        FREE,
        FULL
    };

    // 构造函数
    Buffer(size_t len);

    // 返回可用内存
    size_t avail_len()
    {
        return total_len - used_len;
    }

    // 判空
    bool empty() const { return used_len == 0; }

    // 添加内容
    void append(const char *log_line, size_t len);

    // 清空已有的内存
    void clear();

    // 持久化到磁盘文件中
    void persist(FILE *fp);

    buffer_status status;

    Buffer *prev;
    Buffer *next;

private:
    Buffer(const Buffer &);
    Buffer &operator=(const Buffer &);

    size_t total_len;
    size_t used_len;
    char *data;
};

class Log
{
public:
    // 获取唯一实例
    static Log *getInstance()
    {
        static Log instance;
        return &instance;
    }

    // 初始化，包括设置目录、进程名、日志等级
    // 也会进行创建目录的操作
    void init(const char *log_dir, const char *prog_name, int log_level);

    // 获取日志等级
    int get_level() const { return level; }

    // 持久化
    void persist();

    // 向缓存链表中添加一条日志
    void try_append(const char *lvl, const char *format, ...);

private:
    // 构造函数
    Log();

    // 判断当前文件属性决定是否要重新开一个文件
    bool switch_file(int year, int mon, int day);

    Log(const Log &);
    const Log &operator=(const Log &);

    int buffer_cnt;

    Buffer *producer_ptr;
    Buffer *consumer_ptr;
    Buffer *last_buf;

    FILE *fp;

    // 当前正在写的日志文件的时间
    int _year, _mon, _day;
    // 当日日志文件个数
    int log_count;
    // 进程名
    char prog[128];
    // 目录
    char dir[512];

    // 是否当前设置的目录有写入权限
    bool env_ok; //if log dir ok

    int level;
    // 上次限制写入日志的时间点
    uint64_t last_limit_time;

    utc_timer tm;

    // 互斥锁
    static pthread_mutex_t _mutex;
    // 条件变量
    static pthread_cond_t _cond;

    // 一个缓存块的大小
    static size_t buffer_block_size;
};

// 后台持久化日志的线程函数
void *persist_worker(void *args);

#define LOG_MEM_SET(mem_lmt)                   \
    do                                         \
    {                                          \
        if (mem_lmt < 90 * 1024 * 1024)        \
        {                                      \
            mem_lmt = 90 * 1024 * 1024;        \
        }                                      \
        else if (mem_lmt > 1024 * 1024 * 1024) \
        {                                      \
            mem_lmt = 1024 * 1024 * 1024;      \
        }                                      \
        Log::_one_buff_len = mem_lmt;          \
    } while (0)

// 初始化日志类信息
// 并且创建一个后台线程
#define LOG_INIT(log_dir, prog_name, level)                  \
    do                                                       \
    {                                                        \
        Log::getInstance()->init(log_dir, prog_name, level); \
        pthread_t tid;                                       \
        pthread_create(&tid, NULL, persist_worker, NULL);    \
        pthread_detach(tid);                                 \
    } while (0)

// TRACE日志等级
#define LOG_TRACE(fmt, args...)                                                                 \
    do                                                                                          \
    {                                                                                           \
        if (Log::getInstance()->get_level() >= TRACE)                                           \
        {                                                                                       \
            Log::getInstance()->try_append("[TRACE]", "%s:%d(%s): " fmt "\n",               \
                                            __FILE__, __LINE__, __FUNCTION__, ##args); \
        }                                                                                       \
    } while (0)

// DEBUG日志等级
#define LOG_DEBUG(fmt, args...)                                                                 \
    do                                                                                          \
    {                                                                                           \
        if (Log::getInstance()->get_level() >= DEBUG)                                           \
        {                                                                                       \
            Log::getInstance()->try_append("[DEBUG]", "%s:%d(%s): " fmt "\n",               \
                                           __FILE__, __LINE__, __FUNCTION__, ##args); \
        }                                                                                       \
    } while (0)

// INFO日志等级
#define LOG_INFO(fmt, args...)                                                                  \
    do                                                                                          \
    {                                                                                           \
        if (Log::getInstance()->get_level() >= INFO)                                            \
        {                                                                                       \
            Log::getInstance()->try_append("[INFO]", "%s:%d(%s): " fmt "\n",                \
                                        __FILE__, __LINE__, __FUNCTION__, ##args); \
        }                                                                                       \
    } while (0)

// NORMAL日志等级
#define LOG_NORMAL(fmt, args...)                                                                \
    do                                                                                          \
    {                                                                                           \
        if (Log::getInstance()->get_level() >= INFO)                                            \
        {                                                                                       \
            Log::getInstance()->try_append("[NORMAL]", "%s:%d(%s): " fmt "\n",                \
                                           __FILE__, __LINE__, __FUNCTION__, ##args); \
        }                                                                                       \
    } while (0)

// WARN日志等级
#define LOG_WARN(fmt, args...)                                                                  \
    do                                                                                          \
    {                                                                                           \
        if (Log::getInstance()->get_level() >= WARN)                                            \
        {                                                                                       \
            Log::getInstance()->try_append("[WARN]", "%s:%d(%s): " fmt "\n",                \
                                        __FILE__, __LINE__, __FUNCTION__, ##args); \
        }                                                                                       \
    } while (0)

// ERROR日志等级
#define LOG_ERROR(fmt, args...)                                                                 \
    do                                                                                          \
    {                                                                                           \
        if (Log::getInstance()->get_level() >= ERROR)                                           \
        {                                                                                       \
            Log::getInstance()->try_append("[ERROR]", "%s:%d(%s): " fmt "\n",               \
                                           __FILE__, __LINE__, __FUNCTION__, ##args); \
        }                                                                                       \
    } while (0)

// FATAL日志等级
#define LOG_FATAL(fmt, args...)                                                             \
    do                                                                                      \
    {                                                                                       \
        Log::getInstance()->try_append("[FATAL]", "%s:%d(%s): " fmt "\n",               \
                                       __FILE__, __LINE__, __FUNCTION__, ##args); \
    } while (0)

#endif
