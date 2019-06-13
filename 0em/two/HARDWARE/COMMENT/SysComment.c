

#include <math.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "usart1.h" 
#include "usart3.h" 
#include "delay.h"

#include "SysComment.h"
#include "stm32f10x_it.h"
#include "stm32f10x_iwdg.h"
#include "san_flash.h"
#include "step_driver.h"

#include "as608.h"
#include "led.h"

#include "range_sensor.h"


#define SLAVE_WAIT_TIME 		3000	//3s 从机应答时间

#define LOCK_SET_L_TIME 		50	  //低电平时间 25 ~ 60ms
#define LOCK_SET_H_TIME 		50	  //高电平时间
#define LOCK_SET_OFF_TIME		1200	  //给单片机反应时间，电机回正之后再转


#define MOTO_UP_TIME			800			//电机上升时间
#define MOTO_DOWN_TIME			1500		//电机下降时间




#define MOTO_TRUN_OVER			500			//MOTO 3  电机电平翻转时间


#define MOTO_ANGLE_TIME 		(750 * PWM_PERIOD / PWM_POSITIVE)  //电机转动一定角度的时间
#define MOTO_DEVIATION_TIME 	700  //电机转动时间偏差 200ms 最大的偏差来自于写flash时，因为写flash禁止一切 flash操作（代码的读取），包括中断，

//注意，由于串口接收会占用一定的时间，这里的延时要相应加长，特别是当安卓发送停止命令过来的时候，如果是在继续正转、反转时候发，这个已经多走了一段，往回走是要相应加长时间
//因为
#define LOCK_OPEN_TIME			500			//开锁给低电平时间




PollEvent_t 	g_RangeEvent;
   
float           g_LedArr[LED_BRIGHNESS]; //数组存储加速过程中每一步的装载值


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
u8				g_GlassSendAddr = 0; //要发送的手表地址
u8				g_FingerAck = 0; //应答数据命令
u8				g_AckType = 0; //应答类型

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


//内部函数
void SysTickConfig(void);


/*******************************************************************************
* 名称: SysTick_Handler
* 功能: 系统时钟节拍1MS
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SysTick_Handler(void)
{
	static u16		Tick_1S = 0;

	mSysTick++;
	mSysSec++;
	mTimeRFRX++;

	if (mDelay)
		mDelay--;

	if (++Tick_1S >= 1000)
	{
		Tick_1S 			= 0;

		if (mSysIWDGDog)
		{
			IWDG_ReloadCounter();					/*喂STM32内置硬件狗*/

			if ((++mSysSoftDog) > 5) /*软狗system  DOG 2S over*/
			{
				mSysSoftDog 		= 0;
				NVIC_SystemReset();
			}
		}
	}
}


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
* 名称: Sys_LayerInit
* 功能: 系统初始化
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Sys_LayerInit(void)
{
	SysTickConfig();

	mSysSec 			= 0;
	mSysTick			= 0;
	SysTask.mUpdate 	= TRUE;
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Sys_IWDGConfig(u16 time)
{
	/* 写入0x5555,用于允许狗狗寄存器写入功能 */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* 狗狗时钟分频,40K/64=0.625K()*/
	IWDG_SetPrescaler(IWDG_Prescaler_64);

	/* 喂狗时间 TIME*1.6MS .注意不能大于0xfff*/
	IWDG_SetReload(time);

	/* 喂狗*/
	IWDG_ReloadCounter();

	/* 使能狗狗*/
	IWDG_Enable();
}


/*******************************************************************************
* 名称: Sys_IWDGReloadCounter
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Sys_IWDGReloadCounter(void)
{
	mSysSoftDog 		= 0;						//喂软狗
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


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void FingerTouchTask(void)
{
	SearchResult seach;
	u8 ensure;
	static u8 u8MatchCnt = 0;						//匹配失败次数，默认匹配MATCH_FINGER_CNT次 

	if ((SysTask.bEnFingerTouch == TRUE))
	{
		switch (SysTask.TouchState)
		{
			case TOUCH_MATCH_INIT:
				if (PS_Sta) //有指纹按下
				{
					SysTask.nWaitTime	= 10;		//一段时间再检测
					SysTask.TouchState	= TOUCH_MATCH_AGAIN;
					u8MatchCnt			= 0;
				}

				break;

			case TOUCH_MATCH_AGAIN:
				if (SysTask.nWaitTime == 0) //有指纹按下
				{
					if (PS_Sta) //有指纹按下
					{
						ensure				= PS_GetImage();

						if (ensure == 0x00) //生成特征成功
						{
							//PS_ValidTempleteNum(&u16FingerID); //读库指纹个数
							ensure				= PS_GenChar(CharBuffer1);

							if (ensure == 0x00) //生成特征成功
							{
								ensure				= PS_Search(CharBuffer1, 0, FINGER_MAX_CNT, &seach);

								if (ensure == 0) //匹配成功
								{
									SysTask.u16FingerID = seach.pageID;
									SysTask.nWaitTime	= SysTask.nTick + 2000; //延时一定时间 成功也不能一直发
									SysTask.TouchSub	= TOUCH_SUB_INIT;
									SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配

									//								  SysTask.bEnFingerTouch = FALSE;	 //成功，则禁止指纹模块任务
									//匹配成功，发送到平板端，应答
									g_FingerAck 		= ACK_SUCCESS;
									g_AckType			= FINGER_ACK_TYPE;

									if (SysTask.SendState == SEND_IDLE)
										SysTask.SendState = SEND_TABLET_FINGER;
								}
								else if (u8MatchCnt >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
								{
									//								  SysTask.bEnFingerTouch = FALSE;	 //成功，则禁止指纹模块任务
									//匹配失败，发送到平板端，应答
									g_FingerAck 		= ACK_FAILED;
									g_AckType			= FINGER_ACK_TYPE;

									if (SysTask.SendState == SEND_IDLE)
										SysTask.SendState = SEND_TABLET_FINGER;

									SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
									SysTask.nWaitTime	= SysTask.nTick + 300; //延时一定时间 如果手臭识别不了，再想来一个人检测，
								}
							}
						}

						if (ensure) //匹配失败
						{
							SysTask.nWaitTime	= SysTask.nTick + 10; //延时一定时间 这里检测到的应该是同一个人的指纹
							u8MatchCnt++;
						}
					}
				}

				break;

			case TOUCH_ADD_USER:
				{
					switch (SysTask.TouchSub)
					{
						case TOUCH_SUB_INIT: // *
							if (SysTask.nWaitTime == 0)
							{
								SysTask.TouchSub	= TOUCH_SUB_ENTER;
							}

							break;

						case TOUCH_SUB_ENTER: //
							if ((PS_Sta) && (SysTask.nWaitTime == 0)) //如果有指纹按下
							{
								ensure				= PS_GetImage();

								if (ensure == 0x00)
								{
									ensure				= PS_GenChar(CharBuffer1); //生成特征

									if (ensure == 0x00)
									{
										SysTask.nWaitTime	= 3000; //延时一定时间再去采集
										SysTask.TouchSub	= TOUCH_SUB_AGAIN; //跳到第二步				

										//Please press	agin
										//成功，发送到平板端，应答从新再按一次
										g_FingerAck 		= ACK_AGAIN;
										g_AckType			= FINGER_ACK_TYPE;

										if (SysTask.SendState == SEND_IDLE)
											SysTask.SendState = SEND_TABLET_FINGER;

									}
								}
							}

							break;

						case TOUCH_SUB_AGAIN:
							if ((PS_Sta) && (SysTask.nWaitTime == 0)) //如果有指纹按下
							{
								ensure				= PS_GetImage();

								if (ensure == 0x00)
								{
									ensure				= PS_GenChar(CharBuffer2); //生成特征

									if (ensure == 0x00)
									{
										ensure				= PS_Match(); //对比两次指纹

										if (ensure == 0x00) //成功
										{

											ensure				= PS_RegModel(); //生成指纹模板

											if (ensure == 0x00)
											{

												ensure				= PS_StoreChar(CharBuffer2, SysTask.u16FingerID); //储存模板

												if (ensure == 0x00)
												{
													SysTask.TouchSub	= TOUCH_SUB_WAIT;
													SysTask.nWaitTime	= 3000; //延时一定时间退出

													//													  SysTask.bEnFingerTouch = FALSE;	 //成功，则禁止指纹模块任务
													//成功，发送到平板端，应答
													g_FingerAck 		= ACK_SUCCESS;
													g_AckType			= FINGER_ACK_TYPE;

													if (SysTask.SendState == SEND_IDLE)
														SysTask.SendState = SEND_TABLET_FINGER;
												}
												else 
												{
													//匹配失败，发送到平板端，应答
													SysTask.TouchSub	= TOUCH_SUB_ENTER;
													SysTask.nWaitTime	= 3000; //延时一定时间退出
												}
											}
											else 
											{
												//失败，发送到平板端，应答
												g_FingerAck 		= ACK_FAILED;
												g_AckType			= FINGER_ACK_TYPE;

												if (SysTask.SendState == SEND_IDLE)
													SysTask.SendState = SEND_TABLET_FINGER;

												SysTask.TouchSub	= TOUCH_SUB_ENTER;
												SysTask.nWaitTime	= 3000; //延时一定时间退出
											}
										}
										else 
										{
											//失败，发送到平板端，应答
											g_FingerAck 		= ACK_FAILED;
											g_AckType			= FINGER_ACK_TYPE;

											if (SysTask.SendState == SEND_IDLE)
												SysTask.SendState = SEND_TABLET_FINGER;

											SysTask.TouchSub	= TOUCH_SUB_ENTER;
											SysTask.nWaitTime	= 3000; //延时一定时间退出
										}
									}
								}
							}

							break;

						case TOUCH_SUB_WAIT: // *
							if ((SysTask.nWaitTime == 0))
							{
								SysTask.TouchState	= TOUCH_IDLE;
								SysTask.bEnFingerTouch = FALSE; //成功，则禁止指纹模块任务
								SysTask.TouchSub	= TOUCH_SUB_INIT;

							}

							break;

						default:
							break;
					}
				}
				break;

			case TOUCH_DEL_USER:
				{
					switch (SysTask.TouchSub)
					{
						case TOUCH_SUB_INIT: // *
							if (SysTask.nWaitTime == 0)
							{
								u8MatchCnt			= 0;
								SysTask.TouchSub	= TOUCH_SUB_ENTER;
							}

							break;

						case TOUCH_SUB_ENTER: //
							if (SysTask.nWaitTime)
							{
								break;
							}

							ensure = PS_DeletChar(SysTask.u16FingerID, 1);

							if (ensure == 0)
							{
								//发送到平板端面
								g_FingerAck 		= ACK_SUCCESS;
								g_AckType			= FINGER_ACK_TYPE;

								if (SysTask.SendState == SEND_IDLE)
									SysTask.SendState = SEND_TABLET_FINGER;

								SysTask.TouchSub	= TOUCH_SUB_WAIT;
								SysTask.nWaitTime	= 100; //延时一定时间退出
							}
							else //删除失败 ,
							{
								u8MatchCnt++;
								SysTask.nWaitTime	= 500; //延时一定时间再次删除

								if (u8MatchCnt >= 3) //失败退出
								{
									//发送到平板端面
									g_FingerAck 		= ACK_FAILED;
									g_AckType			= FINGER_ACK_TYPE;

									if (SysTask.SendState == SEND_IDLE)
										SysTask.SendState = SEND_TABLET_FINGER;

									SysTask.TouchSub	= TOUCH_SUB_WAIT;
								}
							}

							break;

						case TOUCH_SUB_WAIT: // *
							if ((SysTask.nWaitTime == 0))
							{
								SysTask.TouchState	= TOUCH_IDLE;
								SysTask.bEnFingerTouch = FALSE; //成功，则禁止指纹模块任务
								SysTask.TouchSub	= TOUCH_SUB_INIT;
							}

							break;

						default:
							break;
					}
				}
				break;

			case TOUCH_IDLE:
			default:
				break;
		}
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
	//	  命令： CMD_UPDATE	 、 CMD_READY 
	//	  参数： 地址，LED模式，增加删除指纹ID,
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
    
}

/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_01_Task(void)
{
	static u8 u8State	= 0;

	//_03电机
	switch (SysTask.Moto_State[GLASS_01])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_01] == 0)
			{
				SysTask.Moto_RunTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_CHANGE_DIR;
				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_RUN;
			}

			break;

		case MOTO_STATE_OPEN_LOCK:
			Moto_1_Start_CCW(MOTO_PUTY_99); //全速回去 开锁
			break;

		case MOTO_STATE_CHANGE_DIR:
			if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD)
			{
				Moto_1_Start_CCW(MOTO_PUTY);
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_NOR;
			}
			else if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_REV)
			{
				Moto_1_Start_CW(MOTO_PUTY);
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_NOR;
			}
			else if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD_REV)
			{ //正反转
				u8State 			= 0;
				Moto_1_Start_CCW(MOTO_PUTY);
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_CHA;
			}

			SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];
			break;

		case MOTO_STATE_RUN_NOR: //普通运行状态
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01])
			{
				Moto_1_Stop();						//先暂停电机转动
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
				SysTask.Moto_StateTime[GLASS_01] = 200;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_01] == 0)
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //检测到零点	
						{
							SysTask.Moto_WaitTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][2];
							SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_WAIT;
							Moto_1_Stop();			//先暂停电机转动
						}
					}

					break;

				case MOTO_SUB_STATE_WAIT:
					if (SysTask.Moto_WaitTime[GLASS_01] == 0)
					{
						SysTask.Moto_RunTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_RUN;

						if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD)
						{
							Moto_1_Start_CCW(MOTO_PUTY);
							SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_NOR;
						}
						else if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_REV)
						{
							Moto_1_Start_CW(MOTO_PUTY);
							SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_NOR;
						}
					}

					break;

				default:
					break;
			}

			break;

		case MOTO_STATE_RUN_CHA: //正反转运行状态
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01])
			{
				Moto_1_Stop();						//先暂停电机转动
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_01] == 0) //TPD时间到
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //检测到零点	
						{
							SysTask.Moto_WaitTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][2];
							SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_WAIT;
							Moto_1_Stop();			//先暂停电机转动
						}
					}
					else if (SysTask.Moto_StateTime[GLASS_01] == 0)
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //检测到零点	切换正反转
						{

							SysTask.Moto_StateTime[GLASS_01] = SysTask.nTick + MOTO_TRUN_OVER + 500; //检测到0点，一段时间后再次检测，避开

							SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DELAY;
							Moto_1_Stop();			//先暂停电机转动
							SysTask.Moto3_SubStateTime = MOTO_TRUN_OVER;
						}

					}

					break;

				case MOTO_SUB_STATE_DELAY:
					if (SysTask.Moto3_SubStateTime != 0) //延时一段时间再翻转
						break;

					if (u8State == 0)
					{
						u8State 			= 1;
						Moto_1_Start_CW(MOTO_PUTY);
					}
					else 
					{

						u8State 			= 0;
						Moto_1_Start_CCW(MOTO_PUTY);
					}

					SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_RUN;
					break;

				case MOTO_SUB_STATE_WAIT:
					if (SysTask.Moto_WaitTime[GLASS_01] == 0)
					{
						u8State 			= 0;
						SysTask.Moto_RunTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_RUN;
						Moto_1_Start_CCW(MOTO_PUTY);
					}

					break;

				default:
					break;
			}

			break;

		case MOTO_STATE_STOP:
			if (SysTask.Moto_WaitTime[GLASS_01] == 0)
			{
				Moto_1_Stop();						//先暂停电机转动

				SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE;
			}

			break;

		case MOTO_STATE_WAIT:
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
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_02_Task(void)
{
	static u8 u8State	= 0;

	//_03电机
	switch (SysTask.Moto_State[GLASS_02])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_02] == 0)
			{
				SysTask.Moto_RunTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][1];
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_CHANGE_DIR;
				SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_RUN;
			}

			break;

		case MOTO_STATE_OPEN_LOCK:
			Moto_2_Start_CCW(MOTO_PUTY_99); //全速回去 开锁
			break;

		case MOTO_STATE_CHANGE_DIR:
			if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_FWD)
			{
				Moto_2_Start_CCW(MOTO_PUTY_B);
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_NOR;
			}
			else if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_REV)
			{
				Moto_2_Start_CW(MOTO_PUTY_B);
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_NOR;
			}
			else if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_FWD_REV)
			{ //正反转
				u8State 			= 0;
				Moto_2_Start_CCW(MOTO_PUTY_B);
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_CHA;
			}

			SysTask.Moto_ModeSave[GLASS_02] = SysTask.Moto_Mode[GLASS_02];
			break;

		case MOTO_STATE_RUN_NOR: //普通运行状态
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02])
			{
				Moto_2_Stop();						//先暂停电机转动
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //先延时一段时间再翻转
				SysTask.Moto_StateTime[GLASS_02] = 200;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_02])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_02] == 0)
					{

						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //检测到零点	
						{
							SysTask.Moto_WaitTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][2];
							SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_WAIT;
							Moto_2_Stop();			//先暂停电机转动
						}
					}

					break;

				case MOTO_SUB_STATE_WAIT:
					if (SysTask.Moto_WaitTime[GLASS_02] == 0)
					{
						SysTask.Moto_RunTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][1];
						SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_RUN;

						if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_FWD)
						{
							Moto_2_Start_CCW(MOTO_PUTY_B);
							SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_NOR;
						}
						else if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_REV)
						{
							Moto_2_Start_CW(MOTO_PUTY_B);
							SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_NOR;
						}
					}

					break;

				default:
					break;
			}

			break;

		case MOTO_STATE_RUN_CHA: //正反转运行状态
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02])
			{
				Moto_2_Stop();						//先暂停电机转动
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //先延时一段时间再翻转
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_02])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_02] == 0)
					{

						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //检测到零点	
						{
							SysTask.Moto_WaitTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][2];
							SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_WAIT;
							Moto_2_Stop();			//先暂停电机转动
						}
					}
					else if (SysTask.Moto_StateTime[GLASS_02] == 0)
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //检测到零点	
						{
							SysTask.Moto_StateTime[GLASS_02] = SysTask.nTick + MOTO_TRUN_OVER + 500; //检测到0点，一段时间后再次检测，避开

							SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_DELAY;
							Moto_2_Stop();			//先暂停电机转动
							SysTask.Moto3_SubStateTime = MOTO_TRUN_OVER;
						}

					}

					break;

				case MOTO_SUB_STATE_DELAY:
					if (SysTask.Moto3_SubStateTime != 0) //延时一段时间再翻转
						break;

					if (u8State == 0)
					{
						u8State 			= 1;
						Moto_2_Start_CW(MOTO_PUTY_B);
					}
					else 
					{

						u8State 			= 0;
						Moto_2_Start_CCW(MOTO_PUTY_B);
					}

					SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_RUN;
					break;

				case MOTO_SUB_STATE_WAIT:
					if (SysTask.Moto_WaitTime[GLASS_02] == 0)
					{
						u8State 			= 0;
						SysTask.Moto_RunTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][1];
						SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_RUN;
						Moto_2_Start_CCW(MOTO_PUTY_B);
					}

					break;

				default:
					break;
			}

			break;

		case MOTO_STATE_STOP:
			if (SysTask.Moto_WaitTime[GLASS_02] == 0)
			{
				Moto_2_Stop();						//先暂停电机转动

				SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE;
			}

			break;

		case MOTO_STATE_WAIT:
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
* 返回: 无
* 说明: 
*******************************************************************************/
void Lock_01_Task(void)
{
	static u8 u8ClockCnt = 0;
	static u8 u8ClockPer = 0;

	switch (SysTask.Lock_State[GLASS_01]) //_03锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.LockMode[GLASS_01] != SysTask.LockModeSave[GLASS_01])
			{

				if (SysTask.LockMode[GLASS_01] == LOCK_ON)
				{
				    SysTask.Moto_State[GLASS_01] = MOTO_STATE_OPEN_LOCK; //  电机开锁状态
					if (SysTask.Moto_WaitTime[GLASS_01] != 0) //如果电机在停止状态
					{
						SysTask.Moto_WaitTime[GLASS_01] = 0;
					}

					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //检测到零点 开锁
					{
			            Moto_1_Start_CW(MOTO_PUTY);   //往回走一点到零点
						SysTask.Lock_StateTime[GLASS_01] = MOTO_TIME_OUT; //
						SysTask.Moto_WaitTime[GLASS_01] = MOTO_DEBOUNCE;
						SysTask.Lock_State[GLASS_01] = LOCK_STATE_DEBOUNSE;
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_STOP; //停止转动
						SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
						SysTask.Lock_OffTime[GLASS_01] = LOCK_OFFTIME;
					}
				}
				else 
				{
					SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
					GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN);
					u8ClockCnt			= 0;
					u8ClockPer			= 0;
					SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_L_TIME;
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_SENT;
				}
			}

			break;

		case LOCK_STATE_DEBOUNSE:
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
			{
				if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //检测到零点 开锁
				{
					GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;

					if (SysTask.SendState == SEND_IDLE)
						SysTask.SendState = SEND_TABLET_ZERO;

					g_FingerAck 		= ACK_SUCCESS;
					g_AckType			= ZERO_ACK_TYPE;
					g_GlassSendAddr 	= GLASS_01;
				}
			}
			else 
			{

#if DEBUG
				u3_printf(" 超时强制退出。。。。。\n");
#endif

				SysTask.LockModeSave[GLASS_01] = LOCK_OFF;
				SysTask.LockMode[GLASS_01] = LOCK_OFF;
				SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;
			}

			break;

		case LOCK_STATE_SENT: //发送3个脉冲给下面单片机，执行关锁
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			if (u8ClockCnt < 4)
			{
				u8ClockCnt++;

				if (u8ClockPer == 0)
				{
					u8ClockPer			= 1;
					GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
					SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_H_TIME;
				}
				else 
				{
					u8ClockPer			= 0;
					GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN);
					SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_L_TIME;
				}
			}
			else 
			{
				SysTask.Lock_State[GLASS_01] = LOCK_STATE_OFF;
				GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
				SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_OFF_TIME; //停止电平

			}

			break;

		case LOCK_STATE_OFF: //
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN);
			SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_L_TIME;
			SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;

			//	点击了转动，这里让他转动起来
			SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //重新开始转动
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
	static u8 u8ClockCnt = 0;
	static u8 u8ClockPer = 0;

	switch (SysTask.Lock_State[GLASS_02]) //_03锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.LockMode[GLASS_02] != SysTask.LockModeSave[GLASS_02])
			{

				if (SysTask.LockMode[GLASS_02] == LOCK_ON)
				{
				    SysTask.Moto_State[GLASS_02] = MOTO_STATE_OPEN_LOCK; //  电机开锁状态
					if (SysTask.Moto_WaitTime[GLASS_02] != 0) //如果电机在停止状态
					{
						SysTask.Moto_WaitTime[GLASS_02] = 0;
					}

					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //检测到零点 开锁
					{
			            Moto_2_Start_CW(MOTO_PUTY);   //往回走一点到零点
						SysTask.Lock_StateTime[GLASS_02] = MOTO_TIME_OUT; //
						SysTask.Moto_WaitTime[GLASS_02] = MOTO_DEBOUNCE;
						SysTask.Lock_State[GLASS_02] = LOCK_STATE_DEBOUNSE;
						SysTask.Moto_State[GLASS_02] = MOTO_STATE_STOP; //停止转动
						SysTask.LockModeSave[GLASS_02] = SysTask.LockMode[GLASS_02];
						SysTask.Lock_OffTime[GLASS_02] = LOCK_OFFTIME;
					}
				}
				else 
				{
					SysTask.LockModeSave[GLASS_02] = SysTask.LockMode[GLASS_02];
					GPIO_ResetBits(LOCK_GPIO, LOCK_02_PIN);
					u8ClockCnt			= 0;
					u8ClockPer			= 0;
					SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_L_TIME;
					SysTask.Lock_State[GLASS_02] = LOCK_STATE_SENT;
				}
			}

			break;

		case LOCK_STATE_DEBOUNSE:
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
			{
				if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //检测到零点 开锁
				{
					GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
					SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;

					if (SysTask.SendState == SEND_IDLE)
						SysTask.SendState = SEND_TABLET_ZERO;

					g_FingerAck 		= ACK_SUCCESS;
					g_AckType			= ZERO_ACK_TYPE;
					g_GlassSendAddr 	= GLASS_02;
				}
			}
			else 
			{

#if DEBUG
				u3_printf(" 超时强制退出。。。。。\n");
#endif

				SysTask.LockModeSave[GLASS_02] = LOCK_OFF;
				SysTask.LockMode[GLASS_02] = LOCK_OFF;
				SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;
			}

			break;

		case LOCK_STATE_SENT: //发送3个脉冲给下面单片机，执行开锁
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;

			if (u8ClockCnt < 4)
			{
				u8ClockCnt++;

				if (u8ClockPer == 0)
				{
					u8ClockPer			= 1;
					GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
					SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_H_TIME;
				}
				else 
				{
					u8ClockPer			= 0;
					GPIO_ResetBits(LOCK_GPIO, LOCK_02_PIN);
					SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_L_TIME;
				}
			}
			else 
			{
				SysTask.Lock_State[GLASS_02] = LOCK_STATE_OFF;
				GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
				SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_OFF_TIME; //停止电平

			}

			break;

		case LOCK_STATE_OFF: //
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;

			GPIO_ResetBits(LOCK_GPIO, LOCK_02_PIN);
			SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_L_TIME;
			SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;

			//	点击了转动，这里让他转动起来
			SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //重新开始转动
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
void RangeEventTask(void)
{
	static u8 u8RangeCnt = 0;
	u16 u16RangeVal 	= 0;

	switch (SysTask.Range_State) //_
	{
		case RANGE_STATE_INIT:
			if (SysTask.Range_StateTime)
				break;

			if (u8RangeCnt == 0)
			{
				RangeSensor_Event(0XA4, &u16RangeVal); //共三个传感器，地址分别为 0xa4 0xa6 0xa8
			}
			else if (u8RangeCnt == 1)
			{
				RangeSensor_Event(0XA6, &u16RangeVal);
			}
			else 
			{
				RangeSensor_Event(0XA8, &u16RangeVal);
			}

			if ((u16RangeVal <= RANGE_DISTANCE) && (u16RangeVal >= 200))  //第一次进入要200，实测为100时一个晚上大概有180次误触发
			{
#if DEBUG
				u3_printf(" 有人进入 传感器：%d --> %d mm\n", u8RangeCnt, u16RangeVal);
#endif
				SysTask.Range_StateTime = 100;		//20ms	
				SysTask.Range_State = RANGE_STATE_DEBOUNCE;
				SysTask.Range_WaitTime = RANGE_LEAFT_TIME;
                break;
			}
			else 
			{
				if (u8RangeCnt == 2)
				{ //300 ms 一次采集
					SysTask.Range_StateTime = RANGE_DETECT_TIME;
					u8RangeCnt			= 0;
					break;
				}
				else 
				{
					SysTask.Range_StateTime = 30;	//20ms	
				}
			}

			u8RangeCnt++;
			break;
            
        case RANGE_STATE_DEBOUNCE: //再次检测 之前感应到人的那个传感器，如果还有 说明 已经有人进入
            if (SysTask.Range_StateTime)
				break;

			if (u8RangeCnt == 0)
			{
				RangeSensor_Event(0XA4, &u16RangeVal); //共三个传感器，地址分别为 0xa4 0xa6 0xa8
			}
			else if (u8RangeCnt == 1)
			{
				RangeSensor_Event(0XA6, &u16RangeVal);
			}
			else 
			{
				RangeSensor_Event(0XA8, &u16RangeVal);
			}

			if ((u16RangeVal <= RANGE_DISTANCE) && (u16RangeVal >= 100))
			{
#if DEBUG
				u3_printf(" 真的有人进入 传感器：%d --> %d mm\n", u8RangeCnt, u16RangeVal);
#endif
                SysTask.LED_Mode = LED_MODE_ON;
				SysTask.Range_StateTime = 50;		//20ms	
				SysTask.Range_State = RANGE_STATE_WAIT_LEAFT;
				SysTask.Range_WaitTime = RANGE_LEAFT_TIME;
                u8RangeCnt = 0;
                break;
			}
			else 
			{
                u8RangeCnt = 0;
				SysTask.Range_StateTime = 100;		//20ms	
				SysTask.Range_State = RANGE_STATE_INIT;
			}
            break;

		case RANGE_STATE_WAIT_LEAFT:
			if (SysTask.Range_StateTime)
				break;

			if (u8RangeCnt == 0)
			{
				RangeSensor_Event(0XA4, &u16RangeVal); //共三个传感器，地址分别为 0xa4 0xa6 0xa8
			}
			else if (u8RangeCnt == 1)
			{
				RangeSensor_Event(0XA6, &u16RangeVal);
			}
			else 
			{
				RangeSensor_Event(0XA8, &u16RangeVal);
			}

			if ((u16RangeVal <= RANGE_DISTANCE) && (u16RangeVal >= 100))
			{
#if DEBUG
				u3_printf(" 传感器：%d --> %d mm\n", u8RangeCnt, u16RangeVal);
#endif

				u8RangeCnt			= 0;
				SysTask.Range_StateTime = 100;		//20ms	
				SysTask.Range_WaitTime = RANGE_LEAFT_TIME;
			}
			else 
			{
				if (SysTask.Range_WaitTime == 0)
				{ //人走

#if DEBUG
					u3_printf("人离开。。");
#endif
                    
                    SysTask.LED_Mode = LED_MODE_OFF;

					SysTask.Range_State = RANGE_STATE_INIT;
					u8RangeCnt			= 0;
					break;
				}

				if (u8RangeCnt == 2)
				{ //300 ms 一次采集
					SysTask.Range_StateTime = RANGE_DETECT_TIME;
					u8RangeCnt			= 0;
					break;
				}
				else 
				{
					u8RangeCnt++;
					SysTask.Range_StateTime = 20;	//20ms	
				}
			}

			break;

		default:
			SysTask.Range_State = RANGE_STATE_INIT;
			break;
	}
}

/* calculate the Period and Freq array value, fill the Period value into the Period register during the timer 
	interrupt.
*calculate the acceleration procedure , a totally 1000 elements array.
* parameter fre[]: point to the array that keeps the freq value.
* period[]: point to the array that keeps the timer period value.
* len: the procedure of acceleration length.it is best thing to set the float number, some compile software maybe 
	transfer error if set it as a int
* fre_max: maximum speed, frequency vale.
* fre_min: start minimum speed, frequency vale. mind : 10000000/65535 = 152, so fre_min can't less than 152.
* flexible: flexible value. adjust the S curves, 数值越大曲线起始越慢
*/
static void Sys_CalculateSModelLine(float arr[],  float len, float arr_max, float arr_min, float flexible)
{
	int 			i	= 0;
	float			deno;
	float			melo;
	float			delt = arr_max - arr_min;
        //原始S型加速，改成指数类型
        //	for (; i < len; i++)
        //	{
        //		melo				= flexible * (i - len / 2) / (len / 2);
        //		deno				= 1.0f / (1 + expf(-melo)); //expf is a library function of exponential(e) 
        //		fre[i]				= delt * deno + fre_min;
        //		period[i]			= (unsigned short) ((float) TIMER_FRE / fre[i]); // TIMER_FRE is the timer driver frequency
        //	}
        
	for (; i < len / 2; i++)
	{
		melo				= flexible * (i - len / 2) / (len / 2);
		deno				= 1.0f / (1 + expf(-melo)); //expf is a library function of exponential(e) 
		arr[i]				= delt * deno + arr_min;
	}

	return;
}
    


#define   LIGHT_ADD_SPEED   5
#define   LIGHT_DEC_SPEED   2
#define   LIGHT_DEC_SPEED_20   6

/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void LedTask(void)
{
    static vu16 u16Brighness = 0;
	switch (SysTask.Led_State) //_
	{
		case LED_STATE_INIT:
			if (SysTask.LED_Mode != SysTask.LED_ModeSave)
			{
				if (SysTask.LED_Mode == LED_MODE_ON)
				{
					SysTask.Led_State	= LED_STATE_ON;
                    u16Brighness = 0;
				}
				else 
				{
                    u16Brighness = LED_BRIGHNESS - 1;
					SysTask.Led_State	= LED_STATE_OFF;
				}
				SysTask.LED_ModeSave = SysTask.LED_Mode;
			}

			break;
            
            case LED_STATE_ON:
                if(SysTask.LED_StateTime)
                    break;
                
                SysTask.LED_StateTime = LIGHT_ADD_SPEED;  //led 亮度更新速度 

                if(u16Brighness < LED_BRIGHNESS - 1){
                    u16Brighness++;
//                    Led_1_Brighness(u16Brighness);
//                    Led_2_Brighness(u16Brighness);
                    
                    Led_1_Brighness(g_LedArr[u16Brighness]);
                    Led_2_Brighness(g_LedArr[u16Brighness]);
                }else{
                    SysTask.Led_State = LED_STATE_INIT;
                }
			break;
            
            case LED_STATE_OFF:
                if(SysTask.LED_StateTime)
                    break;
                
                SysTask.LED_StateTime = LIGHT_DEC_SPEED;  //led 亮度更新速度 

                if(u16Brighness > 0){
                    u16Brighness--;
                    if(u16Brighness < 30)
                        SysTask.LED_StateTime = LIGHT_DEC_SPEED_20;  //led 亮度更新速度 
//                    Led_1_Brighness(u16Brighness);
//                    Led_2_Brighness(u16Brighness);
                    Led_1_Brighness(g_LedArr[u16Brighness]);
                    Led_2_Brighness(g_LedArr[u16Brighness]);
                }else{
                    Led_1_Off();
                    Led_2_Off();
                    SysTask.Led_State = LED_STATE_INIT;
                }
                
			break;

		default:
			SysTask.Led_State = LED_STATE_INIT;
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
void MainTask(void)
{
	SysSaveDataTask();
	LedTask();
	SendTabletTask();								//发送给平板任务
	TabletToHostTask(); 							//接收平板命令任务处理
	Moto_01_Task(); 								//只有正反转的，转半圈
	Moto_02_Task(); 								//中间的 升降， 下降时有个零点检测 ，上升使用时间做判断

	Lock_01_Task();
	Lock_02_Task();
	RangeEventTask();
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
	g_RangeEvent.bInit	= TRUE;
	g_RangeEvent.bMutex = FALSE;
	g_RangeEvent.MachineCtrl.pData = (void *)
	malloc(sizeof(RangeData_t));
	g_RangeEvent.MachineCtrl.CallBack = RangeSensor_Event;
	g_RangeEvent.MachineCtrl.u16InterValTime = 5;	//5ms 进入一次

	g_RangeEvent.MachineRun.bEnable = TRUE;
	g_RangeEvent.MachineRun.u32StartTime = 0;
	g_RangeEvent.MachineRun.u32LastTime = 0;
	g_RangeEvent.MachineRun.u32RunCnt = 0;
	g_RangeEvent.MachineRun.bFinish = FALSE;
	g_RangeEvent.MachineRun.u16RetVal = 0;
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

	Moto_1_Stop();
	Moto_2_Stop();
    
    Sys_CalculateSModelLine(g_LedArr,LED_BRIGHNESS * 2, LED_BRIGHNESS * 2, 0, 5); //最后一个参数
    g_LedArr[LED_BRIGHNESS -1] = LED_BRIGHNESS;
    

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

		//		  memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
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
		SysTask.LockMode[i] = LOCK_OFF; 			//锁状态不在此更新，因为开锁需要密码之后
	}

	SysTask.u8Usart1RevTime = 0;
	SysTask.Range_State = RANGE_STATE_INIT;
	SysTask.Range_StateTime = 0;
	SysTask.Range_WaitTime = 0;
	SysTask.TouchState	= TOUCH_MATCH_INIT;
	SysTask.TouchSub	= TOUCH_SUB_INIT;
	SysTask.bEnFingerTouch = FALSE;
	SysTask.nWaitTime	= 0;
	SysTask.nTick		= 0;
	SysTask.u16SaveTick = 0;
	SysTask.u16PWMVal	= 0;
	SysTask.bEnSaveData = FALSE;
	SysTask.Moto3_SubStateTime = 0;


	SysTask.SendState	= SEND_IDLE;
	SysTask.SendSubState = SEND_SUB_INIT;
	SysTask.bTabReady	= FALSE;

	SysTask.LED_Mode	= LED_MODE_OFF;
	SysTask.LED_ModeSave = LED_MODE_OFF;			// 
	SysTask.Led_State	= LED_STATE_INIT;

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.LockModeSave[i] = LOCK_OFF;
		SysTask.LockMode[i] = LOCK_OFF;
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
		SysTask.Lock_StateTime[i] = 0;				//		  
	}

	//	  SysTask.u16BootTime = 3000;	//3秒等待从机启动完全
}


