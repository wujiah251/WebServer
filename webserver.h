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
const int kTime_Slot = 5;            //最小超时单位  ？？

class Weberver
{
public:
    Webserver();
    ~WebServer();
    // 初始化函数，导入配置
    void init(Config *config);
    // 还没搞清楚
    void thread_pool();
    // 还没搞清楚
    void sql_pool_();
    // 还没搞清楚
    void log_write();
    // 还没搞清楚
    void trig_write();
    // 还没搞清楚
    void event_listen();
    // 还没搞清楚
    void event_loop();
    // 还没搞清楚
    void timer(int connectfd, struct sockaddr_in client_address);
    // 还没搞清楚
    void adjust_timer(util_timer *timer);
    // 还没搞清楚
    void deal_timer(util_timer *timer, int sockfd);
    // 还没搞清楚
    bool deal_clinet_data();
    // 还没搞清楚
    bool deal_signal(bool &timeout, bool &stop_server);
    // 还没搞清楚
    void deal_read(int sockfd);
    // 还没搞清楚
    void deal_write(int sockfd);

public:
    // 不知道怎么写
};

#endif