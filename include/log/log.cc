#include "log.h"
#include <errno.h>
#include <assert.h>      //assert
#include <stdarg.h>      //va_list
#include <sys/stat.h>    //mkdir
#include <sys/syscall.h> //system call

#define MEM_USE_LIMIT (3u * 1024 * 1024 * 1024) //3GB

#define LOG_USE_LIMIT (1u * 1024 * 1024 * 1024) //1GB

// 一条日志长度限制
#define LOG_LEN_LIMIT (4 * 1024) //4KB
// RELOG临界点
#define RELOG_THRESOLD 5
// 消费者等待时间
#define BUFF_WAIT_TIME 1

pthread_mutex_t Log::_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Log::_cond = PTHREAD_COND_INITIALIZER;

size_t Log::buffer_block_size = 1024 * 1024; //1MB

Log::Log()
    : buffer_cnt(3), producer_ptr(NULL), consumer_ptr(NULL), fp(NULL),
      log_count(0), env_ok(false), level(INFO), last_limit_time(0), tm()
{
    //create double linked list
    Buffer *head = new Buffer(buffer_block_size);
    if (!head)
    {
        fprintf(stderr, "no space to allocate Buffer\n");
        exit(1);
    }
    Buffer *current;
    Buffer *prev = head;
    // 创建循环链表
    for (int i = 1; i < buffer_cnt; ++i)
    {
        current = new Buffer(buffer_block_size);
        if (!current)
        {
            fprintf(stderr, "no space to allocate Buffer\n");
            exit(1);
        }
        current->prev = prev;
        prev->next = current;
        prev = current;
    }
    prev->next = head;
    head->prev = prev;
    // 消费者和生产者指向头节点
    producer_ptr = head;
    consumer_ptr = head;
}

// 初始化日志类
void Log::init(const char *log_dir, const char *prog_name, int log_level)
{
    pthread_mutex_lock(&_mutex);

    strncpy(dir, log_dir, 512);
    // 日志文件格式：prog.year_mon_day_tidy.log.n
    strncpy(prog, prog_name, 128);

    // 创建文件
    mkdir(dir, 0777);

    //查看是否存在此目录、目录下是否允许创建文件
    if (access(dir, F_OK | W_OK) == -1)
    {
        fprintf(stderr, "logdir: %s error: %s\n", dir, strerror(errno));
    }
    else
    {
        env_ok = true;
    }

    // 保证日志登记在FATAL到TRACE之间
    if (level > TRACE)
        level = TRACE;
    if (level < FATAL)
        level = FATAL;

    level = log_level;

    pthread_mutex_unlock(&_mutex);
}

void Log::persist()
{
    while (true)
    {
        pthread_mutex_lock(&_mutex);
        // 临界区：
        // 如果缓冲区为空，等待其他线程唤醒
        if (consumer_ptr->status == Buffer::FREE)
        {
            struct timespec tsp;
            struct timeval now;
            gettimeofday(&now, NULL);
            tsp.tv_sec = now.tv_sec;
            tsp.tv_nsec = now.tv_usec * 1000; //nanoseconds
            tsp.tv_sec += BUFF_WAIT_TIME;     //wait for 1 seconds
            pthread_cond_timedwait(&_cond, &_mutex, &tsp);
        }
        // 如果缓冲区为空，继续
        if (consumer_ptr->empty())
        {
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        // 如果缓冲区状态为FREE
        // 这说明生产者和消费者都还指向同一块缓存
        // 标记为FULL，并且强制前移producer_ptr
        if (consumer_ptr->status == Buffer::FREE)
        {
            // 检查是不是FREE状态
            assert(producer_ptr == consumer_ptr); //to test
            producer_ptr->status = Buffer::FULL;
            producer_ptr = producer_ptr->next;
        }

        int year = tm.year, mon = tm.mon, day = tm.day;
        pthread_mutex_unlock(&_mutex);

        if (!switch_file(year, mon, day))
            continue;
        //write
        consumer_ptr->persist(fp);

        // 强制刷新
        fflush(fp);

        pthread_mutex_lock(&_mutex);
        // 清空当前缓冲区

        consumer_ptr->clear();
        // 前移消费者

        consumer_ptr = consumer_ptr->next;
        pthread_mutex_unlock(&_mutex);
    }
}

// 向缓存链表中添加一条日志，不保证成功
void Log::try_append(const char *lvl, const char *format, ...)
{
    int ms;
    // 获取当前时间（秒）
    uint64_t current_sec = tm.get_curr_time(&ms);
    // 如果距离上一次限制写入时间小于1s，直接丢弃
    if (last_limit_time && current_sec - last_limit_time < RELOG_THRESOLD)
        return;

    // 待写入的一条日志
    char log_line[LOG_LEN_LIMIT];
    // 先初始化前缀
    // 格式：lvl[year_month-day hour:min:second.ms][tid]
    // 例子：[ERROR][2021-06-06 14:01:55.211][4294967295]
    int prev_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s.%03d]", lvl, tm.utc_fmt, ms);

    va_list arg_ptr;
    va_start(arg_ptr, format);

    // 添加文件信息：
    // 例如：test.cpp:19(main): your message
    int main_len = vsnprintf(log_line + prev_len, LOG_LEN_LIMIT - prev_len, format, arg_ptr);

    va_end(arg_ptr);

    uint32_t len = prev_len + main_len;

    last_limit_time = 0;
    bool tell_back = false;

    pthread_mutex_lock(&_mutex);
    if (producer_ptr->status == Buffer::FREE && producer_ptr->avail_len() >= len)
    {
        // 可以插入到缓冲区当中
        producer_ptr->append(log_line, len);
    }
    else
    {
        if (producer_ptr->status == Buffer::FREE)
        {
            // 写不下设置为满
            producer_ptr->status = Buffer::FULL;
            // 找到下一个缓冲区
            Buffer *next_buf = producer_ptr->next;
            // 通知后台线程写消费日志
            tell_back = true;

            //如果下一个缓冲块已经满了，那么应该插入一个新的缓存块
            if (next_buf->status == Buffer::FULL)
            {

                //if mem use < MEM_USE_LIMIT, allocate new Buffer
                // 如果加入一个新的缓存块之后，大小会超过限制的最大内存，则生产者继续前移
                // 在这种极端情况下，producer一般会一直空转，直到消费者消费完一块buffer，然后设置为FREE
                if (buffer_block_size * (buffer_cnt + 1) > MEM_USE_LIMIT)
                {
                    fprintf(stderr, "no more log space can use\n");
                    producer_ptr = next_buf;
                    // 设置
                    last_limit_time = current_sec;
                }
                else
                {
                    // 插入一个新的缓存块
                    Buffer *new_buffer = new Buffer(buffer_block_size);
                    buffer_cnt += 1;

                    new_buffer->prev = producer_ptr;
                    producer_ptr->next = new_buffer;
                    new_buffer->next = next_buf;
                    next_buf->prev = new_buffer;

                    producer_ptr = new_buffer;
                }
            }
            else
            {
                // 下一个缓存块为空，可以写入
                producer_ptr = next_buf;
            }
            if (!last_limit_time)
                producer_ptr->append(log_line, len);
        }
        else
        {
            // 此处应该为空，这表示当前缓存块已满
            assert(producer_ptr->status == Buffer::FULL);
            last_limit_time = current_sec;
        }
    }
    pthread_mutex_unlock(&_mutex);
    // 通知后台线程消费日志
    if (tell_back)
    {
        pthread_cond_signal(&_cond);
    }
}

bool Log::switch_file(int my_year, int my_mon, int my_day)
{
    if (!env_ok)
    {
        if (fp)
            fclose(fp);
        fp = fopen("/dev/null", "w");
        return fp != NULL;
    }
    if (!fp)
    {
        // 如果文件为空
        _year = my_year;
        _mon = my_mon;
        _day = my_day;
        char log_path[1024] = {};
        sprintf(log_path, "%s/%s.%d%02d%02d.log", dir, prog, _year, _mon, _day);
        fp = fopen(log_path, "w");
        if (fp)
        {
            log_count += 1;
        }
    }
    else if (_day != my_day)
    {
        // 如果日志之前保留的日期不是现在的，那么说明日期已经到了第二天
        // 需要打开一个新文件了
        fclose(fp);
        char log_path[1024] = {};
        _year = my_year;
        _mon = my_mon;
        _day = my_day;
        sprintf(log_path, "%s/%s.%d%02d%02d.log", dir, prog, _year, _mon, _day);
        fp = fopen(log_path, "w");
        if (fp)
        {
            log_count = 1;
        }
    }
    else if (ftell(fp) >= LOG_USE_LIMIT)
    {
        // 如果文件的大小已经超过了我们限制的大小，也重新打开一个新的日志文件
        // 将当前正在写的文件后缀名更改为log
        // 然后重新打开一个log后缀的文件名
        fclose(fp);
        char old_path[1024] = {};
        char new_path[1024] = {};
        //mv xxx.log.[i] xxx.log.[i + 1]
        for (int i = log_count - 1; i > 0; --i)
        {
            sprintf(old_path, "%s/%s.%d%02d%02d.log.%d", dir, prog, _year, _mon, _day, i);
            sprintf(new_path, "%s/%s.%d%02d%02d.log.%d", dir, prog, _year, _mon, _day, i + 1);
            rename(old_path, new_path);
        }
        //mv xxx.log xxx.log.1
        sprintf(old_path, "%s/%s.%d%02d%02d.log", dir, prog, _year, _mon, _day);
        sprintf(new_path, "%s/%s.%d%02d%02d.log.1", dir, prog, _year, _mon, _day);
        rename(old_path, new_path);
        fp = fopen(old_path, "w");
        if (fp)
        {
            log_count += 1;
        }
    }
    return fp != NULL;
}

// 后台持久化日志的线程函数
void *persist_worker(void *args)
{
    Log::getInstance()->persist();
    return NULL;
}
