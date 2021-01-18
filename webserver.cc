#include "webserver.h"

WebServer::WebServer()
{
    // 创建用来保存http连接类的数组
    users_ = new Http_connect[kMax_Fd];
    // root文件夹路径
    char server_path[200];
    getcwd(server_path, 200); //获取当前项目路径函数
    char root[6] = "/root";
    root_ = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(root_, server_path);
    strcat(root_, root);
    // 定时器，没搞懂
    user_timers_ = new Client_data[kMax_Fd];
}

WebServer::~WebServer()
{
    // 没搞懂

    //
    delete[] users_;       //释放http连接类
    delete[] user_timers_; //释放定时器
    delete thread_pool_;   //释放线程池
}

void WebServer::init(Config *config, string user, string password, string name)
{
    // 初始化服务器类的配置
    database_user_ = user;
    database_password_ = password;
    database_name_ = name;
    port_ = config->port_;
    sql_num_ = config->sql_num_;
    thread_num_ = config->thread_num_;
    log_write_ = config->log_write_;
    opt_linger_ = config->opt_linger_;
    trig_mode_ = config->trig_mode_;
    close_log_ = config->close_log_;
    actor_model_ = config->actor_model_;
}

void WebServer::trig_mode()
{
    // 触发组合模式
    // 暂时不写
}

void WebServer::log_write()
{
    if (close_log_ == 0)
    {
        // 初始化日志
        // 暂时不写
    }
}

void WebServer::sql_pool()
{
    // 初始化数据库连接池
    // 暂时不写
}

void WebServer::thread_pool()
{
    // 暂时不写
}

void WebServer::event_listen()
{
    // 事件监听
    listen_fd_ = socket(PF_INET, SOCK_STREAM, 0); //创建监听套接字
    assert(listen_fd_ >= 0);                      //断言：检测套接字描述符是否有效

    // 优雅关闭连接
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
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    // 允许重用本地地址
    ret = bind(listen_fd_, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listen_fd_, 5); //套接字最大连接排队为5
    assert(ret >= 0);

    utils_.init(TIMESLOT); //不懂

    // epoll创建内核事件表
    // 不懂
}

void WebServer::timer(int connect_fd, struct sockaddr_in client_address)
{
}

void WebServer::adjust_timer(Util_timer *timer)
{
}

void WebServer::deal_timer(Util_timer *timer, int socket_fd)
{
}

bool WebServer::deal_clinet_data()
{
}

bool WebServer::deal_signal(bool &timeout, bool &stop_server)
{
}

void WebServer::deal_read(int socket_fd)
{
}

void WebServer::deal_write(int socket_fd)
{
}

void eventLoop()
{
    bool timeout = false;
    bool stop_server = false;
    while (!stop_server)
    {
    }
}
