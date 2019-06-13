/**

  ******************************************************************************
  */
/* 包含头文件 ----------------------------------------------------------------*/
#include "beep.h"

/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
/* 私有变量 ------------------------------------------------------------------*/
/* 扩展变量 ------------------------------------------------------------------*/
/* 私有函数原形 --------------------------------------------------------------*/
/* 函数体 --------------------------------------------------------------------*/

/**
  * 函数功能: 板载蜂鸣器IO引脚初始化.
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明：使用宏定义方法代替具体引脚号，方便程序移植，只要简单修改bsp_beep.h
  *           文件相关宏定义就可以方便修改引脚。
  */
void BEEP_GPIO_Init(void)
{
   /* 定义IO硬件初始化结构体变量 */
  GPIO_InitTypeDef GPIO_InitStructure;
	
	/* 使能(开启)蜂鸣器引脚对应IO端口时钟 */  
  BEEP_RCC_CLOCKCMD(BEEP_RCC_CLOCKGPIO, ENABLE);

  /* 设定蜂鸣器对应引脚IO编号 */
  GPIO_InitStructure.GPIO_Pin = BEEP_GPIO_PIN;  
  /* 设定蜂鸣器对应引脚IO最大操作速度 ：GPIO_Speed_50MHz */
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
  /* 设定蜂鸣器对应引脚IO为输出模式 */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
  /* 初始化蜂鸣器对应引脚IO */
  GPIO_Init(BEEP_GPIO, &GPIO_InitStructure);
 
  /* 设置引脚输出为低电平，此时蜂鸣器不响 */
  GPIO_ResetBits(BEEP_GPIO,BEEP_GPIO_PIN);  
}

/**
  * 函数功能: 设置板载蜂鸣器的状态
  * 输入参数：state:设置蜂鸣器的状态。
  *             可选值：BEEPState_OFF：蜂鸣器不响；
  *             可选值：BEEPState_ON： 蜂鸣器响。
  * 返 回 值: 无
  * 说    明：该函数使用类似标准库函数的编程方法，方便理解标准库函数编程思想。
  */
void BEEP_StateSet(BEEPState_TypeDef state)
{
  /* 检查输入参数是否合法 */
  assert_param(BEEPState_TypeDef(state));
  
  /* 判断设置的蜂鸣器状态，如果设置为蜂鸣器响 */
  if(state==BEEPState_ON)
  {
    /* 设置引脚输出为高电平，此时蜂鸣器响 */
    GPIO_SetBits(BEEP_GPIO,BEEP_GPIO_PIN);  
  }
  else  /* state=BEEPState_OFF：设置蜂鸣器不响 */
  {
    /* 设置引脚输出为低电平，此时蜂鸣器不响 */
    GPIO_ResetBits(BEEP_GPIO,BEEP_GPIO_PIN);   
  }
}
/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
