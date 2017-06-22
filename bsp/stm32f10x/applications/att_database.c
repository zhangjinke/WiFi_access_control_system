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
 * \brief 考勤信息数据库
 *
 * \internal
 * \par Modification history
 * - 1.00 16-11-03  zhangjinke, first implementation.
 * \endinternal
 */ 

#include "att_database.h"

#include <rtthread.h>
#include <dfs.h>
#include <dfs_posix.h>
#include <finsh.h>
#include "global.h"
#include "stm32_crc.h"


/** \brief 设置调试信息打印开关 */
#ifdef PRINT_ATT_DEBUG_INFO
#define ATT_PRINTF(fmt,args...) rt_kprintf("ATT_PRINTF: "); rt_kprintf(fmt, ##args)
#else
#define ATT_PRINTF(fmt,args...)
#endif

/** \brief 考勤信息数据库路径 */
const char *gp_attendance_database_path = "/save_io.bin";

/** \brief 考勤信息数据库header结构体 */
att_header_t g_att_header;

/**
 * \brief 初始化考勤数据库
 *
 * \param[in] p_att_header 考勤信息头
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_database_init (att_header_t *p_att_header)
{
    int32_t  fd  = -1;
    uint32_t crc = 0;
    
    if (RT_NULL == p_att_header) {
        return -1;
    }
    
    /* 打开人员信息数据库 */
    fd = open(gp_attendance_database_path, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        ATT_PRINTF("open %s failed\r\n", gp_attendance_database_path);
        close(fd);
        return -1;
    } else {
        
        /* 移动文件指针到指定位置 */
        if (lseek(fd, 0, SEEK_SET) == -1) {
            ATT_PRINTF("lseek %s failed\r\n", gp_attendance_database_path);
            close(fd);
            return -1;
        } else {
            
            /* 获取考勤记录header信息 */
            read(fd, p_att_header, sizeof(att_header_t));
            
            /* 计算CRC */
            crc = block_crc_calc((uint8_t *)p_att_header, sizeof(att_header_t) - 4);
            if (crc != p_att_header->crc) {
                ATT_PRINTF("att header crc validators fail, init att database\r\n");
                
                /* 初始化考勤记录数据库 */
                p_att_header->total = 0;
                
                /* 计算CRC */
                p_att_header->crc = block_crc_calc((uint8_t *)p_att_header, sizeof(att_header_t) - 4);
                
                /* 移动文件指针到指定位置 */
                if (lseek(fd, 0, SEEK_SET) == -1) {
                    ATT_PRINTF("lseek %s failed\r\n", gp_attendance_database_path);
                    close(fd);
                    return -1;
                }
                
                /* 写入考勤记录总条数 */
                if (write(fd, p_att_header, sizeof(att_header_t)) != sizeof(att_header_t)) {
                    ATT_PRINTF("write %s failed\r\n", gp_attendance_database_path);
                    close(fd);
                    return -1;
                } else {
                    close(fd);
                    ATT_PRINTF("init att database success\r\n");            
                }        
            } else {
                close(fd);
            }
        }
    }
    
    return 0;
}

/**
 * \brief 获取/设置考勤数据库header
 *
 * \param[in,out] p_att_header 考勤信息头
 * \param[in]     cmd          0: 获取 1: 设置
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t record_header_get_set (att_header_t *p_att_header, uint8_t cmd)
{
    int fd;
    
    /* 检查参数合法性 */
    if (RT_NULL == p_att_header) {
        ATT_PRINTF("p_att_header addr is 0\r\n");
        return -1;
    }
    
    /* 打开人员信息数据库 */
    fd = open(gp_attendance_database_path, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        ATT_PRINTF("open %s failed\r\n", gp_attendance_database_path);
        close(fd);
        return -1;
    } else {
        
        /* 移动文件指针到指定位置 */
        if (lseek(fd, 0, SEEK_SET) == -1) {
            ATT_PRINTF("lseek %s failed\r\n", gp_attendance_database_path);
            close(fd);
            return -1;
        } else {
            if (cmd == 0) 
                {
                /* 获取考勤记录总条数 */
                if (read(fd, p_att_header, sizeof(att_header_t)) != sizeof(att_header_t)) {
                    ATT_PRINTF("read %s failed\r\n", gp_attendance_database_path);
                    close(fd);
                    return -1;
                } else {
                    close(fd);
                    if (p_att_header->crc == block_crc_calc((uint8_t *)p_att_header, sizeof(att_header_t) - 4)) {
                        ATT_PRINTF("get record header total is %d success\r\n",p_att_header->total);
                    } else {
                        ATT_PRINTF("crc validators fail total is %d\r\n",p_att_header->total);
                        return -1;
                    }
                }        
            } else if (cmd == 1) {
                
                /* 计算CRC */
                p_att_header->crc = block_crc_calc((uint8_t *)p_att_header, sizeof(att_header_t) - 4);
                    
                /* 写入考勤记录总条数 */
                if (write(fd, p_att_header, sizeof(att_header_t)) != sizeof(att_header_t)) {
                    ATT_PRINTF("write %s failed\r\n", gp_attendance_database_path);
                    close(fd);
                    return -1;
                } else {
                    close(fd);
                    ATT_PRINTF("set record header %d success\r\n",p_att_header->total);            
                }        
            } else {
                close(fd);
                ATT_PRINTF("get set record count cmd unknown\r\n");
                return -1;
            }
        }
    }
    
    return 0;
}
/**
 * \brief 获取/设置考勤数据库header
 *
 * \param[in,out] p_att_header 考勤信息头
 * \param[in]     cmd          0: 获取 1: 设置
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t fd_record_header_get_set (int fd, att_header_t *p_att_header, uint8_t cmd)
{    
    /* 检查参数合法性 */
    if (RT_NULL == p_att_header) {
        ATT_PRINTF("p_att_header addr is 0\r\n");
        goto failed;
    }

    /* 移动文件指针到指定位置 */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        ATT_PRINTF("lseek %s failed\r\n", gp_attendance_database_path);
        goto failed;
    }
    
    if (cmd == 0) {
        
        /* 获取考勤记录header */
        if (read(fd, p_att_header, sizeof(att_header_t)) != sizeof(att_header_t)) {
            ATT_PRINTF("read %s failed\r\n", gp_attendance_database_path);
            goto failed;
        } else {
            if (p_att_header->crc == block_crc_calc((uint8_t *)p_att_header, sizeof(att_header_t) - 4)) {
                ATT_PRINTF("get record header total is %d success\r\n",p_att_header->total);
            } else {
                ATT_PRINTF("crc validators fail total is %d\r\n",p_att_header->total);
                goto failed;
            }
        }        
    } else if (cmd == 1) {
        
        /* 计算CRC */
        p_att_header->crc = block_crc_calc((uint8_t *)p_att_header, sizeof(att_header_t) - 4);
            
        /* 写入考勤记录header */
        if (write(fd, p_att_header, sizeof(att_header_t)) != sizeof(att_header_t)) {
            ATT_PRINTF("write %s failed\r\n", gp_attendance_database_path);
            goto failed;
        } else {
            ATT_PRINTF("set record header %d success\r\n",p_att_header->total);            
        }        
    } else {
        ATT_PRINTF("get set record count cmd unknown\r\n");
        goto failed;
    }
    
    return 0;
    
failed:
    return -1;
}

/**
 * \brief 添加一条考勤记录
 *
 * \param[in] p_one_att_info 考勤记录结构体
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_record_add (att_info_t *p_one_att_info)
{
    int fd;
    att_header_t att_header;
    
    /* 检查参数合法性 */
    if (RT_NULL == p_one_att_info) {
        ATT_PRINTF("p_one_att_info addr is 0\r\n");
        return -1;
    }

    /* 打开人员信息数据库 */
    fd = open(gp_attendance_database_path, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        ATT_PRINTF("open %s failed\r\n", gp_attendance_database_path);
        return -1;
    }
    
    /* 获取考勤记录header */
    if (fd_record_header_get_set(fd, &att_header, GET_RECORD) != 0) {
        ATT_PRINTF("fd_record_header_get failed\r\n");            
        goto failed;
    } 
        
    /* 移动文件指针到指定位置 */
    if (lseek(fd, sizeof(att_info_t) * att_header.total + sizeof(att_header_t), SEEK_SET) == -1) {
        ATT_PRINTF("lseek %s failed\r\n", gp_attendance_database_path);
        goto failed;
    }

    /* 计算CRC */
    p_one_att_info->crc = block_crc_calc((uint8_t *)p_one_att_info, sizeof(att_info_t) - 4);

    /* 写入考勤记录 */
    if (write(fd, p_one_att_info, sizeof(att_info_t)) != sizeof(att_info_t)) {
        ATT_PRINTF("write %s failed\r\n", gp_attendance_database_path);
        goto failed;
    }
        
    att_header.total++;
    
    /* 保存考勤记录header */
    if (fd_record_header_get_set(fd, &att_header, SET_RECORD) != 0) {
        ATT_PRINTF("fd_record_header_set failed\r\n");            
        goto failed;
    }
    ATT_PRINTF("set att record %d success\r\n", p_one_att_info->user_id);            
    
    close(fd);
    return 0;
    
failed:
    close(fd);
    return -1;
}

/**
 * \brief 删除指定条数考勤记录
 *
 * \param[in] num 待删除的条数
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_record_del (uint16_t num)
{
    int fd;
    att_header_t att_header;
    
    /* 打开人员信息数据库 */
    fd = open(gp_attendance_database_path, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        ATT_PRINTF("open %s failed\r\n", gp_attendance_database_path);
        return -1;
    }
    
    /* 获取考勤记录header */
    if (fd_record_header_get_set(fd, &att_header, GET_RECORD) != 0) {
        ATT_PRINTF("fd_record_header_get failed\r\n");            
        goto failed;
    } 

    if (att_header.total > num) {
        att_header.total -= num;
    } else {
        att_header.total = 0;
    }
    
    /* 保存考勤记录header */
    if (fd_record_header_get_set(fd, &att_header, SET_RECORD) != 0) {
        ATT_PRINTF("fd_record_header_set failed\r\n");            
        goto failed;
    }
    
    close(fd);
    return 0;
    
failed:
    close(fd);
    return -1;
}

/**
 * \brief 获取考勤记录(从最后一条记录开始获取)
 *
 * \param[out] att_info    考勤记录
 * \param[in]  lenth       条数
 * \param[out] p_att_count 获取到的考勤记录条数
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_record_get (att_info_t att_info[], uint8_t lenth, uint16_t *p_att_count)
{
    int fd;
    struct stat file_info;
    att_header_t att_header;
    uint32_t addr = 0;
    uint32_t i = 0;
    uint32_t att_count = 0;
    
    /* 检查参数合法性 */
    if (RT_NULL == att_info) {
        ATT_PRINTF("att_info addr is 0\r\n");
        return -1;
    }

    /* 获取文件信息 */
    if (stat(gp_attendance_database_path, &file_info) <0) {
        ATT_PRINTF("stat %s failed\r\n", gp_attendance_database_path);
        return -1;
    }

    /* 打开考勤信息数据库 */
    fd = open(gp_attendance_database_path, O_RDWR | O_CREAT, 0);
    if (fd < 0) {
        ATT_PRINTF("open %s failed\r\n", gp_attendance_database_path);
        return -1;
    }
    
    /* 获取考勤记录header */
    if (fd_record_header_get_set(fd, &att_header, GET_RECORD) != 0) {
        ATT_PRINTF("fd_record_header_get failed\r\n");            
        goto failed;
    } 
    rt_kprintf("total is %d\r\n", att_header.total);

    if (lenth >= att_header.total) {
        lenth = att_header.total;
    }
    
    i = lenth;
    while (i) {
        addr = (sizeof(att_info_t) * ((i--) - 1)) + sizeof(att_header_t);
        
        /* 判断是否读取完整个文件 */
        if (addr < sizeof(att_header_t)) {
            break;
        }

        /* 移动文件指针到指定位置 */
        if (lseek(fd, addr, SEEK_SET) == -1) {
            ATT_PRINTF("lseek %s failed\r\n", gp_attendance_database_path);
            break;
        }
        
        /* 读取考勤记录 */
        if (read(fd, &att_info[att_count], sizeof(att_info_t)) != sizeof(att_info_t)) {
            ATT_PRINTF("read %s failed\r\n", gp_attendance_database_path);
            break;
        }
        att_count++;
    }
    
    if (RT_NULL != p_att_count) {
        *p_att_count = att_count;
    }
    
    close(fd);
    return 0;
   
failed:
    close(fd);
    return -1;
}
    
/**
 * \brief 清空考勤记录
 *
 * \param[in] p_att_header 考勤信息头
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t att_database_wipe (att_header_t *p_att_header)
{
    rt_memset(p_att_header, 0, sizeof(att_header_t));
    if (unlink(gp_attendance_database_path) == 0) {
        record_header_get_set(p_att_header, SET_RECORD);
        att_database_init(p_att_header);
    } else {
        ATT_PRINTF("att_database_wipe failed\r\n");
        return -1;
    }
    return 0;
}
void wipe_att_db (uint32_t count)
{
    att_database_wipe(&g_att_header);
}
FINSH_FUNCTION_EXPORT_ALIAS(wipe_att_db, wipe_att_db, wipe_att_db)

/**
 * \brief 通过串口打印指定条数考勤记录
 *
 * \param[in] p_att_header 考勤信息头
 * \param[in] count        数量
 *
 * \return 无
 */
void record_print (void)
{
    static att_info_t att_info[100];
    uint16_t get_count;
    uint32_t i;
    
    rt_memset(att_info, 0, sizeof(att_info));
    if (att_record_get(att_info, 100, &get_count)) {
        rt_kprintf("att_record_get failed\r\n");
        return;
    }

    for (i = 0; i < get_count; i++) {
        rt_kprintf("user_id: %5d student_id: %11s name: %4s device_addr: %3d state: %d ",
            att_info[i].user_id, att_info[i].student_id, att_info[i].name, att_info[i].device_addr, att_info[i].state);
        
        rt_kprintf("mac_addr:"MACSTR, MAC2STR(att_info[i].mac_addr));

        rt_kprintf(" time: 20%02d.%02d.%02d  %02d.%02d.%02d   CRC: %08X\r\n",
            att_info[i].year, att_info[i].month, att_info[i].day, att_info[i].hour, 
            att_info[i].minutes, att_info[i].second, att_info[i].crc);
    }
}
FINSH_FUNCTION_EXPORT_ALIAS(record_print, record_print, print record)

/* end of file */
