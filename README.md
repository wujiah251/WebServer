# WebServer
本项目是基于C++的一个轻量级Web服务器。
开始时间：2021.1.16。
good luck！

## 如何使用

- 服务器测试环境  
 - Ubuntu版本16.04
 - MySQL版本5.7.32
- 浏览器测试
 - FireFox
- 创建数据库  
```mysql
create database webserver_database;
// 创建一个名为webserver_database的数据库
user database webserver_database;
// 在这个数据库中创建表user，关系为(username,passwd)
// 保存用户名-密码
create table user(
	username char(50) NULL,
   passwd char(50) NULL
);
// 添加一条数据，内容可以填上任意的用户名和密码
INSERT INTO user(username, passwd) VALUES('wujiahao','wujiahao');
```  
这个数据库只有一个表user，其保存的是用户的用户名、密码。  
- 文件管理  
 - makefile
 - 运行方式
 ```shell
 linux> make
 linux> ./server
 ```

## 压力测试

------------
* 测试示例
```Linux
Linux> ./server -p 12000
// 打开一个新的终端
Linux> ./webbench -c 12000 -t 10  http://127.0.0.1:12000/
```
* 参数

> * `-c` 表示客户端数
> * `-t` 表示时间


## 项目起因

本项目为C++编写的多线程Web服务器，总代码规模为2500-3000。写这个项目是因为前几个月把CSAPP、操作系统、计算机网络学了一遍，想要做个项目来融会贯通，故选择实现一个多线程服务器。具体实现参考了游双的《Linux高性能Web服务器编程》。  
做这个之前可以先写一下CSAPP第11章的那个轻量级web服务器，收益良多。  

## 项目细节

1. 用epoll来实现事件IO复用，ET模式；  
2. 采取同步方式实现Proactor模式；
3. 日志采取异步写入的模式；
4. 定时器采取升序链表实现；
5. 连接关闭方式为直接关闭；
6. 提供数据库连接池、线程池；

