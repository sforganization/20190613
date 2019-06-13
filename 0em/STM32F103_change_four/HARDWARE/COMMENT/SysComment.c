
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


#define SLAVE_WAIT_TIME 		3000	//3s �ӻ�Ӧ��ʱ��

#define LOCK_SET_H_TIME 		1	  //�ߵ�ƽʱ�� 1ms
#define LOCK_SET_OPEN_TIME		3	  //�����͵�ƽʱ��
#define LOCK_SET_CLOSE_TIME 	10	  //�����͵�ƽʱ��


#define LOCK_SET_OFF_TIME		300	  //����Ƭ����Ӧʱ�䣬�������֮����ת


#define MOTO_UP_TIME			800			//�������ʱ��
#define MOTO_DOWN_TIME			1500		//����½�ʱ��




#define MOTO_TRUN_OVER			500			//MOTO 3  �����ƽ��תʱ��


#define MOTO_ANGLE_TIME 		(750 * PWM_PERIOD / PWM_POSITIVE)  //���ת��һ���Ƕȵ�ʱ��
#define MOTO_DEVIATION_TIME 	700  //���ת��ʱ��ƫ�� 200ms ����ƫ��������дflashʱ����Ϊдflash��ֹһ�� flash����������Ķ�ȡ���������жϣ�

//ע�⣬���ڴ��ڽ��ջ�ռ��һ����ʱ�䣬�������ʱҪ��Ӧ�ӳ����ر��ǵ���׿����ֹͣ���������ʱ��������ڼ�����ת����תʱ�򷢣�����Ѿ�������һ�Σ���������Ҫ��Ӧ�ӳ�ʱ��
//��Ϊ
#define LOCK_OPEN_TIME			5			//�������͵�ƽʱ��


//static bool  g_bSensorState = TRUE;
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

u8				g_GlassSendAddr = 0; //Ҫ���͵��ֱ��ַ
u8				g_FingerAck = 0; //Ӧ����������
u8				g_AckType = 0; //Ӧ������


//�ڲ�����
void SysTickConfig(void);


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


//	  /*******************************************************************************
//	  * ����: 
//	  * ����: 
//	  * �β�:		
//	  * ����: ��
//	  * ˵��: 
//	  *******************************************************************************/
//	  void FingerTouchTask(void)
//	  {
//		SearchResult seach;
//		u8 ensure;
//		static u8 u8MatchCnt = 0;						//ƥ��ʧ�ܴ�����Ĭ��ƥ��MATCH_FINGER_CNT�� 
//		if ((SysTask.bEnFingerTouch == TRUE))
//		{
//			switch (SysTask.TouchState)
//			{
//				case TOUCH_MATCH_INIT:
//					if (PS_Sta) //��ָ�ư���
//					{
//						SysTask.nWaitTime	= 10;		//һ��ʱ���ټ��
//						SysTask.TouchState	= TOUCH_MATCH_AGAIN;
//						u8MatchCnt			= 0;
//					}
//					break;
//				case TOUCH_MATCH_AGAIN:
//					if (SysTask.nWaitTime == 0) //��ָ�ư���
//					{
//						if (PS_Sta) //��ָ�ư���
//						{
//							ensure				= PS_GetImage();
//							if (ensure == 0x00) //���������ɹ�
//							{
//								//PS_ValidTempleteNum(&u16FingerID); //����ָ�Ƹ���
//								ensure				= PS_GenChar(CharBuffer1);
//								if (ensure == 0x00) //���������ɹ�
//								{
//									ensure				= PS_Search(CharBuffer1, 0, FINGER_MAX_CNT, &seach);
//									if (ensure == 0) //ƥ��ɹ�
//									{
//										SysTask.u16FingerID = seach.pageID;
//										SysTask.nWaitTime	= SysTask.nTick + 2000; //��ʱһ��ʱ�� �ɹ�Ҳ����һֱ��
//										SysTask.TouchSub	= TOUCH_SUB_INIT;
//										SysTask.TouchState	= TOUCH_MATCH_INIT; //����ƥ��
//										//								  SysTask.bEnFingerTouch = FALSE;	 //�ɹ������ָֹ��ģ������
//										//ƥ��ɹ������͵�ƽ��ˣ�Ӧ��
//										g_FingerAck 		= ACK_SUCCESS;
//										g_AckType			= FINGER_ACK_TYPE;
//										if (SysTask.SendState == SEND_IDLE)
//											SysTask.SendState = SEND_TABLET_FINGER;
//									}
//									else if (u8MatchCnt >= MATCH_FINGER_CNT) // 50 * 3 ��Լ200MS��
//									{
//										//								  SysTask.bEnFingerTouch = FALSE;	 //�ɹ������ָֹ��ģ������
//										//ƥ��ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
//										g_FingerAck 		= ACK_FAILED;
//										g_AckType			= FINGER_ACK_TYPE;
//										if (SysTask.SendState == SEND_IDLE)
//											SysTask.SendState = SEND_TABLET_FINGER;
//										SysTask.TouchState	= TOUCH_MATCH_INIT; //����ƥ��
//										SysTask.nWaitTime	= SysTask.nTick + 300; //��ʱһ��ʱ�� ����ֳ�ʶ���ˣ�������һ���˼�⣬
//									}
//								}
//							}
//							if (ensure) //ƥ��ʧ��
//							{
//								SysTask.nWaitTime	= SysTask.nTick + 10; //��ʱһ��ʱ�� �����⵽��Ӧ����ͬһ���˵�ָ��
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
//								if ((PS_Sta) && (SysTask.nWaitTime == 0)) //�����ָ�ư���
//								{
//									ensure				= PS_GetImage();
//									if (ensure == 0x00)
//									{
//										ensure				= PS_GenChar(CharBuffer1); //��������
//										if (ensure == 0x00)
//										{
//											SysTask.nWaitTime	= 3000; //��ʱһ��ʱ����ȥ�ɼ�
//											SysTask.TouchSub	= TOUCH_SUB_AGAIN; //�����ڶ���				
//											//Please press	agin
//											//�ɹ������͵�ƽ��ˣ�Ӧ������ٰ�һ��
//											g_FingerAck 		= ACK_AGAIN;
//											g_AckType			= FINGER_ACK_TYPE;
//											if (SysTask.SendState == SEND_IDLE)
//												SysTask.SendState = SEND_TABLET_FINGER;
//										}
//									}
//								}
//								break;
//							case TOUCH_SUB_AGAIN:
//								if ((PS_Sta) && (SysTask.nWaitTime == 0)) //�����ָ�ư���
//								{
//									ensure				= PS_GetImage();
//									if (ensure == 0x00)
//									{
//										ensure				= PS_GenChar(CharBuffer2); //��������
//										if (ensure == 0x00)
//										{
//											ensure				= PS_Match(); //�Ա�����ָ��
//											if (ensure == 0x00) //�ɹ�
//											{
//												ensure				= PS_RegModel(); //����ָ��ģ��
//												if (ensure == 0x00)
//												{
//													ensure				= PS_StoreChar(CharBuffer2, SysTask.u16FingerID); //����ģ��
//													if (ensure == 0x00)
//													{
//														SysTask.TouchSub	= TOUCH_SUB_WAIT;
//														SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
//														//													  SysTask.bEnFingerTouch = FALSE;	 //�ɹ������ָֹ��ģ������
//														//�ɹ������͵�ƽ��ˣ�Ӧ��
//														g_FingerAck 		= ACK_SUCCESS;
//														g_AckType			= FINGER_ACK_TYPE;
//														if (SysTask.SendState == SEND_IDLE)
//															SysTask.SendState = SEND_TABLET_FINGER;
//													}
//													else 
//													{
//														//ƥ��ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
//														SysTask.TouchSub	= TOUCH_SUB_ENTER;
//														SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
//													}
//												}
//												else 
//												{
//													//ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
//													g_FingerAck 		= ACK_FAILED;
//													g_AckType			= FINGER_ACK_TYPE;
//													if (SysTask.SendState == SEND_IDLE)
//														SysTask.SendState = SEND_TABLET_FINGER;
//													SysTask.TouchSub	= TOUCH_SUB_ENTER;
//													SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
//												}
//											}
//											else 
//											{
//												//ʧ�ܣ����͵�ƽ��ˣ�Ӧ��
//												g_FingerAck 		= ACK_FAILED;
//												g_AckType			= FINGER_ACK_TYPE;
//												if (SysTask.SendState == SEND_IDLE)
//													SysTask.SendState = SEND_TABLET_FINGER;
//												SysTask.TouchSub	= TOUCH_SUB_ENTER;
//												SysTask.nWaitTime	= 3000; //��ʱһ��ʱ���˳�
//											}
//										}
//									}
//								}
//								break;
//							case TOUCH_SUB_WAIT: // *
//								if ((SysTask.nWaitTime == 0))
//								{
//									SysTask.TouchState	= TOUCH_IDLE;
//									SysTask.bEnFingerTouch = FALSE; //�ɹ������ָֹ��ģ������
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
//									//���͵�ƽ�����
//									g_FingerAck 		= ACK_SUCCESS;
//									g_AckType			= FINGER_ACK_TYPE;
//									if (SysTask.SendState == SEND_IDLE)
//										SysTask.SendState = SEND_TABLET_FINGER;
//									SysTask.TouchSub	= TOUCH_SUB_WAIT;
//									SysTask.nWaitTime	= 100; //��ʱһ��ʱ���˳�
//								}
//								else //ɾ��ʧ�� ,
//								{
//									u8MatchCnt++;
//									SysTask.nWaitTime	= 500; //��ʱһ��ʱ���ٴ�ɾ��
//									if (u8MatchCnt >= 3) //ʧ���˳�
//									{
//										//���͵�ƽ�����
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
//									SysTask.bEnFingerTouch = FALSE; //�ɹ������ָֹ��ģ������
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
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
	//	  ��� CMD_UPDATE	 �� CMD_READY	CMD_CHANGE
	//	  ������ ��ַ��LEDģʽ������ɾ��ָ��ID, CMD_CHANGE�ĵڼ�����
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

		case CMD_CHANGE: // ����������Ϊ�ڼ�����
			if (* (data + 2) <= GLASS_NUM_FOUR) //������ʾ�ڼ�����
			{
				SysTask.bGlassChange = TRUE;
    
                SysTask.WatchDisplay = (Display_T)(*(data + 2));    //��ǰչʾ���ǵڼ�����
                
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
//                        SysTask.u16StepRequire = g_u16StepCnt[* (data + 2)] -g_u16StepCnt[SysTask.GlassNum] + (* (data + 2) - SysTask.GlassNum) * 60;  //ÿ��1600��Ҫ�� 10�����
//                    } 
//                    else
//                    {
//                        SysTask.u16StepRequire =  CIRCLE_STEP_PULSE - g_u16StepCnt[SysTask.GlassNum] + g_u16StepCnt[* (data + 2)] + 4 - ( SysTask.GlassNum ) * 60;  //ÿ��1600��Ҫ�� 10�����,��6400����������4

//                    }
				}

				SysTask.GlassNum	= (GlassNum_T) (* (data + 2));

                for (i = 0; i < GLASS_MAX_CNT; i++)
            	{
            		SysTask.LockModeSave[i] = LOCK_ON;
            		SysTask.LockMode[i] = LOCK_OFF; 			//����ǰ�ȹ���
            		
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
				//���͵�ƽ��ˣ�Ӧ��	 �������
				g_FingerAck 		= ACK_ERROR;
				g_AckType			= ERROR_ACK_TYPE;

				if (SysTask.SendState == SEND_IDLE)
					SysTask.SendState = SEND_TABLET_FINGER;

			}

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
* ����: 
* ����: ����ţ�����ֻ�п����š��������½����¼������������д�������������ʱ���
	�� ��û�д����� 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Moto_01_Task(void)
{
	//�������
	switch (SysTask.Moto_State[GLASS_01])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_01] != 0)
				break;

			SysTask.Moto_State[GLASS_01] = MOTO_STATE_RUN_NOR;

			if (SysTask.Moto_Mode[GLASS_01] == MOTO_FR_FWD) //��ת������
			{
#if DEBUG
				u3_printf(" Moto_01_Task ��ת������\n");
#endif

				SysTask.Moto_RunTime[GLASS_01] = OPEN_DOOR_TIME; //
				SET_DOOR_L_DIR_P();
				Door_L_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_UP;
			}
			else 
			{
#if DEBUG
				u3_printf(" Moto_01_Task ����\n");
#endif

				SET_DOOR_L_DIR_N();
				Door_L_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DOWN; //����
				SysTask.Moto_RunTime[GLASS_01] = 500; //ǿ��ʱ��,ʱ�䵽�͵���PWMռ�ձȼ�����ȥ
			}

			SysTask.Moto_ModeSave[GLASS_01] = SysTask.Moto_Mode[GLASS_01];
			break;

		case MOTO_STATE_RUN_NOR: //��ͨ����״̬
			if (SysTask.Moto_Mode[GLASS_01] != SysTask.Moto_ModeSave[GLASS_01]) //��;ģʽ�ı�
			{
				Door_L_MotoStop();					//����ͣ���ת��  --
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				SysTask.Moto_StateTime[GLASS_01] = 10;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_01])
			{
				case MOTO_SUB_STATE_UP: //����
					if (SysTask.Moto_RunTime[GLASS_01] == 0) //��ʱ�˳�
					{
#if DEBUG
						u3_printf(" Moto_01_Task ��ͣ���\n");
#endif

						Door_L_MotoStop();			//����ͣ���ת��  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}

					break;

				case MOTO_SUB_STATE_DOWN: //���� 
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_L) == 1))
					{
#if DEBUG
						u3_printf(" Moto_01_Task �����ͣ���\n");
#endif

						Door_L_MotoStop();			//����ͣ���ת��  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //��ʱ
					{
#if DEBUG
						u3_printf(" Moto_01_Task ����\n");
#endif

						SET_DOOR_L_DIR_N();
						Door_L_MotoStart(MOTO_PUTY_10);

						SysTask.Moto_RunTime[GLASS_01] = 600;
						SysTask.Moto_SubState[GLASS_01] = MOTO_SUB_STATE_DEC;
					}

					break;

				case MOTO_SUB_STATE_DEC: //����
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_L) == 1))
					{
#if DEBUG
						u3_printf(" Moto_01_Task ����--�����ͣ���\n");
#endif

						Door_L_MotoStop();			//����ͣ���ת��  --
						SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_01] == 0) //��ʱ
					{
#if DEBUG
						u3_printf(" Moto_01_Task ��ʱ��ͣ���\n");
#endif

						SysTask.Moto_RunTime[GLASS_01] = 100;
						Door_L_MotoStop();			//����ͣ���ת��  --
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
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
			}

			break;

		default:
			SysTask.Moto_State[GLASS_01] = MOTO_STATE_IDLE; //
			break;
	}
}


/*******************************************************************************
* ����: 
* ����: ֻ������ת�ģ�������.�������½����¼�����
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Moto_02_Task(void)
{
	//�������
	switch (SysTask.Moto_State[GLASS_02])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_02] != 0)
				break;

			SysTask.Moto_State[GLASS_02] = MOTO_STATE_RUN_NOR;

			if (SysTask.Moto_Mode[GLASS_02] == MOTO_FR_FWD) //��ת������
			{
#if DEBUG
				u3_printf(" Moto_02_Task ��ת������\n");
#endif

				SysTask.Moto_RunTime[GLASS_02] = OPEN_DOOR_TIME; //
				SET_DOOR_R_DIR_P();
				Door_R_MotoStart(MOTO_PUTY_99);

				SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_UP;
			}
			else 
			{
#if DEBUG
				u3_printf(" Moto_02_Task ����\n");
#endif

				SET_DOOR_R_DIR_N();
				Door_R_MotoStart(MOTO_PUTY_99);

				SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_DOWN; //����
				SysTask.Moto_RunTime[GLASS_02] = 450; //ǿ��ʱ��,ʱ�䵽�͵���PWMռ�ձȼ�����ȥ
			}

			SysTask.Moto_ModeSave[GLASS_02] = SysTask.Moto_Mode[GLASS_02];
			break;

		case MOTO_STATE_RUN_NOR: //��ͨ����״̬
			if (SysTask.Moto_Mode[GLASS_02] != SysTask.Moto_ModeSave[GLASS_02]) //��;ģʽ�ı�
			{
				Door_R_MotoStop();					//����ͣ���ת��  --
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				SysTask.Moto_StateTime[GLASS_02] = 10;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_02])
			{
				case MOTO_SUB_STATE_UP: //����
					if (SysTask.Moto_RunTime[GLASS_02] == 0) //��ʱ�˳�
					{
#if DEBUG
						u3_printf(" Moto_02_Task ��ͣ���\n");
#endif

						Door_R_MotoStop();			//����ͣ���ת��  --

						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}

					break;

				case MOTO_SUB_STATE_DOWN: //���� 
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_R) == 1))
					{
#if DEBUG
						u3_printf(" Moto_02_Task �����ͣ���\n");
#endif

						Door_R_MotoStop();			//����ͣ���ת��  --

						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_02] == 0) //��ʱ
					{

#if DEBUG
						u3_printf(" Moto_02_Task ����\n");
#endif

						SET_DOOR_R_DIR_N();
						Door_R_MotoStart(MOTO_PUTY_10);

						SysTask.Moto_RunTime[GLASS_02] = 400;
						SysTask.Moto_SubState[GLASS_02] = MOTO_SUB_STATE_DEC;
					}

					break;

				case MOTO_SUB_STATE_DEC: //����
					if (GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_DOOR_PIN_R) == 1)
					{
#if DEBUG
						u3_printf(" Moto_02_Task ����--�����ͣ���\n");
#endif

						Door_R_MotoStop();			//����ͣ���ת��  --
						SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_02] == 0) //��ʱ
					{
#if DEBUG
						u3_printf(" Moto_02_Task ��ʱ��ͣ���\n");
#endif

						SysTask.Moto_RunTime[GLASS_02] = 100;
						Door_R_MotoStop();			//����ͣ���ת��  --
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
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
			}

			break;

		default:
			SysTask.Moto_State[GLASS_02] = MOTO_STATE_IDLE; //
			break;
	}
}


/*******************************************************************************
* ����: 
* ����: �м�� ������ �½�ʱ�и������ ������ʹ��ʱ�����жϡ���ת������ת��
* �β�:		
* ����: ��
* ˵��:
*******************************************************************************/
void Moto_03_Task(void)
{
	//�������
	switch (SysTask.Moto_State[GLASS_03])
	{
		case MOTO_STATE_INIT:
			if (SysTask.Moto_StateTime[GLASS_03] != 0)
				break;

			SysTask.Moto_State[GLASS_03] = MOTO_STATE_RUN_NOR;

			if (SysTask.Moto_Mode[GLASS_03] == MOTO_FR_FWD) //��ת������
			{
#if DEBUG
				u3_printf(" Moto_03_Task ��ת������\n");
#endif

				SysTask.Moto_RunTime[GLASS_03] = SysTask.nTick + 200; //ǿ��ʱ�䣬up to this time full speed rise
				SET_RISE_STEP_DIR_P();
				Rise_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_UP;
			}
			else 
			{
#if DEBUG
				u3_printf(" Moto_03_Task �½�\n");
#endif

				if (GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_D) == 0)
				{
#if DEBUG
					u3_printf(" Moto_03_Task �Ѿ����\n");
#endif

					SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					SysTask.Moto_ModeSave[GLASS_03] = SysTask.Moto_Mode[GLASS_03];
					break;
				}

				SET_RISE_STEP_DIR_N();
				Rise_MotoStart(MOTO_PUTY_99);
				SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_DOWN; //�½�
				SysTask.Moto_RunTime[GLASS_03] = SysTask.nTick + 1000; ////ǿ��ʱ��,ʱ�䵽�͵���PWMռ�ձȼ�����ȥ
			}

			SysTask.Moto_ModeSave[GLASS_03] = SysTask.Moto_Mode[GLASS_03];
			break;

		case MOTO_STATE_RUN_NOR: //��ͨ����״̬
			if (SysTask.Moto_Mode[GLASS_03] != SysTask.Moto_ModeSave[GLASS_03]) //��;ģʽ�ı�
			{
				Rise_MotoStop();
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
				SysTask.Moto_StateTime[GLASS_03] = 10;
				break;
			}

			switch (SysTask.Moto_SubState[GLASS_03])
			{
				case MOTO_SUB_STATE_UP:
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_U) == 0))
					{
#if DEBUG
						u3_printf(" Moto_03_Task up���ֹͣ\n");
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
						u3_printf(" Moto_03_Task up ���ֹͣ\n");
#endif

						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) //��ʱ�˳�
					{
#if DEBUG
						u3_printf(" Moto_03_Task up ��ʱ�˳� \n");
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
						u3_printf(" Moto_03_Task down���ֹͣ\n");
#endif

						SysTask.Moto_StateTime[GLASS_03] = 200;
						Rise_MotoStop();
						SysTask.Moto_State[GLASS_03] = MOTO_STATE_WAIT; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) //����
					{

#if DEBUG
						u3_printf(" Moto_03_Task ����\n");
#endif
                        
                        SET_RISE_STEP_DIR_N();
                        Rise_MotoStart(MOTO_PUTY_30);

						SysTask.Moto_RunTime[GLASS_03] = 2000;
						SysTask.Moto_SubState[GLASS_03] = MOTO_SUB_STATE_DEC;
					}

					break;

				case MOTO_SUB_STATE_DEC: //����
					if ((GPIO_ReadInputDataBit(SENSOR_GPIO, SENSOR_RISE_PIN_D) == 0))
					{
#if DEBUG
						u3_printf(" Moto_03_Task ����--�����ͣ���\n");
#endif

						Rise_MotoStop();

						SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
					}
					else if (SysTask.Moto_RunTime[GLASS_03] == 0) //��ʱ
					{
#if DEBUG
						u3_printf(" Moto_03_Task ����--��ʱ��ͣ���\n");
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
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT; //����ʱһ��ʱ���ٷ�ת
			}

			break;

		default:
			SysTask.Moto_State[GLASS_03] = MOTO_STATE_IDLE; //
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
void Moto_04_Task(void)
{
	StepMotoData_t * pStepMotoData;

	//_03���
	switch (SysTask.Moto_State[GLASS_04])
	{
		case MOTO_STATE_INIT:
			if (g_StepMotoEvent.bMutex == FALSE)
			{
#if DEBUG
				u3_printf(" Moto_04_Task ת��һ����\n");
#endif

				g_StepMotoEvent.bMutex = TRUE;		//������ֻ����һ��

				//g_StepMotoEvent.MachineCtrl.u32Timeout = 6000; //1S ��ʱ  ,5�ξ���6S
				g_StepMotoEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
				g_StepMotoEvent.MachineRun.bFinish = FALSE;
				pStepMotoData		= (StepMotoData_t *)
				g_StepMotoEvent.MachineCtrl.pData;
				g_StepMotoEvent.MachineCtrl.CallBack(STEP_FIND_ZERO, &g_StepMotoEvent);
			}
			else //ȡ���
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

		case MOTO_STATE_RUN_NOR: //��ͨ����״̬ 1/4 2/4 3/4 Ȧ�������ȥ��һ����Ϊ�˼�С���ʹ�ò������ķ�ʽ
			if (g_StepMotoEvent.bMutex == FALSE)
			{
#if DEBUG
				u3_printf(" Moto_04_Task ת�� %d	/n", SysTask.u16StepRequire);
#endif

				g_StepMotoEvent.bMutex = TRUE;		//������ֻ����һ��
				g_StepMotoEvent.MachineRun.u32StartTime = SysTask.u32SysTimes;
				g_StepMotoEvent.MachineRun.bFinish = FALSE;
				pStepMotoData		= (StepMotoData_t *)
				g_StepMotoEvent.MachineCtrl.pData;
				pStepMotoData->u16Step = SysTask.u16StepRequire;
				g_StepMotoEvent.MachineCtrl.CallBack(STEP_RUN, &g_StepMotoEvent);
			}
			else //ȡ���
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
* ����: 
* ����: 
* �β�:		
			 ___
			|1ms|	3ms |	 
			 ___		 ___		___ 	
���� _______| 	|_______|	|______|   |______

			|1ms|	 10ms	   |	
			 ___				___ 			 ___	 
�أ� _______| 	|______________|   |____________|	|__________

* ����: ��
* ˵��: 
*******************************************************************************/
void Lock_01_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_ONE)    //��ǰչʾ���ǵڼ�����,ֻ��չʾ�ı�ſ��Կ�������ֹ�������������
        return;
    
	switch (SysTask.Lock_State[GLASS_01]) //_02��
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_01] != 0)
				break;

			if (SysTask.LockMode[GLASS_01] != SysTask.LockModeSave[GLASS_01])
			{
				if (SysTask.LockMode[GLASS_01] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_01_Task ����\n");
#endif

					SysTask.Lock_StateTime[GLASS_01] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_01] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_01] = SysTask.LockMode[GLASS_01];
					SysTask.Lock_OffTime[GLASS_01] = 0;
				}
				else //��Ҫ����
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_01_Task ��Ҫ����\n");
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
				u3_printf(" Lock_01_Task �������\n");
#endif
				SysTask.Lock_State[GLASS_01] = LOCK_STATE_DETECT;
				GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
				SysTask.Lock_StateTime[GLASS_01] = SysTask.nTick + LOCK_SET_OFF_TIME; //ֹͣ��ƽ
				SysTask.LockMode[GLASS_01] = LOCK_OFF;
	
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
    if(SysTask.WatchDisplay != DISPLAY_TWO)    //��ǰչʾ���ǵڼ�����,ֻ��չʾ�ı�ſ��Կ�������ֹ�������������
        return;
    
	switch (SysTask.Lock_State[GLASS_02]) //_02��
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_02] != 0)
				break;

			if (SysTask.LockMode[GLASS_02] != SysTask.LockModeSave[GLASS_02])
			{
				if (SysTask.LockMode[GLASS_02] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_02_Task ����\n");
#endif
					SysTask.Lock_StateTime[GLASS_02] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_02] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_02] = SysTask.LockMode[GLASS_02];
					SysTask.Lock_OffTime[GLASS_02] = 0;
				}
				else //��Ҫ����
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_02_Task ��Ҫ����\n");
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
				u3_printf(" Lock_02_Task �������\n");
#endif
				SysTask.Lock_State[GLASS_02] = LOCK_STATE_DETECT;
				GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
				SysTask.Lock_StateTime[GLASS_02] = SysTask.nTick + LOCK_SET_OFF_TIME; //ֹͣ��ƽ
				SysTask.LockMode[GLASS_02] = LOCK_OFF;

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
void Lock_03_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_THREE)    //��ǰչʾ���ǵڼ�����,ֻ��չʾ�ı�ſ��Կ�������ֹ�������������
        return;
    
	switch (SysTask.Lock_State[GLASS_03]) //_02��
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_03] != 0)
				break;

			if (SysTask.LockMode[GLASS_03] != SysTask.LockModeSave[GLASS_03])
			{
				if (SysTask.LockMode[GLASS_03] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_03_Task ����\n");
#endif
					SysTask.Lock_StateTime[GLASS_03] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_03] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_03] = SysTask.LockMode[GLASS_03];
					SysTask.Lock_OffTime[GLASS_03] = 0;
				}
				else //��Ҫ����
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_03_Task ��Ҫ����\n");
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
				u3_printf(" Lock_03_Task �������\n");
#endif

				SysTask.Lock_State[GLASS_03] = LOCK_STATE_DETECT;
				GPIO_SetBits(LOCK_GPIO, LOCK_03_PIN);
				SysTask.Lock_StateTime[GLASS_03] = SysTask.nTick + LOCK_SET_OFF_TIME; //ֹͣ��ƽ
				SysTask.LockMode[GLASS_03] = LOCK_OFF;

			break;

		default:
			break;
	}
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:	ֻ�ÿ���	
* ����: ��
* ˵��: 
*******************************************************************************/
void Lock_04_Task(void)
{
    if(SysTask.WatchDisplay != DISPLAY_FOUR)    //��ǰչʾ���ǵڼ�����,ֻ��չʾ�ı�ſ��Կ�������ֹ�������������
        return;
    
	switch (SysTask.Lock_State[GLASS_04]) //_02��
	{
		case LOCK_STATE_DETECT:
			if (SysTask.Lock_StateTime[GLASS_04] != 0)
				break;

			if (SysTask.LockMode[GLASS_04] != SysTask.LockModeSave[GLASS_04])
			{
				if (SysTask.LockMode[GLASS_04] == LOCK_ON)
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_04_Task ����\n");
#endif
					SysTask.Lock_StateTime[GLASS_04] = LOCK_OPEN_TIME; //
					SysTask.Lock_State[GLASS_04] = LOCK_STATE_OPEN;
					SysTask.LockModeSave[GLASS_04] = SysTask.LockMode[GLASS_04];
					SysTask.Lock_OffTime[GLASS_04] = 0;
				}
				else //��Ҫ����
				{
#if DEBUG_MOTO2
					u3_printf(" Lock_04_Task ��Ҫ����\n");
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
				u3_printf(" Lock_04_Task �������\n");
#endif

				GPIO_SetBits(LOCK_GPIO, LOCK_04_PIN);
				SysTask.Lock_State[GLASS_04] = LOCK_STATE_DETECT;
				SysTask.Lock_StateTime[GLASS_04] = SysTask.nTick + LOCK_SET_OFF_TIME; //ֹͣ��ƽ
				SysTask.LockMode[GLASS_04] = LOCK_OFF;
	

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

				SysTask.Moto_Mode[GLASS_03] = MOTO_FR_REV; //����������Ĭ���ϵ�Ҫ�½�
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_03] = 0;
				SysTask.u16InitWaitTime = 800;		//������ȫ����ȥ�Ϳ��Թ���
			}

			Lock_01_Task();
			Lock_02_Task();
			Lock_03_Task();
			Lock_04_Task();
			break;

		case INIT_STATE_DOWN:
			if ((SysTask.u16InitWaitTime == 0) || (SysTask.Moto_State[GLASS_03] == MOTO_STATE_IDLE))
			{
				SysTask.Moto_Mode[GLASS_01] = MOTO_FR_REV; //�������Ĭ���ϵ�Ҫ����
				SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_01] = 0;

				SysTask.Moto_Mode[GLASS_02] = MOTO_FR_REV; //�������Ĭ���ϵ�Ҫ����
				SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_02] = 0;

				SysTask.InitState	= INIT_STATE_DOOR_CLOSE;
				SysTask.u16InitWaitTime = 4000; 	//�ȴ���ȫ�ر�
			}

			Moto_03_Task(); //�м�� ������ �������½�����һ����������Ӧ
			break;

		case INIT_STATE_DOOR_CLOSE:
			if ((SysTask.u16InitWaitTime == 0) || 
                ((SysTask.Moto_State[GLASS_01] == MOTO_STATE_IDLE) &&
				 (SysTask.Moto_State[GLASS_02] == MOTO_STATE_IDLE)&&
				 (SysTask.Moto_State[GLASS_03] == MOTO_STATE_IDLE)))
			{
				SysTask.InitState	= INIT_STATE_ZERO;

				if (SysTask.u16StepRequire == 0) //����ǵ�һ����
				{
					SysTask.Moto_State[GLASS_04] = MOTO_STATE_INIT;
				}
				else 
				{
					SysTask.Moto_State[GLASS_04] = MOTO_STATE_RUN_NOR;
				}

				SysTask.u16InitWaitTime = 10000;	//�ȴ�
			}

			Moto_03_Task(); //�м�� ������ �������½�����һ����������Ӧ
			Moto_01_Task(); //�����ţ�ֻ������ת�ģ��ض���һ����������Ӧ��������ʱ�ķ����� 
			Moto_02_Task(); //�����ţ�ֻ������ת�ģ��ض���һ����������Ӧ
			break;

		case INIT_STATE_ZERO: //ת����ָ����λ��
			if ((SysTask.Moto_State[GLASS_04] == MOTO_STATE_IDLE) || (SysTask.u16InitWaitTime == 0))
			{
				SysTask.InitState	= INIT_STATE_FINISH;

				if (SysTask.bGlassChange == TRUE) //����ǽ��յ����������׼����������
				{
					SysTask.Moto_Mode[GLASS_01] = MOTO_FR_FWD; //��ת���� 
					SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT;
					SysTask.Moto_WaitTime[GLASS_01] = 0;


					SysTask.Moto_Mode[GLASS_02] = MOTO_FR_FWD; //
					SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT;
					SysTask.Moto_WaitTime[GLASS_02] = 0;


					SysTask.InitState	= INIT_STATE_DOOR_OPEN;
					SysTask.u16InitWaitTime = 2;	//������ȫ���Ϳ�������
				}
				else 
				{

					SysTask.bGlassChange = FALSE;
                    SysTask.u16InitWaitTime = 0;
					SysTask.InitState	= INIT_STATE_FINISH;
				}
			}

			Moto_04_Task(); //�м��ת����   PWM ��ȷ���Ʋ������
			break;

		case INIT_STATE_DOOR_OPEN:
			if ((SysTask.u16InitWaitTime == 0) || ((SysTask.Moto_State[GLASS_01] == MOTO_STATE_IDLE) &&
				 (SysTask.Moto_State[GLASS_02] == MOTO_STATE_IDLE)))
			{
				SysTask.InitState	= INIT_STATE_UP;

				SysTask.Moto_Mode[GLASS_03] = MOTO_FR_FWD; //����������
				SysTask.Moto_State[GLASS_03] = MOTO_STATE_INIT;
				SysTask.Moto_WaitTime[GLASS_03] = 0;
				SysTask.u16InitWaitTime = 6000; 	//������ȫ���Ϳ�������
			}

			Moto_01_Task(); //�����ţ�ֻ������ת�ģ��ض���һ����������Ӧ��������ʱ�ķ����� 
			Moto_02_Task(); //�����ţ�ֻ������ת�ģ��ض���һ����������Ӧ
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
                    
    			//���͵�ƽ��ˣ�Ӧ��	 ����
    			g_FingerAck = ACK_SUCCESS;
    			g_AckType = DETECT_ACK_TYPE;

    			if (SysTask.SendState == SEND_IDLE)
    				SysTask.SendState = SEND_TABLET_FINGER;
            
			}

			Moto_01_Task(); //�����ţ�ֻ������ת�ģ��ض���һ����������Ӧ��������ʱ�ķ����� 
			Moto_02_Task(); //�����ţ�ֻ������ת�ģ��ض���һ����������Ӧ
			Moto_03_Task(); //�м�� ������ �������½�����һ����������Ӧ
			break;

		case INIT_STATE_FINISH:
			if (SysTask.u16InitWaitTime != 0)
				break;

			SendTabletTask(); //���͸�ƽ������
			TabletToHostTask(); //����ƽ������������
			Lock_01_Task();
			Lock_02_Task();
			Lock_03_Task();
			Lock_04_Task();
			Moto_01_Task(); //�����ţ�ֻ������ת�ģ�����һ����������Ӧ
			Moto_02_Task(); //�����ţ�ֻ������ת�ģ�����һ����������Ӧ
			Moto_03_Task(); //�м�� ������ �������½�����һ����������Ӧ
			Moto_04_Task(); //�м��ת����   PWM ��ȷ���Ʋ������	

			if (SysTask.bGlassChange == TRUE) //����ǽ��յ�����������
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
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void MainTask(void)
{
	//ϵͳ�ϵ�׼��ʱ�䣨�����һ����ͣ���˻�û�й������½������š��ٴ��ϵ�Ҫ�ڵ�һʱ������׼��������ʱ�䡣��ƽ���ϵ�ʱ�������
	SystemInitTask();

	//SysSaveDataTask();
	LedTask();


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
	g_StepMotoEvent.bInit = TRUE;
	g_StepMotoEvent.bMutex = FALSE;
	g_StepMotoEvent.MachineCtrl.pData = (void *)
	malloc(sizeof(StepMotoData_t));
	g_StepMotoEvent.MachineCtrl.CallBack = CenterMoto_Event;
	g_StepMotoEvent.MachineCtrl.u16InterValTime = 5; //5ms ����һ��

	g_StepMotoEvent.MachineRun.bEnable = TRUE;
	g_StepMotoEvent.MachineRun.u32StartTime = 0;
	g_StepMotoEvent.MachineRun.u32LastTime = 0;
	g_StepMotoEvent.MachineRun.u32RunCnt = 0;
	g_StepMotoEvent.MachineRun.bFinish = FALSE;
	g_StepMotoEvent.MachineRun.u16RetVal = 0;
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

		//memcpy(SysTask.FlashData.u8FlashData, SysTask.WatchState, sizeof(SysTask.WatchState));
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
	SysTask.InitState	= INIT_STATE_LOCK;			//�ϵ��ȹ���
	SysTask.u16InitWaitTime = 400;					//�����ʱ�� �������õ�ʱ��

	SysTask.SendState	= SEND_IDLE;
	SysTask.SendSubState = SEND_SUB_INIT;
	SysTask.bTabReady	= FALSE;
	SysTask.LED_ModeSave = LED_MODE_DEF;			// ��֤��һ�ο�������ģʽ
	SysTask.GlassNum	= GLASS_NUM_ONE;
	SysTask.bGlassChange = FALSE;
	SysTask.u16StepRequire = 0;

    SysTask.WatchDisplay = DISPLAY_DEF;    //��ǰչʾ���ǵڼ�����
	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		SysTask.LockModeSave[i] = LOCK_ON;
		SysTask.LockMode[i] = LOCK_OFF; 			//�ϵ��һ�����ǹ���
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
		SysTask.Moto_State[i] = MOTO_STATE_INIT;	//��֤����������ת
		SysTask.Moto_SubState[i] = MOTO_SUB_STATE_RUN;
		SysTask.Lock_StateTime[i] = 50; 			//���Ҫ���� 10ms�ٽ�ȥ 	����Ȼ��һ������ֻ��900��us �ߵ�ƽ  
	}

	SysTask.Moto_Mode[GLASS_02] = MOTO_FR_REV;		//����������Ĭ���ϵ�Ҫ�½�
	SysTask.Moto_State[GLASS_02] = MOTO_STATE_INIT;
	SysTask.Moto_WaitTime[GLASS_02] = 0;

	SysTask.Moto_Mode[GLASS_01] = MOTO_FR_REV;		//�������Ĭ���ϵ�Ҫ����
	SysTask.Moto_State[GLASS_01] = MOTO_STATE_INIT;
	SysTask.Moto_WaitTime[GLASS_01] = 0;

	//SysTask.u16BootTime = 3000;	//3��ȴ��ӻ�������ȫ
}


