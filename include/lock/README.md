## sem
信号量，提供函数如下：
```C++
sem();          //默认构造函数
sem(int num);   //构造函数，信号量初始为num
~sem(); //析构函数
bool wait();    //V操作，信号量-1
bool post();    //P操作，信号+1
```
## locker
互斥锁，提供函数如下：  
```C++
locker();   //构造函数
~locker()； //析构函数
lock()；    //上锁
unlock()；  //解锁
pthread_mutex_t *get()；    //返回互斥锁指针
```
## cond
条件变量，提供的函数如下：
```C++
cond();//构造函数
~cond();//析构函数
bool wait(pthread_mutex_t *m_mutex);//等待目标条件变量
bool timewait(pthread_mutex_t *m_mutex, struct timespec t);//等待目标条件变量一定时间
bool signal();//信号唤醒
bool broadcast();//信号唤醒
```
``wait(pthread_mutex_t *m_mutex)``内部调用``pthread_cond_wait()``，此函数内部操作分为以下步骤：
- 先传入一个互斥锁和条件变量，将线程放在条件变量的请求队列后，内部解锁
- 线程等待被pthread_cond_broadcast信号唤醒或者pthread_cond_signal信号唤醒，唤醒后去竞争锁，若竞争到锁，则内部再次加锁。


