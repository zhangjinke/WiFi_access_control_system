/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: beep_door.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 20 日
**
** 描        述: 蜂鸣器、电子锁驱动

** 日志:
2016.09.20  创建本文件
*********************************************************************************************************/

#include "beep_door.h"
#include <rtthread.h>
#include "finsh.h"

/* 延时函数 */
#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

//初始化蜂鸣器、电子锁控制IO为输出口.并使能这两个口的时钟		    
void rt_hw_beep_door_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);	 //使能PE端口时钟

	BEEP = 0;
	DOOR = 1;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5;	 	//端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 	//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 	//IO口速度为50MHz
	GPIO_Init(GPIOE, &GPIO_InitStructure);					 	//根据设定参数初始化

	BEEP = 0;
	DOOR = 1;
}

/* 开锁 */
void open_door(void)
{
	BEEP = 1;
	DOOR = 0;
	
	rt_thread_delayMs(100);
	
	BEEP = 0;
	DOOR = 1;
}
FINSH_FUNCTION_EXPORT(open_door, open door and beep)
