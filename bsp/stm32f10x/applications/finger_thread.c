/*******************************************************************************
*                        WiFi Access Control System
*                       ----------------------------
*                                  EE Bang
*
* Contact information:
* web site:    http://www.cqutlab.cn/
* e-mail:      799548861@qq.com
*******************************************************************************/

/**
 * \file
 * \brief 指纹处理线程
 *
 * \internal
 * \par Modification history
 * - 1.00 17-06-09  zhangjinke, first implementation.
 * \endinternal
 */ 

#include "rfid_thread.h"

#include <rtthread.h>
#include <finsh.h>
#include "global.h"

#ifdef  FINGER_PRINT
rt_uint8_t finger_print_stack[1024];
struct rt_thread finger_print_thread;
#endif  /* FINGER_PRINT */

/** \brief 指纹模块识别到的用户号 */
uint32_t g_finger_user_id = 0;

/**
 * \brief 指纹处理线程,检测到指纹之后发出指纹事件
 *
 * \param[in] p_parameter 线程入口参数
 *
 * \return 无
 */
void finger_print_thread_entry (void *p_parameter)
{
    
    while(1) {
        RT_THREAD_DELAY_MS(5);
    }    
}

/**
 * \brief 发出指纹事件
 *
 * \param[in] state        状态 0: 门外 1: 门内
 * \param[in] card_id_send 卡号
 *
 * \return 无
 */
void finger_print_event_send (uint8_t state, uint32_t user_id)
{
    g_finger_user_id = user_id;
    
    switch(state) {
        
        case 0:
            
            /* 发送读卡事件 */
            rt_event_send(&g_user_check_event, USER_CHECK_FINGER_OUT);    
            break;
        
        case 1:
            
            /* 发送读卡事件 */
            rt_event_send(&g_user_check_event, USER_CHECK_FINGER_IN);    
            break;
        
        default: 
            rt_kprintf("状态错误 state is %d , 0: 门外 1: 门内",state);
    }
}
FINSH_FUNCTION_EXPORT_ALIAS(finger_print_event_send, send_finger, send finger event)
