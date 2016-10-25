/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: p_database.h
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 08 日
**
** 描        述: 用户信息数据库

** 日志:
2016.09.08  创建本文件
*********************************************************************************************************/

#ifndef _P_DATABASE_H_
#define _P_DATABASE_H_

#include <rtthread.h>
#include <sys.h>

/* 打印调试信息 */
//#define printDebugInfo
/* 最大用户总量 */
#define MAX_USER_NUM		1000

#define GET_USER			0	//获取用户
#define SET_USER			1	//设置用户

#define ADD_ONE_USER		0	//添加用户
#define DEL_ONE_USER		1	//删除用户
#define GET_ONE_USER		2	//获取用户信息

/* 获取结构体成员偏移宏定义 */
#define OFFSET(Type, member) ( (u32)&(((struct Type*)0)->member) )
#define MEMBER_SIZE(Type, member) sizeof(((struct Type*)0)->member)

/*用户号	激活状态	卡号	学号	姓名	权限	进出状态	有效期	指纹特征值
2字节	1字节	4字节	6字节	8字节	16字节	16字节	7字节	193*5字节
1-1000	0:失效 1:有效	11个数字(前补0)	右边补0x00	每一位对应一个门(最多128个门)	每一位对应一个门0:门内 1:门外	年月日时分秒	最多5个指纹	*/

__packed struct user_info //总1040字节
{
	u16 user_id;		// 4-5			//用户号
	u32 card_id;		// 6-9			//卡号
	u8 effective;		// 10			//激活状态 0:失效 1:有效
	u8 student_id[16];	// 11-26		//学号 11个数字(前补0)
	u8 name[8];			// 27-34		//姓名 右边补0x00
	u8 authority[16];	// 35-50		//权限
	u8 state[16];		// 51-66		//进出状态
	u8 is_time_limit;	// 67			//是否有时间限制 0:无限制 1:有限制
	u16 year;			// 68-69		//年(有效期，超过之后用户失效)
	u8 month;			// 70			//月
	u8 day;				// 71			//日
	u8 hour;			// 72			//时
	u8 minutes;			// 73			//分
	u8 second;			// 74			//秒
	u8 finger[5][193];	// 75-1039		//指纹特征值
	u32 crc;			// 1040-1043	//本成员之前所有成员按字节计算的CRC值
};

__packed struct card_id_struct //总6字节
{
	u16 user_id;		//用户号
	u32 card_id;		//卡号
};

extern const char *user_info_database_path;

/* 只包含用户号、卡号的结构体数组,由卡号排序之后使用2分法搜索 */
extern struct card_id_struct card_id_array[MAX_USER_NUM];

int bin_search(struct card_id_struct sSource[], int array_size, int key);

extern s8 get_set_user_num(u16 *user_num, u8 cmd);//获取/设置数据库中的人员信息
extern s8 get_set_user_num_max(u16 *user_num_max, u8 cmd);//获取/设置数据库中的最大用户号
extern s8 add_del_get_one_user(struct user_info *one_user_info, u8 cmd);//添加/删除/获取数据库中的一个用户(需要设置one_user_info->user_id参数)
s8 get_set_state(u8 *state, u16 user_id, u8 cmd);
extern s8 init_card_array(struct card_id_struct card_array[]);//初始化卡号结构数组

#endif
