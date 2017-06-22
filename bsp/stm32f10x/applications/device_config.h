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
 * \brief 设备配置文件相关
 * 
 * \internal
 * \par Modification history
 * - 1.00 16-09-19 zhangjinke, first implementation
 * \endinternal
 */
#ifndef __DEVICE_CONFIG
#define __DEVICE_CONFIG

#include <rtthread.h>
#include "global.h"

#define PRINT_CFG_DEBUG_INFO           /**< \brief 打印调试信息 */

#define GET_DEVICE        0            /**< \brief 获取设备xxx */
#define SET_DEVICE        1            /**< \brief 设置设备xxx */

/** \brief 系统配置结构 */
__packed typedef struct device_config
{
    uint8_t  device_addr;                /**< \brief 本机地址 */
    uint8_t  this_device_name[64];       /**< \brief 本机名称 */
    uint8_t  server_ip[4];               /**< \brief 服务器IP地址 */
    uint16_t server_port;                /**< \brief 服务器端口 */
    uint8_t  router_ssid[32 + 1];        /**< \brief 路由器名称 */
    uint8_t  router_passwd[256 + 1];     /**< \brief 路由器密码 */
    uint8_t  router_auth;                /**< \brief 路由器加密方式 */
    uint8_t  router_bssid[6];            /**< \brief 路由器MAC地址 */
    uint8_t  mesh_ssid[32 + 1];          /**< \brief mesh网络名称 */
    uint8_t  mesh_passwd[256 + 1];       /**< \brief mesh网络密码 */
    uint8_t  mesh_group_id[6];           /**< \brief mesh组ID */
    uint32_t crc;                        /**< \brief crc校验 */
} device_config_t;

/** \brief 系统配置信息 */
extern device_config_t g_device_config;

/**
 * \brief 初始化设备配置
 *
 * \param[in] p_device_config 设备配置结构体
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t device_config_init (device_config_t *p_device_config);

/**
 * \brief 获取/设置设备配置
 *
 * \param[in,out] device_config 设备配置结构体
 * \param[in]     cmd 0: 获取设备配置 1: 设置设备配置
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t device_config_get_set (device_config_t *p_device_config, uint8_t cmd);

/**
 * \brief 打印设备配置信息
 *
 * \param[in] p_device_config 设备配置结构体
 *
 * \return 无
 */
void device_config_print (device_config_t *p_device_config);

#endif /* __DEVICE_CONFIG */

/* end of file */
