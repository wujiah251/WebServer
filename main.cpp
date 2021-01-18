#include "config.h"
// 配置文件

int main(int argc, char *argv[])
{
    // 数据库信息，分别为数据库名称，登陆名，密码
    // 需要先在电脑中建立数据库
    string user = "wujiahao";
    string password = "wujiahao";
    string databasename = "webserver_database";

    // 创建配置类的实例和命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    // 初始化
}
