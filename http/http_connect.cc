#include <mysql/mysql.h>
#include <fstream>
#include "http_connect.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbiden";
const char *error_403_form = "Your do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

Locker lock;
map<string, string> users;

void Http_connect::init_mysql_result(Connection_pool *connection_pool)
{
    MYSQL *mysql = NULL;
    ConnectionRAII mysql_connect(&mysql, connection_pool);
    // 在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT usename,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }
    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);
    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);
    // 返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

//对文件描述符设置非阻塞
int set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epoll_fd, int fd, bool one_shot, int trig_mode)
{
    epoll_event event;
    event.data.fd = fd;
    if (1 == trig_mode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    set_nonblocking(fd);
}

//从内核时间表删除描述符
void remove_fd(int epoll_fd, int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//将事件重置为EPOLLONESHOT
void mod_fd(int epoll_fd, int fd, int ev, int trig_mode)
{
    epoll_event event;
    event.data.fd = fd;
    if (trig_mode == 1)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

int Http_connect::user_count_ = 0;
int Http_connect::epoll_fd_ = -1;

//关闭连接，关闭一个连接，客户总量减1
void Http_connect::close_connect(bool real_close = true)
{
    if (real_close && socket_fd_ != -1)
    {
        printf("close %d\n", socket_fd_);
        remove_fd(epoll_fd_, socket_fd_);
        socket_fd_ = -1;
        user_count_--;
    }
}

//初始化连接，外部调用初始化套接字地址
void Http_connect::init(int socket_fd, const sockaddr_in &addr, char *root, int trig_mode,
                        int close_log, string user, string password, string sqlname)
{
    socket_fd_ = socket_fd;
    address_ = addr;
    addfd(epoll_fd_, socket_fd, true, trig_mode_);
    user_count_++;

    // 当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件内容完全为空
    doc_root_ = root;
    trig_mode_ = trig_mode;
    close_log_ = close_log;

    strcpy(sql_user_, user.c_str());
    strcpy(sql_password_, password.c_str());
    strcpy(sql_name_, sqlname.c_str());
    init();
}

//初始化新接受的连接
//check_state默认为分析请求行状态
void Http_connect::init()
{
    mysql = NULL;
    bytes_to_send_ = 0;
    bytes_have_send_ = 0;
    check_state_ = CHECK_STATE_REQUESTLINE;
    linger_ = false;
    method_ = GET;
    url_ = 0;
    version_ = 0;
    content_length_ = 0;
    host_ = 0;
    start_line_ = 0;
    checked_idx_ = 0;
    read_idx_ = 0;
    write_idx_ = 0;
    cgi = 0;
    state_ = 0;
    timer_flag_ = 0;
    improve_ = 0;
    memset(read_buf_, '\0', READ_BUFFER_SIZE);
    memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
    memset(real_file_, '\0', FILENAME_LEN);
}

//状态机，用于分析出一行的内容
//返回值为行的读取状态，有LINE_BAD,LINE_OPEN
Http_connect::LINE_STATUS Http_connect::parse_line()
{
    char temp;
    for (; checked_idx_ < read_idx_; ++checked_idx_)
    {
        temp = read_buf_[checked_idx_];
        if (temp == '\r')
        {
            if ((checked_idx_ + 1) == read_idx_)
                return LINE_OPEN;
            else if (read_buf_[checked_idx_ + 1] == '\n')
            {
                read_buf_[checked_idx_++] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r')
            {
                read_buf_[checked_idx_ - 1] = '\0';
                read_buf_[checked_idx_++] = '\0';
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
bool Http_connect::read_once()
{
    if (read_idx_ >= READ_BUFFER_SIZE)
        return false;
    int bytes_read = 0;

    if (0 == trig_mode_)
    {
        //LT读取数据
        bytes_read = recv(socket_fd_, read_buf_ + read_idx_, READ_BUFFER_SIZE - read_idx_, 0);
        read_idx_ += bytes_read;
        if (bytes_read <= 0)
            return false;
        return true;
    }
    else
    {
        //ET读数据
        while (true)
        {
            bytes_read = recv(socket_fd_, read_buf_ + read_idx_, READ_BUFFER_SIZE - read_idx_, 0);
            if (bytes_read == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0)
            {
                return false;
            }
            read_idx_ += bytes_read;
        }
        return true;
    }
}

//解析http请求行，获得请求方法，目标url及http版本号
Http_connect::HTTP_CODE Http_connect::parse_request_line(char *text)
{
    url_ = strpbrk(text, " \t");
    if (!url_)
    {
        return BAD_REQUEST;
    }
    url_ += '\0';
    char *method = text;
    if (strcasecmp(method, "POST") == 0)
    {
        method_ = POST;
        cgi = 1;
    }
    else
        return BAD_REQUEST;
    url_ += strspn(url_, " \t");
    version_ = strpbrk(url_, " \t");
    if (!version_)
        return BAD_REQUEST;
    *version_++ = '\0';
    version_ += strspn(version_, " \t");
    if (strcasecmp(version_, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if (strncasecmp(url_, "http://", 7) == 0)
    {
        url_ += 7;
        url_ = strchr(url_, '/');
    }
    if (strncasecmp(url_, "https://", 8) == 0)
    {
        url_ += 8;
        url_ = strchr(url_, '/');
    }
    if (!url_ || url_[0] != '/')
        return BAD_REQUEST;
    //当url为/时，现实判断界面
    if (strlen(url_) == 1)
        strcat(url_, "judge.html");
    check_state_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析http请求的一个头部信息
Http_connect::HTTP_CODE Http_connect::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        if (content_length_ != 0)
        {
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            linger_ = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        content_length_ = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    }
    else
    {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

//判断http请求是否被完整读入
Http_connect::HTTP_CODE Http_connect::parse_content(char *text)
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char **text = 0;
    while ((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK))
    {
        text = get_line();
        start_line_ = checked_idx_;
        LOG_INFO("%s", text);
        switch (check_state_)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if (ret == GET_REQUEST)
            {
                return do_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (ret == GET_REQUEST)
                return do_request();
            line_status = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

Http_connect::HTTP_CODE Http_connect::do_request()
{
    strcpy(real_file_, doc_root_);
    int len = strlen(doc_root_);
    const char *p = strrchr(url_, '/');
    //处理cgi
    if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3'))
    {
        //根据标志判断是登陆检测还是注册检测
        char flag = url_[1];
        char *url_real_ = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real_, "/");
        strcat(url_real_, url_ + 2);
        strncpy(real_file_ + len, url_real_, FILENAME_LEN - len - 1);
        free(url_real_);

        //将用户名和密码提取出来
        char name[100], password[100];
        int i;
        for (i = 5; string_[i] != '&'; ++i)
            name[i - 5] = string_[i];
        name[i - 5] = '\0';
        int j = 0;
        for (i = i + 10; string_[i] != '\0'; ++i, ++j)
            password[j] = string_[i];
        password[j] = '\0';

        if (*(p + 1) == '3')
        {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "ISERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if (users.find(name) == users.end())
            {
                lock.lock();
                int res = mysql_query(mysql, sql_insert);
                users.insert(pair<string, string>(name, password));
                lock.unlock();

                if (!res)
                    strcpy(url_, "/log.html");
                else
                    strcpy(url_, "/registerError.html");
            }
            else
                strcpy(url_, "/registerError.html");
        }
        else if (*(p + 1) == '2')
        {
            //如果是登陆，直接判断
            //若浏览器输入的用户名和密码在表中可以茶道，返回1，否则返回0
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(url_, "/welcome.html");
            else
                strcpy(url_, "/logError.html");
        }
    }
    if (*(p + 1) == '0')
    {
        char *url_real_ = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real_, "/register.html");
        strncpy(real_file_ + len, url_real_, strlen(url_real_));
        free(url_real_);
    }
    else if (*(p + 1) == '1')
    {
        char *url_real_ = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real_, "/log.html");
        strncpy(real_file_ + len, url_real_, strlen(url_real_));
        free(url_real_);
    }
    else if (*(p + 1) == '5')
    {
        char *url_real_ = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real_, "picture.html");
        strncpy(real_file_ + len, url_real_, strlen(url_real_));
        free(url_real_);
    }
    else if (*(p + 1) == '6')
    {
        char *url_real_ = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real_, "/video.html");
        strncpy(real_file_ + len, url_real_, strlen(url_real_));
        free(url_real_);
    }
    else if (*(p + 1) == '7')
    {
        char *url_real_ = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real_, "/fans.html");
        strncpy(real_file_ + len, url_real_, strlen(url_real_));
        free(url_real_);
    }
    else
        strcpy(real_file_ + len, url_, FILENAME_LEN - len - 1);

    if (stat(real_file_, &file_stat_) < 0)
        return NO_RESOURCE;
    if (!(file_stat_.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;
    if (S_ISDIR(file_stat_.st_mode))
        return BAD_REQUEST;

    int fd = open(real_file_, O_RDONLY);
    file_address_ = (char *)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}
void Http_connect::unmap()
{
    if (file_address_)
    {
        munmap(file_address_, file_stat_.st_size);
        file_address_ = 0;
    }
}

bool Http_connect::write()
{
    int temp = 0;
    if (bytes_to_send_ == 0)
    {
        mod_fd(epoll_fd_, socket_fd_, EPOLLIN, trig_mode_);
        init();
        return true;
    }
    while (true)
    {
    }
}