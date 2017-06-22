#ifndef _RFID_THREAD_H_
#define _RFID_THREAD_H_

#include <rtthread.h>
#include <stdint.h>

#define USER_CHECK_CARD_IN         (1 << 0)    //门内刷卡事件
#define USER_CHECK_CARD_OUT         (1 << 1)   //门外刷卡事件

extern rt_uint8_t rc522_stack[ 1024 ];    //线程栈
extern struct rt_thread rc522_thread;     //线程控制块

/* 读取出的卡号 */
extern uint32_t g_read_card_id;

extern void rc522_thread_entry(void* parameter);

#endif
