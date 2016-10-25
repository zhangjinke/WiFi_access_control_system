/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: device_config.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 19 日
**
** 描        述: 设备配置文件相关

** 日志:
2016.09.19  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include "device_config.h"
/* 文件系统相关头文件 */
#include <dfs.h>
#include <dfs_posix.h>
/* 延时函数 */
#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

/* 设置调试信息打印开关 */
#ifdef printCfgDebugInfo
#define cfg_printf(fmt,args...) rt_kprintf(fmt, ##args)
#else
#define cfg_printf(fmt,args...)
#endif

/* 设备配置文件路径 */
const char *device_config_path = "/config.bin";

/* 设备地址 */
u8 device_addr = 0;
/* 设备名称 */
char this_device_name[64];

/*******************************************************************************
* 函数名 	: get_set_device_addr
* 描述   	: 获取/设置设备地址
* 输入     	: - device_addr: 设备地址 - cmd: 0: 获取设备地址 1: 设置设备地址
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_device_addr(u8 *device_addr, u8 cmd)
{
	int fd;
	/* 打开设备配置文件 */
	fd = open(device_config_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		cfg_printf("open %s failed\r\n", device_config_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, 0, SEEK_SET) == -1)
		{
			cfg_printf("lseek %s failed\r\n", device_config_path);
			close(fd);
			return -1;
		}
		else
		{
			if (cmd == 0)
			{
				/* 获取设备地址 */
				if (read(fd, device_addr, 1) != 1)
				{
					cfg_printf("read %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					cfg_printf("get device addr %d success\r\n", *device_addr);			
				}		
			}
			else if (cmd == 1)
			{
				/* 写入设备地址 */
				if (write(fd, device_addr, 1) != 1)
				{
					cfg_printf("write %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					cfg_printf("set device addr %d success\r\n", *device_addr);			
				}		
			}
			else
			{
				close(fd);
				cfg_printf("set device addr cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	close(fd);
	return 0;
}


/*******************************************************************************
* 函数名 	: get_set_device_name
* 描述   	: 获取/设置设备名称
* 输入     	: - device_name: 设备名称 - cmd: 0: 获取设备名称 1: 设置设备名称
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_device_name(char *device_name, u8 cmd)
{
	int fd;
	/* 打开设备配置文件 */
	fd = open(device_config_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		cfg_printf("open %s failed\r\n", device_config_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, 1, SEEK_SET) == -1)
		{
			cfg_printf("lseek %s failed\r\n", device_config_path);
			close(fd);
			return -1;
		}
		else
		{
			if (cmd == 0)
			{
				/* 获取设备名称 */
				if (read(fd, device_name, 64) != 64)
				{
					cfg_printf("read %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					cfg_printf("get device name %s success\r\n", device_name);			
				}		
			}
			else if (cmd == 1)
			{
				/* 写入设备名称 */
				if (write(fd, device_name, 64) != 64)
				{
					cfg_printf("write %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					cfg_printf("set device name %s success\r\n", device_name);			
				}		
			}
			else
			{
				close(fd);
				cfg_printf("set device name cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	return 0;
}

