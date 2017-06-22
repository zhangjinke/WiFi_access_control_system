/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: rfid_thread.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 08 日
**
** 描        述: rfid卡扫描线程

** 日志:
2016.09.08  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include "rfid_thread.h"
#include "finsh.h"
#include "global.h"
#include "rc522.h"

#ifdef  RC522
rt_uint8_t rc522_stack[ 1024 ];
struct rt_thread rc522_thread;
#endif  /* RC522 */

/* 读取出的卡号 */
uint32_t g_read_card_id = 0;

/*******************************************************************************
* 函数名     : rc522_thread_entry
* 描述       : rc522线程,检测到刷卡之后发出刷卡事件
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
void rc522_thread_entry(void* parameter)
{
    uint32_t old_in_card_id, old_out_card_id;    //保存上次读取的卡号
    uint32_t in_timer = 0, out_timer = 0;    //超时时间计数器，防止同一张卡读取多次
    uint8_t in_timer_en = 0, out_timer_en = 0;    //超时时间计数器使能位 1: 使能
    uint8_t is_in_over_time = 0, is_out_over_time = 0;    //是否超过阈值时间 0: 已经超过
    
    /* 初始化RC522设备 */
    InitRC522();
    while(1)
    {
        /* 切换为门内RFID设备并读卡 */
        rt_spi_rc522_device = rt_spi_rc522_in_device;
        if(ReadID(&g_read_card_id) == 0)
        {
            /* 比较当前读取的卡号和上次读取的卡号，不同则马上发送刷卡事件 */
            if(old_in_card_id != g_read_card_id)
            {
                is_in_over_time = 0;
                old_in_card_id = g_read_card_id;
            }
            /* 判断是否经过阈值时间，同一张卡只有经过阈值时间才能再次刷入 */
            if(is_in_over_time == 0)
            {
                is_in_over_time = 1;
                in_timer_en = 1;
                rt_kprintf("in card event\r\n");
                /* 发送读卡事件 */
                rt_event_send(&g_user_check_event, USER_CHECK_CARD_IN);    
            }
        }
        /* 切换为门外RFID设备并读卡 */
        rt_spi_rc522_device = rt_spi_rc522_out_device;
        if(ReadID(&g_read_card_id) == 0)
        {
            /* 比较当前读取的卡号和上次读取的卡号，不同则马上发送刷卡事件 */
            if(old_out_card_id != g_read_card_id)
            {
                is_out_over_time = 0;
                old_out_card_id = g_read_card_id;
            }
            /* 判断是否经过阈值时间，同一张卡只有经过阈值时间才能再次刷入 */
            if(is_out_over_time == 0)
            {
                is_out_over_time = 1;
                out_timer_en = 1;
                rt_kprintf("out card event\r\n");
                /* 发送读卡事件 */
                rt_event_send(&g_user_check_event, USER_CHECK_CARD_OUT);    
            }
        }
        /* 超时时间计数器 */
        if(in_timer_en)
        {
            in_timer++;
            /* 达到阈值时间，设置相应标志位 */
            if(in_timer >= 50) 
            {
                is_in_over_time = 0;
                in_timer = 0;
                in_timer_en = 0;
            }
        }
        /* 超时时间计数器 */        
        if(out_timer_en)
        {
            out_timer++;
            /* 达到阈值时间，设置相应标志位 */
            if(out_timer >= 50) 
            {
                is_out_over_time = 0;
                out_timer = 0;
                out_timer_en = 0;
            }
        }
        RT_THREAD_DELAY_MS(5);
    }    
}

/*******************************************************************************
* 函数名     : send_get_card_event
* 描述       : 发出刷卡事件
* 输入         : - state: 状态 0: 门外 1: 门内 - card_id_send: 卡号
* 输出         : None
* 返回值    : None
*******************************************************************************/
void send_get_card_event(uint8_t state, uint32_t card_id_send)
{
    switch(state)
    {
        case 0:
        {
            /* 发送读卡事件 */
            g_read_card_id = card_id_send;
            rt_event_send(&g_user_check_event, USER_CHECK_CARD_OUT);    
        }    break;
        case 1:
        {
            /* 发送读卡事件 */
            g_read_card_id = card_id_send;
            rt_event_send(&g_user_check_event, USER_CHECK_CARD_IN);    
        }    break;
        default: rt_kprintf("状态错误 state is %d , 0: 门外 1: 门内",state);
    }
}
FINSH_FUNCTION_EXPORT_ALIAS(send_get_card_event, send_card, send get card event)
