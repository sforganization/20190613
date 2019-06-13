
#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include "usart1.h" 
#include "usart3.h" 
#include "delay.h"

#include "SysComment.h"
#include "stm32f10x_it.h"
#include "stm32f10x_iwdg.h"
#include "san_flash.h"

#include "step_driver.h"


#include "machine.h"


#define SLAVE_WAIT_TIME 		3000	//3s 从机应答时间

#define LOCK_SET_H_TIME 		1	  //高电平时间 1ms
#define LOCK_SET_OPEN_TIME		3	  //开锁低电平时间
#define LOCK_SET_CLOSE_TIME 	10	  //关锁低电平时间


#define LOCK_SET_OFF_TIME		300	  //给单片机反应时间，电机回正之后再转


#define MOTO_UP_TIME			800			//电机上升时间
#define MOTO_DOWN_TIME			1500		//电机下降时间




#define MOTO_TRUN_OVER			500			//MOTO 3  电机电平翻转时间


#define MOTO_ANGLE_TIME 		(750 * PWM_PERIOD / PWM_POSITIVE)  //电机转动一定角度的时间
#define MOTO_DEVIATION_TIME 	700  //电机转动时间偏差 200ms 最大的偏差来自于写flash时，因为写flash禁止一切 flash操作（代码的读取），包括中断，

//注意，由于串口接收会占用一定的时间，这里的延时要相应加长，特别是当安卓发送停止命令过来的时候，如果是在继续正转、反转时候发，这个已经多走了一段，往回走是要相应加长时间
//因为
#define LOCK_OPEN_TIME			5			//开锁给低电平时间


//static bool  g_bSensorState = TRUE;
typedef enum 
{
ACK_SUCCESS = 0,									//成功 
ACK_FAILED, 										//失败
ACK_AGAIN,											//再次输入
ACK_ERROR,											//命令出错
ACK_DEF = 0xFF, 
} Finger_Ack;


//内部变量
static vu16 	mDelay;

char			priBuf[120] =
{
	0
};


u32 			g_au32DataSaveAddr[6] =
{
	//103ze 512K
	//	0x0807D000, 
	//	0x0807D800, 
	//	0x0807E000, 
	//	0x0807E800, 
	//	0x0807F000, 
	//	0x0807F800
	//103c8  64K
	0X0800E800, 
	0X0800EC00, 
	0x0800F000, 
	0x0800F400, 
	0x0800F800, 
	0x0800FC00
};


vu32			g_au16MoteTime[][3] =
{
	//{
	//	MOTO_TIME_OFF, 0, 0
	//},	
	{
		MOTO_TIME_TPD, 43200000, 43200000
	},
	{
		MOTO_TIME_650, 120000, 942000
	},
	{
		MOTO_TIME_750, 120000, 800000
	},
	{
		MOTO_TIME_850, 120000, 693000
	},
	{
		MOTO_TIME_1000, 120000, 570000
	},
	{
		MOTO_TIME_1950, 120000, 234000
	},
	{
		MOTO_TIME_ANGLE, MOTO_ANGLE_TIME, 2
	},
};


static vu16 	g_u16StepCnt[] =
{
	0, 1600, 3200, 4800
};


PollEvent_t 	g_StepMotoEvent;

u8				g_GlassSendAddr = 0; //要发送的手表地址
u8				g_FingerAck = 0; //应答数据命令
u8				g_AckType = 0; //应答类型


//内部函数
void SysTickConfig(void);


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void DelayUs(uint16_t nCount)
{
	u32 			del = nCount * 5;

	//48M 0.32uS
	//24M 0.68uS
	//16M 1.02us
	while (del--)
		;
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void DelayMs(uint16_t nCount)
{
	unsigned int	ti;

	for (; nCount > 0; nCount--)
	{
		for (ti = 0; ti < 4260; ti++)
			; //16M/980-24M/1420 -48M/2840
	}
}


/*******************************************************************************
* 名称: Strcpy()
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
******************************************************************************/
void Strcpy(u8 * str1, u8 * str2, u8 len)
{
	for (; len > 0; len--)
	{
		*str1++ 			= *str2++;
	}
}


/*******************************************************************************
* 名称: Strcmp()
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
******************************************************************************/
bool Strcmp(u8 * str1, u8 * str2, u8 len)
{
	for (; len > 0; len--)
	{
		if (*str1++ != *str2++)
			return FALSE;
	}

	return TRUE;
}


/*******************************************************************************
* 名称: Sys_DelayMS()
* 功能: 系统延迟函数
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Sys_DelayMS(uint16_t nms)
{
	mDelay				= nms + 1;

	while (mDelay != 0x0)
		;
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SysTickConfig(void)
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	/* Setup SysTick Timer for 1ms interrupts  */
	if (SysTick_Config(SystemCoreClock / 1000))
	{
		/* Capture error */
		while (1)
			;
	}

	/* Configure the SysTick handler priority */
	NVIC_SetPriority(SysTick_IRQn, 0x0);

#if (								SYSINFOR_PRINTF == 1)
	printf("SysTickConfig:Tick=%d/Second\r\n", 1000);
#endif
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void LedInit(void) //
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LED_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_GPIO, &GPIO_InitStructure);
	GPIO_SetBits(LED_GPIO, LED_PIN);
}


//	  /*******************************************************************************
//	  * 名称: 
//	  * 功能: 
//	  * 形参:		
//	  * 返回: 无
//	  * 说明: 
//	  *******************************************************************************/
//	  void FingerTouchTask(void)
//	  {
//		SearchResult seach;
//		u8 ensure;
//		static u8 u8MatchCnt = 0;						//匹配失败次数，默认匹配MATCH_FINGER_CNT次 
//		if ((SysTask.bEnFingerTouch == TRUE))
//		{
//			switch (SysTask.TouchState)
//			{
//				case TOUCH_MATCH_INIT:
//					if (PS_Sta) //有指纹按下
//					{
//						SysTask.nWaitTime	= 10;		//一段时间再检测
//						SysTask.TouchState	= TOUCH_MATCH_AGAIN;
//						u8MatchCnt			= 0;
//					}
//					break;
//				case TOUCH_MATCH_AGAIN:
//					if (SysTask.nWaitTime == 0) //有指纹按下
//					{
//						if (PS_Sta) //有指纹按下
//						{
//							ensure				= PS_GetImage();
//							if (ensure == 0x00) //生成特征成功
//							{
//								//PS_ValidTempleteNum(&u16FingerID); //读库指纹个数
//								ensure				= PS_GenChar(CharBuffer1);
//								if (ensure == 0x00) //生成特征成功
//								{
//									ensure				= PS_Search(CharBuffer1, 0, FINGER_MAX_CNT, &seach);
//									if (ensure == 0) //匹配成功
//									{
//										SysTask.u16FingerID = seach.pageID;
//										SysTask.nWaitTime	= SysTask.nTick + 2000; //延时一定时间 成功也不能一直发
//										SysTask.TouchSub	= TOUCH_SUB_INIT;
//										SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
//										//								  SysTask.bEnFingerTouch = FALSE;	 //成功，则禁止指纹模块任务
//										//匹配成功，发送到平板端，应答
//										g_FingerAck 		= ACK_SUCCESS;
//										g_AckType			= FINGER_ACK_TYPE;
//										if (SysTask.SendState == SEND_IDLE)
//											SysTask.SendState = SEND_TABLET_FINGER;
//									}
//									else if (u8MatchCnt >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
//									{
//										//								  SysTask.bEnFingerTouch = FALSE;	 //成功，则禁止指纹模块任务
//										//匹配失败，发送到平板端，应答
//										g_FingerAck 		= ACK_FAILED;
//										g_AckType			= FINGER_ACK_TYPE;
//										if (SysTask.SendState == SEND_IDLE)
//											SysTask.SendState = SEND_TABLET_FINGER;
//										SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
//										SysTask.nWaitTime	= SysTask.nTick + 300; //延时一定时间 如果手臭识别不了，再想来一个人检测，
//									}
//								}
//							}
//							if (ensure) //匹配失败
//							{
//								SysTask.nWaitTime	= SysTask.nTick + 10; //延时一定时间 这里检测到的应该是同一个人的指纹
//								u8MatchCnt++;
//							}
//						}
//					}
//					break;
//				case TOUCH_ADD_USER:
//					{
//						switch (SysTask.TouchSub)
//						{
//							case TOUCH_SUB_INIT: // *
//								if (SysTask.nWaitTime == 0)
//								{
//									SysTask.TouchSub	= TOUCH_SUB_ENTER;
//								}
//								break;
//							case TOUCH_SUB_ENTER: //
//								if ((PS_Sta) && (SysTask.nWaitTime == 0)) //如果有指纹按下
//								{
//									ensure				= PS_GetImage();
//									if (ensure == 0x00)
//									{
//										ensure				= PS_GenChar(CharBuffer1); //生成特征
//										if (ensure == 0x00)
//										{
//											SysTask.nWaitTime	= 3000; //延时一定时间再去采集
//											SysTask.TouchSub	= TOUCH_SUB_AGAIN; //跳到第二步				
//											//Please press	agin
//											//成功，发送到平板端，应答从新再按一次
//											g_FingerAck 		= ACK_AGAIN;
//											g_AckType			= FINGER_ACK_TYPE;
//											if (SysTask.SendState == SEND_IDLE)
//												SysTask.SendState = SEND_TABLET_FINGER;
//										}
//									}
//								}
//								break;
//							case TOUCH_SUB_AGAIN:
//								if ((PS_Sta) && (SysTask.nWaitTime == 0)) //如果有指纹按下
//								{
//									ensure				= PS_GetImage();
//									if (ensure == 0x00)
//									{
//										ensure				= PS_GenChar(CharBuffer2); //生成特征
//										if (ensure == 0x00)
//										{
//											ensure				= PS_Match(); //对比两次指纹
//											if (ensure == 0x00) //成功
//											{
//												ensure				= PS_RegModel(); //生成指纹模板
//												if (ensure == 0x00)
//												{
//													ensure				= PS_StoreChar(CharBuffer2, SysTask.u16FingerID); //储存模板
//													if (ensure == 0x00)
//													{
//														SysTask.TouchSub	= TOUCH_SUB_WAIT;
//														SysTask.nWaitTime	= 3000; //延时一定时间退出
//														//													  SysTask.bEnFingerTouch = FALSE;	 //成功，则禁止指纹模块任务
//														//成功，发送到平板端，应答
//														g_FingerAck 		= ACK_SUCCESS;
//														g_AckType			= FINGER_ACK_TYPE;
//														if (SysTask.SendState == SEND_IDLE)
//															SysTask.SendState = SEND_TABLET_FINGER;
//													}
//													else 
//													{
//														//匹配失败，发送到平板端，应答
//														SysTask.TouchSub	= TOUCH_SUB_ENTER;
//														SysTask.nWaitTime	= 3000; //延时一定时间退出
//													}
//												}
//												else 
//												{
//													//失败，发送到平板端，应答
//													g_FingerAck 		= ACK_FAILED;
//													g_AckType			= FINGER_ACK_TYPE;
//													if (SysTask.SendState == SEND_IDLE)
//														SysTask.SendState = SEND_TABLET_FINGER;
//													SysTask.TouchSub	= TOUCH_SUB_ENTER;
//													SysTask.nWaitTime	= 3000; //延时一定时间退出
//												}
//											}
//											else 
//											{
//												//失败，发送到平板端，应答
//												g_FingerAck 		= ACK_FAILED;
//												g_AckType			= FINGER_ACK_TYPE;
//												if (SysTask.SendState == SEND_IDLE)
//													SysTask.SendState = SEND_TABLET_FINGER;
//												SysTask.TouchSub	= TOUCH_SUB_ENTER;
//												SysTask.nWaitTime	= 3000; //延时一定时间退出
//											}
//										}
//									}
//								}
//								break;
//							case TOUCH_SUB_WAIT: // *
//								if ((SysTask.nWaitTime == 0))
//								{
//									SysTask.TouchState	= TOUCH_IDLE;
//									SysTask.bEnFingerTouch = FALSE; //成功，则禁止指纹模块任务
//									SysTask.TouchSub	= TOUCH_SUB_INIT;
//								}
//								break;
//							default:
//								break;
//						}
//					}
//					break;
//				case TOUCH_DEL_USER:
//					{
//						switch (SysTask.TouchSub)
//						{
//							case TOUCH_SUB_INIT: // *
//								if (SysTask.nWaitTime == 0)
//								{
//									u8MatchCnt			= 0;
//									SysTask.TouchSub	= TOUCH_SUB_ENTER;
//								}
//								break;
//							case TOUCH_SUB_ENTER: //
//								if (SysTask.nWaitTime)
//								{
//									break;
//								}
//								ensure = PS_DeletChar(SysTask.u16FingerID, 1);
//								if (ensure == 0)
//								{
//									//发送到平板端面
//									g_FingerAck 		= ACK_SUCCESS;
//									g_AckType			= FINGER_ACK_TYPE;
//									if (SysTask.SendState == SEND_IDLE)
//										SysTask.SendState = SEND_TABLET_FINGER;
//									SysTask.TouchSub	= TOUCH_SUB_WAIT;
//									SysTask.nWaitTime	= 100; //延时一定时间退出
//								}
//								else //删除失败 ,
//								{
//									u8MatchCnt++;
//									SysTask.nWaitTime	= 500; //延时一定时间再次删除
//									if (u8MatchCnt >= 3) //失败退出
//									{
//										//发送到平板端面
//										g_FingerAck 		= ACK_FAILED;
//										g_AckType			= FINGER_ACK_TYPE;
//										if (SysTask.SendState == SEND_IDLE)
//											SysTask.SendState = SEND_TABLET_FINGER;
//										SysTask.TouchSub	= TOUCH_SUB_WAIT;
//									}
//								}
//								break;
//							case TOUCH_SUB_WAIT: // *
//								if ((SysTask.nWaitTime == 0))
//								{
//									SysTask.TouchState	= TOUCH_IDLE;
//									SysTask.bEnFingerTouch = FALSE; //成功，则禁止指纹模块任务
//									SysTask.TouchSub	= TOUCH_SUB_INIT;
//								}
//								break;
//							default:
//								break;
//						}
//					}
//					break;
//				case TOUCH_IDLE:
//				default:
//					break;
//			}
//		}
//	  }

/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void LedTask(void)
{
	if (SysTask.LED_Mode != SysTask.LED_ModeSave)
	{
		if (SysTask.LED_Mode == LED_MODE_ON)
			GPIO_ResetBits(LED_GPIO, LED_PIN);
		else 
			GPIO_SetBits(LED_GPIO, LED_PIN);

		SysTask.LED_ModeSave = SysTask.LED_Mode;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 校验和
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
u8 CheckSum(u8 * pu8Addr, u8 u8Count)
{
	u8 i				= 0;
	u8 u8Result 		= 0;

	for (i = 0; i < u8Count; i++)
	{
		u8Result			+= pu8Addr[i];
	}

	return ~ u8Result;
}


/*******************************************************************************
* 名称: 
* 功能: 从机包发送
*		包头 + 从机地址 + 	 手表地址 + 命令 + 数据包[3] + 校验和 + 包尾
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SlavePackageSend(u8 u8SlaveAddr, u8 u8GlassAddr, u8 u8Cmd, u8 * u8Par)
{
	u8 i				= 0;
	u8 u8SendArr[9] 	=
	{
		0
	};

	u8SendArr[0]		= 0xAA; 					//包头
	u8SendArr[1]		= u8SlaveAddr;				//从机地址
	u8SendArr[2]		= u8GlassAddr;				//手表地址
	u8SendArr[3]		= u8Cmd;					//命令
	u8SendArr[4]		= u8Par[0]; 				//参数1
	u8SendArr[5]		= u8Par[1]; 				//参数2
	u8SendArr[6]		= u8Par[2]; 				//参数3

	u8SendArr[7]		= CheckSum(u8SendArr, 7);	//校验和
	u8SendArr[8]		= 0x55; 					//包尾


	for (i = 0; i < 8; i++)
	{
		USART_SendData(USART3, u8SendArr[i]);
	}
}


#define RECV_PAKAGE_CHECK		10	 //   10个数据检验

/*******************************************************************************
* 名称: ，接收包处理。。。平板发给主机的命令包处理
* 功能: 判断中断接收的数组有没有更新数据包 或者就绪包
* 形参:		
* 返回: -1, 失败 0:成功
* 说明: 
*******************************************************************************/
static int Usart1JudgeStr(void)
{
	//	  包头 + 命令 +参数[3]		+数据[3]	  +  校验和 + 包尾
	//	  包头：0XAA
	//	  命令： CMD_UPDATE	 、 CMD_READY	CMD_CHANGE
	//	  参数： 地址，LED模式，增加删除指纹ID, CMD_CHANGE的第几个表
	//	  数据：（锁开关 + 方向 + 时间）
	//	  校验和：
	//	  包尾： 0X55
	//主机发给平板的包分为两种。。一种是单个，另一种是全部表数据
	const char * data;

	u8 u8Cmd;
	u8 i;
	int tmp 			= 0;
	u8 str[8]			=
	{
		0
	};
	u8 u8CheckSum		= 0;
	str[0]				= 0xAA;

	data				= strstr((const char *) USART1_RX_BUF, (const char *) str);

	if (!data)
		return - 1;

	if (* (data + 9) != 0X55) //包尾
		return - 1;

	for (i = 0; i < RECV_PAKAGE_CHECK; i++)
		u8CheckSum += * (data + i);

	if (u8CheckSum != 0x55 - 1) //检验和不等于包尾 返回失败
		return - 1;


	u8Cmd				= * (data + 1);

	switch (u8Cmd)
	{
		case CMD_READY:
			SysTask.bTabReady = TRUE;
			SysTask.SendState = SEND_INIT;
			break;

		case CMD_UPDATE:
			if (* (data + 2) == 0xFF) //该全部手表
			{
				for (i = 0; i < GLASS_MAX_CNT; i++)
				{
					//SysTask.WatchState[i].u8LockState  = *(data + 5);  ////锁状态不在此更新，因为开锁需要密码之后
					SysTask.WatchState[i].u8Dir = * (data + 6);
					SysTask.WatchState[i].MotoTime = * (data + 7);

					SysTask.Moto_Mode[i] = (MotoFR) * (data + 6);
					SysTask.Moto_Time[i] = (MotoTime_e) * (data + 7);
					SysTask.Moto_RunTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][1];
					SysTask.Moto_WaitTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][2];

				}
			}
			else //单个手表的状态更新
			{
				//				  SysTask.WatchState[*(data + 2)].u8LockState  = *(data + 5);//锁状态不在此更新，因为开锁需要密码之后
				SysTask.WatchState[* (data + 2)].u8Dir = * (data + 6);
				SysTask.WatchState[* (data + 2)].MotoTime = * (data + 7);
				SysTask.Moto_Mode[* (data + 2)] = (MotoFR) * (data + 6);
				SysTask.Moto_Time[* (data + 2)] = (MotoTime_e) * (data + 7);

				tmp 				= SysTask.Moto_Time[* (data + 2)];
				SysTask.Moto_RunTime[* (data + 2)] = g_au16MoteTime[tmp][1];
				SysTask.Moto_WaitTime[* (data + 2)] = g_au16MoteTime[tmp][2];
			}

			SysTask.u16SaveTick = SysTask.nTick + SAVE_TICK_TIME; //数据更新保存
			break;

		case CMD_ADD_USER:
			if (* (data + 2) <= FINGER_MAX_CNT)
			{
				SysTask.bEnFingerTouch = TRUE;
				SysTask.TouchState	= TOUCH_ADD_USER;
				SysTask.TouchSub	= TOUCH_SUB_INIT; //如果从删除指纹到增加指纹
				SysTask.u16FingerID = * (data + 2);
			}

			break;

		case CMD_DEL_USER:
			if (* (data + 2) <= FINGER_MAX_CNT)
			{
				SysTask.bEnFingerTouch = TRUE;
				SysTask.TouchState	= TOUCH_DEL_USER;
				SysTask.TouchSub	= TOUCH_SUB_INIT; //如果从删除指纹到增加指纹
				SysTask.u16FingerID = * (data + 2);
			}

			break;

		case CMD_LOCK_MODE: //
			if (* (data + 2) == 0xFF) //全部锁状态
			{
				for (i = 0; i < GLASS_MAX_CNT; i++)
				{
					SysTask.LockMode[i] = * (data + 5);
				}
			}
			else //单个锁状态更新
			{
				SysTask.LockMode[* (data + 2)] = * (data + 5); //电机转起来锁状态就是OFF?
			}

			break;

		//	 
		//			case CMD_RUN_MOTO:
		//				if (* (data + 2) == 0xFF) //该全部手表
		//				{
		//					for (i = 0; i < GLASS_MAX_CNT; i++)
		//					{
		//						SysTask.LockMode[i] = LOCK_OFF; //电机转起来锁状态就是OFF?
		//						SysTask.Moto_State[i] = MOTO_STATE_INIT;
		//					}
		//				}
		//				else //单个手表的状态更新
		//				{
		//					SysTask.LockMode[* (data + 2)] = LOCK_OFF; //电机转起来锁状态就是OFF?
		//					SysTask.Moto_State[* (data + 2)] = MOTO_STATE_INIT;
		//				}
		//	 
		//				break;
		//	 
		case CMD_LED_MODE:
			if ((* (data + 2) == LED_MODE_ON) || (* (data + 2) == LED_MODE_OFF)) //LED模式有效
			{
				SysTask.LED_Mode	= * (data + 2);
			}

			SysTask.u16SaveTick = SysTask.nTick + SAVE_TICK_TIME; //数据更新保存
			break;

		case CMD_PASSWD_ON: // 密码模式，开启指纹
			SysTask.bEnFingerTouch = TRUE;
			SysTask.TouchState = TOUCH_MATCH_INIT;
			break;

		case CMD_PASSWD_OFF: // 密码模式，关闭指纹
			SysTask.bEnFingerTouch = FALSE;
			SysTask.TouchState = TOUCH_IDLE;
			break;

		case CMD_CHANGE: // 更换表，参数为第几个表
			if (* (data + 2) <= GLASS_NUM_FOUR) //参数表示第几个表
			{
				SysTask.bGlassChange = TRUE;
    
                SysTask.WatchDisplay = (Display_T)(*(data + 2));    //当前展示的是第几个表
                
				if (* (data + 2) == 0)
				{
					SysTask.u16StepRequire = 0;
				}
				else 
				{
//				
					SysTask.u16StepRequire = * (data + 2) >
						 SysTask.GlassNum ? g_u16StepCnt[* (data + 2)] -g_u16StepCnt[SysTask.GlassNum]: CIRCLE_STEP_PULSE - g_u16StepCnt[SysTask.GlassNum] + g_u16StepCnt[* (data + 2)];
					
					

                    if (SysTask.GlassNum == 0){
                        if (* (data + 2) != 3)
						    SysTask.u16StepRequire += 60;
                        else
                            SysTask.u16StepRequire += 20;
                        }
					else 					if (* (data + 2) == 3)
						SysTask.u16StepRequire -= 40;
                    
//					if(* (data + 2) > SysTask.GlassNum)
//                    {
//                        SysTask.u16StepRequire = g_u16StepCnt[* (data + 2)] -g_u16StepCnt[SysTask.GlassNum] + (* (data + 2) - SysTask.GlassNum) * 60;  //每个1600步要加 10的误差
//                    } 
//                    else
//                    {
//                        SysTask.u16StepRequire =  CIRCLE_STEP_PULSE - g_u16StepCnt[SysTask.GlassNum] + g_u16StepCnt[* (data + 2)] + 4 - ( SysTask.GlassNum ) * 60;  //每个1600步要加 10的误差,共6400步，所以是4

//                    }
				}

				SysTask.GlassNum	= (GlassNum_T) (* (data + 2));

                for (i = 0; i < GLASS_MAX_CNT; i++)
            	{
            		SysTask.LockModeSave[i] = LOCK_ON;
            		SysTask.LockMode[i] = LOCK_OFF; 			//换表前先关锁
            		
    				GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
    				GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
    				GPIO_SetBits(LOCK_GPIO, LOCK_03_PIN);
    				GPIO_SetBits(LOCK_GPIO, LOCK_04_PIN);
            	}
                    
#if DEBUG
				u3_printf(" USART  SysTask.GlassNum: %d,  SysTask.u16StepRequire: %d /n", * (data + 2), SysTask.u16StepRequire);
#endif
			}
			else 
			{
				//发送到平板端，应答	 命令错误
				g_FingerAck 		= ACK_ERROR;
				g_AckType			= ERROR_ACK_TYPE;

				if (SysTask.SendState == SEND_IDLE)
					SysTask.SendState = SEND_TABLET_FINGER;

			}

			break;

		default:
			//发送到平板端，应答	 命令错误
			g_FingerAck = ACK_ERROR;
			g_AckType = ERROR_ACK_TYPE;

			if (SysTask.SendState == SEND_IDLE)
				SysTask.SendState = SEND_TABLET_FINGER;

			break;
	}

	return 0; //		   
}


/*******************************************************************************
* 名称: 
* 功能: 平板给主机发送的命令包处理
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void TabletToHostTask(void)
{
	if (USART1_RX_STA & 0X8000) //接收到平板数据
	{
		Usart1JudgeStr();
		USART1_RX_STA		= 0;					//清空状态
	}

	//	  if(USART3_RX_STA & 0X8000)//接收到平板数据
	//	  {
	//		  Usart3JudgeStr();
	//		  USART3_RX_STA   =   0; //清空状态
	//	  }
}


/*******************************************************************************
* 名称: 
* 功能: 主机发送全部状态给平板  零点
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SendToTablet_Zero(u8 u8GlassAddr)
{
	//	  包头 + 地址 + 命令 + 数据包[48] + 校验和 + 包尾
	//	  包头：0XAA
	//	  手表地址：0X8~：最高位为1表示全部						~:表示有几个手表
	//				0x01：第一个手表
	//	  命令：
	//	  数据：（锁开关 + 方向 + 时间）* 16 = 48
	//	  校验和：
	//	  包尾： 0X55
	u8 i;
	u8 u8aSendArr[53]	=
	{
		0
	};
	SendArrayUnion_u SendArrayUnion;

	u8aSendArr[1]		= MCU_ACK_ADDR; 			//应答地址

	u8aSendArr[2]		= ZERO_ACK_TYPE;			//应答类型零点
	u8aSendArr[3]		= u8GlassAddr;				//手表地址

	u8aSendArr[0]		= 0xAA;

	for (i = 0; i < 47; i++)
	{
		u8aSendArr[4 + i]	= SendArrayUnion.u8SendArray[i];
	}

	u8aSendArr[51]		= CheckSum(u8aSendArr, 51); //校验和
	u8aSendArr[52]		= 0x55; 					//包尾s 

	for (i = 0; i < 53; i++)
	{
		Usart1_SendByte(u8aSendArr[i]);
	}
}


/*******************************************************************************
* 名称: 
* 功能: 主机发送全部状态给平板
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SendToTablet(u8 u8GlassAddr)
{
	//	  包头 + 地址 + 命令 + 数据包[48] + 校验和 + 包尾
	//	  包头：0XAA
	//	  手表地址：0X8~：最高位为1表示全部						~:表示有几个手表
	//				0x01：第一个手表
	//	  命令：
	//	  数据：（锁开关 + 方向 + 时间）* 16 = 48
	//	  校验和：
	//	  包尾： 0X55
	u8 i;
	u8 u8aSendArr[53]	=
	{
		0
	};
	SendArrayUnion_u SendArrayUnion;

	if (u8GlassAddr == MCU_ACK_ADDR) //应答地址
	{
		u8aSendArr[2]		= g_AckType;			//应答类型
		u8aSendArr[3]		= g_FingerAck;			//命令数据	应答错误类型
	}
	else 
	{
		memcpy(SendArrayUnion.WatchState, SysTask.WatchState, sizeof(SysTask.WatchState));

		u8aSendArr[2]		= 0x11; 				//命令数据保留用 

		for (i = 0; i < 48; i++)
		{
			u8aSendArr[3 + i]	= SendArrayUnion.u8SendArray[i];
		}
	}

	u8aSendArr[0]		= 0xAA;
	u8aSendArr[1]		= u8GlassAddr;
	u8aSendArr[51]		= CheckSum(u8aSendArr, 51); //校验和
	u8aSendArr[52]		= 0x55; 					//包尾s 

	for (i = 0; i < 53; i++)
	{
		Usart1_SendByte(u8aSendArr[i]);
	}
}


/*******************************************************************************
* 名称: 
* 功能: 主机发送传感器状态给平板
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SendToTabletDetect(u8 u8GlassAddr)
{
	//	  包头 + 地址 + 命令 + 数据包[48] + 校验和 + 包尾
	//	  包头：0XAA
	//	  手表地址：0X8~：最高位为1表示全部						~:表示有几个手表
	//				0x01：第一个手表
	//	  命令：
	//	  数据：（锁开关 + 方向 + 时间）* 16 = 48
	//	  校验和：
	//	  包尾： 0X55
	u8 i;
	u8 u8aSendArr[53]	=
	{
		0
	};

	u8aSendArr[0]		= 0xAA;
	u8aSendArr[2]		= g_AckType;				//应答类型
	u8aSendArr[3]		= g_FingerAck;				//命令数据	应答错误类型
	u8aSendArr[1]		= u8GlassAddr;
	u8aSendArr[51]		= CheckSum(u8aSendArr, 51); //校验和
	u8aSendArr[52]		= 0x55; 					//包尾s 

	for (i = 0; i < 53; i++)
	{
		Usart1_SendByte(u8aSendArr[i]);
	}
}


/*******************************************************************************
* 名称: 
* 功能: 发送给平板的任务
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SendTabletTask(void)
{
	switch (SysTask.SendState)
	{
		case SEND_INIT:
			if (SysTask.bTabReady == TRUE)
			{
				//SLAVE_SEND_ALL; 
				//				  g_GlassSendAddr = GLASS_SEND_ALL | GLASS_MAX_CNT;
				SendToTablet(GLASS_SEND_ALL | GLASS_MAX_CNT); //代表手表个数
				SysTask.SendState	= SEND_IDLE;
			}

			break;

		case SEND_TABLET_FINGER: //发送模块应答给平板
			SendToTablet(MCU_ACK_ADDR); //
			SysTask.SendState = SEND_IDLE;
			break;

		case SEND_TABLET_DETECT: //发送传感器状态给平板
			SendToTablet(MCU_ACK_ADDR); //
			SysTask.SendState = SEND_IDLE;
			break;

		case SEND_TABLET_ZERO: //发送模块零点给平板,错过了就不发了，平板端有超时机制
			SendToTablet_Zero(g_GlassSendAddr); //	这样的话只能发送最后一个的了，如果存在总线冲突，按照人的手速来再次点击可以在200ms以上，所以道理上来说冲突的机率还是很小的
			SysTask.SendState = SEND_IDLE;
			break;

		/* 现在阶段没有使用这部分的内容，因为不做应答检验，所以下面这部分暂时用不?
			?
		case SEND_TABLET_SINGLE://发送单个从机状态给平板
			SendToTablet(g_SlaveSendAddr, g_GlassSendAddr);
			break;
		*/
		case SEND_IDLE:
		default:
			SysTask.SendState = SEND_IDLE;
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 主机接发送给从机发送的命令包处理
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void HostToTabletTask(void)
{
	//	  static u8 i = 0;
	//	  static u8 j = 0;
	//	  static u8 state = 0;
	//SysTask.WatchStateSave, SysTask.SlaveState
	//	  switch(state)
	//		  {
	//		  case HtoS_CHECK:
	//			  if(	  (SysTask.WatchStateSave.MotoTime != SysTask.WatchState.MotoTime)
	//				 ||   (SysTask.WatchStateSave.MotoTime != SysTask.WatchState.MotoTime)
	//				 ||   (SysTask.WatchStateSave.MotoTime != SysTask.WatchState.MotoTime))
	//			  {
	//				  state = HtoS_SEND;
	//			  }
	//			  else
	//			  {
	//				  if(i == 7){
	//					  j++;
	//					  if(j == 4) j = 0;
	//					  i = 0;
	//				   }
	//				  else
	//					  i++;
	//			  }
	//			  break;
	//		  case HtoS_SEND:
	//			  SysTask.WatchState.MotoTime = SysTask.WatchStateSave.MotoTime;
	//			  SysTask.WatchState.MotoTime = SysTask.WatchStateSave.MotoTime;
	//			  SysTask.WatchState.MotoTime = SysTask.WatchStateSave.MotoTime;
	//			  state = HtoS_WAIT;
	//			  break;
	//		  case HtoS_WAIT:
	//			  if(SysTask.u16HtoSWaitTime == 0)
	//				  state = HtoS_CHECK;
	//			  break;
	//		  default:
	//			  break;
	//	  }
}


#define OPEN_DOOR_TIME			420

/*******************************************************************************
* 名称: 
* 功能: 左边门，。。只有开关门。可以由下降的事件驱动。关门有传感器，开门用时间控
	制 ，没有传感器 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_01_Task(void)
{
	//升降电机
	switch (SysTask.Moto_State[GLASS_01])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_01] != 0)
				break;

			SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_NOR;

			if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD) //正转，开门
			{
#if DEBUG
				u3_printf(" Moto_01_Task 正转，开门\n");
#endif

				SysTask.Moto_RunTime[GLASS_01] = OPEN_DOOR_TIME; //
				SET_DOOR_L_DIR_P();
				Door_L_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP;
			}
			else 
			{
#if DEBUG
				u3_printf(" Moto_01_Task 关门\n");
#endif

				SET_DOOR_L_DIR_N();
				Door_L_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN; //关门
				SysTask.Moto_RunTime[GLASS_01] = 500; //强制时间,时间到就调整PWM占空比减速下去
			}

			SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];
			break;

		case MOTO_STATE_RUN_NOR: //普通运行状态
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01]) //中途模式改变
			{
				Door_L_MotoStop();					//先暂停电机转动  --
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
				SysTask.Moto_StateTime[GLASS_01] = 10;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_UP: //开门
					if (SysTask.Moto_RunTime[GLASS_01] == 0) //超时退出
					{
#if DEBUG
						u3_printf(" Moto_01_Task 暂停电机\n");
#endif

						Door_L_MotoStop();			//先暂停电机转动  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}

					break;

				case MOTO_SUB_STATE_DOWN: //关门 
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_L) == 1))
					{
#if DEBUG
						u3_printf(" Moto_01_Task 零点暂停电机\n");
#endif

						Door_L_MotoStop();			//先暂停电机转动  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //超时
					{
#if DEBUG
						u3_printf(" Moto_01_Task 减速\n");
#endif

						SET_DOOR_L_DIR_N();
						Door_L_MotoStart(MOTO_PUTY_10);

						SysTask.Moto_RunTime[GLASS_01] = 600;
						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DEC;
					}

					break;

				case MOTO_SUB_STATE_DEC: //减速
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_L) == 1))
					{
#if DEBUG
						u3_printf(" Moto_01_Task 减速--零点暂停电机\n");
#endif

						Door_L_MotoStop();			//先暂停电机转动  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //超时
					{
#if DEBUG
						u3_printf(" Moto_01_Task 超时暂停电机\n");
#endif

						SysTask.Moto_RunTime[GLASS_01] = 100;
						Door_L_MotoStop();			//先暂停电机转动  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}

					break;

				default:
					SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					break;
			}

			break;

		case MOTO_STATE_WAIT:
			break;

		case MOTO_STATE_IDLE:
			//			if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD)
			//				SysTask.Moto_Mode[GLASS_01] = MOTO_FR_REV;
			//			else 
			//				SysTask.Moto_Mode[GLASS_01] = MOTO_FR_FWD;
			//			SysTask.Moto_StateTime[GLASS_01] = 2000;
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01])
			{
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
			}

			break;

		default:
			SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 只有正反转的，开关门.可以由下降的事件驱动
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_02_Task(void)
{
	//升降电机
	switch (SysTask.Moto_State[GLASS_02])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_02] != 0)
				break;

			SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_NOR;

			if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_FWD) //正转，开门
			{
#if DEBUG
				u3_printf(" Moto_02_Task 正转，开门\n");
#endif

				SysTask.Moto_RunTime[GLASS_02] = OPEN_DOOR_TIME; //
				SET_DOOR_R_DIR_P();
				Door_R_MotoStart(MOTO_PUTY_99);

				SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_UP;
			}
			else 
			{
#if DEBUG
				u3_printf(" Moto_02_Task 关门\n");
#endif

				SET_DOOR_R_DIR_N();
				Door_R_MotoStart(MOTO_PUTY_99);

				SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_DOWN; //关门
				SysTask.Moto_RunTime[GLASS_02] = 450; //强制时间,时间到就调整PWM占空比减速下去
			}

			SysTask.Moto_ModeSave[GLASS_02] = SysTask.Moto_Mode[GLASS_02];
			break;

		case MOTO_STATE_RUN_NOR: //普通运行状态
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02]) //中途模式改变
			{
				Door_R_MotoStop();					//先暂停电机转动  --
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //先延时一段时间再翻转
				SysTask.Moto_StateTime[GLASS_02] = 10;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_02])
			{
				case MOTO_SUB_STATE_UP: //开门
					if (SysTask.Moto_RunTime[GLASS_02] == 0) //超时退出
					{
#if DEBUG
						u3_printf(" Moto_02_Task 暂停电机\n");
#endif

						Door_R_MotoStop();			//先暂停电机转动  --

						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}

					break;

				case MOTO_SUB_STATE_DOWN: //关门 
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_R) == 1))
					{
#if DEBUG
						u3_printf(" Moto_02_Task 零点暂停电机\n");
#endif

						Door_R_MotoStop();			//先暂停电机转动  --

						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_02] == 0) //超时
					{

#if DEBUG
						u3_printf(" Moto_02_Task 减速\n");
#endif

						SET_DOOR_R_DIR_N();
						Door_R_MotoStart(MOTO_PUTY_10);

						SysTask.Moto_RunTime[GLASS_02] = 400;
						SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_DEC;
					}

					break;

				case MOTO_SUB_STATE_DEC: //减速
					if (GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_R) == 1)
					{
#if DEBUG
						u3_printf(" Moto_02_Task 减速--零点暂停电机\n");
#endif

						Door_R_MotoStop();			//先暂停电机转动  --
						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_02] == 0) //超时
					{
#if DEBUG
						u3_printf(" Moto_02_Task 超时暂停电机\n");
#endif

						SysTask.Moto_RunTime[GLASS_02] = 100;
						Door_R_MotoStop();			//先暂停电机转动  --
						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}
					break;

				default:
					SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					break;
			}

			break;

		case MOTO_STATE_WAIT:
			break;

		case MOTO_STATE_IDLE:
			//			if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_FWD)
			//				SysTask.Moto_Mode[GLASS_02] = MOTO_FR_REV;
			//			else 
			//				SysTask.Moto_Mode[GLASS_02] = MOTO_FR_FWD;
			//			SysTask.Moto_StateTime[GLASS_02] = 2000;
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02])
			{
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //先延时一段时间再翻转
			}

			break;

		default:
			SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 中间的 升降， 下降时有个零点检测 ，上升使用时间做判断。正转升，反转降
* 形参:		
* 返回: 无
* 说明:
*******************************************************************************/
void Moto_03_Task(void)
{
	//升降电机
	switch (SysTask.Moto_State[GLASS_03])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_03] != 0)
				break;

			SysTask.Moto_State[GLASS_03] = MOTO_STATE_RUN_NOR;

			if (SysTask.Moto_Mode[GLASS_03] == MOTO_FR_FWD) //正转，上升
			{
#if DEBUG
				u3_printf(" Moto_03_Task 正转，上升\n");
#endif

				SysTask.Moto_RunTime[GLASS_03] = SysTask.nTick + 200; //强制时间，up to this time full speed rise
				SET_RISE_STEP_DIR_P();
				Rise_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_UP;
			}
			else 
			{
#if DEBUG
				u3_printf(" Moto_03_Task 下降\n");
#endif

				if (GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_D) == 0)
				{
#if DEBUG
					u3_printf(" Moto_03_Task 已经最低\n");
#endif

					SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					SysTask.Moto_ModeSave[GLASS_03] = SysTask.Moto_Mode[GLASS_03];
					break;
				}

				SET_RISE_STEP_DIR_N();
				Rise_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_DOWN; //下降
				SysTask.Moto_RunTime[GLASS_03] = SysTask.nTick + 1000; ////强制时间,时间到就调整PWM占空比减速下去
			}

			SysTask.Moto_ModeSave[GLASS_03] = SysTask.Moto_Mode[GLASS_03];
			break;

		case MOTO_STATE_RUN_NOR: //普通运行状态
			if (SysTask.Moto_Mode[GLASS_03] != SysTask.Moto_ModeSave[GLASS_03]) //中途模式改变
			{
				Rise_MotoStop();
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT; //先延时一段时间再翻转
				SysTask.Moto_StateTime[GLASS_03] = 10;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_03])
			{
				case MOTO_SUB_STATE_UP:
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_U) == 0))
					{
#if DEBUG
						u3_printf(" Moto_03_Task up零点停止\n");
#endif

						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) // full speed
					{
#if DEBUG
						u3_printf(" Moto_03_Task up  full speed \n");
#endif
                        SET_RISE_STEP_DIR_P();
                        Rise_MotoStart(MOTO_PUTY_99);

                        SysTask.Moto_RunTime[GLASS_03] = 1700;
                        SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_FULL;
					}

					break;
				case MOTO_SUB_STATE_FULL:
                    if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_U) == 0))
					{
#if DEBUG
						u3_printf(" Moto_03_Task up 零点停止\n");
#endif

						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) //超时退出
					{
#if DEBUG
						u3_printf(" Moto_03_Task up 超时退出 \n");
#endif
						SysTask.Moto_RunTime[GLASS_03] = 100;
						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
                    break;

				case MOTO_SUB_STATE_DOWN:
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_D) == 0))
					{
#if DEBUG
						u3_printf(" Moto_03_Task down零点停止\n");
#endif

						SysTask.Moto_StateTime[GLASS_03] = 200;
						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_WAIT; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) //减速
					{

#if DEBUG
						u3_printf(" Moto_03_Task 减速\n");
#endif
                        
                        SET_RISE_STEP_DIR_N();
                        Rise_MotoStart(MOTO_PUTY_30);

						SysTask.Moto_RunTime[GLASS_03] = 2000;
						SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_DEC;
					}

					break;

				case MOTO_SUB_STATE_DEC: //减速
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_D) == 0))
					{
#if DEBUG
						u3_printf(" Moto_03_Task 减速--零点暂停电机\n");
#endif

						Rise_MotoStop();

						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) //超时
					{
#if DEBUG
						u3_printf(" Moto_03_Task 减速--超时暂停电机\n");
#endif

						SysTask.Moto_RunTime[GLASS_03] = 100;
						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
                    
                    break;

				default:
					SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					break;
			}

			break;

		case MOTO_STATE_WAIT:
			SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
			break;

		case MOTO_STATE_IDLE:
			//			if (SysTask.Moto_Mode[GLASS_03] == MOTO_FR_FWD)
			//				SysTask.Moto_Mode[GLASS_03] = MOTO_FR_REV;
			//			else 
			//				SysTask.Moto_Mode[GLASS_03] = MOTO_FR_FWD;
			//			SysTask.Moto_StateTime[GLASS_03] = 2000;
			if (SysTask.Moto_Mode[GLASS_03] != SysTask.Moto_ModeSave[GLASS_03])
			{
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT; //先延时一段时间再翻转
			}

			break;

		default:
			SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_04_Task(void)
{
	StepMotoData_t * pStepMotoData;

	//_03电机
	switch (SysTask.Moto_State[GLASS_04])
	{
		case MOTO_STATE_INIT:
			if (g_StepMotoEvent.bMutex == FALSE)
			{
#if DEBUG
				u3_printf(" Moto_04_Task 转第一个表\n");
#endif

				g_StepMotoEvent.bMutex = TRUE;		//上锁，只操作一次

				//g_StepMotoEvent.MachineCtrl.u32Timeout = 6000; //1S 超时  ,5次就是6S
				g_StepMotoEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
				g_StepMotoEvent.MachineRun.bFinish = FALSE;
				pStepMotoData		= (StepMotoData_t *)
				g_StepMotoEvent.MachineCtrl.pData;
				g_StepMotoEvent.MachineCtrl.CallBack(STEP_FIND_ZERO, &g_StepMotoEvent);
			}
			else //取结果
			{
				g_StepMotoEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
				g_StepMotoEvent.MachineCtrl.CallBack(STEP_FIND_ZERO, &g_StepMotoEvent);
			}

			if (g_StepMotoEvent.MachineRun.bFinish == TRUE)
			{
#if DEBUG
				u3_printf(" Moto_04_Task bFinish\n");
#endif

				SysTask.Moto_State[GLASS_04] = MOTO_STATE_IDLE;
				g_StepMotoEvent.bMutex = FALSE;
			}

			break;

		case MOTO_STATE_RUN_NOR: //普通运行状态 1/4 2/4 3/4 圈，如果是去第一个表为了减小误差使用查找零点的方式
			if (g_StepMotoEvent.bMutex == FALSE)
			{
#if DEBUG
				u3_printf(" Moto_04_Task 转动 %d	/n", SysTask.u16StepRequire);
#endif

				g_StepMotoEvent.bMutex = TRUE;		//上锁，只操作一次
				g_StepMotoEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
				g_StepMotoEvent.MachineRun.bFinish = FALSE;
				pStepMotoData		= (StepMotoData_t *)
				g_StepMotoEvent.MachineCtrl.pData;
				pStepMotoData->u16Step = SysTask.u16StepRequire;
				g_StepMotoEvent.MachineCtrl.CallBack(STEP_RUN, &g_StepMotoEvent);
			}
			else //取结果
			{
				g_StepMotoEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
				g_StepMotoEvent.MachineCtrl.CallBack(STEP_RUN, &g_StepMotoEvent);
			}

			if (g_StepMotoEvent.MachineRun.bFinish == TRUE)
			{
#if DEBUG
				u3_printf(" Moto_04_Task bFinish\n");
#endif

				SysTask.Moto_State[GLASS_04] = MOTO_STATE_IDLE;
				g_StepMotoEvent.bMutex = FALSE;
			}

			break;

		case MOTO_STATE_IDLE:
			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
			 ___
			|1ms|	3ms |	 
			 ___		 ___		___ 	
开： _______| 	|_______|	|______|   |______

			|1ms|	 10ms	   |	
			 ___				___ 			 ___	 
关： _______| 	|______________|   |____________|	|__________

* 返回: 无
* 说明: 
*******************************************************************************/
void Lock_01_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_ONE)    //当前展示的是第几个表,只有展示的表才可以开锁，防止误操作，表表掉下
        return;
    
	switch (SysTask.Lock_State[GLASS_01]) //_02锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			if (SysTask.LockMode[GLASS_01] != SysTask.LockModeSave[GLASS_01])
			{
				if (SysTask.LockMode[GLASS_01] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_01_Task 开锁\n");
#endif

					SysTask.Lock_StateTime[GLASS_01] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
					SysTask.Lock_OffTime[GLASS_01] = 0;
				}
				else //将要关锁
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_01_Task 将要关锁\n");
#endif
					SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
					SysTask.Lock_StateTime[GLASS_01] = 0;
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_CLOSE;
				}
			}

			break;

		case LOCK_STATE_OPEN:
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN);
				SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;
			break;

		case LOCK_STATE_CLOSE: //
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

#if DEBUG_MOTO2
				u3_printf(" Lock_01_Task 关锁完成\n");
#endif
				SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;
				GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
				SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_OFF_TIME; //停止电平
				SysTask.LockMode[GLASS_01] = LOCK_OFF;
	
			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:	
* 返回: 无
* 说明: 
*******************************************************************************/
void Lock_02_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_TWO)    //当前展示的是第几个表,只有展示的表才可以开锁，防止误操作，表表掉下
        return;
    
	switch (SysTask.Lock_State[GLASS_02]) //_02锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;

			if (SysTask.LockMode[GLASS_02] != SysTask.LockModeSave[GLASS_02])
			{
				if (SysTask.LockMode[GLASS_02] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_02_Task 开锁\n");
#endif
					SysTask.Lock_StateTime[GLASS_02] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_02] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_02] = SysTask.LockMode[GLASS_02];
					SysTask.Lock_OffTime[GLASS_02] = 0;
				}
				else //将要关锁
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_02_Task 将要关锁\n");
#endif
					SysTask.LockModeSave[GLASS_02] = SysTask.LockMode[GLASS_02];
					SysTask.Lock_StateTime[GLASS_02] = 0;
					SysTask.Lock_State[GLASS_02] = LOCK_STATE_CLOSE;
				}
			}

			break;

		case LOCK_STATE_OPEN:
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;


					GPIO_ResetBits(LOCK_GPIO, LOCK_02_PIN);
				SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;

			break;

		case LOCK_STATE_CLOSE: //
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;

#if DEBUG_MOTO2
				u3_printf(" Lock_02_Task 关锁完成\n");
#endif
				SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;
				GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
				SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_OFF_TIME; //停止电平
				SysTask.LockMode[GLASS_02] = LOCK_OFF;

			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Lock_03_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_THREE)    //当前展示的是第几个表,只有展示的表才可以开锁，防止误操作，表表掉下
        return;
    
	switch (SysTask.Lock_State[GLASS_03]) //_02锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_03] != 0)
				break;

			if (SysTask.LockMode[GLASS_03] != SysTask.LockModeSave[GLASS_03])
			{
				if (SysTask.LockMode[GLASS_03] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_03_Task 开锁\n");
#endif
					SysTask.Lock_StateTime[GLASS_03] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_03] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_03] = SysTask.LockMode[GLASS_03];
					SysTask.Lock_OffTime[GLASS_03] = 0;
				}
				else //将要关锁
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_03_Task 将要关锁\n");
#endif
					SysTask.LockModeSave[GLASS_03] = SysTask.LockMode[GLASS_03];
					SysTask.Lock_StateTime[GLASS_03] = 0;
					SysTask.Lock_State[GLASS_03] = LOCK_STATE_CLOSE;
				}
			}

			break;

		case LOCK_STATE_OPEN:
			if (SysTask.Lock_StateTime[GLASS_03] != 0)
				break;

				GPIO_ResetBits(LOCK_GPIO, LOCK_03_PIN);
				SysTask.Lock_State[GLASS_03] = LOCK_STATE_DETECT;

			break;

		case LOCK_STATE_CLOSE: //
			if (SysTask.Lock_StateTime[GLASS_03] != 0)
				break;

#if DEBUG
				u3_printf(" Lock_03_Task 关锁完成\n");
#endif

				SysTask.Lock_State[GLASS_03] = LOCK_STATE_DETECT;
				GPIO_SetBits(LOCK_GPIO, LOCK_03_PIN);
				SysTask.Lock_StateTime[GLASS_03] = SysTask.nTick + LOCK_SET_OFF_TIME; //停止电平
				SysTask.LockMode[GLASS_03] = LOCK_OFF;

			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:	只用开锁	
* 返回: 无
* 说明: 
*******************************************************************************/
void Lock_04_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_FOUR)    //当前展示的是第几个表,只有展示的表才可以开锁，防止误操作，表表掉下
        return;
    
	switch (SysTask.Lock_State[GLASS_04]) //_02锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_04] != 0)
				break;

			if (SysTask.LockMode[GLASS_04] != SysTask.LockModeSave[GLASS_04])
			{
				if (SysTask.LockMode[GLASS_04] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_04_Task 开锁\n");
#endif
					SysTask.Lock_StateTime[GLASS_04] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_04] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_04] = SysTask.LockMode[GLASS_04];
					SysTask.Lock_OffTime[GLASS_04] = 0;
				}
				else //将要关锁
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_04_Task 将要关锁\n");
#endif
					SysTask.LockModeSave[GLASS_04] = SysTask.LockMode[GLASS_04];
					SysTask.Lock_StateTime[GLASS_04] = 0;
					SysTask.Lock_State[GLASS_04] = LOCK_STATE_CLOSE;
				}
			}

			break;

		case LOCK_STATE_OPEN:
			if (SysTask.Lock_StateTime[GLASS_04] != 0)
				break;

				SysTask.Lock_State[GLASS_04] = LOCK_STATE_DETECT;
				GPIO_ResetBits(LOCK_GPIO, LOCK_04_PIN);
			break;

		case LOCK_STATE_CLOSE: //
			if (SysTask.Lock_StateTime[GLASS_04] != 0)
				break;

#if DEBUG_MOTO2
				u3_printf(" Lock_04_Task 关锁完成\n");
#endif

				GPIO_SetBits(LOCK_GPIO, LOCK_04_PIN);
				SysTask.Lock_State[GLASS_04] = LOCK_STATE_DETECT;
				SysTask.Lock_StateTime[GLASS_04] = SysTask.nTick + LOCK_SET_OFF_TIME; //停止电平
				SysTask.LockMode[GLASS_04] = LOCK_OFF;
	

			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SysSaveDataTask(void)
{
	static u8 u8Status	= 0;
	static u8 u8count	= 0;
	u8 i;

	if (SysTask.bEnSaveData == FALSE)
		return;

	switch (u8Status)
	{
		case SAVE_INIT:
			memset(SysTask.FlashData.u8FlashData, 0, sizeof(SysTask.FlashData.u8FlashData)); //清0

			//			  memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
			for (i = 0; i < GLASS_MAX_CNT; i++)
			{
				SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
				SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
				SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
			}

			SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LED模式
			u8Status = SAVE_WRITE;
			break;

		case SAVE_WRITE:
			if (FLASH_WriteMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, DATA_READ_CNT + LED_MODE_CNT + PASSWD_CNT))
			{
				u8count++;
				u8Status			= SAVE_WAIT;
				SysTask.u16SaveWaitTick = 100;		// 100ms 延时再重新写入
			}
			else 
			{
				u8Status			= SAVE_EXIT;
			}

			break;

		case SAVE_WAIT:
			if (SysTask.u16SaveWaitTick == 0)
			{
				u8Status			= SAVE_WRITE;

				if (u8count++ >= 7) //每页尝试8次
				{
					if (++SysTask.u16AddrOffset < ADDR_MAX_OFFSET) //共5页
					{
						for (i = 0; i < 3; i++) //重写8次最多
						{
							if (!FLASH_WriteMoreData(g_au32DataSaveAddr[0], &SysTask.u16AddrOffset, 1))
								break;

							delay_ms(20);
						}
					}
					else 
					{
						u8Status			= SAVE_EXIT;
					}
				}
			}

			break;

		case SAVE_EXIT:
			SysTask.bEnSaveData = FALSE;
			u8Status = SAVE_INIT;
			u8count = 0;
			break;

		default:
			u8Status = SAVE_EXIT;
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SystemInitTask(void)
{
	//	if (SysTask.bReady == TRUE)
	//		return;
	switch (SysTask.InitState)
	{
		case INIT_STATE_LOCK:
			if (SysTask.u16InitWaitTime == 0)
			{
				SysTask.InitState	= INIT_STATE_DOWN;

				SysTask.Moto_Mode[GLASS_03] = MOTO_FR_REV; //这个上升电机默认上电要下降
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_03] = 0;
				SysTask.u16InitWaitTime = 800;		//不用完全降下去就可以关门
			}

			Lock_01_Task();
			Lock_02_Task();
			Lock_03_Task();
			Lock_04_Task();
			break;

		case INIT_STATE_DOWN:
			if ((SysTask.u16InitWaitTime == 0) || (SysTask.Moto_State[GLASS_03] == MOTO_STATE_IDLE))
			{
				SysTask.Moto_Mode[GLASS_01] = MOTO_FR_REV; //这个开门默认上电要关门
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_01] = 0;

				SysTask.Moto_Mode[GLASS_02] = MOTO_FR_REV; //这个开门默认上电要关门
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_02] = 0;

				SysTask.InitState	= INIT_STATE_DOOR_CLOSE;
				SysTask.u16InitWaitTime = 4000; 	//等待完全关闭
			}

			Moto_03_Task(); //中间的 升降， 上升，下降都有一个传感器感应
			break;

		case INIT_STATE_DOOR_CLOSE:
			if ((SysTask.u16InitWaitTime == 0) || 
                ((SysTask.Moto_State[GLASS_01] == MOTO_STATE_IDLE) &&
				 (SysTask.Moto_State[GLASS_02] == MOTO_STATE_IDLE)&&
				 (SysTask.Moto_State[GLASS_03] == MOTO_STATE_IDLE)))
			{
				SysTask.InitState	= INIT_STATE_ZERO;

				if (SysTask.u16StepRequire == 0) //如果是第一个表
				{
					SysTask.Moto_State[GLASS_04] = MOTO_STATE_INIT;
				}
				else 
				{
					SysTask.Moto_State[GLASS_04] = MOTO_STATE_RUN_NOR;
				}

				SysTask.u16InitWaitTime = 10000;	//等待
			}

			Moto_03_Task(); //中间的 升降， 上升，下降都有一个传感器感应
			Moto_01_Task(); //开关门，只有正反转的，关都有一个传感器感应，开用延时的方法打开 
			Moto_02_Task(); //开关门，只有正反转的，关都有一个传感器感应
			break;

		case INIT_STATE_ZERO: //转动到指定表位置
			if ((SysTask.Moto_State[GLASS_04] == MOTO_STATE_IDLE) || (SysTask.u16InitWaitTime == 0))
			{
				SysTask.InitState	= INIT_STATE_FINISH;

				if (SysTask.bGlassChange == TRUE) //如果是接收到更换表命令，准备开门上升
				{
					SysTask.Moto_Mode[GLASS_01] = MOTO_FR_FWD; //正转开门 
					SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT;
					SysTask.Moto_WaitTime[GLASS_01] = 0;


					SysTask.Moto_Mode[GLASS_02] = MOTO_FR_FWD; //
					SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT;
					SysTask.Moto_WaitTime[GLASS_02] = 0;


					SysTask.InitState	= INIT_STATE_DOOR_OPEN;
					SysTask.u16InitWaitTime = 2;	//不用完全开就可以上升
				}
				else 
				{

					SysTask.bGlassChange = FALSE;
                    SysTask.u16InitWaitTime = 0;
					SysTask.InitState	= INIT_STATE_FINISH;
				}
			}

			Moto_04_Task(); //中间的转轴电机   PWM 精确控制步进电机
			break;

		case INIT_STATE_DOOR_OPEN:
			if ((SysTask.u16InitWaitTime == 0) || ((SysTask.Moto_State[GLASS_01] == MOTO_STATE_IDLE) &&
				 (SysTask.Moto_State[GLASS_02] == MOTO_STATE_IDLE)))
			{
				SysTask.InitState	= INIT_STATE_UP;

				SysTask.Moto_Mode[GLASS_03] = MOTO_FR_FWD; //这个上升电机
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_03] = 0;
				SysTask.u16InitWaitTime = 6000; 	//不用完全开就可以上升
			}

			Moto_01_Task(); //开关门，只有正反转的，关都有一个传感器感应，开用延时的方法打开 
			Moto_02_Task(); //开关门，只有正反转的，关都有一个传感器感应
			break;

		case INIT_STATE_UP:
			if ((SysTask.u16InitWaitTime == 0) || (SysTask.Moto_State[GLASS_03] == MOTO_STATE_IDLE))
			{

#if DEBUG
				u3_printf(" INIT_STATE_UP u16InitWaitTime%d , Moto_State%d\n", SysTask.u16InitWaitTime, SysTask.Moto_State[GLASS_03]);
#endif
                SysTask.u16InitWaitTime = 0;

				SysTask.InitState	= INIT_STATE_FINISH;
				SysTask.bGlassChange = FALSE;
                    
    			//发送到平板端，应答	 命令
    			g_FingerAck = ACK_SUCCESS;
    			g_AckType = DETECT_ACK_TYPE;

    			if (SysTask.SendState == SEND_IDLE)
    				SysTask.SendState = SEND_TABLET_FINGER;
            
			}

			Moto_01_Task(); //开关门，只有正反转的，关都有一个传感器感应，开用延时的方法打开 
			Moto_02_Task(); //开关门，只有正反转的，关都有一个传感器感应
			Moto_03_Task(); //中间的 升降， 上升，下降都有一个传感器感应
			break;

		case INIT_STATE_FINISH:
			if (SysTask.u16InitWaitTime != 0)
				break;

			SendTabletTask(); //发送给平板任务
			TabletToHostTask(); //接收平板命令任务处理
			Lock_01_Task();
			Lock_02_Task();
			Lock_03_Task();
			Lock_04_Task();
			Moto_01_Task(); //开关门，只有正反转的，关有一个传感器感应
			Moto_02_Task(); //开关门，只有正反转的，关有一个传感器感应
			Moto_03_Task(); //中间的 升降， 上升，下降都有一个传感器感应
			Moto_04_Task(); //中间的转轴电机   PWM 精确控制步进电机	

			if (SysTask.bGlassChange == TRUE) //如果是接收到更换表命令
			{
				SysTask.InitState	= INIT_STATE_LOCK;
			}

			break;

		case INIT_STATE_IDLE:
		default:
			SysTask.InitState = INIT_STATE_LOCK;
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void MainTask(void)
{
	//系统上电准备时间（如果上一次是停电了还没有关锁。下降。关门。再次上电要在第一时间做好准备工作）时间。和平板上电时间差不多就行
	SystemInitTask();

	//SysSaveDataTask();
	LedTask();


}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SysEventInit(void)
{
	g_StepMotoEvent.bInit = TRUE;
	g_StepMotoEvent.bMutex = FALSE;
	g_StepMotoEvent.MachineCtrl.pData = (void *)
	malloc(sizeof(StepMotoData_t));
	g_StepMotoEvent.MachineCtrl.CallBack = CenterMoto_Event;
	g_StepMotoEvent.MachineCtrl.u16InterValTime = 5; //5ms 进入一次

	g_StepMotoEvent.MachineRun.bEnable = TRUE;
	g_StepMotoEvent.MachineRun.u32StartTime = 0;
	g_StepMotoEvent.MachineRun.u32LastTime = 0;
	g_StepMotoEvent.MachineRun.u32RunCnt = 0;
	g_StepMotoEvent.MachineRun.bFinish = FALSE;
	g_StepMotoEvent.MachineRun.u16RetVal = 0;
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SysInit(void)
{
	u8 i, j;

	SysTask.u16AddrOffset = FLASH_ReadHalfWord(g_au32DataSaveAddr[0]); //读取
	memset(SysTask.FlashData.u8FlashData, 0, sizeof(SysTask.FlashData.u8FlashData));

	if ((SysTask.u16AddrOffset == 0xffff) || (SysTask.u16AddrOffset >= ADDR_MAX_OFFSET) ||
		 (SysTask.u16AddrOffset == 0)) //初始状态 flash是0xffff
	{
		SysTask.u16AddrOffset = 1;					//从第一页开始写入

		for (i = 0; i < 8; i++) //重写8次最多
		{
			if (!FLASH_WriteMoreData(g_au32DataSaveAddr[0], &SysTask.u16AddrOffset, 1))
				break;

			delay_ms(20);
		}

		for (i = 0; i < GLASS_MAX_CNT; i++)
		{
			SysTask.WatchState[i].u8LockState = 0;
			SysTask.WatchState[i].u8Dir = MOTO_FR_FWD;
			SysTask.WatchState[i].MotoTime = MOTO_TIME_650;

			if (i == 0)
			{
				SysTask.WatchState[i].u8Dir = MOTO_FR_FWD_REV; //只有正反转和停止模式，别无选择
			}
		}

		//memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
		for (i = 0; i < GLASS_MAX_CNT; i++)
		{
			SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
			SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
			SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
		}

		SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LED模式
		FLASH_WriteMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, sizeof(SysTask.FlashData.u16FlashData) / 2);
	}

	FLASH_ReadMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, sizeof(SysTask.FlashData.u16FlashData) / 2);


	for (i = 0; i < DATA_READ_CNT; i++)
	{
		if (SysTask.FlashData.u16FlashData[i] == 0xFFFF) //读出的FLASH有错，说明页就有问题，等待保存时再进行重新校验写入哪一页
		{
			for (j = 0; j < GLASS_MAX_CNT; j++)
			{
				SysTask.WatchState[j].u8LockState = 0;
				SysTask.WatchState[j].u8Dir = MOTO_FR_FWD;
				SysTask.WatchState[j].MotoTime = MOTO_TIME_650;

				if (i == 0)
				{
					SysTask.WatchState[i].u8Dir = MOTO_FR_FWD_REV; //只有正反转和停止模式，别无选择
				}
			}

			//memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
			for (i = 0; i < GLASS_MAX_CNT; i++)
			{
				SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
				SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
				SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
			}

			SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LED模式

			break;
		}
	}

	//	  使用memcpy会导致TIM 2 TICk无法进入中断，其他串口中断没有受到影响，原因未知，复制8个是没有问题
	//	  复制9个以上就会出现。
	//	  memcpy(SysTask.WatchState, SysTask.FlashData.u8FlashData, 10);
	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.WatchState[i].MotoTime = SysTask.FlashData.Data.WatchState[i].MotoTime;
		SysTask.WatchState[i].u8Dir = SysTask.FlashData.Data.WatchState[i].u8Dir;

		//		  SysTask.WatchState[i].u8LockState   = SysTask.FlashData.Data.WatchState[i].u8LockState;  //锁状态现在不用上传，由下边单片机保证
	}

	SysTask.LED_Mode	= SysTask.FlashData.Data.u16LedMode;

	if ((SysTask.LED_Mode != LED_MODE_ON) && (SysTask.LED_Mode != LED_MODE_OFF))
	{
		SysTask.LED_Mode	= LED_MODE_ON;			//LED模式
	}

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.Moto_Mode[i] = (MotoFR) (SysTask.WatchState[i].u8Dir);
		SysTask.Moto_Time[i] = (MotoTime_e) (SysTask.WatchState[i].MotoTime);
		SysTask.Moto_RunTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][1];
		SysTask.Moto_WaitTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][2];
	}

	SysEventInit();
	SysTask.TouchState	= TOUCH_MATCH_INIT;
	SysTask.TouchSub	= TOUCH_SUB_INIT;
	SysTask.bEnFingerTouch = FALSE;
	SysTask.nWaitTime	= 0;
	SysTask.nTick		= 0;
	SysTask.u16SaveTick = 0;
	SysTask.u16PWMVal	= 0;
	SysTask.bEnSaveData = FALSE;
	SysTask.Moto3_SubStateTime = 0;
	SysTask.u8Usart1RevTime = 0;
	SysTask.u8Usart2RevTime = 0;

	SysTask.bReady		= FALSE;
	SysTask.InitState	= INIT_STATE_LOCK;			//上电先关锁
	SysTask.u16InitWaitTime = 400;					//这个延时是 给关锁用的时间

	SysTask.SendState	= SEND_IDLE;
	SysTask.SendSubState = SEND_SUB_INIT;
	SysTask.bTabReady	= FALSE;
	SysTask.LED_ModeSave = LED_MODE_DEF;			// 保证第一次可以运行模式
	SysTask.GlassNum	= GLASS_NUM_ONE;
	SysTask.bGlassChange = FALSE;
	SysTask.u16StepRequire = 0;

    SysTask.WatchDisplay = DISPLAY_DEF;    //当前展示的是第几个表
	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.LockModeSave[i] = LOCK_ON;
		SysTask.LockMode[i] = LOCK_OFF; 			//上电第一件事是关锁
		GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
		GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
		GPIO_SetBits(LOCK_GPIO, LOCK_03_PIN);
		GPIO_SetBits(LOCK_GPIO, LOCK_04_PIN);
	}

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.PWM_State[i] = PWM_STATE_INIT;
	}

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.PWM_RunTime[i] = 0;
	}

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.Moto_StateTime[i] = 0;
		SysTask.Moto_State[i] = MOTO_STATE_INIT;	//保证开机起来会转
		SysTask.Moto_SubState[i] = MOTO_SUB_STATE_RUN;
		SysTask.Lock_StateTime[i] = 50; 			//这个要关锁 10ms再进去 	，不然第一个波形只有900多us 高电平  
	}

	SysTask.Moto_Mode[GLASS_02] = MOTO_FR_REV;		//这个上升电机默认上电要下降
	SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT;
	SysTask.Moto_WaitTime[GLASS_02] = 0;

	SysTask.Moto_Mode[GLASS_01] = MOTO_FR_REV;		//这个开门默认上电要关门
	SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT;
	SysTask.Moto_WaitTime[GLASS_01] = 0;

	//SysTask.u16BootTime = 3000;	//3秒等待从机启动完全
}


