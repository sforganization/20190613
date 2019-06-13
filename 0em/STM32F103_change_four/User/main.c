	 
#include <string.h>

#include "delay.h"
#include "sys.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "timer.h"
#include "wdg.h"
#include "step_driver.h"


#define _MAININC_
#include "SysComment.h"
#undef _MAININC_

void SysTickTask(void);


/*******************************************************************************
* ����: 
* ����: ������һ����Ƭ���ϵ����\�ϵ�  �͵�ƽ�������ߵ�ƽ����
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void LockGPIOInit(void)
{
	/* ����IOӲ����ʼ���ṹ����� */
	GPIO_InitTypeDef GPIO_InitStructure;

	LOCK_RCC_CLOCKCMD(LOCK_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin =  LOCK_01_PIN | LOCK_02_PIN | LOCK_03_PIN | LOCK_04_PIN;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	// ��Ҫ��� 5 ���ĸߵ�ƽ���Ϳ������ⲿ��һ���������裬������ԴΪ 5 �������Ұ� GPIO ����Ϊ��©ģʽ�����������̬ʱ������������͵�Դ������� 5���ĵ�ƽ

	GPIO_Init(LOCK_GPIO, &GPIO_InitStructure);

	GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
	GPIO_SetBits(LOCK_GPIO, LOCK_02_PIN);
	GPIO_SetBits(LOCK_GPIO, LOCK_03_PIN);  
	GPIO_SetBits(LOCK_GPIO, LOCK_04_PIN); 
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
    
	GPIO_InitStructure.GPIO_Pin =   LOCK_ZERO_STEP_MOTO_PIN;

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

//	GPIO_InitStructure.GPIO_Pin =   MOTO_DOOR_L_PIN_P | MOTO_DOOR_R_PIN_P;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//

//	GPIO_Init(MOTO_P_GPIO, &GPIO_InitStructure);

//	/* ʹ��(����)���Ŷ�ӦIO�˿�ʱ��                     -*/
//	MOTO_N_RCC_CLOCKCMD(MOTO_N_RCC_CLOCKGPIO, ENABLE);
//    
//	GPIO_InitStructure.GPIO_Pin =    MOTO_DOOR_L_PIN_N | MOTO_DOOR_R_PIN_N;

//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//

//	GPIO_Init(MOTO_N_GPIO, &GPIO_InitStructure);
//    
//	GPIO_ResetBits(MOTO_P_GPIO, MOTO_DOOR_L_PIN_P); //ֹͣ 
//	GPIO_ResetBits(MOTO_N_GPIO, MOTO_DOOR_R_PIN_P); //
//	
//	GPIO_ResetBits(MOTO_P_GPIO, MOTO_DOOR_L_PIN_N); //ֹͣ 
//	GPIO_ResetBits(MOTO_N_GPIO, MOTO_DOOR_R_PIN_N); //
	
//	GPIO_ResetBits(MOTO_P_GPIO, MOTO_RISE_PIN_P); //ֹͣ    ������� ʹ��TIM4 PWM pb6 pb7
//	GPIO_ResetBits(MOTO_N_GPIO, MOTO_RISE_PIN_N); //  
}


/*******************************************************************************
* ����: 
* ����: 
* �β�:		
* ����: ��
* ˵��: 
*******************************************************************************/
void SensorGPIOInit(void)
{
	/* ����IOӲ����ʼ���ṹ����� */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* ʹ��(����)���Ŷ�ӦIO�˿�ʱ��               ��Ϊ PB3 PB4 ��JTAG�� ����Ҫ������ʱ��             + */
    SENSOR_RCC_CLOCKCMD(SENSOR_RCC_CLOCKGPIO|RCC_APB2Periph_AFIO,ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_InitStructure.GPIO_Pin =   SENSOR_DOOR_PIN_L | SENSOR_DOOR_PIN_R | SENSOR_RISE_PIN_U | SENSOR_RISE_PIN_D;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//

	GPIO_Init(SENSOR_GPIO, &GPIO_InitStructure);
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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //

	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

//maxоƬ���� �ο� ���ʹ�õ���max3232 sp3232
//���������ſۡ������ˣ������ǵ�Դ��IO�ĵ�ƽ����ʱ�䲻ͬ�����£�һ��Ҫ��֤�ȼ�VCC��������룬�������ٱ�֤����ͬʱ�������ɵ�����������Ϳ��ܷ����ſ�����
// tim4 moto pwm  PB8 dir  PB9 TIM4 CH4     PB6 CH1  PB7 CH2
// tim2 tick
int main(void)
{
	delay_init();									//��ʱ������ʼ��	  
	NVIC_Configuration();							//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ� ��������ֻ����һ��;				 
	LockGPIOInit();
	Moto_ZeroGPIOInit();
	Moto_DirGPIOInit();   
    CenterMotoInit(0xffff, (72000000 / TIMER_FRE) - 1);   //ϵͳʱ��72M 
    usart1_init(115200);            //��ƽ��ͨ��
    
//    Usart1_SendByte(0x88);
#if DEBUG || DEBUG_MOTO2
    usart3_init(115200); //��ʱ����
    Debug_GPIOInit();
#endif
    SensorGPIOInit();  // �����õ�PB3 PB4  ���� PA15 Ҫ������ʼ��
//  Usart3_SendByte(0x66);
//	IWDG_Init(4, 3750);								//��Ƶϵ��Ϊ64������ֵΪ625�����ʱ��Ϊ1S                      3750    6S
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

    static int state = 0;

	SysTask.u32SysTimes++;							//ϵͳ����ʱ��

	if (u32SysSoftDog++ > 200)  //ι��
	{
		u32SysSoftDog		= 0;
		IWDG_Feed();
	}
    
    if(SysTask.u8Usart1RevTime){
        SysTask.u8Usart1RevTime--;
        if(SysTask.u8Usart1RevTime == 0){
            USART1_RX_STA		|= 1 << 15; 			//��ǽ������
        }
    }
    
    if (SysTask.u16InitWaitTime)
        {
        SysTask.u16InitWaitTime--;
    }

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

    

			if (state)
			{
				state				= 0;
				GPIO_ResetBits(GPIOC, GPIO_Pin_13);
			}
			else 
			{
				state				= 1;
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
			}
}


