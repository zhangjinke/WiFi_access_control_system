#ifndef _WIFI_THREAD_H_
#define _WIFI_THREAD_H_

#include <rtthread.h>
#include <stdint.h>

__packed struct wifi_pack
{
    uint32_t  crc;   /**< \brief crc校验 */
    uint8_t   cmd;   /**< \brief 命令 */
    uint16_t  lenth; /**< \brief 数据长度 */
    uint8_t  *data;  /**< \brief 数据 */
};

extern rt_uint8_t wifi_stack[ 1024 ];    //线程栈
extern struct rt_thread wifi_thread;     //线程控制块
extern rt_thread_t gp_esp8266_info_get_tid; /**< \brief 指向线程控制块的指针 */

extern void wifi_thread_entry(void* parameter);

extern struct wifi_pack wifi_pack_recv;
extern uint8_t is_recv_wifi_pack;

extern uint8_t station_addr[6];
extern uint8_t softap_addr[6];

int8_t wifi_send(uint8_t cmd, uint16_t data_lenth, uint8_t *data);

/*
 * \brief 获取esp8266相关信息
 *
 * \param[in] p_parameter 线程入口参数
 *
 * \return 无
 */
void esp8266_info_get_entry (void *p_parameter);

#endif
