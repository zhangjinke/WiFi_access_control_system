/******************************************************************************
* @ File name --> ds1307.c
* @ Author    --> By@ Sam Chan
* @ Version   --> V1.0
* @ Date      --> 11 - 27 - 2013
* @ Brief     --> 时钟芯片DS1307驱动
* @           --> 本驱动函数兼容DS1307、DS1338、DS1338Z芯片
*
* @ Copyright (C) 20**
* @ All rights reserved
*******************************************************************************
*
*                                  File Update
* @ Version   --> V1.
* @ Author    --> By@
* @ Date      --> 
* @ Revise    --> 
*
******************************************************************************/

#include "ds1307.h"

/******************************************************************************
                                    定义变量
******************************************************************************/

Time_Typedef TimeValue;  //定义时间缓存指针
uint8_t Time_Buffer[8];    //时间日历数据缓存

/******************************************************************************
* Function Name --> DS1307某寄存器写入一个字节数据
* Description   --> none
* Input         --> REG_ADD：要操作寄存器地址
*                   dat：要写入的数据
* Output        --> none
* Reaturn       --> none 
******************************************************************************/
void DS1307_Write_Byte(uint8_t REG_ADD,uint8_t dat)
{
    IIC_Start();
    if(!(IIC_Write_Byte(DS1307_Write))) //发送写命令并检查应答位
    {
        IIC_Write_Byte(REG_ADD);
        IIC_Write_Byte(dat);
    }
    IIC_Stop();
}
/******************************************************************************
* Function Name --> DS1307某寄存器读取一个字节数据
* Description   --> none
* Input         --> REG_ADD：要操作寄存器地址
* Output        --> none
* Reaturn       --> 读取到的数值
******************************************************************************/
uint8_t DS1307_Read_Byte(uint8_t REG_ADD)
{
    uint8_t rcv;

    IIC_Start();
    if(!(IIC_Write_Byte(DS1307_Write))) //发送写命令并检查应答位
    {
        IIC_Write_Byte(REG_ADD);    //发送要操作的寄存器地址
        IIC_Start();    //重启总线
        IIC_Write_Byte(DS1307_Read);    //发送读取命令
        rcv = IIC_Read_Byte();
        IIC_Ack(0x01); //发送非应答信号
    }
    IIC_Stop();
    return rcv;
}
/******************************************************************************
* Function Name --> DS1307对时间日历寄存器操作，写入数据或者读取数据
* Description   --> 连续写入n字节或者连续读取n字节数据
* Input         --> REG_ADD：要操作寄存器起始地址
*                   *WBuff：写入数据缓存
*                   num：写入数据数量
*                   mode：操作模式。0：写入数据操作。1：读取数据操作
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS1307_Operate_Register(uint8_t REG_ADD,uint8_t *pBuff,uint8_t num,uint8_t mode)
{
    uint8_t i;
    if(mode)    //读取数据
    {
        IIC_Start();
        if(!(IIC_Write_Byte(DS1307_Write)))    //发送写命令并检查应答位
        {
            IIC_Write_Byte(REG_ADD);    //定位起始寄存器地址
            IIC_Start();    //重启总线
            IIC_Write_Byte(DS1307_Read);    //发送读取命令
            for(i = 0;i < num;i++)
            {
                *pBuff = IIC_Read_Byte();    //读取数据
                if(i == (num - 1))    IIC_Ack(0x01);    //发送非应答信号
                else IIC_Ack(0x00);    //发送应答信号
                pBuff++;
            }
        }
        IIC_Stop();    
    }
    else    //写入数据
    {             
        IIC_Start();
        if(!(IIC_Write_Byte(DS1307_Write)))    //发送写命令并检查应答位
        {
            IIC_Write_Byte(REG_ADD);    //定位起始寄存器地址
            for(i = 0;i < num;i++)
            {
                IIC_Write_Byte(*pBuff);    //写入数据
                pBuff++;
            }
        }
        IIC_Stop();
    }
}
/******************************************************************************
* Function Name --> DS1307读取或者写入时间信息
* Description   --> 连续写入n字节或者连续读取n字节数据
* Input         --> *pBuff：写入数据缓存
*                   mode：操作模式。0：写入数据操作。1：读取数据操作
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS1307_ReadWrite_Time(uint8_t mode)
{
    uint8_t Time_Register[8];    //定义时间缓存
    
    if(mode)    //读取时间信息
    {
        DS1307_Operate_Register(Address_second,Time_Register,7,1);    //从秒地址（0x00）开始读取时间日历数据
        
        /******将数据复制到时间结构体中，方便后面程序调用******/
        TimeValue.second =     (Time_Register[0] & 0x0f) + ((Time_Register[0] & 0x70)>>4)*10;    //秒数据
        TimeValue.minute =     (Time_Register[1] & 0x0f) + ((Time_Register[1] & 0x70)>>4)*10;    //分钟数据
        TimeValue.hour =     (Time_Register[2] & 0x0f) + ((Time_Register[2] & 0x30)>>4)*10;        //小时数据
        TimeValue.week =     Time_Register[3] & Shield_weekBit;                                    //星期数据
        TimeValue.date =     (Time_Register[4] & 0x0f) + ((Time_Register[4] & 0x30)>>4)*10;        //日数据
        TimeValue.month =     (Time_Register[5] & 0x0f) + ((Time_Register[5] & 0x10)>>4)*10;    //月数据
        TimeValue.year =     (Time_Register[6] & 0x0f) + ((Time_Register[6] & 0xf0)>>4)*10;        //年数据
    }
    else
    {
        /******从时间结构体中复制数据进来******/
        Time_Register[0] = TimeValue.second | Control_Chip_Run;    //秒，启动芯片
        Time_Register[1] = TimeValue.minute;    //分钟
        Time_Register[2] = TimeValue.hour | Hour_Mode24;    //小时，24小时制
        Time_Register[3] = TimeValue.week;    //星期
        Time_Register[4] = TimeValue.date;    //日        
        Time_Register[5] = TimeValue.month;    //月
        Time_Register[6] = TimeValue.year;    //年
        
        DS1307_Operate_Register(Address_second,Time_Register,7,0);    //从秒地址（0x00）开始写入时间日历数据
    }
}
/******************************************************************************
* Function Name --> DS1307测试好坏
* Description   --> 在DS1307芯片的RAM的最后一个地址写入一个数据并读出来判断
*                   与上次写入的值相等，不是第一次上电，否则则初始化时间
* Input         --> none
* Output        --> none
* Reaturn       --> 0：设备正常并不是第一次上电
*                   1：设备错误或者已损坏
******************************************************************************/    
uint8_t DS1307_Check(void)
{
    if(DS1307_Read_Byte(RAM_Address55) == test_data)    return 0;    //设备正常，不是第一次上电
    else    return 1;
}
/******************************************************************************
* Function Name --> DS1307内置的RAM写数据操作
* Description   --> none
* Input         --> *pBuff：写数据存放区
*                   WRadd：读写起始地址，范围在RAM_Address0 ~ RAM_Address55之间，最后一位地址有其他用途
*                   num：读写字节数据的数量，范围在1 ~ 55之间
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS1307_RAM_Write_Data(uint8_t* pBuff,uint8_t WRadd,uint8_t num)
{
    uint8_t i;
    uint8_t ADDremain;   //写入数据组数
    
    /******判断写入数据的起始地址范围******/

    if(WRadd >= RAM_Address55)  return; //最后一个RAM单元操作，直接退出

    /******判断发送数据的组数目******/

    if((WRadd + num) >= (RAM_Address55 - 1))    ADDremain = RAM_Address55 - 1 - WRadd;  //超出范围，写入余下的空间
    else    ADDremain = num;    //没超出空间，直接写完

    IIC_Start();
    if(!(IIC_Write_Byte(DS1307_Write))) //发送写命令并检查应答信号
    {
        IIC_Write_Byte(WRadd);  //发送写入数据首地址
        for(i = 0;i < ADDremain;i++)
        {
            IIC_Write_Byte(pBuff[i]);   //写入数据
        }
    }
    IIC_Stop();
}
/******************************************************************************
* Function Name --> DS1307内置的RAM读数据操作
* Description   --> none
* Input         --> WRadd：读写起始地址，范围在RAM_Address0 ~ RAM_Address55之间，最后一位地址有其他用途
*                   num：读写字节数据的数量，范围在1 ~ 55之间
* Output        --> *pBuff：读数据存放区
* Reaturn       --> none
******************************************************************************/
void DS1307_RAM_Read_Data(uint8_t* pBuff,uint8_t WRadd,uint8_t num)
{
    uint8_t i;
    uint8_t ADDremain;

    /******判断读取数据的起始地址范围******/

    if(WRadd >= RAM_Address55)  return; //最后一个RAM单元操作，直接退出

    /******最后一个地址被用作检测DS1307来用，所以不读最后一个地址数据******/

    if((WRadd + num) >= RAM_Address55)  ADDremain = RAM_Address55 - 1 - WRadd;  //超出范围了，读取起始地址到倒数第二个地址空间的数据
    else    ADDremain = num;    //没超出地址范围，全部读取完

    IIC_Start();
    if(!(IIC_Write_Byte(DS1307_Write))) //发送写命令并检查应答信号
    {
        IIC_Write_Byte(WRadd);  //发送读取数据开始寄存器地址
        IIC_Start();
        if(!(IIC_Write_Byte(DS1307_Read)))  //发送读取命令并检查应答信号
        {
            for(i = 0;i < ADDremain;i++)
            {
                pBuff[i] = IIC_Read_Byte(); //开始接收num组数据
                if(i == (ADDremain - 1))    IIC_Ack(0x01); //读取完最后一组数据，发送非应答信号
                else    IIC_Ack(0x00);  //发送应答信号
            }
        }
    }
    IIC_Stop();
}
/******************************************************************************
* Function Name --> 时间日历初始化
* Description   --> none
* Input         --> *TimeVAL：RTC芯片寄存器值指针
* Output        --> none
* Reaturn       --> none
******************************************************************************/
void DS1307_Time_Init(Time_Typedef *TimeVAL)
{    
    //时间日历数据
    Time_Buffer[0] = ((((TimeVAL->second%10)&0x0f) | ((TimeVAL->second/10)&0x0f)<<4)) &~(1<<7);    //启动RTC芯片
    Time_Buffer[1] = (((TimeVAL->minute%10)&0x0f) | ((TimeVAL->minute/10)&0x0f)<<4);
    Time_Buffer[2] = (((TimeVAL->hour%10)&0x0f) | ((TimeVAL->hour/10)&0x03)<<4);
    Time_Buffer[3] = TimeVAL->week&0x07;
    Time_Buffer[4] = (((TimeVAL->date%10)&0x0f) | ((TimeVAL->date/10)&0x03)<<4);
    Time_Buffer[5] = (((TimeVAL->month%10)&0x0f) | ((TimeVAL->month/10)&0x01)<<4);
    Time_Buffer[6] = (((TimeVAL->year%10)&0x0f) | ((TimeVAL->year/10)&0x0f)<<4);
    //频率输出设置

    #ifdef  Chip_Type   //如果定义了，则使用的是DS1307芯片
    
        Time_Buffer[7] = TimeVAL->SQWE;    //频率输出控制

    #else   //没定义，则使用的是DS1338或者DS1338Z芯片

        Time_Buffer[7] = TimeVAL->SQWE | OSF_Enable;    //频率输出控制

    #endif

//    DS1307_Write_Byte(Address_second, Control_Chip_Run);    //先启动芯片
    
    DS1307_Operate_Register(Address_second,Time_Buffer,8,0);    //从秒寄存器（0x00）开始写入8组数据

    DS1307_Write_Byte(RAM_Address55, test_data);    //向最后一个RAM地址写入识别值
}

