#ifndef WEBSERVER_LOG_BLOCK_QUEUE_H_
#define WEBSERVER_LOG_BLOCK_QUEUE_H_

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <../lock/locker.h>
using namespace std;

// 线程安全的循环队列

template <class T>
class Block_queue
{
public:
    Block_queue(int max_size = 1000)
    {
        if (max_size <= 0)
        {
            exit(-1);
        }
        max_size_ = max_size;
        array_ = new T[max_size];
        size_ = 0;
        front_ = -1;
        back_ = -1;
    }

    void clear()
    {
        mutex_.lock();
        size_ = 0;
        front_ = -1;
        back_ = -1;
        mutex_.unlock();
    }
    ~_Block_queue()
    {
        mutex_.lock();
        if (array_ != NULL)
            delete[] array_;
        mutex_.unlock();
    }
    bool full()
    {
        mutex_.lock();
        if (size_ >= max_size_)
        {
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();
        return false;
    }
    bool empty()
    {
        mutex_.lock();
        if (size_ == 0)
        {
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();
        return false;
    }
    bool front(T &value)
    {
        mutex_.lock();
        if (0 == size_)
        {
            mutex_.unlock();
            return false;
        }
        value = array_[front_];
        mutex_.unlock();
        return true;
    }
    bool back(T &value)
    {
        mutex_.lock();
        if (size_ == 0)
        {
            mutex_.unlock();
            return false;
        }
        value = array_[back_];
        mutex_.unlock();
        return true;
    }
    int size()
    {
        int tmp = 0;
        mutex_.lock();
        tmp = size_;
        mutex_.unlock();
        return tmp;
    }
    int max_size()
    {
        int tmp = 0;
        mutex_.lock();
        tmp = max_size_;
        mutex_.unlock();
        return tmp;
    }
    bool push(const T &item)
    {
        mutex_.lock();
        if (size_ >= max_size)
        {
            cond_.broadcast();
            mutex.unlock();
            return false;
        }
        back_ = (back_ + 1) % max_size();
        array_[back_] = item;
        size_++;
        cond_.broadcast();
        mutex_.unlock();
        return true;
    }
    bool pop(T &item)
    {
        mutex_.lock();
        while (size_ <= 0)
        {
            if (!cond_.wait())
            {
                mutex_.unlock();
                return false;
            }
        }
        // ***有点问题
        front_ = (front_ + 1) % max_size();
        item = array_[front_];
        size_--;
        mutex_.unlock();
        return true;
    }
    bool pop(T &item, int timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        mutex_.lock();
        if (size_ <= 0)
        {
            t.tv_sec = now.tv_sec + ms_timeout / 1000; //秒
            t.tv_nsec = (ms_timeout % 1000) * 1000;    //纳秒
            // 疑惑？？
            if (!cond_.time_wait(t){
                mutex_.unlock();
                return false;
            }
        }
        if (size_ <= 0)
        {
            mutex_.unlock();
            return false;
        }
        front_ = (front_ + 1) % max_size_;
        item = array[front_];
        size_--;
        mutex_.unlock();
        return true;
    }

private:
    Locker mutex_;
    Cond cond_;

    T *array_;
    int size_;
    int max_size_;
    int front_;
    int back_;
};

#endif