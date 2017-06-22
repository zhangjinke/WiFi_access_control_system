#ifndef _TOUCH_SCREEN_H_
#define _TOUCH_SCREEN_H_
#include "global.h"
#include "ILI93xx.h"

//与触摸屏有关定义，根据实际情况填写
#define TOUCH_AD_TOP		160  	//按下触摸屏的顶部，写下 Y 轴模拟输入值。
#define TOUCH_AD_BOTTOM		3990 	//按下触摸屏的底部，写下 Y 轴模拟输入值。
#define TOUCH_AD_LEFT 		160		//按下触摸屏的左侧，写下 X 轴模拟输入值。
#define TOUCH_AD_RIGHT		3990	//按下触摸屏的右侧，写下 X 轴模拟输入值。

#define TP_PRES_DOWN 0x80  		//触屏被按下	  
#define TP_CATH_PRES 0x40  		//有按键按下了 
#define CT_MAX_TOUCH  5    		//电容屏支持的点数,固定为5点

//触摸屏控制器
typedef struct 
{
	uint8_t (*init)(void);			//初始化触摸屏控制器
	uint8_t (*scan)(LCD_TypeDef *TFTLCD, uint8_t);				//扫描触摸屏.0,屏幕扫描;1,物理坐标;	 
	void (*adjust)(LCD_TypeDef *TFTLCD);		//触摸屏校准 
	uint16_t x[CT_MAX_TOUCH]; 		//当前坐标
	uint16_t y[CT_MAX_TOUCH];		//电容屏有最多5组坐标,电阻屏则用x[0],y[0]代表:此次扫描时,触屏的坐标,用
								//x[4],y[4]存储第一次按下时的坐标. 
	uint8_t  sta;					//笔的状态 
								//b7:按下1/松开0; 
	                            //b6:0,没有按键按下;1,有按键按下. 
								//b5:保留
								//b4~b0:电容触摸屏按下的点数(0,表示未按下,1表示按下)
/////////////////////触摸屏校准参数(电容屏不需要校准)//////////////////////								
	float xfac;					
	float yfac;
	short xoff;
	short yoff;	   
//新增的参数,当触摸屏的左右上下完全颠倒时需要用到.
//b0:0,竖屏(适合左右为X坐标,上下为Y坐标的TP)
//   1,横屏(适合左右为Y坐标,上下为X坐标的TP) 
//b1~6:保留.
//b7:0,电阻屏
//   1,电容屏 
	uint8_t touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev0;	 	//触屏控制器在touch.c里面定义
extern _m_tp_dev tp_dev1;	 	//触屏控制器在touch.c里面定义

//电阻屏芯片连接引脚	   
#define tp_irq0  		PBin(7)  	/* 门外触摸屏 */
#define tp_irq1  		PBin(6)  	/* 门内触摸屏 */
   
//电阻屏函数
extern void TP_Write_Byte(LCD_TypeDef *TFTLCD, uint8_t num);						//向控制芯片写入一个数据
extern uint16_t TP_Read_AD(LCD_TypeDef *TFTLCD, uint8_t CMD);							//读取AD转换值
extern uint16_t TP_Read_XOY(LCD_TypeDef *TFTLCD, uint8_t xy);							//带滤波的坐标读取(X/Y)
extern uint8_t TP_Read_XY(LCD_TypeDef *TFTLCD, uint16_t *x,uint16_t *y);					//双方向读取(X+Y)
extern uint8_t TP_Read_XY2(LCD_TypeDef *TFTLCD, uint16_t *x,uint16_t *y);					//带加强滤波的双方向坐标读取
extern void TP_Drow_Touch_Point(LCD_TypeDef *TFTLCD, uint16_t x,uint16_t y,uint16_t color);//画一个坐标校准点
extern void TP_Draw_Big_Point(LCD_TypeDef *TFTLCD, uint16_t x,uint16_t y,uint16_t color);	//画一个大点
extern void TP_Save_Adjdata(LCD_TypeDef *TFTLCD);						//保存校准参数
extern uint8_t TP_Get_Adjdata(LCD_TypeDef *TFTLCD);						//读取校准参数
extern void TP_Adjust(LCD_TypeDef *TFTLCD);							//触摸屏校准
extern void TP_Adj_Info_Show(LCD_TypeDef *TFTLCD, uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t x3,uint16_t y3,uint16_t fac);//显示校准信息
//电阻屏/电容屏 共用函数
extern uint8_t TP_Scan(LCD_TypeDef *TFTLCD, uint8_t tp);								//扫描
extern uint8_t TP_Init(void);								//初始化
 
#endif

