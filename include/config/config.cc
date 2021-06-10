#include "config.h"

Config::Config()
{
    //端口号,默认12000
    port = 12000;

    //数据库连接池数量,默认8
    sql_num = 6;

    //线程池内的线程数量,默认8
    thread_num = 6;
}

void Config::parse_arg(int argc, char *argv[])
{
    int opt;
    const char *str = "p:s:t:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        case 's':
        {
            sql_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}