/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: user_check_thread.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 08 日
**
** 描        述: 接收到刷卡或刷指纹事件之后进行权限检测，通过则开门

** 日志:
2016.09.08  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include "user_check_thread.h"
#include "rfid_thread.h"
#include "p_database.h"
#include "device_config.h"
/* 文件系统相关头文件 */
#include <dfs.h>
#include <dfs_posix.h>
/* 蜂鸣器、门锁相关头文件 */
#include "beep_door.h"

/* 延时函数 */
#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

rt_uint8_t user_check_stack[ 1024 ];	//线程栈
struct rt_thread user_check_thread; 	//线程控制块

/* 定义一个人员信息结构体 */
struct user_info user_info_struct_get;
u16 user_num;

/*******************************************************************************
* 函数名 	: user_check_thread_entry
* 描述   	: 接收到刷卡或刷指纹事件之后进行权限检测，通过则开锁
* 输入     	: - parameter: 线程入口参数
* 输出     	: None
* 返回值    : None
*******************************************************************************/
void user_check_thread_entry(void* parameter)
{
	rt_err_t status = RT_EOK;
	rt_uint32_t recved_event = 0;
	u16 user_num = 0, max_user_num = 0;
	s32 search_result = 0;
	u32 i;
		
	if(get_set_user_num(&user_num,GET_USER) == -1)
	{
		rt_kprintf("get user num failed\r\n");
	}
	
	if(get_set_user_num_max(&max_user_num,GET_USER) == -1)
	{
		rt_kprintf("get user num max failed\r\n");
	}
	while(1)
	{
		/* 等待刷卡或刷指纹事件 */
		status = rt_event_recv(	&user_check_event,						//事件对象的句柄
								card_in_check|card_out_check,			//接收线程感兴趣的事件
								RT_EVENT_FLAG_CLEAR|RT_EVENT_FLAG_OR,	//逻辑或、清除事件
								RT_WAITING_FOREVER,						//永不超时
								&recved_event							//指向收到的事件
							  );
		/* 根据接收到的事件执行不同的处理 */
		if(status == RT_EOK)
		{
			/* 刷卡事件 */
			if ((recved_event&card_in_check) || (recved_event&card_out_check))
			{
				/* 搜索卡号对应的用户号 */
				search_result = bin_search(card_id_array, user_num, read_card_id);
				if(search_result == -1)
				{
					if(recved_event&card_in_check) { rt_kprintf("in card id %X is not exist\r\n", read_card_id); }
					else { rt_kprintf("out card id %X is not exist\r\n", read_card_id); }
				}
				else	/* 搜索到对应的用户号 */
				{
					/* 获取详细用户信息 */
					user_info_struct_get.user_id = card_id_array[search_result].user_id;
					add_del_get_one_user(&user_info_struct_get, GET_ONE_USER);
					/* 判断card id是否正确 */
					if (user_info_struct_get.card_id != card_id_array[search_result].card_id) { continue; }
					/* 判断是否是超级管理员 */
					if ((user_info_struct_get.authority[15]&(1 << 7)) == (1 << 7) )
					{//最高位为1则为超级管理员，直接开门
						rt_kprintf("\r\n超级管理员%s，欢迎光临，\r\n", user_info_struct_get.name);
						/* 开锁 */
						open_door();
						continue;
					}
					/* 判断用户是否激活 */
					if (user_info_struct_get.effective != 1)
					{
						rt_kprintf("\r\n未激活！\r\n");
						continue;
					}
					/* 判断是否是有限时长用户 */
					if (user_info_struct_get.is_time_limit == 1)
					{
						/* 如果过期，提示 */
						if(0)
						{
							rt_kprintf("\r\n超期！\r\n");
							continue;					
						}
					}
					/* 确认权限 */
					if ((user_info_struct_get.authority[device_addr/8]&(1 << (device_addr%8))) != (1 << (device_addr%8)) )
					{
						rt_kprintf("\r\n权限不足！\r\n");
						continue;
					}
					/* 检查进出状态 */
					switch(recved_event)
					{
						/* 门内刷卡 */
						case card_in_check:
						{
							if ((user_info_struct_get.state[device_addr/8]&(1 << (device_addr%8))) != (1 << (device_addr%8)) )
							{	//0:门外 1:门内    如果在门外
								rt_kprintf("\r\n进门未刷卡！\r\n");
								continue;
							}
							else
							{
								/* 将状态设置为门外 */
								user_info_struct_get.state[device_addr/8] &=~ (1 << (device_addr%8));
								get_set_state(user_info_struct_get.state,user_info_struct_get.user_id,SET_USER);
								/* 开锁 */
								open_door();
							}
						}	break;
						/* 门外刷卡 */
						case card_out_check:
						{
							if ((user_info_struct_get.state[device_addr/8]&(1 << (device_addr%8))) == (1 << (device_addr%8)) )
							{	//0:门外 1:门内    如果在门内
								rt_kprintf("\r\n出门未刷卡！\r\n");
								continue;
							}
							else
							{
								/* 将状态设置为门内 */
								user_info_struct_get.state[device_addr/8] |= (1 << (device_addr%8));
								get_set_state(user_info_struct_get.state,user_info_struct_get.user_id,SET_USER);
								/* 开锁 */
								open_door();
							}
						}
						default : break;
					}
										
					/* 打印相关信息 */
					rt_kprintf("\r\nuser id is %d\r\n", user_info_struct_get.user_id);
					rt_kprintf("card id is %X\r\n", user_info_struct_get.card_id);
					rt_kprintf("effective is %d\r\n", user_info_struct_get.effective);
					rt_kprintf("name is %s\r\n", user_info_struct_get.name);
					rt_kprintf("student id is %s\r\n", user_info_struct_get.student_id);
					/* 打印权限 */
					rt_kprintf("authority is \r\n"); 
					for (i = 0; i<sizeof(user_info_struct_get.authority); i++)
					{
						rt_kprintf("%02X ", user_info_struct_get.authority[i]);
					}
					rt_kprintf("\r\n"); 
				}
			}
		}
	}	
}

