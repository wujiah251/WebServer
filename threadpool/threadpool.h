#ifndef WEBSERVER_THREADPOOL_THREADPOOL_H_
#define WEBSERVER_THREADPOOL_THREADPOOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class Threadpool
{
public:
    //thread_number是线程池中的线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量
    Threadpool(int actor_model, Connection_pool *connect_pool, int thread_number = 8, int max_requests = 10000);
    ~Threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    //工作线程运行的函数，它不断从工作队列中取出任务并执行之
    static void *worker(void *arg);
    void run();

private:
    int thread_number_;             //线程池中的线程数量
    int max_requests_;              //请求队列中允许的最大请求数
    pthread_t *threads_;            //描述线程池的数组，其大小为thread_number_
    std::list<T *> work_queue_;     //工作队列
    Locker queue_locker_;           //保护请求队列的互斥锁
    Sem queue_stat_;                //是否有任务需要处理
    Connection_pool *connect_pool_; //数据库连接池
    int actor_model_;               //模型切换
};
template <typename T>
Threadpool<T>::Threadpool(int actor_model, Connection_pool *connect_pool, int thread_number, int max_requests)
    : actor_model_(actor_model), thread_number_(thread_number), max_requests_(max_requests), threads_(NULL), connect_pool_(connect_pool)
{
    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }
    threads_ = new pthread_t[thread_number];
    if (threads_ == NULL)
        throw std::exception();
    for (int i = 0; i < thread_number_; i++)
    {
        if (pthread_create(threads_ + i, NULL, worker, this) != 0)
        {
            delete[] threads_;
            throw std::exception();
        }
        // 让线程池中的每个线程变成分离的，这样子线程在运行完成后就不需要显式地等待每个对等线程终止。
        // 在这种情况下，每个对等线程都应该在它开始处理请求之前分离它自身，这样就能在它终止后回收它的内存资源了。
        if (pthread_detach(threads_[i]))
        {
            delete[] threads_;
            throw std::exception();
        }
    }
}
// 析构函数
template <typename T>
Threadpool<T>::~Threadpool()
{
    delete[] threads_;
}
// 添加工作任务
template <typename T>
bool Threadpool<T>::append(T *request, int state)
{
    queue_locker_.lock();
    if (work_queue_.size() >= max_requests_)
    {
        queue_locker_.unlock();
        return false;
    }
    request->state_ = state;
    work_queue_.push_back(request);
    queue_locker_.unlock();
    queue_stat_.post();
    return true;
}
template <typename T>
bool Threadpool<T>::append_p(T *request)
{
    queue_locker_.lock();
    if (work_queue_.size() >= max_requests_)
    {
        queue_locker_.unlock();
        return false;
    }
    work_queue_.push_back(request);
    queue_locker_.unlock();
    queue_stat_.post();
    return true;
}

template <typename T>
void *Threadpool<T>::worker(void *arg)
{
    Threadpool *pool = (Threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void Threadpool<T>::run()
{
    while (true)
    {
        queue_stat_.wait();
        queue_locker_.lock();
        if (work_queue_.empty())
        {
            queue_locker_.unlock();
            continue;
        }
        T *request = work_queue_.front();
        work_queue_.pop_front();
        queue_locker_.unlock();
        if (!request)
            continue;
        if (actor_model_ == 1)
        {
            if (request->state_ == 0)
            {
                if (request->read_once())
                {
                    request->improve_ = 1;
                    ConnectionRAII mysql_connect(&request->mysql, connect_pool_);
                    request->process();
                }
                else
                {
                    request->improve_ = 1;
                    request->timer_flag_ = 1;
                }
            }
            else
            {
                if (request->write())
                {
                    request->improve_ = 1;
                }
                else
                {
                    request->improve_ = 1;
                    request->timer_flag_ = 1;
                }
            }
        }
        else
        {
            ConnectionRAII mysql_connect(&request->mysql, connect_pool_);
            request->process();
        }
    }
};

#endif