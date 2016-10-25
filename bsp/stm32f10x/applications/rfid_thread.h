#ifndef _RFID_THREAD_H_
#define _RFID_THREAD_H_

#include <rtthread.h>
#include <sys.h>

#define card_in_check 		(1 << 0)	//门内刷卡事件
#define card_out_check 		(1 << 1)	//门外刷卡事件
#define finger_in_check 	(1 << 2)	//门内刷指纹事件
#define finger_out_check 	(1 << 3)	//门外刷指纹事件

extern rt_uint8_t rc522_stack[ 1024 ];	//线程栈
extern struct rt_thread rc522_thread;	//线程控制块

/* 刷卡或刷指纹事件控制块 */
extern struct rt_event user_check_event;
/* 读取出的卡号 */
extern u32 read_card_id;

extern void rc522_thread_entry(void* parameter);

#endif
