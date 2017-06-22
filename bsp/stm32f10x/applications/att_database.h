/*******************************************************************************
*                        WiFi Access Control System
*                       ----------------------------
*                                  EE Bang
*
* Contact information:
* web site:    http://www.cqutlab.cn/
* e-mail:      799548861@qq.com
*******************************************************************************/

/**
 * \file
 * \brief 考勤信息数据库
 *
 * \internal
 * \par Modification history
 * - 1.00 16-11-03  zhangjinke, first implementation.
 * \endinternal
 */ 

#ifndef __ATT_DATABASE_H
#define __ATT_DATABASE_H

#include <stdint.h>

#if 0
#define PRINT_ATT_DEBUG_INFO    /**< \brief 打印调试信息 */
#endif

#define GET_RECORD            0 /**< \brief 获取记录 */
#define SET_RECORD            1 /**< \brief 设置记录 */

/** \brief 人员信息结构体 */
__packed typedef struct att_info
{
    uint16_t user_id;            /**< \brief 用户号 */
    uint8_t  student_id[16];     /**< \brief 学号 */
    uint8_t  name[16];           /**< \brief 姓名 */
    uint8_t  device_addr;        /**< \brief 设备地址 */
    uint8_t  mac_addr[6];        /**< \brief mac地址 */
    uint8_t  state;              /**< \brief 进出状态 0:出门 1:进门 */
    uint16_t year;               /**< \brief 年(有效期，超过之后用户失效) */
    uint8_t  month;              /**< \brief 月 */
    uint8_t  day;                /**< \brief 日 */
    uint8_t  hour;               /**< \brief 时 */
    uint8_t  minutes;            /**< \brief 分 */
    uint8_t  second;             /**< \brief 秒 */
    uint32_t crc;                /**< \brief 本成员之前所有成员按字节计算的CRC值 */
} att_info_t;

/** \brief 人员信息数据库头 */
__packed typedef struct att_header
{
    uint32_t total;              /**< \brief 总条数 */
    uint32_t crc;                /**< \brief crc校验 */
} att_header_t;

extern att_header_t g_att_header;                               /* 考勤信息数据库header结构体 */

/**
 * \brief 初始化考勤数据库
 *
 * \param[in] p_att_header 考勤信息头
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_database_init (att_header_t *p_att_header);

/**
 * \brief 获取/设置考勤数据库header
 *
 * \param[in,out] p_att_header 考勤信息头
 * \param[in]     cmd          0: 获取 1: 设置
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t record_header_get_set (att_header_t *p_att_header, uint8_t cmd);

/**
 * \brief 添加一条考勤记录
 *
 * \param[in] p_one_att_info 考勤记录结构体
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_record_add (att_info_t *p_one_att_info);

/**
 * \brief 删除指定条数考勤记录
 *
 * \param[in] num 待删除的条数
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_record_del (uint16_t num);

/**
 * \brief 获取考勤记录(从最后一条记录开始获取)
 *
 * \param[out] att_info    考勤记录
 * \param[in]  lenth       条数
 * \param[out] p_att_count 获取到的考勤记录条数
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_record_get (att_info_t att_info[], uint8_t lenth, uint16_t *p_att_count);

/**
 * \brief 清空考勤记录
 *
 * \param[in] p_att_header 考勤信息头
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_database_wipe (att_header_t *p_att_header);

#endif /* __ATT_DATABASE_H */

/* end of file */
