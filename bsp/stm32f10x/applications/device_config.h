/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: device_config.h
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 19 日
**
** 描        述: 设备配置文件相关

** 日志:
2016.09.19  创建本文件
*********************************************************************************************************/

#ifndef _DEVICE_CONFIG_
#define _DEVICE_CONFIG_

#include <rtthread.h>
#include <sys.h>

/* 打印调试信息 */
#define printCfgDebugInfo

#define GET_DEVICE		0			//获取设备xxx
#define SET_DEVICE		1			//设置设备xxx

__packed struct device_config
{
	u8  device_addr;				//本机地址
	s8  this_device_name[64];			//本机名称
	u32 crc;						//crc校验
};

extern struct device_config device_config_t;

extern s8 init_device_config(void);                                      /* 初始化设备配置 */
extern s8 get_set_device_config(struct device_config *device_config_t, u8 cmd); /* 获取/设置设备配置 */


#endif
