# webserver说明

## webserver
服务器类

### void init(int,string,string,string,int,int,int,int,int,int,int)  
初始化相关参数，包括服务器端口、数据库用户名、数据库密码等等。

### void thread_pool()  
初始化线程池

### void sql_pool()  
初始化数据库连接池

### void log_write()  
初始化日志类，并且打开一个日志文件

### void trig_mode  
设置监听套接字和已连接套接字的触发模式（LT/ET）

### void eventListen()  
事件监听函数：  
1）打开一个监听套接字  
2）设置这个监听套接字的相关参数  
3）设置Uilts类成员utils  
4）创建epoll内核事件表  
5）向utils添加套接字事件（包括设置定时器和将套接字和epoll关联起来）  
6）创建通信管道，并将读管道和epoll关联起来  
7）将管道变量、epoll套接字描述符赋值给Utils  
8）设置信号SIGPIPE、SIGALRM、SIGTERM的处理
9）发出SIGALRM信号

### void eventLoop()
事件循环函数。  
1）调用``epoll_wait()``等待IO事件发生。  
2）处理事件：  
如果是监听套接字，则处理监听数据。  
如果对应文件描述符断开连接/发生错误，则阐述对应的管理类（定时器），调用deal_timer()。  
如果是信号管道的读端口且有可读事件发生，则调用信号处理函数，并返回服务器停止、超时信息。  
如果是读事件，处理客户连接上的读事件（dealwithdata）。  
如果是写事件，处理客户连接上的写事件（dealwithwrite）。  
3）如果超时，则调用utils的定时任务，并输出日志信息

### void timer(int connfd, struct sockaddr_in，client_address)  
1）初始化套接字对应的http连接类  
2）初始化定时器，并加入定时器链表

### void adjust_timer(util_timer *timer)
修改定时器终止时间，调整定时器在链表中的位置。

### void deal_timer(util_timer *timer, int sockfd)
删除定时器，关闭套接字。

### bool dealclinetdata()
1）接收客户端连接，其中有LT/ET模式可选。  
2）在http连接管理类、定时器管理类中初始化这个连接。

### bool dealwithsignal(bool &timeout, bool &stop_server)
1）从管道读取信号到缓冲区。  
2）处理SIGARM信号（返回timeout信息）  
3）处理终止信号（返回stop_server信息）

### void dealwithread(int sockfd)
提供了Reactor和Proacotr两种事件处理模式。  
Reactor：   
调整定时器。  
将事件放入请求队列。    
？？？？  
Proactor：  
读取数据，若读取失败调用deal_timer()，读取成功则将事件添加进入请求队列，调整定时器（adjust_timer）。


### void dealwithwrite(int sockfd)
提供了Reactor和Proacotr两种事件处理模式。  
Reactor：   
调整定时器。  
将事件放入请求队列。    
？？？？  
Proactor：  
读取数据，若读取失败调用deal_timer()，读取成功则将事件添加进入请求队列，调整定时器（adjust_timer）。