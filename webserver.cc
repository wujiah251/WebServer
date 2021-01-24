#include "webserver.h"

WebServer::WebServer()
{
    // 创建用来保存http连接类的数组
    // 每一个已连接套接字对应一个Http_connect
    users_ = new Http_connect[kMax_Fd];
    // root文件夹路径
    char server_path[200];
    getcwd(server_path, 200); //获取当前项目路径函数
    char root[6] = "/root";
    root_ = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(root_, server_path);
    strcat(root_, root);
    // root_ = "......./WebServer/root"
    // 定时器
    users_timer_ = new Client_data[kMax_Fd];
}

WebServer::~WebServer()
{
    close(epoll_fd_);
    close(listen_fd_);
    close(pipe_fd_[1]);
    close(pipe_fd_[0]);
    delete[] users_;       //释放http连接类
    delete[] users_timer_; //释放定时器
    delete thread_pool_;   //释放线程池
}

void WebServer::init(int port, string user, string password, string database_name, int log_write, int opt_linger,
                     int trig_mode, int sql_num, int thread_num, int close_log, int actor_model)
{
    // 初始化服务器类的配置
    port_ = port;
    database_user_ = user;
    database_password_ = password;
    database_name_ = database_name;
    sql_num_ = sql_num;
    thread_num_ = thread_num;
    log_write_ = log_write;
    opt_linger_ = opt_linger;
    trig_mode_ = trig_mode;
    close_log_ = close_log;
    actor_model_ = actor_model;
}

void WebServer::trig_mode()
{
    // 触发组合模式：监听套接字和已连接套接字的触发模式
    if (trig_mode_ == 0)
    {
        // LT+LT
        listen_trig_mode_ = 0;
        connect_trig_mode_ = 0;
    }
    else if (trig_mode_ == 1)
    {
        // LT+ET
        listen_trig_mode_ = 0;
        connect_trig_mode_ = 1;
    }
    else if (trig_mode_ == 2)
    {
        // ET+LT
        listen_trig_mode_ = 1;
        connect_trig_mode_ = 0;
    }
    else if (trig_mode_ == 3)
    {
        // ET+ET
        listen_trig_mode_ = 1;
        connect_trig_mode_ = 1;
    }
}

void WebServer::log_write()
{
    if (close_log_ == 0)
    {
        // 没有关闭日志
        // 初始化日志
        // 参数分别为：日志写入文件名，关闭日志，日志缓冲区大小，日志但个文件最大行数，工作队列大小
        if (log_write_ == 1)
            Log::get_instance()->init("./ServerLog", close_log_, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", close_log_, 2000, 800000, 0);
    }
}

void WebServer::sql_pool()
{
    // 初始化数据库连接池
    connect_pool_ = Connection_pool::get_instance(); //获得实例
    // 初始化：用户名，密码，数据库名，mysql默认端口为3306，最大连接数，是否关闭日志
    connect_pool_->init("localhost", database_user_, database_password_, database_name_, 3306, sql_num_, close_log_);
    // 初始化数据库读取表
    users_->init_mysql_result(connect_pool_);
}

void WebServer::thread_pool()
{
    // 构造一个线程池实例
    thread_pool_ = new Threadpool<Http_connect>(actor_model_, connect_pool_, thread_num_);
}

void WebServer::event_listen()
{
    // 事件监听
    listen_fd_ = socket(PF_INET, SOCK_STREAM, 0); //创建监听套接字
    assert(listen_fd_ >= 0);                      //断言：检测套接字描述符是否有效

    // 设置关闭方式：是否优雅关闭连接
    if (opt_linger_ == 0)
    {
        struct linger tmp = {0, 1}; //优雅的退出
        setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
        // level-SOL_SOCKET,optname-SO_LINGER
        // 若有数据待发送则等待发送完关闭
        // 详见UNIX网络编程卷一P150
    }
    else if (opt_linger_ == 1)
    {
        struct linger tmp = {1, 1};
        setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
        /*{l_onoff!=0,l_linger>0}
        这种方式下，在调用closesocket的时候不会立刻返回，内核会延迟一段时间，这个时间就由l_linger得值来决定。
        如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，closesocket会返回正确，
        socket描述符优雅性退出。否则，closesocket会直接返回 错误值，未发送数据丢失，socket描述符被强制性退出。
        需要注意的时，如果socket描述符被设置为非堵塞型，则closesocket会直接返回值。*/
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address)); //置零
    address.sin_family = AF_INET;     //IPv4
    //htonl和htons会将结果用大端法存储
    //htonl将一个主机字节顺序表示的32位数值转换为TCP/IP网络顺序字节
    //0.0.0.0，通配地址，表示内核将等到套接字已连接（TCP）或已在套接字上发出数据报（UDP）时才选择一个本地IP地址
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    //htons将一个主机字节顺序表示的16位数值转换为TCP/IP网络顺序字节
    address.sin_port = htons(port_);

    int flag = 1;
    // 允许重用本地地址
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    ret = bind(listen_fd_, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listen_fd_, 5); //套接字最大连接排队为5
    assert(ret >= 0);

    utils_.init(kTime_Slot);

    // epoll创建内核事件表
    epoll_event events[kMax_Event_Number];
    epoll_fd_ = epoll_create(5); //无效参数，只要大于0即可
    assert(epoll_fd_ != -1);

    utils_.add_fd(epoll_fd_, listen_fd_, false, listen_trig_mode_);
    Http_connect::epoll_fd_ = epoll_fd_;

    // 进程间通信
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipe_fd_);
    assert(ret != -1);
    utils_.set_nonblocking(pipe_fd_[1]); //非阻塞
    utils_.add_fd(epoll_fd_, pipe_fd_[0], false, 0);

    utils_.add_sig(SIGPIPE, SIG_IGN);
    utils_.add_sig(SIGALRM, utils_.sig_handler, false);
    utils_.add_sig(SIGTERM, utils_.sig_handler, false);

    alarm(kTime_Slot);

    //工具类，信号和描述符基础操作
    Utils::u_pipe_fd_ = pipe_fd_;
    Utils::u_epoll_fd_ = epoll_fd_;
}

void WebServer::timer(int connect_fd, struct sockaddr_in client_address)
{
    users_[connect_fd].init(connect_fd, client_address, root_, connect_trig_mode_, close_log_,
                            database_user_, database_password_, database_name_);
    // 初始化监听数据
    // 创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer_[connect_fd].address_ = client_address;
    users_timer_[connect_fd].socket_fd_ = connect_fd;
    Util_timer *timer = new Util_timer;
    timer->user_data_ = &users_timer_[connect_fd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire_ = cur + 3 * kTime_Slot;
    users_timer_[connect_fd].timer_ = timer;
    utils_.timer_lst_.add_timer(timer); //将定时器添加进定时器链表
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(Util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire_ = cur + 3 * kTime_Slot;
    utils_.timer_lst_.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void WebServer::deal_timer(Util_timer *timer, int socket_fd)
{
    timer->cb_func(&users_timer_[socket_fd]);
    if (timer)
    {
        utils_.timer_lst_.del_timer(timer); //删除定时器
    }
    LOG_INFO("close fd %d", users_timer_[socket_fd].socket_fd_);
}

bool WebServer::deal_clinet_data()
{
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);
    if (listen_trig_mode_ == 0)
    {
        // LT模式
        int connect_fd = accept(listen_fd_, (struct sockaddr *)&client_address, &client_address_length);
        if (connect_fd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (Http_connect::user_count_ >= kMax_Fd)
        {
            utils_.show_error(connect_fd, "Internet server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connect_fd, client_address);
    }
    else
    {
        // ET模式
        while (true)
        {
            int connect_fd = accept(listen_fd_, (struct sockaddr *)&client_address, &client_address_length);
            if (connect_fd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (Http_connect::user_count_ >= kMax_Fd)
            {
                utils_.show_error(connect_fd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connect_fd, client_address);
        }
        return false;
    }
    return true;
}

bool WebServer::deal_signal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(pipe_fd_[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::deal_read(int socket_fd)
{
    Util_timer *timer = users_timer_[socket_fd].timer_;

    if (actor_model_ == 1)
    {
        //reactor
        if (timer)
        {
            adjust_timer(timer);
        }
        // 若检测到读事件，将该事件放入请求队列
        thread_pool_->append(users_ + socket_fd, 0);

        while (true)
        {
            if (1 == users_[socket_fd].improve_)
            {
                if (1 == users_[socket_fd].timer_flag_)
                {
                    deal_timer(timer, socket_fd);
                    users_[socket_fd].timer_flag_ = 0;
                }
                users_[socket_fd].timer_flag_ = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (users_[socket_fd].read_once())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users_[socket_fd].get_address()->sin_addr));
            //若检测到读事件，将该事件放入请求队列
            thread_pool_->append_p(users_ + socket_fd);
            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer, socket_fd);
        }
    }
}

void WebServer::deal_write(int socket_fd)
{
    Util_timer *timer = users_timer_[socket_fd].timer_;
    if (actor_model_ == 1)
    {
        //reactor
        if (timer)
            adjust_timer(timer);
        thread_pool_->append(users_ + socket_fd, 1);
        while (true)
        {
            if (1 == users_[socket_fd].improve_)
            {
                if (1 == users_[socket_fd].timer_flag_)
                {
                    deal_timer(timer, socket_fd);
                    users_[socket_fd].timer_flag_ = 0;
                }
                users_[socket_fd].improve_ = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (users_[socket_fd].write())
        {
            LOG_INFO("send data to the client(%s)",
                     inet_ntoa(users_[socket_fd].get_address()->sin_addr));
            if (timer)
                adjust_timer(timer);
        }
        else
        {
            deal_timer(timer, socket_fd);
        }
    }
}

void WebServer::event_loop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(epoll_fd_, epoll_events_, kMax_Event_Number, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int socket_fd = epoll_events_[i].data.fd;

            //处理新到的客户连接
            if (socket_fd == listen_fd_)
            {
                bool flag = deal_clinet_data();
                if (false == flag)
                    continue;
            }
            else if (epoll_events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                Util_timer *timer = users_timer_[socket_fd].timer_;
                deal_timer(timer, socket_fd);
            }
            else if ((socket_fd == pipe_fd_[0]) && (epoll_events_[i].events & EPOLLIN))
            {
                // 处理信号
                bool flag = deal_signal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "deal_client_data failure");
            }
            else if (epoll_events_[i].events & EPOLLIN)
            {
                //处理从客户连接上收到的数据
                deal_read(socket_fd);
            }
            else if (epoll_events_[i].events & EPOLLOUT)
            {
                deal_write(socket_fd);
            }
        }
        if (timeout)
        {
            utils_.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}
