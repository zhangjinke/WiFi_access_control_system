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
#include "p_database.h"

#include <rtthread.h>
#include <finsh.h>
#include <dfs.h>
#include <dfs_posix.h>
#include <stdlib.h>
#include "global.h"

/** \brief 设置调试信息打印开关 */
#ifdef printDebugInfo
#define p_printf(fmt,args...) rt_kprintf("p_printf: "); rt_kprintf(fmt, ##args)
#else
#define p_printf(fmt,args...)
#endif

/** \brief 用户信息数据库路径 */
const char *user_info_database_path = "/member.bin";

/** \brief 只包含卡号、用户号的结构体数组,由卡号排序之后使用2分法搜索 */
card_id_user_id_link_t g_card_id_user_id_list[MAX_USER_NUM];

/** \brief 只包含指纹号、用户号的结构体数组,由指纹号排序之后使用2分法搜索 */
finger_id_user_id_link_t g_finger_id_user_id_list[MAX_USER_NUM];

/**
 * \brief 卡号-用户号数组二分搜索与快速排序的比较函数(递减)
 *
 * \param[in] p_a 成员a
 * \param[in] p_b 成员b
 *
 * \return a大于b返回小于0，a等于b返回0，a小于b返回大于0
 */
int card_compar (const void *p_a, const void *p_b)
{
    if (((card_id_user_id_link_t *)p_a)->card_id > ((card_id_user_id_link_t *)p_b)->card_id) {
        return -1;
    } else 
    if (((card_id_user_id_link_t *)p_a)->card_id < ((card_id_user_id_link_t *)p_b)->card_id) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * \brief 获取用户数量与最大用户号
 *
 * \param[out] p_user_num    用户数量
 * \param[out] p_max_user_id 最大用户号
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t user_num_get (uint16_t *p_user_num, uint16_t *p_max_user_id)
{
    int fd = 0;
    struct stat file_info;
    uint16_t user_id = 0;
    uint16_t user_num = 0;
    uint16_t max_user_num = 0;
    uint32_t i = 0;
    uint32_t addr = 0;
    
    /* 获取文件信息 */
    if (stat(user_info_database_path, &file_info) < 0) {
        p_printf("stat %s failed\r\n", user_info_database_path);
        return -1;
    }
    
    /* 打开人员信息数据库 */
    fd = open(user_info_database_path, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        p_printf("open %s failed\r\n", user_info_database_path);
        close(fd);
        return -1;
    }
    
    /* 搜索计算用户数与最大用户号 */
    for (i = 0; i < (file_info.st_size - HEADER_SIZE) / sizeof(user_info_t); i++) {
        addr = (sizeof(user_info_t) * i) + HEADER_SIZE;

        /* 移动文件指针到指定位置 */
        if (lseek(fd, addr, SEEK_SET) == -1) {
            p_printf("lseek addr %d %s failed\r\n", addr, user_info_database_path);
            goto failed;
        } 

        /* 获取用户号 */
        if (read(fd, &user_id, 2) != 2)
        {
            p_printf("read %s failed\r\n", user_info_database_path);
            goto failed;
        }
        
        if (user_id != 0) {
            user_num++;
            if (user_id > max_user_num) {
                max_user_num = user_id;
            }
        }
    }
    
    if (RT_NULL != p_user_num) {
        *p_user_num = user_num;
    }
    if (RT_NULL != p_max_user_id) {
        *p_max_user_id = max_user_num;
    }

    close(fd);
    return 0;

failed:
    close(fd);
    return -1;
}
FINSH_FUNCTION_EXPORT_ALIAS(user_num_get, user_num_get, user_num_get)

/*******************************************************************************
* 函数名     : add_del_get_one_user
* 描述       : 添加/删除/获取数据库中的一个用户(需要设置one_user_info->user_id参数)
* 输入         : - one_user_info: 人员信息结构体 - cmd: 0: 添加 1: 删除 2: 获取
* 输出         : None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
int8_t add_del_get_one_user (user_info_t *one_user_info, uint8_t cmd)
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
        if (lseek(fd, (sizeof(user_info_t)*(one_user_info->user_id-1)) + HEADER_SIZE, SEEK_SET) == -1)
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
                    if (write(fd, one_user_info, sizeof(user_info_t)) != sizeof(user_info_t))
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
                }    break;
                
                /* 删除用户 */
                case 1:
                {
                    /* 写入用户信息 */
                    rt_memset(one_user_info, 0,sizeof(user_info_t));//将用户信息清零
                    if (write(fd, one_user_info, sizeof(user_info_t)) != sizeof(user_info_t))
                    {
                        p_printf("write %s failed\r\n", user_info_database_path);
                        close(fd);
                        return -1;
                    }
                    else
                    {
                        close(fd);
                        p_printf("del user %d success\r\n", one_user_info->user_id);            
                    }
                }    break;
                
                /* 读取用户信息 */
                case 2:
                {
                    /* 读取用户信息 */
                    if (read(fd, one_user_info, sizeof(user_info_t)) != sizeof(user_info_t))
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
                }    break;
                
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

/**
 * \brief 获取指定条数用户CRC
 *
 * \param[out] crc       获取到的用户crc
 * \param[out] p_num_get 获取到的用户crc条数
 * \param[in]  start     起始用户号
 * \param[in]  num       待获取的条数
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t user_crc_get (uint32_t crc[], uint16_t *p_num_get, uint16_t start, uint16_t num)
{
    int fd;
    
    uint16_t user_num     = 0;
    uint16_t user_id_max  = 0;
    
    uint32_t i         = 0;
    uint32_t addr      = 0;
    uint32_t get_count = 0;
    
    /* 获取用户数量与最大用户号 */
    if (user_num_get(&user_num, &user_id_max) != 0) {
        p_printf("user_num_get failed\r\n");
        return -1;
    }
    
    if (start > user_id_max) {
        p_printf("start > user_id_max\r\n");
        if (RT_NULL != p_num_get) {
            *p_num_get = 0;
        }
        return 0;
    } else if (start == 0) {
        p_printf("start == 0\r\n");
        return -1;
    } else if (start + num > user_id_max) {
        num = user_id_max - start + 1;
    }
        
    /* 打开人员信息数据库 */
    fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
    if (fd < 0)
    {
        p_printf("open %s failed\r\n", user_info_database_path);
        return -1;
    }

    /* 获取用户crc */
    for (i = start; i < start + num; i++) {
        
        /* 计算文件偏移 */
        addr = (sizeof(user_info_t) * (i - 1)) + OFFSET(user_info,crc) + HEADER_SIZE;
        
        /* 移动文件指针到指定位置 */
        if (lseek(fd, addr, SEEK_SET) == -1) {
            p_printf("lseek %s failed\r\n", user_info_database_path);
            goto failed;
        }

        /* 读取用户信息 */
        if (read(fd, &crc[get_count], 4) != 4) {
            p_printf("read %s failed\r\n", user_info_database_path);
            goto failed;
        }
        get_count++;
    }
    
    if (RT_NULL != p_num_get) {
        *p_num_get = get_count;
    }
            
    close(fd);
    return 0;
    
failed:
    close(fd);
    return -1;
}

void crc_test(uint16_t start, uint16_t num)
{
    uint32_t crc[20];
    uint16_t num_get;
//    uint16_t start = 1;
//    uint16_t num = 100;
    
    if (user_crc_get(crc, &num_get, start, num) != 0)
    {
        rt_kprintf("user_crc_get failed\r\n");
    } else {
        rt_kprintf("num_get %d\r\n", num_get);
        for (int i = 0; i < num_get; i++) {
            rt_kprintf("%08X ", crc[i]);
        }
        rt_kprintf("\r\n");
    }
}
FINSH_FUNCTION_EXPORT_ALIAS(crc_test, crc_test, crc_test)

/*******************************************************************************
* 函数名     : get_set_state
* 描述       : 获取/设置指定用户的进出状态
* 输入         : - state: 状态 - user_id: 用户号 - cmd: 0: 获取状态 1: 设置状态
* 输出         : None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
int8_t get_set_state(uint8_t *state, uint16_t user_id, uint8_t cmd)
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
        if (lseek(fd, (sizeof(user_info_t)*(user_id-1)) + OFFSET(user_info,state) + HEADER_SIZE, SEEK_SET) == -1)
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
                }    break;
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
                }    break;
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

/**
 * \brief 初始化卡号-用户号数组
 *
 * \param[out] card_array 卡号-用户号数组
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t card_array_init(card_id_user_id_link_t card_array[])
{
    int fd;
    int i;
    uint16_t user_num;
    uint16_t max_user_id;
    uint16_t effective_num = 0;
    uint32_t addr = 0;
    
    /* 获取用户数量与最大用户号 */
    if(user_num_get(&user_num, &max_user_id) == -1) {
        p_printf("get user num max failed\r\n");
        return -1;
    }
    
    p_printf("user_num: %d  max_user_num: %d\r\n", user_num, max_user_id);

    /* 打开人员信息数据库 */
    fd = open(user_info_database_path, O_RDWR|O_CREAT, 0);
    if (fd < 0)
    {
        p_printf("open %s failed\r\n", user_info_database_path);
        goto failed;
    }

    for (i = 1; i <= max_user_id; i++) {
        
        addr = (sizeof(user_info_t)*(i-1)) + HEADER_SIZE;
        p_printf("lseek addr %d\r\n", addr);

        /* 移动文件指针到指定位置 */
        if (lseek(fd, addr, SEEK_SET) == -1) {
            p_printf("lseek addr %d %s failed\r\n", addr, user_info_database_path);
            goto failed;
        } 
        
        /* 读取用户信息 */
        if (read(fd, (void *)&card_array[effective_num], sizeof(card_id_user_id_link_t)) != sizeof(card_id_user_id_link_t)) {
            p_printf("read %s failed\r\n", user_info_database_path);
            goto failed;
        }
        
        if (card_array[effective_num].user_id != 0) {
            effective_num++;
            p_printf("%d: get user %d, %08X\r\n", i, card_array[effective_num - 1].user_id, card_array[effective_num - 1].card_id);            
        }

    }
    
    /* 判断用户数量是否正确 */
    if (effective_num != user_num) {
        p_printf("init card array failed! user_num is %d  effective_num is %d\r\n", user_num, effective_num);
        goto failed;
    }

    /* 使用快速排序算法将数组排序 */    
    qsort(card_array, MAX_USER_NUM, sizeof(card_id_user_id_link_t), card_compar);

    close(fd);
    return 0;
    
failed:
    close(fd);
    return -1;

}
