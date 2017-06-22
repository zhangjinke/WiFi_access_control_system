/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: spi_bus.h
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 06 日
**
** 描        述: stm32 spi相关函数

** 日志:
2016.09.06  创建本文件
*********************************************************************************************************/
#ifndef _SPI_BUS_H_
#define _SPI_BUS_H_
#include "stm32f10x.h"
#include <drivers/spi.h>

struct stm32_spi_bus
{
    struct rt_spi_bus parent;
    SPI_TypeDef * SPI;
};

struct stm32_spi_cs
{
    GPIO_TypeDef * GPIOx;
    uint16_t GPIO_Pin;
};

extern void rt_hw_stm32_spi_bus_init(void);
     
#endif

