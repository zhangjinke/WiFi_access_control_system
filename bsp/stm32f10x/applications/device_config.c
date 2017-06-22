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
 * \brief 设备配置文件相关
 *
 * \internal
 * \par Modification history
 * - 1.00 16-09-19  zhangjinke, first implementation.
 * \endinternal
 */ 
#include "device_config.h"

#include <rtthread.h>
#include <finsh.h>
#include <dfs.h>
#include <dfs_posix.h>
#include "global.h"
#include "stm32_crc.h"
#include "esp8266.h"

/** \brief 设置调试信息打印开关 */
#ifdef PRINT_CFG_DEBUG_INFO
#define cfg_printf(fmt,args...) rt_kprintf(fmt, ##args)
#else
#define cfg_printf(fmt,args...)
#endif

/** \brief 设备配置文件路径 */
const char *gp_device_config_path = "/config.bin";

device_config_t g_device_config;

/**
 * \brief 初始化设备配置
 *
 * \param[in] p_device_config 设备配置结构体
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t device_config_init (device_config_t *p_device_config)
{
    int fd = -1;
    uint32_t crc = 0;
    
    /* 打开设备配置 */
    fd = open(gp_device_config_path, O_RDWR|O_CREAT, 0);
    if (fd < 0) {
        cfg_printf("open %s failed\r\n", gp_device_config_path);
        close(fd);
        return -1;
    } else {
        
        /* 移动文件指针到指定位置 */
        if (lseek(fd, 0, SEEK_SET) == -1) {
            cfg_printf("lseek %s failed\r\n", gp_device_config_path);
            close(fd);
            return -1;
        } else {
            
            /* 获取设备配置信息 */
            read(fd, p_device_config, sizeof(device_config_t));
            
            /* 计算CRC */
            crc = block_crc_calc((uint8_t *)(p_device_config), sizeof(device_config_t) - 4);
            if (crc != p_device_config->crc) {
                cfg_printf("device config crc validators fail, init device config\r\n");

                /* 初始化设备配置信息 */
                rt_memset(p_device_config, 0, sizeof(device_config_t));

                /* 计算CRC */
                p_device_config->crc = block_crc_calc((uint8_t *)(p_device_config), sizeof(device_config_t) - 4);
                
                /* 移动文件指针到指定位置 */
                if (lseek(fd, 0, SEEK_SET) == -1) {
                    cfg_printf("lseek %s failed\r\n", gp_device_config_path);
                    close(fd);
                    return -1;
                }
                
                /* 写入设备配置 */
                if (write(fd, p_device_config, sizeof(device_config_t)) != sizeof(device_config_t)) {
                    cfg_printf("write %s failed\r\n", gp_device_config_path);
                    close(fd);
                    return -1;
                } else {
                    close(fd);
                    cfg_printf("init device config success\r\n");            
                }        
            } else {
                close(fd);
            }
        }
    }
    
    return 0;
}

/**
 * \brief 获取/设置设备配置
 *
 * \param[in,out] device_config 设备配置结构体
 * \param[in]     cmd 0: 获取设备配置 1: 设置设备配置
 *
 * \retval  0 成功
 * \retval -1 失败
 */
int8_t device_config_get_set (device_config_t *p_device_config, uint8_t cmd)
{
    int fd;
    
    /* 检查参数合法性 */
    if (p_device_config == 0) {
        cfg_printf("p_device_config addr is 0\r\n");
        return -1;
    }
    
    /* 打开人员信息数据库 */
    fd = open(gp_device_config_path, O_RDWR|O_CREAT, 0);
    if (fd < 0) {
        cfg_printf("open %s failed\r\n", gp_device_config_path);
        close(fd);
        return -1;
    } else {
        /* 移动文件指针到指定位置 */
        if (lseek(fd, 0, SEEK_SET) == -1) {
            cfg_printf("lseek %s failed\r\n", gp_device_config_path);
            close(fd);
            return -1;
        } else {
            if (cmd == 0) {
                
                /* 获取设备配置 */
                if (read(fd, p_device_config, sizeof(device_config_t)) != sizeof(device_config_t)) {
                    cfg_printf("read %s failed\r\n", gp_device_config_path);
                    close(fd);
                    return -1;
                } else {
                    close(fd);
                    if (p_device_config->crc == block_crc_calc((uint8_t *)p_device_config, sizeof(device_config_t) - 4)) {
                        cfg_printf("get device config addr is %d, name is %s success\r\n",p_device_config->device_addr, p_device_config->this_device_name);
                    } else {
                        cfg_printf("crc validators fail addr is %d, name is %s success\r\n",p_device_config->device_addr, p_device_config->this_device_name);
                        return -1;
                    }
                }        
            } else if (cmd == 1) 
            {
                /* 计算CRC */
                p_device_config->crc = block_crc_calc((uint8_t *)p_device_config, sizeof(device_config_t) - 4);
                
                /* 写入设备配置 */
                if (write(fd, p_device_config, sizeof(device_config_t)) != sizeof(device_config_t))
                {
                    cfg_printf("write %s failed\r\n", gp_device_config_path);
                    close(fd);
                    return -1;
                } else {
                    close(fd);
                    cfg_printf("set device config addr is %d, name is %s success\r\n",p_device_config->device_addr, p_device_config->this_device_name);
                }        
            } else {
                close(fd);
                cfg_printf("get set device config cmd unknown\r\n");
                return -1;
            }
        }
    }
    
    return 0;
}

/**
 * \brief 打印设备配置信息
 *
 * \param[in] p_device_config 设备配置结构体
 *
 * \return 无
 */
void device_config_print (device_config_t *p_device_config)
{
    rt_kprintf("\r\n===============device_config===============\r\n");
    rt_kprintf("device addr:%d name:\"%s\"\r\n",p_device_config->device_addr, p_device_config->this_device_name);
    rt_kprintf("server_ip: %d.%d.%d.%d port:%d\r\n", p_device_config->server_ip[0], p_device_config->server_ip[1], p_device_config->server_ip[2], p_device_config->server_ip[3], p_device_config->server_port);
    rt_kprintf("router_ssid:\"%s\" passwd:\"%s\" auth:%d\r\n", p_device_config->router_ssid, p_device_config->router_passwd, p_device_config->router_auth);
    rt_kprintf("router_bssid:"MACSTR"\r\n", MAC2STR(p_device_config->router_bssid));
    rt_kprintf("mesh_ssid:\"%s\" passwd:\"%s\"\r\n", p_device_config->mesh_ssid, p_device_config->mesh_passwd);
    rt_kprintf("mesh_group_id: "MACSTR"\r\n", MAC2STR(p_device_config->mesh_group_id));
    rt_kprintf("===========================================\r\n\r\n");
}

void cfg_print(void)
{
    device_config_print(&g_device_config);
}
FINSH_FUNCTION_EXPORT(cfg_print, cfg_print)

/* end of file */
