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
Linux> ./webbench -c 5000 -t 10  http://127.0.0.1:12000/
```
* 参数

> * `-c` 表示客户端数
> * `-t` 表示时间