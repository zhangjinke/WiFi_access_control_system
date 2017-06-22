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
 * \brief esp8266指令
 *
 * \internal
 * \par Modification history
 * - 1.00 17-01-31 zhangjinke, first implementation.
 * \endinternal
 */ 
#ifndef __ESP8266_CMD_H
#define __ESP8266_CMD_H

#include "esp8266.h"

typedef enum auth_mode {
    AUTH_OPEN = 0,
    AUTH_WEP,
    AUTH_WPA_PSK,
    AUTH_WPA2_PSK,
    AUTH_WPA_WPA2_PSK,
    AUTH_MAX
} auth_mode_t;

#define CMD_RETURN_RECV                   (0)    /* 直接返回接收到的数据 */
#define CMD_GET_SDK_VERSION               (1)    /* 获取sdk版本号 */
#define CMD_GET_FLASH_SIZE_MAP            (2)    /* 查询Flash size以及Flash map */
#define CMD_WIFI_GET_IP_INFO              (3)    /* 查询IP地址 */
#define CMD_WIFI_GET_MACADDR              (4)    /* 查询mac地址 */
#define CMD_GET_DEVICE_LIST               (5)    /* 获取mesh设备列表 */
#define CMD_SEND_MESH_DATA                (6)    /* 向mesh网络中发送数据 */
#define CMD_SERVER_ADDR_SET               (7)    /* 设置服务器IP与端口 */
#define CMD_MESH_GROUP_ID_SET             (8)    /* 设置MESH组ID */
#define CMD_ROUTER_SET                    (9)    /* 设置路由器信息 */
#define CMD_MESH_WIFI_SET                 (10)   /* 设置MESH网络信息 */
#define CMD_MESH_INIT                     (11)   /* 初始化MESH */

#define CMD_SEND_DATA_TO_MCU              (254)  /* 发送数据到mcu */



/**
 * \brief 等待esp8266应答
 *
 * \param[in] cmd 命令
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t wait_ack (uint8_t cmd);

/**
 * \brief 测试hspi通信
 *
 * \param 无
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t test_hspi (void);

/**
 * \brief 获取sdk版本号
 *
 * \param[out] p_ver   sdk版本号
 * \param[out] p_lenth sdk版本号字符串长度
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t sdk_version_get (uint8_t *p_ver, uint16_t *p_lenth);

/**
 * \brief 查询Flash size以及Flash map
 *
 * \param[out] p_flash_size_map 获取到的flash信息
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t get_flash_size_map (uint8_t *p_flash_size_map);

/**
 * \brief 查询IP地址
 *
 * \param[out] p_station_ip station ip地址
 * \param[out] p_ap_ip      ap ip地址
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t wifi_ip_get (ip_info_t *p_station_ip, ip_info_t *p_ap_ip);

/**
 * \brief 查询mac地址
 *
 * \param[out] p_station_ip station ip地址
 * \param[out] p_ap_ip      ap ip地址
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t wifi_mac_addr_get (uint8_t *p_station_mac, uint8_t *p_ap_mac);

/**
 * \brief 获取mesh设备列表
 *
 * \param[out] p_node_list mesh设备列表
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t device_list_get (struct mesh_device_list_type *p_node_list);

/**
 * \brief 设置服务器IP与端口
 *
 * \param[in] p_ip ip地址
 * \param[in] port 端口
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t server_addr_set (uint8_t *p_ip, uint16_t port);

/**
 * \brief 设置MESH组ID
 *
 * \param[in] p_id MESH组ID
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t mesh_group_id_set (uint8_t *p_id);

/**
 * \brief 设置路由器信息
 *
 * \param[in] p_ssid   路由器名称
 * \param[in] p_passwd 路由器密码
 * \param[in] auth     加密方式
 * \param[in] p_mac    路由器mac地址(只有连接隐藏wifi时才需要配置)
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t router_set (uint8_t *p_ssid, uint8_t *p_passwd, uint8_t auth, uint8_t *p_mac);

/**
 * \brief 设置MESH网络信息
 *
 * \param[in] p_ssid   路由器名称
 * \param[in] p_passwd 路由器密码
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t mesh_wifi_set (uint8_t *p_ssid, uint8_t *p_passwd);

/**
 * \brief 初始化MESH
 *
 * \param 无
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t mesh_init (void);

#endif /* __ESP8266_CMD_H */

/* end of file */
