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

#include "stm32_crc.h"

/* 获取结构体成员偏移宏定义 */
#define OFFSET(Type, member) ( (u32)&(((struct Type*)0)->member) )
#define MEMBER_SIZE(Type, member) sizeof(((struct Type*)0)->member)

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

struct device_config device_config_t;
/*******************************************************************************
* 函数名 	: init_device_config
* 描述   	: 初始化设备配置
* 输入     	: None
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 init_device_config(void)
{
	int fd = -1;
	u32 crc = 0;
	
	/* 打开设备配置 */
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
			/* 获取设备配置信息 */
			read(fd, &device_config_t, sizeof(struct device_config));
			
			/* 计算CRC */
			crc = CalcBlockCRC((u8 *)(&device_config_t), sizeof(struct device_config) - 4);
			if (crc != device_config_t.crc)
			{
				cfg_printf("device config crc validators fail, init device config\r\n");
				/* 初始化设备配置信息 */
				device_config_t.device_addr = 0;
				rt_memset(device_config_t.this_device_name, 0, MEMBER_SIZE(device_config, this_device_name));
				/* 计算CRC */
				device_config_t.crc = CalcBlockCRC((u8 *)(&device_config_t), sizeof(struct device_config) - 4);
				
				/* 移动文件指针到指定位置 */
				if (lseek(fd, 0, SEEK_SET) == -1)
				{
					cfg_printf("lseek %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				/* 写入设备配置 */
				if (write(fd, &device_config_t, sizeof(struct device_config)) != sizeof(struct device_config))
				{
					cfg_printf("write %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					cfg_printf("init device config success\r\n");			
				}		
			}
			else
			{
				close(fd);
			}
		}
	}
	
	return 0;
}

/*******************************************************************************
* 函数名 	: get_set_device_config
* 描述   	: 获取/设置设备配置
* 输入     	: - device_config: 设备配置结构体 - cmd: 0: 获取设备配置 1: 设置设备配置
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_device_config(struct device_config *device_config_t, u8 cmd)
{
	int fd;
	
	/* 检查参数合法性 */
	if (device_config_t == 0)
	{
		cfg_printf("device_config_t addr is 0\r\n");
		return -1;
	}
	/* 打开人员信息数据库 */
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
				/* 获取考勤记录总条数 */
				if (read(fd, device_config_t, sizeof(struct device_config)) != sizeof(struct device_config))
				{
					cfg_printf("read %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					if (device_config_t->crc == CalcBlockCRC((u8 *)device_config_t, sizeof(struct device_config) - 4))
					{
						cfg_printf("get device config addr is %d, name is %s success\r\n",device_config_t->device_addr, device_config_t->this_device_name);
					}
					else
					{
						cfg_printf("crc validators fail addr is %d, name is %s success\r\n",device_config_t->device_addr, device_config_t->this_device_name);
						return -1;
					}
				}		
			}
			else if (cmd == 1)
			{
				/* 计算CRC */
				device_config_t->crc = CalcBlockCRC((u8 *)device_config_t, sizeof(struct device_config) - 4);
				/* 写入考勤记录总条数 */
				if (write(fd, device_config_t, sizeof(struct device_config)) != sizeof(struct device_config))
				{
					cfg_printf("write %s failed\r\n", device_config_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					cfg_printf("set device config addr is %d, name is %s success\r\n",device_config_t->device_addr, device_config_t->this_device_name);
				}		
			}
			else
			{
				close(fd);
				cfg_printf("get set device config cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	return 0;
}
