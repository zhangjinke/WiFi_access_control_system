/**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: pwm.c
**
** 创   建   人: 张进科
**
** 文件创建日期: 2016 年 09 月 29 日
**
** 描        述: 液晶背光PWM驱动

** 日志:
2016.09.08  创建本文件
*********************************************************************************************************/

#include "pwm.h"
#include "finsh.h"

/*******************************************************************************
* 函数名 	: set_lcd_led
* 描述   	: 设置LCD背光灯亮度
* 输入     	: - device: 0门外 1门内 - permillage: 亮度(0-1000)
* 输出     	: None
* 返回值    	: None
*******************************************************************************/
void set_lcd_led(u8 device, u16 permillage)
{
	switch(device)
	{
		case 0:
		{
			TIM_SetCompare2(TIM3,permillage*14400/1000);
		}	break;
		case 1:
		{
			TIM_SetCompare1(TIM3,permillage*14400/1000);		   
		}	break;
		default: rt_kprintf("设备选择错误 device is %d , 0: 门外 1: 门内",device);
	}
}
FINSH_FUNCTION_EXPORT(set_lcd_led, set lcd led)

/*******************************************************************************
* 函数名 	: rt_hw_lcd_led_init
* 描述   	: 初始化LCD背光灯
* 输入     	: - arr: 自动重装值 - psc: 时钟预分频数
* 输出     	: None
* 返回值    	: None
*******************************************************************************/
void rt_hw_lcd_led_init(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);	//使能定时器3时钟
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC  | RCC_APB2Periph_AFIO, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟
	
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE); //Timer3完全重映射
 
   //设置该引脚为复用输出功能,输出TIM3的PWM脉冲波形
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7; //TIM3_CH1、TIM_CH2 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIO
 
   //初始化TIM3
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

	set_lcd_led(0, 0);	//关闭门外液晶背光
	set_lcd_led(1, 0);	//关闭门内液晶背光
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR1上的预装载寄存器
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器
	
	//初始化TIM3 Channel PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择定时器模式:TIM脉冲宽度调制模式2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性:TIM输出比较极性低
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC1
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC2
	
	TIM_Cmd(TIM3, ENABLE);  //使能TIM3
}
