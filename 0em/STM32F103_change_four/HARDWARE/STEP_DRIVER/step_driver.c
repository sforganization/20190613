
#include "step_driver.h"
#include "SysComment.h"
#include <math.h>

static vs32 	g_stepRun = 4800; //Ҫ�������еĲ���	  

float			g_fre[ACCELERATED_SPEED_LENGTH]; //����洢���ٹ�����ÿһ����Ƶ�� 
unsigned short	g_period[ACCELERATED_SPEED_LENGTH]; //���鴢����ٹ�����ÿһ����ʱ�����Զ�װ��ֵ	 

static volatile StepMotoState_T g_status = ACCEL; //�ж�״̬��״̬
static volatile bool g_Ch1Enable = FALSE; //
static volatile bool g_Ch2Enable = FALSE; //
static volatile bool g_Ch3Enable = FALSE; //
static volatile bool g_Ch4Enable = FALSE; //

static volatile s32 g_acce_num_config = 0; //���ü��ٵĲ���

static volatile s32 g_acce_num = 0; //���ٵĲ��� 
static volatile u8 g_remote = 0; //Զ�˼��ٹ��������


static StepEventState_T EventState = STEP_STATE_INIT;
static StepEventSubState_T EventSubState = STEP_SUB_STATE_INIT;


/**
  * ��������: ͨ�ö�ʱ�� TIMx,x[2,3,4,5]�ж����ȼ�����
  * �������: ��
  * �� �� ֵ: ��
  * ˵    ������
  */
void TIM4_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* �����ж���Ϊ0 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	/* �����ж���Դ */
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;

	/* ���������ȼ�Ϊ 0 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;

	/* ������ռ���ȼ�Ϊ3 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;

	/*��ʱ��ʹ�� */
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
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
static void CalculateSModelLine(float fre[], unsigned short period[], float len, float fre_max, float fre_min, 
	float flexible)
{
	int 			i	= 0;
	float			deno;
	float			melo;
	float			delt = fre_max - fre_min;

	for (; i < len; i++)
	{
		melo				= flexible * (i - len / 2) / (len / 2);
		deno				= 1.0f / (1 + expf(-melo)); //expf is a library function of exponential(e) 
		fre[i]				= delt * deno + fre_min;
		period[i]			= (unsigned short) ((float) TIMER_FRE / fre[i]); // TIMER_FRE is the timer driver frequency
	}

	return;
}

//ʹ��H��
//    �����ת��DIR=1 PWM=PWM  
//    �����ת��DIR=0 PWM=PWM
//    ͣ��ɲ����DIR=X PWM=0(XΪ����״̬)
//    ���Թ�����0��-99����PWM����ռ�ձȣ�ʹ������Եõ��㹻��������ѹ
static void Set_TIM4_Ch1_Enable(u16 arr, u16 puty)
{
    g_Ch1Enable     = TRUE;
	TIM_SetAutoreload(TIM4, arr);
	TIM_SetCompare1(TIM4, puty);
}

static void Set_TIM4_Ch1_Disable(void)
{
    // �ر�ͨ��
    g_Ch1Enable     = FALSE;
    TIM_SetCompare1(TIM4, 0);
}

static void Set_TIM4_Ch2_Enable(u16 arr, u16 puty)
{
	g_Ch2Enable		= TRUE;
	TIM_SetAutoreload(TIM4, arr);
	TIM_SetCompare2(TIM4, puty);
}

static void Set_TIM4_Ch2_Disable(void)
{
	// �ر�ͨ��
	g_Ch2Enable		= FALSE;
	TIM_SetCompare2(TIM4, 0);
}

static void Set_TIM4_Ch3_Enable(u16 arr, u16 puty)
{
	g_Ch3Enable		= TRUE;
	TIM_SetAutoreload(TIM4, arr);
	TIM_SetCompare3(TIM4, puty);
}

static void Set_TIM4_Ch3_Disable(void)
{
	// �ر�ͨ��
	g_Ch3Enable		= FALSE;
	TIM_SetCompare3(TIM4, 0);
}


static void Set_TIM4_Ch4_Enable(void)
{
	g_Ch4Enable		= TRUE;

}

static void Set_TIM4_Ch4_Disable(void)
{
	// �ر�ͨ��
	g_Ch4Enable		= FALSE;
	TIM_SetCompare4(TIM4, 0);
}

void TIM4_IRQHandler(void)
{
	int 			tmp = 0;

	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) //��PWMģʽ������ʡ�� I �������
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

		if (g_Ch4Enable)   //�м���ת���
		{
			//��ʱ��ʹ�÷�תģʽ����Ҫ���������жϲ����һ����������
			//�����PWMģʽ ֻ��Ҫ��һ�ξͿ��Ե�һ������
			//if (TIM_GetITStatus(TIM4, TIM_IT_CC4) != RESET)  //���ʹ�ñȽϷ�תģʽ�����������
			switch (g_status)
			{
				case ACCEL: //����
					TIM_SetAutoreload(TIM4, g_period[g_acce_num]);
					TIM_SetCompare4(TIM4, g_period[g_acce_num] / 2); //���������ʹ�� ���� ������ Ҳ���Բ��ñȽ���� ��������pwmģʽ���������2 ����һ��������ռ�ձ�Ϊ50%
					g_acce_num++; //��������� ����һ��
					g_remote = 0;

					if (g_acce_num >= ACCELERATED_SPEED_LENGTH)
					{
						g_remote			= 1;
					}

					if (g_acce_num >= g_acce_num_config)
					{
						//					if (g_stepRun <= 0)    //��Ϊ�ս�����ʱ��ǰ��Ӧ�û���һ�����壬û�з���������RUN�����һ�������ֲ�
						//						g_status = DECEL;
						//					  else
						g_status			= RUN;
					}

					break;

				case RUN: //����
					if (g_stepRun <= 0)
						g_status = DECEL;

					g_stepRun--;
					break;

				case DECEL: //����
					g_acce_num--;

					if ((EventState == STEP_STATE_INIT) && (EventSubState == STEP_SUB_STATE_DEC)) //�����ʱ������Ҫ����
					{
						if (g_remote == 1)
						{
							tmp 				= (g_acce_num * ACCELERATED_SPEED_LENGTH / DEC_STEP_NUM);
							TIM_SetAutoreload(TIM4, g_period[tmp]);
							TIM_SetCompare4(TIM4, g_period[tmp] / 2);
						}
						else 
						{
							TIM_SetAutoreload(TIM4, g_period[g_acce_num]);
							TIM_SetCompare4(TIM4, g_period[g_acce_num] / 2);
						}
					}
					else 
					{
						TIM_SetAutoreload(TIM4, g_period[g_acce_num]);
						TIM_SetCompare4(TIM4, g_period[g_acce_num] / 2);
					}

					if (g_acce_num <= 1)
						g_status = STOP;

					break;

				case STOP: //ֹͣ
					Set_TIM4_Ch4_Disable();
					g_status = ACCEL;
			}
		}
	}
}


//ʹ�ö�ʱ��4		  ch4 pwm��Ϊ�����������  PB9
void CenterMotoInit(u16 arr, u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM4_NVIC_Configuration();

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); // 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; //	 tim4 ch1 ch2 ch4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	STEP_DIR_RCC_CLOCKCMD(STEP_DIR_RCC_CLOCKGPIO, ENABLE); //
	GPIO_InitStructure.GPIO_Pin = STEP_DOOR_DIR_PIN_L | STEP_DOOR_DIR_PIN_R | STEP_RISE_DIR_PIN| STEP_CENTER_DIR_PIN; 	//	 DIR
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //  because the PWM driver IS 5v supply so we should pull up 47k to vcc 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(STEP_DIR_GPIO, &GPIO_InitStructure);

	SET_CENTER_STEP_DIR_P();
    SET_RISE_STEP_DIR_P();
    SET_DOOR_L_DIR_P();
    SET_DOOR_R_DIR_P();

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0x0A;		//Ϊ�˽��жϣ�һ���õȴ�10ms�ͽ���
	TIM_TimeBaseStructure.TIM_Prescaler = psc;		//��ʱ��Ƶ�ʼ�����16λ��ʱ����0xffff
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;	//0
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;				//
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //must   �͵�ƽ�����ź�
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);

	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //must
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);		//ch1 ch2�ǿ��ƿ��ŵ����   ch3 ���������
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);

    
	TIM_ClearFlag(TIM4, TIM_IT_Update);
	TIM_CtrlPWMOutputs(TIM4, ENABLE);				//
	TIM_ARRPreloadConfig(TIM4, ENABLE); 			//
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable); //
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable); //
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable); //
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable); //

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM4, ENABLE);  
}


/*step	��Ҫת���ĽǶ�*/
void CenterMotoRun(u16 step)
{
	if (step >= ACCELERATED_SPEED_LENGTH * 2)
	{
		g_stepRun			= step - ACCELERATED_SPEED_LENGTH * 2; //���ٽ׶ε�Ϊ�ܲ�����ȥ���ٺͼ��ٽ׶εĲ���

	}
	else 
	{
		g_stepRun			= step; 				//��0��ʱ�� ��һ���պ���1/4Ȧ
	}


	CalculateSModelLine(g_fre, g_period, ACCELERATED_SPEED_LENGTH, FRE_MAX, FRE_MIN, FIEXIBLE); //���һ������

	//		  for(i = 0; i < ACCELERATED_SPEED_LENGTH; i++){
	//			  g_period[i] = 50;
	//		  }
	Set_TIM4_Ch4_Enable();
}


void CenterMotoStop(void)
{
	Set_TIM4_Ch4_Disable();
}

/*******************************************************************************
* ����: 
* ����: 
* �β�:	
* ����: ��
* ˵��: 
*******************************************************************************/
int CenterMoto_Event(char opt, void * data)
{
	//	  static  u8	  u8EventCmd;
	PollEvent_t *	pEvent;
	StepMotoData_t * pStepMotoData;

	static vu32 	u32TimeSave = 0;
	static s32		s32StepNum = 0; 				//��Ҫ�����ߵĲ���

	pEvent				= data;
	pStepMotoData		= (StepMotoData_t *)
	pEvent->MachineCtrl.pData;

	switch (EventState)
	{
		case STEP_STATE_INIT:
			pEvent->MachineRun.bFinish = FALSE;

			switch (opt)
			{
				case STEP_FIND_ZERO: //��ʼ����ԭ��
					switch (EventSubState)
					{
						case STEP_SUB_STATE_INIT: //ȥ��ԭ��
							SET_CENTER_STEP_DIR_P();
							g_acce_num_config = ACCELERATED_SPEED_LENGTH;
							CenterMotoRun(CIRCLE_STEP_PULSE + 20); //���100������ 1600 1/4		200 1/32
							EventSubState = STEP_SUB_STATE_ZERO_L;
							g_remote = 0;
							break;

						case STEP_SUB_STATE_ZERO_L:
							if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_STEP_MOTO_PIN) == 0) //��⵽���	
							{
								g_status			= DECEL; //����ֱ��ֹͣ
								g_acce_num			= DEC_STEP_NUM;
								s32StepNum			= g_acce_num;
								EventSubState		= STEP_SUB_STATE_DEC;
							}
							else if (g_Ch4Enable == FALSE) //ת��һȦû���ҵ���㣬���˳���
							{
								pEvent->MachineRun.u16RetVal = EVENT_TIME_OUT;
								EventState			= STEP_STATE_EXIT;
							}

							break;

						case STEP_SUB_STATE_DEC: //����ֹͣ
							if (g_Ch4Enable == FALSE) //�Ѿ�ֹͣ��
							{
								SET_CENTER_STEP_DIR_N();
								g_acce_num_config	= s32StepNum + 50; //  ��ǰ���߼�����
								EventSubState		= STEP_SUB_STATE_ZERO_R;

								//�����ߣ������λ��
								CenterMotoRun(0);	//
							}

							break;

						case STEP_SUB_STATE_ZERO_R:
							if (GPIO_ReadInputDataBit(LOCK_ZERO_GPIO, LOCK_ZERO_STEP_MOTO_PIN) == 0) //��⵽���	
							{
								g_status			= DECEL;
								g_acce_num			= DEBOUNCH_STEP_NUM; //5�������������м�λ��
								EventSubState		= STEP_SUB_STATE_DEBOUNCH;
							}
							else if (g_Ch4Enable == FALSE) //ת�경��û���ҵ���㣬���˳���
							{
								pEvent->MachineRun.u16RetVal = EVENT_TIME_OUT;
								EventState			= STEP_STATE_EXIT;
							}

							break;

						case STEP_SUB_STATE_DEBOUNCH:
							if (g_Ch4Enable == FALSE) //�Ѿ�ֹͣ��
							{
								EventState			= STEP_STATE_EXIT;
								EventSubState		= STEP_SUB_STATE_INIT;
							}

							break;

						default:
							EventSubState = STEP_SUB_STATE_INIT;
							break;
					}

					break;

				case STEP_RUN: //��ͨ����״̬
					switch (EventSubState)
					{
						case STEP_SUB_STATE_INIT:
							SET_CENTER_STEP_DIR_P();

                            if(pStepMotoData->u16Step < CIRCLE_STEP_PULSE / 4){
                                g_acce_num_config = pStepMotoData->u16Step / 2;
                                CenterMotoRun(pStepMotoData->u16Step % 2);
                            }else{
    							g_acce_num_config = ACCELERATED_SPEED_LENGTH;
    							CenterMotoRun(pStepMotoData->u16Step);
                            }
							EventSubState = STEP_SUB_STATE_DEBOUNCH;
							break;

						case STEP_SUB_STATE_DEBOUNCH:
							if (g_Ch4Enable == FALSE) //�Ѿ�ֹͣ��
							{
								EventState			= STEP_STATE_EXIT;
								EventSubState		= STEP_SUB_STATE_INIT;
							}

							break;

						default:
							EventSubState = STEP_SUB_STATE_INIT;
							break;
					}

					break;
			}

			break;

		case STEP_STATE_WAIT: // �ȴ�hal��״̬����
			//			  if (1 & 0X8000) //���յ�һ������
			//			  {
			//				  
			//			  }
			//			  else if (pEvent->MachineRun.u32LastTime - pEvent->MachineRun.u32StartTime >
			//				   pEvent->MachineCtrl.u32Timeout) //״̬����ʱ�˳�
			//			  {
			//				  pEvent->MachineRun.u16RetVal = EVENT_TIME_OUT;
			//				  EventState		 = STEP_STATE_EXIT;
			//			  }
			EventState = STEP_STATE_EXIT;
			break;

		case STEP_STATE_EXIT:
			pEvent->MachineRun.bFinish = TRUE;
			EventState = STEP_STATE_INIT; //Ϊ��һ�γ�ʼ��״̬��
			EventSubState = STEP_SUB_STATE_INIT;
			break;

		default:
			break;
	}

	return 0;
}


//��
void Door_L_MotoStart(u16 puty)
{
	Set_TIM4_Ch1_Enable(MOTO_ARR,  puty);
}

void Door_L_MotoStop(void)
{
	Set_TIM4_Ch1_Disable();
}

void Door_R_MotoStart(u16 puty)
{
	Set_TIM4_Ch2_Enable(MOTO_ARR,  puty);
}

void Door_R_MotoStop(void)
{
	Set_TIM4_Ch2_Disable();
}

//
void Rise_MotoStart(u16 puty)
{
	Set_TIM4_Ch3_Enable(MOTO_ARR,  puty);
}

void Rise_MotoStop(void)
{
	Set_TIM4_Ch3_Disable();
}


