#ifndef WEBSERVER_CGIMYSQL_SQL_CONNECTION_POOL_H_
#define WEBSERVER_CGIMYSQL_SQL_CONNECTION_POOL_H_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class Connection_pool
{
public:
    MYSQL *get_connection();                 //获取数据库连接
    bool release_connection(MYSQL *connect); //释放连接
    int get_free_connection();               //获取连接
    void destroy_pool();                     //销毁所有连接

    //单例模式
    static Connection_pool *get_instance();
    void init(string url, string user, string password, string database_name, int port, int max_connection, int close_log);

private:
    Connection_pool();
    ~Connection_pool();

    int max_connection_;         //最大连接数
    int cur_connection_;         //当前已经使用的连接数
    int free_connection_;        //当前空闲的连接数
    Locker lock_;                //互斥锁
    list<MYSQL *> connect_list_; //连接池
    Sem reserve_;                //信号量，用来表示可用的空闲连接数

public:
    string url_;           //主机地址
    string port_;          //数据库端口号
    string user_;          //登陆数据库用户名
    string password_;      //登陆数据库密码
    string database_name_; //使用数据库名
    int close_log_;        //日志开关
};

class ConnectionRAII
{
public:
    ConnectionRAII(MYSQL **connect, Connection_pool *connect_pool);
    ~ConnectionRAII();

private:
    MYSQL *connectRAII_;
    Connection_pool *poolRAII_;
};

#endif