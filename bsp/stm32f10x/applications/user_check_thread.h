#ifndef _USER_CHECK_THREAD_H_
#define _USER_CHECK_THREAD_H_

#include <rtthread.h>

extern rt_uint8_t user_check_stack[ 1024 ];    //线程栈
extern struct rt_thread user_check_thread;     //线程控制块

extern void user_check_thread_entry(void* parameter);

#endif
