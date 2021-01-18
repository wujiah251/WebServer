#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>
#include "log.h"

using namespace std;

Log::Log()
{
    count_ = 0;
    is_async_ = false;
}
Log::~Log()
{

    if (file_ptr_ != NULL)
    {
        fclose(file_ptr_);
    }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    if (max_queue_size >= 1)
    {
        is_asunc = true;
    }
}
