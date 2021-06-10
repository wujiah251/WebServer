#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include "timer.h"

// 已经排序的定时器链表
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();
    // 添加一个定时器
    void add_timer(util_timer *timer);
    // 更新一个定时器在链表中的位置
    void adjust_timer(util_timer *timer);
    // 删除一个定时器
    void del_timer(util_timer *timer);
    // 滴答一下，删除过期的定时器，并调用回调函数
    void tick();

private:
    void add_timer(util_timer *timer, util_timer *lst_head);

    util_timer *head;
    util_timer *tail;
};







#endif