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

#define GET_DEVICE		0	//获取设备xxx
#define SET_DEVICE		1	//设置设备xxx


extern u8 device_addr;
extern char this_device_name[64];

extern s8 get_set_device_addr(u8 *device_addr, u8 cmd);	//获取/设置设备地址
extern s8 get_set_device_name(char *device_name, u8 cmd);	//获取/设置设备名称


#endif
