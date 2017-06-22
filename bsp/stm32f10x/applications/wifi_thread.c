/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: wifi_thread.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 11 日
**
** 描        述: 通信相关线程

** 日志:
2016.09.11  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include <finsh.h>
#include "wifi_thread.h"
#include "esp8266.h"
#include "stm32_crc.h"
#include "esp8266_cmd.h"
#include "server_process.h"
#include "device_config.h"
#include "global.h"


rt_uint8_t wifi_stack[ 1024 ];    //线程栈
struct rt_thread wifi_thread;     //线程控制块

rt_thread_t gp_esp8266_info_get_tid = RT_NULL; /**< \brief 指向线程控制块的指针 */

struct wifi_pack wifi_pack_recv;
uint8_t is_recv_wifi_pack = 0;

uint8_t station_addr[6];
uint8_t softap_addr[6];

/*
 * \brief 获取esp8266相关信息
 *
 * \param[in] p_parameter 线程入口参数
 *
 * \return 无
 */
void esp8266_info_get_entry (void *p_parameter)
{
    /* 等待wifi初始化完成 */
    rt_thread_delay(1000);
    
    if (0 != wifi_mac_addr_get(station_addr, softap_addr)) {
        rt_kprintf("wifi get macaddr failed\r\n");
    } else {
        rt_kprintf("station mac:" MACSTR " softap mac:" MACSTR, 
                   MAC2STR(station_addr), 
                   MAC2STR(softap_addr));
    }
    
    /* 设置服务器IP与端口 */
    if (server_addr_set(g_device_config.server_ip,g_device_config.server_port)) { 
        rt_kprintf("server_addr_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 设置MESH组ID */
    if (mesh_group_id_set(g_device_config.mesh_group_id)) { 
        rt_kprintf("mesh_group_id_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 设置路由器信息 */
    if (router_set(g_device_config.router_ssid, g_device_config.router_passwd, g_device_config.router_auth, g_device_config.router_bssid)) { 
        rt_kprintf("router_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 设置MESH网络信息 */
    if (mesh_wifi_set(g_device_config.mesh_ssid, g_device_config.mesh_passwd)) { 
        rt_kprintf("mesh_wifi_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 初始化MESH */
    if (mesh_init()) { 
        rt_kprintf("mesh_init failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
}

/*******************************************************************************
* 函数名     : wifi_thread_entry
* 描述       : 通信相关线程
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
void wifi_thread_entry(void* parameter)
{
    rt_err_t status = RT_EOK;     /* 接收函数返回值 */
    rt_uint32_t recved_event = 0; /* 收到的事件 */
    static uint32_t par_lenth = sizeof(struct wifi_pack) - MEMBER_SIZE(wifi_pack, data); /* 包中的参数大小 */
    static uint32_t crc_lenth = MEMBER_SIZE(wifi_pack, crc); /* CRC大小 */
    struct mesh_header_format *header = NULL;
    
    memset(station_addr, 0, sizeof(station_addr));
    memset(softap_addr, 0, sizeof(softap_addr));
    
    while(1)
    {
        /* 等待接收事件 */
        status = rt_event_recv(    &esp8266_event,                       /* 事件对象的句柄 */
                                HSPI_RX,                              /* 接收线程感兴趣的事件 */
                                RT_EVENT_FLAG_CLEAR|RT_EVENT_FLAG_OR, /* 逻辑或、清除事件 */
                                RT_WAITING_FOREVER,                   /* 永不超时 */
                                &recved_event                         /* 指向收到的事件 */
                              );
        /* 根据接收到的事件执行不同的处理 */
        if(status == RT_EOK)
        {
            /* hspi接收事件 */
            if (recved_event & HSPI_RX)
            {
                /* hspi接收完成 */
                if (esp8266_spi_read() == 0)
                {
                    rt_memcpy(&wifi_pack_recv, recv_pack, par_lenth);
                    /* 校验包长度 */
                    if (wifi_pack_recv.lenth + par_lenth != recv_lenth)
                    {
                        is_recv_pack = 0; /* 重新等待接收数据 */
                        rt_kprintf("lenth verify failed\r\n");
                        continue;
                    }
                    /* CRC校验 */
                    if (wifi_pack_recv.crc != block_crc_calc(recv_pack + crc_lenth, par_lenth - crc_lenth + wifi_pack_recv.lenth))
                    {
                        is_recv_pack = 0; /* 重新等待接收数据 */
                        rt_kprintf("crc verify failed\r\n");
                        continue;
                    }
                    
                    wifi_pack_recv.data = recv_pack + par_lenth;
                    is_recv_wifi_pack = 1;
                    
//                    rt_kprintf("cmd: %d, lenth: %d, crc: %08X\r\n", wifi_pack_recv.cmd, wifi_pack_recv.lenth, wifi_pack_recv.crc);
//                    
//                    for (i = 0; i < wifi_pack_recv.lenth; i++)
//                    {
//                        rt_kprintf("%02X ", *(wifi_pack_recv.data + i));                
//                    }
//                    rt_kprintf("\r\n");    
                    
                    switch (wifi_pack_recv.cmd) {
                    
                    case CMD_SEND_DATA_TO_MCU:
                        header = (struct mesh_header_format *)wifi_pack_recv.data;
                        if (header->oe == 0)
                        {
                            data_process(header->user_data, header->len - ESP_MESH_HEAD_SIZE);
                        }
                        
                        break;
                    
//                    case CMD_WIFI_GET_MACADDR:
//                        if (12 == wifi_pack_recv.lenth) {
//                            memcpy(station_addr, wifi_pack_recv.data, 6);
//                            memcpy(softap_addr, wifi_pack_recv.data + 6, 6);
//                        } else {
//                            rt_kprintf("%s", wifi_pack_recv.data);
//                        }
//                        
//                        break;
                        
                    default:
//                        rt_kprintf("wifi_pack_recv.cmd: %d\r\n", wifi_pack_recv.cmd);
                        break;
                    
                    }
                    
                    is_recv_pack = 0; /* 重新等待接收数据 */
                }
            }
        }
    }    
}

int8_t wifi_send(uint8_t cmd, uint16_t data_lenth, uint8_t *data)
{
    struct wifi_pack wifi_pack_send;
    uint8_t *send_pack = NULL;
    uint32_t par_lenth = sizeof(struct wifi_pack) - MEMBER_SIZE(wifi_pack, data); /* 包中的参数大小 */
    uint32_t crc_lenth = MEMBER_SIZE(wifi_pack, crc); /* CRC大小 */
    
    /* 申请内存 */
    send_pack = (uint8_t *)rt_malloc(data_lenth + par_lenth);
    if (!send_pack) 
    { 
        rt_kprintf("send_pack memory failed\r\n");

        return -RT_ENOMEM;
    }

    wifi_pack_send.cmd = cmd;                /* 命令 */
    wifi_pack_send.lenth = data_lenth;       /* 数据长度 */
    
    /* 将除CRC之外的其它参数拷贝到缓冲区 */
    rt_memcpy(send_pack + crc_lenth, (uint8_t *)&wifi_pack_send + crc_lenth, par_lenth - crc_lenth);
    
    /* 将数据拷贝到缓冲区 */
    rt_memcpy(send_pack + par_lenth, data, data_lenth);
    
    /* 计算CRC */
    wifi_pack_send.crc = block_crc_calc(send_pack + crc_lenth, par_lenth - crc_lenth + data_lenth);
    
    /* 将CRC拷贝到缓冲区 */
    rt_memcpy(send_pack, &wifi_pack_send, crc_lenth);
    
    /* 发送数据 */
    if (esp8266_spi_write(send_pack, par_lenth + data_lenth) != 0)
    {
        rt_free(send_pack);
        return -1;
    }
    
    rt_free(send_pack);
    return 0;
}

int8_t sendTest(uint8_t cmd, uint32_t lenth)
{
    uint8_t buf[1024*5];
    int i;
    
    for (i = 0; i < lenth; i++)
    {
        buf[i] = (uint8_t)i;
    }
    
    return wifi_send(cmd, lenth, buf);
}
FINSH_FUNCTION_EXPORT(sendTest, sendTest)

int8_t sendServer()
{
    uint8_t data[] = "\x00\x00\x00\x00\x00\x00Hello World";
    return wifi_send(CMD_SEND_MESH_DATA, sizeof(data), data);
}
FINSH_FUNCTION_EXPORT(sendServer, sendServer)
