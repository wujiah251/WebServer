#ifndef WEBSERVER_TIMER_LST_TIMER_H_
#define WEBSERVER_TIMER_LST_TIMER_H_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <time.h>

struct Client_data
{
    sockaddr_in address_;
    int socket_fd_;
    Util_timer *timer_;
};

class Util_timer
{
public:
    time_t expire_;
    void (*cb_func)(Client_data *);
    Client_data *user_data_;
    Util_timer *prev;
    Util_timer *next;

    Util_timer() : prev(NULL), next(NULL) {}
};

void cb_func(Client_data *user_data);

class Sort_timer_lst
{
public:
    Sort_timer_lst();
    ~Sort_timer_lst();

    void add_timer(Util_timer *timer);
    void adjust_timer(Util_timer *timer);
    void del_timer(Util_timer *timer);
    void tick();

private:
    void add_timer(Util_timer *timer, Util_timer *lst_head);

    Util_timer *head_;
    Util_timer *tail_;
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);
    // 对文件描述符设置非阻塞
    int set_nonblocking(int fd);
    // 将内核事件注册读事件，ET模式，并选择开启EPOLLONESHOT
    void add_fd(int epoll_fd, int fd, bool one_shot, int trig_mode);
    // 信号处理函数
    static void sig_handler(int sig);
    // 设置信号函数
    void add_sig(int sig, void(handler)(int), bool restart = true);
    // 定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();
    void show_error(int connect_fd, const char *info);

public:
    static int *u_pipe_fd_;
    Sort_timer_lst timer_lst_;
    static int u_epoll_fd_;
    int TIMESLOT;
};

#endif