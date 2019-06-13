	 
#include <string.h>

#include "delay.h"
#include "sys.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "as608.h"

#include "timer.h"
#include "led.h"

//#include "pwm.h"
#include "step_driver.h"
#include "range_sensor.h"

#define _MAININC_
#include "SysComment.h"
#undef _MAININC_
#define usart2_baund			57600//����2�����ʣ�����ָ��ģ�鲨���ʸ��ģ�ע�⣺ָ��ģ��Ĭ��57600��

void SysTickTask(void);


/*******************************************************************************
* ����: 
* ����: ������һ����Ƭ���ϵ����\�ϵ�
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void LockGPIOInit(void)
{
	/* ����IOӲ����ʼ���ṹ����� */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* ʹ��(����)KEY1���Ŷ�ӦIO�˿�ʱ�� */
	LOCK_RCC_CLOCKCMD(LOCK_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin =  LOCK_01_PIN | LOCK_02_PIN;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	// ��Ҫ��� 5 ���ĸߵ�ƽ���Ϳ������ⲿ��һ���������裬������ԴΪ 5 �������Ұ� GPIO ����Ϊ��©ģʽ�����������̬ʱ������������͵�Դ������� 5���ĵ�ƽ

	GPIO_Init(LOCK_GPIO, &GPIO_InitStructure);

	GPIO_ResetBits(LOCK_GPIO, LOCK_01_PIN);
	GPIO_ResetBits(LOCK_GPIO, LOCK_02_PIN);
}

/*******************************************************************************
* ����: 
* ����: �����io
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Moto_ZeroGPIOInit(void)
{
	/* ����IOӲ����ʼ���ṹ����� */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* ʹ��(����)KEY1���Ŷ�ӦIO�˿�ʱ�� */
	LOCK_ZERO_RCC_CLOCKCMD(LOCK_ZERO_RCC_CLOCKGPIO, ENABLE);
    
	GPIO_InitStructure.GPIO_Pin =   LOCK_ZERO_01_PIN | LOCK_ZERO_02_PIN;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	 //��������

	GPIO_Init(LOCK_ZERO_GPIO, &GPIO_InitStructure);
}

/*******************************************************************************
* ����: 
* ����: �������io  һ����һ������������ת����
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Moto_DirGPIOInit(void)
{
//	/* ����IOӲ����ʼ���ṹ����� */
//	GPIO_InitTypeDef GPIO_InitStructure;

//	/* ʹ��(����)���Ŷ�ӦIO�˿�ʱ��                         + */
//	MOTO_P_RCC_CLOCKCMD(MOTO_P_RCC_CLOCKGPIO, ENABLE);

//	GPIO_InitStructure.GPIO_Pin =   MOTO_01_PIN_P | MOTO_02_PIN_P;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//

//	GPIO_Init(MOTO_P_GPIO, &GPIO_InitStructure);

//	/* ʹ��(����)���Ŷ�ӦIO�˿�ʱ��                     -*/
//	MOTO_N_RCC_CLOCKCMD(MOTO_N_RCC_CLOCKGPIO, ENABLE);
//    
//	GPIO_InitStructure.GPIO_Pin =   MOTO_01_PIN_N | MOTO_02_PIN_N;

//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//

//	GPIO_Init(MOTO_N_GPIO, &GPIO_InitStructure);
}


/*******************************************************************************
* ����: 
* ����: debug
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void Debug_GPIOInit(void)
{
	/* ����IOӲ����ʼ���ṹ����� */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* ʹ��(����)KEY1���Ŷ�ӦIO�˿�ʱ�� */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
	GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_13;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	 //��������

	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

//maxоƬ���� �ο�
//���������ſۡ������ˣ������ǵ�Դ��IO�ĵ�ƽ����ʱ�䲻ͬ�����£�һ��Ҫ��֤�ȼ�VCC��������룬�������ٱ�֤����ͬʱ�������ɵ�����������Ϳ��ܷ����ſ�����
// tim4 moto pwm     PB6 CH1  PB7 CH2 PB8 CH4 PB9 CH4  
// tim2 tick
// tim3 LED PWM
// USART1 ƽ��

int main(void)
{
	delay_init();									//��ʱ������ʼ��	  
	NVIC_Configuration();							//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ� ��������ֻ����һ��;				 
	
    RangeSensor_Init();
	LockGPIOInit();
	Moto_ZeroGPIOInit();
    Led_Init();
    //ʹ����·PWM���� ����ʱ�ò���
	//Moto_DirGPIOInit();   
    MotoPWMInit(0xffff, (72000000 / TIMER_FRE) - 1);   //ϵͳʱ��72M  MOTO pwm TIM4 
    RangeSensor_Init();
    usart1_init(115200);
    
    //  Usart1_SendByte(0x88);
#if DEBUG || DEBUG_MOTO2
    usart3_init(115200); //��ʱ����
    Debug_GPIOInit();
#endif
    //  Usart3_SendByte(0x66);
    //	usart2_init(usart2_baund);						//��ʼ������2,������ָ��ģ��ͨѶ
	
    SysInit();
	GENERAL_TIMx_Configuration();

	while (1)
	{
		MainTask();

		if (SysTask.nTick)
		{
			SysTask.nTick--;
			SysTickTask();
		}
	}
}

extern vu16 	USART1_RX_STA;
extern vu16 	USART2_RX_STA;
extern vu16 	USART3_RX_STA;

/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SysTickTask(void)
{
	vu16 static 	u16SecTick = 0; 				//�����
	u8  i;

	SysTask.u32SysTimes++;							//ϵͳ����ʱ��

	if (u16SecTick++ >= 1000) //������
	{
		u16SecTick			= 0;
        
       if (SysTask.u16SaveTick)
        {   
    		SysTask.u16SaveTick--;
            if(SysTask.u16SaveTick == 0)
                SysTask.bEnSaveData = TRUE;  //��������
        }
	}

    if(SysTask.u8Usart1RevTime){
        SysTask.u8Usart1RevTime--;
        if(SysTask.u8Usart1RevTime == 0){
            USART1_RX_STA		|= 1 << 15; 			//��ǽ������
        }
    }
    if(SysTask.LED_StateTime)
        SysTask.LED_StateTime--;

    if(SysTask.Range_StateTime)
        SysTask.Range_StateTime--;

    if(SysTask.Range_WaitTime)
        SysTask.Range_WaitTime--;

    if (SysTask.u16PWMVal == PWM_PERIOD){
        SysTask.u16PWMVal = 0;
    }
    
	if (SysTask.nWaitTime)
	{
		SysTask.nWaitTime--;
	}
    
    if(SysTask.u16SaveWaitTick)
        SysTask.u16SaveWaitTick--;
    if(SysTask.Moto3_SubStateTime)
        SysTask.Moto3_SubStateTime--;

    for(i = 0; i < GLASS_MAX_CNT; i++)
    {
        if(SysTask.Moto_StateTime[i])   SysTask.Moto_StateTime[i]--;
        if(SysTask.Moto_RunTime[i])     SysTask.Moto_RunTime[i]--;
        if(SysTask.Moto_WaitTime[i])    SysTask.Moto_WaitTime[i]--;
        if(SysTask.Lock_StateTime[i])   SysTask.Lock_StateTime[i]--;
        if(SysTask.Lock_OffTime[i])     SysTask.Lock_OffTime[i]--;
        if(SysTask.PWM_RunTime[i])     SysTask.PWM_RunTime[i]--;
    }

    

	//			if (state)
	//			{
	//				state				= 0;
	//				GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	//			}
	//			else 
	//			{
	//				state				= 1;
	//				GPIO_SetBits(GPIOC, GPIO_Pin_13);
	//			}
}





