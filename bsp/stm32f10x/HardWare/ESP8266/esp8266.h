#ifndef _ESP8266_H_
#define _ESP8266_H_

#include "sys.h"

#define hspi_rx         (1 << 0) //hspi接收事件

#define ESP8266_EN      PCout(5) //ESP8266 EN
#define ESP8266_RST     PBout(0) //ESP8266 RST
#define ESP8266_BOOT    PCout(4) //ESP8266 BOOT

enum esp8266_io
{
	EN = 0,
	RST,
	CS,
	BOOT
};

/* esp8266事件控制块 */
extern struct rt_event esp8266_event;
 
extern u8 wr_rdy, rd_rdy;

extern u8 recv_pack[1024*5];
extern u8 is_recv_pack;
extern u32 recv_lenth;

extern s8 init_esp8266(void);
extern s8 esp8266_spi_read(void);
extern s8 esp8266_spi_write(u8 *pack, u32 lenth);

#endif
