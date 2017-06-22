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
 * \brief 处理服务器发送的数据
 *
 * \internal
 * \par Modification history
 * - 1.00 17-04-23  zhangjinke, first implementation.
 * \endinternal
 */ 

#include "server_process.h"

#include <string.h>
#include "ds1307.h"
#include "wifi_thread.h"
#include "esp8266_cmd.h"
#include "stm32_crc.h"
#include "p_database.h"
#include "att_database.h"

/**
 * \brief 处理服务器发送的数据
 *
 * \param[in] p_data 数据首地址
 * \param[in] lenth  数据长度
 *
 * \return 无
 */
void data_process (uint8_t *p_data, uint16_t lenth)
{
    uint32_t crc = 0;
    uint8_t  cmd = p_data[0];
    user_info_t *p_user_info;
    static user_info_t user_info;
    uint16_t user_id;
    uint16_t att_num_get;
    uint16_t att_num;
    static att_header_t att_header;
    static att_info_t att_info[10];
    static uint32_t user_crc[100];
    static uint8_t buf[1024];
    uint16_t user_crc_start;
    uint16_t user_crc_num;
    uint16_t user_crc_num_get;
    uint16_t user_id_max;

    switch (cmd) {
    
    case CMD_SERVER_USER_ADD: /* 添加用户 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* 对接收到的数据进行crc校验 */
        crc = block_crc_calc(p_data, 1 + sizeof(user_info_t));
        if (*((uint32_t *)&p_data[ 1 + sizeof(user_info_t)]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = 0;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("add user crc failed\r\n");
            break;
        }
        
        p_user_info = (user_info_t *)(p_data + 1);
        if (0 != add_del_get_one_user (p_user_info, ADD_ONE_USER))
        {
            buf[13] = 1;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("add user %d failed\r\n", p_user_info->user_id);
        } else {
            buf[13] = 0;
            buf[14] = CMD_SERVER_ACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("add user %d success\r\n", p_user_info->user_id);
        }

        break;
    
    case CMD_SERVER_USER_DEL: /* 删除用户 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* 对接收到的数据进行crc校验 */
        crc = block_crc_calc(p_data, 3);
        if (*((uint32_t *)&p_data[3]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = 0;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("del user crc failed\r\n");
            break;
        }
        
        user_id = (p_data[1] | (p_data[2] << 8));
        user_info.user_id = user_id;

        if (0 != add_del_get_one_user (&user_info, DEL_ONE_USER)) {
            buf[13] = 1;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("del user %d failed\r\n", user_id);
        } else {
            buf[13] = 0;
            buf[14] = CMD_SERVER_ACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("del user %d success\r\n", user_id);
        }

        break;
    
    case CMD_SERVER_CONNECT: /* 联机 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* 对接收到的数据进行crc校验 */
        crc = block_crc_calc(p_data, 1);
        if (*((uint32_t *)&p_data[1]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = 0;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);

            rt_kprintf("connect crc failed\r\n");
            break;
        }
    
        buf[13] = 0;
        buf[14] = CMD_SERVER_ACK;
        
        /* 联机成功，发送ACK */
        wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
        
        rt_kprintf("connect!\r\n");

        break;
    
    case CMD_SERVER_TIMESYNC: /* 同步时间 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* crc校验 */
        crc = block_crc_calc(p_data, 9);
        if (*((uint32_t *)&p_data[9]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = 0;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("time sync crc failed\r\n");
            break;
        }
    
        TimeValue.year = (p_data[1] | (p_data[2] << 8)) - 2000;
        TimeValue.month = p_data[3];
        TimeValue.date = p_data[4];
        TimeValue.hour = p_data[5];
        TimeValue.minute = p_data[6];
        TimeValue.second = p_data[7];
        TimeValue.week = p_data[8];
        DS1307_Time_Init(&TimeValue);

        buf[13] = 0;
        buf[14] = CMD_SERVER_ACK;
        
        /* 时间同步成功，发送ACK */
        wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
        
        rt_kprintf("time sync 20%02d-%02d-%02d %02d-%02d-%02d weed: %d\r\n", TimeValue.year, TimeValue.month, 
            TimeValue.date, TimeValue.hour, TimeValue.minute, TimeValue.second, TimeValue.week);

        break;
    
    case CMD_SERVER_ATT_GET: /* 获取指定条数考勤信息 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* crc校验 */
        crc = block_crc_calc(p_data, 3);
        if (*((uint32_t *)&p_data[3]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 16, buf);
            
            rt_kprintf("att get crc failed\r\n");
            break;
        }
    
        att_num_get = (p_data[1] | (p_data[2] << 8));
        
        /* 获取考勤记录 */
        if ((record_header_get_set(&att_header, GET_RECORD) != 0) ||
            (att_record_get(att_info, att_num_get, &att_num) != 0)) {
            buf[13] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 16, buf);
            
            rt_kprintf("att_record_get failed\r\n");
            break;
        } else {
            buf[13] = CMD_SERVER_ACK;
            rt_memcpy(buf + 14, (void *)&att_header.total, 4);
            rt_memcpy(buf + 18, &att_num, 2);
            rt_memcpy(buf + 20, att_info, att_num * sizeof(att_info_t));
            
            /* crc校验 */
            crc = block_crc_calc(buf + 6, 14 + att_num * sizeof(att_info_t));
            rt_memcpy(buf + 20 + att_num * sizeof(att_info_t), &crc, 4);
            
            /* 获取考勤记录成功，发送ACK */
            wifi_send(CMD_SEND_MESH_DATA, 6 + 14 + att_num * sizeof(att_info_t) + 4, buf);
            
            rt_kprintf("att get %d success\r\n", att_num);
        }

        break;
        
    case CMD_SERVER_LIST_RELOAD: /* 重新加载卡号-用户号，指纹号-用户号列表 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* 对接收到的数据进行crc校验 */
        crc = block_crc_calc(p_data, 1);
        if (*((uint32_t *)&p_data[1]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = 0;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);

            rt_kprintf("list reload crc failed\r\n");
            break;
        }
        
        /* 初始化卡号数组 */
        if (card_array_init(g_card_id_user_id_list) == -1) {
            buf[13] = 1;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("init card array failed\r\n");
        } else {
            buf[13] = 0;
            buf[14] = CMD_SERVER_ACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            rt_kprintf("init card array success\r\n");
        }
        
        rt_kprintf("list reload success!\r\n");

        break;

    case CMD_SERVER_ATT_DEL: /* 删除指定条数考勤记录 */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* 对接收到的数据进行crc校验 */
        crc = block_crc_calc(p_data, 3);
        if (*((uint32_t *)&p_data[3]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = 0;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);

            rt_kprintf("att del crc failed\r\n");
            break;
        }
        
        att_num = (p_data[1] | (p_data[2] << 8));
        
        /* 删除指定条数考勤记录 */
        if (att_record_del(att_num) != 0) {
            buf[13] = 1;
            buf[14] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            
            rt_kprintf("att del %d failed\r\n", att_num);
        } else {
            buf[13] = 0;
            buf[14] = CMD_SERVER_ACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 9, buf);
            rt_kprintf("att del %d success\r\n", att_num);
        }
        
        break;
    
    case CMD_SERVER_USER_CRC_GET: /* 获取指定条数用户CRC */
        
        /* 添加目标地址 */
        memset(buf, 0, 6);
    
        /* 添加本机地址 */
        memcpy(buf + 6, station_addr, 6);
    
        /* 添加cmd */
        buf[12] = cmd;
    
        /* crc校验 */
        crc = block_crc_calc(p_data, 5);
        if (*((uint32_t *)&p_data[5]) != crc) {
            
            /* crc校验失败，发送NACK */
            buf[13] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 16, buf);
            
            rt_kprintf("user_crc_get crc failed\r\n");
            break;
        }
    
        user_crc_start = (p_data[1] | (p_data[2] << 8));
        user_crc_num = (p_data[3] | (p_data[4] << 8));
        
        /* 获取用户crc */
        if ((user_num_get(RT_NULL, &user_id_max) != 0) ||
            (user_crc_get(user_crc, &user_crc_num_get, user_crc_start, user_crc_num) != 0)) {
            buf[13] = CMD_SERVER_NACK;
            wifi_send(CMD_SEND_MESH_DATA, 6 + 16, buf);
            
            rt_kprintf("user_crc_get failed user_crc_start: %d user_crc_num: %d\r\n", user_crc_start, user_crc_num);
            break;
        } else {
            buf[13] = CMD_SERVER_ACK;
            rt_memcpy(buf + 14, (void *)&user_id_max, 2);
            rt_memcpy(buf + 16, &user_crc_num_get, 2);
            rt_memcpy(buf + 18, user_crc, user_crc_num_get * 4);
            
            /* crc校验 */
            crc = block_crc_calc(buf + 6, 12 + user_crc_num_get * 4);
            rt_memcpy(buf + 18 + user_crc_num_get * 4, &crc, 4);
            
            /* 获取用户crc成功，发送ACK */
            wifi_send(CMD_SEND_MESH_DATA, 6 + 12 + user_crc_num_get * 4 + 4, buf);
            
            rt_kprintf("user_crc_get %d success\r\n", user_crc_num_get);
        }

        break;

    default:
        ;
    }
}

/* end of file */
