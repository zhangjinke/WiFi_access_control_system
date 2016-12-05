#include "ILI93xx.h"
#include "stdlib.h"
#include "font.h" 
#include "usart.h"	 
#include "pwm.h"
#include <rtthread.h>

#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

void DelayUs(u32 dwTime)
{
    u32 dwCurCounter=0;                                //当前时间计数值
    u32 dwPreTickVal=SysTick->VAL;                     //上一次SYSTICK计数值
    u32 dwCurTickVal;                                  //上一次SYSTICK计数值
    dwTime=dwTime*(72000000/1000000);    //需延时时间，共多少时间节拍
    for(;;){
        dwCurTickVal=SysTick->VAL;
        if(dwCurTickVal<dwPreTickVal){
            dwCurCounter=dwCurCounter+dwPreTickVal-dwCurTickVal;
        }
        else{
            dwCurCounter=dwCurCounter+dwPreTickVal+SysTick->LOAD-dwCurTickVal;
        }
        dwPreTickVal=dwCurTickVal;
        if(dwCurCounter>=dwTime){
            return;
        }
    }
}

//LCD的画笔颜色和背景色	   
u16 POINT_COLOR=0x0000;	//画笔颜色
u16 BACK_COLOR=0xFFFF;  //背景色 
  
//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev0;
_lcd_dev lcddev1;
	 
//写寄存器函数
//regval:寄存器值
void LCD_WR_REG(LCD_TypeDef *TFTLCD, u16 regval)
{
	TFTLCD->LCD_REG=regval;//写入要写的寄存器序号	 
}
//写LCD数据
//data:要写入的值
void LCD_WR_DATA(LCD_TypeDef *TFTLCD, u16 data)
{
	TFTLCD->LCD_RAM=data;		 
}
//读LCD数据
//返回值:读到的值
u16 LCD_RD_DATA(LCD_TypeDef *TFTLCD)
{
	vu16 ram;			//防止被优化
	ram=TFTLCD->LCD_RAM;	
	return ram;	 
}					   
//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(LCD_TypeDef *TFTLCD, u16 LCD_Reg,u16 LCD_RegValue)
{
	TFTLCD->LCD_REG = LCD_Reg;		//写入要写的寄存器序号	 
	TFTLCD->LCD_RAM = LCD_RegValue;//写入数据	    		 
}	   
//读寄存器
//LCD_Reg:寄存器地址
//返回值:读到的数据
u16 LCD_ReadReg(LCD_TypeDef *TFTLCD, u16 LCD_Reg)
{
	LCD_WR_REG(TFTLCD, LCD_Reg);		//写入要读的寄存器序号
	//DelayUs(5);		  
	return LCD_RD_DATA(TFTLCD);		//返回读到的值
}
//开始写GRAM
void LCD_WriteRAM_Prepare(LCD_TypeDef *TFTLCD)
{
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}
 	TFTLCD->LCD_REG=lcddev->wramcmd;	  
}	 
//LCD写GRAM
//RGB_Code:颜色值
void LCD_WriteRAM(LCD_TypeDef *TFTLCD, u16 RGB_Code)
{							    
	TFTLCD->LCD_RAM = RGB_Code;//写十六位GRAM
}
//从ILI93xx读出的数据为GBR格式，而我们写入的时候为RGB格式。
//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
u16 LCD_BGR2RGB(u16 c)
{
	u16  r,g,b,rgb;   
	b=(c>>0)&0x1f;
	g=(c>>5)&0x3f;
	r=(c>>11)&0x1f;	 
	rgb=(b<<11)+(g<<5)+(r<<0);		 
	return(rgb);
} 
//当mdk -O1时间优化时需要设置
//延时i
void opt_delay(u8 i)
{
	while(i--);
}

//读取个某点的颜色值	 
//x,y:坐标
//返回值:此点的颜色
u16 LCD_ReadPoint(LCD_TypeDef *TFTLCD, u16 x,u16 y)
{
 	u16 r=0;
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}
	
	if(x>=lcddev->width||y>=lcddev->height)return 0;	//超过了范围,直接返回		   
	LCD_SetCursor(TFTLCD, x,y);	    
	LCD_WR_REG(TFTLCD, 0X22);      		 			//其他IC发送读GRAM指令
 	r=LCD_RD_DATA(TFTLCD);								//dummy Read	   
	opt_delay(2);	  
 	r=LCD_RD_DATA(TFTLCD);  		  						//实际坐标颜色
	
	return LCD_BGR2RGB(r);						//其他IC
}			 

//设置光标位置
//Xpos:横坐标
//Ypos:纵坐标
void LCD_SetCursor(LCD_TypeDef *TFTLCD, u16 Xpos, u16 Ypos)
{
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}

	if(lcddev->dir==1)Xpos=lcddev->width-1-Xpos;//横屏其实就是调转x,y坐标
	LCD_WriteReg(TFTLCD, lcddev->setxcmd, Xpos);
	LCD_WriteReg(TFTLCD, lcddev->setycmd, Ypos);
}

//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/5408/9341/5310/5510/1963等IC已经实际测试	   	   
void LCD_Scan_Dir(LCD_TypeDef *TFTLCD, u8 dir)
{
	u16 regval=0;
	u16 dirreg=0;
	
	switch(dir)
	{
		case L2R_U2D://从左到右,从上到下
			regval|=(1<<5)|(1<<4)|(0<<3); 
			break;
		case L2R_D2U://从左到右,从下到上
			regval|=(0<<5)|(1<<4)|(0<<3); 
			break;
		case R2L_U2D://从右到左,从上到下
			regval|=(1<<5)|(0<<4)|(0<<3);
			break;
		case R2L_D2U://从右到左,从下到上
			regval|=(0<<5)|(0<<4)|(0<<3); 
			break;	 
		case U2D_L2R://从上到下,从左到右
			regval|=(1<<5)|(1<<4)|(1<<3); 
			break;
		case U2D_R2L://从上到下,从右到左
			regval|=(1<<5)|(0<<4)|(1<<3); 
			break;
		case D2U_L2R://从下到上,从左到右
			regval|=(0<<5)|(1<<4)|(1<<3); 
			break;
		case D2U_R2L://从下到上,从右到左
			regval|=(0<<5)|(0<<4)|(1<<3); 
			break;	 
	} 
	dirreg=0X03;
	regval|=1<<12; 
	LCD_WriteReg(TFTLCD, dirreg,regval);
}

//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(LCD_TypeDef *TFTLCD, u16 x,u16 y)
{
	LCD_SetCursor(TFTLCD, x,y);		//设置光标位置 
	LCD_WriteRAM_Prepare(TFTLCD);	//开始写入GRAM
	TFTLCD->LCD_RAM=POINT_COLOR; 
}

//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(LCD_TypeDef *TFTLCD, u16 x,u16 y,u16 color)
{
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}
	
	if(lcddev->dir==1)x=lcddev->width-1-x;//横屏其实就是调转x,y坐标
	LCD_WriteReg(TFTLCD, lcddev->setxcmd,x);
	LCD_WriteReg(TFTLCD, lcddev->setycmd,y);
	
	TFTLCD->LCD_REG=lcddev->wramcmd; 
	TFTLCD->LCD_RAM=color; 
}

//设置LCD显示方向
//dir:0,竖屏；1,横屏
void LCD_Display_Dir(LCD_TypeDef *TFTLCD, u8 dir)
{
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}

	if(dir==0)			//竖屏
	{
		lcddev->dir=0;	//竖屏
		lcddev->width=240;
		lcddev->height=320;
		lcddev->wramcmd=0X22;
		lcddev->setxcmd=0X20;
		lcddev->setycmd=0X21;  
	}else 				//横屏
	{	  				
		lcddev->dir=1;	//横屏
		lcddev->width=320;
		lcddev->height=240;
		lcddev->wramcmd=0X22;
		lcddev->setxcmd=0X21;
		lcddev->setycmd=0X20;  
	} 
	LCD_Scan_Dir(TFTLCD, DFT_SCAN_DIR);	//默认扫描方向
}	 

//初始化lcd
//该初始化函数可以初始化各种ILI93XX液晶,但是其他函数是基于ILI9320的!!!
//在其他型号的驱动芯片上没有测试! 
void TFTLCD_Init(void)
{
	_lcd_dev *lcddev = 0;
	LCD_TypeDef *TFTLCD = 0;	
 	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  readWriteTiming; 
	FSMC_NORSRAMTimingInitTypeDef  writeTiming;

	rt_hw_lcd_led_init(14400-1,1-1);	 //1分频。PWM频率=72000000/14400=5Khz
	set_lcd_led(1, 0);	//关闭门内液晶背光

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);		 //使能FSMC时钟
	//使能PORTC,D,E,F,G以及AFIO复用功能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE|RCC_APB2Periph_GPIOF|RCC_APB2Periph_GPIOG|RCC_APB2Periph_AFIO,ENABLE);

 	//PORTD复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_14|GPIO_Pin_15;  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	  
	//PORTE复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOE, &GPIO_InitStructure); 
	  
	//PORTF复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOF, &GPIO_InitStructure); 
	  
   	//PORTG复用推挽输出	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_10;    /* NE2, NE3 */ 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOG, &GPIO_InitStructure); 
 
	readWriteTiming.FSMC_AddressSetupTime = 0x01;	 //地址建立时间（ADDSET）为2个HCLK 1/36M=27ns
    readWriteTiming.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（ADDHLD）模式A未用到	
    readWriteTiming.FSMC_DataSetupTime = 0x0f;		 // 数据保存时间为16个HCLK,因为液晶驱动IC的读数据的时候，速度不能太快，尤其对1289这个IC。
    readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
    readWriteTiming.FSMC_CLKDivision = 0x00;
    readWriteTiming.FSMC_DataLatency = 0x00;
    readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 
    
	writeTiming.FSMC_AddressSetupTime = 0x00;	 //地址建立时间（ADDSET）为1个HCLK  
    writeTiming.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（A		
    writeTiming.FSMC_DataSetupTime = 0x03;		 ////数据保存时间为4个HCLK	
    writeTiming.FSMC_BusTurnAroundDuration = 0x00;
    writeTiming.FSMC_CLKDivision = 0x00;
    writeTiming.FSMC_DataLatency = 0x00;
    writeTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 

    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable; // 不复用数据地址
    FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;// FSMC_MemoryType_SRAM;  //SRAM   
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;//存储器数据宽度为16bit   
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable; 
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;   
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;  
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;	//  存储器写使能
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;   
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable; // 读写使用不同的时序
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable; 
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming; //读写时序
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &writeTiming;  //写时序

    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM2; /* NE2, LCD_CS_IN */
    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);              /* 初始化FSMC配置 */
   	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM2, ENABLE);              /* 使能BANK2 */

    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM3; /* NE3, LCD_CS_OUT */
    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);              /* 初始化FSMC配置 */
   	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM3, ENABLE);              /* 使能BANK3 */
	
	rt_thread_delayMs(50); 					// delay 50 ms
	for (u8 i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			TFTLCD = LCD0;
			lcddev = &lcddev0;
		}
		else
		{
			TFTLCD = LCD1;
			lcddev = &lcddev1;
		}
		
		lcddev->id=LCD_ReadReg(TFTLCD, 0x0000);	//读ID（9320/9325/9328/4531/4535等IC）   
		if(lcddev->id==0x9328)//ILI9328   OK  
		{
			LCD_WriteReg(TFTLCD, 0x00EC,0x108F);// internal timeing      
			LCD_WriteReg(TFTLCD, 0x00EF,0x1234);// ADD        
			LCD_WriteReg(TFTLCD, 0x0001,0x0100);     
			LCD_WriteReg(TFTLCD, 0x0002,0x0700);//电源开启                    
			LCD_WriteReg(TFTLCD, 0x0003,(1<<12)|(3<<4)|(0<<3) );//65K    
			LCD_WriteReg(TFTLCD, 0x0004,0x0000);                                   
			LCD_WriteReg(TFTLCD, 0x0008,0x0202);	           
			LCD_WriteReg(TFTLCD, 0x0009,0x0000);         
			LCD_WriteReg(TFTLCD, 0x000a,0x0000);//display setting         
			LCD_WriteReg(TFTLCD, 0x000c,0x0001);//display setting          
			LCD_WriteReg(TFTLCD, 0x000d,0x0000);//0f3c          
			LCD_WriteReg(TFTLCD, 0x000f,0x0000);
			//电源配置
			LCD_WriteReg(TFTLCD, 0x0010,0x0000);   
			LCD_WriteReg(TFTLCD, 0x0011,0x0007);
			LCD_WriteReg(TFTLCD, 0x0012,0x0000);                                                                 
			LCD_WriteReg(TFTLCD, 0x0013,0x0000);                 
			LCD_WriteReg(TFTLCD, 0x0007,0x0001);                 
			rt_thread_delayMs(50); 
			LCD_WriteReg(TFTLCD, 0x0010,0x1490);   
			LCD_WriteReg(TFTLCD, 0x0011,0x0227);
			rt_thread_delayMs(50); 
			LCD_WriteReg(TFTLCD, 0x0012,0x008A);                  
			rt_thread_delayMs(50); 
			LCD_WriteReg(TFTLCD, 0x0013,0x1a00);   
			LCD_WriteReg(TFTLCD, 0x0029,0x0006);
			LCD_WriteReg(TFTLCD, 0x002b,0x000d);
			rt_thread_delayMs(50); 
			LCD_WriteReg(TFTLCD,0x0020,0x0000);                                                            
			LCD_WriteReg(TFTLCD,0x0021,0x0000);           
			rt_thread_delayMs(50); 
			//伽马校正
			LCD_WriteReg(TFTLCD, 0x0030,0x0000); 
			LCD_WriteReg(TFTLCD, 0x0031,0x0604);   
			LCD_WriteReg(TFTLCD, 0x0032,0x0305);
			LCD_WriteReg(TFTLCD, 0x0035,0x0000);
			LCD_WriteReg(TFTLCD, 0x0036,0x0C09); 
			LCD_WriteReg(TFTLCD, 0x0037,0x0204);
			LCD_WriteReg(TFTLCD, 0x0038,0x0301);        
			LCD_WriteReg(TFTLCD, 0x0039,0x0707);     
			LCD_WriteReg(TFTLCD, 0x003c,0x0000);
			LCD_WriteReg(TFTLCD, 0x003d,0x0a0a);
			rt_thread_delayMs(50); 
			LCD_WriteReg(TFTLCD, 0x0050,0x0000); //水平GRAM起始位置 
			LCD_WriteReg(TFTLCD, 0x0051,0x00ef); //水平GRAM终止位置                    
			LCD_WriteReg(TFTLCD, 0x0052,0x0000); //垂直GRAM起始位置                    
			LCD_WriteReg(TFTLCD, 0x0053,0x013f); //垂直GRAM终止位置  

			LCD_WriteReg(TFTLCD, 0x0060,0xa700);        
			LCD_WriteReg(TFTLCD, 0x0061,0x0001); 
			LCD_WriteReg(TFTLCD, 0x006a,0x0000);
			LCD_WriteReg(TFTLCD, 0x0080,0x0000);
			LCD_WriteReg(TFTLCD, 0x0081,0x0000);
			LCD_WriteReg(TFTLCD, 0x0082,0x0000);
			LCD_WriteReg(TFTLCD, 0x0083,0x0000);
			LCD_WriteReg(TFTLCD, 0x0084,0x0000);
			LCD_WriteReg(TFTLCD, 0x0085,0x0000);

			LCD_WriteReg(TFTLCD, 0x0090,0x0010);     
			LCD_WriteReg(TFTLCD, 0x0092,0x0600);  
			//开启显示设置     
			LCD_WriteReg(TFTLCD, 0x0007,0x0133); 
		} 
		LCD_Display_Dir(TFTLCD, 1);	//EMWIN实验默认设置为横屏
		LCD_Clear(TFTLCD, BLACK);		//显示全黑
	}
	set_lcd_led(1, 250);	//点亮门内液晶背光
	set_lcd_led(0, 250);	//点亮门外液晶背光
}

//清屏函数
//color:要清屏的填充色
void LCD_Clear(LCD_TypeDef *TFTLCD, u16 color)
{
	u32 index=0;      
	u32 totalpoint;
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}
	
	totalpoint=lcddev->width;
	totalpoint*=lcddev->height; 			//得到总点数
 	LCD_SetCursor(TFTLCD, 0x00,0x0000);	//设置光标位置 
	LCD_WriteRAM_Prepare(TFTLCD);     		//开始写入GRAM	 	  
	for(index=0;index<totalpoint;index++)
	{
		TFTLCD->LCD_RAM=color;	
	}
}  
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Fill(LCD_TypeDef *TFTLCD, u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{          
	u16 i,j;
	u16 xlen=0;
	
	xlen=ex-sx+1;	 
	for(i=sy;i<=ey;i++)
	{
		LCD_SetCursor(TFTLCD, sx,i);      				//设置光标位置 
		LCD_WriteRAM_Prepare(TFTLCD);     			//开始写入GRAM	  
		for(j=0;j<xlen;j++)TFTLCD->LCD_RAM=color;	//显示颜色 	    
	}
}
//在指定区域内填充指定颜色块			 
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
void LCD_Color_Fill(LCD_TypeDef *TFTLCD, u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)
{  
	u16 height,width;
	u16 i,j;
	width=ex-sx+1; 			//得到填充的宽度
	height=ey-sy+1;			//高度
 	for(i=0;i<height;i++)
	{
 		LCD_SetCursor(TFTLCD, sx,sy+i);   	//设置光标位置 
		LCD_WriteRAM_Prepare(TFTLCD);     //开始写入GRAM
		for(j=0;j<width;j++)TFTLCD->LCD_RAM=color[i*width+j];//写入数据 
	}		  
}  
//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void LCD_DrawLine(LCD_TypeDef *TFTLCD, u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		LCD_DrawPoint(TFTLCD, uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    
//画矩形	  
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(LCD_TypeDef *TFTLCD, u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(TFTLCD, x1,y1,x2,y1);
	LCD_DrawLine(TFTLCD, x1,y1,x1,y2);
	LCD_DrawLine(TFTLCD, x1,y2,x2,y2);
	LCD_DrawLine(TFTLCD, x2,y1,x2,y2);
}
//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void LCD_Draw_Circle(LCD_TypeDef *TFTLCD, u16 x0,u16 y0,u8 r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		LCD_DrawPoint(TFTLCD, x0+a,y0-b);             //5
 		LCD_DrawPoint(TFTLCD, x0+b,y0-a);             //0           
		LCD_DrawPoint(TFTLCD, x0+b,y0+a);             //4               
		LCD_DrawPoint(TFTLCD, x0+a,y0+b);             //6 
		LCD_DrawPoint(TFTLCD, x0-a,y0+b);             //1       
 		LCD_DrawPoint(TFTLCD, x0-b,y0+a);             
		LCD_DrawPoint(TFTLCD, x0-a,y0-b);             //2             
  		LCD_DrawPoint(TFTLCD, x0-b,y0-a);             //7     	         
		a++;
		//使用Bresenham算法画圆     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
} 									  
//在指定位置显示一个字符
//x,y:起始坐标
//num:要显示的字符:" "--->"~"
//size:字体大小 12/16/24
//mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(LCD_TypeDef *TFTLCD, u16 x,u16 y,u8 num,u8 size,u8 mode)
{  							  
    u8 temp,t1,t;
	u16 y0=y;
	u8 csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}

 	num=num-' ';//得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for(t=0;t<csize;t++)
	{   
		if(size==12)temp=asc2_1206[num][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[num][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[num][t];	//调用2412字体
		else return;								//没有的字库
		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)LCD_Fast_DrawPoint(TFTLCD, x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(TFTLCD, x,y,BACK_COLOR);
			temp<<=1;
			y++;
			if(y>=lcddev->height)return;		//超区域了
			if((y-y0)==size)
			{
				y=y0;
				x++;
				if(x>=lcddev->width)return;	//超区域了
				break;
			}
		}  	 
	}  	    	   	 	  
}   
//m^n函数
//返回值:m^n次方.
u32 LCD_Pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}			

//显示数字,高位为0,则不显示
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//color:颜色 
//num:数值(0~4294967295);	 
void LCD_ShowNum(LCD_TypeDef *TFTLCD, u16 x,u16 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(TFTLCD, x+(size/2)*t,y,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(TFTLCD, x+(size/2)*t,y,temp+'0',size,0); 
	}
} 
//显示数字,高位为0,还是显示
//x,y:起点坐标
//num:数值(0~999999999);	 
//len:长度(即要显示的位数)
//size:字体大小
//mode:
//[7]:0,不填充;1,填充0.
//[6:1]:保留
//[0]:0,非叠加显示;1,叠加显示.
void LCD_ShowxNum(LCD_TypeDef *TFTLCD, u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
{  
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				if(mode&0X80)LCD_ShowChar(TFTLCD, x+(size/2)*t,y,'0',size,mode&0X01);  
				else LCD_ShowChar(TFTLCD, x+(size/2)*t,y,' ',size,mode&0X01);  
 				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(TFTLCD, x+(size/2)*t,y,temp+'0',size,mode&0X01); 
	}
} 
//显示字符串
//x,y:起点坐标
//width,height:区域大小  
//size:字体大小
//*p:字符串起始地址		  
void LCD_ShowString(LCD_TypeDef *TFTLCD, u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)
{         
	u8 x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//退出
        LCD_ShowChar(TFTLCD, x,y,*p,size,0);
        x+=size/2;
        p++;
    }  
}
