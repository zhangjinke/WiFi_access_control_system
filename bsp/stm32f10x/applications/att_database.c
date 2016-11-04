/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: att_database.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 11 月 03 日
**
** 描        述: 考勤信息数据库

** 日志:
2016.11.03  创建本文件
*********************************************************************************************************/

#include <rtthread.h>
#include "att_database.h"
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
#ifdef printAttDebugInfo
#define att_printf(fmt,args...) rt_kprintf(fmt, ##args)
#else
#define att_printf(fmt,args...)
#endif

/* 考勤信息数据库路径 */
const char *attendance_database_path = "/save_io.bin";
/* 考勤信息数据库header结构体 */
struct att_header att_header_t;

/*******************************************************************************
* 函数名 	: init_att_database
* 描述   	: 初始化考勤数据库
* 输入     	: None
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 init_att_database(void)
{
	int fd = -1;
	u32 crc = 0;
	
	/* 打开人员信息数据库 */
	fd = open(attendance_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		att_printf("open %s failed\r\n", attendance_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, 0, SEEK_SET) == -1)
		{
			att_printf("lseek %s failed\r\n", attendance_database_path);
			close(fd);
			return -1;
		}
		else
		{
			/* 获取考勤记录header信息 */
			read(fd, &att_header_t, sizeof(struct att_header));
			
			/* 计算CRC */
			crc = CalcBlockCRC((u8 *)(&att_header_t), sizeof(struct att_header) - 4);
			if (crc != att_header_t.crc)
			{
				att_printf("att header crc validators fail, init att database\r\n");
				/* 初始化考勤记录数据库 */
				att_header_t.total = 0;
				att_header_t.not_upload = 0;
				/* 计算CRC */
				att_header_t.crc = CalcBlockCRC((u8 *)(&att_header_t), sizeof(struct att_header) - 4);
				
				/* 移动文件指针到指定位置 */
				if (lseek(fd, 0, SEEK_SET) == -1)
				{
					att_printf("lseek %s failed\r\n", attendance_database_path);
					close(fd);
					return -1;
				}
				/* 写入考勤记录总条数 */
				if (write(fd, &att_header_t, sizeof(struct att_header)) != sizeof(struct att_header))
				{
					att_printf("write %s failed\r\n", attendance_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					att_printf("init att database success\r\n");			
				}		
			}
		}
	}
	
	return 0;
}

/*******************************************************************************
* 函数名 	: get_set_record_count
* 描述   	: 获取/设置数据库中考勤记录总条数
* 输入     	: - record_count: 考勤记录总条数 - cmd: 0: 获取 1: 设置
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_record_count(u32 *record_count, u8 cmd)
{
	int fd;
	
	/* 检查参数合法性 */
	if (record_count == 0)
	{
		att_printf("record_count addr is 0\r\n");
		return -1;
	}
	/* 打开人员信息数据库 */
	fd = open(attendance_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		att_printf("open %s failed\r\n", attendance_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, 0, SEEK_SET) == -1)
		{
			att_printf("lseek %s failed\r\n", attendance_database_path);
			close(fd);
			return -1;
		}
		else
		{
			if (cmd == 0)
			{
				/* 获取考勤记录总条数 */
				if (read(fd, record_count, MEMBER_SIZE(att_header, total)) != MEMBER_SIZE(att_header, total))
				{
					att_printf("read %s failed\r\n", attendance_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					att_printf("get record count %d success\r\n", *record_count);			
				}		
			}
			else if (cmd == 1)
			{
				/* 写入考勤记录总条数 */
				if (write(fd, record_count, MEMBER_SIZE(att_header, total)) != MEMBER_SIZE(att_header, total))
				{
					att_printf("write %s failed\r\n", attendance_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					att_printf("set record count %d success\r\n", *record_count);			
				}		
			}
			else
			{
				close(fd);
				att_printf("get set record count cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	return 0;
}
/*******************************************************************************
* 函数名 	: get_set_not_upload
* 描述   	: 获取/设置数据库中考勤记录未上传条数
* 输入     	: - record_count: 考勤记录未上传条数 - cmd: 0: 获取 1: 设置
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_not_upload(u32 *not_upload, u8 cmd)
{
	int fd;
	
	/* 检查参数合法性 */
	if (not_upload == 0)
	{
		att_printf("not_upload addr is 0\r\n");
		return -1;
	}
	/* 打开人员信息数据库 */
	fd = open(attendance_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		att_printf("open %s failed\r\n", attendance_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, OFFSET(att_header, not_upload), SEEK_SET) == -1)
		{
			att_printf("lseek %s failed\r\n", attendance_database_path);
			close(fd);
			return -1;
		}
		else
		{
			if (cmd == 0)
			{
				/* 获取考勤记录总条数 */
				if (read(fd, not_upload, MEMBER_SIZE(att_header, not_upload)) != MEMBER_SIZE(att_header, not_upload))
				{
					att_printf("read %s failed\r\n", attendance_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					att_printf("get not upload record count %d success\r\n", *not_upload);			
				}		
			}
			else if (cmd == 1)
			{
				/* 写入考勤记录总条数 */
				if (write(fd, not_upload, MEMBER_SIZE(att_header, not_upload)) != MEMBER_SIZE(att_header, not_upload))
				{
					att_printf("write %s failed\r\n", attendance_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					att_printf("set upload record record count %d success\r\n", *not_upload);			
				}		
			}
			else
			{
				close(fd);
				att_printf("get set upload record record count cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	return 0;
}

/*******************************************************************************
* 函数名 	: add_delone_att_record
* 描述   	: 添加/删除一条考勤记录
* 输入     	: - one_att_info: 考勤记录结构体 - cmd: 0: 获取 1: 添加
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 add_one_att_record(struct att_info *one_att_info, u8 cmd)
{
	int fd;
	
	/* 检查参数合法性 */
	if (one_att_info == 0)
	{
		att_printf("one_att_info addr is 0\r\n");
		return -1;
	}

	/* 打开人员信息数据库 */
	fd = open(attendance_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		att_printf("open %s failed\r\n", attendance_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, (sizeof(struct att_info) * (one_att_info->record_id)) + sizeof(struct att_header), SEEK_SET) == -1)
		{
			att_printf("lseek %s failed\r\n", attendance_database_path);
			close(fd);
			return -1;
		}
		else
		{
			switch(cmd)
			{
				/* 获取考勤记录 */
				case 0:
				{
					/* 读取 */
					if (read(fd, one_att_info, sizeof(struct att_info)) != sizeof(struct att_info))
					{
						att_printf("read %s failed\r\n", attendance_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						att_printf("get att record %d success\r\n", one_att_info->record_id);			
					}
				}	break;
				/* 添加考勤记录 */
				case 1:
				{
					/* 写入 */
					if (write(fd, one_att_info, sizeof(struct att_info)) != sizeof(struct att_info))
					{
						att_printf("write %s failed\r\n", attendance_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						att_printf("add att record %d success\r\n", one_att_info->user_id);			
					}
				}	break;
				default :
				{
					close(fd);
					att_printf("add delone att record cmd unknown\r\n");					
					return -1;
				}
			}
		}
	}
	
	return 0;
}
