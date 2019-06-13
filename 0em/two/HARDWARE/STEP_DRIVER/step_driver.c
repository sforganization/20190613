
#include "step_driver.h"
#include "SysComment.h"
#include <math.h>

static vs32 	g_stepRun = 4800; //要匀速运行的步数	  

float			g_fre[ACCELERATED_SPEED_LENGTH]; //数组存储加速过程中每一步的频率 
unsigned short	g_period[ACCELERATED_SPEED_LENGTH]; //数组储存加速过程中每一步定时器的自动装载值	 

static volatile bool g_Ch1Enable = FALSE; //
static volatile bool g_Ch2Enable = FALSE; //
static volatile bool g_Ch3Enable = FALSE; //
static volatile bool g_Ch4Enable = FALSE; //

static volatile s32 g_acce_num_config = 0; //配置加速的步数

static volatile s32 g_acce_num = 0; //加速的步数 
static volatile u8 g_remote = 0; //远端加速过来到零点


/**
  * 函数功能: 通用定时器 TIMx,x[2,3,4,5]中断优先级配置
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明：无
  */
void TIM4_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* 设置中断组为0 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	/* 设置中断来源 */
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;

	/* 设置主优先级为 0 */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;

	/* 设置抢占优先级为3 */
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;

	/*定时器使能 */
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
* flexible: flexible value. adjust the S curves, 数值越大曲线起始越慢
*/
//static void CalculateSModelLine(float fre[], unsigned short period[], float len, float fre_max, float fre_min, 
//	float flexible)
//{
//	int 			i	= 0;
//	float			deno;
//	float			melo;
//	float			delt = fre_max - fre_min;

//	for (; i < len; i++)
//	{
//		melo				= flexible * (i - len / 2) / (len / 2);
//		deno				= 1.0f / (1 + expf(-melo)); //expf is a library function of exponential(e) 
//		fre[i]				= delt * deno + fre_min;
//		period[i]			= (unsigned short) ((float) TIMER_FRE / fre[i]); // TIMER_FRE is the timer driver frequency
//	}

//	return;
//}

//使用H桥
//    电机正转：DIR=1 PWM=PWM  
//    电机反转：DIR=0 PWM=PWM
//    停车刹车：DIR=X PWM=0(X为任意状态)
//    可以工作在0％-99％的PWM调制占空比，使电机可以得到足够的驱动电压
static void Set_TIM4_Ch1_Enable(u16 arr, u16 puty)
{
    g_Ch1Enable     = TRUE;
	TIM_SetAutoreload(TIM4, arr);
	TIM_SetCompare1(TIM4, puty);
}

static void Set_TIM4_Ch1_Disable(void)
{
    // 关闭通道
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
	// 关闭通道
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
	// 关闭通道
	g_Ch3Enable		= FALSE;
	TIM_SetCompare3(TIM4, 0);
}


static void Set_TIM4_Ch4_Enable(u16 arr, u16 puty)
{
	g_Ch4Enable		= TRUE;
	TIM_SetAutoreload(TIM4, arr);
	TIM_SetCompare4(TIM4, puty);
}


static void Set_TIM4_Ch4_Disable(void)
{
	// 关闭通道
	g_Ch4Enable		= FALSE;
	TIM_SetCompare4(TIM4, 0);
}

void TIM4_IRQHandler(void)
{

	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) //用PWM模式，可以省下 I 这个变量
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}


//使用定时器4		  ch4 pwm作为步进电机控制  PB9
void MotoPWMInit(u16 arr, u16 psc)
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

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0x0A;		//为了进中断，一设置等待10ms就进入
	TIM_TimeBaseStructure.TIM_Prescaler = psc;		//定时器频率计数，16位定时器，0xffff
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;	//0
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;				//
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //must
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);		//ch1 ch2是控制开门电机的   ch3 是升降电机
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);

    
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


//正转
void Moto_1_Start_CCW(u16 puty)
{
	Set_TIM4_Ch1_Enable(MOTO_ARR,  puty);
	Set_TIM4_Ch2_Disable();
}

//反转
void Moto_1_Start_CW(u16 puty)
{
	Set_TIM4_Ch2_Enable(MOTO_ARR,  puty);
	Set_TIM4_Ch1_Disable();
}

//停止
void Moto_1_Stop(void)
{
	Set_TIM4_Ch1_Disable();
	Set_TIM4_Ch2_Disable();
}

//正转
void Moto_2_Start_CCW(u16 puty)
{
	Set_TIM4_Ch3_Enable(MOTO_ARR,  puty);
	Set_TIM4_Ch4_Disable();
}

//反转
void Moto_2_Start_CW(u16 puty)
{
	Set_TIM4_Ch4_Enable(MOTO_ARR,  puty);
	Set_TIM4_Ch3_Disable();
}

//停止
void Moto_2_Stop(void)
{
	Set_TIM4_Ch3_Disable();
	Set_TIM4_Ch4_Disable();
}






