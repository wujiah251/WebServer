#include "log.h"

// 构造函数
Buffer::Buffer(size_t len)
    : status(FREE), prev(NULL), next(NULL), total_len(len), used_len(0)
{
    data = new char[len];
    if (!data)
    {
        fprintf(stderr, "no space to allocate _data\n");
        exit(1);
    }
}

// 添加字符串到缓冲区
void Buffer::append(const char *log_line, size_t len)
{
    if (avail_len() < len)
        return;
    memcpy(data + used_len, log_line, len);
    used_len += len;
}

// 清空已用缓冲区
void Buffer::clear()
{
    used_len = 0;
    status = FREE;
}

// 持久化到磁盘文件
void Buffer::persist(FILE *fp)
{
    uint32_t write_len = fwrite(data, 1, used_len, fp);
    if (write_len != used_len)
    {
        fprintf(stderr, "write log to disk error, wt_len:%u,used_len:%ld\n", write_len, used_len);
    }
}
