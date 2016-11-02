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

#include <rfid_thread.h>
#include <user_check_thread.h>
#include <wifi_thread.h>
#include "rtc_thread.h"

#include "p_database.h"
#include "device_config.h"
#include "esp8266.h"
#include "beep_door.h"

#ifdef  TOUCH_SCREEN
#include "touch_screen.h"
#endif  /* TOUCH_SCREEN */

#ifdef  TFT
#include "ILI93xx.h"
#endif  /* TFT */

#ifdef  STemWin
#include "GUI.h"
#include "WM.h"
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

#ifdef  EmWin_Demo
static rt_uint8_t emwin_demo_stack[ 8192 ];	//线程栈
static struct rt_thread emwin_demo_thread; 	//线程控制块
#endif  /* EmWin_Demo */

#ifdef  TOUCH_SCREEN
static rt_uint8_t touch_screen_stack[ 2048 ];	//线程栈
static struct rt_thread touch_screen_thread; 	//线程控制块
#endif  /* TOUCH_SCREEN */

#define rt_thread_delayMs(x) rt_thread_delay(rt_tick_from_millisecond(x))

/*******************************************************************************
* 函数名 	: touch_screen_thread_entry
* 描述   	: 触摸屏扫描线程
* 输入     	: - parameter: 线程入口参数
* 输出     	: None
* 返回值    : None
*******************************************************************************/
#ifdef  TOUCH_SCREEN
static void touch_screen_thread_entry(void* parameter)
{	
	while(1)
	{
		#ifdef  STemWin
		GUI_TOUCH_Exec();
		#endif  /* STemWin */
		rt_thread_delayMs(5);
	}	
}
#endif  /* TOUCH_SCREEN */

/*******************************************************************************
* 函数名 	: emwin_demo_thread_entry
* 描述   	: emwin_demo线程
* 输入     	: - parameter: 线程入口参数
* 输出     	: None
* 返回值    : None
*******************************************************************************/
#ifdef  EmWin_Demo
static void emwin_demo_thread_entry(void* parameter)
{	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//开启CRC时钟
	WM_SetCreateFlags(WM_CF_MEMDEV);
	GUI_Init();  			//STemWin初始化
	GUI_DispString("hello world");
	while(1)
	{
		#ifdef  EmWin_Demo
		#include "GUIDEMO.h"
		GUIDEMO_Main();	
		#endif  /* EmWin_Demo */
		rt_thread_delayMs(5);
	}	
}
#endif  /* EmWin_Demo */

/*******************************************************************************
* 函数名 	: user_init_thread_entry
* 描述   	: 用户初始化进程
* 输入     	: - parameter: 线程入口参数
* 输出     	: None
* 返回值    	: None
*******************************************************************************/
void user_init_thread_entry(void* parameter)
{
	rt_err_t result;

	void rt_enter_critical(void); /* 进入临界区*/
		
	rt_hw_beep_door_init();
	/* 初始化设备地址 */
	get_set_device_addr(&device_addr,GET_DEVICE);	
	rt_kprintf("device addr is %d\r\n",device_addr);
	/* 初始化设备名称 */
//	rt_memset(this_device_name, 0, sizeof(this_device_name));
//	sprintf(this_device_name,"411");
//	get_set_device_name(this_device_name,SET_DEVICE);	
	get_set_device_name(this_device_name,GET_DEVICE);	
	/* 初始化卡号数组 */
	if (init_card_array(card_id_array) == -1)
	{
		rt_kprintf("init card array failed\r\n");
	}
	/* 初始化事件对象 */
	rt_event_init(&user_check_event, "user_check_event", RT_IPC_FLAG_FIFO);	

    /* 初始化SPI总线 */
	rt_hw_stm32_spi_bus_init();
#ifdef  TFT
	/* 初始化TFT液晶 */
	TFTLCD_Init();
	POINT_COLOR = WHITE;
	BACK_COLOR = BLACK;
	LCD_ShowString(0,0,320,16,16,(u8 *)"WiFi Access Control System");
#endif  /* TFT */

#ifdef  TOUCH_SCREEN
	/* 初始化触摸屏 */
	TP_Init();
#endif  /* TOUCH_SCREEN */
		
#ifdef  EmWin_Demo
    /* 初始化emwin demo线程 */
    result = rt_thread_init(&emwin_demo_thread,
                            "emwin_demo",
                            emwin_demo_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&emwin_demo_stack[0],
                            sizeof(emwin_demo_stack),
                            15,
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
                            18,
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
    result = rt_thread_init(&wifi_thread,
                            "wifi",
                            wifi_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&wifi_stack[0],
                            sizeof(wifi_stack),
                            19,
                            5);
    if (result == RT_EOK)
    {
        //rt_thread_startup(&wifi_thread);
    }
	
	void rt_exit_critical(void); /* 退出临界区*/
	
    /* 初始化rtc线程 */
    result = rt_thread_init(&rtc_thread,
                            "rtc",
                            rtc_thread_entry,
                            RT_NULL,
                            (rt_uint8_t*)&rtc_stack[0],
                            sizeof(rtc_stack),
                            5,
                            5);
    if (result == RT_EOK)
    {
        rt_thread_startup(&rtc_thread);
    }
	
	void rt_exit_critical(void); /* 退出临界区*/
}
