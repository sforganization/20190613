
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
#include "ws2812b.h"




#define SLAVE_WAIT_TIME 		3000		//3s 从机应答时间

#define LOCK_SET_L_TIME 		50			//低电平时间 25 ~ 60ms
#define LOCK_SET_H_TIME 		50			//高电平时间
#define LOCK_SET_OFF_TIME		1200		//给单片机反应时间，电机回正之后再转

#define MOTO_UP_TIME			800			//电机上升时间
#define MOTO_DOWN_TIME			1500		//电机下降时间

#define POWER_ON_DELAY			250 		//上电一段时间再去使用 指纹模块，实际1020A 需要200ms					 1020C 需要100ms ,注意后面的操作还要加上多次操作，防止芯片差异造成的影响

#define LOCK_OPEN_TIME			500			//开锁给低电平时间

#define STREAM_TIME 			150			//流水时间
#define BLINK_TIME				240			//闪烁时间
#define FAILED_TIME 			160			//闪烁时间

#define BLINK_CNT_1 			2			//闪烁次数
#define BLINK_CNT_5 			10			//闪烁次数 00 /2 =5次
#define BLINK_CNT_6 			12			//闪烁次数 00 /2 =5次
#define BLINK_CNT_8 			16			//闪烁次数 00 /2 =5次
#define BLINK_CNT_10			20			//闪烁次数


#define KEY_PRESS_TIME			6000		//按键按下时间	3S长按是清除所有用户
#define KEY_DEBOUCH_TIME		40			//按键消抖时间40ms
#define KEY_SHORT_TIME			3000		//短按时间


#define ROOT_ID_NUM_1			1			//1020A 因为要抬手录入两次指纹，所以相当于一个指纹占用两个ID
#define ROOT_ID_NUM_2			2


#define RECV_PAKAGE_CHECK		10	 // 	10个数据检验

static bool 	g_bSensorState = TRUE;

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


char			priBuf[120] =
{
	0
};


PollEvent_t 	g_FingerEvent;

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


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void FingerTouchTask(void)
{
	int retVal			= 0;
	static u8 u8MatchCnt = 0;						//匹配失败次数，默认匹配MATCH_FINGER_CNT次 
	static u8 u8SetCnt	= 0;						//设置失败次数
	int userID			= 0XFFFF;
	FingerData_t * pfingerData;

	switch (SysTask.TouchState)
	{
		case TOUCH_MATCH_INIT:
			if (SysTask.u32TouchWaitTime != 0)
				break;

			if (PS_Sta) //有指纹按下
			{
#if DEBUG
				u3_printf("TOUCH_MATCH_INIT 有按下\n");
#endif

				if (SysTask.LockMode[GLASS_01] == LOCK_ON)
				{
#if DEBUG
					u3_printf("TOUCH_MATCH_INIT 执行关锁\n");
#endif

					SysTask.Stream_State = STREAM_STATE_INIT; //关锁
					SysTask.u32StreamTime = 0;

					SysTask.LockMode[GLASS_01] = LOCK_OFF; //已经开锁，在这可以把锁关了 
					SysTask.u32TouchWaitTime = SysTask.nTick + 3000; //延时一定时间 成功之后防止马上关锁
					break;
				}

				SysTask.u32TouchWaitTime = SysTask.nTick + POWER_ON_DELAY; //一段时间再检测， 让指纹模块MCU上电完全 ,实测1020A需要200ms	1020c 100ms
				FINGER_POWER_ON();					//开启指纹模块电源
				SysTask.TouchState	= TOUCH_MATCH_AGAIN;
				u8MatchCnt			= 0;

			}

			break;

		case TOUCH_MATCH_AGAIN:
			if (SysTask.u32TouchWaitTime == 0) //有指纹按下
			{
#if 0
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
#endif

				//u8Ensure			= Finger_Search(&userID); //1：N搜索指纹库
				if (g_FingerEvent.bMutex == FALSE)
				{
					g_FingerEvent.bMutex = TRUE;	//上锁，只操作一次
					g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT; //1S 超时  ,5次就是6S
					g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
					g_FingerEvent.MachineRun.bFinish = FALSE;
					g_FingerEvent.MachineCtrl.CallBack(FINGER_SEARCHING, &g_FingerEvent);
				}
				else 
				{
					g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
					g_FingerEvent.MachineCtrl.CallBack(FINGER_SEARCHING, &g_FingerEvent);
				}

				if (g_FingerEvent.MachineRun.bFinish == TRUE)
				{
					if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
					{
#if DEBUG
						u3_printf("超时退出。。。。。\n");
#endif
					}

					pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
					userID				= pfingerData->u8FingerID;
					g_FingerEvent.bMutex = FALSE;
				}
				else 
				{
					break;
				}


#if 0
				GPIO_ResetBits(GPIOC, GPIO_Pin_13);
#endif

				if (g_FingerEvent.MachineRun.u16RetVal == TRUE) //匹配成功
				{
					pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
					userID				= pfingerData->u8FingerID;

#if DEBUG
					u3_printf("TOUCH_MATCH_AGAIN 匹配成功，lock开锁 ID %d\n", userID);
#endif

					SysTask.Stream_State = STREAM_STATE_OPEN_LOCK; //开锁，全亮

					//SysTask.u16FingerfCnt = seach.pageID;
					SysTask.u32TouchWaitTime = SysTask.nTick + 3000; //延时一定时间 成功之后防止马上关锁
					SysTask.TouchSub	= TOUCH_SUB_INIT;
					SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
					SysTask.LockMode[GLASS_01] = LOCK_ON; //匹配成功，可以开锁

					FINGER_POWER_OFF(); 			//关闭指纹模块电源
				}
				else 
				{
#if DEBUG
					u3_printf("TOUCH_MATCH_AGAIN 匹配失败\n");
#endif

					SysTask.u32TouchWaitTime = SysTask.nTick + 100; //延时一定时间 如果手臭识别不了，再想来一个人检测，

					//匹配失败
					if (u8MatchCnt++ >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
					{
						SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
						SysTask.u8StreamShoWCnt = BLINK_CNT_8;
						FINGER_POWER_OFF(); 		//关闭指纹模块电源
						SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
						SysTask.u32TouchWaitTime = SysTask.nTick + 500; //延时一定时间 如果手臭识别不了，再想来一个人检测，
					}
				}
			}

			break;

		case TOUCH_ADD_USER:
			{
				switch (SysTask.TouchSub)
				{
					case TOUCH_SUB_INIT: // *
						if (SysTask.u16FingerfCnt > FINGER_ID_MAX - 2) //因为一个指纹要录入两次（有抬手再按下去的动作） ，所以要减去2
						{
#if DEBUG
							u3_printf("TOUCH_ADD_USER ");
							snprintf(priBuf, 50, "SysTask.u16FingerfCnt : %d\n", SysTask.u16FingerfCnt);
							u3_printf(priBuf);
							u3_printf("\n");
							u3_printf("用户已满\n");
#endif

							SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
							SysTask.u8StreamShoWCnt = BLINK_CNT_8;

							SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
							SysTask.u32TouchWaitTime = SysTask.nTick + POWER_ON_DELAY; //延时一定时间
							return;
						}
						else if ((PS_Sta) && (SysTask.u32TouchWaitTime == 0))
						{
#if DEBUG
							u3_printf("TOUCH_ADD_USER ");
							snprintf(priBuf, 50, "SysTask.u16FingerfCnt : %d\n", SysTask.u16FingerfCnt);
							u3_printf(priBuf);
							u3_printf("开启指纹模块电源-->添加\n");
#endif

							FINGER_POWER_ON();		//开启指纹模块电源
							SysTask.u32TouchWaitTime = POWER_ON_DELAY; //延时一定时间再去设置重复模式
							u8MatchCnt			= 0;
							SysTask.u16FingerfCnt++;
							SysTask.TouchSub	= TOUCH_SUB_SETMODE_ONE;
							u8SetCnt			= 0;
							SysTask.Stream_State = STREAM_STATE_BLINK_TWO; //
						}

						break;

					case TOUCH_SUB_SETMODE_ONE: // *
						if (SysTask.u32TouchWaitTime == 0)
						{
							if (g_FingerEvent.bMutex == FALSE)
							{
								g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
								g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT; //1S 超时  ,5次就是6S
								g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineRun.bFinish = FALSE;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_MODE_SET, &g_FingerEvent);
							}
							else 
							{
								g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_MODE_SET, &g_FingerEvent);
							}

							if (g_FingerEvent.MachineRun.bFinish == TRUE)
							{
								if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
								{
#if DEBUG
									u3_printf("超时退出。。。。。\n");
#endif
								}

								g_FingerEvent.bMutex = FALSE;
							}
							else 
							{
								break;				//等待下层上传的结果状态
							}

							u8SetCnt++;

							//if (!Finger_ModeSet())
							if (g_FingerEvent.MachineRun.u16RetVal == TRUE)
							{
#if DEBUG
								u3_printf("设置重复模式成功\n");
								snprintf(priBuf, 10, "CNT : %d\n", u8SetCnt);
								u3_printf(priBuf);
								u3_printf("\n");
#endif

#if DEBUG
								u3_printf("第一，采集指纹\n");
#endif

								SysTask.TouchSub	= TOUCH_SUB_ENTER;
							}
							else 
							{
#if DEBUG
								u3_printf("设置重复模式失败\n");
#endif

								if (u8SetCnt <= 5)
									SysTask.u32TouchWaitTime = 100;
								else 
									SysTask.TouchSub = TOUCH_SUB_ENTER;
							}
						}

						break;

					case TOUCH_SUB_ENTER: //
						if ((SysTask.u32TouchWaitTime == 0)) //时间到去采集
						{
							if (g_FingerEvent.bMutex == FALSE)
							{
								SysTask.Stream_State = STREAM_STATE_BLINK_TWO; //
								g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
								g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT * 5; //1S 超时  ,5次就是6S
								g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineRun.bFinish = FALSE;
								pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
								pfingerData->u8FingerID = SysTask.u16FingerfCnt;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_ENROLL, &g_FingerEvent);
							}
							else 
							{
								pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
								pfingerData->u8FingerID = SysTask.u16FingerfCnt;
								g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_ENROLL, &g_FingerEvent);
							}

							if (g_FingerEvent.MachineRun.bFinish == TRUE)
							{
								if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
								{
#if DEBUG
									u3_printf("超时退出。。。。。\n");
#endif
								}

								pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
								userID				= pfingerData->u8FingerID;
								g_FingerEvent.bMutex = FALSE;
							}
							else 
							{
								break;				//等待下层上传的结果状态
							}

							//u8Ensure			= Finger_Enroll(SysTask.u16FingerfCnt);
							if (g_FingerEvent.MachineRun.u16RetVal == TRUE)
							{
#if DEBUG
								u3_printf("第一，采集指纹成功\n");
#endif

								SysTask.Stream_State = STREAM_STATE_BLINK_THREE; //

								u8MatchCnt			= 0;
								SysTask.u32TouchWaitTime = 300; //延时一定时间再去采集
								SysTask.TouchSub	= TOUCH_SUB_AGAIN_PON; //跳到第二步		
								FINGER_POWER_OFF(); //关闭指纹模块电源,等待录入第二次指纹		
							}
							else 
							{

								if (u8MatchCnt++ >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
								{
#if DEBUG
									u3_printf("第一，采集指纹失败，超过最大次数，退出添加\n");
#endif

									SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
									SysTask.u8StreamShoWCnt = BLINK_CNT_8;

									SysTask.u16FingerfCnt -= 1;
									FINGER_POWER_OFF(); //关闭指纹模块电源

									SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配		
									SysTask.TouchSub	= TOUCH_SUB_INIT;
								}
								else 
								{
#if DEBUG
									u3_printf("第一，采集指纹失败 ");
									snprintf(priBuf, 10, "CNT : %d\n", u8MatchCnt);
									u3_printf(priBuf);
									u3_printf("\n");
#endif

									//匹配失败这里要进行一定的提示操作
									SysTask.u32TouchWaitTime = 100; //延时一定时间再次添加
								}
							}
						}

						break;

					case TOUCH_SUB_AGAIN_PON:
						if ((PS_Sta) && (SysTask.u32TouchWaitTime == 0))
						{
							FINGER_POWER_ON();		//开启指纹模块电源
							SysTask.u32TouchWaitTime = POWER_ON_DELAY; //延时一定时间再去采集
							u8MatchCnt			= 0;
							SysTask.u16FingerfCnt++;
							SysTask.TouchSub	= TOUCH_SUB_SETMODE_TWO;
							u8SetCnt			= 0;
						}

						break;

					case TOUCH_SUB_SETMODE_TWO: // *
						if (SysTask.u32TouchWaitTime == 0)
						{

							if (g_FingerEvent.bMutex == FALSE)
							{
								g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
								g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT; //1S 超时  ,5次就是6S
								g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineRun.bFinish = FALSE;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_MODE_SET, &g_FingerEvent);
							}
							else 
							{
								g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_MODE_SET, &g_FingerEvent);
							}

							if (g_FingerEvent.MachineRun.bFinish == TRUE)
							{
								if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
								{
#if DEBUG
									u3_printf("超时退出。。。。。\n");
#endif
								}

								g_FingerEvent.bMutex = FALSE;
							}
							else 
							{
								break;				//等待下层上传的结果状态
							}

							u8SetCnt++;

							if (!g_FingerEvent.MachineRun.u16RetVal)
							{
#if DEBUG
								u3_printf("2. 设置重复模式成功\n");
								snprintf(priBuf, 10, "CNT : %d\n", u8SetCnt);
								u3_printf(priBuf);
								u3_printf("\n");
#endif

								SysTask.TouchSub	= TOUCH_SUB_AGAIN;
							}
							else 
							{
								if (u8SetCnt <= 8)
									SysTask.u32TouchWaitTime = 50;
								else 
									SysTask.TouchSub = TOUCH_SUB_AGAIN;
							}
						}

						break;

					case TOUCH_SUB_AGAIN:
						if ((SysTask.u32TouchWaitTime == 0)) //
						{
							if (g_FingerEvent.bMutex == FALSE)
							{
								g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
								g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT * 5; //1S 超时  ,5次就是6S
								g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineRun.bFinish = FALSE;
								pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
								pfingerData->u8FingerID = SysTask.u16FingerfCnt;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_ENROLL, &g_FingerEvent);
							}
							else 
							{
								pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
								pfingerData->u8FingerID = SysTask.u16FingerfCnt;
								g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
								g_FingerEvent.MachineCtrl.CallBack(FINGER_ENROLL, &g_FingerEvent);
							}

							if (g_FingerEvent.MachineRun.bFinish == TRUE)
							{
								if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
								{
#if DEBUG
									u3_printf("超时退出。。。。。\n");
#endif
								}

								pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
								userID				= pfingerData->u8FingerID;
								g_FingerEvent.bMutex = FALSE;
							}
							else 
							{
								break;				//等待下层上传的结果状态
							}

							//							u8Ensure			= Finger_Enroll(SysTask.u16FingerfCnt); //存入另外一个ID中，一个指纹一共存两个，，，注意，这里的指纹不一定是同一个
							if (g_FingerEvent.MachineRun.u16RetVal == TRUE)
							{
#if DEBUG
								u3_printf("第二，采集指纹成功\n");
#endif

								SysTask.Stream_State = STREAM_STATE_SUCCESS; //

								SysTask.u16SaveTick = 1; //1秒后保存
								SysTask.TouchSub	= TOUCH_SUB_WAIT;
								SysTask.u32TouchWaitTime = 300; //延时一定时间退出
							}
							else 
							{
								//采集指纹失败
								if (u8MatchCnt++ >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
								{
#if DEBUG
									u3_printf("采集指纹失败超过次数\n");
									u3_printf("删除第一次保存的，退出添加\n");
#endif

									SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
									SysTask.u8StreamShoWCnt = BLINK_CNT_8;

									Finger_Delete(SysTask.u16FingerfCnt - 1); //添加失败，删除第一个录入的
									SysTask.u16FingerfCnt -= 2;
									FINGER_POWER_OFF(); //关闭指纹模块电源

									SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配		
									SysTask.TouchSub	= TOUCH_SUB_INIT;
								}
								else 
								{
#if DEBUG
									u3_printf("采集指纹失败，重新采集\n");
#endif

									//匹配失败这里要进行一定的提示操作
									//SysTask.TouchSub	= TOUCH_SUB_ENTER;	 //重复模式这里不用再次搞
									SysTask.u32TouchWaitTime = 60; //延时一定时间再次添加
								}
							}
						}

						break;

					case TOUCH_SUB_WAIT: // *
						if ((SysTask.u32TouchWaitTime == 0))
						{
							FINGER_POWER_OFF(); 	//关闭指纹模块电源

							SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配		
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
						if (SysTask.u32TouchWaitTime == 0)
						{
							FINGER_POWER_ON();		//开启指纹模块电源
							u8MatchCnt			= 0;
							SysTask.TouchSub	= TOUCH_SUB_ENTER;
							SysTask.u32TouchWaitTime = POWER_ON_DELAY; //
						}

						break;

					case TOUCH_SUB_ENTER: //
						if (SysTask.u32TouchWaitTime)
						{
							break;
						}

						if (g_FingerEvent.bMutex == FALSE)
						{
							g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
							g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT * 5; //1S 超时  ,5次就是6S
							g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
							g_FingerEvent.MachineRun.bFinish = FALSE;
							pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
							pfingerData->u8FingerID = SysTask.u16FingerfCnt;
							g_FingerEvent.MachineCtrl.CallBack(FINGER_CLEAR, &g_FingerEvent);
						}
						else 
						{
							pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
							pfingerData->u8FingerID = SysTask.u16FingerfCnt;
							g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
							g_FingerEvent.MachineCtrl.CallBack(FINGER_CLEAR, &g_FingerEvent);
						}

						if (g_FingerEvent.MachineRun.bFinish == TRUE)
						{
							if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
							{
#if DEBUG
								u3_printf("超时退出。。。。。\n");
#endif
							}

							pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
							userID				= pfingerData->u8FingerID;
							g_FingerEvent.bMutex = FALSE;
						}
						else 
						{
							break;					//等待下层上传的结果状态
						}

						//u8Ensure = Finger_Clear(); //删除整个指纹库
						if (g_FingerEvent.MachineRun.u16RetVal == TRUE)
						{
#if DEBUG
							u3_printf("删除整个指纹库，成功\n");
#endif

							SysTask.Stream_State = STREAM_STATE_SUCCESS; //

							SysTask.u16FingerfCnt = 0;
							SysTask.u16SaveTick = 1; //1秒后保存
							SysTask.TouchSub	= TOUCH_SUB_WAIT;
							SysTask.u32TouchWaitTime = 100; //延时一定时间退出
						}
						else //删除失败 ,
						{
							u8MatchCnt++;
							SysTask.u32TouchWaitTime = 100; //延时一定时间再次删除

							if (u8MatchCnt >= 3) //失败退出
							{
#if DEBUG
								u3_printf("删除整个指纹库， 失败\n");
#endif

								SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
								SysTask.u8StreamShoWCnt = BLINK_CNT_8;

								SysTask.TouchSub	= TOUCH_SUB_WAIT;
							}
						}

						break;

					case TOUCH_SUB_WAIT: // *
						if ((SysTask.u32TouchWaitTime == 0))
						{
							FINGER_POWER_OFF(); 	//关闭指纹模块电源
							SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配		
							SysTask.TouchSub	= TOUCH_SUB_INIT;
						}

						break;

					default:
						break;
				}
			}
			break;

		case TOUCH_KEY_SHORT: //短按
			//短按，添加指纹
			//如果指纹模块里面还没有保存用户，则保存为ROOT用户
			//如果已经保存有指纹，则用ROOT用户验证成功之后，才可以添加用户（ROOT用户为第一个添加的用户）
			switch (SysTask.TouchSub)
			{
				case TOUCH_SUB_INIT: // 
					if (SysTask.u16FingerfCnt == 0) //当前还没有保存有用户。直接添加
					{
#if DEBUG
						u3_printf("TOUCH_KEY_SHORT 没有ROOT用户，直接添加为ROOT\n");
#endif
						SysTask.u32TouchWaitTime = 6000; //超时退出

						SysTask.TouchState	= TOUCH_ADD_USER;
					}
					else 
					{
#if DEBUG
						snprintf(priBuf, 50, "SysTask.u16FingerfCnt : %d\n", SysTask.u16FingerfCnt);
						u3_printf(priBuf);
						u3_printf("已经有ROOT，确认ROOT\n");
#endif

						SysTask.Stream_State = STREAM_STATE_BLINK_ONE; //

						SysTask.TouchSub	= TOUCH_SUB_CHECK_ROOT;
						SysTask.u32TouchWaitTime = 6000; //超时退出
						u8MatchCnt			= 0;
					}

					break;

				case TOUCH_SUB_CHECK_ROOT: // 确认ROOT用户 
					if (PS_Sta) //有指纹按下
					{
#if DEBUG
						u3_printf("TOUCH_SUB_AGAIN 有按下\n");
#endif

						SysTask.u32TouchWaitTime = SysTask.nTick + POWER_ON_DELAY; //一段时间再检测， 让指纹模块MCU上电完全 ,实测1020A需要200ms	1020c 100ms
						FINGER_POWER_ON();			//开启指纹模块电源
						SysTask.TouchSub	= TOUCH_SUB_AGAIN;
					}
					else if (SysTask.u32TouchWaitTime == 0)
					{
						//超时退出
#if DEBUG
						u3_printf("没有识别到ROOT 超时退出 \n");
#endif
						SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
						SysTask.TouchSub	= TOUCH_SUB_INIT;
						SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
						SysTask.u8StreamShoWCnt = BLINK_CNT_6;
					}

					break;

				case TOUCH_SUB_AGAIN:
					if (SysTask.u32TouchWaitTime == 0) //有指纹按下
					{
						if (g_FingerEvent.bMutex == FALSE)
						{
							g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
							g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT; //1S 超时  ,5次就是6S
							g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
							g_FingerEvent.MachineRun.bFinish = FALSE;
							g_FingerEvent.MachineCtrl.CallBack(FINGER_SEARCHING, &g_FingerEvent);
						}
						else 
						{
							g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
							g_FingerEvent.MachineCtrl.CallBack(FINGER_SEARCHING, &g_FingerEvent);
						}

						if (g_FingerEvent.MachineRun.bFinish == TRUE)
						{
							if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
							{
#if DEBUG
								u3_printf("超时退出。。。。。\n");
#endif
							}

							pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
							userID				= pfingerData->u8FingerID;
							g_FingerEvent.bMutex = FALSE;
						}
						else 
						{
							break;
						}

						//		u8Ensure			= Finger_Search(&userID); //1：N搜索指纹库
						if (g_FingerEvent.MachineRun.u16RetVal == TRUE) //匹配成功
						{

							pfingerData 		= ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
							userID				= pfingerData->u8FingerID;

							if ((userID != ROOT_ID_NUM_1) && (userID != ROOT_ID_NUM_2))
							{ //非ROOT用户

#if DEBUG
								u3_printf("TOUCH_MATCH_AGAIN 匹配失败	非ROOT用户");
								snprintf(priBuf, 50, "userID : %d\n", userID);
								u3_printf(priBuf);
								u3_printf("\n");
#endif

								SysTask.u32TouchWaitTime = SysTask.nTick + 100; //延时一定时间 再次试试

								//匹配失败
								if (u8MatchCnt++ >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
								{

#if DEBUG
									u3_printf("TOUCH_MATCH_AGAIN 匹配失败超次，退出 \n");
#endif

									SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
									SysTask.u8StreamShoWCnt = BLINK_CNT_8;

									FINGER_POWER_OFF(); //关闭指纹模块电源
									SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
									SysTask.u32TouchWaitTime = SysTask.nTick + 500; //延时一定时间 
								}
							}
							else 
							{
#if DEBUG
								u3_printf("确认为ROOT，添加指纹\n");
#endif

								FINGER_POWER_OFF(); //先关闭指纹模块电源，不然识别不到有 touch电平变化 
                                
                                SysTask.Stream_State = STREAM_STATE_BLINK_TWO; //灯闪烁提示

								SysTask.TouchState	= TOUCH_ADD_USER;
								SysTask.TouchSub	= TOUCH_SUB_INIT;
								SysTask.u32TouchWaitTime = 0;
							}
						}
						else 
						{
#if DEBUG
							u3_printf("TOUCH_MATCH_AGAIN 匹配失败\n");
#endif

							SysTask.u32TouchWaitTime = SysTask.nTick + 100; //延时一定时间 如果手臭识别不了，再想来一个人检测，

							//匹配失败
							if (u8MatchCnt++ >= MATCH_FINGER_CNT) // 50 * 3 大约200MS秒
							{
#if DEBUG
								u3_printf("TOUCH_MATCH_AGAIN 匹配失败超次，退出 \n");
#endif
                                
                                SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
                                SysTask.u8StreamShoWCnt = BLINK_CNT_8;

								FINGER_POWER_OFF(); //关闭指纹模块电源
								SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配
								SysTask.TouchSub	= TOUCH_SUB_INIT;
								SysTask.u32TouchWaitTime = SysTask.nTick + 500; //延时一定时间 如果手臭识别不了，再想来一个人检测，
							}
						}
					}

					break;

				default:
					break;
			}

			break;

		case TOUCH_CHECK: //读取ID看与EEPROM是否一致, 设置指纹的录入模式为可重复
			if (SysTask.u32TouchWaitTime)
				return;

			switch (SysTask.TouchSub)
			{
				case TOUCH_SUB_INIT: // *
					//					if (SysTask.u32TouchWaitTime == 0)
					//					{
					//						u8SetCnt++;
					//						if (!Finger_ModeSet())
					//						{
					//#if DEBUG
					//							u3_printf("TOUCH_CHECK 成功");
					//#endif
					//							SysTask.TouchSub	= TOUCH_SUB_ENTER;
					//						}
					//						else 
					//						{
					//							SysTask.u32TouchWaitTime = 50;
					//							SysTask.TouchSub	= TOUCH_SUB_WAIT; //失败重新设置进去
					//						}
					//					}
					//每一次添加前才用设置可重复添加模式
					SysTask.u32TouchWaitTime = 5;
					SysTask.TouchSub = TOUCH_SUB_ENTER;
					break;

				case TOUCH_SUB_ENTER: //
					if (SysTask.u32TouchWaitTime)
					{
						break;
					}

					if (g_FingerEvent.bMutex == FALSE)
					{
						g_FingerEvent.bMutex = TRUE; //上锁，只操作一次
						g_FingerEvent.MachineCtrl.u32Timeout = FINGER_REV_TIME_OUT; //1S 超时  ,5次就是6S
						g_FingerEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
						g_FingerEvent.MachineRun.bFinish = FALSE;
						g_FingerEvent.MachineCtrl.CallBack(FINGER_READ, &g_FingerEvent);
					}
					else 
					{
						g_FingerEvent.MachineRun.u32LastTime = SysTask.u32SysTimes;
						g_FingerEvent.MachineCtrl.CallBack(FINGER_READ, &g_FingerEvent);
					}

					if (g_FingerEvent.MachineRun.bFinish == TRUE)
					{
						if (g_FingerEvent.MachineRun.u16RetVal == EVENT_TIME_OUT) //超时退出
						{
#if DEBUG
							u3_printf("超时退出。。。。。\n");
#endif
						}

						g_FingerEvent.bMutex = FALSE;
					}
					else 
					{
						break;						//等待下层上传的结果状态
					}

					//retVal = Finger_Read();
					pfingerData = ((FingerData_t *) g_FingerEvent.MachineCtrl.pData);
					retVal = pfingerData->u8FingerID;

					if (retVal != -1)
					{
#if DEBUG
						snprintf(priBuf, 50, "SysTask.u16FingerfCnt : %d	 read cnt:	%d\n", SysTask.u16FingerfCnt, retVal);
						u3_printf(priBuf);
						u3_printf("\n");
#endif

						SysTask.TouchSub	= TOUCH_SUB_EXIT;

						if (((unsigned int) retVal) != SysTask.u16FingerfCnt)
						{
#if DEBUG
							u3_printf("保存的与读取的不匹配\n");
#endif

							SysTask.Stream_State = STREAM_STATE_FAILED; //灯闪烁提示
							SysTask.u8StreamShoWCnt = BLINK_CNT_8;

							if ((retVal <= FINGER_ID_MAX) && (retVal & 0x01 == 0)) //因为1020A 保存一个用户占用两个ID
							{

#if DEBUG
								u3_printf("保存新值 ");
								snprintf(priBuf, 50, "SysTask.u16FingerfCnt : %d\n", SysTask.u16FingerfCnt);
								u3_printf(priBuf);
								u3_printf("\n");
#endif

								SysTask.u16FingerfCnt = retVal;
								SysTask.u16SaveTick = 1; //1秒后保存
							}
							else 
							{
#if DEBUG
								u3_printf("全部清空\n");
#endif

								//TODO	要不要删除上面的指纹信息
								SysTask.TouchState	= TOUCH_DEL_USER;
								SysTask.TouchSub	= TOUCH_SUB_INIT;

								SysTask.u16FingerfCnt = 0;
								SysTask.u16SaveTick = 1; //1秒后保存
							}
						}
					}

					break;

				case TOUCH_SUB_WAIT: // *
					if ((SysTask.u32TouchWaitTime == 0))
					{
						if (u8SetCnt >= 20) //设置20次失败，则返回
							SysTask.TouchSub = TOUCH_SUB_ENTER;
						else 
							SysTask.TouchSub = TOUCH_SUB_INIT;
					}

					break;

				case TOUCH_SUB_EXIT: // *
					if ((SysTask.u32TouchWaitTime == 0))
					{
						FINGER_POWER_OFF(); 		//关闭指纹模块电源
						SysTask.TouchState	= TOUCH_MATCH_INIT; //重新匹配	 
						SysTask.TouchSub	= TOUCH_SUB_INIT;
						u8SetCnt			= 0;
					}

					break;

				default:
					break;
			}

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
void LedTask(void)
{
	//	  if (SysTask.LockMode[GLASS_01] != SysTask.LockModeSave[GLASS_01])
	//	  {
	//		  if (SysTask.LockMode[GLASS_01] == LOCK_ON) //将要开锁
	//		  {
	//			  GPIO_SetBits(LED_GPIO, LED_PIN_SINGLE);
	//		  }
	//		  else //将要关锁
	//		  {
	//			  GPIO_ResetBits(LED_GPIO, LED_PIN_SINGLE);
	//		  }
	//		  
	//		SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
	//	  }
}


/*******************************************************************************
* 名称: 
* 功能: 流水灯
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void LedStreamTask(void)
{
	static u8 u8LedNum	= 0;

	static u8 u8Blink	= 0;

	//流水灯效果
	switch (SysTask.Stream_State)
	{
		case STREAM_STATE_INIT:
			if (SysTask.u32StreamTime != 0)
				break;

			ws2812bClearAll();
			u8LedNum = 0;
			u8Blink = 0;

			if (SysTask.LockMode[GLASS_01] == LOCK_ON) //开锁在开锁状态
			{
				SysTask.Stream_State = STREAM_STATE_OPEN_LOCK;
			}
			else //关锁
			{
				SysTask.Stream_State = STREAM_STATE_STREAM;
			}

			break;

		case STREAM_STATE_STREAM: //流水
			if (SysTask.u32StreamTime != 0)
				break;

			SysTask.u32StreamTime = STREAM_TIME;

			if (u8LedNum > WS2812B_NUM)
			{
				ws2812bClearAll();
				u8LedNum			= 0;
				break;
			}
			else 
			{
				ws2812bSet(u8LedNum, LED_BRIGHNESS); //0X60 亮度，可调
			}

			u8LedNum++;
			break;

		case STREAM_STATE_OPEN_LOCK:
			if (SysTask.u32StreamTime != 0)
				break;

			ws2812bSetAll(LED_BRIGHNESS); //开锁时全亮，关锁再去关闭
			SysTask.u32StreamTime = (vu32) - 1;
			break;

		case STREAM_STATE_BLINK_ONE: //闪烁第一个灯
			if (SysTask.u32StreamTime != 0)
				break;

			SysTask.u32StreamTime = BLINK_TIME;

			if (u8Blink)
			{
				ws2812bSet(0, LED_BRIGHNESS);		//0X60 亮度，可调
				u8Blink 			= 0;
			}
			else 
			{
				u8Blink 			= 1;
				ws2812bClearAll();
			}

			break;

		case STREAM_STATE_BLINK_TWO: //闪烁第二个灯
			if (SysTask.u32StreamTime != 0)
				break;

			SysTask.u32StreamTime = BLINK_TIME;

			if (u8Blink)
			{
				ws2812bClearAll();
				ws2812bSet(0, LED_BRIGHNESS);		//0X60 亮度，可调 
				ws2812bSet(1, LED_BRIGHNESS);		//0X60 亮度，可调 
				u8Blink 			= 0;
			}
			else 
			{
				u8Blink 			= 1;
				ws2812bClear(1);
			}

			break;

		case STREAM_STATE_BLINK_THREE: //闪烁第三个灯
			if (SysTask.u32StreamTime != 0)
				break;

			SysTask.u32StreamTime = BLINK_TIME;

			if (u8Blink)
			{
				ws2812bClearAll();
				ws2812bSet(0, LED_BRIGHNESS);		//0X60 亮度，可调 
				ws2812bSet(1, LED_BRIGHNESS);		//0X60 亮度，可调 
				ws2812bSet(2, LED_BRIGHNESS);		//0X60 亮度，可调 
				u8Blink 			= 0;
			}
			else 
			{
				u8Blink 			= 1;
				ws2812bClear(2);
			}

			break;

		case STREAM_STATE_SUCCESS: //成功匹配
			if (SysTask.u32StreamTime != 0)
				break;

			SysTask.u32StreamTime = 2000; //亮2S 表示成功，再进入流水
			SysTask.Stream_State = STREAM_STATE_INIT;
			ws2812bSetAll(LED_BRIGHNESS);
			break;

		case STREAM_STATE_FAILED: //失败
			if (SysTask.u32StreamTime != 0)
				break;

			if (SysTask.u8StreamShoWCnt == 0) //闪烁完成，进入正常的流水状态
			{
				u8LedNum			= 0;
				u8Blink 			= 0;
				SysTask.Stream_State = STREAM_STATE_INIT;
				SysTask.u32StreamTime = 500;
				break;
			}

			SysTask.u32StreamTime = FAILED_TIME;

			if (SysTask.u8StreamShoWCnt & 0x01)
			{
				ws2812bClearAll();
			}
		else 
			{
				ws2812bSetAll(LED_BRIGHNESS);
			}

			SysTask.u8StreamShoWCnt--;
			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: SOFT PWM 控制电机转动
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void PWM_01_Task(void)
{
	static u8 u8StateSave = PWM_STATE_INIT;

	if (SysTask.PWM_State[GLASS_01] == u8StateSave)
		return;

	//A电机
	switch (SysTask.PWM_State[GLASS_01])
	{
		case PWM_STATE_INIT:
			break;

		case PWM_STATE_FEW_P:
			TIM_SetCompare3(TIM3, PWM_POSITIVE); //修改PWM占空比 ,正转
			TIM_SetCompare4(TIM3, 0); //

			//TIM3->CCR4=0;
			TIM_CtrlPWMOutputs(TIM3, ENABLE); // 使能输出PWM
			break;

		case PWM_STATE_REV_P:
			TIM_SetCompare4(TIM3, PWM_POSITIVE); //修改PWM占空比, 反转
			TIM_SetCompare3(TIM3, 0); //

			//TIM3->CCR3=0;
			TIM_CtrlPWMOutputs(TIM3, ENABLE); // 使能输出PWM
			break;

		case PWM_STATE_OPEN: //开锁
			switch (SysTask.PWM_SubState[GLASS_01])
			{
				case PWM_SUB_FEW: //正转
#if DEBUG
					u3_printf(" PWM 全速正转开锁\n");
#endif

					TIM_SetCompare3(TIM3, PWM_PERIOD); //修改PWM占空比 ,正转
					TIM_SetCompare4(TIM3, 0);
					break;

				case PWM_SUB_REV: //反转
#if DEBUG
					u3_printf(" PWM 全速反转开锁\n");
#endif

					TIM_SetCompare4(TIM3, PWM_PERIOD); //修改PWM占空比, 反转
					TIM_SetCompare3(TIM3, 0);
					break;

				default:
					break;
			}

			TIM_CtrlPWMOutputs(TIM3, ENABLE); // 使能输出PWM
			break;

		case PWM_STATE_STOP:
			TIM_SetCompare4(TIM3, 0); //
			TIM_SetCompare3(TIM3, 0); //
			TIM_CtrlPWMOutputs(TIM3, DISABLE); // 停止输出PWM，停机
		default:
			break;
	}

	u8StateSave 		= SysTask.PWM_State[GLASS_01];
}


/*******************************************************************************
* 名称: 
* 功能: 只有正反转的，转半圈 因为设置约束，电机信号线不能隔离
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_01_Task(void)
{
	//A电机
	switch (SysTask.Moto_State[GLASS_01])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD_REV)
			{
				SysTask.Moto_Time[GLASS_01] = MOTO_TIME_ANGLE; //转一定角度
				SysTask.Moto_RunTime[GLASS_01] =
					 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1] +MOTO_DEVIATION_TIME; //加系统偏差
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_FIND_ZERO;
				SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];


#if DEBUG_MOTO
				u3_printf("MOTO_STATE_INIT");
				u3_printf("上升，正转 去找原点。。\n");
#endif

				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP;

				//GPIO_SetBits(MOTO_P_GPIO, MOTO_01_PIN_P); //先正转
				//GPIO_ResetBits(MOTO_N_GPIO, MOTO_01_PIN_N); //
				SysTask.PWM_State[GLASS_01] = PWM_STATE_FEW_P; //先正转
			}
			else 
			{
				//如果此时，还没有回到原点？只有在开始还没有进入转动状态时按下平板的stop键才会进入，按道理这种情况非常少见，几乎没有
				//如果flash保存的是这种状态，则也有这种可能 
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_STOP;
				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN;
				SysTask.Moto_RunTime[GLASS_01] = 0;
				SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];

			}

			break;

		case MOTO_STATE_FIND_ZERO:
			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_UP: //上升，正转
					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // 找到原点，
					{
#if DEBUG_MOTO
						u3_printf("MOTO_STATE_FIND_ZERO ");
						u3_printf("正转 找到原点，");
						u3_printf("\n");
#endif

						SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_CHA;
						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP_F; //继续正转 forward
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //转过一定的角度没有找到原点，返回去找原点
					{
#if DEBUG_MOTO
						u3_printf("MOTO_STATE_FIND_ZER");
						u3_printf("正转 没有到原点");
						u3_printf("\n");
#endif

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN;

						//以临界正转终点为初始条件，前面已经正转一个角度，现在要先纠正回到之前位置再反转，所以时间要乘以 2
						//默认上一次关机是没有检测到零点时间加上MOTO_DEVIATION_TIME
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + (g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1] +MOTO_DEVIATION_TIME + MOTO_DEVIATION_TIME) * 2 + MOTO_DEVIATION_TIME;

						//GPIO_SetBits(MOTO_N_GPIO, MOTO_01_PIN_N); //反转
						//GPIO_ResetBits(MOTO_P_GPIO, MOTO_01_PIN_P); //
						SysTask.PWM_State[GLASS_01] = PWM_STATE_REV_P;
					}

					break;

				case MOTO_SUB_STATE_DOWN: //下降， 反转
					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // 找到原点，
					{
#if DEBUG_MOTO
						u3_printf("MOTO_STATE_FIND_ZERO");
						u3_printf("反转 找到原点，\n");
#endif

						SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_CHA;
						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN_F; //继续返转 forward
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //转过一定的角度没有找到原点，零点失效，以当前角度为反转终点
					{
#if DEBUG_MOTO
						u3_printf("MOTO_STATE_FIND_ZERO");
						u3_printf("反转没有到原点\n");
#endif

						SysTask.Moto_State[GLASS_01] = MOTO_STATE_CHANGE_DIR; //从头开始正转

						g_bSensorState		= FALSE;

						//发送状态给平板端
						g_FingerAck 		= ACK_FAILED;
						g_AckType			= DETECT_ACK_TYPE;
						g_GlassSendAddr 	= GLASS_01;

						if (SysTask.SendState == SEND_IDLE)
							SysTask.SendState = SEND_TABLET_DETECT;
					}

					break;

				default:
					break;
			}

			break;

		case MOTO_STATE_CHANGE_DIR:
			SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_CHA;
			SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP_F;
			SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];

#if DEBUG_MOTO
			u3_printf("\n---------------正常运行状态-----------\n");
#endif

			break;

		case MOTO_STATE_RUN_CHA: //正反转运行状态， 只有这种状态 ，在这个模式下面
			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_UP: //上升，正转
					if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，正转回去找原点 开锁. 这部分要在moto那里改，不可以在PWM那里改
					{
						SysTask.PWM_State[GLASS_01] = PWM_STATE_OPEN;
						SysTask.PWM_SubState[GLASS_01] = PWM_SUB_FEW;
						break;
					}

					if (SysTask.Moto_RunTime[GLASS_01] == 0) // 由时间控制 ，说明已经转了固定角度
					{
#if DEBUG_MOTO
						u3_printf("MOTO_SUB_STATE_UP");
						u3_printf("正转\n");
#endif

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_ZERO_R;

						if (g_bSensorState == TRUE)
							SysTask.Moto_RunTime[GLASS_01] =
								 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1] +MOTO_DEVIATION_TIME; //下一步会去找零点，所以要加上一个系统偏差的时间
						else 
							SysTask.Moto_RunTime[GLASS_01] =
								 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1]; //没有感应到零点的情况下以时间的基准，不用加上偏差

						SysTask.PWM_State[GLASS_01] = PWM_STATE_FEW_P; //先正转
					}

					break;

				case MOTO_SUB_STATE_ZERO_R: //找零点
					if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，正转回去找原点 开锁. 这部分要在moto那里改，不可以在PWM那里改
					{
						SysTask.PWM_State[GLASS_01] = PWM_STATE_OPEN;
						SysTask.PWM_SubState[GLASS_01] = PWM_SUB_FEW;
					}

					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // 找到原点，
					{
#if DEBUG_MOTO
						u3_printf(" MOTO_SUB_STATE_ZERO_R");
						u3_printf("正转 找到原点，\n");
#endif

						if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，不做任何处理
							break;

						if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01]) //状态改变，由转动进入停止状态时候会进入这个分支 
						{
#if DEBUG_MOTO
							u3_printf(" MOTO_SUB_STATE_ZERO_R");
							u3_printf("状态改变\n");
#endif

							SysTask.PWM_State[GLASS_01] = PWM_STATE_STOP;

							SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
							break;
						}

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP_F;
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //转过一定的角度没有找到原点
					{
#if DEBUG_MOTO
						u3_printf(" MOTO_SUB_STATE_ZERO_R");
						u3_printf("正转	没有找到原点，\n");
#endif

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP_F;
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
					}

					break;

				case MOTO_SUB_STATE_UP_F: //继续正转 forward
					if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，正转回去找原点 开锁. 这部分要在moto那里改，不可以在PWM那里改
					{
						SysTask.PWM_State[GLASS_01] = PWM_STATE_OPEN;
						SysTask.PWM_SubState[GLASS_01] = PWM_SUB_REV;
						break;
					}

					if (SysTask.Moto_RunTime[GLASS_01] == 0) // 由时间控制 ，说明已经转了固定角度
					{
#if DEBUG_MOTO
						u3_printf("MOTO_SUB_STATE_UP_F");
						u3_printf("继续正转，\n");
#endif

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN;
					}

					break;

				case MOTO_SUB_STATE_DOWN: //下降， 反转
#if DEBUG_MOTO
					u3_printf("MOTO_SUB_STATE_DOWN");
					u3_printf("反转，\n");
#endif

					if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，正转回去找原点 开锁. 这部分要在moto那里改，不可以在PWM那里改
					{
						SysTask.PWM_State[GLASS_01] = PWM_STATE_OPEN;
						SysTask.PWM_SubState[GLASS_01] = PWM_SUB_REV;
						break;
					}

					if (g_bSensorState == TRUE)
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1] + MOTO_DEVIATION_TIME;
					else 
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];

					SysTask.PWM_State[GLASS_01] = PWM_STATE_REV_P;
					SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_ZERO_L;
					break;

				case MOTO_SUB_STATE_ZERO_L: //找零点
					if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，正转回去找原点 开锁. 这部分要在moto那里改，不可以在PWM那里改
					{
						SysTask.PWM_State[GLASS_01] = PWM_STATE_OPEN;
						SysTask.PWM_SubState[GLASS_01] = PWM_SUB_REV;
					}

					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // 找到原点，
					{
#if DEBUG_MOTO
						u3_printf(" MOTO_SUB_STATE_ZERO_L");
						u3_printf("找到原点，\n");
#endif

						if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，不做任何处理
							break;

						if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01]) //状态改变，由转动进入停止状态时候会进入这个分支 
						{
#if DEBUG_MOTO
							u3_printf(" MOTO_SUB_STATE_ZERO_L");
							u3_printf("状态改变\n");
#endif

							//GPIO_ResetBits(MOTO_P_GPIO, MOTO_01_PIN_P); //先暂停电机转动
							//GPIO_ResetBits(MOTO_N_GPIO, MOTO_01_PIN_N); //先暂停电机转动
							SysTask.PWM_State[GLASS_01] = PWM_STATE_STOP;
							SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
							break;
						}

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN_F;
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1] + MOTO_DEVIATION_TIME;
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //转过一定的角度没有找到原点，
					{
#if DEBUG_MOTO
						u3_printf("MOTO_SUB_STATE_ZERO_L");
						u3_printf("没有找到 原点，\n");
#endif

						if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，不做任何处理
							break;

						if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01]) //状态改变，由转动进入停止状态时候会进入这个分支 
						{
							SysTask.PWM_State[GLASS_01] = PWM_STATE_STOP;
							SysTask.PWM_SubState[GLASS_01] = PWM_SUB_INIT;

							SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
							break;
						}

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN_F;
						SysTask.Moto_RunTime[GLASS_01] =
							 SysTask.nTick + g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][1];
					}

					break;

				case MOTO_SUB_STATE_DOWN_F: //继续返转 forward
					if (SysTask.Lock_State[GLASS_01] == LOCK_STATE_DEBOUNSE) //准备开锁，正转回去找原点 开锁. 这部分要在moto那里改，不可以在PWM那里改
					{
						SysTask.PWM_State[GLASS_01] = PWM_STATE_OPEN;
						SysTask.PWM_SubState[GLASS_01] = PWM_SUB_FEW;
						break;
					}

					if (SysTask.Moto_RunTime[GLASS_01] == 0) // 由时间控制 ，说明 已经转了固定角度
					{
#if DEBUG_MOTO
						u3_printf("MOTO_SUB_STATE_DOWN_F");
						u3_printf("继续反转，\n");
						u3_printf("\n------------- 重复-------------\n");
#endif

						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP;
					}

					break;

				default:
					break;
			}

			break;

		case MOTO_STATE_STOP:
			if ((GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) ||
				 (SysTask.PWM_State[GLASS_01] == PWM_STATE_STOP)) //要求回到原点才开锁
			{
#if DEBUG_MOTO
				u3_printf(" MOTO_STATE_STOP 停止\n");
#endif

				//GPIO_ResetBits(MOTO_P_GPIO, MOTO_01_PIN_P); //先暂停电机转动
				//GPIO_ResetBits(MOTO_N_GPIO, MOTO_01_PIN_N); //先暂停电机转动
				SysTask.PWM_State[GLASS_01] = PWM_STATE_STOP;
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE;
			}

			break;

		case MOTO_STATE_WAIT:
			break;

		case MOTO_STATE_IDLE:
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01])
			{
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //先延时一段时间再翻转
				break;
			}

			break;

		default:
			break;
	}
}


/*******************************************************************************
* 名称: 
* 功能: 开锁任务，如果指纹识别正确则可以开锁，如果 在开锁状态下任意一个指纹按下?
	?关锁，没有录入的也可以关 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Lock_01_Task(void)
{

	switch (SysTask.Lock_State[GLASS_01]) //_01锁
	{
		case LOCK_STATE_DETECT:
			if (SysTask.LockMode[GLASS_01] != SysTask.LockModeSave[GLASS_01])
			{
				if (SysTask.LockMode[GLASS_01] == LOCK_ON) //将要开锁
				{
					SysTask.Lock_StateTime[GLASS_01] = MOTO_DEBOUNCE; //延时
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_DEBOUNSE;
					SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
					SysTask.Lock_OffTime[GLASS_01] = LOCK_OFFTIME;
					GPIO_SetBits(LED_GPIO, LED_PIN_SINGLE);
				}
				else //将要关锁
				{
					SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
					SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_L_TIME;
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_SENT;
					GPIO_ResetBits(LED_GPIO, LED_PIN_SINGLE);
				}
			}

			break;

		case LOCK_STATE_DEBOUNSE: //
			if (SysTask.Lock_StateTime[GLASS_01] == 0)
			{
				if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) != 0) //要求回到原点才开锁
					break;

#if DEBUG
				u3_printf(" lock 开锁。。。。。\n");
#endif

				SysTask.Moto_WaitTime[GLASS_01] = MOTO_DEBOUNCE;
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_STOP; //停止转动

				SysTask.Lock_State[GLASS_01] = LOCK_STATE_OPEN;


				GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN); //开锁
				SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_HAL_OPEN;
			}

			break;

		case LOCK_STATE_OPEN:
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

#if DEBUG
			u3_printf(" lock 停止开锁信号发送\n");
#endif

			GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN); //停止开锁信号发送
			SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;

			if (SysTask.SendState == SEND_IDLE)
				SysTask.SendState = SEND_TABLET_ZERO;

			g_FingerAck = ACK_SUCCESS;
			g_AckType = ZERO_ACK_TYPE;
			g_GlassSendAddr = GLASS_01;
			break;

		case LOCK_STATE_SENT:
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

#if DEBUG
			u3_printf(" lock 开关锁。。。。。\n");
#endif

			GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN); //开锁和关锁都 是发送一个短时间的高电平 
			SysTask.Lock_State[GLASS_01] = LOCK_STATE_OFF;
			SysTask.Lock_StateTime[GLASS_01] = LOCK_HAL_OPEN; ////延时
			break;

		case LOCK_STATE_OFF: //
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN); //停止开锁信号发送
			SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_L_TIME;
			SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;

			// 因为转动的单片机没有应答我们这边  ，所以这个不用上传到平板端，默认是转动一开始就是成功的
			//			if (SysTask.SendState == SEND_IDLE)
			//				SysTask.SendState = SEND_TABLET_FINGER;
			//			g_FingerAck = ACK_SUCCESS;
			//			g_AckType = ZERO_ACK_TYPE;
			//			g_GlassSendAddr = GLASS_01;
			//	点击了转动，这里让他转动起来,且要延时一段时间，主关锁完全 之后再转动
			SysTask.Moto_StateTime[GLASS_01] = MOTO_LOCK_SUCCESS;
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

			//				memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
			for (i = 0; i < GLASS_MAX_CNT; i++)
			{
				SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
				SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
				SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
			}

			SysTask.FlashData.Data.u16FingerfCnt = SysTask.u16FingerfCnt;
			SysTask.FlashData.Data.u16LedMode = LED_MODE_DEF; //LED模式
			u8Status = SAVE_WRITE;
			break;

		case SAVE_WRITE:
			if (FLASH_WriteMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, DATA_READ_CNT + LED_MODE_CNT + FINGER_CNT))
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
* 功能:按键任务
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void KeyTask(void)
{
	int pressTime		= 0;

	switch (SysTask.Key_State)
	{
		case KEY_INIT:
			if (!GPIO_ReadInputDataBit(KEY_GPIO, KEY_01_PIN))
			{
				SysTask.KeyStateTime = KEY_PRESS_TIME;
				SysTask.Key_State	= KEY_DEBOUCH;
			}

			break;

		case KEY_DEBOUCH:
			if (!GPIO_ReadInputDataBit(KEY_GPIO, KEY_01_PIN))
				break;

			pressTime = KEY_PRESS_TIME - SysTask.KeyStateTime;

			if (SysTask.TouchState != TOUCH_MATCH_INIT) //指纹模块还在其他 的任务状态 里面没有出来 ，等待指纹 模块任务状态 退出之后 再进入
				return;

			if (pressTime < KEY_DEBOUCH_TIME)
			{ //抖动
				SysTask.Key_State	= KEY_INIT;
			}
			else if (pressTime < KEY_SHORT_TIME)
			{
				SysTask.Key_State	= KEY_ADD_USER;
			}
			else 
			{
				SysTask.Key_State	= KEY_CLR_USER;
			}

			break;

		case KEY_ADD_USER:
#if DEBUG
			u3_printf("短按，添加用户\n");
#endif

			SysTask.TouchState = TOUCH_KEY_SHORT;
			SysTask.TouchSub = TOUCH_SUB_INIT;
			SysTask.Key_State = KEY_WAIT;
			SysTask.KeyStateTime = 400; //延时一定时间再允许操作key
			break;

		case KEY_CLR_USER:
#if DEBUG
			u3_printf("长按，删除用户\n");
#endif

			SysTask.TouchState = TOUCH_DEL_USER;
			SysTask.Key_State = KEY_WAIT;
			SysTask.KeyStateTime = 400; //延时一定时间再允许操作key
			break;

		case KEY_WAIT:
			if ((SysTask.KeyStateTime != 0) || (SysTask.TouchState != TOUCH_MATCH_INIT)) //增加删除用户需要一定的时间
				break;

#if DEBUG
			u3_printf("-----重启按键任务-----\n");
#endif

			SysTask.Key_State = KEY_INIT;
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
void MainTask(void)
{
	SysSaveDataTask();
	KeyTask();
	FingerTouchTask();								//指纹模块
	Moto_01_Task(); 								//只有正反转的
	PWM_01_Task();
	Lock_01_Task();
	LedTask();
	LedStreamTask();
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
	g_FingerEvent.bInit = TRUE;
	g_FingerEvent.bMutex = FALSE;
	g_FingerEvent.MachineCtrl.pData = (void *)
	malloc(sizeof(FingerData_t));
	g_FingerEvent.MachineCtrl.CallBack = Finger_Event;
	g_FingerEvent.MachineCtrl.u16InterValTime = 5;	//5ms 进入一次

	g_FingerEvent.MachineRun.bEnable = TRUE;
	g_FingerEvent.MachineRun.u32StartTime = 0;
	g_FingerEvent.MachineRun.u32LastTime = 0;
	g_FingerEvent.MachineRun.u32RunCnt = 0;
	g_FingerEvent.MachineRun.bFinish = FALSE;
	g_FingerEvent.MachineRun.u16RetVal = 0;
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

		//			memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
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

			//SysTask.FlashData.Data.u16FingerfCnt = SysTask.u16FingerfCnt;
			//SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LED模式
			break;
		}

	}

	//		使用memcpy会导致TIM 2 TICk无法进入中断，其他串口中断没有受到影响，原因未知，复制8个是没有问题
	//		复制9个以上就会出现。
	//		memcpy(SysTask.WatchState, SysTask.FlashData.u8FlashData, 10);
	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.WatchState[i].MotoTime = SysTask.FlashData.Data.WatchState[i].MotoTime;
		SysTask.WatchState[i].u8Dir = SysTask.FlashData.Data.WatchState[i].u8Dir;

		//			SysTask.WatchState[i].u8LockState	= SysTask.FlashData.Data.WatchState[i].u8LockState;  //锁状态现在不用上传，由下边单片机保证
	}

	SysTask.LED_Mode	= SysTask.FlashData.Data.u16LedMode;
	SysTask.u16FingerfCnt = SysTask.FlashData.Data.u16FingerfCnt;

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

	SysTask.TouchSub	= TOUCH_SUB_INIT;
	SysTask.bEnFingerTouch = FALSE;
	SysTask.nTick		= 0;
	SysTask.u16SaveTick = 0;
	SysTask.bEnSaveData = FALSE;

	SysTask.TouchState	= TOUCH_CHECK;				//优先设置指纹模式
	FINGER_POWER_ON();								//给指纹模块DSP上电，，后面会检查一次指纹ID 是否与  EEPRO		M  一致
	SysTask.u32TouchWaitTime = POWER_ON_DELAY;		//


	SysTask.SendState	= SEND_IDLE;
	SysTask.SendSubState = SEND_SUB_INIT;
	SysTask.bTabReady	= FALSE;
	SysTask.LED_ModeSave = LED_MODE_DEF;			// 保证第一次可以运行模式
	SysTask.Stream_State = STREAM_STATE_INIT;
	SysTask.u32StreamTime = 0;
	SysTask.Key_State	= KEY_INIT;
	SysTask.KeyStateTime = 0;
	SysTask.u8StreamShoWCnt = 0;



	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.LockModeSave[i] = LOCK_OFF;
		SysTask.LockMode[i] = LOCK_OFF;
	}


	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.PWM_State[i] = PWM_STATE_INIT;
		SysTask.PWM_SubState[i] = PWM_SUB_INIT;

	}

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.Moto_StateTime[i] = 0;
		SysTask.Moto_State[i] = MOTO_STATE_INIT;	//保证开机起来会转
		SysTask.Moto_SubState[i] = MOTO_SUB_STATE_RUN;
		SysTask.Lock_StateTime[i] = 0;				//			
	}

	SysEventInit();

	//		SysTask.u16BootTime = 3000;	//3秒等待从机启动完全
}


