/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: wifi_thread.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 11 日
**
** 描        述: 通信相关线程

** 日志:
2016.09.11  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include "wifi_thread.h"
#include "esp8266.h"

#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

rt_uint8_t wifi_stack[ 1024 ];	//线程栈
struct rt_thread wifi_thread; 	//线程控制块

/*******************************************************************************
* 函数名 	: wifi_thread_entry
* 描述   	: 通信相关线程
* 输入     	: - parameter: 线程入口参数
* 输出     	: None
* 返回值    : None
*******************************************************************************/
void wifi_thread_entry(void* parameter)
{
	init_esp8266();
	while(1)
	{
		WriteTest();
		rt_thread_delayMs(10);
		ReadTest();
		rt_thread_delayMs(10);
	}	
}
