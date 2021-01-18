#include "config.h"

Config::Config()
{
    // 默认端口号
    port_ = 9006;
    //日志写入方式
    log_write_ = 0;
    // 触发组合模式，默认listenfd LT + connectfd LT
    trig_mode_ = 0;
    // listenfd触发模式，默认LT
    listen_trig_mode_ = 0;
    // connectfd触发模式，默认LT
    connect_trig_mode_ = 0;
    // 优雅关闭连接，默认不使用
    opt_linger_ = 0;
    // 数据库连接池数量，默认8
    sql_num_ = 8;
    // 线程池内的线程数量，默认8
    thread_num_ = 8;
    // 关闭日志，默认不关闭
    close_log_ = 0;
    // 并发模型，默认是proactor
    actor_model_ = 0;
}

void Config::parse_arg(int argc, char *argv[])
{
    int opt;
    const char *optstr = "p:l:m:o:s:t:c:a";
    // 解析命令行参数，optarg是外部变量
    while ((opt = getopt(argc, argv, optstr)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port_ = atoi(optarg);
            break;
        case 'l':
            log_write_ = atoi(optarg);
            break;
        case 'm':
            trig_mode_ = atoi(optarg);
            break;
        case 'o':
            opt_linger_ = atoi(optarg);
            break;
        case 's':
            sql_num_ = atoi(optarg);
            break;
        case 't':
            thread_num_ = atoi(optarg);
            break;
        case 'c':
            close_log_ = atoi(optarg);
            break;
        case 'a':
            actor_model_ = atoi(optarg);
            break;
        default:
            break;
        }
    }
}