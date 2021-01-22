#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

Connection_pool::Connection_pool()
{
    cur_connection_ = 0;
    free_connection_ = 0;
}
Connection_pool *Connection_pool::get_instance()
{
    static Connection_pool connect_pool;
    return &connect_pool;
}

//构造初始化
void Connection_pool::init(string url, string user, string password, string database_name, int port, int max_connection, int close_log)
{
    url_ = url;
    port_ = port;
    user_ = user;
    password_ = password;
    database_name_ = database_name;
    close_log_ = close_log;

    for (int i = 0; i < max_connection; i++)
    {
        MYSQL *connect = NULL;
        connect = mysql_init(connect);
        if (connect == NULL)
        {
            LOG_ERROR("MYSQL Error");
            exit(1);
        }
        connect = mysql_real_connect(connect, url.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port, NULL, 0);
        if (connect == NULL)
        {
            LOG_ERROR("MySql Error");
            exit(1);
        }
        connect_list_.push_back(connect);
        ++free_connection_;
    }
    reserve_ = Sem(free_connection_);
    max_connection_ = free_connection_;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *Connection_pool::get_connection()
{
    MYSQL *connect = NULL;
    if (0 == connect_list_.size())
        return NULL;
    reserve_.wait();
    lock_.lock();
    connect = connect_list_.front();
    connect_list_.pop_front();
    --free_connection_;
    ++cur_connection_;
    lock_.unlock();
    return connect;
}

//释放当前使用的连接
bool Connection_pool::release_connection(MYSQL *connect)
{
    if (connect == NULL)
    {
        return false;
    }
    lock_.lock();
    connect_list_.push_back(connect);
    ++free_connection_;
    --cur_connection_;
    lock_.unlock();
    reserve_.post();
    return true;
}

//销毁数据库连接池
void Connection_pool::destroy_pool()
{
    lock_.lock();
    if (connect_list_.size() > 0)
    {
        list<MYSQL *>::iterator it;
        for (it = connect_list_.begin(); it != connect_list_.end(); ++it)
        {
            MYSQL *connect = *it;
            mysql_close(connect);
        }
        cur_connection_ = 0;
        free_connection_ = 0;
        connect_list_.clear();
    }
    lock_.unlock();
}

//返回当前空闲的连接数
int Connection_pool::get_free_connection()
{
    return this->free_connection_;
}

Connection_pool::~Connection_pool()
{
    destroy_pool();
}

// ConnectionRAII:
ConnectionRAII::ConnectionRAII(MYSQL **SQL, Connection_pool *connect_pool)
{
    *SQL = connect_pool->get_connection();
    connectRAII_ = *SQL;
    poolRAII_ = connect_pool;
}
ConnectionRAII::~ConnectionRAII()
{
    poolRAII_->release_connection(connectRAII_);
}