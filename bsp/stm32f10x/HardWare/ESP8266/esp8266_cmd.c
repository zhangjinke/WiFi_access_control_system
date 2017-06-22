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
 * \brief esp8266指令
 *
 * \internal
 * \par Modification history
 * - 1.00 17-01-31 zhangjinke, first implementation.
 * \endinternal
 */ 
#include "esp8266_cmd.h"

#include <rtthread.h>
#include <finsh.h>
#include "wifi_thread.h"
#include "global.h"

struct wifi_pack wifi_pack_send;
struct mesh_device_list_type g_node_list;


/**
 * \brief 等待esp8266应答
 *
 * \param[in] cmd 命令
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t wait_ack (uint8_t cmd)
{
    uint32_t time = 1000;
    while(1) {
        if (is_recv_wifi_pack == 1) { 
            is_recv_wifi_pack = 0;
            break; 
        }
        RT_THREAD_DELAY_MS(1);
        time--;
        if (time == 0) { 
            return -1; 
        }
    }
    if (wifi_pack_recv.cmd != cmd) { 
        return -2;
    }
    if ((wifi_pack_recv.data[0] == 'e')&&
        (wifi_pack_recv.data[1] == 'r')&&
        (wifi_pack_recv.data[2] == 'r')) {
        return -3;
    }
    
    return 0;
}

/**
 * \brief 测试hspi通信
 *
 * \param 无
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t test_hspi (void)
{
    int8_t recv;
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_RETURN_RECV, 0, 0);
    if ((recv = wait_ack(CMD_RETURN_RECV)) < 0) {
        return recv;
    }
    
    return 0;
}
FINSH_FUNCTION_EXPORT(test_hspi, test_hspi)

/**
 * \brief 获取sdk版本号
 *
 * \param[out] p_ver   sdk版本号
 * \param[out] p_lenth sdk版本号字符串长度
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t sdk_version_get (uint8_t *p_ver, uint16_t *p_lenth)
{
    int8_t recv;
    
    if (RT_NULL == p_ver) { return -1; }
    if (RT_NULL == p_lenth) { return -1; }
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_GET_SDK_VERSION, 0, 0);
    if ((recv = wait_ack(CMD_GET_SDK_VERSION)) < 0) {
        return recv;
    }
    rt_memcpy(p_ver, wifi_pack_recv.data, wifi_pack_recv.lenth);
    p_ver[wifi_pack_recv.lenth] = '\0';
    *p_lenth = wifi_pack_recv.lenth;
    
    return 0;
}

/**
 * \brief 查询Flash size以及Flash map
 *
 * \param[out] p_flash_size_map 获取到的flash信息
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t get_flash_size_map (uint8_t *p_flash_size_map)
{
    int8_t recv;
    
    if (RT_NULL == p_flash_size_map) { return -1; }
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_GET_FLASH_SIZE_MAP, 0, 0);
    if ((recv = wait_ack(CMD_GET_FLASH_SIZE_MAP)) < 0) {
        return recv;
    }
    if (wifi_pack_recv.lenth != 1) { return -3; }
    *p_flash_size_map = wifi_pack_recv.data[0];
    
    return 0;
}

/**
 * \brief 查询IP地址
 *
 * \param[out] p_station_ip station ip地址
 * \param[out] p_ap_ip      ap ip地址
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t wifi_ip_get (ip_info_t *p_station_ip, ip_info_t *p_ap_ip)
{
    int8_t recv;
    
    if (RT_NULL == p_station_ip) { return -1; }
    if (RT_NULL == p_ap_ip) { return -1; }
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_WIFI_GET_IP_INFO, 0, 0);
    if ((recv = wait_ack(CMD_WIFI_GET_IP_INFO)) < 0) {
        return recv;
    }
    if (wifi_pack_recv.lenth != 24) { return -3; }
    rt_memcpy(p_station_ip, wifi_pack_recv.data, 12);
    rt_memcpy(p_ap_ip, wifi_pack_recv.data + 12, 12);
    
    return 0;
}

/**
 * \brief 查询mac地址
 *
 * \param[out] p_station_ip station ip地址
 * \param[out] p_ap_ip      ap ip地址
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t wifi_mac_addr_get (uint8_t *p_station_mac, uint8_t *p_ap_mac)
{
    int8_t recv = 0;
    
    if (RT_NULL == p_station_mac) { return -1; }
    if (RT_NULL == p_ap_mac) { return -1; }
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_WIFI_GET_MACADDR, 0, 0);
    if ((recv = wait_ack(CMD_WIFI_GET_MACADDR)) < 0) {
        return recv;
    }
    if (wifi_pack_recv.lenth != 12) { return -3; }
    rt_memcpy(p_station_mac, wifi_pack_recv.data, 6);
    rt_memcpy(p_ap_mac, wifi_pack_recv.data + 6, 6);
    
    return 0;
}

/**
 * \brief 获取mesh设备列表
 *
 * \param[out] p_node_list mesh设备列表
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t device_list_get (struct mesh_device_list_type *p_node_list)
{
    int8_t recv;
    
    if (RT_NULL == p_node_list) { return -1; }
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_GET_DEVICE_LIST, 0, 0);
    if ((recv = wait_ack(CMD_GET_DEVICE_LIST)) < 0) {
        return recv;
    }
    p_node_list->size = wifi_pack_recv.lenth;
    p_node_list->scale = wifi_pack_recv.lenth/6;
    rt_memcpy(p_node_list->root.mac, wifi_pack_recv.data, 6);
    p_node_list->list = (struct mesh_device_mac_type*)(wifi_pack_recv.data + 6);

    return 0;
}

/**
 * \brief 设置服务器IP与端口
 *
 * \param[in] p_ip ip地址
 * \param[in] port 端口
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t server_addr_set (uint8_t *p_ip, uint16_t port)
{
    int8_t recv;
    uint8_t buf[6];
    
    if (RT_NULL == p_ip) { return -1; }
    
    rt_memcpy(buf + 0, p_ip, 4);
    rt_memcpy(buf + 4, &port, 2);
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_SERVER_ADDR_SET, 6, buf);
    if ((recv = wait_ack(CMD_SERVER_ADDR_SET)) < 0) {
        return recv;
    }

    return 0;
}

/**
 * \brief 设置MESH组ID
 *
 * \param[in] p_id MESH组ID
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t mesh_group_id_set (uint8_t *p_id)
{
    int8_t recv;
    
    if (RT_NULL == p_id) { return -1; }
        
    is_recv_wifi_pack = 0;
    wifi_send(CMD_MESH_GROUP_ID_SET, 6, p_id);
    if ((recv = wait_ack(CMD_MESH_GROUP_ID_SET)) < 0) {
        return recv;
    }

    return 0;
}

/**
 * \brief 设置路由器信息
 *
 * \param[in] p_ssid   路由器名称
 * \param[in] p_passwd 路由器密码
 * \param[in] auth     加密方式
 * \param[in] p_mac    路由器mac地址(只有连接隐藏wifi时才需要配置)
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t router_set (uint8_t *p_ssid, uint8_t *p_passwd, uint8_t auth, uint8_t *p_mac)
{
    int8_t recv;
    
    if (RT_NULL == p_ssid) { return -1; }
    if (RT_NULL == p_passwd) { return -1; }
    if (RT_NULL == p_mac) { return -1; }
    
    uint32_t lenth = (32 + 1) + (256 + 1) + (1) + (6);
    uint8_t *buf = rt_malloc(lenth);
    if (RT_NULL == buf) {
        rt_kprintf("router_set malloc %d byte failed\r\n", lenth);
        return -1;
    }
    
    rt_memcpy(buf + 0,                    p_ssid,   32 + 1);
    rt_memcpy(buf + 32 + 1,               p_passwd, 256 + 1);
    rt_memcpy(buf + 32 + 1 + 256 + 1,    &auth,   1);
    rt_memcpy(buf + 32 + 1 + 256 + 1 + 1, p_mac,    6);

    is_recv_wifi_pack = 0;
    wifi_send(CMD_ROUTER_SET, lenth, buf);
    if ((recv = wait_ack(CMD_ROUTER_SET)) < 0) {
        rt_free(buf);
        return recv;
    }

    rt_free(buf);
    return 0;
}

/**
 * \brief 设置MESH网络信息
 *
 * \param[in] p_ssid   路由器名称
 * \param[in] p_passwd 路由器密码
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t mesh_wifi_set (uint8_t *p_ssid, uint8_t *p_passwd)
{
    int8_t recv;
    
    if (RT_NULL == p_ssid) { return -1; }
    if (RT_NULL == p_passwd) { return -1; }
    
    uint32_t lenth = (32 + 1) + (256 + 1);
    uint8_t *buf = rt_malloc(lenth);
    if (RT_NULL == buf) {
        rt_kprintf("mesh_wifi_set malloc %d byte failed\r\n", lenth);
        return -1;
    }
    
    rt_memcpy(buf + 0,      p_ssid,   32 + 1);
    rt_memcpy(buf + 32 + 1, p_passwd, 256 + 1);

    is_recv_wifi_pack = 0;
    wifi_send(CMD_MESH_WIFI_SET, lenth, buf);
    if ((recv = wait_ack(CMD_MESH_WIFI_SET)) < 0) {
        rt_free(buf);
        return recv;
    }

    rt_free(buf);
    return 0;
}

/**
 * \brief 初始化MESH
 *
 * \param 无
 *
 * \retval  0 接收到应答
 * \retval -1 超时
 * \retval -2 发送与接收的cmd不匹配
 * \retval -3 接收到错误
 */
int8_t mesh_init (void)
{
    int8_t recv;
    
    is_recv_wifi_pack = 0;
    wifi_send(CMD_MESH_INIT, 0, 0);
    if ((recv = wait_ack(CMD_MESH_INIT)) < 0) {
        return recv;
    }

    return 0;
}

void esp8266_cmd_test ()
{
    int idx = 0;
    uint8_t ver[10];
    uint16_t len;
    uint8_t flash_size_map = 0, *flash_size_map_str;
    struct ip_info station_ip, ap_ip;
    uint8_t station_mac[6], ap_mac[6];
    
    uint8_t  server_ip[] = {112, 74, 216, 117};
    uint16_t server_port = 9200;
    
    uint8_t  group_id[] = {0, 0, 0, 0, 0, 0};

    static uint8_t router_ssid[32 + 1] = "peace";
    static uint8_t router_passwd[256 + 1] = "zzzzzzzzx";
    static uint8_t router_auth = AUTH_WPA2_PSK;
    static uint8_t router_bssid[6] = {0, 0, 0, 0, 0, 0};

    static uint8_t mesh_ssid[32 + 1] = "MESH_EE_BANG";
    static uint8_t mesh_passwd[256 + 1] = "zzzzzzzzy";

    if (test_hspi()) { 
        rt_kprintf("%s", wifi_pack_recv.data);
    } else { 
        rt_kprintf("test_hspi success\r\n"); 
    }
    
    if (sdk_version_get(ver, &len)) { 
        rt_kprintf("%s", wifi_pack_recv.data);
    } else { 
        rt_kprintf("sdk_version_get:%s ver_len:%d\r\n", ver, len); 
    }
    
    if (get_flash_size_map(&flash_size_map)) { 
        rt_kprintf("%s", wifi_pack_recv.data);
    } else {
        switch(flash_size_map)
        {
            case 0: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_4M_MAP_256_256"; } break;
            case 1: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_2M"; } break;
            case 2: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_8M_MAP_512_512"; } break;
            case 3: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_16M_MAP_512_512"; } break;
            case 4: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_32M_MAP_512_512"; } break;
            case 5: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_16M_MAP_1024_1024"; } break;
            case 6: { flash_size_map_str = (uint8_t *)"FLASH_SIZE_32M_MAP_1024_1024"; } break;
            default: { flash_size_map_str = (uint8_t *)"ERR FLASH_SIZE"; } break;
        }
        rt_kprintf("get_flash_size_map:%d  %s\r\n", flash_size_map, flash_size_map_str);
    }
    
    if (wifi_ip_get(&station_ip, &ap_ip)) { 
        rt_kprintf("%s", wifi_pack_recv.data);
    } else {
        rt_kprintf("station_ip:%d.%d.%d.%d  mask:%d.%d.%d.%d  gw:%d.%d.%d.%d\r\n", 
                    station_ip.ip[0], station_ip.ip[1], station_ip.ip[2], station_ip.ip[3],
                    station_ip.netmask[0], station_ip.netmask[1], station_ip.netmask[2], station_ip.netmask[3],
                    station_ip.gw[0], station_ip.gw[1], station_ip.gw[2], station_ip.gw[3]
        );
        rt_kprintf("softap_ip: %d.%d.%d.%d  mask:%d.%d.%d.%d  gw:%d.%d.%d.%d\r\n", 
                    ap_ip.ip[0], ap_ip.ip[1], ap_ip.ip[2], ap_ip.ip[3],
                    ap_ip.netmask[0], ap_ip.netmask[1], ap_ip.netmask[2], ap_ip.netmask[3],
                    ap_ip.gw[0], ap_ip.gw[1], ap_ip.gw[2], ap_ip.gw[3]
        );
    }
    
    if (wifi_mac_addr_get(station_mac, ap_mac)) { 
        rt_kprintf("%s", wifi_pack_recv.data);
    } else {
        rt_kprintf("station_mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n", station_mac[0], station_mac[1], station_mac[2], station_mac[3], station_mac[4], station_mac[5]);
        rt_kprintf("softap_mac:  %02x:%02x:%02x:%02x:%02x:%02x\r\n", ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
    }
    
    if (device_list_get(&g_node_list)) { 
        rt_kprintf("%s", wifi_pack_recv.data);
    } else {
        rt_kprintf("=====mac list info=====\r\n");
        rt_kprintf("root: " MACSTR "\r\n", MAC2STR(g_node_list.root.mac));
        if (g_node_list.scale >= 2)
        {
            for (idx = 0; idx < g_node_list.scale - 1; idx ++)
                rt_kprintf("idx:%d, " MACSTR "\r\n", idx, MAC2STR(g_node_list.list[idx].mac));
        }
        rt_kprintf("=====mac list end======\r\n");
    }
    
    /* 设置服务器IP与端口 */
    if (server_addr_set(server_ip,server_port)) { 
        rt_kprintf("server_addr_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 设置MESH组ID */
    if (mesh_group_id_set(group_id)) { 
        rt_kprintf("mesh_group_id_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 设置路由器信息 */
    if (router_set(router_ssid, router_passwd, router_auth, router_bssid)) { 
        rt_kprintf("router_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 设置MESH网络信息 */
    if (mesh_wifi_set(mesh_ssid, mesh_passwd)) { 
        rt_kprintf("mesh_wifi_set failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
    
    /* 初始化MESH */
    if (mesh_init()) { 
        rt_kprintf("mesh_init failed\r\n"); 
    } else {
        rt_kprintf("%s", wifi_pack_recv.data);
    }
}
void cmd_test()
{
    esp8266_cmd_test();
}
FINSH_FUNCTION_EXPORT(cmd_test, esp8266_cmd_test)

/* end of file */
