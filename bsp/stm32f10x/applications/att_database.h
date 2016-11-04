/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: att_database.h
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 11 月 03 日
**
** 描        述: 考勤信息数据库

** 日志:
2016.11.03  创建本文件
*********************************************************************************************************/

#ifndef _ATT_DATABASE_H_
#define _ATT_DATABASE_H_

#include <rtthread.h>
#include <sys.h>

/* 打印调试信息 */
#define printAttDebugInfo

#define GET_RECORD			0	//获取记录
#define SET_RECORD			1	//设置记录

__packed struct att_info
{
	u8  is_upload;			//是否上传
	u8  is_delete;			//是否删除
	u32 record_id;			//考勤记录ID号
	u16 user_id;			//用户号
	u8  student_id[16];		//学号 11个数字(前补0)
	u8  name[8];			//姓名 右边补0x00
	u8  state[16];			//进出状态
	u16 year;				//年(有效期，超过之后用户失效)
	u8  month;				//月
	u8  day;				//日
	u8  hour;				//时
	u8  minutes;			//分
	u8  second;				//秒
	u32 crc;				//本成员之前所有成员按字节计算的CRC值
};
__packed struct att_header
{
	u32 total;				//总条数
	u32 not_upload;			//未上传条数
	u32 crc;				//crc校验
};

extern struct att_header att_header_t;                               /* 考勤信息数据库header结构体 */

extern s8 init_att_database(void);                                   /* 初始化考勤数据库 */
extern s8 get_set_record_count(u32 *record_count, u8 cmd);           /* 获取/设置数据库中考勤记录总条数 */
extern s8 get_set_not_upload(u32 *not_upload, u8 cmd);               /* 获取/设置数据库中考勤记录未上传条数 */
extern s8 add_one_att_record(struct att_info *one_att_info, u8 cmd); /* 添加/删除一条考勤记录 */

#endif
