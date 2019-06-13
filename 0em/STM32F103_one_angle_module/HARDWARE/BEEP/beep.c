#include "beep.h"
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "timer.h"

u8 BGM = 0;
u8 BGM_LENGTH[6] = {0,5,4,36,12,33};  //len 为共几个音节
u8 BGM_change_flg = 0;
u8 BGM_volum; 

u16 CL[7]={262,294,330,349,392,440,494};
u16 CM[7]={523,587,659,698,784,880,988};
u16 CH[7]={1047,1175,1319,1397,1568,1760,1976};
u16 DL[7]={294,330,370,392,440,494,554};
u16 DM[7]={587,659,740,784,880,988,1109};
u16 DH[7]={1175,1319,1480,1568,1760,1976,2217};
u16 EL[7]={330,370,415,440,494,554,622};
u16 EM[7]={659,740,831,880,988,1109,1245};
u16 EH[7]={1319,1480,1661,1760,1976,1,1};//除数不能为0，用1代替
u16 FL[7]={349,392,440,466,523,587,659};
u16 FM[7]={698,784,880,932,1047,1175,1319};
u16 FH[7]={1397,1568,1760,1865,1,1,1};

#define TWO_TIGER_LENGTH  36

	
//定时器3中断服务程序
void TIM4_IRQHandler(void)   //TIM3中断
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
	{
	    TIM_ClearITPendingBit(TIM4, TIM_IT_Update  );  //清除TIMx的中断待处理位:TIM 中断源 		//LED1=!LED1;  
	}
}


//使用定时器4        ch3 pwm作为音乐频率的发生器，占空比可以调节音量大小
void BeepInit(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);// 
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);  //
	                                                                     	
	GPIO_InitStructure.GPIO_Pin   =  GPIO_Pin_9; 
	GPIO_InitStructure.GPIO_Mode  =  GPIO_Mode_AF_PP;  //
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	TIM_TimeBaseStructure.TIM_Period = arr;     //
	TIM_TimeBaseStructure.TIM_Prescaler =psc;    //
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //
 
 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //
	TIM_OCInitStructure.TIM_Pulse = 0; //
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);  //
 
    TIM_CtrlPWMOutputs(TIM4,ENABLE);	//
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);  //
	TIM_ARRPreloadConfig(TIM4, ENABLE); //
	TIM_Cmd(TIM4, ENABLE);  //
}

//蜂鸣器发出声音 
//usFreq即发声频率，音调,volume_level是音量等级  共1000级
void BeepSound(unsigned short usFraq,unsigned int volume_level)   //usFraq是发声频率，即真实世界一个音调的频率。
{
	unsigned long Autoreload;
	if((usFraq<=122)||(usFraq>20000))
	{
		BeepQuiet();
	}
	else
	{
		Autoreload=(8000000/usFraq)-1;  //频率变为自动重装值
		TIM_SetAutoreload(TIM4,Autoreload);
		TIM_SetCompare4(TIM4,Autoreload * volume_level / 1000 );   //音量  
	}
}

//蜂鸣器停止发声
void BeepQuiet(void)
{
	TIM_SetCompare4(TIM4,0);
}


