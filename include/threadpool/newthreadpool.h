#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <vector>
#include "./eventloop.h"
#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"

using std::vector;

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request);

private:
    int curIndex;                //当前循环索引，用于负载均衡
    int thread_number;           //线程池中的线程数
    int max_requests;            //请求队列中允许的最大请求数
    connection_pool *m_connPool; //数据库连接池
    pthread_t *threads;          //描述线程池的数组，其大小为m_thread_number
    eventLoop<T> **loops;        //线程循环队列
};
template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests)
    : curIndex(0), thread_number(thread_number), max_requests(max_requests),
      m_connPool(connPool), threads(NULL), loops(NULL)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    threads = new pthread_t[thread_number];
    assert(threads != NULL);
    loops = new eventLoop<T> *[thread_number];

    for (int i = 0; i < thread_number; ++i)
    {
        loops[i] = new eventLoop<T>(connPool, max_requests);
        if (pthread_create(threads + i, NULL, eventLoop<T>::worker, loops[i]) != 0)
        {
            throw std::exception();
        }
        if (pthread_detach(threads[i]))
        {
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    for (int i = 0; i < thread_number; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    delete[] threads;
    for (int i = 0; i < thread_number; ++i)
    {
        delete loops[i];
    }
    delete[] loops;
}

template <typename T>
bool threadpool<T>::append(T *request)
{
    eventLoop<T> *loop = loops[curIndex];
    if (loop->append(request))
    {
        // 负载均衡
        curIndex = (curIndex + 1) % thread_number;
        return true;
    }
    return false;
}

#endif
