
#include "led.h"

#include "SysComment.h"



//��ʱ��3�жϷ������			
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //�Ǹ����ж�
	{
//		USART1_RX_STA		|= 1 << 15; 			//��ǽ������
//		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //���TIM3�����жϱ�־	  
//		TIM_Cmd(TIM3, DISABLE); 					//�ر�TIM3 
	}
}

static void Set_TIM3_Ch3_Enable(u16 arr, u16 puty)
{
	TIM_SetAutoreload(TIM3, arr);
	TIM_SetCompare3(TIM3, puty);
}

static void Set_TIM3_Ch3_Disable(void)
{
    // �ر�ͨ��
    TIM_SetCompare3(TIM3, 0);
}

static void Set_TIM3_Ch4_Enable(u16 arr, u16 puty)
{
	TIM_SetAutoreload(TIM3, arr);
	TIM_SetCompare4(TIM3, puty);
}

static void Set_TIM3_Ch4_Disable(void)
{
	// �ر�ͨ��
	TIM_SetCompare4(TIM3, 0);
}

void Led_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //TIM3ʱ��ʹ��    
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; //	 tim3 ch3 ch4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = LED_ARR; 		//��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler = LED_PSC;		//����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
    
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;				//
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //must
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);		
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);


    TIM_CtrlPWMOutputs(TIM3, ENABLE);				  //
	TIM_ARRPreloadConfig(TIM3, ENABLE); 			  //
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable); //
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable); //

//	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);  
}


void Led_1_Brighness(u16 puty)//PB0  CH3
{
	Set_TIM3_Ch3_Enable(LED_ARR,  puty);
}

void Led_1_Off(void)
{
	Set_TIM3_Ch3_Disable();
}

void Led_2_Brighness(u16 puty)   //PB1  CH4
{
	Set_TIM3_Ch4_Enable(LED_ARR,  puty);
}

void Led_2_Off(void)
{
	Set_TIM3_Ch4_Disable();
}








