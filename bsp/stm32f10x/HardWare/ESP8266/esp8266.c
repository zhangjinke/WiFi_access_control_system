#include "esp8266.h"
#include "sys.h"
#include "spi_bus.h"
#include <drivers/spi.h>
#include <rtthread.h>


#define delay_ms(ms) rt_thread_delay(rt_tick_from_millisecond(ms))

struct rt_spi_device *rt_spi_esp8266_device;//esp8266设备
struct rt_spi_message esp8266_message;	//SPI设备通信用消息结构体

u8 esp8266_readBuf[64],esp8266_writeBuf[64];	//SPI通信缓存
u8 wr_rdy = 1, rd_rdy = 1;

//MASTER_WRITE_DATA_TO_SLAVE_CMD 	2
//MASTER_READ_DATA_FROM_SLAVE_CMD 	3
//MASTER_WRITE_STATUS_TO_SLAVE_CMD 	1
//MASTER_READ_STATUS_FROM_SLAVE_CMD 4

//static u32 counter = 0x01;

void esp8266_spi_transmit(u8 cmd, u8 addr, u8 *buf)
{
	int i;
	/* 参数范围检测 */
	if (((cmd != 2)&&(cmd != 3))||(buf == NULL))
	{
		return;
	}
	esp8266_writeBuf[0] = cmd;
	esp8266_writeBuf[1] = addr;
	/* 0x02 写数据  0x03读数据 */
	if (cmd == 0x02)
	{
		for (i = 0; i < 32; i++) { esp8266_writeBuf[i + 2] = buf[i]; }
	}
	else
	{
		for (i = 0; i < 32; i++) { esp8266_writeBuf[i + 2] = 0; }
	}
		
	rt_memset(esp8266_readBuf,0,sizeof(esp8266_readBuf));
	esp8266_message.send_buf = esp8266_writeBuf;
	esp8266_message.recv_buf = esp8266_readBuf;	//设置读写缓存
	esp8266_message.length = 34;				//设置读写长度
	esp8266_message.cs_take = 1;				//开始通信时拉低CS
	esp8266_message.cs_release = 1;				//结束通信时拉高CS
	esp8266_message.next = RT_NULL;
	rt_spi_transfer_message(rt_spi_esp8266_device, &esp8266_message);//进行一次数据传输
	
	/* 将读取到的数据写入buf中 */
	if (cmd == 0x03)
	{
		for (i = 0; i < 32; i++) { buf[i] = esp8266_readBuf[i + 2]; }
	}
}

u8 buf[32];
u8 pack[1024];
//首字节固定为command byte，用于大于31字节数据包的分包辅助命令。
//11xx xxxx 两位，作为数据分包标志，首包1xxx xxxx，末包x1xx xxxx。
//此两位不置，表示是中间的数据分包。xx11 1111 共6位数据，表示当前包的长度
void WriteTest(void)
{
	int i,j;
	int num = 1024/31 + 1;			//包数
	int last_byte = 1024%31;	//最后一包有效字节数
	
	/* 赋初值 */
	for (i = 0; i < sizeof(pack); i++) { pack[i] = (u8)i; }
	
	for (i = 0; i < num; i++)
	{
		/* 设置command byte */
		if (i == 0) { buf[0] = ((1<<7) | 31);}
		else if (i != num - 1) { buf[0] = 31; }
		else { buf[0] = ((1<<6) | last_byte); }
		/* 将数据复制到发送buf中 */
		if(i == num - 1) { for (j = 0; j < last_byte; j++) { buf[j + 1] = pack[i*31 + j]; } }
		else { for (j = 0; j < 31; j++) { buf[j + 1] = pack[i*31 + j]; } }
		
		while(wr_rdy != 1);
		wr_rdy = 0;
		esp8266_spi_transmit(0x02, 0, buf);	
	}
}

void ReadTest(void)
{
	int i;
	
	while(rd_rdy != 1);
	for (i = 0; i < 32; i++) { buf[i] = i; }
	
	esp8266_spi_transmit(0x03, 0, buf);

//	rt_kprintf("\r\nesp8266 read Buf is \r\n");
//	for (i = 0; i < 34; i++)
//	{
//		rt_kprintf("%02X ", esp8266_readBuf[i]);
//	}
}

void esp8266_exti_init(void)
{
 	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;
//	GPIO_InitTypeDef  GPIO_InitStructure;
	
 	//使能端口时钟
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOF, ENABLE);
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//使能复用功能时钟

//	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;//C4
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //设置成下拉输入
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//IO口速度为50MHz
// 	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化		

//	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_7;//F7
// 	GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化		
	
    //GPIOC.4 中断线以及中断初始化配置 上升沿触发
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource4);

  	EXTI_InitStructure.EXTI_Line = EXTI_Line4;	//GOIO0
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

    //GPIOF.7 中断线以及中断初始化配置 上升沿触发
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOF,GPIO_PinSource7);
	
  	EXTI_InitStructure.EXTI_Line = EXTI_Line7;	//GOIO2
  	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

  	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;				//使能GOIO0所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	//抢占优先级2， 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;			//子优先级3
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure); 

    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;				//使能GOIO2所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	//抢占优先级2， 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;			//子优先级2
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure);
}

void esp8266_attach_device()
{
	static struct rt_spi_device esp8266_spi_device;
	static struct stm32_spi_cs  esp8266_spi_cs;
	GPIO_InitTypeDef  GPIO_InitStructure;
	
 	//使能端口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB, ENABLE);
	
	//配置esp8266 RST引脚
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;//B0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//IO口速度为50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化	
	//配置RC522_IN_CS引脚
    esp8266_spi_cs.GPIOx = GPIOA;
    esp8266_spi_cs.GPIO_Pin = GPIO_Pin_4;

	GPIO_InitStructure.GPIO_Pin = esp8266_spi_cs.GPIO_Pin;//esp8266_cs
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//IO口速度为50MHz
	GPIO_Init(esp8266_spi_cs.GPIOx, &GPIO_InitStructure);		//根据设定参数初始化
	GPIO_SetBits(esp8266_spi_cs.GPIOx,esp8266_spi_cs.GPIO_Pin);//输出高
	//附着设备到SPI总线
	rt_spi_bus_attach_device(&esp8266_spi_device, "ESP8266", "SPI1", (void*)&esp8266_spi_cs);
}

s8 init_esp8266(void)
{
	/* 在SPI1_BUS上附着esp8266设备 */
	esp8266_attach_device();
	/* 配置esp8266设备 */
	rt_spi_esp8266_device = (struct rt_spi_device *) rt_device_find("ESP8266");//查找设备
	if (rt_spi_esp8266_device == RT_NULL)
	{
		return -1;
	}

	/* config spi */
	{
		struct rt_spi_configuration cfg;
		cfg.data_width = 8;
		cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
		cfg.max_hz = 0.1 * 1000 * 1000;
		rt_spi_configure(rt_spi_esp8266_device, &cfg);
	}
	/* 初始化中断线 */
	esp8266_exti_init();
	
	return 0;
}

void check_state_line(void)
{
	if (esp8266_wr_state == 1) { wr_rdy = 1; }
	if (esp8266_rd_state == 1) { rd_rdy = 1; }
}

/* GPIO0上升沿中断 */
void EXTI4_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line4) == SET)
	{
		/* 主机发送数据处理完毕，可以进行下一次写入 */
		wr_rdy = 1;
		/* 清除LINE4上的中断标志位 */
		EXTI_ClearITPendingBit(EXTI_Line4);
	}
}

/* GPIO2上升沿中断 */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line7) == SET)
	{
		/* 从机更新发送缓存，主机可以读取 */
		rd_rdy = 1;
		/* 清除LINE7上的中断标志位 */
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
}
