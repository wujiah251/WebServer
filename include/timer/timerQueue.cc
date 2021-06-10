#include "timerQueue.h"

// 插入一个新的定时器
void timerQueue::addTimer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    Entry entry{timer->expire, timer};
    timerList.insert(entry);
}

// 更新定时器的位置
void timerQueue::adjustTimer(util_timer *timer, time_t new_expire)
{
    if (!timer)
    {
        return;
    }
    // 先删除旧的
    timerList.erase({timer->expire, timer});
    // 插入新的
    Entry newEntry{new_expire, timer};
    timerList.insert(newEntry);
}

// 删除定时器
void timerQueue::deleteTimer(util_timer *timer)
{
    if (timer->callback)
    {
        timer->callback(timer->user_data);
    }
    timerList.erase({timer->expire, timer});
}

// 滴答一下
void timerQueue::tick()
{
    time_t cur = time(NULL);
    // 将所有小于当前时间的定时器删除
    Entry bound{cur, nullptr};
    auto it = timerList.begin();
    while (it != timerList.end())
    {
        if (*it < bound)
        {
            it->second->callback(it->second->user_data);
            timerList.erase(it++);
        }
        else
        {
            break;
        }
    }
}