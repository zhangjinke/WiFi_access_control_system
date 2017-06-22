/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: main.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 06 日
**
** 描        述: 这是系统多处理器初始化函数

** 日志:
2016.09.06  创建本文件
*********************************************************************************************************/
#include <rtthread.h>
#include <finsh.h>
#include <stdint.h>
#include <rtdevice.h>

#include "rfid_thread.h"
#include "user_check_thread.h"
#include "wifi_thread.h"
#include "rtc_thread.h"

#include "p_database.h"
#include "att_database.h"

#include "device_config.h"
#include "esp8266.h"
#include "beep_door.h"
#include "esp8266_cmd.h"
#include "global.h"
#include "misc.h"

#ifdef  TOUCH_SCREEN
#include "touch_screen.h"
#endif  /* TOUCH_SCREEN */

#ifdef  TFT
#include "ILI93xx.h"
#endif  /* TFT */

#ifdef  STemWin
#include "GUI.h"
#include "WM.h"

#include "WindowDLG.h"
#endif  /* STemWin */

#ifdef  RT_USING_SDIO
#include <drivers/mmcsd_core.h>
#include <drivers/sdio.h>
#include <drivers/sd.h>
#endif  /* RT_USING_SDIO */

#ifdef RT_USING_DFS
#include <dfs.h>
#include <dfs_posix.h>
#endif  /* RT_USING_DFS */

#ifdef  RT_USING_SPI
#include "spi_bus.h"
#endif  /* RT_USING_SPI */

#ifdef  STemWin
static rt_uint8_t emwin_stack[ 8192 ];    //线程栈
static struct rt_thread emwin_thread;     //线程控制块
#endif  /* STemWin */

#ifdef  EmWin_Demo
static rt_uint8_t emwin_demo_stack[ 8192 ];    //线程栈
static struct rt_thread emwin_demo_thread;     //线程控制块
#endif  /* EmWin_Demo */

#ifdef  TOUCH_SCREEN
static rt_uint8_t touch_screen_stack[ 2048 ];    //线程栈
static struct rt_thread touch_screen_thread;     //线程控制块
#endif  /* TOUCH_SCREEN */

/*******************************************************************************
* 函数名     : touch_screen_thread_entry
* 描述       : 触摸屏扫描线程
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
#ifdef  TOUCH_SCREEN
static void touch_screen_thread_entry(void* parameter)
{
    GUI_PID_STATE State0, State1;
    uint16_t touch_x0 = 0, touch_x1 = 0;
    uint16_t touch_y0 = 0, touch_y1 = 0;
    while(1)
    {
        #ifdef  STemWin
        if (tp_irq0 == 0)
        {
            TP_Read_XY(LCD0, &touch_y0, &touch_x0);
            
             State0.x = tp_dev0.xfac*touch_x0+tp_dev0.xoff;//将结果转换为屏幕坐标
            State0.y = tp_dev0.yfac*touch_y0+tp_dev0.yoff;  
            State0.Pressed = 1;
            State0.Layer = 0;
            GUI_TOUCH_StoreStateEx(&State0);
        }
        else
        {
            State0.Pressed = 0;
            State0.Layer = 0;    
            GUI_TOUCH_StoreStateEx(&State0);
        }
        if (tp_irq1 == 0)
        {
            TP_Read_XY(LCD1, &touch_y1, &touch_x1);

             State1.x = tp_dev1.xfac*touch_x1+tp_dev1.xoff;//将结果转换为屏幕坐标
            State1.y = tp_dev1.yfac*touch_y1+tp_dev1.yoff;  
            State1.Pressed = 1;
            State1.Layer = 1;
            GUI_TOUCH_StoreStateEx(&State1);
        }
        else
        {
            State1.Pressed = 0;
            State1.Layer = 1;    
            GUI_TOUCH_StoreStateEx(&State1);
        }
        #endif  /* STemWin */
        RT_THREAD_DELAY_MS(10);
    }    
}
#endif  /* TOUCH_SCREEN */

/*******************************************************************************
* 函数名     : emwin_demo_thread_entry
* 描述       : emwin_demo线程
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
#ifdef  STemWin
static void emwin_thread_entry(void* parameter)
{    
    WM_SetCreateFlags(WM_CF_MEMDEV);
    GUI_Init();              //STemWin初始化
    
//    GUI_Exec();
    gui_init();
    
    while(1)
    {
        GUI_SelectLayer(1);
        GUI_Delay(100);
    }    
}
#endif  /* STemWin */

/*******************************************************************************
* 函数名     : emwin_demo_thread_entry
* 描述       : emwin_demo线程
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值    : None
*******************************************************************************/
#ifdef  EmWin_Demo
static void emwin_demo_thread_entry(void* parameter)
{    
    RT_THREAD_DELAY_MS(10);
    while(1)
    {
        #ifdef  EmWin_Demo
        #include "GUIDEMO.h"
        GUIDEMO_Main();    
        #endif  /* EmWin_Demo */
        GUI_Delay(100);
    }    
}
#endif  /* EmWin_Demo */

rt_device_t p_device_finger_in_usart;

rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    static uint8_t rx_buf[128];
    uint32_t rx_length;
    
    rx_length = rt_device_read(p_device_finger_in_usart, 0, rx_buf, size);
    
    for (int i = 0; i < rx_length; i++)
    {
        rt_kprintf("%02X ", rx_buf[i]);
    }
    rt_kprintf("\r\n");
}

void finger ()
{
    uint8_t finger_get[] = {0xF5, 0x23, 0x00, 0x00, 0x00, 0x00, 0x23, 0xF5};
    rt_err_t err;
    rt_serial_t *p_device_serial;
    
    p_device_finger_in_usart = rt_device_find("uart1");
    if (p_device_finger_in_usart != RT_NULL)
    {

        p_device_serial = (rt_serial_t *)p_device_finger_in_usart;
        p_device_serial->config.baud_rate = 115200;
        
        /* 设置回调函数及打开设备*/
        err = rt_device_set_rx_indicate(p_device_finger_in_usart, uart_input);
        rt_kprintf("rt_device_set_rx_indicate %d\r\n", err);
        
        err = rt_device_open(p_device_finger_in_usart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        rt_kprintf("rt_device_open %d\r\n", err);
        
        err = rt_device_write(p_device_finger_in_usart, 0, finger_get, sizeof(finger_get));
        rt_kprintf("rt_device_write %d\r\n", err);
    }

}
    
/*******************************************************************************
* 函数名     : user_init_thread_entry
* 描述       : 用户初始化进程
* 输入         : - parameter: 线程入口参数
* 输出         : None
* 返回值        : None
*******************************************************************************/
void user_init_thread_entry(void* parameter)
{
    rt_err_t result;
    rt_thread_t p_thread;
    
    /* 进入临界区*/
    void rt_enter_critical(void); 
        
    /* 开启CRC时钟 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);
    
    /* 初始化蜂鸣器、门锁 */
    rt_hw_beep_door_init();
    
    /* 初始化设备配置 */
    device_config_init(&g_device_config);
    
    /* 配置设备信息 */
    rt_memset(&g_device_config, 0, sizeof(device_config_t));
    g_device_config.device_addr = 3;
    strcpy((char *)g_device_config.this_device_name, "EE Bang");
    g_device_config.server_ip[0] = 192;
    g_device_config.server_ip[1] = 168;
    g_device_config.server_ip[2] = 43;
    g_device_config.server_ip[3] = 79;
    g_device_config.server_port = 2756;
    strcpy((char *)g_device_config.router_ssid, "peace");
    strcpy((char *)g_device_config.router_passwd, "zzzzzzzzx");
    g_device_config.router_auth = AUTH_WPA2_PSK;
    strcpy((char *)g_device_config.mesh_ssid, "EE Bang MESH");
    strcpy((char *)g_device_config.mesh_passwd, "zzzzzzzzy");

    device_config_get_set(&g_device_config,SET_DEVICE);
    device_config_print(&g_device_config);

//    finger();
    
    /* 初始化卡号数组 */
    if (card_array_init(g_card_id_user_id_list) == -1)
    {
        rt_kprintf("init card array failed\r\n");
    }
    
    /* 初始化考勤数据库 */
    att_database_init(&g_att_header);
    
    /* 初始化事件对象 */
    rt_event_init(&esp8266_event, "esp8266_event", RT_IPC_FLAG_FIFO);
    rt_event_init(&g_user_check_event, "g_user_check_event", RT_IPC_FLAG_FIFO);
    
    /* 初始化SPI总线 */
    rt_hw_stm32_spi_bus_init();
    
#ifdef  TFT
    
    /* 初始化TFT液晶 */
    TFTLCD_Init();
    POINT_COLOR = WHITE;
    BACK_COLOR = BLACK;
    LCD_ShowString(LCD1,0,0,320,16,16,(uint8_t *)"WiFi Access Control System");
    set_lcd_led(0, 0);
    set_lcd_led(1, 0);
#endif  /* TFT */

#ifdef  TOUCH_SCREEN

    /* 初始化触摸屏 */
    TP_Init();
#endif  /* TOUCH_SCREEN */

    /* 初始化ESP8266 */
    init_esp8266();

#ifdef  STemWin

    /* 初始化emwin线程 */
    result = rt_thread_init(&emwin_thread,
                            "emwin",
                            emwin_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&emwin_stack[0],
                            sizeof(emwin_stack),
                            18,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&emwin_thread);
    }
#endif  /* STemWin */
        
#ifdef  EmWin_Demo
    
    /* 初始化emwin demo线程 */
    result = rt_thread_init(&emwin_demo_thread,
                            "emwin_demo",
                            emwin_demo_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&emwin_demo_stack[0],
                            sizeof(emwin_demo_stack),
                            18,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&emwin_demo_thread);
    }
#endif  /* EmWin_Demo */
    
#ifdef  TOUCH_SCREEN
    
    /* 初始化touch_screen线程 */
    result = rt_thread_init(&touch_screen_thread,
                            "touch_screen",
                            touch_screen_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&touch_screen_stack[0],
                            sizeof(touch_screen_stack),
                            17,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&touch_screen_thread);
    }
#endif  /* TOUCH_SCREEN */

#ifdef  RC522
    
    /* 初始化rc522线程 */
    result = rt_thread_init(&rc522_thread,
                            "rc522",
                            rc522_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&rc522_stack[0],
                            sizeof(rc522_stack),
                            25,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&rc522_thread);
    }
#endif  /* RC522 */
    
    /* 初始化权限检查线程 */
    result = rt_thread_init(&user_check_thread,
                            "user_check",
                            user_check_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&user_check_stack[0],
                            sizeof(user_check_stack),
                            19,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&user_check_thread);
    }
    
    /* 初始化wifi线程 */
    p_thread = rt_thread_create("wifi",
                                wifi_thread_entry, 
                                RT_NULL,
                                64 * 1024, 
                                5, 
                                5);
    if (p_thread != RT_NULL)
        rt_thread_startup(p_thread);

//    result = rt_thread_init(&wifi_thread,
//                            "wifi",
//                            wifi_thread_entry,
//                            RT_NULL,
//                            (rt_uint8_t*)&wifi_stack[0],
//                            sizeof(wifi_stack),
//                            5,
//                            5);
//    if (result == RT_EOK)
//    {
//        rt_thread_startup(&wifi_thread);
//    }
    
    /* 创建esp8266信息获取线程 */
    gp_esp8266_info_get_tid = rt_thread_create("esp8266_info_get",
                                               esp8266_info_get_entry, 
                                               RT_NULL,
                                               512, 
                                               16, 
                                               5);
    if (gp_esp8266_info_get_tid != RT_NULL) {
        rt_thread_startup(gp_esp8266_info_get_tid);
    }
    
    /* 初始化rtc线程 */
    result = rt_thread_init(&rtc_thread,
                            "rtc",
                            rtc_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&rtc_stack[0],
                            sizeof(rtc_stack),
                            26,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&rtc_thread);
    }
        
    /* 退出临界区*/
    void rt_exit_critical(void); 
    
    while(1) {
        rt_thread_delay(1000);
    }
}

__asm void SystemReset(void)
{
    MOV R0, #1           //; 
    MSR FAULTMASK, R0    //; 清除FAULTMASK 禁止一切中断产生
    LDR R0, =0xE000ED0C  //;
    LDR R1, =0x05FA0004  //; 
    STR R1, [R0]         //; 系统软件复位   

deadloop
    B deadloop        //; 死循环使程序运行不到下面的代码
}
FINSH_FUNCTION_EXPORT(SystemReset, SystemReset)
