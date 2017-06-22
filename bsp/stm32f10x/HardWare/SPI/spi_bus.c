/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: spi_bus.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 06 日
**
** 描        述: stm32 spi相关函数

** 日志:
2016.09.06  创建本文件
*********************************************************************************************************/
#include "spi_bus.h"
#include <drivers/spi.h>
#include <rtthread.h>

static rt_err_t configure(struct rt_spi_device *device, struct rt_spi_configuration *configuration)
{
    struct stm32_spi_bus *stm32_spi_bus = (struct stm32_spi_bus *)device->bus;
    
    SPI_InitTypeDef SPI_InitStructure;

    SPI_StructInit(&SPI_InitStructure);
    
    /* data_width */
    if(configuration->data_width <= 8)
    {
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    }
    else if(configuration->data_width <= 16)
    {
        SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
    }
    else
    {
        return RT_EIO;
    }
    /* baudrate */
    {
        uint32_t SPI_APB_CLOCK;
        uint32_t stm32_spi_max_clock;
        uint32_t max_hz;

        stm32_spi_max_clock = 36000000;
        max_hz = configuration->max_hz;

        if(max_hz > stm32_spi_max_clock)
        {
            max_hz = stm32_spi_max_clock;
        }
        
        if(stm32_spi_bus->SPI == SPI1) { SPI_APB_CLOCK = SystemCoreClock; }
        else { SPI_APB_CLOCK = SystemCoreClock / 2; }

        /* STM32F1xx SPI1 MAX 36Mhz */
        /* STM32F1xx SPI2/SPI3 MAX 18Mhz */
        if(max_hz >= SPI_APB_CLOCK/2)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
        }
        else if(max_hz >= SPI_APB_CLOCK/4)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
        }
        else if(max_hz >= SPI_APB_CLOCK/8)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
        }
        else if(max_hz >= SPI_APB_CLOCK/16)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
        }
        else if(max_hz >= SPI_APB_CLOCK/32)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
        }
        else if(max_hz >= SPI_APB_CLOCK/64)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
        }
        else if(max_hz >= SPI_APB_CLOCK/128)
        {
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
        }
        else
        {
            /*  min prescaler 256 */
            SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
        }
    } /* baudrate */

    /* CPOL */
    if(configuration->mode & RT_SPI_CPOL)
    {
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    }
    else
    {
        SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    }
    /* CPHA */
    if(configuration->mode & RT_SPI_CPHA)
    {
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    }
    else
    {
        SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    }
    /* MSB or LSB */
    if(configuration->mode & RT_SPI_MSB)
    {
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    }
    else
    {
        SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
    }
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_NSS  = SPI_NSS_Soft;
    SPI_InitStructure.SPI_CRCPolynomial = 7;    //CRC值计算的多项式
    /* init SPI */
    SPI_I2S_DeInit(stm32_spi_bus->SPI);
    SPI_Init(stm32_spi_bus->SPI, &SPI_InitStructure);
    /* Enable SPI_MASTER */
    SPI_Cmd(stm32_spi_bus->SPI, ENABLE);
    SPI_CalculateCRC(stm32_spi_bus->SPI, DISABLE);

    return RT_EOK;
}

static rt_uint32_t xfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    struct stm32_spi_bus *stm32_spi_bus = (struct stm32_spi_bus *)device->bus;
    struct rt_spi_configuration *config = &device->config;
    SPI_TypeDef * SPI = stm32_spi_bus->SPI;
    struct stm32_spi_cs *stm32_spi_cs = device->parent.user_data;
    rt_uint32_t size = message->length;

    /* take CS */
    if(message->cs_take)
    {
        GPIO_ResetBits(stm32_spi_cs->GPIOx, stm32_spi_cs->GPIO_Pin);
    }
    if(config->data_width <= 8)
    {
        const rt_uint8_t * send_ptr = message->send_buf;
        rt_uint8_t * recv_ptr = message->recv_buf;

        while(size--)
        {
            rt_uint8_t data = 0xFF;

            if(send_ptr != RT_NULL)
            {
                data = *send_ptr++;
            }

            //Wait until the transmit buffer is empty
            while (SPI_I2S_GetFlagStatus(SPI, SPI_I2S_FLAG_TXE) == RESET);
            // Send the byte
            SPI_I2S_SendData(SPI, data);

            //Wait until a data is received
            while (SPI_I2S_GetFlagStatus(SPI, SPI_I2S_FLAG_RXNE) == RESET);
            // Get the received data
            data = SPI_I2S_ReceiveData(SPI);

            if(recv_ptr != RT_NULL)
            {
                *recv_ptr++ = data;
            }
        }
    }
    else if(config->data_width <= 16)
    {
        const rt_uint16_t * send_ptr = message->send_buf;
        rt_uint16_t * recv_ptr = message->recv_buf;

        while(size--)
        {
            rt_uint16_t data = 0xFF;

            if(send_ptr != RT_NULL)
            {
                data = *send_ptr++;
            }

            //Wait until the transmit buffer is empty
            while (SPI_I2S_GetFlagStatus(SPI, SPI_I2S_FLAG_TXE) == RESET);
            // Send the byte
            SPI_I2S_SendData(SPI, data);

            //Wait until a data is received
            while (SPI_I2S_GetFlagStatus(SPI, SPI_I2S_FLAG_RXNE) == RESET);
            // Get the received data
            data = SPI_I2S_ReceiveData(SPI);

            if(recv_ptr != RT_NULL)
            {
                *recv_ptr++ = data;
            }
        }
    }
    /* release CS */
    if(message->cs_release)
    {
        GPIO_SetBits(stm32_spi_cs->GPIOx, stm32_spi_cs->GPIO_Pin);
    }

    return message->length;
}

static void SPI_RCC_Configuration(void)
{
#if defined(RT_USING_SPI1)
    /* Enable SPI GPIO clocks */
    RCC_APB2PeriphClockCmd(    RCC_APB2Periph_GPIOA, ENABLE );//PORTA时钟使能 
    /* Enable SPI clock */
    RCC_APB2PeriphClockCmd(    RCC_APB2Periph_SPI1,  ENABLE );//SPI1时钟使能     
#endif /* RT_USING_SPI1 */
    
#if defined(RT_USING_SPI2)
    /* Enable SPI GPIO clocks */
    RCC_APB2PeriphClockCmd(    RCC_APB2Periph_GPIOB, ENABLE );//PORTB时钟使能 
    /* Enable SPI clock */
    RCC_APB1PeriphClockCmd(    RCC_APB1Periph_SPI2,  ENABLE );//SPI2时钟使能     
#endif /* RT_USING_SPI2 */
    
#if defined(RT_USING_SPI3)
    /* Enable SPI GPIO clocks */
    RCC_APB2PeriphClockCmd(    RCC_APB2Periph_GPIOB, ENABLE );//PORTB时钟使能 
    /* Enable SPI clock */
    RCC_APB1PeriphClockCmd(    RCC_APB1Periph_SPI3,  ENABLE );//SPI3时钟使能     
#endif /* RT_USING_SPI2 */
}

static void SPI_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

#if defined(RT_USING_SPI1)
    /* 配置SPI1相关引脚 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //PA5/6/7复用推挽输出 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOB

     GPIO_SetBits(GPIOA,GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);  //PA5/6/7上拉
#endif /* RT_USING_SPI1 */

#if defined(RT_USING_SPI2)
    /* 配置SPI2相关引脚 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //PB13/14/15复用推挽输出 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB

     GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);  //PB13/14/15上拉
#endif /* RT_USING_SPI2 */

#if defined(RT_USING_SPI3)
    /* 配置SPI3相关引脚 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //PB3/4/5复用推挽输出 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB

     GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);  //PB3/4/5上拉
#endif /* RT_USING_SPI3 */
}

#if defined(RT_USING_SPI1)
struct stm32_spi_bus stm32_spi1_bus;
struct rt_spi_bus rt_spi1_bus;
static const struct rt_spi_ops rt_spi1_ops = 
{
    configure,
    xfer
};
#endif /* RT_USING_SPI1 */

#if defined(RT_USING_SPI2)
struct stm32_spi_bus stm32_spi2_bus;
struct rt_spi_bus rt_spi2_bus;
static const struct rt_spi_ops rt_spi2_ops = 
{
    configure,
    xfer
};
#endif /* RT_USING_SPI2 */

#if defined(RT_USING_SPI3)
struct stm32_spi_bus stm32_spi3_bus;
struct rt_spi_bus rt_spi3_bus;
static const struct rt_spi_ops rt_spi3_ops = 
{
    configure,
    xfer
};
#endif /* RT_USING_SPI3 */

void rt_hw_stm32_spi_bus_init(void)
{
    SPI_RCC_Configuration();
    SPI_GPIO_Configuration();
    
#if defined(RT_USING_SPI1)    
    stm32_spi1_bus.SPI = SPI1;
    rt_spi_bus_register(&(stm32_spi1_bus.parent),"SPI1",&rt_spi1_ops);
#endif /* RT_USING_SPI1 */
    
#if defined(RT_USING_SPI2)    
    stm32_spi2_bus.SPI = SPI2;
    rt_spi_bus_register(&(stm32_spi2_bus.parent),"SPI2",&rt_spi2_ops);
#endif /* RT_USING_SPI2 */
    
#if defined(RT_USING_SPI3)
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//JTAG-DP 失能 + SW-DP 使能
    stm32_spi3_bus.SPI = SPI3;
    rt_spi_bus_register(&(stm32_spi3_bus.parent),"SPI3",&rt_spi3_ops);
#endif /* RT_USING_SPI3 */
}
