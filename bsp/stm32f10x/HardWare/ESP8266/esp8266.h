#ifndef _ESP8266_H_
#define _ESP8266_H_

#include "sys.h"

#define HSPI_RX         (1 << 0) //hspi接收事件
#define INIT_OK         (1 << 1) //esp8266初始化完成事件

#define ESP8266_EN      PCout(5) //ESP8266 EN
#define ESP8266_RST     PBout(0) //ESP8266 RST
#define ESP8266_BOOT    PCout(4) //ESP8266 BOOT

enum esp8266_io
{
	EN = 0,
	RST,
	CS,
	BOOT
};

struct ip_info 
{
    u8 ip[4];
    u8 netmask[4];
    u8 gw[4];
};

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

#define ESP_MESH_HEAD_SIZE              (16)
#define ESP_MESH_VER                    (0)
#define ESP_MESH_GROUP_ID_LEN           (6)
#define ESP_MESH_ADDR_LEN               (6)
#define ESP_MESH_OPTION_MAX_LEN         (255)
#define ESP_MESH_PKT_LEN_MAX            (1300)
#define ESP_MESH_FRAG_ID_MASK           (0xFFFF)
#define ESP_MESH_FRAG_IDX_MASK          (0x3FFF)
#define ESP_MESH_OT_LEN_LEN             (sizeof(uint16_t))
#define ESP_MESH_HLEN                   (sizeof(struct mesh_header_format))
#define ESP_MESH_OPTION_HLEN            (sizeof(struct mesh_header_option_format))
#define ESP_MESH_OP_MAX_PER_PKT         ((ESP_MESH_PKT_LEN_MAX - ESP_MESH_HLEN) / ESP_MESH_OPTION_MAX_LEN)
#define ESP_MESH_DEV_MAX_PER_OP         ((ESP_MESH_OPTION_MAX_LEN - ESP_MESH_OPTION_HLEN) / ESP_MESH_ADDR_LEN)
#define ESP_MESH_DEV_MAX_PER_PKT        (ESP_MESH_OP_MAX_PER_PKT * ESP_MESH_DEV_MAX_PER_OP)
#define ESP_MESH_BCAST_ADDR             (mesh_bcast_addr.addr)
#define ESP_MESH_MCAST_ADDR             (mesh_mcast_addr.addr)

__packed struct mesh_header_option_format {
    uint8_t otype;      // option type
    uint8_t olen;       // current option length
    uint8_t ovalue[];   // option value
};

__packed struct mesh_header_option_header_type {
    uint16_t ot_len;    // option total length;
//    struct mesh_header_option_format olist[0];  // option list
};

__packed struct mesh_header_format {
    uint8_t ver:2;          // version of mesh
    uint8_t oe: 1;          // option exist flag
    uint8_t cp: 1;          // piggyback congest permit in packet
    uint8_t cr: 1;          // piggyback congest request in packet
    uint8_t rsv:3;          // reserve for fulture;
    __packed struct {
        uint8_t d:  1;      // direction, 1:upwards, 0:downwards
        uint8_t p2p:1;      // node to node packet
        uint8_t protocol:6; // protocol used by user data;
    } proto;
    uint16_t len;           // packet total length (include mesh header)
    uint8_t dst_addr[ESP_MESH_ADDR_LEN];  // destiny address
    uint8_t src_addr[ESP_MESH_ADDR_LEN];  // source address
    uint8_t user_data[];    // 用户数据，仅当没有选项字节时该成员才可用
//    struct mesh_header_option_header_type option[0];  // mesh option /* c99不能像gnu c一样支持柔性数组嵌套，故屏蔽 */
};

__packed struct mesh_device_mac_type {
    uint8_t mac[6];
};

struct mesh_device_list_type {
    uint16_t size;
    uint16_t scale;  // include root
    struct mesh_device_mac_type root;
    struct mesh_device_mac_type *list;
};

/* esp8266事件控制块 */
extern struct rt_event esp8266_event;
 
extern u8 wr_rdy, rd_rdy;

extern u8 recv_pack[1024*5];
extern u8 is_recv_pack;
extern u32 recv_lenth;

extern s8 init_esp8266(void);
extern s8 esp8266_spi_read(void);
extern s8 esp8266_spi_write(u8 *pack, u32 lenth);

#endif
