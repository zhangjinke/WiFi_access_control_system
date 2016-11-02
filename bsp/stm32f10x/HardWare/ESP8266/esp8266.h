#ifndef _ESP8266_H_
#define _ESP8266_H_

#include "sys.h"

#define esp8266_wr_state PCin(4)
#define esp8266_rd_state PFin(7)

extern u8 wr_rdy, rd_rdy;

extern s8 init_esp8266(void);
extern void WriteTest(void);
extern void ReadTest(void);
void check_state_line(void);

#endif
