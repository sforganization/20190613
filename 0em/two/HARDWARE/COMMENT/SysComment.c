

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


#define SLAVE_WAIT_TIME 		3000	//3s �ӻ�Ӧ��ʱ��

#define LOCK_SET_L_TIME 		50	  //�͵�ƽʱ�� 25 ~ 60ms
#define LOCK_SET_H_TIME 		50	  //�ߵ�ƽʱ��
#define LOCK_SET_OFF_TIME		1200	  //����Ƭ����Ӧʱ�䣬�������֮����ת


#define MOTO_UP_TIME			800			//�������ʱ��
#define MOTO_DOWN_TIME			1500		//����½�ʱ��




#define MOTO_TRUN_OVER			500			//MOTO 3  �����ƽ��תʱ��


#define MOTO_ANGLE_TIME 		(750 * PWM_PERIOD / PWM_POSITIVE)  //���ת��һ���Ƕȵ�ʱ��
#define MOTO_DEVIATION_TIME 	700  //���ת��ʱ��ƫ�� 200ms ����ƫ��������дflashʱ����Ϊдflash��ֹһ�� flash����������Ķ�ȡ���������жϣ�

//ע�⣬���ڴ��ڽ��ջ�ռ��һ����ʱ�䣬�������ʱҪ��Ӧ�ӳ����ر��ǵ���׿����ֹͣ���������ʱ��������ڼ�����ת����תʱ�򷢣�����Ѿ�������һ�Σ���������Ҫ��Ӧ�ӳ�ʱ��
//��Ϊ
#define LOCK_OPEN_TIME			500			//�������͵�ƽʱ��




PollEvent_t 	g_RangeEvent;
   
float           g_LedArr[LED_BRIGHNESS]; //����洢���ٹ�����ÿһ����װ��ֵ


typedef enum 
{
ACK_SUCCESS = 0,									//�ɹ� 
ACK_FAILED, 										//ʧ��
ACK_AGAIN,											//�ٴ�����
ACK_ERROR,											//�������
ACK_DEF = 0xFF, 
} Finger_Ack;


//�ڲ�����
static vu16 	mDelay;
u8				g_GlassSendAddr = 0; //Ҫ���͵��ֱ��ַ
u8				g_FingerAck = 0; //Ӧ����������
u8				g_AckType = 0; //Ӧ������

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


//�ڲ�����
void SysTickConfig(void);


/*******************************************************************************
* ����: SysTick_Handler
* ����: ϵͳʱ�ӽ���1MS
* �β�:		
* ����: ��
* ˵��: 
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
			IWDG_ReloadCounter();					/*ιSTM32����Ӳ����*/

			if ((++mSysSoftDog) > 5) /*��system  DOG 2S over*/
			{
				mSysSoftDog 		= 0;
				NVIC_SystemReset();
			}
		}
	}
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
* ����: Strcpy()
* ����: 
* �β�:		
* ����: ��
* ˵��: 
******************************************************************************/
void Strcpy(u8 * str1, u8 * str2, u8 len)
{
	for (; len > 0; len--)
	{
		*str1++ 			= *str2++;
	}
}


/*******************************************************************************
* ����: Strcmp()
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
* ����: Sys_DelayMS()
* ����: ϵͳ�ӳٺ���
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Sys_DelayMS(uint16_t nms)
{
	mDelay				= nms + 1;

	while (mDelay != 0x0)
		;
}


/*******************************************************************************
* ����: Sys_LayerInit
* ����: ϵͳ��ʼ��
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Sys_LayerInit(void)
{
	SysTickConfig();

	mSysSec 			= 0;
	mSysTick			= 0;
	SysTask.mUpdate 	= TRUE;
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Sys_IWDGConfig(u16 time)
{
	/* д��0x5555,�����������Ĵ���д�빦�� */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* ����ʱ�ӷ�Ƶ,40K/64=0.625K()*/
	IWDG_SetPrescaler(IWDG_Prescaler_64);

	/* ι��ʱ�� TIME*1.6MS .ע�ⲻ�ܴ���0xfff*/
	IWDG_SetReload(time);

	/* ι��*/
	IWDG_ReloadCounter();

	/* ʹ�ܹ���*/
	IWDG_Enable();
}


/*******************************************************************************
* ����: Sys_IWDGReloadCounter
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Sys_IWDGReloadCounter(void)
{
	mSysSoftDog 		= 0;						//ι��
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void FingerTouchTask(void)
{
	SearchResult seach;
	u8 ensure;
	static u8 u8MatchCnt = 0;						//ƥ��ʧ�ܴ�����Ĭ��ƥ��MATCH_FINGER_CNT�� 

	if ((SysTask.bEnFingerTouch == TRUE))
	{
		switch (SysTask.TouchState)
		{
			case TOUCH_MATCH_INIT:
				if (PS_Sta) //��ָ�ư���
				{
					SysTask.nWaitTime	= 10;		//һ��ʱ���ټ��
					SysTask.TouchState	= TOUCH_MATCH_AGAIN;
					u8MatchCnt			= 0;
				}

				break;

			case TOUCH_MATCH_AGAIN:
				if (SysTask.nWaitTime == 0) //��ָ�ư���
				{
					if (PS_Sta) //��ָ�ư���
					{
						ensure				= PS_GetImage();

						if (ensure == 0x00) //���������ɹ�
						{
							//PS_ValidTempleteNum(&u16FingerID); //����ָ�Ƹ���
							ensure				= PS_GenChar(CharBuffer1);

							if (ensure == 0x00) //���������ɹ�
							{
								ensure				= PS_Search(CharBuffer1, 0, FINGER_MAX_CNT, &seach);

								if (ensure == 0) //ƥ��ɹ�
								{
									SysTask.u16FingerID = seach.pageID;
									SysTask.nWaitTime	= SysTask.nTick + 2000; //��ʱһ��ʱ�� �ɹ�Ҳ����һֱ��
									SysTask.TouchSub	= TOUCH_SUB_INIT;
									SysTask.TouchState	= TOUCH_MATCH_INIT; //����ƥ��

									//								  SysTask.bEnFingerTouch = FALSE;	 //�ɹ������ָֹ��ģ������
									//ƥ��ɹ������͵�ƽ��ˣ�Ӧ��
									g_FingerAck 		= ACK_SUCCESS;
									g_AckType			= FINGER_ACK_TYPE;

									if (SysTask.SendState == SEND_IDLE)
										SysTask.SendState = SEND_TABLET_FINGER;
								}
								else if (u8MatchCnt >= MATCH_FINGER_CNT) // 50 * 3 ��Լ200MS��
								{
									//								  SysTask.bEnFingerTouch = FALSE;	 //�ɹ������ָֹ��ģ������
									//ƥ��ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
									g_FingerAck 		= ACK_FAILED;
									g_AckType			= FINGER_ACK_TYPE;

									if (SysTask.SendState == SEND_IDLE)
										SysTask.SendState = SEND_TABLET_FINGER;

									SysTask.TouchState	= TOUCH_MATCH_INIT; //����ƥ��
									SysTask.nWaitTime	= SysTask.nTick + 300; //��ʱһ��ʱ�� ����ֳ�ʶ���ˣ�������һ���˼�⣬
								}
							}
						}

						if (ensure) //ƥ��ʧ��
						{
							SysTask.nWaitTime	= SysTask.nTick + 10; //��ʱһ��ʱ�� �����⵽��Ӧ����ͬһ���˵�ָ��
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
							if ((PS_Sta) && (SysTask.nWaitTime == 0)) //�����ָ�ư���
							{
								ensure				= PS_GetImage();

								if (ensure == 0x00)
								{
									ensure				= PS_GenChar(CharBuffer1); //��������

									if (ensure == 0x00)
									{
										SysTask.nWaitTime	= 3000; //��ʱһ��ʱ����ȥ�ɼ�
										SysTask.TouchSub	= TOUCH_SUB_AGAIN; //�����ڶ���				

										//Please press	agin
										//�ɹ������͵�ƽ��ˣ�Ӧ������ٰ�һ��
										g_FingerAck 		= ACK_AGAIN;
										g_AckType			= FINGER_ACK_TYPE;

										if (SysTask.SendState == SEND_IDLE)
											SysTask.SendState = SEND_TABLET_FINGER;

									}
								}
							}

							break;

						case TOUCH_SUB_AGAIN:
							if ((PS_Sta) && (SysTask.nWaitTime == 0)) //�����ָ�ư���
							{
								ensure				= PS_GetImage();

								if (ensure == 0x00)
								{
									ensure				= PS_GenChar(CharBuffer2); //��������

									if (ensure == 0x00)
									{
										ensure				= PS_Match(); //�Ա�����ָ��

										if (ensure == 0x00) //�ɹ�
										{

											ensure				= PS_RegModel(); //����ָ��ģ��

											if (ensure == 0x00)
											{

												ensure				= PS_StoreChar(CharBuffer2, SysTask.u16FingerID); //����ģ��

												if (ensure == 0x00)
												{
													SysTask.TouchSub	= TOUCH_SUB_WAIT;
													SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�

													//													  SysTask.bEnFingerTouch = FALSE;	 //�ɹ������ָֹ��ģ������
													//�ɹ������͵�ƽ��ˣ�Ӧ��
													g_FingerAck 		= ACK_SUCCESS;
													g_AckType			= FINGER_ACK_TYPE;

													if (SysTask.SendState == SEND_IDLE)
														SysTask.SendState = SEND_TABLET_FINGER;
												}
												else 
												{
													//ƥ��ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
													SysTask.TouchSub	= TOUCH_SUB_ENTER;
													SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
												}
											}
											else 
											{
												//ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
												g_FingerAck 		= ACK_FAILED;
												g_AckType			= FINGER_ACK_TYPE;

												if (SysTask.SendState == SEND_IDLE)
													SysTask.SendState = SEND_TABLET_FINGER;

												SysTask.TouchSub	= TOUCH_SUB_ENTER;
												SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
											}
										}
										else 
										{
											//ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
											g_FingerAck 		= ACK_FAILED;
											g_AckType			= FINGER_ACK_TYPE;

											if (SysTask.SendState == SEND_IDLE)
												SysTask.SendState = SEND_TABLET_FINGER;

											SysTask.TouchSub	= TOUCH_SUB_ENTER;
											SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
										}
									}
								}
							}

							break;

						case TOUCH_SUB_WAIT: // *
							if ((SysTask.nWaitTime == 0))
							{
								SysTask.TouchState	= TOUCH_IDLE;
								SysTask.bEnFingerTouch = FALSE; //�ɹ������ָֹ��ģ������
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
								//���͵�ƽ�����
								g_FingerAck 		= ACK_SUCCESS;
								g_AckType			= FINGER_ACK_TYPE;

								if (SysTask.SendState == SEND_IDLE)
									SysTask.SendState = SEND_TABLET_FINGER;

								SysTask.TouchSub	= TOUCH_SUB_WAIT;
								SysTask.nWaitTime	= 100; //��ʱһ��ʱ���˳�
							}
							else //ɾ��ʧ�� ,
							{
								u8MatchCnt++;
								SysTask.nWaitTime	= 500; //��ʱһ��ʱ���ٴ�ɾ��

								if (u8MatchCnt >= 3) //ʧ���˳�
								{
									//���͵�ƽ�����
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
								SysTask.bEnFingerTouch = FALSE; //�ɹ������ָֹ��ģ������
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
* ����: 
* ����: У���
* �β�:		
* ����: ��
* ˵��: 
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
* ����: 
* ����: �ӻ�������
*		��ͷ + �ӻ���ַ + 	 �ֱ��ַ + ���� + ���ݰ�[3] + У��� + ��β
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SlavePackageSend(u8 u8SlaveAddr, u8 u8GlassAddr, u8 u8Cmd, u8 * u8Par)
{
	u8 i				= 0;
	u8 u8SendArr[9] 	=
	{
		0
	};

	u8SendArr[0]		= 0xAA; 					//��ͷ
	u8SendArr[1]		= u8SlaveAddr;				//�ӻ���ַ
	u8SendArr[2]		= u8GlassAddr;				//�ֱ��ַ
	u8SendArr[3]		= u8Cmd;					//����
	u8SendArr[4]		= u8Par[0]; 				//����1
	u8SendArr[5]		= u8Par[1]; 				//����2
	u8SendArr[6]		= u8Par[2]; 				//����3

	u8SendArr[7]		= CheckSum(u8SendArr, 7);	//У���
	u8SendArr[8]		= 0x55; 					//��β


	for (i = 0; i < 8; i++)
	{
		USART_SendData(USART3, u8SendArr[i]);
	}
}


#define RECV_PAKAGE_CHECK		10	 //   10�����ݼ���

/*******************************************************************************
* ����: �����հ���������ƽ�巢�����������������
* ����: �ж��жϽ��յ�������û�и������ݰ� ���߾�����
* �β�:		
* ����: -1, ʧ�� 0:�ɹ�
* ˵��: 
*******************************************************************************/
static int Usart1JudgeStr(void)
{
	//	  ��ͷ + ���� +����[3]		+����[3]	  +  У��� + ��β
	//	  ��ͷ��0XAA
	//	  ��� CMD_UPDATE	 �� CMD_READY 
	//	  ������ ��ַ��LEDģʽ������ɾ��ָ��ID,
	//	  ���ݣ��������� + ���� + ʱ�䣩
	//	  У��ͣ�
	//	  ��β�� 0X55
	//��������ƽ��İ���Ϊ���֡���һ���ǵ�������һ����ȫ��������
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

	if (* (data + 9) != 0X55) //��β
		return - 1;

	for (i = 0; i < RECV_PAKAGE_CHECK; i++)
		u8CheckSum += * (data + i);

	if (u8CheckSum != 0x55 - 1) //����Ͳ����ڰ�β ����ʧ��
		return - 1;


	u8Cmd				= * (data + 1);

	switch (u8Cmd)
	{
		case CMD_READY:
			SysTask.bTabReady = TRUE;
			SysTask.SendState = SEND_INIT;
			break;

		case CMD_UPDATE:
			if (* (data + 2) == 0xFF) //��ȫ���ֱ�
			{
				for (i = 0; i < GLASS_MAX_CNT; i++)
				{
					//SysTask.WatchState[i].u8LockState  = *(data + 5);  ////��״̬���ڴ˸��£���Ϊ������Ҫ����֮��
					SysTask.WatchState[i].u8Dir = * (data + 6);
					SysTask.WatchState[i].MotoTime = * (data + 7);

					SysTask.Moto_Mode[i] = (MotoFR) * (data + 6);
					SysTask.Moto_Time[i] = (MotoTime_e) * (data + 7);
					SysTask.Moto_RunTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][1];
					SysTask.Moto_WaitTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][2];

				}
			}
			else //�����ֱ��״̬����
			{
				//				  SysTask.WatchState[*(data + 2)].u8LockState  = *(data + 5);//��״̬���ڴ˸��£���Ϊ������Ҫ����֮��
				SysTask.WatchState[* (data + 2)].u8Dir = * (data + 6);
				SysTask.WatchState[* (data + 2)].MotoTime = * (data + 7);
				SysTask.Moto_Mode[* (data + 2)] = (MotoFR) * (data + 6);
				SysTask.Moto_Time[* (data + 2)] = (MotoTime_e) * (data + 7);

				tmp 				= SysTask.Moto_Time[* (data + 2)];
				SysTask.Moto_RunTime[* (data + 2)] = g_au16MoteTime[tmp][1];
				SysTask.Moto_WaitTime[* (data + 2)] = g_au16MoteTime[tmp][2];
			}

			SysTask.u16SaveTick = SysTask.nTick + SAVE_TICK_TIME; //���ݸ��±���
			break;

		case CMD_ADD_USER:
			if (* (data + 2) <= FINGER_MAX_CNT)
			{
				SysTask.bEnFingerTouch = TRUE;
				SysTask.TouchState	= TOUCH_ADD_USER;
				SysTask.TouchSub	= TOUCH_SUB_INIT; //�����ɾ��ָ�Ƶ�����ָ��
				SysTask.u16FingerID = * (data + 2);
			}

			break;

		case CMD_DEL_USER:
			if (* (data + 2) <= FINGER_MAX_CNT)
			{
				SysTask.bEnFingerTouch = TRUE;
				SysTask.TouchState	= TOUCH_DEL_USER;
				SysTask.TouchSub	= TOUCH_SUB_INIT; //�����ɾ��ָ�Ƶ�����ָ��
				SysTask.u16FingerID = * (data + 2);
			}

			break;

		case CMD_LOCK_MODE: //
			if (* (data + 2) == 0xFF) //ȫ����״̬
			{
				for (i = 0; i < GLASS_MAX_CNT; i++)
				{
					SysTask.LockMode[i] = * (data + 5);
				}
			}
			else //������״̬����
			{
				SysTask.LockMode[* (data + 2)] = * (data + 5); //���ת������״̬����OFF?
			}

			break;

		//	 
		//			case CMD_RUN_MOTO:
		//				if (* (data + 2) == 0xFF) //��ȫ���ֱ�
		//				{
		//					for (i = 0; i < GLASS_MAX_CNT; i++)
		//					{
		//						SysTask.LockMode[i] = LOCK_OFF; //���ת������״̬����OFF?
		//						SysTask.Moto_State[i] = MOTO_STATE_INIT;
		//					}
		//				}
		//				else //�����ֱ��״̬����
		//				{
		//					SysTask.LockMode[* (data + 2)] = LOCK_OFF; //���ת������״̬����OFF?
		//					SysTask.Moto_State[* (data + 2)] = MOTO_STATE_INIT;
		//				}
		//	 
		//				break;
		//	 
		case CMD_LED_MODE:
			if ((* (data + 2) == LED_MODE_ON) || (* (data + 2) == LED_MODE_OFF)) //LEDģʽ��Ч
			{
				SysTask.LED_Mode	= * (data + 2);
			}

			SysTask.u16SaveTick = SysTask.nTick + SAVE_TICK_TIME; //���ݸ��±���
			break;

		case CMD_PASSWD_ON: // ����ģʽ������ָ��
			SysTask.bEnFingerTouch = TRUE;
			SysTask.TouchState = TOUCH_MATCH_INIT;
			break;

		case CMD_PASSWD_OFF: // ����ģʽ���ر�ָ��
			SysTask.bEnFingerTouch = FALSE;
			SysTask.TouchState = TOUCH_IDLE;
			break;

		default:
			//���͵�ƽ��ˣ�Ӧ��	 �������
			g_FingerAck = ACK_ERROR;
			g_AckType = ERROR_ACK_TYPE;

			if (SysTask.SendState == SEND_IDLE)
				SysTask.SendState = SEND_TABLET_FINGER;

			break;
	}

	return 0; //		   
}


/*******************************************************************************
* ����: 
* ����: ƽ����������͵����������
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void TabletToHostTask(void)
{
	if (USART1_RX_STA & 0X8000) //���յ�ƽ������
	{
		Usart1JudgeStr();
		USART1_RX_STA		= 0;					//���״̬
	}

	//	  if(USART3_RX_STA & 0X8000)//���յ�ƽ������
	//	  {
	//		  Usart3JudgeStr();
	//		  USART3_RX_STA   =   0; //���״̬
	//	  }
}


/*******************************************************************************
* ����: 
* ����: ��������ȫ��״̬��ƽ��  ���
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SendToTablet_Zero(u8 u8GlassAddr)
{
	//	  ��ͷ + ��ַ + ���� + ���ݰ�[48] + У��� + ��β
	//	  ��ͷ��0XAA
	//	  �ֱ��ַ��0X8~�����λΪ1��ʾȫ��						~:��ʾ�м����ֱ�
	//				0x01����һ���ֱ�
	//	  ���
	//	  ���ݣ��������� + ���� + ʱ�䣩* 16 = 48
	//	  У��ͣ�
	//	  ��β�� 0X55
	u8 i;
	u8 u8aSendArr[53]	=
	{
		0
	};
	SendArrayUnion_u SendArrayUnion;

	u8aSendArr[1]		= MCU_ACK_ADDR; 			//Ӧ���ַ

	u8aSendArr[2]		= ZERO_ACK_TYPE;			//Ӧ���������
	u8aSendArr[3]		= u8GlassAddr;				//�ֱ��ַ

	u8aSendArr[0]		= 0xAA;

	for (i = 0; i < 47; i++)
	{
		u8aSendArr[4 + i]	= SendArrayUnion.u8SendArray[i];
	}

	u8aSendArr[51]		= CheckSum(u8aSendArr, 51); //У���
	u8aSendArr[52]		= 0x55; 					//��βs 

	for (i = 0; i < 53; i++)
	{
		Usart1_SendByte(u8aSendArr[i]);
	}
}


/*******************************************************************************
* ����: 
* ����: ��������ȫ��״̬��ƽ��
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SendToTablet(u8 u8GlassAddr)
{
	//	  ��ͷ + ��ַ + ���� + ���ݰ�[48] + У��� + ��β
	//	  ��ͷ��0XAA
	//	  �ֱ��ַ��0X8~�����λΪ1��ʾȫ��						~:��ʾ�м����ֱ�
	//				0x01����һ���ֱ�
	//	  ���
	//	  ���ݣ��������� + ���� + ʱ�䣩* 16 = 48
	//	  У��ͣ�
	//	  ��β�� 0X55
	u8 i;
	u8 u8aSendArr[53]	=
	{
		0
	};
	SendArrayUnion_u SendArrayUnion;

	if (u8GlassAddr == MCU_ACK_ADDR) //Ӧ���ַ
	{
		u8aSendArr[2]		= g_AckType;			//Ӧ������
		u8aSendArr[3]		= g_FingerAck;			//��������	Ӧ���������
	}
	else 
	{
		memcpy(SendArrayUnion.WatchState, SysTask.WatchState, sizeof(SysTask.WatchState));

		u8aSendArr[2]		= 0x11; 				//�������ݱ����� 

		for (i = 0; i < 48; i++)
		{
			u8aSendArr[3 + i]	= SendArrayUnion.u8SendArray[i];
		}
	}

	u8aSendArr[0]		= 0xAA;
	u8aSendArr[1]		= u8GlassAddr;
	u8aSendArr[51]		= CheckSum(u8aSendArr, 51); //У���
	u8aSendArr[52]		= 0x55; 					//��βs 

	for (i = 0; i < 53; i++)
	{
		Usart1_SendByte(u8aSendArr[i]);
	}
}


/*******************************************************************************
* ����: 
* ����: �������ʹ�����״̬��ƽ��
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SendToTabletDetect(u8 u8GlassAddr)
{
	//	  ��ͷ + ��ַ + ���� + ���ݰ�[48] + У��� + ��β
	//	  ��ͷ��0XAA
	//	  �ֱ��ַ��0X8~�����λΪ1��ʾȫ��						~:��ʾ�м����ֱ�
	//				0x01����һ���ֱ�
	//	  ���
	//	  ���ݣ��������� + ���� + ʱ�䣩* 16 = 48
	//	  У��ͣ�
	//	  ��β�� 0X55
	u8 i;
	u8 u8aSendArr[53]	=
	{
		0
	};

	u8aSendArr[0]		= 0xAA;
	u8aSendArr[2]		= g_AckType;				//Ӧ������
	u8aSendArr[3]		= g_FingerAck;				//��������	Ӧ���������
	u8aSendArr[1]		= u8GlassAddr;
	u8aSendArr[51]		= CheckSum(u8aSendArr, 51); //У���
	u8aSendArr[52]		= 0x55; 					//��βs 

	for (i = 0; i < 53; i++)
	{
		Usart1_SendByte(u8aSendArr[i]);
	}
}


/*******************************************************************************
* ����: 
* ����: ���͸�ƽ�������
* �β�:		
* ����: ��
* ˵��: 
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
				SendToTablet(GLASS_SEND_ALL | GLASS_MAX_CNT); //�����ֱ����
				SysTask.SendState	= SEND_IDLE;
			}

			break;

		case SEND_TABLET_FINGER: //����ģ��Ӧ���ƽ��
			SendToTablet(MCU_ACK_ADDR); //
			SysTask.SendState = SEND_IDLE;
			break;

		case SEND_TABLET_DETECT: //���ʹ�����״̬��ƽ��
			SendToTablet(MCU_ACK_ADDR); //
			SysTask.SendState = SEND_IDLE;
			break;

		case SEND_TABLET_ZERO: //����ģ������ƽ��,����˾Ͳ����ˣ�ƽ����г�ʱ����
			SendToTablet_Zero(g_GlassSendAddr); //	�����Ļ�ֻ�ܷ������һ�����ˣ�����������߳�ͻ�������˵��������ٴε��������200ms���ϣ����Ե�������˵��ͻ�Ļ��ʻ��Ǻ�С��
			SysTask.SendState = SEND_IDLE;
			break;

		/* ���ڽ׶�û��ʹ���ⲿ�ֵ����ݣ���Ϊ����Ӧ����飬���������ⲿ����ʱ�ò�?
			?
		case SEND_TABLET_SINGLE://���͵����ӻ�״̬��ƽ��
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
* ����: 
* ����: �����ӷ��͸��ӻ����͵����������
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void HostToTabletTask(void)
{
    
}

/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Moto_01_Task(void)
{
	static u8 u8State	= 0;

	//_03���
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
			Moto_1_Start_CCW(MOTO_PUTY_99); //ȫ�ٻ�ȥ ����
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
			{ //����ת
				u8State 			= 0;
				Moto_1_Start_CCW(MOTO_PUTY);
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_CHA;
			}

			SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];
			break;

		case MOTO_STATE_RUN_NOR: //��ͨ����״̬
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01])
			{
				Moto_1_Stop();						//����ͣ���ת��
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				SysTask.Moto_StateTime[GLASS_01] = 200;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_01] == 0)
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //��⵽���	
						{
							SysTask.Moto_WaitTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][2];
							SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_WAIT;
							Moto_1_Stop();			//����ͣ���ת��
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

		case MOTO_STATE_RUN_CHA: //����ת����״̬
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01])
			{
				Moto_1_Stop();						//����ͣ���ת��
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_01] == 0) //TPDʱ�䵽
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //��⵽���	
						{
							SysTask.Moto_WaitTime[GLASS_01] = g_au16MoteTime[SysTask.Moto_Time[GLASS_01]][2];
							SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_WAIT;
							Moto_1_Stop();			//����ͣ���ת��
						}
					}
					else if (SysTask.Moto_StateTime[GLASS_01] == 0)
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //��⵽���	�л�����ת
						{

							SysTask.Moto_StateTime[GLASS_01] = SysTask.nTick + MOTO_TRUN_OVER + 500; //��⵽0�㣬һ��ʱ����ٴμ�⣬�ܿ�

							SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DELAY;
							Moto_1_Stop();			//����ͣ���ת��
							SysTask.Moto3_SubStateTime = MOTO_TRUN_OVER;
						}

					}

					break;

				case MOTO_SUB_STATE_DELAY:
					if (SysTask.Moto3_SubStateTime != 0) //��ʱһ��ʱ���ٷ�ת
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
				Moto_1_Stop();						//����ͣ���ת��

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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Moto_02_Task(void)
{
	static u8 u8State	= 0;

	//_03���
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
			Moto_2_Start_CCW(MOTO_PUTY_99); //ȫ�ٻ�ȥ ����
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
			{ //����ת
				u8State 			= 0;
				Moto_2_Start_CCW(MOTO_PUTY_B);
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_CHA;
			}

			SysTask.Moto_ModeSave[GLASS_02] = SysTask.Moto_Mode[GLASS_02];
			break;

		case MOTO_STATE_RUN_NOR: //��ͨ����״̬
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02])
			{
				Moto_2_Stop();						//����ͣ���ת��
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				SysTask.Moto_StateTime[GLASS_02] = 200;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_02])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_02] == 0)
					{

						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //��⵽���	
						{
							SysTask.Moto_WaitTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][2];
							SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_WAIT;
							Moto_2_Stop();			//����ͣ���ת��
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

		case MOTO_STATE_RUN_CHA: //����ת����״̬
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02])
			{
				Moto_2_Stop();						//����ͣ���ת��
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_02])
			{
				case MOTO_SUB_STATE_RUN:
					if (SysTask.Moto_RunTime[GLASS_02] == 0)
					{

						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //��⵽���	
						{
							SysTask.Moto_WaitTime[GLASS_02] = g_au16MoteTime[SysTask.Moto_Time[GLASS_02]][2];
							SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_WAIT;
							Moto_2_Stop();			//����ͣ���ת��
						}
					}
					else if (SysTask.Moto_StateTime[GLASS_02] == 0)
					{
						if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //��⵽���	
						{
							SysTask.Moto_StateTime[GLASS_02] = SysTask.nTick + MOTO_TRUN_OVER + 500; //��⵽0�㣬һ��ʱ����ٴμ�⣬�ܿ�

							SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_DELAY;
							Moto_2_Stop();			//����ͣ���ת��
							SysTask.Moto3_SubStateTime = MOTO_TRUN_OVER;
						}

					}

					break;

				case MOTO_SUB_STATE_DELAY:
					if (SysTask.Moto3_SubStateTime != 0) //��ʱһ��ʱ���ٷ�ת
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
				Moto_2_Stop();						//����ͣ���ת��

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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Lock_01_Task(void)
{
	static u8 u8ClockCnt = 0;
	static u8 u8ClockPer = 0;

	switch (SysTask.Lock_State[GLASS_01]) //_03��
	{
		case LOCK_STATE_DETECT:
			if (SysTask.LockMode[GLASS_01] != SysTask.LockModeSave[GLASS_01])
			{

				if (SysTask.LockMode[GLASS_01] == LOCK_ON)
				{
				    SysTask.Moto_State[GLASS_01] = MOTO_STATE_OPEN_LOCK; //  �������״̬
					if (SysTask.Moto_WaitTime[GLASS_01] != 0) //��������ֹͣ״̬
					{
						SysTask.Moto_WaitTime[GLASS_01] = 0;
					}

					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //��⵽��� ����
					{
			            Moto_1_Start_CW(MOTO_PUTY);   //������һ�㵽���
						SysTask.Lock_StateTime[GLASS_01] = MOTO_TIME_OUT; //
						SysTask.Moto_WaitTime[GLASS_01] = MOTO_DEBOUNCE;
						SysTask.Lock_State[GLASS_01] = LOCK_STATE_DEBOUNSE;
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_STOP; //ֹͣת��
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
				if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_01_PIN) == 0) // //��⵽��� ����
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
				u3_printf(" ��ʱǿ���˳�����������\n");
#endif

				SysTask.LockModeSave[GLASS_01] = LOCK_OFF;
				SysTask.LockMode[GLASS_01] = LOCK_OFF;
				SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;
			}

			break;

		case LOCK_STATE_SENT: //����3����������浥Ƭ����ִ�й���
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
				SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_OFF_TIME; //ֹͣ��ƽ

			}

			break;

		case LOCK_STATE_OFF: //
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN);
			SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_L_TIME;
			SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;

			//	�����ת������������ת������
			SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //���¿�ʼת��
			break;

		default:
			break;
	}
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Lock_02_Task(void)
{
	static u8 u8ClockCnt = 0;
	static u8 u8ClockPer = 0;

	switch (SysTask.Lock_State[GLASS_02]) //_03��
	{
		case LOCK_STATE_DETECT:
			if (SysTask.LockMode[GLASS_02] != SysTask.LockModeSave[GLASS_02])
			{

				if (SysTask.LockMode[GLASS_02] == LOCK_ON)
				{
				    SysTask.Moto_State[GLASS_02] = MOTO_STATE_OPEN_LOCK; //  �������״̬
					if (SysTask.Moto_WaitTime[GLASS_02] != 0) //��������ֹͣ״̬
					{
						SysTask.Moto_WaitTime[GLASS_02] = 0;
					}

					if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //��⵽��� ����
					{
			            Moto_2_Start_CW(MOTO_PUTY);   //������һ�㵽���
						SysTask.Lock_StateTime[GLASS_02] = MOTO_TIME_OUT; //
						SysTask.Moto_WaitTime[GLASS_02] = MOTO_DEBOUNCE;
						SysTask.Lock_State[GLASS_02] = LOCK_STATE_DEBOUNSE;
						SysTask.Moto_State[GLASS_02] = MOTO_STATE_STOP; //ֹͣת��
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
				if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_02_PIN) == 0) // //��⵽��� ����
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
				u3_printf(" ��ʱǿ���˳�����������\n");
#endif

				SysTask.LockModeSave[GLASS_02] = LOCK_OFF;
				SysTask.LockMode[GLASS_02] = LOCK_OFF;
				SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;
			}

			break;

		case LOCK_STATE_SENT: //����3����������浥Ƭ����ִ�п���
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
				SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_OFF_TIME; //ֹͣ��ƽ

			}

			break;

		case LOCK_STATE_OFF: //
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;

			GPIO_ResetBits(LOCK_GPIO, LOCK_02_PIN);
			SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_L_TIME;
			SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;

			//	�����ת������������ת������
			SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //���¿�ʼת��
			break;

		default:
			break;
	}
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
				RangeSensor_Event(0XA4, &u16RangeVal); //����������������ַ�ֱ�Ϊ 0xa4 0xa6 0xa8
			}
			else if (u8RangeCnt == 1)
			{
				RangeSensor_Event(0XA6, &u16RangeVal);
			}
			else 
			{
				RangeSensor_Event(0XA8, &u16RangeVal);
			}

			if ((u16RangeVal <= RANGE_DISTANCE) && (u16RangeVal >= 200))  //��һ�ν���Ҫ200��ʵ��Ϊ100ʱһ�����ϴ����180���󴥷�
			{
#if DEBUG
				u3_printf(" ���˽��� ��������%d --> %d mm\n", u8RangeCnt, u16RangeVal);
#endif
				SysTask.Range_StateTime = 100;		//20ms	
				SysTask.Range_State = RANGE_STATE_DEBOUNCE;
				SysTask.Range_WaitTime = RANGE_LEAFT_TIME;
                break;
			}
			else 
			{
				if (u8RangeCnt == 2)
				{ //300 ms һ�βɼ�
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
            
        case RANGE_STATE_DEBOUNCE: //�ٴμ�� ֮ǰ��Ӧ���˵��Ǹ���������������� ˵�� �Ѿ����˽���
            if (SysTask.Range_StateTime)
				break;

			if (u8RangeCnt == 0)
			{
				RangeSensor_Event(0XA4, &u16RangeVal); //����������������ַ�ֱ�Ϊ 0xa4 0xa6 0xa8
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
				u3_printf(" ������˽��� ��������%d --> %d mm\n", u8RangeCnt, u16RangeVal);
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
				RangeSensor_Event(0XA4, &u16RangeVal); //����������������ַ�ֱ�Ϊ 0xa4 0xa6 0xa8
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
				u3_printf(" ��������%d --> %d mm\n", u8RangeCnt, u16RangeVal);
#endif

				u8RangeCnt			= 0;
				SysTask.Range_StateTime = 100;		//20ms	
				SysTask.Range_WaitTime = RANGE_LEAFT_TIME;
			}
			else 
			{
				if (SysTask.Range_WaitTime == 0)
				{ //����

#if DEBUG
					u3_printf("���뿪����");
#endif
                    
                    SysTask.LED_Mode = LED_MODE_OFF;

					SysTask.Range_State = RANGE_STATE_INIT;
					u8RangeCnt			= 0;
					break;
				}

				if (u8RangeCnt == 2)
				{ //300 ms һ�βɼ�
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
* flexible: flexible value. adjust the S curves, ��ֵԽ��������ʼԽ��
*/
static void Sys_CalculateSModelLine(float arr[],  float len, float arr_max, float arr_min, float flexible)
{
	int 			i	= 0;
	float			deno;
	float			melo;
	float			delt = arr_max - arr_min;
        //ԭʼS�ͼ��٣��ĳ�ָ������
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
                
                SysTask.LED_StateTime = LIGHT_ADD_SPEED;  //led ���ȸ����ٶ� 

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
                
                SysTask.LED_StateTime = LIGHT_DEC_SPEED;  //led ���ȸ����ٶ� 

                if(u16Brighness > 0){
                    u16Brighness--;
                    if(u16Brighness < 30)
                        SysTask.LED_StateTime = LIGHT_DEC_SPEED_20;  //led ���ȸ����ٶ� 
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
			memset(SysTask.FlashData.u8FlashData, 0, sizeof(SysTask.FlashData.u8FlashData)); //��0

			//			  memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
			for (i = 0; i < GLASS_MAX_CNT; i++)
			{
				SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
				SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
				SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
			}

			SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LEDģʽ
			u8Status = SAVE_WRITE;
			break;

		case SAVE_WRITE:
			if (FLASH_WriteMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, DATA_READ_CNT + LED_MODE_CNT + PASSWD_CNT))
			{
				u8count++;
				u8Status			= SAVE_WAIT;
				SysTask.u16SaveWaitTick = 100;		// 100ms ��ʱ������д��
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

				if (u8count++ >= 7) //ÿҳ����8��
				{
					if (++SysTask.u16AddrOffset < ADDR_MAX_OFFSET) //��5ҳ
					{
						for (i = 0; i < 3; i++) //��д8�����
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void MainTask(void)
{
	SysSaveDataTask();
	LedTask();
	SendTabletTask();								//���͸�ƽ������
	TabletToHostTask(); 							//����ƽ������������
	Moto_01_Task(); 								//ֻ������ת�ģ�ת��Ȧ
	Moto_02_Task(); 								//�м�� ������ �½�ʱ�и������ ������ʹ��ʱ�����ж�

	Lock_01_Task();
	Lock_02_Task();
	RangeEventTask();
}

/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SysEventInit(void)
{
	g_RangeEvent.bInit	= TRUE;
	g_RangeEvent.bMutex = FALSE;
	g_RangeEvent.MachineCtrl.pData = (void *)
	malloc(sizeof(RangeData_t));
	g_RangeEvent.MachineCtrl.CallBack = RangeSensor_Event;
	g_RangeEvent.MachineCtrl.u16InterValTime = 5;	//5ms ����һ��

	g_RangeEvent.MachineRun.bEnable = TRUE;
	g_RangeEvent.MachineRun.u32StartTime = 0;
	g_RangeEvent.MachineRun.u32LastTime = 0;
	g_RangeEvent.MachineRun.u32RunCnt = 0;
	g_RangeEvent.MachineRun.bFinish = FALSE;
	g_RangeEvent.MachineRun.u16RetVal = 0;
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:	
* ����: ��
* ˵��: 
*******************************************************************************/
void SysInit(void)
{
	u8 i, j;

	Moto_1_Stop();
	Moto_2_Stop();
    
    Sys_CalculateSModelLine(g_LedArr,LED_BRIGHNESS * 2, LED_BRIGHNESS * 2, 0, 5); //���һ������
    g_LedArr[LED_BRIGHNESS -1] = LED_BRIGHNESS;
    

	SysTask.u16AddrOffset = FLASH_ReadHalfWord(g_au32DataSaveAddr[0]); //��ȡ
	memset(SysTask.FlashData.u8FlashData, 0, sizeof(SysTask.FlashData.u8FlashData));

	if ((SysTask.u16AddrOffset == 0xffff) || (SysTask.u16AddrOffset >= ADDR_MAX_OFFSET) ||
		 (SysTask.u16AddrOffset == 0)) //��ʼ״̬ flash��0xffff
	{
		SysTask.u16AddrOffset = 1;					//�ӵ�һҳ��ʼд��

		for (i = 0; i < 8; i++) //��д8�����
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
				SysTask.WatchState[i].u8Dir = MOTO_FR_FWD_REV; //ֻ������ת��ֹͣģʽ������ѡ��
			}
		}

		//		  memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
		for (i = 0; i < GLASS_MAX_CNT; i++)
		{
			SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
			SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
			SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
		}

		SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LEDģʽ

		FLASH_WriteMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, sizeof(SysTask.FlashData.u16FlashData) / 2);
	}

	FLASH_ReadMoreData(g_au32DataSaveAddr[SysTask.u16AddrOffset], SysTask.FlashData.u16FlashData, sizeof(SysTask.FlashData.u16FlashData) / 2);


	for (i = 0; i < DATA_READ_CNT; i++)
	{
		if (SysTask.FlashData.u16FlashData[i] == 0xFFFF) //������FLASH�д�˵��ҳ�������⣬�ȴ�����ʱ�ٽ�������У��д����һҳ
		{
			for (j = 0; j < GLASS_MAX_CNT; j++)
			{
				SysTask.WatchState[j].u8LockState = 0;
				SysTask.WatchState[j].u8Dir = MOTO_FR_FWD;
				SysTask.WatchState[j].MotoTime = MOTO_TIME_650;

				if (i == 0)
				{
					SysTask.WatchState[i].u8Dir = MOTO_FR_FWD_REV; //ֻ������ת��ֹͣģʽ������ѡ��
				}
			}

			//memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
			for (i = 0; i < GLASS_MAX_CNT; i++)
			{
				SysTask.FlashData.Data.WatchState[i].MotoTime = SysTask.WatchState[i].MotoTime;
				SysTask.FlashData.Data.WatchState[i].u8Dir = SysTask.WatchState[i].u8Dir;
				SysTask.FlashData.Data.WatchState[i].u8LockState = SysTask.WatchState[i].u8LockState;
			}

			SysTask.FlashData.Data.u16LedMode = LED_MODE_ON; //LEDģʽ

			break;
		}
	}

	//	  ʹ��memcpy�ᵼ��TIM 2 TICk�޷������жϣ����������ж�û���ܵ�Ӱ�죬ԭ��δ֪������8����û������
	//	  ����9�����Ͼͻ���֡�
	//	  memcpy(SysTask.WatchState, SysTask.FlashData.u8FlashData, 10);
	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.WatchState[i].MotoTime = SysTask.FlashData.Data.WatchState[i].MotoTime;
		SysTask.WatchState[i].u8Dir = SysTask.FlashData.Data.WatchState[i].u8Dir;

		//		  SysTask.WatchState[i].u8LockState   = SysTask.FlashData.Data.WatchState[i].u8LockState;  //��״̬���ڲ����ϴ������±ߵ�Ƭ����֤
	}

	SysTask.LED_Mode	= SysTask.FlashData.Data.u16LedMode;

	if ((SysTask.LED_Mode != LED_MODE_ON) && (SysTask.LED_Mode != LED_MODE_OFF))
	{
		SysTask.LED_Mode	= LED_MODE_ON;			//LEDģʽ
	}

	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.Moto_Mode[i] = (MotoFR) (SysTask.WatchState[i].u8Dir);
		SysTask.Moto_Time[i] = (MotoTime_e) (SysTask.WatchState[i].MotoTime);
		SysTask.Moto_RunTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][1];
		SysTask.Moto_WaitTime[i] = g_au16MoteTime[SysTask.Moto_Time[i]][2];
		SysTask.LockMode[i] = LOCK_OFF; 			//��״̬���ڴ˸��£���Ϊ������Ҫ����֮��
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
		SysTask.Moto_State[i] = MOTO_STATE_INIT;	//��֤����������ת
		SysTask.Moto_SubState[i] = MOTO_SUB_STATE_RUN;
		SysTask.Lock_StateTime[i] = 0;				//		  
	}

	//	  SysTask.u16BootTime = 3000;	//3��ȴ��ӻ�������ȫ
}


