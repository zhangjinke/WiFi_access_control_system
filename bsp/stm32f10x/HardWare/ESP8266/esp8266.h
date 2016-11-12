#ifndef _ESP8266_H_
#define _ESP8266_H_

#include "sys.h"

#define esp8266_wr_state PCin(4)
#define esp8266_rd_state PFin(7)

#define hspi_rx         (1 << 0) //hspi接收事件

/* esp8266事件控制块 */
extern struct rt_event esp8266_event;

extern u8 wr_rdy, rd_rdy;

extern s8 init_esp8266(void);
extern void check_state_line(void);
extern s8 esp8266_spi_read(void);

#endif
