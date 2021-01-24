#ifndef WEBSERVER_LOCK_LOCKER_H_
#define WEBSERVER_LOCK_LOCKER_H_

#include <semaphore.h>
#include <pthread.h>
#include <exception>

// 信号量类，基于sem_t类
// 数据成员sem_
// 提供构造函数：sem()、sem(int num)
// 析构函数：~sem()
// 提供wait()函数post()函数
class Sem
{
public:
    // 函数返回0表示成功，失败返回-1，并设置errno
    Sem()
    {
        if (sem_init(&sem_, 0, 0) != 0)
        {
            // 第二个0表示这三一个局部信号量，不在进程间共享
            throw std::exception();
        }
    }
    Sem(int num)
    {
        if (sem_init(&sem_, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~Sem()
    {
        if (sem_destroy(&sem_) != 0)
        {
            throw std::exception();
        }
    }
    bool wait()
    {
        return sem_wait(&sem_) == 0;
    }
    bool post()
    {
        return sem_post(&sem_) == 0;
    }

private:
    sem_t sem_;
};

// 互斥锁类，基于pthread_mutex_t
// 构造函数和析构函数：locker()、~locker()
// 加锁lock()、unlock()
class Locker
{
public:
    Locker()
    {
        if (pthread_mutex_init(&mutex_, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~Locker()
    {
        pthread_mutex_destroy(&mutex_);
    }
    bool lock()
    {
        return pthread_mutex_lock(&mutex_) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&mutex_) == 0;
    }
    pthread_mutex_t *get()
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;
};

// 条件变量类，基于pthread_cond_、pthread_mutex_t
// 构造函数和析构函数：Cond()、~Cond()
// wait()、time_wait()、signal()
class Cond
{
public:
    Cond()
    {
        if (pthread_cond_init(&cond_, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~Cond()
    {
        pthread_cond_destroy(&cond_);
    }
    bool wait(pthread_mutex_t *mutex)
    {
        // mutex是用于保护目标变量的互斥锁，调用前必须确保已经加锁
        int ret = 0;
        ret = pthread_cond_wait(&cond_, mutex);
        return ret == 0;
    }
    bool time_wait(pthread_mutex_t *mutex, struct timespec t)
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&cond_, mutex, &t);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&cond_) == 0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&cond_) == 0;
    }

private:
    pthread_cond_t cond_;
};

#endif