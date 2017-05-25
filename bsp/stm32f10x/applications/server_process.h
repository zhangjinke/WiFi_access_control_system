/*******************************************************************************
*                        WiFi Access Control System
*                       ----------------------------
*                                  EE BANG
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

#ifndef __SERVER_PROCESS_H
#define __SERVER_PROCESS_H

#include <stdint.h>

#define CMD_SERVER_USER_ADD     (0x01) /**< \brief 添加用户 */
#define CMD_SERVER_USER_DEL     (0x02) /**< \brief 删除用户 */
#define CMD_SERVER_CONNECT      (0x03) /**< \brief 联机 */
#define CMD_SERVER_TIMESYNC     (0x04) /**< \brief 同步时间 */
#define CMD_SERVER_ATT_GET      (0x05) /**< \brief 获取指定条数考勤信息 每次最多25条 */
#define CMD_SERVER_DOOR_OPEN    (0x06) /**< \brief 开门 */
#define CMD_SERVER_CONFIG_SET   (0x07) /**< \brief 下传配置信息 */

#define CMD_SERVER_FILE_START   (0xF0) /**< \brief 开始文件传输 */
#define CMD_SERVER_FILE_DATA    (0xF1) /**< \brief 文件数据 */
#define CMD_SERVER_FILE_END     (0xF2) /**< \brief 文件传输结束 */

#define CMD_SERVER_ACK          (0x55) /**< \brief ACK */
#define CMD_SERVER_NACK         (0xAA) /**< \brief NACK */


/**
 * \brief 处理服务器发送的数据
 *
 * \param[in] cmd    : 命令
 * \param[in] p_data : 数据首地址
 * \param[in] lenth  : 数据长度
 *
 * \return 无
 */
void data_process(uint8_t cmd, uint8_t *p_data, uint16_t lenth);

#endif /* __SERVER_PROCESS_H */

/* end of file */
