#ifndef _ESP8266_CMD_H_
#define _ESP8266_CMD_H_

#include "sys.h"

#define CMD_RETURN_RECV                   (0)	 /* 直接返回接收到的数据 */
#define CMD_GET_SDK_VERSION               (1)    /* 获取sdk版本号 */
#define CMD_GET_FLASH_SIZE_MAP            (2)    /* 查询Flash size以及Flash map */
#define CMD_WIFI_GET_IP_INFO              (3)    /* 查询IP地址 */
#define CMD_WIFI_GET_MACADDR              (4)    /* 查询mac地址 */
#define CMD_GET_DEVICE_LIST               (5)    /* 获取mesh设备列表 */
#define CMD_SEND_MESH_DATA                (6)    /* 向mesh网络中发送数据 */

#define CMD_SEND_DATA_TO_MCU              (254)  /* 发送数据到mcu */


s8 wifi_get_macaddr(u8 station_mac[], u8 ap_mac[]);

#endif
