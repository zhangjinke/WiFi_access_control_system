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
	rt_err_t status = RT_EOK;
	rt_uint32_t recved_event = 0;
	
	/* 初始化ESP8266 */
	init_esp8266();
	while(1)
	{
		/* 等待刷卡或刷指纹事件 */
		status = rt_event_recv(	&esp8266_event,                       //事件对象的句柄
								hspi_rx,                              //接收线程感兴趣的事件
								RT_EVENT_FLAG_CLEAR|RT_EVENT_FLAG_OR, //逻辑或、清除事件
								RT_WAITING_FOREVER,                   //永不超时
								&recved_event                         //指向收到的事件
							  );
		/* 根据接收到的事件执行不同的处理 */
		if(status == RT_EOK)
		{
			/* hspi接收事件 */
			if (recved_event & hspi_rx)
			{
				esp8266_spi_read();
			}
		}
	}	
}
