#ifndef _WIFI_THREAD_H_
#define _WIFI_THREAD_H_

#include <rtthread.h>
#include <sys.h>

__packed struct wifi_pack
{
	u32  crc;				/* crc校验 */
	u8   dst[6];			/* 目标地址 */
	u8   src[6];			/* 源地址 */
	u8   cmd;				/* 命令 */
	u16  lenth;			    /* 长度 */
	u8  *data;				/* 数据 */
};

extern rt_uint8_t wifi_stack[ 1024 ];	//线程栈
extern struct rt_thread wifi_thread; 	//线程控制块
extern void wifi_thread_entry(void* parameter);

#endif
