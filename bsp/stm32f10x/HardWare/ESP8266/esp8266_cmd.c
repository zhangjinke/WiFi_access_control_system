#include "esp8266_cmd.h"
#include "esp8266.h"
#include "wifi_thread.h"
#include "finsh.h"
#include <rtthread.h>

#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

struct wifi_pack wifi_pack_send;
struct mesh_device_list_type g_node_list;

s8 wait_ack(u8 cmd)
{
	u32 time = 1000;
	while(1)
	{
		if (is_recv_wifi_pack == 1) 
		{ 
			is_recv_wifi_pack = 0;
			break; 
		}
		rt_thread_delayMs(1);
		time--;
		if (time == 0) { return -1; }
	}
	if (wifi_pack_recv.cmd != cmd) { return -2; }
	if ((wifi_pack_recv.data[0] == 'e')&&(wifi_pack_recv.data[1] == 'r')&&
		(wifi_pack_recv.data[2] == 'r'))
	{
		return -3;
	}
	
	return 0;
}

s8 test_hspi(void)
{
	is_recv_wifi_pack = 0;
	wifi_send(CMD_RETURN_RECV, 0, 0);
	if (wait_ack(CMD_RETURN_RECV) < 0)
	{
		return -1;
	}
	
	return 0;
}
FINSH_FUNCTION_EXPORT(test_hspi, test_hspi)

s8 get_sdk_version(u8 ver[], u16 *lenth)
{
	if (!ver) { return -1; }
	if (!lenth) { return -1; }
	
	is_recv_wifi_pack = 0;
	wifi_send(CMD_GET_SDK_VERSION, 0, 0);
	if (wait_ack(CMD_GET_SDK_VERSION) < 0)
	{
		return -1;
	}
	rt_memcpy(ver, wifi_pack_recv.data, wifi_pack_recv.lenth);
	ver[wifi_pack_recv.lenth] = '\0';
	*lenth = wifi_pack_recv.lenth;
	
	return 0;
}

s8 get_flash_size_map(u8 *flash_size_map)
{
	if (!flash_size_map) { return -1; }
	
	is_recv_wifi_pack = 0;
	wifi_send(CMD_GET_FLASH_SIZE_MAP, 0, 0);
	if (wait_ack(CMD_GET_FLASH_SIZE_MAP) < 0)
	{
		return -1;
	}
	if (wifi_pack_recv.lenth != 1) { return -3; }
	*flash_size_map = wifi_pack_recv.data[0];
	
	return 0;
}

s8 wifi_get_ip_info(struct ip_info *station_ip, struct ip_info *ap_ip)
{
	if (!station_ip) { return -1; }
	if (!ap_ip) { return -1; }
	
	is_recv_wifi_pack = 0;
	wifi_send(CMD_WIFI_GET_IP_INFO, 0, 0);
	if (wait_ack(CMD_WIFI_GET_IP_INFO) < 0)
	{
		return -1;
	}
	if (wifi_pack_recv.lenth != 24) { return -3; }
	rt_memcpy(station_ip, wifi_pack_recv.data, 12);
	rt_memcpy(ap_ip, wifi_pack_recv.data + 12, 12);
	
	return 0;
}

s8 wifi_get_macaddr(u8 station_mac[], u8 ap_mac[])
{
	if (!station_mac) { return -1; }
	if (!ap_mac) { return -1; }
	
	is_recv_wifi_pack = 0;
	wifi_send(CMD_WIFI_GET_MACADDR, 0, 0);
	if (wait_ack(CMD_WIFI_GET_MACADDR) < 0)
	{
		return -1;
	}
	if (wifi_pack_recv.lenth != 12) { return -3; }
	rt_memcpy(station_mac, wifi_pack_recv.data, 6);
	rt_memcpy(ap_mac, wifi_pack_recv.data + 6, 6);
	
	return 0;
}

s8 get_device_list(struct mesh_device_list_type *g_node_list)
{
	if (!g_node_list) { return -1; }
	
	is_recv_wifi_pack = 0;
	wifi_send(CMD_GET_DEVICE_LIST, 0, 0);
	if (wait_ack(CMD_GET_DEVICE_LIST) < 0)
	{
		return -1;
	}
	g_node_list->size = wifi_pack_recv.lenth;
	g_node_list->scale = wifi_pack_recv.lenth/6;
	rt_memcpy(g_node_list->root.mac, wifi_pack_recv.data, 6);
	g_node_list->list = (struct mesh_device_mac_type*)(wifi_pack_recv.data + 6);

	return 0;
}

void esp8266_cmd_test()
{
	int idx = 0;
	u8 ver[10];
	u16 len;
	u8 flash_size_map = 0, *flash_size_map_str;
	struct ip_info station_ip, ap_ip;
	u8 station_mac[6], ap_mac[6];

	if (test_hspi()) { rt_kprintf("test_hspi failed\r\n"); }
	else  { rt_kprintf("test_hspi success\r\n"); }
	
	if (get_sdk_version(ver, &len)) { rt_kprintf("get_sdk_version failed\r\n"); }
	else  { rt_kprintf("get_sdk_version:%s ver_len:%d\r\n", ver, len); }
	
	if (get_flash_size_map(&flash_size_map)) { rt_kprintf("get_flash_size_map failed\r\n"); }
	else
	{
		switch(flash_size_map)
		{
			case 0: { flash_size_map_str = (u8 *)"FLASH_SIZE_4M_MAP_256_256"; } break;
			case 1: { flash_size_map_str = (u8 *)"FLASH_SIZE_2M"; } break;
			case 2: { flash_size_map_str = (u8 *)"FLASH_SIZE_8M_MAP_512_512"; } break;
			case 3: { flash_size_map_str = (u8 *)"FLASH_SIZE_16M_MAP_512_512"; } break;
			case 4: { flash_size_map_str = (u8 *)"FLASH_SIZE_32M_MAP_512_512"; } break;
			case 5: { flash_size_map_str = (u8 *)"FLASH_SIZE_16M_MAP_1024_1024"; } break;
			case 6: { flash_size_map_str = (u8 *)"FLASH_SIZE_32M_MAP_1024_1024"; } break;
			default: { flash_size_map_str = (u8 *)"ERR FLASH_SIZE"; } break;
		}
		rt_kprintf("get_flash_size_map:%d  %s\r\n", flash_size_map, flash_size_map_str);
	}
	
	if (wifi_get_ip_info(&station_ip, &ap_ip)) { rt_kprintf("wifi_get_ip_info failed\r\n"); }
	else
	{
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
	
	if (wifi_get_macaddr(station_mac, ap_mac)) { rt_kprintf("wifi_get_macaddr failed\r\n"); }
	else
	{
		rt_kprintf("station_mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n", station_mac[0], station_mac[1], station_mac[2], station_mac[3], station_mac[4], station_mac[5]);
		rt_kprintf("softap_mac:  %02x:%02x:%02x:%02x:%02x:%02x\r\n", ap_mac[0], ap_mac[1], ap_mac[2], ap_mac[3], ap_mac[4], ap_mac[5]);
	}
	
	if (get_device_list(&g_node_list)) { rt_kprintf("get_device_list failed\r\n"); }
	else
	{
		rt_kprintf("=====mac list info=====\r\n");
		rt_kprintf("root: " MACSTR "\r\n", MAC2STR(g_node_list.root.mac));
		if (g_node_list.scale >= 2)
		{
			for (idx = 0; idx < g_node_list.scale - 1; idx ++)
				rt_kprintf("idx:%d, " MACSTR "\r\n", idx, MAC2STR(g_node_list.list[idx].mac));
		}
		rt_kprintf("=====mac list end======\r\n");
	}
}
void cmd_test()
{
	esp8266_cmd_test();
}
FINSH_FUNCTION_EXPORT(cmd_test, esp8266_cmd_test)
