#include "log.h"

// 构造函数
utc_timer::utc_timer()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // 设置系统时间，单位分别为秒、分钟
    sys_sec = tv.tv_sec;
    sys_min = sys_sec / 60;
    struct tm cur_tm;
    // 系统调用：获取utc时间
    localtime_r((time_t *)&sys_sec, &cur_tm);
    year = cur_tm.tm_year + 1900;
    mon = cur_tm.tm_mon + 1;
    day = cur_tm.tm_mday;
    hour = cur_tm.tm_hour;
    min = cur_tm.tm_min;
    sec = cur_tm.tm_sec;
    reset_utc_fmt();
}

// 获取当前时间
uint64_t utc_timer::get_curr_time(int *msec)
{
    struct timeval tv;
    //get current ts
    gettimeofday(&tv, NULL);
    if (msec)
    {
        // 获取微秒信息
        *msec = tv.tv_usec / 1000;
    }
    // 如果不是同一秒，更新utc_fmt
    if ((uint32_t)tv.tv_sec != sys_sec)
    {
        sec = tv.tv_sec % 60;
        sys_sec = tv.tv_sec;
        // 如果不是同一分钟，则重新调用localtime
        if (sys_sec / 60 != sys_min)
        {
            sys_min = sys_sec / 60;
            struct tm cur_tm;
            localtime_r((time_t *)&sys_sec, &cur_tm);
            year = cur_tm.tm_year + 1900;
            mon = cur_tm.tm_mon + 1;
            day = cur_tm.tm_mday;
            hour = cur_tm.tm_hour;
            min = cur_tm.tm_min;
            // 更新格式化的utc时间
            reset_utc_fmt();
        }
        else
        {
            // 更新格式化的utc时间，只更新秒的部分
            reset_utc_fmt_sec();
        }
    }
    return tv.tv_sec;
}

// 重置utc时间
void utc_timer::reset_utc_fmt()
{
    snprintf(utc_fmt, 20, "%d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);
}

// 重置utc时间的秒部分
void utc_timer::reset_utc_fmt_sec()
{
    snprintf(utc_fmt + 17, 3, "%02d", sec);
}
