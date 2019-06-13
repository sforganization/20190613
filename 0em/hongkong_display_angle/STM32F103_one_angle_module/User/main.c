
#include <string.h>

#include <stdio.h>

#include "delay.h"
#include "sys.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"

#include "timer.h"
#include "pwm.h"

//#include "pwm.h"
#include "ws2812b.h"
#include "wdg.h"


#define _MAININC_
#include "SysComment.h"
#undef _MAININC_
#define usart2_baund			57600//串口2波特率，根据指纹模块波特率更改（注意：指纹模块默认57600）


void SysTickTask(void);


void DebugGPIOInit(void)
{
	/* 定义IO硬件初始化结构体变量 */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能(开启)KEY1引脚对应IO端口时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 需要输出 5 伏的高电平，就可以在外部接一个上拉电阻，上拉电源为 5 伏，并且把 GPIO 设置为开漏模式，当输出高阻态时，由上拉电阻和电源向外输出 5伏的电平

	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_Pin_13);

    delay_ms(3);
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}


/*******************************************************************************
* 名称: 
* 功能: 控制另一个单片机上电掉电\上电
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void LockGPIOInit(void)
{
	/* 定义IO硬件初始化结构体变量 */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能(开启)KEY1引脚对应IO端口时钟 */
	LOCK_RCC_CLOCKCMD(LOCK_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LOCK_01_PIN;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 需要输出 5 伏的高电平，就可以在外部接一个上拉电阻，上拉电源为 5 伏，并且把 GPIO 设置为开漏模式，当输出高阻态时，由上拉电阻和电源向外输出 5伏的电平

	GPIO_Init(LOCK_GPIO, &GPIO_InitStructure);

	GPIO_SetBits(LOCK_GPIO, LOCK_01_PIN);
}


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void LEDGPIOInit(void)
{
	/* 定义IO硬件初始化结构体变量 */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能(开启)KEY1引脚对应IO端口时钟 */
	LED_RCC_CLOCKCMD(LED_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LED_PIN_SINGLE | LED_PIN_STREAM;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 需要输出 5 伏的高电平，就可以在外部接一个上拉电阻，上拉电源为 5 伏，并且把 GPIO 设置为开漏模式，当输出高阻态时，由上拉电阻和电源向外输出 5伏的电平

	GPIO_Init(LED_GPIO, &GPIO_InitStructure);

	GPIO_ResetBits(LED_GPIO, LED_PIN_SINGLE);
	GPIO_ResetBits(LED_GPIO, LED_PIN_STREAM);
}


/*******************************************************************************
* 名称: 
* 功能: 零点检测io
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_ZeroGPIOInit(void)
{
	/* 定义IO硬件初始化结构体变量 */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能(开启)KEY1引脚对应IO端口时钟 */
	LOCK_ZERO_RCC_CLOCKCMD(LOCK_ZERO_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = LOCK_ZERO_01_PIN;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉输入

	GPIO_Init(KEY_GPIO, &GPIO_InitStructure);
}


/*******************************************************************************
* 名称: 
* 功能: 按键
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void KeyGPIOInit(void)
{
	/* 定义IO硬件初始化结构体变量 */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能(开启)KEY1引脚对应IO端口时钟 */
	LOCK_ZERO_RCC_CLOCKCMD(KEY_RCC_CLOCKGPIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = KEY_01_PIN;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉输入

	GPIO_Init(LOCK_ZERO_GPIO, &GPIO_InitStructure);
}


/*******************************************************************************
* 名称: 
* 功能: 方向控制io  一个正一个负，控制旋转方向, 使用PWM控制 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Moto_DirGPIOInit(void)
{
	//	/* 定义IO硬件初始化结构体变量 */
	//	GPIO_InitTypeDef GPIO_InitStructure;
	//	/* 使能(开启)引脚对应IO端口时钟 						+ */
	//	MOTO_P_RCC_CLOCKCMD(MOTO_P_RCC_CLOCKGPIO, ENABLE);
	//	GPIO_InitStructure.GPIO_Pin =	MOTO_01_PIN_P;
	//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//
	//	GPIO_Init(MOTO_P_GPIO, &GPIO_InitStructure);
	//	/* 使能(开启)引脚对应IO端口时钟 					-*/
	//	MOTO_N_RCC_CLOCKCMD(MOTO_N_RCC_CLOCKGPIO, ENABLE);
	//	  
	//	GPIO_InitStructure.GPIO_Pin =	MOTO_01_PIN_N;
	//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//
	//	GPIO_Init(MOTO_N_GPIO, &GPIO_InitStructure);
}


/*******************************************************************************
* 名称: 
* 功能: debug
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void Debug_GPIOInit(void)
{
	/* 定义IO硬件初始化结构体变量 */
	GPIO_InitTypeDef GPIO_InitStructure;

	/* 使能(开启)KEY1引脚对应IO端口时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉输入

	GPIO_Init(GPIOC, &GPIO_InitStructure);
}


int main(void)
{
	delay_init();									//延时函数初始化	  
	NVIC_Configuration();							//设置NVIC中断分组2:2位抢占优先级，2位响应优先级 整个程序只设置一次;				 
	usart2_init(19200); 							//初始化串口2,用于与指纹模块通讯 ，里面会初始化一个定时器 作为接收数据 的判断 
	KeyGPIOInit();
	LEDGPIOInit();
	LockGPIOInit();
	Moto_ZeroGPIOInit();
	Moto_DirGPIOInit();

	FingerGPIO_Init();								//初始化FR读状态引脚  识别到指纹按压输出高电平，因为复用关系初始化动作放到最后面

	usart3_init(115200); //debug 用

	//读摸出感应状态(触摸感应时输出高电平信号)
#if DEBUG
	DebugGPIOInit();
	u3_printf("----------------start--------------\n");
#endif

	San_PWM_Init(PWM_PERIOD, PWM_PRESCALER);		//200hz
	ws2812bInit();									//里面会开一个定时器1作为pwm输出 
	IWDG_Init(4, 3750);								//分频系数为64，重载值为625，溢出时间为1S                      3750    6S
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


/*******************************************************************************
* 名称: 
* 功能: 
* 形参:		
* 返回: 无
* 说明: 
*******************************************************************************/
void SysTickTask(void)
{
	vu16 static 	u16SecTick = 0; 				//秒计数
	u8				i;

	if (u32SysSoftDog++ > 200)  //喂狗
	{
		u32SysSoftDog		= 0;
		IWDG_Feed();
	}

	SysTask.u32SysTimes++;							//系统运行时间

	if (u16SecTick++ >= 1000) //秒任务
	{
		u16SecTick			= 0;

		if (SysTask.u16SaveTick)
		{
			SysTask.u16SaveTick--;

			if (SysTask.u16SaveTick == 0)
				SysTask.bEnSaveData = TRUE; //开启保存
		}
	}

	if (SysTask.u16SaveWaitTick)
		SysTask.u16SaveWaitTick--;

	if (SysTask.u32TouchWaitTime)
		SysTask.u32TouchWaitTime--;

	if (SysTask.u32StreamTime)
		SysTask.u32StreamTime--;

	if (SysTask.KeyStateTime)
		SysTask.KeyStateTime--;



	for (i = 0; i < GLASS_MAX_CNT; i++)
	{
		if (SysTask.Moto_StateTime[i])
			SysTask.Moto_StateTime[i] --;

		if (SysTask.Moto_RunTime[i])
			SysTask.Moto_RunTime[i] --;

		if (SysTask.Moto_WaitTime[i])
			SysTask.Moto_WaitTime[i] --;

		if (SysTask.Lock_StateTime[i])
			SysTask.Lock_StateTime[i] --;

		if (SysTask.Lock_OffTime[i])
			SysTask.Lock_OffTime[i] --;
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


