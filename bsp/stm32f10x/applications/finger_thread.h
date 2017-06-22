/*******************************************************************************
*                        WiFi Access Control System
*                       ----------------------------
*                                  EE Bang
*
* Contact information:
* web site:    http://www.cqutlab.cn/
* e-mail:      799548861@qq.com
*******************************************************************************/

/**
 * \file
 * \brief 指纹处理线程
 *
 * \internal
 * \par Modification history
 * - 1.00 17-06-09  zhangjinke, first implementation.
 * \endinternal
 */ 

#ifndef __FINGER_THREAD_H
#define __FINGER_THREAD_H

#include <rtthread.h>

extern rt_uint8_t rc522_stack[ 1024 ];    //线程栈
extern struct rt_thread rc522_thread;    //线程控制块

/* 刷卡或刷指纹事件控制块 */
extern struct rt_event user_check_event;
/* 读取出的卡号 */
extern uint32_t read_card_id;

extern void rc522_thread_entry(void* parameter);

#endif
