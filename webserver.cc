#include "webserver.h"

WebServer::WebServer()
{
    // 创建用来保存http连接的数组
    users = new http_connect[kMax_Fd];
    getcwd(server_path, 200);
    char root[6] = "/root";
}