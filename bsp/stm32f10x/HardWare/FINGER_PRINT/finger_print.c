#include "finger_print.h"


uint8_t u3receive_short_ok = 0;			//串口3接收8字节完成标志
uint8_t u3receive_long_ok = 0;			//串口3接收>8字节完成标志
uint16_t u3receive_len = 0;					//串口3     8字节接收计数器
uint8_t receive_short_ok = 0;				//串口2接收8字节完成标志
uint8_t receive_long_ok = 0;				//串口2接收>8字节完成标志
uint16_t receive_len = 0;						//串口2     8字节接收计数器
uint8_t i = 0;											//for循环用变量
Serialize_Pack receive_pack;
Show_Text show_text;
uint8_t receiveMode = 0;					//接收模式 0: 8字节  1: >8字节
uint8_t dataLen = 0;						//>8字节接收计数器
uint8_t receiveMore[200];				//>8字节接收缓存


//=========全局变量==========
uint8_t gTxBuf[207];				 		//串行发送缓存

uint8_t getUserCount(USART_TypeDef* USARTx);

void ResetFingerFlag(void)		//指纹模块标志位复位
{
	uint8_t i;	
	u3receive_short_ok = 0;			//串口3接收8字节完成标志
	u3receive_long_ok = 0;			//串口3接收>8字节完成标志
	u3receive_len = 0;						//串口3     8字节接收计数器
	receive_short_ok = 0;				//接收8字节完成标志
	receive_long_ok = 0;					//接收>8字节完成标志
	receive_len = 0;							//8字节接收计数器uint8_t receiveMode = 0;					//接收模式 0:8字节  1:>8字节
	receiveMode = 0;						//接收模式 0:8字节  1:>8字节
	dataLen = 0;								//>8字节接收计数器
	for(i=0;i<200;i++)
	{receiveMore[i]=0;	}					
	for(i=0;i<8;i++)
	{receive_pack.buf[i]=0;	}
}


void TxByte(USART_TypeDef* USARTx,uint8_t temp)
{
	while((USARTx->SR&0X40)==0);
	USARTx->DR = temp;
}

/******************串口发送子程序bit SendUART(U8 Scnt,U8 Rcnt,U8 Delay)******/
/*功能：向DSP发送数据********************************************************/
/*参数：Scnt发送字节数；Rcnt接收字节数； Delay延时等待数*********************/
/*返回值：TRUE 成功；FALSE 失败**********************************************/
uint8_t TxAndRsCmd(USART_TypeDef* USARTx,uint16_t Scnt, uint16_t Rcnt, uint8_t Delay)
{
	uint16_t  i,CheckSum;
	uint32_t RsTimeCnt;
	TxByte(USARTx,CMD_HEAD);		//标志头	 
	CheckSum = 0;
	for (i = 0; i < Scnt; i++)
	{
		TxByte(USARTx,gTxBuf[i]);		 
		CheckSum ^= gTxBuf[i];
	}	
	TxByte(USARTx,CheckSum);
	TxByte(USARTx,CMD_TAIL); 		//标志尾 
	for(i = 0;i < 8;i++) {receive_pack.buf[i] = 0;}	//清空接收缓存
	RsTimeCnt = Delay * 120000;
	
	if(USARTx==USART2)
	{
		receive_len = 0;
		receive_short_ok = 0;
		receive_long_ok = 0;
		while (receive_len < Rcnt && RsTimeCnt > 0)
		RsTimeCnt--;
		if (receive_len != Rcnt) return ACK_TIMEOUT;
	}
	else if(USARTx==USART3)
	{
		u3receive_len = 0;
		u3receive_short_ok = 0;
		u3receive_long_ok = 0;	
		while (u3receive_len < Rcnt && RsTimeCnt > 0)
		RsTimeCnt--;
		if (u3receive_len != Rcnt) return ACK_TIMEOUT;
	}
	if (receive_pack.pack.HEAD != CMD_HEAD) return ACK_FAIL;
	if (receive_pack.pack.TAIL != CMD_TAIL) return ACK_FAIL;
	if (receive_pack.pack.CMD != (gTxBuf[0])) return ACK_FAIL;

	return ACK_SUCCESS;
}	 

/*********************************************************************
*功    能：验证权限是否在合法范围
*入口参数：无
*出口参数：成功或失败
*时间：2015年6月27日 13:13:16
*********************************************************************/
uint8_t IsMasterUser(uint8_t UserID)
{
    if ((UserID == 1) || (UserID == 2) || (UserID == 3)) return TRUE;
			else  return FALSE;
}	 

/*********************************************************************
*功    能：使模块进入休眠状态（命令/应答均为 8 字节）
*入口参数：无
*出口参数：成功或失败
*时间：2015年6月26日 16:55:47
*********************************************************************/
uint8_t setLpMode(USART_TypeDef* USARTx)
{
  uint8_t m;
	
	gTxBuf[0] = CMD_LP_MODE;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
	  return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：设置/读取指纹添加模式（命令/应答均为 8 字节）
*入口参数：com:  0:设置新的添加模式 1：读取当前添加模式
					 mode: 0:允许重复  1:禁止重复
					读取的添加模式储存在gRsBuf[3]
*出口参数：成功或失败
*时间：2015年6月26日 16:51:01
*********************************************************************/
uint8_t setAndReadMode(USART_TypeDef* USARTx,uint8_t com,uint8_t mode)
{
  uint8_t m;	
	gTxBuf[0] = CMD_SET_READ_ADDMODE;
	gTxBuf[1] = 0;
	gTxBuf[2] = mode;
	gTxBuf[3] = com;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
	  return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：添加指纹（命令/应答均为 8 字节）
*入口参数：userNum:用户号[1:4095]
		  lim:权限[1:3]
*出口参数：成功或失败
*时间：2015年6月26日 21:30:43
*********************************************************************/
uint8_t addUser(USART_TypeDef* USARTx,uint16_t userNum,uint8_t lim)
{
	uint8_t m;
	uint16_t userCount;	
	getUserCount(USARTx);
	userCount = (receive_pack.pack.Q1<<8)+receive_pack.pack.Q2;
	if (userCount >= USER_MAX_CNT)
		return ACK_FULL;
	
	gTxBuf[0] = CMD_ADD_1;
	gTxBuf[1] = userNum >> 8;		//用户号高八位
	gTxBuf[2] = userNum & 0xFF;		//用户号低八位
	gTxBuf[3] = lim;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 200);
	
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
	    gTxBuf[0] = CMD_ADD_2;
			m = TxAndRsCmd(USARTx,5, 8, 200);
			if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
			{
					gTxBuf[0] = CMD_ADD_3;
					m = TxAndRsCmd(USARTx,5, 8, 200);
					if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
					{
							return ACK_SUCCESS;
					}
					else
						return ACK_FAIL;
			}
			else
				return ACK_FAIL;
	}
	else
		return ACK_FAIL;
}

/*********************************************************************
*功    能：删除指定用户（命令/应答均为 8 字节）
*入口参数：userNum:用户号[1:4095]
*出口参数：成功或失败
*时间：2015年6月27日 13:00:42
*********************************************************************/
uint8_t deleteOneUser(USART_TypeDef* USARTx,uint16_t userNum)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_DEL_ONE;
	gTxBuf[1] = userNum >> 8;		//用户号高八位
	gTxBuf[2] = userNum & 0xFF;		//用户号低八位
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
		return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：删除所有用户（命令/应答均为 8 字节）
*入口参数：无
*出口参数：成功或失败
*时间：2015年6月27日 13:03:02
*********************************************************************/
uint8_t deleteAllUser(USART_TypeDef* USARTx)
{
 	uint8_t m;
	
	gTxBuf[0] = CMD_DEL_ALL;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;
	
	m = TxAndRsCmd(USARTx,5, 8, 50);
	
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{	    
		return ACK_SUCCESS;
	}
	else
	{
		return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：取用户总数（命令/应答均为 8 字节）
*入口参数：无
*出口参数：成功或失败
		  用户数低八位储存在gRsBuf[3]，高八位储存在gRsBuf[2]
*时间：2015年6月27日 13:06:52
*********************************************************************/
uint8_t getUserCount(USART_TypeDef* USARTx)
{
	uint8_t m;
	gTxBuf[0] = CMD_USER_CNT;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{	    
		return ACK_SUCCESS;
	}
	else
	{
		return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：比对 1:1（命令/应答均为 8 字节）
*入口参数：userNum:用户号[1:4095]
*出口参数：成功或失败
*时间：2015年6月27日 13:11:24
*********************************************************************/
uint8_t matchOne(USART_TypeDef* USARTx,uint8_t userNum)
{
  uint8_t m;
	
	gTxBuf[0] = CMD_MATCH_ONE;
	gTxBuf[1] = userNum >> 8;		//用户号高八位
	gTxBuf[2] = userNum & 0xFF;		//用户号低八位
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{	    
		return ACK_SUCCESS;
	}
	else
	{
		return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：比对 1:N（命令/应答均为 8 字节）
*入口参数：无
*出口参数：状态
		  用户号低八位储存在gRsBuf[3]，高八位储存在gRsBuf[2]，权限储存在gRsBuf[4]
*时间：2015年6月27日 13:14:39
*********************************************************************/
uint8_t matchN(USART_TypeDef* USARTx)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_MATCH_N;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;
	
	m = TxAndRsCmd(USARTx,5, 8, 150);
	
	if ((m == ACK_SUCCESS) && (IsMasterUser(receive_pack.pack.Q3) == TRUE))
	{	
		return ACK_SUCCESS;
	}
	else if(receive_pack.pack.Q3 == ACK_NO_USER)
	{
		return ACK_NO_USER;
	}
	else if(receive_pack.pack.Q3 == ACK_TIMEOUT)
	{
		return ACK_TIMEOUT;
	}
	else
	{
		return ACK_GO_OUT;
	}
}


/*********************************************************************
*功    能：比对 1:N（命令/应答均为 8 字节）
*入口参数：无
*出口参数：状态
		  用户号低八位储存在gRsBuf[3]，高八位储存在gRsBuf[2]，权限储存在gRsBuf[4]
*时间：2015年6月27日 13:14:39
*********************************************************************/
void fastMatchN(USART_TypeDef* USARTx)
{
	TxByte(USARTx,CMD_HEAD);
	TxByte(USARTx,CMD_MATCH_N);
	TxByte(USARTx,0x00);
	TxByte(USARTx,0x00);
	TxByte(USARTx,0x00);
	TxByte(USARTx,0x00);
	TxByte(USARTx,0x0C);
	TxByte(USARTx,CMD_TAIL);
}

/*********************************************************************
*功    能：取用户权限（命令/应答均为 8 字节）
*入口参数：userNum：用户号
		  读取用户权限储存在gRsBuf[4]
*出口参数：成功或失败
*时间：2015年6月26日 16:51:01
*********************************************************************/
uint8_t getLim(USART_TypeDef* USARTx,uint16_t userNum)
{
  uint8_t m;
	
	gTxBuf[0] = CMD_GETLIM;
	gTxBuf[1] = userNum >> 8;
	gTxBuf[2] = userNum & 0xFF;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
	  return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：取 DSP 模块版本号（命令为 8 字节/应答>8 字节）
*入口参数：无
*出口参数：成功或失败
		  版本号储存在:gRsBuf[9]:gRsBuf[17]
*时间：2015年6月27日 17:00:25
*********************************************************************/
uint8_t getVer(USART_TypeDef* USARTx)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_GET_VER;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 20, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：设置/读取比对等级（命令/应答均为 8 字节）
*入口参数：cmd   0:设置新的比对等级 1:读取当前比对等级
		  level 0:新比对等级 1:当前比对等级
*出口参数：成功或失败
		  读取的比对等级储存在:gRsBuf[3]
*时间：2015年6月27日 16:59:58
*********************************************************************/
uint8_t setAndReadLevel(USART_TypeDef* USARTx,uint8_t cmd,uint8_t level)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_COM_LEV;
	gTxBuf[1] = 0;
	gTxBuf[2] = level;
	gTxBuf[3] = cmd;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：采集图像并提取特征值上传（命令为 8 字节/应答>8 字节）
*入口参数：无
*出口参数：成功或失败
		  读取的比对等级储存在:gRsBuf[12]:gRsBuf[204]
*时间：2015年6月27日 16:59:58
*********************************************************************/
uint8_t getEigen(USART_TypeDef* USARTx)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_GET_EIGEN;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 207, 100);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}


/*********************************************************************
*功    能：下传指纹特征值与 DSP 模块数据库指纹比对 1： 1（命令>8 字节/应答为 8 字节）
*入口参数：特征值
*出口参数：成功或失败
		  用户号高位储存在:gRsBuf[2]
		  用户号低位储存在:gRsBuf[3]
		  用户权限储存在:gRsBuf[4]
*时间：2015年9月6日 8:47:39
*********************************************************************/
uint8_t downloadAndMatchONE(USART_TypeDef* USARTx,uint16_t userNum,uint8_t *eigen)
{
	uint8_t m;
	uint16_t i;
	
	gTxBuf[0] = CMD_DOWN_MATCH_ONE;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0xC4;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	gTxBuf[5] = 0x86;	
	gTxBuf[6] = CMD_TAIL;	
	gTxBuf[7] = CMD_HEAD;	
	gTxBuf[8] = userNum>>8;
	gTxBuf[9] = userNum&0xFF;
	gTxBuf[10] = 0;	
	for(i = 11;i<=204;i++)
	{
		gTxBuf[i] = eigen[i-11];
	}
	m = TxAndRsCmd(USARTx,204, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}


/*********************************************************************
*功    能：下传指纹特征值与 DSP 模块数据库指纹比对 1： N（命令>8 字节/应答为 8 字节）
*入口参数：特征值
*出口参数：成功或失败
		  用户号高位储存在:gRsBuf[2]
		  用户号低位储存在:gRsBuf[3]
		  用户权限储存在:gRsBuf[4]
*时间：2015年6月29日 13:39:39
*********************************************************************/
uint8_t downloadAndMatchN(USART_TypeDef* USARTx,uint8_t *eigen)
{
	uint8_t m;
	uint16_t i;
	
	gTxBuf[0] = CMD_DOWN_MATCH_N;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0xC4;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	gTxBuf[5] = 0x87;	
	gTxBuf[6] = CMD_TAIL;	
	gTxBuf[7] = CMD_HEAD;	
	gTxBuf[8] = 0;
	gTxBuf[9] = 0;
	gTxBuf[10] = 0;	
	for(i = 11;i<=204;i++)
	{
		gTxBuf[i] = eigen[i-11];
	}
	m = TxAndRsCmd(USARTx,204, 8, 10);
		
	if (m == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else if(receive_pack.pack.Q3 == ACK_NO_USER)
	{
		return ACK_NO_USER;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：上传 DSP 模块数据库内指定用户特征值（命令为 8 字节/应答>8 字节）
*入口参数：无
*出口参数：成功或失败
		  上传的特征值储存在:gRsBuf[12]:gRsBuf[204]
		  用户号高位储存在:gRsBuf[9]
		  用户号低位储存在:gRsBuf[10]
		  用户权限储存在:gRsBuf[11]
*时间：2015年6月27日 19:38:55
*********************************************************************/
uint8_t uploadEigen(USART_TypeDef* USARTx,uint16_t userNum)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_UP_EIGEN;
	gTxBuf[1] = userNum >> 8;
	gTxBuf[2] = userNum & 0xFF;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 207, 100);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：下传特征值并按指定用户号存入 DSP 模块数据库（命令>8 字节/应答为 8 字节）
*入口参数：无
*出口参数：成功或失败
		  读取的比对等级储存在:gRsBuf[12]:gRsBuf[204]
		  用户号高位储存在:gRsBuf[9]
		  用户号高位储存在:gRsBuf[10]
		  用户权限储存在:gRsBuf[11]
*时间：2015年6月27日 19:38:55
*********************************************************************/
uint8_t downloadAddUser(USART_TypeDef* USARTx,uint16_t userNum,uint8_t lim,uint8_t *eigen)
{
	uint8_t m;
	uint16_t i,userCount;	//,delay = 2000
	
	getUserCount(USARTx);	//取用户总数
	userCount = (receive_pack.pack.Q1<<8)+receive_pack.pack.Q2;
	if (userCount >= USER_MAX_CNT)
		return ACK_FULL;
	
	ResetFingerFlag();	//指纹模块标志位复位
	gTxBuf[0] = CMD_DOWN_ADD;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0xc4;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	gTxBuf[5] = 0x85;	
	gTxBuf[6] = CMD_TAIL;	
	gTxBuf[7] = CMD_HEAD;	
	gTxBuf[8] = userNum >> 8;
	gTxBuf[9] = userNum & 0xFF;
	gTxBuf[10] = lim;	
	for(i = 11;i<204;i++)
	{
		gTxBuf[i] = eigen[i-11];
	}
	ResetFingerFlag();	//指纹模块标志位复位
	m = TxAndRsCmd(USARTx,204, 8, 10);
			
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：取已登录所有用户用户号及权限（命令为 8 字节/应答>8 字节）
*入口参数：无
*出口参数：成功或失败
		  用户数高位储存在:gRsBuf[9]
		  用户数高位储存在:gRsBuf[10]
		  用户权限储存在:gRsBuf[11]
		  以后每三字节分别储存:用户号高八位、用户号低八位、权限
*时间：2015年6月27日 20:09:55
*********************************************************************/
uint8_t getAllUser(USART_TypeDef* USARTx)
{
	uint8_t m,num;
	
	getUserCount(USARTx);
	num = (receive_pack.pack.Q1<<8)+receive_pack.pack.Q2;
	gTxBuf[0] = CMD_GET_USERNUM;
	gTxBuf[1] = 0;
	gTxBuf[2] = 0;
	gTxBuf[3] = 0;
	gTxBuf[4] = 0;	
	m = TxAndRsCmd(USARTx,5, (3*num+2)+11, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

/*********************************************************************
*功    能：设置/读取指纹采集等待超时时间（命令/应答均为 8 字节）
*入口参数：cmd 0:设置新的超时时间1:读取当前超时时间
		  timeOut:[0:255] 若此值为 0，若无指纹按压则指纹采集过程将一直持续
					若此值非0，在tout*T0 时间内若无指纹按压则系统将超时退出。
*出口参数：成功或失败
		  当前超时时间储存在:gRsBuf[3]
*时间：2015年6月27日 20:09:55
*********************************************************************/
uint8_t setAndGetTimeOut(USART_TypeDef* USARTx,uint8_t cmd,uint8_t timeOut)
{
	uint8_t m;
	
	gTxBuf[0] = CMD_TIMEOUT;
	gTxBuf[1] = 0;
	gTxBuf[2] = timeOut;
	gTxBuf[3] = cmd;
	gTxBuf[4] = 0;	
	
	m = TxAndRsCmd(USARTx,5, 8, 10);
		
	if (m == ACK_SUCCESS && receive_pack.pack.Q3 == ACK_SUCCESS)
	{
		return ACK_SUCCESS;
	}
	else
	{
	 	return ACK_FAIL;
	}
}

void Get_Showdata()
{
	sprintf(show_text.HEAD,"0x%X",receive_pack.pack.HEAD);
	sprintf(show_text.CMD,"0x%X",receive_pack.pack.CMD);
	sprintf(show_text.Q1,"0x%X",receive_pack.pack.Q1);
	sprintf(show_text.Q2,"0x%X",receive_pack.pack.Q2);
	sprintf(show_text.Q3,"0x%X",receive_pack.pack.Q3);
	sprintf(show_text.NONE,"0x%X",receive_pack.pack.NONE);
	sprintf(show_text.CHK,"0x%X",receive_pack.pack.CHK);
	sprintf(show_text.TAIL,"0x%X",receive_pack.pack.TAIL);
}

unsigned char CheckSum = 0,ret = 0;

unsigned char Transmit_Data(void)
{
	CheckSum = receive_pack.pack.CMD;
	CheckSum ^= receive_pack.pack.Q1;
	CheckSum ^= receive_pack.pack.Q2;
	CheckSum ^= receive_pack.pack.Q3;
	CheckSum ^= receive_pack.pack.NONE;						//计算校验值
	if((CheckSum == receive_pack.pack.CHK) 
		  && (receive_pack.pack.HEAD == 0xF5) 
	    && (receive_pack.pack.TAIL == 0xF5))	{ret = 1;}
	else										{ret = 0;}	//判断是否为合法数据
	if(    (receive_pack.pack.CMD == 0x26) 					//取模块版本号
		  || (receive_pack.pack.CMD == 0x24) 				//采集图像并上传
	    || (receive_pack.pack.CMD == 0x23) 					//采集图像并提取特征值上传
			|| (receive_pack.pack.CMD == 0x31) 				//上传 DSP 模块数据库内指定用户特征值
			|| (receive_pack.pack.CMD == 0x2B)) 			//取已登录所有用户用户号及权限
	{
		receiveMode = 1;									//准备接受接下来的数据
		receive_len=0;										//清除接收数据计数器
		u3receive_len=0;									//清除接收数据计数器
		dataLen = receive_pack.pack.Q2 + (receive_pack.pack.Q3 << 8);//读取待接受数据长度
		for(i = 0;i < dataLen + 3;i++) {receiveMore[i] = 0;} //清除接收缓存

	}
	return ret;
}

//串口2初始化
 void uart2_init(uint32_t bound)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART2，
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能GPIOA时钟
 	USART_DeInit(USART2);  //复位串口2
	
	//USART1_TX   PA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2
   
    //USART1_RX	  PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3

   //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//一般设置为115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART2, &USART_InitStructure); //初始化串口
	USART_ClearFlag(USART2, USART_FLAG_TC);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断
    USART_Cmd(USART2, ENABLE);                    //使能串口 
}

//串口2中断服务函数
void USART2_IRQHandler1(void)                					//串口2中断服务程序
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		if((receiveMode == 0) && (receive_short_ok == 0))		//数据为8个字节
		{
			receive_pack.buf[receive_len++] = USART2->DR & 0xFF;//接收串口数据
			if((receive_pack.pack.HEAD != 0xF5) || (receive_pack.pack.NONE != 0))
			{
				receive_len = 0;
				for(i = 0;i < 8;i++)							//清空接收缓存
				{
					receive_pack.buf[i] = 0;
				}
			}
			if(receive_pack.pack.CMD == 0xF5)	{receive_len--;}//如果将包尾当作包头接受，修正之	
			if(receive_len >= 8)								//接收满8个字节
			{
				if(Transmit_Data() == 1){receive_short_ok = 1;}
				else					{receive_short_ok = 0;}	//通过校验之后置位接收完成标志
				Get_Showdata();
				//receive_len=0;									//清除接收数据计数器
			}
		}else	if(receive_long_ok ==0)							//数据为dataLen个字节
		{
			receiveMore[receive_len++] = USART2->DR & 0xFF;		//接收串口数据
			if(receiveMore[0] != 0xF5)							//判断包头
			{
				receive_len = 0;
				receiveMode = 0;
				for(i = 0;i < dataLen + 3;i++) {receiveMore[i] = 0;}//清除接收缓存
			}
			if(receive_len >= dataLen + 3)						//判断是否接收够数据
			{
				CheckSum = 0;
				for(i = 1; i <= dataLen;i++)
				{
					CheckSum ^= receiveMore[i];
				}												//计算校验和
				if((receiveMore[dataLen + 2] == 0xF5) 
						&& (receiveMore[dataLen + 1] == CheckSum)){receive_long_ok = 1;}
				else											  {receive_long_ok = 0;}//判断收到的数据是否合法
				//receive_len=0;
				receiveMode = 0;								//返回接受8字节模式
			}
		}
	} 
} 


//串口3初始化
void uart3_init(uint32_t bound)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//使能USART2，
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能GPIOA时钟
 	USART_DeInit(USART3);  //复位串口3
	
	//USART3_TX   PB.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB10
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化PB.10
   
    //USART3_RX	  PB11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOB, &GPIO_InitStructure);  //初始化PB.11

   //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//一般设置为115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART3, &USART_InitStructure); //初始化串口
	USART_ClearFlag(USART3, USART_FLAG_TC);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启中断
    USART_Cmd(USART3, ENABLE);                    //使能串口 
}


//串口3中断服务函数
void USART3_IRQHandler1(void)                					//串口3中断服务程序
{
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		if((receiveMode == 0) && (u3receive_short_ok == 0))		//数据为8个字节
		{
			receive_pack.buf[u3receive_len++] = USART3->DR & 0xFF;//接收串口数据
			if((receive_pack.pack.HEAD != 0xF5) || (receive_pack.pack.NONE != 0))
			{
				u3receive_len = 0;
				for(i = 0;i < 8;i++)							//清空接收缓存
				{
					receive_pack.buf[i] = 0;
				}
			}
			if(receive_pack.pack.CMD == 0xF5)	{u3receive_len--;}//如果将包尾当作包头接受，修正之	
			if(u3receive_len >= 8)								//接收满8个字节
			{
				if(Transmit_Data() == 1){u3receive_short_ok = 1;}
				else					{u3receive_short_ok = 0;}	//通过校验之后置位接收完成标志
				Get_Showdata();
			}
		}else	if(u3receive_long_ok ==0)							//数据为dataLen个字节
		{
			receiveMore[u3receive_len++] = USART3->DR & 0xFF;		//接收串口数据
			if(receiveMore[0] != 0xF5)							//判断包头
			{
				u3receive_len = 0;
				receiveMode = 0;
				for(i = 0;i < dataLen + 3;i++) {receiveMore[i] = 0;}//清除接收缓存
			}
			if(u3receive_len >= dataLen + 3)						//判断是否接收够数据
			{
				CheckSum = 0;
				for(i = 1; i <= dataLen;i++)
				{
					CheckSum ^= receiveMore[i];
				}												//计算校验和
				if((receiveMore[dataLen + 2] == 0xF5) 
						&& (receiveMore[dataLen + 1] == CheckSum)){u3receive_long_ok = 1;}
				else											  {u3receive_long_ok = 0;}//判断收到的数据是否合法
				//receive_len=0;
				receiveMode = 0;								//返回接受8字节模式
			}
		}
	} 
} 

