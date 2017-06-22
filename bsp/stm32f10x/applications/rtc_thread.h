#ifndef _RTC_THREAD_H_
#define _RTC_THREAD_H_

#include <rtthread.h>
#include "ds1307.h"

extern Time_Typedef TimeValue;  //定义时间缓存指针(ds1307.h)

extern rt_uint8_t rtc_stack[ 1024 ];    //线程栈
extern struct rt_thread rtc_thread;     //线程控制块
extern void rtc_thread_entry(void* parameter);

#endif
