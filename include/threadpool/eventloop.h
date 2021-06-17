#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"
#include <assert.h>
#include <vector>
using std::vector;

template <typename T>
class eventLoop
{
public:
    eventLoop(connection_pool *connPool, int max_requests);
    ~eventLoop();
    bool append(T *request); //添加请求
    static void *worker(void *arg);

public:
    locker queueLocker; //互斥锁，用于保护队列

private:
    int maxRequests;         // 最大请求数
    vector<T *> requestList; // 请求队列
    MYSQL *mysqlConn;        // 数据库连接
    connection_pool *connPool;

private:
    void run();
};

template <typename T>
eventLoop<T>::eventLoop(connection_pool *connPool, int max_requests)
    : connPool(connPool), maxRequests(max_requests)
{
    assert(maxRequests > 0);
    assert(connPool != NULL);
}
template <typename T>
eventLoop<T>::~eventLoop()
{
}

template <typename T>
bool eventLoop<T>::append(T *request)
{
    queueLocker.lock();
    if (requestList.size() >= maxRequests)
    {
        queueLocker.unlock();
        return false;
    }
    requestList.push_back(request);
    queueLocker.unlock();
    return true;
}

template <typename T>
void *eventLoop<T>::worker(void *arg)
{
    eventLoop<T> *loop = (eventLoop<T> *)arg;
    loop->run();
    return nullptr;
}

template <typename T>
void eventLoop<T>::run()
{
    // 获取一个数据库连接
    connectionRAII mysqlconRAII(&mysqlConn, connPool);
    while (true)
    {
        vector<T *> tempList;
        {
            queueLocker.lock();
            //临界区，交换队列，用于减少阻塞
            tempList.swap(requestList);
            queueLocker.unlock();
        }
        if (tempList.size() == 0)
        {
            continue;
        }
        // 接下来只需要消费tempList即可
        // 分配一个数据库连接池
        for (int i = 0; i < tempList.size(); ++i)
        {
            T *request = tempList[i];
            if (!request)
            {
                continue;
            }
            // TODO:
            request->mysql = mysqlConn;
            request->process();
        }
        tempList.clear();
    }
}

#endif