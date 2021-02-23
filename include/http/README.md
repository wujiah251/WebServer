## Http_connect

``http_connect.h``、``http_connect.cc``中实现了http服务类-http_connect(为连接提供http服务，一个连接对应一个http_connect对象），还是实现了``setnoblocking()``、``addfd()``、``removefd()``、``modfd()``等功能函数。  
上述功能函数用于设置epoll描述符的相关属性、向epoll内核事件表中添加/删除事件的功能。



提供以下成员函数


### ``void http_connect::initmysql_result(connection_pool* connPool)``

初始化对象的mysql连接，并将用户数据库数据读入到外部变量查找表users中。  




### ``void http_connect::close_conn()``

关闭一个连接

## ``void http_connect::init(int sockfd, const sockaddr_in &addr, char *root, int TRIGMode,int close_log, string user, string passwd, string sqlname)``

初始化连接：  
1. 将文件描述符添加到内核事件表中（addfd()）；  
2. 设置http响应方式；  
3. 设置日志是否关闭；  
4. 设置数据库信息；  
5. 调用init()初始化状态信息以及缓冲区。  

### ``void http_connect::init()``

初始化http_connect对象的各种信息以及缓冲区。  

### ``http_connect::LINE_STATUS http_connect::parse_line()``

状态机
。。。。。。

### ``bool http_connect::read_once()``

读取客户数据，直到数据读取完毕或者无数据可读。  
1. 读缓冲区用完，返回false；  
2. 选择读取方式LT或者ET；  
3. 选择LT模式则只调用一次recv读取数据，并返回；  
4. 选择ET模式则循环调用recv直到读取完毕或者发生错误；  

### ``http_connect::HTTP_CODE http_connect::parse_request_line()``

读取http请求报文，并解析返回状态码。  

1. 检测是否是有效http请求，否则返回BAD_REQUEST；  
2. 读取url和http版本；  
3. 确认是否为主界面url（登陆/注册）；  
4. 修改请求行状态m_check_state为CHECK_STATE_HEADER并返回NO_REQUEST;

### ``http_connect::HTTP_CODE http_connect::parse_headers(char *text)``

解析一个http请求的首部行信息。  
1. 为空则返回GET_REQUESt；    
2. 匹配Connection（长连接或者短连接）；    
3. 匹配Content-length（实体部分长度）；  
4. 匹配Host（客户主机）；  
5. 如果前面无法匹配，则向日志写入信息；  
6. 返回NO_REQUEST。  

### ``http_connect::HTTP_CODE http_connect::parse_content(char *text)``

判断http请求是否被完整的读入缓冲区。

### ``http_connect::HTTP_CODE http_connect::process_read()``

1. 定位到读缓冲区内开始位置；  
2. 修改开始位置到m_checked_idx；  
3. 判断http请求的读取状态；  
4. 如果是正在解析请求行，则调用parse_request_line()；  
5. 如果是正在解析首部行，则调用parse_headers(text)，若返回GET_REQUEST则调用do_request()执行响应；  
6. 如果是正在解析实体，则调用parse_content()，若返回GET_REQUEST则调用do_request()执行响应，者之LINE_OPEN；  
7. 如果都不是，返回内部错误。  

### ``void http_connect::unmap()``

释放读入到内存的被请求资源。

### ``bool http_connect::add_reponse(const char *format, ...)``

向要返回的报文中添加响应，即写入缓冲区中。
还有相关函数``add_status_line()``、``add_headers()``、``add_content()``，分别表示写入状态行，写入首部行，写入实体。

