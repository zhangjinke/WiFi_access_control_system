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
#include <stdint.h>

/* 打印调试信息 */
#if 0
#define printDebugInfo
#endif

/* 最大用户总量 */
#define MAX_USER_NUM        200

/* header大小 */
#define HEADER_SIZE         8

#define GET_USER            0    //获取用户
#define SET_USER            1    //设置用户

#define ADD_ONE_USER        0    //添加用户
#define DEL_ONE_USER        1    //删除用户
#define GET_ONE_USER        2    //获取用户信息

/** \brief 用户信息结构 总1058字节 */
__packed typedef struct user_info
{
    uint16_t user_id;         /**< \brief 0-1         用户号 */
    uint32_t card_id;         /**< \brief 2-5         卡号 */
    uint8_t  effective;       /**< \brief 6           激活状态 0:失效 1:有效 */
    uint8_t  student_id[16];  /**< \brief 7-22        学号 11个数字(前补0) */
    uint8_t  name[16];        /**< \brief 23-38       姓名 右边补0x00 */
    uint8_t  authority[16];   /**< \brief 39-54       权限 */
    uint8_t  is_time_limit;   /**< \brief 55          是否有时间限制 0:无限制 1:有限制 */
    uint16_t year;            /**< \brief 56-57       年(有效期，超过之后用户失效) */
    uint8_t  month;           /**< \brief 58          月 */
    uint8_t  day;             /**< \brief 59          日 */
    uint8_t  hour;            /**< \brief 60          时 */
    uint8_t  minutes;         /**< \brief 61          分 */
    uint8_t  second;          /**< \brief 62          秒 */
    uint16_t finger_index[5]; /**< \brief 63-72       指纹特征值分别在指纹模块中的用户号 */
    uint8_t  finger[5][193];  /**< \brief 73-1037     指纹特征值 */
    uint32_t crc;             /**< \brief 1038-1041   本成员之前所有成员按字节计算的CRC值 */
    uint8_t  state[16];       /**< \brief 1042-1057   进出状态 */
} user_info_t;

/** \brief 卡号与用户号结构 */
__packed typedef struct card_id_user_id_link
{
    uint16_t user_id;        /**< \brief 用户号 */
    uint32_t card_id;        /**< \brief 卡号 */
} card_id_user_id_link_t;

/** \brief 指纹号与用户号结构 */
__packed typedef struct finger_user_id_id_link
{
    uint16_t user_id;        /**< \brief 用户号 */
    uint16_t finger_id;      /**< \brief 指纹号 */
} finger_id_user_id_link_t;

extern const char *user_info_database_path;

/* 只包含用户号、卡号的结构体数组,由卡号排序之后使用2分法搜索 */
extern card_id_user_id_link_t g_card_id_user_id_list[MAX_USER_NUM];

/**
 * \brief 卡号-用户号数组二分搜索与快速排序的比较函数(递减)
 *
 * \param[in] p_a 成员a
 * \param[in] p_b 成员b
 *
 * \return a大于b返回小于0，a等于b返回0，a小于b返回大于0
 */
int card_compar (const void *p_a, const void *p_b);

/**
 * \brief 获取用户数量与最大用户号
 *
 * \param[out] p_user_num    用户数量
 * \param[out] p_max_user_id 最大用户号
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t user_num_get (uint16_t *p_user_num, uint16_t *p_max_user_id);

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
int8_t user_crc_get (uint32_t crc[], uint16_t *p_num_get, uint16_t start, uint16_t num);

extern int8_t get_set_user_num(uint16_t *user_num, uint8_t cmd);//获取/设置数据库中的人员信息
extern int8_t get_set_user_num_max(uint16_t *user_num_max, uint8_t cmd);//获取/设置数据库中的最大用户号
extern int8_t add_del_get_one_user(user_info_t *one_user_info, uint8_t cmd);//添加/删除/获取数据库中的一个用户(需要设置one_user_info->user_id参数)
int8_t get_set_state(uint8_t *state, uint16_t user_id, uint8_t cmd);
extern int8_t card_array_init(card_id_user_id_link_t card_array[]);//初始化卡号结构数组

#endif
