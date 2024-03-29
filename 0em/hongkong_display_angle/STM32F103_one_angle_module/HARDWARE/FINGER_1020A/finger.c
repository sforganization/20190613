
#include <string.h>
#include "delay.h"	
#include "usart2.h"

#include"finger.h"
#include "machine.h"
#include "usart3.h"

#include "stm32f10x.h"

#include "stm32f10x_conf.h"

// software serial #1: RX = digital pin 3, TX = digital pin 2
//使用 3 口和 2口作软件串口输出
//#if ENABLE_DEBUG
//SoftwareSerial Finger_mySerial(3, 2);
//#endif
#define UART_BUF_LEN			8
#define BUF_N					8

unsigned char	rBuf[UART_BUF_LEN] =
{
	0
};


//接收返回信息
unsigned char	tBuf[UART_BUF_LEN] =
{
	0
};


//发送命令或者数据
//unsigned char g_ucUartRxEnd; //接收返回信息结束标志
unsigned char	g_ucUartRxLen; //接收返回信息长度
unsigned char	l_ucFPID; //用户编号

//初始化为下拉输入			
//读摸出感应状态(触摸感应时输出高电平信号 一瞬间，所以识别到了还要另一个IO作为电源控制)
void FingerGPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	FINGER_TOUCH_RCC_CLOCKCMD(FINGER_TOUCH_RCC_CLOCKGPIO, ENABLE); //使能GPIO时钟

	//初始化读状态引脚GPIOA
	GPIO_InitStructure.GPIO_Pin = FINGER_TOUCH_PIN; //
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;	//输入下拉模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//初始化GPIO	



	/* 使能(开启)引脚对应IO端口时钟 						+ */
	FINGER_POWER_RCC_CLOCKCMD(FINGER_POWER_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = FINGER_POWER_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //

	GPIO_Init(FINGER_POWER_GPIO, &GPIO_InitStructure);
}


void Finger_ClearRecvBuf(void)
{
	int 			i;

	USART2_RX_STA		= 0;						//清零  这里要清零，，，因为那个指纹模块一上电会发送一堆东西过来

	for (i = 0; i < USART2_MAX_RECV_LEN; i++)
	{
		USART2_RX_BUF[i]	= 0;
	}
}


/*******************************************************************	
**功能：串口发送一个字节
**参数：
**返回：无									
*******************************************************************/
static void MYUSART_SendData(u8 data)
{
	Usart2_SendByte(data);

	//	while ((USART2->SR & 0X40) == 0)
	//		;
	//	USART2->DR			= data;
}


/*******************************************************************	
**功能：串口发送数据 
**参数：
**返回：无									
*******************************************************************/
void UartSend(unsigned char * Datar, unsigned char cLength)
{
	do 
	{
		MYUSART_SendData(* (Datar++));
	}
	while(--cLength != 0);
}


/*******************************************************************	
**功能：延时程序
**参数：
**返回：无									
*******************************************************************/
void Delay_ms(unsigned int ms)
{
	delay_ms(ms);
}


//判断中断接收的数组有没有应答包
//waittime为等待中断接收数据的时间（单位1ms）
//返回值：数据包首地址
static u8 * JudgeStr(u16 waittime)
{
	char *			data;
	u8				str[2];

	str[0]				= 0xf5;
	str[1]				= '\0';

	USART2_RX_STA		= 0;

	while (--waittime)
	{
		delay_ms(1);

		if (USART2_RX_STA & 0X8000) //接收到一次数据
		{
			USART2_RX_STA		= 0;
			data				= strstr((const char *) USART2_RX_BUF, (const char *) str);

			if (data)
				return (u8 *)
				data;
		}
	}

	return 0;
}


/*******************************************************************************
**功能：等待数据包发送完成
**参数：
**返回：无
*******************************************************************************/
unsigned char WaitFpData(void)
{
	u8				i;

	u8 *			recv;							//

	recv				= JudgeStr(4000);

	if (recv)
	{
		for (i = 0; i < 8; i++)
		{
			rBuf[i] 			= recv[i];
		}

		return TRUE;
	}
	else 
	{
		return FALSE; //指纹芯片没有返回
	}
}


/*******************************************************************************
**功能: 计算校验值
**参数: 要发送的数据指针地址
**返回: 校验值
*******************************************************************************/
unsigned char CmdGenCHK(unsigned char wLen, unsigned char * ptr)
{
	unsigned char	i, temp = 0;

	for (i = 0; i < wLen; i++)
	{
		temp				^= * (ptr + i);
	}

	return temp;
}


/*******************************************************************************
**功能: 发送控制指纹芯片指令
**参数: wLen 数据长度
	cpPara 发送的数据
**返回：void
*******************************************************************************/
void UART_SendPackage(unsigned char wLen, unsigned char * ptr)
{
	unsigned int	i	= 0, len = 0;

	tBuf[0] 			= DATA_START;				//指令包

	for (i = 0; i < wLen; i++) // data in packet 
	{
		tBuf[1 + i] 		= * (ptr + i);
	}

	tBuf[wLen + 1]		= CmdGenCHK(wLen, ptr); 	//Generate checkout data
	tBuf[wLen + 2]		= DATA_END;
	len 				= wLen + 3;

	g_ucUartRxLen		= len;

	Finger_ClearRecvBuf();
	UartSend(tBuf, len);
}


/*******************************************************************************
**功能：返回信息处理
**参数： cmd 不同命令不同处理
**返回：处理结果
*******************************************************************************/
unsigned char Check_Package(unsigned char cmd)
{
	unsigned char	flag = FALSE;

	//	if (!WaitFpData())
	//		return flag; //等待接收返回信息
	if (rBuf[0] != DATA_START)
		return flag;

	if (rBuf[1] != cmd)
		return flag;

	if (rBuf[6] != CmdGenCHK(g_ucUartRxLen - 3, &rBuf[1]))
		return flag;

	switch (cmd)
	{
		case CMD_ENROLL1:
		case CMD_ENROLL2:
		case CMD_ENROLL3:
			if (F1020_ACK_SUCCESS == rBuf[4])
				flag = TRUE;
			else if (F1020_ACK_USER_EXIST == rBuf[4])
			{
				//	  Spk_HaveUser();
				//Delay_ms(1500);
				flag				= TRUE;

				//return 22; //用户已经存在，返回22
			}

			break;

		case CMD_DELETE: //删除指定编号指纹
		case CMD_CLEAR: //清空所有指纹
		case CMD_IDENTIFY: //1:1比对
			if (F1020_ACK_SUCCESS == rBuf[4])
				flag = TRUE;

			break;

		case CMD_USERNUMB: //取用户总数
			if (F1020_ACK_SUCCESS == rBuf[4])
			{
				flag				= TRUE;
				l_ucFPID			= rBuf[3];
			}

			break;

		case CMD_SEARCH: //1:N比对
			if ((1 == rBuf[4]) || (2 == rBuf[4]) || (3 == rBuf[4]))
			{
				flag				= TRUE;
				l_ucFPID			= rBuf[3];
			}

			break;

		case CMD_MODE_SET: //指纹录入模式设置
			if ((F1020_ACK_SUCCESS == rBuf[4]) && (rBuf[3]) == F1020_MODE_REC) //设置为可重复模式
			{
				flag				= TRUE;
			}

			break;

		default:
			break;
	}

#if ENABLE_DEBUG

	//	  Finger_mySerial.print("packcheck cmd:  return: ");
	//	Finger_mySerial.println(cmd);
	//	Finger_mySerial.println(flag);
#endif

	return flag;
}


/*******************************************************************************
**功能：以CharBuffer1 或CharBuffer2 中的特征文件搜索整个或部分指纹库
**参数：
**返回：无
*******************************************************************************/
void FP_Search(void)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_SEARCH;				//1:N比对
	* (buf + 1) 		= 0x00;
	* (buf + 2) 		= 0x00;
	* (buf + 3) 		= 0x00;
	* (buf + 4) 		= 0x00;

	UART_SendPackage(5, buf);
}


/*******************************************************************************
**功能：以CharBuffer1 或CharBuffer2 中的特征文件搜索 1 : 1 匹配
**参数：
**返回：无
*******************************************************************************/
void FP_Search_One(unsigned int u_id)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_IDENTIFY; 			//1:1比对
	* (buf + 1) 		= u_id >> 8;
	* (buf + 2) 		= u_id & 0xff;
	* (buf + 3) 		= 0x00;
	* (buf + 4) 		= 0x00;

	UART_SendPackage(5, buf);
}


/*******************************************************************************
**功能：清空 flash 指纹库
**参数：
**返回：无
*******************************************************************************/
void FP_Clear(void)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_CLEAR;
	* (buf + 1) 		= 0x00;
	* (buf + 2) 		= 0x00;
	* (buf + 3) 		= 0x00;
	* (buf + 4) 		= 0x00;

	UART_SendPackage(5, buf);
}


/*******************************************************************************
**功能：删除指定编号指纹
**参数：u_id
**返回：void
*******************************************************************************/
void FP_Delete(unsigned int u_id)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_DELETE;
	* (buf + 1) 		= u_id >> 8;
	* (buf + 2) 		= u_id & 0xff;
	* (buf + 3) 		= 0x00;
	* (buf + 4) 		= 0x00;
	UART_SendPackage(5, buf);
}


/*******************************************************************************
**功能：1:1比对
**参数：u_id
**返回：void
*******************************************************************************/
void FP_Identify(unsigned int u_id)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_IDENTIFY;
	* (buf + 1) 		= u_id >> 8;
	* (buf + 2) 		= u_id & 0xff;
	* (buf + 3) 		= 0x00;
	* (buf + 4) 		= 0x00;
	UART_SendPackage(5, buf);
}


/*******************************************************************************
**注册指纹
**输入两次指纹注册一个指纹模板
**参数：UserID 指纹号
*******************************************************************************/
void Enroll_Step1(unsigned int u_id)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_ENROLL1;
	* (buf + 1) 		= u_id >> 8;
	* (buf + 2) 		= u_id & 0xff;
	* (buf + 3) 		= 1;
	* (buf + 4) 		= 0x00;

	UART_SendPackage(5, buf);
}


/*******************************************************************************
**注册指纹
**输入两次指纹注册一个指纹模板
**参数：UserID 指纹号
*******************************************************************************/
void Enroll_Step2(unsigned int u_id)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_ENROLL2;
	* (buf + 1) 		= u_id >> 8;
	* (buf + 2) 		= u_id & 0xff;
	* (buf + 3) 		= 1;
	* (buf + 4) 		= 0x00;

	UART_SendPackage(5, buf);
}


/*******************************************************************************
**注册指纹
**输入三次指纹注册一个指纹模板
**参数：UserID 指纹号
*******************************************************************************/
void Enroll_Step3(unsigned int u_id)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_ENROLL3;
	* (buf + 1) 		= u_id >> 8;
	* (buf + 2) 		= u_id & 0xff;
	* (buf + 3) 		= 1;
	* (buf + 4) 		= 0x00;

	UART_SendPackage(5, buf);
}


/*******************************************************************************
**注册指纹，第一次按下
**输入三次指纹注册一个指纹模板，1020A小面阵传感器建议采集6次。
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Enroll(unsigned int u_id)
{
	Enroll_Step1(u_id);

	if (FALSE == Check_Package(CMD_ENROLL1))
		return FALSE;

	Delay_ms(100);

	Enroll_Step2(u_id);

	if (FALSE == Check_Package(CMD_ENROLL2))
		return FALSE;

	Delay_ms(100);

	Enroll_Step2(u_id);

	if (FALSE == Check_Package(CMD_ENROLL2))
		return FALSE;

	Enroll_Step2(u_id);

	if (FALSE == Check_Package(CMD_ENROLL2))
		return FALSE;

	Delay_ms(100);

	Enroll_Step2(u_id);

	if (FALSE == Check_Package(CMD_ENROLL2))
		return FALSE;

	Delay_ms(100);

	Enroll_Step3(u_id);
	return Check_Package(CMD_ENROLL3);
}


/*******************************************************************************
**清空指纹
**
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Clear(void)
{
	FP_Clear();

	//	if(FALSE == WaitFpData())return FALSE;
	return Check_Package(CMD_CLEAR);
}


/*******************************************************************************
**删除指定指纹
**
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Delete(unsigned int u_id)
{
	FP_Delete(u_id);

	//	if(FALSE == WaitFpData())return FALSE;
	return Check_Package(CMD_DELETE);
}


/*******************************************************************************
**读取用户总数
**
**参数?N
*******************************************************************************/
int Finger_Read(void)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_USERNUMB;
	* (buf + 1) 		= 0x00;
	* (buf + 2) 		= 0x00;
	* (buf + 3) 		= 0x00;
	* (buf + 4) 		= 0x00;
	UART_SendPackage(5, buf);

	//	if (Check_Package(CMD_USERNUMB))
	//	{
	//		return l_ucFPID;
	//	}
	return 0; //
}


/*******************************************************************************
**读取用户总数
**
**参数
*******************************************************************************/
unsigned char Finger_Search(int * userID)
{
	FP_Search();
	*userID 			= l_ucFPID;
	return Check_Package(CMD_SEARCH);
}


/*******************************************************************************
**设置指纹录入模式
**，应答的数据是现在的状态（可以理解为接收到指令之前的状态）。所以，需要再发一?
	?，查到的才是上次指令更改后的
**参数?N
*******************************************************************************/
int Finger_ModeSet(void)
{
	unsigned char	buf[BUF_N];

	*buf				= CMD_MODE_SET;
	* (buf + 1) 		= 0x00;
	* (buf + 2) 		= F1020_MODE_REC; 					// 0：允许重复1：禁止重复
	* (buf + 3) 		= 0x00; 					//0 设置新的添加模式 1读取
	* (buf + 4) 		= 0x00;


	UART_SendPackage(5, buf);

	//	if (Check_Package(CMD_MODE_SET))
	//	{
	//		if(rBuf[3] == * (buf + 2)) //应答的数据是现在的状态（可以理解为接收到指令之前的状态）。所以，需要再发一次，查到的才是上次指令更改后的
	//			return 0;
	//		  else
	//			  return -1;
	//	}
	return 0; //
}


/*******************************************************************************
* 名称: 
* 功能: 实测发送到指纹模块用时5ms
* 形参:	
* 返回: 无
* 说明: 
*******************************************************************************/

int Finger_Event(char opt, void * data)
{
	u8				str[2];
    static  u8 A1020_cmd;
	int 			i;
	char *			pRev;
	static FingerState_T FingerState = FINGER_STATE_INIT;
	static FingerSub_T fingerSubState = FINGER_SUB_INIT;
	FingerData_t *	pFingerData;
	PollEvent_t *	pEvent;

    static vu32  u32TimeSave = 0;

	str[0]				= 0xf5;
	str[1]				= '\0';

	pEvent				= data;
	pFingerData 		= (FingerData_t *)
	pEvent->MachineCtrl.pData;

	switch (FingerState)
	{
		case FINGER_STATE_INIT:
			pEvent->MachineRun.bFinish = FALSE;

			switch (opt)
			{
				case FINGER_MODE_SET:
#if FINGER_DEBUG
                    u3_printf("finger  模式设置\n");
#endif
					Finger_ModeSet();
                    A1020_cmd = CMD_MODE_SET;
					break;

				case FINGER_SEARCHING: //1: N
#if FINGER_DEBUG
                    u3_printf("finger  搜索指纹\n");
#endif

//#if FINGER_DEBUG
//	GPIO_SetBits(GPIOC, GPIO_Pin_13);
//#endif
					FP_Search();
//#if FINGER_DEBUG
//	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
//                    
//#endif
                    A1020_cmd = CMD_SEARCH;
					break;

				case FINGER_READ:
#if FINGER_DEBUG
                    u3_printf("finger  读取指纹个数\n");
#endif
					Finger_Read();
                    A1020_cmd = CMD_USERNUMB;
					break;

				case FINGER_ENROLL: //注册指纹
				    if(pEvent->MachineRun.u32LastTime - u32TimeSave < 100)   //延时
                        break;
                    
#if FINGER_DEBUG
                                        u3_printf("finger  注册指纹： %d \n", fingerSubState);
#endif

                    u32TimeSave = pEvent->MachineRun.u32LastTime;
					switch (fingerSubState)
					{
						case FINGER_SUB_INIT:
							Enroll_Step1(pFingerData->u8FingerID);
                            A1020_cmd = CMD_ENROLL1;
							break;

						case FINGER_SUB_STEP1:
						case FINGER_SUB_STEP2:
						case FINGER_SUB_STEP3:
						case FINGER_SUB_STEP4:
							A1020_cmd= CMD_ENROLL2;
							Enroll_Step2(pFingerData->u8FingerID);
							break;

						case FINGER_SUB_STEP5:
							A1020_cmd = CMD_ENROLL3;
							Enroll_Step3(pFingerData->u8FingerID);
							break;

						default:
							break;
					}
                    fingerSubState++;
					break;

				case FINGER_IDENTIFY: //验证指纹 1: 1
					A1020_cmd = CMD_IDENTIFY;
					break;

				case FINGER_CLEAR: //清空指纹
#if FINGER_DEBUG
                    u3_printf("finger  清空 \n");
#endif
					A1020_cmd = CMD_CLEAR;
					FP_Clear();
					break;

				case FINGER_DELETE: //删除指定指纹
					FP_Delete(pFingerData->u8FingerID);
					A1020_cmd = CMD_DELETE;
					break;
			}

//			pEvent->MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT;
			FingerState = FINGER_STATE_WAIT; //延时等待接收状态
			break;

		case FINGER_STATE_WAIT: // 等待hal层状态返回
			if (USART2_RX_STA & 0X8000) //接收到一次数据
			{
				USART2_RX_STA		= 0;
				pRev				= strstr((const char *) USART2_RX_BUF, (const char *) str);

				if (data)
				{

					for (i = 0; i < 8; i++)
					{
						rBuf[i] 			= pRev[i];
					}

					FingerState 		= FINGER_STATE_CHECK;
				}
			}
			else if (pEvent->MachineRun.u32LastTime - pEvent->MachineRun.u32StartTime >
				 pEvent->MachineCtrl.u32Timeout) //状态机超时退出
			{
			    pEvent->MachineRun.u16RetVal = EVENT_TIME_OUT;
				FingerState 		= FINGER_STATE_EXIT;
			}

			break;

		case FINGER_STATE_CHECK:
//#if FINGER_DEBUG
//	GPIO_SetBits(GPIOC, GPIO_Pin_13);
//#endif
			pEvent->MachineRun.u16RetVal = Check_Package(A1020_cmd); //在里面加入可重复模式的判断 

			if ((opt == FINGER_ENROLL) && (fingerSubState <= FINGER_SUB_STEP5)) //注册指纹要多次
			{
				if (pEvent->MachineRun.u16RetVal == TRUE)
					FingerState = FINGER_STATE_INIT;
				else
					FingerState = FINGER_STATE_EXIT;
			}
			else
			{
				FingerState 		= FINGER_STATE_EXIT;
			}
            
#if FINGER_DEBUG
                GPIO_ResetBits(GPIOC, GPIO_Pin_13);
                                A1020_cmd = CMD_SEARCH;
#endif

			break;

		case FINGER_STATE_EXIT:
			pEvent->MachineRun.bFinish = TRUE;
			pFingerData->u8FingerID = l_ucFPID;
			fingerSubState = FINGER_SUB_INIT;
			FingerState = FINGER_STATE_INIT; //为下一次初始化状态机
			break;

		default:
			break;
	}

	return 0;
}


