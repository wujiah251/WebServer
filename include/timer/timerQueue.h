#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include "timer.h"
#include <set>
using std::pair;
using std::set;

class timerQueue
{
public:
    typedef pair<time_t, util_timer *> Entry;
    typedef set<Entry> TimerList;
    timerQueue() {}
    ~timerQueue() {}
    void addTimer(util_timer *timer);
    void adjustTimer(util_timer *timer, time_t new_expire);
    void deleteTimer(util_timer *timer);
    void tick();

private:
    TimerList timerList;
};

#endif