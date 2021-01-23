#ifndef WEBSERVER__WEBSERVER_H_
#define WEBSERVER__WEBSERVER_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "threadpool/threadpool.h"
#include "http/http_connect.h"
#include "config.h"

const int kMax_Fd = 65536;           //最大文件描述符
const int kMax_Event_Number = 10000; //最大事件数
const int kTime_Slot = 5;            //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();
    // 初始化函数，导入配置
    void init(Config *config, string user, string password, string name);
    // 线程池
    void thread_pool();
    // 初始化数据库连接池
    void sql_pool();
    // 初始化日志类
    void log_write();
    // 设置触发组合模式：监听套接字、已连接套接字
    void trig_mode();
    //
    void event_listen();
    // 还没搞清楚
    void event_loop();
    // 还没搞清楚
    void timer(int connect_fd, struct sockaddr_in client_address);
    // 还没搞清楚
    void adjust_timer(Util_timer *timer);
    // 还没搞清楚
    void deal_timer(Util_timer *timer, int socket_fd);
    // 还没搞清楚
    bool deal_clinet_data();
    // 还没搞清楚
    bool deal_signal(bool &timeout, bool &stop_server);
    // 还没搞清楚
    void deal_read(int socket_fd);
    // 还没搞清楚
    void deal_write(int socket_fd);

public:
    int port_;        // 服务器端口号
    char *root_;      //根路径，保存文本资源
    int log_write_;   //日志写入方式
    int close_log_;   //是否关闭日志
    int actor_model_; //并发模型选择

    int pipe_fd_[2]; //管道
    int epoll_fd_;
    // http连接类数组
    Http_connect *users_;

    // 数据库相关
    Connection_pool *connect_pool_; //数据库连接池
    string database_user_;          //登陆数据库用户名
    string database_password_;      //登陆数据库密码
    string database_name_;          //数据库名称
    int sql_num_;                   //连接池规模

    //线程池
    Threadpool<Http_connect> *thread_pool_;
    int thread_num_; //线程池规模

    // epoll_event相关
    epoll_event epoll_events_[kMax_Event_Number];

    int listen_fd_;         // 监听套接字标识符
    int opt_linger_;        // 优雅关闭连接
    int trig_mode_;         // 触发组合模式
    int listen_trig_mode_;  // listen_fd触发模式
    int connect_trig_mode_; // connect_fd触发模式

    // 定时器相关
    Client_data *users_timer_;
    Utils utils_;
};

#endif