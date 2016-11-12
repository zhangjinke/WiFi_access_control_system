#include "esp8266.h"
#include "sys.h"
#include "spi_bus.h"
#include <drivers/spi.h>
#include <rtthread.h>
#include "finsh.h"

#define delay_ms(ms) rt_thread_delay(rt_tick_from_millisecond(ms))

#define start_bit (1 << 7)
#define end_bit (1 << 6)

struct rt_spi_device *rt_spi_esp8266_device;//esp8266设备
struct rt_spi_message esp8266_message;	//SPI设备通信用消息结构体

static u8 esp8266_readBuf[64],esp8266_writeBuf[64];	//SPI通信缓存
u8 wr_rdy = 1, rd_rdy = 0;

/* esp8266事件控制块 */
struct rt_event esp8266_event;

/*******************************************************************************
* 函数名 	: esp8266_spi_transmit
* 描述   	: SPI传输函数
* 输入     	: - cmd: 传输指令
*			    MASTER_WRITE_DATA_TO_SLAVE_CMD    2
*			    MASTER_READ_DATA_FROM_SLAVE_CMD   3
*			    MASTER_WRITE_STATUS_TO_SLAVE_CMD  1
*			    MASTER_READ_STATUS_FROM_SLAVE_CMD 4
*             - addr: 地址，通常为0
*             - buf: 缓冲区，传入写入数据或传出读出数据
* 输出     	: None
* 返回值    : None
*******************************************************************************/
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


/*******************************************************************************
* 函数名 	: esp8266_spi_write
* 描述   	: SPI传输函数
*             首字节固定为command byte，用于大于31字节数据包的分包辅助命令。
*             11xx xxxx 两位，作为数据分包标志，首包1xxx xxxx，末包x1xx xxxx。
*             此两位不置，表示是中间的数据分包。xx11 1111 共6位数据，表示当前包的长度
* 输入     	: - cmd: 传输指令
* 输出     	: None
* 返回值    : -1: 失败 0: 成功
*******************************************************************************/
s8 esp8266_spi_write(u8 *pack, u32 lenth)
{
	int i,j,t = 50000;
	u8 buf[32];
	int num = lenth/31 + 1;		//包数
	int last_byte = lenth%31;	//最后一包有效字节数
	
	if (last_byte == 0)
	{
		num--;
		last_byte = 31;
	}
	/* 检查参数合法性 */
	if (pack == NULL)
	{
		return -1;
	}
	
	for (i = 0; i < num; i++)
	{
		buf[0] = 0;                              	/* 清空command byte */
		/* 设置command byte */
		if (i != num - 1)        					/* 如果不为首末包,设置当前包长度 */
		{
			buf[0] = 31; 
		}
		else                              			/* 设置当前包长度并添加末包标志位 */
		{
			buf[0] = (end_bit | last_byte);
		}
		if (i == 0) { buf[0] |= start_bit;}     	/* 添加首包标志位 */
		
		/* 将数据复制到发送buf中 */
		if(i == num - 1) 
		{
			for (j = 0; j < last_byte; j++) 
			{
				buf[j + 1] = pack[i*31 + j]; 
			}
		}
		else
		{
			for (j = 0; j < 31; j++) 
			{
				buf[j + 1] = pack[i*31 + j];
			}
		}
		
		/* 等待发送准备好标志位 */
		while((wr_rdy != 1)&&(t != 0))
		{
			t--;
		}
		wr_rdy = 0;
		if (t == 0)
		{
			return -1; /* 等待超时，返回错误 */
		}
		/* 启动一次SPI传输 */
		esp8266_spi_transmit(0x02, 0, buf);	
	}
	
	return 0;
}

s8 WriteTest(u32 lenth)
{
	u8 buf[1024*5];
	int i;
	
	for (i = 0; i < sizeof(buf); i++)
	{
		buf[i] = (u8)i;
	}
	return esp8266_spi_write(buf,lenth);
}
FINSH_FUNCTION_EXPORT(WriteTest, WiFiWriteTest)

volatile static u8 temp = 0;
s8 esp8266_spi_read(void)
{
	int i = 0;
	static u8 buf[32], pack[1024*5];
	static u8 isReceive = 0, receive_pack = 0;
	static u32 pack_counter = 0, pack_lenth = 0;
	rt_memset(buf, 0, sizeof(buf));
	/* 启动一次SPI传输 */
	esp8266_spi_transmit(0x03, 0, buf);
temp++;
if (temp == 2)
{
	temp = temp;
}
	if (( buf[0] & start_bit) == start_bit)
	{
		i = 0;
		receive_pack = 0;
		isReceive = 1;
		pack_counter = 0;
		pack_lenth = 0;
//		rt_kprintf("\r\nstart_bit\r\n");
	}
	if (isReceive)
	{
		pack_lenth += buf[0] & 0x3f;
		if (( buf[0] & end_bit) == end_bit)
		{
			for (i = 0; i < (buf[0] & 0x3f); i++)
			{
				pack[pack_counter*31 + i] = buf[i + 1];
			}
			isReceive = 0;
			receive_pack = 1;
//			rt_kprintf("end_bit\r\n");
		}
		else
		{
			for (i = 0; i < 31; i++)
			{
				pack[pack_counter*31 + i] = buf[i + 1];
			}
			pack_counter++;
		}
	}
	if (receive_pack)
	{
		receive_pack = 0;
		rt_kprintf("data lenth is %d: \r\n",pack_lenth);
		for(i=0;i<pack_lenth;i++)
		{
			rt_kprintf("%02X ",pack[i]);
		}
		rt_kprintf("\r\n");
	}
	i++;
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
		cfg.max_hz = 30 * 1000 * 1000;
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
		/* 发送hspi接收事件 */
		rt_event_send(&esp8266_event, hspi_rx);	
		/* 清除LINE7上的中断标志位 */
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
}
