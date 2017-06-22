#include "global.h"
#include "rc522.h"
#include "string.h"            //字符串操作
#include "spi_bus.h"
#include <drivers/spi.h>
#include <rtthread.h>

/*******************************
*SPI1连线说明：         门内
*1--SS  <----->PA4            
*2--SCK <----->PA5
*3--MOSI<----->PA7
*4--MISO<----->PA6
*5--悬空
*6--GND <----->GND
*7--RST <----->PC4
*8--VCC <----->VCC·
************************************/
/*******************************
*SPI3连线说明：         门外
*1--SS  <----->PG15            
*2--SCK <----->PB3
*3--MOSI<----->PB5
*4--MISO<----->PB4
*5--悬空
*6--GND <----->GND
*7--RST <----->PB6
*8--VCC <----->VCC·
************************************/

/*全局变量*/
unsigned char CT[2];//卡类型
unsigned char get_card_id_array[4];//卡ID数组

struct rt_spi_device *rt_spi_rc522_device;//RC522设备(当前操作的设备)
struct rt_spi_device *rt_spi_rc522_in_device;//RC522设备(门内)
struct rt_spi_device *rt_spi_rc522_out_device;//RC522设备(门外)
struct rt_spi_message rc522_message;    //SPI设备通信用消息结构体

uint8_t rc522_readBuf[2],rc522_writeBuf[2];    //SPI通信缓存

#define delay_ns(ms) rt_thread_delay(rt_tick_from_millisecond(1))
#define delay_ms(ms) rt_thread_delay(rt_tick_from_millisecond(ms))

void rc522_attach_device()
{
    static struct rt_spi_device rc522_in_spi_device;
    static struct stm32_spi_cs  rc522_in_spi_cs;
    static struct rt_spi_device rc522_out_spi_device;
    static struct stm32_spi_cs  rc522_out_spi_cs;
    GPIO_InitTypeDef  GPIO_InitStructure;
    
     //使能PA,PB,PG端口时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOG, ENABLE);
    
    //配置RC522 RST引脚
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8;//RST_IN
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;     //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    //IO口速度为50MHz
     GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化
    
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6;//RST_OUT
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;     //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    //IO口速度为50MHz
     GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化
    
    //配置RC522_IN_CS引脚
    rc522_in_spi_cs.GPIOx = GPIOB;
    rc522_in_spi_cs.GPIO_Pin = GPIO_Pin_12;

    GPIO_InitStructure.GPIO_Pin = rc522_in_spi_cs.GPIO_Pin;//rc522_in_cs
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;     //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    //IO口速度为50MHz
    GPIO_Init(rc522_in_spi_cs.GPIOx, &GPIO_InitStructure);        //根据设定参数初始化
    GPIO_SetBits(rc522_in_spi_cs.GPIOx,rc522_in_spi_cs.GPIO_Pin);//输出高
    //附着门内RFID设备到SPI1总线
    rt_spi_bus_attach_device(&rc522_in_spi_device, "RFID_IN", "SPI2", (void*)&rc522_in_spi_cs);
    
    //配置RC522_OUT_CS引脚
    rc522_out_spi_cs.GPIOx = GPIOA;
    rc522_out_spi_cs.GPIO_Pin = GPIO_Pin_15;

    GPIO_InitStructure.GPIO_Pin = rc522_out_spi_cs.GPIO_Pin;//rc522_out_cs
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;     //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    //IO口速度为50MHz
    GPIO_Init(rc522_out_spi_cs.GPIOx, &GPIO_InitStructure);        //根据设定参数初始化
    GPIO_SetBits(rc522_out_spi_cs.GPIOx,rc522_out_spi_cs.GPIO_Pin);//输出高
    //附着门外RFID设备到SPI1总线
    rt_spi_bus_attach_device(&rc522_out_spi_device, "RFID_OUT", "SPI2", (void*)&rc522_out_spi_cs);
}

int8_t InitRC522(void)
{
    /* 在SPI2_BUS上附着RC522_IN设备 */
    rc522_attach_device();
    /* 配置门内RFID设备 */
    rt_spi_rc522_in_device = (struct rt_spi_device *) rt_device_find("RFID_IN");//查找RFID_IN设备
    if (rt_spi_rc522_in_device == RT_NULL)
    {
        return -1;
    }

    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
        cfg.max_hz = 10 * 1000 * 1000;
        rt_spi_configure(rt_spi_rc522_in_device, &cfg);
    }
    
    SET_RC522_IN_RST;
    delay_ns(10);
    CLR_RC522_IN_RST;
    delay_ns(10);
    SET_RC522_IN_RST;
    delay_ns(10);

    rt_spi_rc522_device = rt_spi_rc522_in_device;//设置当前RFID设备为门内设备
    PcdReset();             
    PcdAntennaOff();
    delay_ms(2);  
    PcdAntennaOn();
    M500PcdConfigISOType( 'A' );
    /* 配置门外RFID设备 */
    rt_spi_rc522_out_device = (struct rt_spi_device *) rt_device_find("RFID_OUT");//查找RFID_IN设备
    if (rt_spi_rc522_out_device == RT_NULL)
    {
        return -1;
    }

    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
        cfg.max_hz = 10 * 1000 * 1000;
        rt_spi_configure(rt_spi_rc522_out_device, &cfg);
    }

    SET_RC522_OUT_RST;
    delay_ns(10);
    CLR_RC522_OUT_RST;
    delay_ns(10);
    SET_RC522_OUT_RST;
    delay_ns(10);

    rt_spi_rc522_device = rt_spi_rc522_out_device;//设置当前RFID设备为门外设备
    PcdReset();             
    PcdAntennaOff();
    delay_ms(2);  
    PcdAntennaOn();
    M500PcdConfigISOType( 'A' );
    
    return 0;
}
void Reset_RC522(void)
{
  PcdReset();
  PcdAntennaOff();
  delay_ms(2);  
  PcdAntennaOn();
}                         
/////////////////////////////////////////////////////////////////////
//功    能：寻卡
//参数说明: req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//                   pTagType[OUT]：卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(uint8_t req_code,uint8_t *pTagType)
{
    char   status;  
    uint8_t   unLen;
    uint8_t   ucComMF522Buf[MAXRLEN]; 

    ClearBitMask(Status2Reg,0x08);
    WriteRawRC(BitFramingReg,0x07);
    SetBitMask(TxControlReg,0x03);
 
    ucComMF522Buf[0] = req_code;

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

    if ((status == MI_OK) && (unLen == 0x10))
    {    
        *pTagType     = ucComMF522Buf[0];
        *(pTagType+1) = ucComMF522Buf[1];
    }
    else
    {   status = MI_ERR;   }
   
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：防冲撞
//参数说明: pSnr[OUT]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(uint8_t *pSnr)
{
    char   status;
    uint8_t   i,snr_check=0;
    uint8_t   unLen;
    uint8_t   ucComMF522Buf[MAXRLEN]; 
    

    ClearBitMask(Status2Reg,0x08);
    WriteRawRC(BitFramingReg,0x00);
    ClearBitMask(CollReg,0x80);
 
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

    if (status == MI_OK)
    {
         for (i=0; i<4; i++)
         {   
             *(pSnr+i)  = ucComMF522Buf[i];
             snr_check ^= ucComMF522Buf[i];
         }
         if (snr_check != ucComMF522Buf[i])
         {   status = MI_ERR;    }
    }
    
    SetBitMask(CollReg,0x80);
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：选定卡片
//参数说明: pSnr[IN]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(uint8_t *pSnr)
{
    char   status;
    uint8_t   i;
    uint8_t   unLen;
    uint8_t   ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
        ucComMF522Buf[i+2] = *(pSnr+i);
        ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
    ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   status = MI_OK;  }
    else
    {   status = MI_ERR;    }

    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：验证卡片密码
//参数说明: auth_mode[IN]: 密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥 
//          addr[IN]：块地址
//          pKey[IN]：密码
//          pSnr[IN]：卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////               
char PcdAuthState(uint8_t   auth_mode,uint8_t   addr,uint8_t *pKey,uint8_t *pSnr)
{
    char   status;
    uint8_t   unLen;
    uint8_t   ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
//    for (i=0; i<6; i++)
//    {    ucComMF522Buf[i+2] = *(pKey+i);   }
//    for (i=0; i<6; i++)
//    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
    memcpy(&ucComMF522Buf[2], pKey, 6); 
    memcpy(&ucComMF522Buf[8], pSnr, 4); 
    
    status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {   status = MI_ERR;   }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：读取M1卡一块数据
//参数说明: addr[IN]：块地址
//          p [OUT]：读出的数据，16字节
//返    回: 成功返回MI_OK
///////////////////////////////////////////////////////////////////// 
char PcdRead(uint8_t   addr,uint8_t *p )
{
    char   status;
    uint8_t   unLen;
    uint8_t   i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
   
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if ((status == MI_OK) && (unLen == 0x90))
 //   {   memcpy(p , ucComMF522Buf, 16);   }
    {
        for (i=0; i<16; i++)
        {    *(p +i) = ucComMF522Buf[i];   }
    }
    else
    {   status = MI_ERR;   }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：写数据到M1卡一块
//参数说明: addr[IN]：块地址
//          p [IN]：写入的数据，16字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(uint8_t   addr,uint8_t *p )
{
    char   status;
    uint8_t   unLen;
    uint8_t   i,ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
        //memcpy(ucComMF522Buf, p , 16);
        for (i=0; i<16; i++)
        {    
            ucComMF522Buf[i] = *(p +i);   
        }
        CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
    uint8_t   status;
    uint8_t   unLen;
    uint8_t   ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    status = status;//消除警告
    return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
void CalulateCRC(uint8_t *pIn ,uint8_t   len,uint8_t *pOut )
{
    uint8_t   i,n;
    ClearBitMask(DivIrqReg,0x04);
    WriteRawRC(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);
    for (i=0; i<len; i++)
    {   WriteRawRC(FIFODataReg, *(pIn +i));   }
    WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do 
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));
    pOut [0] = ReadRawRC(CRCResultRegL);
    pOut [1] = ReadRawRC(CRCResultRegM);
}

/////////////////////////////////////////////////////////////////////
//功    能：复位RC522
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{
    WriteRawRC(CommandReg,PCD_RESETPHASE);
    WriteRawRC(CommandReg,PCD_RESETPHASE);
    delay_ns(10);
    
    WriteRawRC(ModeReg,0x3D);            //和Mifare卡通讯，CRC初始值0x6363
    WriteRawRC(TReloadRegL,30);           
    WriteRawRC(TReloadRegH,0);
    WriteRawRC(TModeReg,0x8D);
    WriteRawRC(TPrescalerReg,0x3E);
    
    WriteRawRC(TxAutoReg,0x40);//必须要
   
    return MI_OK;
}
//////////////////////////////////////////////////////////////////////
//设置RC632的工作方式 
//////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(uint8_t   type)
{
   if (type == 'A')                     //ISO14443_A
   { 
       ClearBitMask(Status2Reg,0x08);
       WriteRawRC(ModeReg,0x3D);//3F
       WriteRawRC(RxSelReg,0x86);//84
       WriteRawRC(RFCfgReg,0x7F);   //4F
          WriteRawRC(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
       WriteRawRC(TReloadRegH,0);
       WriteRawRC(TModeReg,0x8D);
       WriteRawRC(TPrescalerReg,0x3E);
       delay_ns(1000);
       PcdAntennaOn();
   }
   else{ return 1; }
   
   return MI_OK;
}
/////////////////////////////////////////////////////////////////////
//功    能：读RC632寄存器
//参数说明：Address[IN]:寄存器地址
//返    回：读出的值
/////////////////////////////////////////////////////////////////////
uint8_t ReadRawRC(uint8_t Address)
{
    rc522_writeBuf[0] = ((Address<<1)&0x7E)|0x80;

    rc522_message.send_buf = rc522_writeBuf;
    rc522_message.recv_buf = rc522_readBuf;    //设置读写缓存
    rc522_message.length = 1;            //设置读写长度ReadID
    rc522_message.cs_take = 1;            //开始通信时拉低CS
    rc522_message.cs_release = 0;        //结束通信时不拉高CS
    rc522_message.next = RT_NULL;
    rt_spi_transfer_message(rt_spi_rc522_device, &rc522_message);//进行一次数据传输

    rc522_writeBuf[0] = 0x00;
    rc522_message.cs_release = 1;        //结束通信时拉高CS
    rt_spi_transfer_message(rt_spi_rc522_device, &rc522_message);//进行一次数据传输
    
    return rc522_readBuf[0];
}

/////////////////////////////////////////////////////////////////////
//功    能：写RC632寄存器
//参数说明：Address[IN]:寄存器地址
//          value[IN]:写入的值
/////////////////////////////////////////////////////////////////////
void WriteRawRC(uint8_t Address, uint8_t value)
{  
    rc522_writeBuf[0] = ((Address<<1)&0x7E);
    rc522_writeBuf[1] = value;

    rc522_message.send_buf = rc522_writeBuf;
    rc522_message.recv_buf = rc522_readBuf;    //设置读写缓存
    rc522_message.length = 2;            //设置读写长度
    rc522_message.cs_take = 1;            //开始通信时拉低CS
    rc522_message.cs_release = 1;        //结束通信时拉高CS
    rc522_message.next = RT_NULL;
    rt_spi_transfer_message(rt_spi_rc522_device, &rc522_message);//进行一次数据传输
}
/////////////////////////////////////////////////////////////////////
//功    能：置RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:置位值
/////////////////////////////////////////////////////////////////////
void SetBitMask(uint8_t   reg,uint8_t   mask)  
{
    char   tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg,tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//功    能：清RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:清位值
/////////////////////////////////////////////////////////////////////
void ClearBitMask(uint8_t   reg,uint8_t   mask)  
{
    char   tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 

/////////////////////////////////////////////////////////////////////
//功    能：通过RC522和ISO14443卡通讯
//参数说明：Command[IN]:RC522命令字
//          pIn [IN]:通过RC522发送到卡片的数据
//          InLenByte[IN]:发送数据的字节长度
//          pOut [OUT]:接收到的卡片返回数据
//          *pOutLenBit[OUT]:返回数据的位长度
/////////////////////////////////////////////////////////////////////
char PcdComMF522(uint8_t   Command, 
                 uint8_t *pIn , 
                 uint8_t   InLenByte,
                 uint8_t *pOut , 
                 uint8_t *pOutLenBit)
{
    char   status = MI_ERR;
    uint8_t   irqEn   = 0x00;
    uint8_t   waitFor = 0x00;
    uint8_t   lastBits;
    uint8_t   n;
    uint16_t   i;
    switch (Command)
    {
        case PCD_AUTHENT:
            irqEn   = 0x12;
            waitFor = 0x10;
            break;
        case PCD_TRANSCEIVE:
            irqEn   = 0x77;
            waitFor = 0x30;
            break;
        default:
            break;
    }
   
    WriteRawRC(ComIEnReg,irqEn|0x80);
    ClearBitMask(ComIrqReg,0x80);    //清所有中断位
    WriteRawRC(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);         //清FIFO缓存
    
    for (i=0; i<InLenByte; i++)
    {   WriteRawRC(FIFODataReg, pIn [i]);    }
    WriteRawRC(CommandReg, Command);      
    
    if (Command == PCD_TRANSCEIVE)
    {    SetBitMask(BitFramingReg,0x80);  }     //开始传送
                                             
    i = 256;    //根据时钟频率调整，操作M1卡最大等待时间25ms
    do
    {
        n = ReadRawRC(ComIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitFor));
    ClearBitMask(BitFramingReg,0x80);

    if (i!=0)
    {    
        if(!(ReadRawRC(ErrorReg)&0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {   status = MI_NOTAGERR;   }
            if (Command == PCD_TRANSCEIVE)
            {
                   n = ReadRawRC(FIFOLevelReg);
                  lastBits = ReadRawRC(ControlReg) & 0x07;
                if (lastBits)
                {   *pOutLenBit = (n-1)*8 + lastBits;   }
                else
                {   *pOutLenBit = n*8;   }
                if (n == 0)
                {   n = 1;    }
                if (n > MAXRLEN)
                {   n = MAXRLEN;   }
                for (i=0; i<n; i++)
                {   pOut [i] = ReadRawRC(FIFODataReg);    }
            }
        }
        else
        {   status = MI_ERR;   }
        
    }

    SetBitMask(ControlReg,0x80);           // stop timer now
    WriteRawRC(CommandReg,PCD_IDLE); 
    return status;
}

/////////////////////////////////////////////////////////////////////
//开启天线  
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(void)
{
    uint8_t   i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
}


/////////////////////////////////////////////////////////////////////
//关闭天线
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

/////////////////////////////////////////////////////////////////////
//功    能：扣款和充值
//参数说明: dd_mode[IN]：命令字
//               0xC0 = 扣款
//               0xC1 = 充值
//          addr[IN]：钱包地址
//          pValue[IN]：4字节增(减)值，低位在前
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////                 
char PcdValue(uint8_t dd_mode,uint8_t addr,uint8_t *pValue)
{
    char status;
    uint8_t  unLen;
    uint8_t ucComMF522Buf[MAXRLEN]; 
    //uint8_t i;
    
    ucComMF522Buf[0] = dd_mode;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
        memcpy(ucComMF522Buf, pValue, 4);
        //for (i=0; i<16; i++)
        //{    ucComMF522Buf[i] = *(pValue+i);   }
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
        unLen = 0;
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
        if (status != MI_ERR)
        {    status = MI_OK;    }
    }
    
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = PICC_TRANSFER;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]); 
   
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：备份钱包
//参数说明: sourceaddr[IN]：源地址
//          goaladdr[IN]：目标地址
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdBakValue(uint8_t sourceaddr, uint8_t goaladdr)
{
    char status;
    uint8_t  unLen;
    uint8_t ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_RESTORE;
    ucComMF522Buf[1] = sourceaddr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
    
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = 0;
        ucComMF522Buf[1] = 0;
        ucComMF522Buf[2] = 0;
        ucComMF522Buf[3] = 0;
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
 
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
        if (status != MI_ERR)
        {    status = MI_OK;    }
    }
    
    if (status != MI_OK)
    {    return MI_ERR;   }
    
    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = goaladdr;

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }

    return status;
}

/*************************************
*函数功能：显示卡的卡号，以十六进制显示
*参数：x，y 坐标
*        p 卡号的地址
*        charcolor 字符的颜色
*        bkcolor   背景的颜色
***************************************/
void TurnID(uint8_t *p, char* card_id)     //转换卡的卡号，以十六进制显示
{
    unsigned char i;
    for(i=0;i<4;i++)
    {
        card_id[i*2]=p[i]>>4;
        card_id[i*2]>9?(card_id[i*2]+='7'):(card_id[i*2]+='0');
        card_id[i*2+1]=p[i]&0x0f;
        card_id[i*2+1]>9?(card_id[i*2+1]+='7'):(card_id[i*2+1]+='0');
    }
    card_id[8]=0;
}

int8_t ReadID(uint32_t* card_id)//读卡
{
    unsigned char status;

    status = PcdRequest(PICC_REQALL,CT);/*尋卡*/
    if(status==MI_OK)//尋卡成功
    {
        status = PcdAnticoll(get_card_id_array);/*防冲撞*/         
    }
    if (status==MI_OK)//防衝撞成功
    {        
        status=MI_ERR;
        /* 将获取到的卡号数组转换为32位整形 */
        *card_id =     get_card_id_array[3]+(get_card_id_array[2]<<8)+
                    (get_card_id_array[1]<<16)+(get_card_id_array[0]<<24);
        return 0;
    }
    return -1;
}
