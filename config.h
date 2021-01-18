#ifndef WEBSERVER__CONFIG_H_
#define WEBSERVER__CONFIG_H_

#include "webserver.h"

using namespace std;
// 项目的配置类
// 配置参数分别为 端口(p)，日志写入方式(l)，触发组合模式(m)，关闭连接(o)
// 数据库连接池数量(s)，线程池内的线程数量，是否关闭日志(c)，并发模型选择(a)
class Config
{
public:
    Config();
    ~Config();

    // 命令参数解析函数
    void parse_arg(int argc, char *argv[]);

    int port_;              //端口号
    int log_write_;         //日志写入方式
    int trig_mode_;         //触发组合模式
    int listen_trig_mode_;  //listen_fd触发模式
    int connect_trig_mode_; //connect_fd触发模式
    int opt_linger_;        //优雅关闭连接(TCP连接断开方式分为优雅的断开和强制断开两种方式)
    int sql_num_;           //数据库连接池数量
    int thread_num_;        //线程池内的线程数量
    int close_log_;         //是否关闭日志
    int actor_model_;       //并发模型选择
};

#endif