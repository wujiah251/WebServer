#include "lst_timer.h"
#include "../http/http_connect.h"

Sort_timer_lst::Sort_timer_lst()
{
    head_ = NULL;
    tail_ = NULL;
}

Sort_timer_lst::~Sort_timer_lst()
{
    Util_timer *tmp = head_;
    while (tmp)
    {
        head_ = tmp->next;
        delete tmp;
        tmp = head_;
    }
}

void Sort_timer_lst::add_timer(Util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head_)
    {
        head_ = tail_ = timer;
        return;
    }
    if (timer->expire_ < head_->expire_)
    {
        //升序
        timer->next = head_;
        head_->prev = timer;
        head_ = timer;
        return;
    }
    // timer比head_大的情况
    add_timer(timer, head_);
}

void Sort_timer_lst::add_timer(Util_timer *timer, Util_timer *lst_head)
{
    Util_timer *prev = lst_head;
    Util_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire_ < tmp->expire_)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail_ = timer;
    }
}

void Sort_timer_lst::adjust_timer(Util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    Util_timer *tmp = timer->next;
    if (!tmp || (timer->expire_ < tmp->expire_))
    {
        return;
    }
    if (timer == head_)
    {
        head_ = head_->next;
        head_->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head_);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void Sort_timer_lst::del_timer(Util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (timer == head_ && timer == tail_)
    {
        delete timer;
        head_ = NULL;
        tail_ = NULL;
        return;
    }
    if (timer == head_)
    {
        head_ = head_->next;
        head_->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail_)
    {
        tail_ = tail_->prev;
        tail_->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void Sort_timer_lst::tick()
{
    if (!head_)
    {
        return;
    }
    time_t cur = time(NULL);
    Util_timer *tmp = head_;
    while (tmp)
    {
        if (cur < tmp->expire_)
        {
            break;
        }
        tmp->cb_func(tmp->user_data_);
        head_ = tmp->next;
        if (head_)
        {
            head_->prev = NULL;
        }
        delete tmp;
        tmp = head_;
    }
}

void Utils::init(int timeslot)
{
    TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);      //F_GETFL：获取文件标志状态
    int new_option = old_option | O_NONBLOCK; //O_NONBLOCK：非阻塞
    fcntl(fd, F_SETFL, new_option);           //F——SETFL：设置文件状态标志
    return old_option;
}
//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::add_fd(int epoll_fd, int fd, bool one_shot, int trig_mode)
{
    // ？？？？？？
    epoll_event event;
    event.data.fd = fd;
    if (trig_mode == 1)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot)
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
    set_nonblocking(fd);
}

// 信号处理函数
void Utils::sig_handler(int sig)
{
    // ??????
    // 为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipe_fd_[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::add_sig(int sig, void(handler)(int), bool restart)
{
    // ?????
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    timer_lst_.tick();
    alarm(TIMESLOT);
}

void Utils::show_error(int connect_fd, const char *info)
{
    send(connect_fd, info, strlen(info), 0);
    close(connect_fd);
}

int *Utils::u_pipe_fd_ = 0;
int Utils::u_epoll_fd_ = 0;

class Utils;
void cb_func(Client_data *user_data)
{
    epoll_ctl(Utils::u_epoll_fd_, EPOLL_CTL_DEL, user_data->socket_fd_, 0);
    assert(user_data);
    close(user_data->socket_fd_);
    Http_connect::user_count_--;
}
