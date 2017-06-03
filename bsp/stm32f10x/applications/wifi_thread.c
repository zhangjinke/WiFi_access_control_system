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
#include "finsh.h"
#include "wifi_thread.h"
#include "esp8266.h"
#include "stm32_crc.h"
#include "esp8266_cmd.h"
#include "server_process.h"

/* 获取结构体成员偏移宏定义 */
#define OFFSET(Type, member) ( (u32)&(((struct Type*)0)->member) )
#define MEMBER_SIZE(Type, member) sizeof(((struct Type*)0)->member)

#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

rt_uint8_t wifi_stack[ 1024 ];	//线程栈
struct rt_thread wifi_thread; 	//线程控制块

rt_thread_t gp_esp8266_info_get_tid = RT_NULL; /**< \brief 指向线程控制块的指针 */

struct wifi_pack wifi_pack_recv;
u8 is_recv_wifi_pack = 0;

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
    
	if (0 != wifi_get_macaddr(station_addr, softap_addr)) {
		rt_kprintf("wifi get macaddr failed\r\n");
	} else {
        rt_kprintf("station mac:" MACSTR " softap mac:" MACSTR, 
                   MAC2STR(station_addr), 
                   MAC2STR(softap_addr));
    }
    
    esp8266_cmd_test();
}

/*******************************************************************************
* 函数名 	: wifi_thread_entry
* 描述   	: 通信相关线程
* 输入     	: - parameter: 线程入口参数
* 输出     	: None
* 返回值    : None
*******************************************************************************/
void wifi_thread_entry(void* parameter)
{
	rt_err_t status = RT_EOK;     /* 接收函数返回值 */
	rt_uint32_t recved_event = 0; /* 收到的事件 */
	static u32 par_lenth = sizeof(struct wifi_pack) - MEMBER_SIZE(wifi_pack, data); /* 包中的参数大小 */
	static u32 crc_lenth = MEMBER_SIZE(wifi_pack, crc); /* CRC大小 */
	struct mesh_header_format *header = NULL;
	
	memset(station_addr, 0, sizeof(station_addr));
	memset(softap_addr, 0, sizeof(softap_addr));
	
	while(1)
	{
		/* 等待接收事件 */
		status = rt_event_recv(	&esp8266_event,                       /* 事件对象的句柄 */
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
					
//					rt_kprintf("cmd: %d, lenth: %d, crc: %08X\r\n", wifi_pack_recv.cmd, wifi_pack_recv.lenth, wifi_pack_recv.crc);
//					
//					for (i = 0; i < wifi_pack_recv.lenth; i++)
//					{
//						rt_kprintf("%02X ", *(wifi_pack_recv.data + i));				
//					}
//					rt_kprintf("\r\n");	
					
					switch (wifi_pack_recv.cmd) {
					
					case CMD_SEND_DATA_TO_MCU:
						header = (struct mesh_header_format *)wifi_pack_recv.data;
						if (header->oe == 0)
						{
							data_process(header->user_data[0], header->user_data + 1, header->len - ESP_MESH_HEAD_SIZE - 1);
						}
						
						break;
					
//					case CMD_WIFI_GET_MACADDR:
//						if (12 == wifi_pack_recv.lenth) {
//							memcpy(station_addr, wifi_pack_recv.data, 6);
//							memcpy(softap_addr, wifi_pack_recv.data + 6, 6);
//						} else {
//							rt_kprintf("%s", wifi_pack_recv.data);
//						}
//						
//						break;
						
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

s8 wifi_send(u8 cmd, u16 data_lenth, u8 *data)
{
	struct wifi_pack wifi_pack_send;
	u8 *send_pack = NULL;
	u32 par_lenth = sizeof(struct wifi_pack) - MEMBER_SIZE(wifi_pack, data); /* 包中的参数大小 */
	u32 crc_lenth = MEMBER_SIZE(wifi_pack, crc); /* CRC大小 */
	
	/* 申请内存 */
	send_pack = (u8 *)rt_malloc(data_lenth + par_lenth);
    if (!send_pack) 
    { 
        rt_kprintf("send_pack memory failed\r\n");

        return -RT_ENOMEM;
    }

	wifi_pack_send.cmd = cmd;                /* 命令 */
	wifi_pack_send.lenth = data_lenth;       /* 数据长度 */
	/* 将除CRC之外的其它参数拷贝到缓冲区 */
	rt_memcpy(send_pack + crc_lenth, (u8 *)&wifi_pack_send + crc_lenth, par_lenth - crc_lenth);
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

s8 sendTest(u8 cmd, u32 lenth)
{
	u8 buf[1024*5];
	int i;
	
	for (i = 0; i < lenth; i++)
	{
		buf[i] = (u8)i;
	}
	
	return wifi_send(cmd, lenth, buf);
}
FINSH_FUNCTION_EXPORT(sendTest, sendTest)

s8 sendServer()
{
	u8 data[] = "\x00\x00\x00\x00\x00\x00Hello World";
	return wifi_send(CMD_SEND_MESH_DATA, sizeof(data), data);
}
FINSH_FUNCTION_EXPORT(sendServer, sendServer)
