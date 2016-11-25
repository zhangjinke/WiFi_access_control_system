#include "touch_screen.h" 
#include "ILI93xx.h"
#include "stdlib.h"
#include "math.h"
#include "spi_bus.h"
#include <drivers/spi.h>
#include <rtthread.h>
#include <dfs_posix.h>

#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

struct rt_spi_device *rt_spi_tp_device;
struct rt_spi_message rt_spi_tp_message;
const char *file_name = "/tp_adj.bin";

_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0, 
	0,
	0,
	0,
	0,	  	 		
	0,
	0,	  	 		
};					
//默认为touchtype=0的数据.
u8 CMD_RDX=0XD0;
u8 CMD_RDY=0X90;

u8 tp_readBuf[1],tp_writeBuf[1];
//SPI读数据 
//从触摸屏IC读取adc值
//CMD:指令
//返回值:读到的数据	   
u16 TP_Read_AD(u8 CMD)	  
{ 	 
	u16 Num=0;
	tp_writeBuf[0] = CMD;
	rt_spi_tp_message.send_buf = tp_writeBuf;
	rt_spi_tp_message.recv_buf = tp_readBuf;	//设置读写缓存
	rt_spi_tp_message.length = 1;			//设置读写长度
	rt_spi_tp_message.cs_take = 1;			//开始通信时拉低CS
	rt_spi_tp_message.cs_release = 0;		//结束通信时不拉高CS
	rt_spi_tp_message.next = RT_NULL;
	rt_spi_transfer_message(rt_spi_tp_device, &rt_spi_tp_message);//进行一次数据传输
	
	tp_writeBuf[0] = 0x00;
	rt_spi_tp_message.cs_take = 0;			//开始通信时不拉低CS
	rt_spi_transfer_message(rt_spi_tp_device, &rt_spi_tp_message);//进行一次数据传输
	Num = tp_readBuf[0];
	Num <<= 8;
	
	rt_spi_tp_message.cs_release = 1;		//结束通信时拉高CS
	rt_spi_transfer_message(rt_spi_tp_device, &rt_spi_tp_message);//进行一次数据传输
	Num += tp_readBuf[0];
	
	Num >>= 3;//取有效位
	return(Num);   
}
//读取一个坐标值(x或者y)
//连续读取READ_TIMES次数据,对这些数据升序排列,
//然后去掉最低和最高LOST_VAL个数,取平均值 
//xy:指令（CMD_RDX/CMD_RDY）
//返回值:读到的数据
#define READ_TIMES 5 	//读取次数
#define LOST_VAL 1	  	//丢弃值
u16 TP_Read_XOY(u8 xy)
{
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;
	u16 temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);		 		    
	for(i=0;i<READ_TIMES-1; i++)//排序
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//升序排列
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 
//读取x,y坐标
//最小值不能少于100.
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
u8 TP_Read_XY(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;			 	 		  
	xtemp=TP_Read_XOY(CMD_RDX);
	ytemp=TP_Read_XOY(CMD_RDY);	  												   
	//if(xtemp<100||ytemp<100)return 0;//读数失败
	*x=xtemp;
	*y=ytemp;
	return 1;//读数成功
}
//连续2次读取触摸屏IC,且这两次的偏差不能超过
//ERR_RANGE,满足条件,则认为读数正确,否则读数错误.	   
//该函数能大大提高准确度
//x,y:读取到的坐标值
//返回值:0,失败;1,成功。
#define ERR_RANGE 50 //误差范围 
u8 TP_Read_XY2(u16 *x,u16 *y) 
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
    flag=TP_Read_XY(&x1,&y1);   
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);	   
    if(flag==0)return(0);   
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;	  
}  
//////////////////////////////////////////////////////////////////////////////////		  
//与LCD部分有关的函数  
//画一个触摸点
//用来校准用的
//x,y:坐标
//color:颜色
void TP_Drow_Touch_Point(LCD_TypeDef *TFTLCD, u16 x,u16 y,u16 color)
{
	POINT_COLOR=color;
	LCD_DrawLine(TFTLCD, x-12,y,x+13,y);//横线
	LCD_DrawLine(TFTLCD, x,y-12,x,y+13);//竖线
	LCD_DrawPoint(TFTLCD, x+1,y+1);
	LCD_DrawPoint(TFTLCD, x-1,y+1);
	LCD_DrawPoint(TFTLCD, x+1,y-1);
	LCD_DrawPoint(TFTLCD, x-1,y-1);
	LCD_Draw_Circle(TFTLCD, x,y,6);//画中心圈
}	  
//画一个大点(2*2的点)		   
//x,y:坐标
//color:颜色
void TP_Draw_Big_Point(LCD_TypeDef *TFTLCD, u16 x,u16 y,u16 color)
{	    
	POINT_COLOR=color;
	LCD_DrawPoint(TFTLCD, x,y);//中心点 
	LCD_DrawPoint(TFTLCD, x+1,y);
	LCD_DrawPoint(TFTLCD, x,y+1);
	LCD_DrawPoint(TFTLCD, x+1,y+1);	 	  	
}						  
//////////////////////////////////////////////////////////////////////////////////		  
//触摸按键扫描
//tp:0,屏幕坐标;1,物理坐标(校准等特殊场合用)
//返回值:当前触屏状态.
//0,触屏无触摸;1,触屏有触摸
u8 TP_Scan(u8 tp)
{			   
	if(PEN==0)//有按键按下
	{
		if(tp)TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]);//读取物理坐标
		else if(TP_Read_XY2(&tp_dev.x[0],&tp_dev.y[0]))//读取屏幕坐标
		{
	 		tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xoff;//将结果转换为屏幕坐标
			tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yoff;  
	 	} 
		if((tp_dev.sta&TP_PRES_DOWN)==0)//之前没有被按下
		{		 
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;//按键按下  
			tp_dev.x[4]=tp_dev.x[0];//记录第一次按下时的坐标
			tp_dev.y[4]=tp_dev.y[0];  	   			 
		}			   
	}else
	{
		if(tp_dev.sta&TP_PRES_DOWN)//之前是被按下的
		{
			tp_dev.sta&=~(1<<7);//标记按键松开	
		}else//之前就没有被按下
		{
			tp_dev.x[4]=0;
			tp_dev.y[4]=0;
			tp_dev.x[0]=0xffff;
			tp_dev.y[0]=0xffff;
		}	    
	}
	return tp_dev.sta&TP_PRES_DOWN;//返回当前的触屏状态
}	  
//////////////////////////////////////////////////////////////////////////	 
//保存在EEPROM里面的地址区间基址,占用14个字节(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+13)
#define SAVE_ADDR_BASE 40
//保存校准参数										    
void TP_Save_Adjdata(void)
{
	int fd, size;
	char buffer[14];
	
	rt_memcpy(buffer, (u8*)&tp_dev.xfac, 13);//将要写入的数据存入缓存
	buffer[13] = 0x0A;//在最后，写0X0A标记校准过了
	fd = open(file_name, O_RDWR|O_CREAT, 0);
	if (fd >= 0)
	{
		size = write(fd, buffer, 14);//强制保存&tp_dev.xfac地址开始的14个字节数据，即保存到tp_dev.touchtype
		if(size == 14) { rt_kprintf("save %s is success\r\n",file_name); }
		else { rt_kprintf("save %s is failed\r\n",file_name); }
	}
	else
	{
		rt_kprintf("open %s is failed\r\n",file_name);
	}
	close(fd);//关闭文件
}
//得到保存在EEPROM里面的校准值
//返回值：1，成功获取数据
//        0，获取失败，要重新校准
u8 TP_Get_Adjdata(void)
{					  
	u8 temp;
	int fd, size;
	char buffer[14];
	
	fd = open(file_name, O_RDWR|O_CREAT, 0);
	if (fd >= 0)
	{
		size = read(fd, buffer, 14);
		if(size == 14)
		{
			temp = buffer[13];//读取标记字,看是否校准过！
			if(temp==0X0A)//触摸屏已经校准过了			   
			{
				rt_memcpy((u8*)&tp_dev.xfac, buffer, 15);//获取之前保存的校准数据 
				if(tp_dev.touchtype)//X,Y方向与屏幕相反
				{
					CMD_RDX=0X90;
					CMD_RDY=0XD0;	 
				}else				   //X,Y方向与屏幕相同
				{
					CMD_RDX=0XD0;
					CMD_RDY=0X90;
				}
				close(fd);//关闭文件
				return 1;	 
			}
		}
	}
	else
	{
		rt_kprintf("open %s is failed\r\n",file_name);
	}
	close(fd);//关闭文件
	return 0;
}	 
//提示字符串
u8* const TP_REMIND_MSG_TBL="Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";
 					  
//提示校准结果(各个参数)
void TP_Adj_Info_Show(LCD_TypeDef *TFTLCD, u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
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

	POINT_COLOR=RED;
	LCD_ShowString(TFTLCD, 40,160,lcddev->width,lcddev->height,16,"x1:");
 	LCD_ShowString(TFTLCD, 40+80,160,lcddev->width,lcddev->height,16,"y1:");
 	LCD_ShowString(TFTLCD, 40,180,lcddev->width,lcddev->height,16,"x2:");
 	LCD_ShowString(TFTLCD, 40+80,180,lcddev->width,lcddev->height,16,"y2:");
	LCD_ShowString(TFTLCD, 40,200,lcddev->width,lcddev->height,16,"x3:");
 	LCD_ShowString(TFTLCD, 40+80,200,lcddev->width,lcddev->height,16,"y3:");
	LCD_ShowString(TFTLCD, 40,220,lcddev->width,lcddev->height,16,"x4:");
 	LCD_ShowString(TFTLCD, 40+80,220,lcddev->width,lcddev->height,16,"y4:");  
 	LCD_ShowString(TFTLCD, 40,240,lcddev->width,lcddev->height,16,"fac is:");     
	LCD_ShowNum(TFTLCD, 40+24,160,x0,4,16);		//显示数值
	LCD_ShowNum(TFTLCD, 40+24+80,160,y0,4,16);	//显示数值
	LCD_ShowNum(TFTLCD, 40+24,180,x1,4,16);		//显示数值
	LCD_ShowNum(TFTLCD, 40+24+80,180,y1,4,16);	//显示数值
	LCD_ShowNum(TFTLCD, 40+24,200,x2,4,16);		//显示数值
	LCD_ShowNum(TFTLCD, 40+24+80,200,y2,4,16);	//显示数值
	LCD_ShowNum(TFTLCD, 40+24,220,x3,4,16);		//显示数值
	LCD_ShowNum(TFTLCD, 40+24+80,220,y3,4,16);	//显示数值
 	LCD_ShowNum(TFTLCD, 40+56,240,fac,3,16); 	//显示数值,该数值必须在95~105范围之内.
}
		 
//触摸屏校准代码
//得到四个校准参数
void TP_Adjust(LCD_TypeDef *TFTLCD)
{								 
	u16 pos_temp[4][2];//坐标缓存值
	u8  cnt=0;	
	u16 d1,d2;
	u32 tem1,tem2;
	double fac; 	
	u16 outtime=0;
	_lcd_dev *lcddev;
	
	if (TFTLCD == LCD0)
	{
		lcddev = &lcddev0;
	}
	else
	{
		lcddev = &lcddev1;
	}
 	cnt=0;				
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(TFTLCD, WHITE);//清屏   
	POINT_COLOR=RED;//红色 
	LCD_Clear(TFTLCD, WHITE);//清屏 	   
	POINT_COLOR=BLACK;
	LCD_ShowString(TFTLCD, 40,40,160,100,16,(u8*)TP_REMIND_MSG_TBL);//显示提示信息
	TP_Drow_Touch_Point(TFTLCD, 20,20,RED);//画点1 
	tp_dev.sta=0;//消除触发信号 
	tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误	 
	while(1)//如果连续10秒钟没有按下,则自动退出
	{
		tp_dev.scan(1);//扫描物理坐标
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
		{	
			outtime=0;		
			tp_dev.sta&=~(1<<6);//标记按键已经被处理过了.
						   			   
			pos_temp[cnt][0]=tp_dev.x[0];
			pos_temp[cnt][1]=tp_dev.y[0];
			cnt++;	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(TFTLCD, 20,20,WHITE);				//清除点1 
					TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,20,RED);	//画点2
					break;
				case 2:
 					TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,20,WHITE);	//清除点2
					TP_Drow_Touch_Point(TFTLCD, 20,lcddev->height-20,RED);	//画点3
					break;
				case 3:
 					TP_Drow_Touch_Point(TFTLCD, 20,lcddev->height-20,WHITE);			//清除点3
 					TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,lcddev->height-20,RED);	//画点4
					break;
				case 4:	 //全部四个点已经得到
	    		    //对边相等
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,2的距离
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到3,4的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05||d1==0||d2==0)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,lcddev->height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(TFTLCD, 20,20,RED);								//画点1
 						TP_Adj_Info_Show(TFTLCD, pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,3的距离
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,4的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,lcddev->height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(TFTLCD, 20,20,RED);								//画点1
 						TP_Adj_Info_Show(TFTLCD, pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						continue;
					}//正确了
								   
					//对角线相等
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,4的距离
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,3的距离
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//不合格
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,lcddev->height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(TFTLCD, 20,20,RED);								//画点1
 						TP_Adj_Info_Show(TFTLCD, pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						continue;
					}//正确了
					//计算结果
					tp_dev.xfac=(float)(lcddev->width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac		 
					tp_dev.xoff=(lcddev->width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff
						  
					tp_dev.yfac=(float)(lcddev->height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
					tp_dev.yoff=(lcddev->height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff  
					if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(TFTLCD, lcddev->width-20,lcddev->height-20,WHITE);	//清除点4
   	 					TP_Drow_Touch_Point(TFTLCD, 20,20,RED);								//画点1
						LCD_ShowString(TFTLCD, 40,26,lcddev->width,lcddev->height,16,"TP Need readjust!");
						tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
						if(tp_dev.touchtype)//X,Y方向与屏幕相反
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else				   //X,Y方向与屏幕相同
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					POINT_COLOR=BLUE;
					LCD_Clear(TFTLCD, WHITE);//清屏
					LCD_ShowString(TFTLCD, 35,110,lcddev->width,lcddev->height,16,"Touch Screen Adjust OK!");//校正完成
					rt_thread_delayMs(1000);
					TP_Save_Adjdata();  
 					LCD_Clear(TFTLCD, WHITE);//清屏   
					return;//校正完成				 
			}
		}
		rt_thread_delayMs(10);
		outtime++;
		if(outtime>1000)
		{
			TP_Get_Adjdata();
			break;
	 	} 
 	}
}

void tp_attach_device()
{
	static struct rt_spi_device spi_device;
	static struct stm32_spi_cs  spi_cs;
	GPIO_InitTypeDef  GPIO_InitStructure;
	
 	//使能PB,PG端口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOE, ENABLE);
	//配置TP_CS引脚
    spi_cs.GPIOx = GPIOE;
    spi_cs.GPIO_Pin = GPIO_Pin_2;

	GPIO_InitStructure.GPIO_Pin = spi_cs.GPIO_Pin;		//tp_cs PB12
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//IO口速度为50MHz
	GPIO_Init(spi_cs.GPIOx, &GPIO_InitStructure);		//根据设定参数初始化GPIOB.12
	GPIO_SetBits(spi_cs.GPIOx,spi_cs.GPIO_Pin);			//PB.12 输出高
	//配置中断输入引脚
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6;//B6
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOE2,3,4

	rt_spi_bus_attach_device(&spi_device, "SPI_TP", "SPI3", (void*)&spi_cs);
}

//触摸屏初始化  		    
//返回值:0,没有进行校准
//       1,进行过校准
u8 TP_Init(LCD_TypeDef *TFTLCD)
{
	/* 在SPI3_BUS上附着触摸屏设备 */
	tp_attach_device();
	
	rt_spi_tp_device = (struct rt_spi_device *) rt_device_find("SPI_TP");
	if (rt_spi_tp_device == RT_NULL)
	{
		return 0;
	}

	/* config spi */
	{
		struct rt_spi_configuration cfg;
		cfg.data_width = 8;
		cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
		cfg.max_hz = 2 * 1000 * 1000;
		rt_spi_configure(rt_spi_tp_device, &cfg);
	}
	
	TP_Read_XY(&tp_dev.x[0],&tp_dev.y[0]);//第一次读取初始化	 
	if(TP_Get_Adjdata())return 0;//已经校准
	else			  		//未校准?
	{ 										    
		LCD_Clear(TFTLCD, WHITE);	//清屏
		TP_Adjust(TFTLCD);  		//屏幕校准  
	}			
	TP_Get_Adjdata();	
	return 1; 									 
}

