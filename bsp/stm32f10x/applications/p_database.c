/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: p_database.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 08 日
**
** 描        述: 用户信息数据库

** 日志:
2016.09.08  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include "p_database.h"
/* 文件系统相关头文件 */
#include <dfs.h>
#include <dfs_posix.h>
/* 延时函数 */
#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))
/* 设置调试信息打印开关 */
#ifdef printDebugInfo
#define p_printf(fmt,args...) rt_kprintf(fmt, ##args)
#else
#define p_printf(fmt,args...)
#endif

/* 用户信息数据库路径 */
const char *user_info_database_path = "/member.bin";

/* 只包含用户号、卡号的结构体数组,由卡号排序之后使用2分法搜索 */
struct card_id_struct card_id_array[MAX_USER_NUM];

/*******************************************************************************
* 函数名 	: bin_search
* 描述   	: 对分搜索
* 输入     	: - sSource: 由小到大排序好的数组 - array_size: 数组大小 - key:目标数值
* 输出     	: None
* 返回值    : -1: 未搜索到 其它: 数据所在数组的下标
*******************************************************************************/
int bin_search(struct card_id_struct sSource[], int array_size, int key)  
{     
    int low = 0, high = array_size - 1, mid;  
      
    while (low <= high)  
    {         
        mid = (low + high) / 2;//获取中间的位置  
          
        if (sSource[mid].card_id == key)              
            return mid; //找到则返回相应的位置  
        if (sSource[mid].card_id > key)            
            high = mid - 1; //如果比key大，则往低的位置查找  
        else  
            low = mid + 1;  //如果比key小，则往高的位置查找  
    }     
    return -1;    
}

/*******************************************************************************
* 函数名 	: quik_sort
* 描述   	: 快速排序算法
* 输入     	: - card_id_array: 需要排序的数组 - low: 数组下边界 - high:数组上边界
* 输出     	: None
* 返回值    : None
*******************************************************************************/
void quik_sort(struct card_id_struct card_id_array[],int low,int high)
{
	int i = low;
	int j = high;  
	struct card_id_struct temp = card_id_array[i]; 

	if (low < high)
	{
		while(i < j) 
		{
			while((card_id_array[j].card_id >= temp.card_id) && (i < j))
			{ 
				j--; 
			}
			card_id_array[i] = card_id_array[j];
			while((card_id_array[i].card_id <= temp.card_id) && (i < j))
			{
				i++; 
			}  
			card_id_array[j]= card_id_array[i];
		}
		card_id_array[i] = temp;
		quik_sort(card_id_array,low,i-1);
		quik_sort(card_id_array,j+1,high);
	}
	else
	{
		return;
	}
}

/*******************************************************************************
* 函数名 	: get_set_user_num
* 描述   	: 获取/设置数据库中的用户信息数量
* 输入     	: - user_num: 人员总数 - cmd: 0: 获取人员总数 1: 设置人员总数
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_user_num(u16 *user_num, u8 cmd)
{
	int fd;
	/* 打开人员信息数据库 */
	fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		p_printf("open %s failed\r\n", user_info_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, 0, SEEK_SET) == -1)
		{
			p_printf("lseek %s failed\r\n", user_info_database_path);
			close(fd);
			return -1;
		}
		else
		{
			if (cmd == 0)
			{
				/* 获取人员总数 */
				if (read(fd, user_num, 2) != 2)
				{
					p_printf("read %s failed\r\n", user_info_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					p_printf("get user num %d success\r\n", *user_num);			
				}		
			}
			else if (cmd == 1)
			{
				/* 写入人员总数 */
				if (write(fd, user_num, 2) != 2)
				{
					p_printf("write %s failed\r\n", user_info_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					p_printf("set user num %d success\r\n", *user_num);			
				}		
			}
			else
			{
				close(fd);
				p_printf("set user num cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	return 0;
}

/*******************************************************************************
* 函数名 	: get_set_user_num
* 描述   	: 获取/设置数据库中的最大用户号
* 输入     	: - user_num: 人员总数 - cmd: 0: 获取最大用户号 1: 设置最大用户号
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_user_num_max(u16 *user_num_max, u8 cmd)
{
	int fd;
	/* 打开人员信息数据库 */
	fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		p_printf("open %s failed\r\n", user_info_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, 2, SEEK_SET) == -1)
		{
			p_printf("lseek %s failed\r\n", user_info_database_path);
			close(fd);
			return -1;
		}
		else
		{
			if (cmd == 0)
			{
				/* 获取最大用户号 */
				if (read(fd, user_num_max, 2) != 2)
				{
					p_printf("read %s failed\r\n", user_info_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					p_printf("get user num max %d success\r\n", *user_num_max);			
				}		
			}
			else if (cmd == 1)
			{
				/* 写入最大用户号 */
				if (write(fd, user_num_max, 2) != 2)
				{
					p_printf("write %s failed\r\n", user_info_database_path);
					close(fd);
					return -1;
				}
				else
				{
					close(fd);
					p_printf("set user num max %d success\r\n", *user_num_max);			
				}		
			}
			else
			{
				close(fd);
				p_printf("set user num max cmd unknown\r\n");
				return -1;
			}
		}
	}
	
	return 0;
}

/*******************************************************************************
* 函数名 	: add_del_get_one_user
* 描述   	: 添加/删除/获取数据库中的一个用户(需要设置one_user_info->user_id参数)
* 输入     	: - one_user_info: 人员信息结构体 - cmd: 0: 添加 1: 删除 2: 获取
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 add_del_get_one_user(struct user_info *one_user_info, u8 cmd)
{
	int fd;

	/* 打开人员信息数据库 */
	fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		p_printf("open %s failed\r\n", user_info_database_path);
		close(fd);
		return -1;
	}
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, (sizeof(struct user_info)*(one_user_info->user_id-1)) + HEADER_SIZE, SEEK_SET) == -1)
		{
			p_printf("lseek %s failed\r\n", user_info_database_path);
			close(fd);
			return -1;
		}
		else
		{
			switch(cmd)
			{
				/* 添加用户 */
				case 0:
				{
					/* 写入用户信息 */
					if (write(fd, one_user_info, sizeof(struct user_info)) != sizeof(struct user_info))
					{
						p_printf("write %s failed\r\n", user_info_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						p_printf("add user %d success\r\n", one_user_info->user_id);			
					}
				}	break;
				/* 删除用户 */
				case 1:
				{
					/* 写入用户信息 */
					rt_memset(one_user_info, 0,sizeof(struct user_info));//将用户信息清零
					if (write(fd, one_user_info, sizeof(struct user_info)) != sizeof(struct user_info))
					{
						p_printf("write %s failed\r\n", user_info_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						p_printf("add user %d success\r\n", one_user_info->user_id);			
					}
				}	break;
				/* 读取用户信息 */
				case 2:
				{
					/* 读取用户信息 */
					if (read(fd, one_user_info, sizeof(struct user_info)) != sizeof(struct user_info))
					{
						p_printf("read %s failed\r\n", user_info_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						p_printf("get user %d success\r\n", one_user_info->user_id);			
					}
				}	break;
				default :
				{
					close(fd);
					p_printf("add_del_get one user cmd unknown\r\n");					
					return -1;
				}
			}
		}
	}
	
	return 0;
}

/*******************************************************************************
* 函数名 	: get_set_state
* 描述   	: 获取/设置指定用户的进出状态
* 输入     	: - state: 状态 - user_id: 用户号 - cmd: 0: 获取状态 1: 设置状态
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 get_set_state(u8 *state, u16 user_id, u8 cmd)
{
	int fd;

	/* 打开人员信息数据库 */
	fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		p_printf("open %s failed\r\n", user_info_database_path);
		close(fd);
		return -1;
	}	
	else
	{
		/* 移动文件指针到指定位置 */
		if (lseek(fd, (sizeof(struct user_info)*(user_id-1)) + OFFSET(user_info,state) + HEADER_SIZE, SEEK_SET) == -1)
		{
			p_printf("lseek %s failed\r\n", user_info_database_path);
			close(fd);
			return -1;
		}
		else
		{
			switch(cmd)
			{
				/* 获取进出状态 */
				case 0:
				{
					/* 读取进出状态 */
					if (read(fd, state, MEMBER_SIZE(user_info,state)) != MEMBER_SIZE(user_info,state))
					{
						p_printf("read %s failed\r\n", user_info_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						p_printf("get state %d success\r\n", user_id);			
					}
				}	break;
				/* 设置进出状态 */
				case 1:
				{
					/* 写入进出状态 */
					if (write(fd, state, MEMBER_SIZE(user_info,state)) != MEMBER_SIZE(user_info,state))
					{
						p_printf("write %s failed\r\n", user_info_database_path);
						close(fd);
						return -1;
					}
					else
					{
						close(fd);
						p_printf("set state %d success\r\n", user_id);			
					}
				}	break;
				default :
				{
					close(fd);
					p_printf("get set state cmd unknown\r\n");					
					return -1;
				}
			}
		}
	}
	
	return 0;
}




/*******************************************************************************
* 函数名 	: init_card_array
* 描述   	: 初始化卡号结构数组
* 输入     	: - card_array: 卡号结构数组
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 init_card_array(struct card_id_struct card_array[])
{
	int fd;
	int i;
	u16 user_num,max_user_num;
	u16 effective_num = 0;
	
	if(get_set_user_num(&user_num,GET_USER) == -1)
	{
		p_printf("get user num failed\r\n");
		return -1;
	}
	
	if(get_set_user_num_max(&max_user_num,GET_USER) == -1)
	{
		p_printf("get user num max failed\r\n");
		return -1;
	}
	/* 打开人员信息数据库 */
	fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
	if (fd < 0)
	{
		p_printf("open %s failed\r\n", user_info_database_path);
		close(fd);
		return -1;
	}
	else
	{
		for (i = 1; i<=max_user_num; i++)
		{
			/* 移动文件指针到指定位置 */
			if (lseek(fd, (sizeof(struct user_info)*(i-1)) + HEADER_SIZE, SEEK_SET) == -1)
			{
				p_printf("lseek %s failed\r\n", user_info_database_path);
				close(fd);
				return -1;
			}
			else
			{
				/* 读取用户信息 */
				if (read(fd, (void *)&card_array[effective_num], sizeof(struct card_id_struct)) != sizeof(struct card_id_struct))
				{
					p_printf("read %s failed\r\n", user_info_database_path);
					close(fd);
					return -1;
				}
				else
				{
					if (card_array[effective_num].user_id != 0)
					{
						effective_num++;
					}
					p_printf("get user %d success\r\n", i);			
				}
			}	
		}
		/* 判断用户数量是否正确 */
		if (effective_num != user_num)
		{
			p_printf("init card array failed! user_num is %d  effective_num is %d\r\n", user_num, effective_num);
		}
		else
		{
			/* 使用快速排序算法将数组排序 */
			quik_sort(card_array, 0, user_num - 1);
		}
	}
	
	close(fd);
	return 0;
}
