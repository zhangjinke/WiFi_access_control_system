#include "esp8266.h"
#include "sys.h"
#include "spi_bus.h"
#include <drivers/spi.h>
#include <rtthread.h>


#define delay_ms(ms) rt_thread_delay(rt_tick_from_millisecond(ms))

struct rt_spi_device *rt_spi_esp8266_device;//esp8266设备
struct rt_spi_message esp8266_message;	//SPI设备通信用消息结构体

u8 esp8266_readBuf[64],esp8266_writeBuf[64];	//SPI通信缓存

//MASTER_WRITE_DATA_TO_SLAVE_CMD 	2
//MASTER_READ_DATA_FROM_SLAVE_CMD 	3
//MASTER_WRITE_STATUS_TO_SLAVE_CMD 	1
//MASTER_READ_STATUS_FROM_SLAVE_CMD 4

static u32 counter = 0x01;
void WriteTest(void)
{
	int i;
	esp8266_writeBuf[0] = 0x02;//0x02 写数据  0x03读数据
	esp8266_writeBuf[1] = 0x00;
	
	esp8266_writeBuf[2] = counter++;
	for (i = 0; i < 30; i++)
	{
		esp8266_writeBuf[i + 3] = 2;
	}
	esp8266_writeBuf[33] = esp8266_writeBuf[2];
		
	rt_memset(esp8266_readBuf,0,sizeof(esp8266_readBuf));
	esp8266_message.send_buf = esp8266_writeBuf;
	esp8266_message.recv_buf = esp8266_readBuf;	//设置读写缓存
	esp8266_message.length = 34;			//设置读写长度
	esp8266_message.cs_take = 1;			//开始通信时拉低CS
	esp8266_message.cs_release = 1;		//结束通信时拉高CS
	esp8266_message.next = RT_NULL;
	rt_spi_transfer_message(rt_spi_esp8266_device, &esp8266_message);//进行一次数据传输
	
	rt_kprintf("\r\nesp8266 write Buf is \r\n");
	for (i = 0; i < 34; i++)
	{
		rt_kprintf("%02X ", esp8266_writeBuf[i]);
	}
}

void ReadTest(void)
{
	int i;
	esp8266_writeBuf[0] = 0x03;//0x02 写数据  0x03读数据
	esp8266_writeBuf[1] = 0x00;
	
	rt_memset(esp8266_writeBuf + 2, 0, 32);
	
	rt_memset(esp8266_readBuf,0,sizeof(esp8266_readBuf));
	esp8266_message.send_buf = esp8266_writeBuf;
	esp8266_message.recv_buf = esp8266_readBuf;	//设置读写缓存
	esp8266_message.length = 34;			//设置读写长度
	esp8266_message.cs_take = 1;			//开始通信时拉低CS
	esp8266_message.cs_release = 1;		//结束通信时拉高CS
	esp8266_message.next = RT_NULL;
	rt_spi_transfer_message(rt_spi_esp8266_device, &esp8266_message);//进行一次数据传输
	
	rt_kprintf("\r\nesp8266 read Buf is \r\n");
	for (i = 0; i < 34; i++)
	{
		rt_kprintf("%02X ", esp8266_readBuf[i]);
	}
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
		
	return 0;
}
