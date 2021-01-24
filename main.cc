#include "config.h"
// 配置文件

int main(int argc, char *argv[])
{
    // 数据库信息，分别为数据库名称，登陆名，密码
    // 需要先在电脑中建立数据库
    string user = "root";
    string password = "wujiahao";
    string database_name = "webserver_database";

    // 创建配置类的实例和命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;
    // 初始化
    server.init(config.port_, user, password, database_name, config.log_write_,
                config.opt_linger_, config.trig_mode_, config.sql_num_,
                config.thread_num_, config.close_log_, config.actor_model_);
    // 日志
    server.log_write();
    // 数据库
    server.sql_pool();
    // 线程池
    server.thread_pool();
    // 触发模式
    server.trig_mode();
    // 监听
    server.event_listen();
    // 运行
    server.event_loop();
    return 0;
}
