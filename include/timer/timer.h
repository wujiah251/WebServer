#ifndef LST_TIMER
#define LST_TIMER

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
#include "../log/log.h"

class util_timer;

// 客户端相关数据
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

// 定时器
class util_timer
{
public:
    typedef void (*TimerCallback)(client_data *);

    util_timer(time_t expire, client_data *data, TimerCallback cb)
        : expire(expire), user_data(data), callback(cb)
    {
    }
    void set_expire(time_t ex)
    {
        expire = ex;
    }
    void set_client_data(client_data *data)
    {
        user_data = data;
    }

public:
    time_t expire;
    TimerCallback callback;
    client_data *user_data;
};

// 回调函数
void default_callback(client_data *user_data);

#endif
