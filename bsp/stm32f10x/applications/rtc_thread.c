/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: rtc_thread.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 11 日
**
** 描        述: 实时时钟相关线程

** 日志:
2016.09.11  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include <time.h>
#include "rtc_thread.h"
#include "soft_iic.h"
#include "ds1307.h"
#include "finsh.h"
#include "global.h"

rt_uint8_t rtc_stack[ 1024 ];    //线程栈
struct rt_thread rtc_thread;     //线程控制块

/*******************************************************************************
* 函数名     : rtc_thread_entry
* 描述       : 通信相关线程
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
void rtc_thread_entry(void* parameter)
{
    IIC_GPIO_Init();
    
    if(DS1307_Check() != 0)
    {
        rt_kprintf("DS1307 check failed, init it!\r\n");
        TimeValue.year = 0;
        TimeValue.month = 1;
        TimeValue.date = 1;
        TimeValue.hour = 0;
        TimeValue.minute = 0;
        TimeValue.second = 0;
        TimeValue.week = 6;
        DS1307_Time_Init(&TimeValue);
    }
    if(DS1307_Check() != 0)
    {
        rt_kprintf("init ds1307 failed!\r\n");
    }
    
    while(1)
    {
        DS1307_ReadWrite_Time(1);
//        rt_kprintf("20%02d-%02d-%02d %02d-%02d-%02d weed: %d\r\n", TimeValue.year, TimeValue.month, 
//                    TimeValue.date, TimeValue.hour, TimeValue.minute, TimeValue.second, TimeValue.week);
        RT_THREAD_DELAY_MS(1000);
    }    
}

/**
 * \brief 设置系统时间
 *
 * \param[in] year   年
 * \param[in] month  月
 * \param[in] date   日
 * \param[in] hour   时
 * \param[in] minute 分
 * \param[in] second 秒
 * \param[in] week   星期
 *                   
 * \return 无
 */
void set_time(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second, uint8_t week)
{
    TimeValue.year = year;
    TimeValue.month = month;
    TimeValue.date = date;
    TimeValue.hour = hour;
    TimeValue.minute = minute;
    TimeValue.second = second;
    TimeValue.week = week;
    DS1307_Time_Init(&TimeValue);
}
FINSH_FUNCTION_EXPORT(set_time, set time and date)

/*******************************************************************************
* 函数名     : get_time
* 描述       : 获取系统时间
* 输入         : - 年月日时分秒星期
* 输出         : None
* 返回值    : None
*******************************************************************************/
void get_time(void)
{
    rt_kprintf("20%02d-%02d-%02d %02d-%02d-%02d weed: %d\r\n", TimeValue.year, TimeValue.month, 
                TimeValue.date, TimeValue.hour, TimeValue.minute, TimeValue.second, TimeValue.week);
}
FINSH_FUNCTION_EXPORT(get_time, get system time and date)

