/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: user_check_thread.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 08 日
**
** 描        述: 接收到刷卡或刷指纹事件之后进行权限检测，通过则开门

** 日志:
2016.09.08  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include <time.h>
#include "user_check_thread.h"
#include "wifi_thread.h"
#include "rfid_thread.h"
#include "p_database.h"
#include "device_config.h"
#include <dfs.h>
#include <dfs_posix.h>
#include "beep_door.h"
#include "att_database.h"
#include "rtc_thread.h"
#include "global.h"
#include "stm32_crc.h"

/** \brief 刷卡或刷指纹事件控制块 */
struct rt_event g_user_check_event;

rt_uint8_t user_check_stack[ 1024 ];    //线程栈
struct rt_thread user_check_thread;     //线程控制块


/*******************************************************************************
* 函数名     : add_one_att_record
* 描述       : 添加一条考勤记录
* 输入         : - user_info_temp: 人员信息结构体 -recved_event: 事件(刷卡、耍指纹)
* 输出         : None
* 返回值    : None
*******************************************************************************/
int8_t add_one_att_record(user_info_t *user_info_temp, uint32_t recved_event)
{
    att_info_t att_info_temp;
    att_header_t att_header_temp;

    /* 设置考勤信息 */
    rt_memset(&att_info_temp, 0 ,sizeof(att_info_t));
    att_info_temp.user_id = user_info_temp->user_id;
    rt_memcpy(&att_info_temp.student_id, &user_info_temp->student_id, MEMBER_SIZE(att_info, student_id));
    rt_memcpy(&att_info_temp.name, &user_info_temp->name, MEMBER_SIZE(att_info, name));
    att_info_temp.device_addr = g_device_config.device_addr;
    rt_memcpy(att_info_temp.mac_addr, station_addr, sizeof(station_addr));
    if ((recved_event & USER_CHECK_CARD_IN) || (recved_event & USER_CHECK_FINGER_IN))
    {
        att_info_temp.state = 0; /* 设置状态为出门 */
    }
    else
    {
        att_info_temp.state = 1; /* 设置状态为进门 */
    }
    att_info_temp.year    = TimeValue.year + 2000;
    att_info_temp.month   = TimeValue.month;
    att_info_temp.day     = TimeValue.date;
    att_info_temp.hour    = TimeValue.hour;
    att_info_temp.minutes = TimeValue.minute;
    att_info_temp.second  = TimeValue.second;
    
    /* 保存考勤信息 */
    if (att_record_add(&att_info_temp) != 0){
        rt_kprintf("att_record_get_set failed\r\n");
    }
    
    return 0;
}

/*******************************************************************************
* 函数名     : user_check_thread_entry
* 描述       : 接收到刷卡或刷指纹事件之后进行权限检测，通过则开锁
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
void user_check_thread_entry(void* parameter)
{
    rt_err_t status = RT_EOK;
    rt_uint32_t recved_event = 0;
//    uint16_t user_num = 0, max_user_num = 0;
    int32_t search_result = 0;
    uint32_t i = 0;
    uint32_t crc = 0;
    char limit_time[20], now_time[20];
    card_id_user_id_link_t card_id_user_id_key;
    card_id_user_id_link_t *p_card_id_user_id_recv;
    static user_info_t user_info_struct_get; /* 定义一个人员信息结构体 */
    
//    if(get_set_user_num(&user_num, GET_USER) == -1)
//    {
//        rt_kprintf("get user num failed\r\n");
//    }
//    
//    if(get_set_user_num_max(&max_user_num, GET_USER) == -1)
//    {
//        rt_kprintf("get user num max failed\r\n");
//    }
    while(1)
    {
        /* 清空获取到的人员信息 */
        rt_memset(&user_info_struct_get, 0, sizeof(user_info_t));
        
        /* 等待刷卡或刷指纹事件 */
        status = rt_event_recv(&g_user_check_event,                       //事件对象的句柄
                               USER_CHECK_CARD_IN |                       //接收线程感兴趣的事件
                               USER_CHECK_CARD_OUT |
                               USER_CHECK_FINGER_IN |
                               USER_CHECK_FINGER_OUT,            
                               RT_EVENT_FLAG_CLEAR|RT_EVENT_FLAG_OR,    //逻辑或、清除事件
                               RT_WAITING_FOREVER,                      //永不超时
                               &recved_event                            //指向收到的事件
                              );
        
        /* 根据接收到的事件执行不同的处理 */
        if(status == RT_EOK) {
            
            if ((recved_event & USER_CHECK_CARD_IN) || (recved_event & USER_CHECK_CARD_OUT)) { /* 刷卡事件 */
                
                card_id_user_id_key.card_id = g_read_card_id;
                
                /* 搜索卡号对应的用户号 */
                p_card_id_user_id_recv = bsearch(&card_id_user_id_key,
                                                  g_card_id_user_id_list, 
                                                  MAX_USER_NUM, 
                                                  sizeof(card_id_user_id_link_t), 
                                                  card_compar);

                if (p_card_id_user_id_recv == RT_NULL) {
                    if(recved_event & USER_CHECK_CARD_IN) { 
                        rt_kprintf("in card id %X is not exist\r\n", g_read_card_id); 
                    } else { 
                        rt_kprintf("out card id %X is not exist\r\n", g_read_card_id); 
                    }
                    continue;
                } else {    /* 搜索到对应的用户号 */
                    user_info_struct_get.user_id = p_card_id_user_id_recv->user_id;
                    
                    /* 获取详细用户信息 */
                    if (add_del_get_one_user(&user_info_struct_get, GET_ONE_USER) != 0) {
                        rt_kprintf("获取详细用户信息失败 user id:%d\r\n", g_card_id_user_id_list[search_result].user_id);
                    }
                    
                    /* crc校验 */
                    crc = block_crc_calc((uint8_t *)&user_info_struct_get, sizeof(user_info_t) - 4 - 16);
                    if (crc != user_info_struct_get.crc) {
                        rt_kprintf("用户信息crc校验错误\r\n");
                        continue;
                    }
                    
                    /* 判断card id是否正确 */
                    if (user_info_struct_get.card_id != p_card_id_user_id_recv->card_id) { 
                        continue; 
                    }
                }
            } else if ((recved_event & USER_CHECK_FINGER_IN) || (recved_event & USER_CHECK_FINGER_OUT)) { /* 指纹事件 */
            
            } else {
                rt_kprintf("unknown user check event 0x%08X\r\n", recved_event);
                continue;
            }
            
            /* 判断是否是超级管理员 */
            if ((user_info_struct_get.authority[15]&(1 << 7)) == (1 << 7) ) { //最高位为1则为超级管理员，直接开门
                rt_kprintf("\r\n超级管理员，欢迎光临 uID:%d\r\n", user_info_struct_get.user_id);
                
                /* 开锁 */
                open_door();
                
                /* 记录考勤信息 */
                add_one_att_record(&user_info_struct_get, recved_event);
                continue;
            }
            
            /* 判断用户是否激活 */
            if (user_info_struct_get.effective != 1)
            {
                rt_kprintf("\r\n未激活！\r\n");
                continue;
            }
            
            /* 判断是否是有限时长用户 */
            if (user_info_struct_get.is_time_limit == 1)
            {
                sprintf(now_time, 
                        "%04d-%02d-%02d %02d:%02d:%02d", 
                        TimeValue.year + 2000,
                        TimeValue.month,
                        TimeValue.date,
                        TimeValue.hour,
                        TimeValue.minute,
                        TimeValue.second);
                sprintf(limit_time, 
                        "%04d-%02d-%02d %02d:%02d:%02d", 
                        user_info_struct_get.year,
                        user_info_struct_get.month,
                        user_info_struct_get.day,
                        user_info_struct_get.hour,
                        user_info_struct_get.minutes,
                        user_info_struct_get.second);
                
                /* 如果过期，提示 */
                if (strcmp(now_time, limit_time) >= 0) {
                    rt_kprintf("\r\n超期！\r\n");
                    continue;                    
                }
            }
            
            /* 确认权限 */
            if (user_info_struct_get.authority[15] & (1 << 6)) {
            } else if ((user_info_struct_get.authority[g_device_config.device_addr/8]&(1 << (g_device_config.device_addr%8))) != (1 << (g_device_config.device_addr%8)))
            {
                rt_kprintf("\r\n权限不足！\r\n");
                continue;
            }
            
            /* 检查进出状态 */
            switch(recved_event) {
                
                /* 门内刷卡 */
                case USER_CHECK_CARD_IN:
                    if ((user_info_struct_get.state[g_device_config.device_addr / 8] & (1 << (g_device_config.device_addr % 8))) != (1 << (g_device_config.device_addr%8)) )
                    {    //0:门外 1:门内    如果在门外
                        rt_kprintf("\r\n进门未刷卡！\r\n");
                        continue;
                    }
                    else
                    {
                        /* 将状态设置为门外 */
                        user_info_struct_get.state[g_device_config.device_addr/8] &=~ (1 << (g_device_config.device_addr%8));
                        get_set_state(user_info_struct_get.state,user_info_struct_get.user_id,SET_USER);
                        /* 开锁 */
                        open_door();
                    }
                    break;
                    
                /* 门外刷卡 */
                case USER_CHECK_CARD_OUT:
                    if ((user_info_struct_get.state[g_device_config.device_addr/8]&(1 << (g_device_config.device_addr%8))) == (1 << (g_device_config.device_addr%8)) )
                    {    //0:门外 1:门内    如果在门内
                        rt_kprintf("\r\n出门未刷卡！\r\n");
                        continue;
                    }
                    else
                    {
                        /* 将状态设置为门内 */
                        user_info_struct_get.state[g_device_config.device_addr/8] |= (1 << (g_device_config.device_addr%8));
                        get_set_state(user_info_struct_get.state,user_info_struct_get.user_id,SET_USER);
                        /* 开锁 */
                        open_door();
                    }
                    break;
                    
                /* 门内刷指纹 */
                case USER_CHECK_FINGER_IN:
                    break;
                
                /* 门外刷指纹 */
                case USER_CHECK_FINGER_OUT:
                    break;
                
                default : 
                    ; /* VOID */
            }
            
            /* 记录考勤信息 */
            add_one_att_record(&user_info_struct_get, recved_event);
            
            if (user_info_struct_get.authority[15] & (1 << 6)) {
                rt_kprintf("\r\n管理员，欢迎光临 uID:%d", user_info_struct_get.user_id);
            }
            
            /* 打印相关信息 */
            rt_kprintf("\r\nuser id is %d\r\n", user_info_struct_get.user_id);
            rt_kprintf("card id is %X\r\n", user_info_struct_get.card_id);
            rt_kprintf("effective is %d\r\n", user_info_struct_get.effective);
            rt_kprintf("name is %s\r\n", user_info_struct_get.name);
            rt_kprintf("student id is %s\r\n", user_info_struct_get.student_id);
            
            /* 打印权限 */
            rt_kprintf("authority is \r\n"); 
            for (i = 0; i<sizeof(user_info_struct_get.authority); i++)
            {
                rt_kprintf("%02X ", user_info_struct_get.authority[i]);
            }
            rt_kprintf("\r\n"); 
        }
    }    
}

