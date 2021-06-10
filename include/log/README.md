## block_queue
一个线程安全的循环队列，操作和普通队列基本一致，不做过多说明。
## log
日志类，采用单例模式。  
单例模式是最常用的设计模式之一，保证一个类只有一个实例，并提供一个访问它的全局访问点，该实例被所有程序模块共享。  
实现思路：私有化它的构造函数，以防止外界创建单例类的对象；使用类的私有静态指针变量窒息那个类的唯一实例，并用一个公有的静态方法获取该实例。  
单例模式有两种实现方法，分别是懒汉和饿汉模式。顾名思义，懒汉模式，即非常懒，不用的时候不去初始化，所以在第一次被使用时才进行初始化；饿汉模式，即迫不及待，在程序运行时立即初始化。
我的实现是这样的：
```C++
class Log
{
public:
    //C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }
    //其他
private:
    Log();
    virtual ~Log();
    //其他
```
静态成员instance只会被生成一次保证了单例。不过需要注意的是C++11之前的在创建``instance``需要加锁，因为编译器不保证内部静态变量的线程安全。C++11之后不需要了。  


log包含如下变量：
```C++
char dir_name[128]; //路径名
char log_name[128]; //log文件名
int m_split_lines;  //日志最大行数
int m_log_buf_size; //日志缓冲区大小
long long m_count;  //日志行数记录
int m_today;        //因为按天分类,记录当前时间是那一天
FILE *m_fp;         //打开log的文件指针
char *m_buf;
block_queue<string> *m_log_queue;   //阻塞队列
locker m_mutex;                     //互斥锁
int m_close_log;                    //关闭日志
```
提供了如下成员函数：
### bool init(const char *file_name,int close_log,int log_buf_size=8192,int split_lines,int max_queue_size)
分为异步写入和同步写入两种方式,如果参数max_queue_size>=1表示采取异步。  
异步模式：  
初始化阻塞队列，并且创建一个线程（调用flush_log_thread）读取阻塞队列写日志。  
生成日志文件名（添加时间信息）。  
创建/打开日志文件。  

### void write_log(int level, const char *format, ...)
生成日志写入条目，包括类型（INFO、ERROR）、时间等信息。
判断当前日期和类的日期是否一致，不一致则刷新该文件写入缓冲区并关闭之前的文件描述符，打开新的文件。  
判断异步写入还是同步写入：
如果同步，直接写入文件。  
如果异步，将代写入信息push进阻塞队列。
### void flush(void)  
线程安全的刷新文件缓冲区（加互斥锁）。
### static void *flush_log_thread(void *args)
提供给写日志工作线程的调用函数。这个函数调用私有成员函数``async_write_log()``
### void *async_write_log()
异步写入日志函数，从阻塞队列取出日志条目，并写入日志。