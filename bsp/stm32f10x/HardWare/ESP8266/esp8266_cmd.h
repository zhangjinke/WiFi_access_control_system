#ifndef _ESP8266_CMD_H_
#define _ESP8266_CMD_H_

#include "sys.h"

#define cmd_return_recv                   (0)	 /* 直接返回接收到的数据 */
#define cmd_get_sdk_version               (1)    /* 获取sdk版本号 */
#define cmd_get_flash_size_map            (2)    /* 查询Flash size以及Flash map */
#define cmd_wifi_get_ip_info              (3)    /* 查询IP地址 */
#define cmd_wifi_get_macaddr              (4)    /* 查询mac地址 */
#define cmd_get_device_list               (5)    /* 获取mesh设备列表 */

#define cmd_send_data_to_mcu              (254)  /* 发送数据到mcu */

#endif
