/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: stm32_crc.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 11 月 04 日
**
** 描        述: 计算CRC

** 日志:
2016.11.04  创建本文件
*********************************************************************************************************/

#include "stm32_crc.h"
#include "stm32f10x_crc.h"

/**
 * \brief 计算CRC
 *
 * \param[in] p_data : 需要被计算的数据
 * \param[in] lenth  : 需要计算的字节数
 *
 * \return 计算结果
 */
uint32_t block_crc_calc (uint8_t p_data[], uint32_t lenth)
{
    uint32_t index = 0;
    
    CRC_ResetDR(); /* 复位CRC */
    for(index = 0; index < lenth; index++)
    {
        CRC->DR = p_data[index];
    }
    return (CRC->DR);
}
