# lst_timer说明
## client_data
```C++
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};
```
保存套接字描述符、地址、定时器。  
## util_timer
```C++
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire;  //定时器终止时间

    void (*cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};
```
定时器类，数据成员包括前继结点、后继结点、终止时间、客户数据类。
### cb_func(client_data *)
负责关闭已连接套接字，并设置epoll内核事件注册表。  
此函数在定时器超时时候调用。
## sort_timer_lst
升序定时器链表。  
按照终止时间升序排列的定时器双向链表。  
包括数据成员头指针、尾指针。  
提供公有成员函数：  
void add_timer(util_timer*)  
void adjust_timer(util_timer*)  
void del_timer(util_timer*)  
void tick()  
### add_timer(util_timer*)
添加定时器到链表中
### adjust_timer(util_timer*)
调整定时器在链表中的位置。  
由于事件的发生定时器终止时间可能会发生变化，也需要相应的调整其在链表中的位置。
同时插入新的定时器，也需要调整位置
### del_timer(utile_timer*)
删除定时器
### tick()
可以理解为滴答，即查看当前时间，将定时器链表中超时间的定时器对象删除，并调用``cb_func()``删除相应的注册事件和关闭套接字描述符。

## Utils
包含静态数据成员有：``int *u_pipefd``：这是管道，用于通信；``int u_epollfd``：这是epoll文件描述符。    
包含数据成员：``sort_timer_lst m_timer_lst``：定时器链表；``int m_TIMESLOT``：时间间隔？？？
提供的成员函数：  
void init(int timeslot)  
int setnonblocking(int fd)  
void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)  
static void sig_handler(int sig)  
void addsig(int sig, void(handler)(int), bool restart = true)  
void timer_handler()  
void show_error(int connfd, const char *info);

### void init(int timeslot)
设置定时器时间间隔
### int setnonblocking(int fd)  
将文件描述符设置为非阻塞，即立即返回，不等待I/O。
### void addfd(int epollfd, int fd, bool one_shot, int TRIGMode)  
向epoll内核注册表注册套接字事件，并设置触发模式LT/ET。  
one_shot用于设置同一套接字能否被多个线程处理。
### void sig_handler(int sig)  
静态成员函数。  
信号处理函数：用于传递信号到管道
### void addsig(int sig, void(handler)(int), bool restart = true)  
为sig信号设置信号处理函数handler
### void timer_handler()  
定时任务处理  
### void show_error(int connfd, const char *info)  
向套接字输出错误
一般只用于输出超时信息