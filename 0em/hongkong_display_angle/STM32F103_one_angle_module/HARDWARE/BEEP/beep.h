#ifndef __BEEP_H__
#define __BEEP_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include <stm32f10x.h>

/* 类型定义 ------------------------------------------------------------------*/
typedef enum
{
  BEEPState_OFF = 0,
  BEEPState_ON,
}BEEPState_TypeDef;
#define IS_BEEP_STATE(STATE)           (((STATE) == BEEPState_OFF) || ((STATE) == BEEPState_ON))

/* 宏定义 --------------------------------------------------------------------*/
/*
 * 以下宏定义内容跟开发板硬件息息相关，需要查看开发板电路原理图才能正确编写。
 * 例如，查原理图可在蜂鸣器接在stm32f103芯片的PB0引脚上，所以有关蜂鸣器引脚的
 * 宏定义都是与GPIOB，GPIO_Pin_0相关的，我们专门把这些与开发板硬件相关的内容定
 * 义为宏，这对于修改或者移植程序非常方便。
 */
#define BEEP_RCC_CLOCKCMD             RCC_APB2PeriphClockCmd
#define BEEP_RCC_CLOCKGPIO            RCC_APB2Periph_GPIOB
#define BEEP_GPIO_PIN                 GPIO_Pin_14
#define BEEP_GPIO                     GPIOB

#if 0         /* 调用标准库函数方法 */

#define BEEP_ON                       GPIO_SetBits(BEEP_GPIO,BEEP_GPIO_PIN)
#define BEEP_OFF                      GPIO_ResetBits(BEEP_GPIO,BEEP_GPIO_PIN)
#define BEEP_TOGGLE                   {BEEP_GPIO->ODR ^=BEEP_GPIO_PIN;}  //输出反转

#else       /* 直接操作寄存器方法 */

#define BEEP_ON                       {BEEP_GPIO->BSRR=BEEP_GPIO_PIN;}    //输出高电平
#define BEEP_OFF                      {BEEP_GPIO->BRR=BEEP_GPIO_PIN;}   //输出低电平
#define BEEP_TOGGLE                   {BEEP_GPIO->ODR ^=BEEP_GPIO_PIN;}  //输出反转

#endif

/* 扩展变量 ------------------------------------------------------------------*/
/* 函数声明 ------------------------------------------------------------------*/
void BEEP_GPIO_Init(void);
void BEEP_StateSet(BEEPState_TypeDef state);
  
#endif  // __BSP_BEEP_H__

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
