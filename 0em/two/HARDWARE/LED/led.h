#ifndef __LED_H
#define __LED_H
#include "sys.h"



#define LED_ARR               (1000 - 1)              //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ 
#define LED_PSC               (24 - 1)                //Ԥ��Ƶֵ     ��TIMxʱ��Ƶ��Ϊ72M / (PWM_PRESCALER + 1) = 4000K, pwmƵ��Ϊ  72M / (PWM_PRESCALER + 1) / ��PWM_PERIOD + 1�� = hz

#define LED_BRIGHNESS         (LED_ARR + 1)

void Led_Init(void);
void Led_1_Brighness(u16 puty);//PB0  CH3
void Led_1_Off(void);

void Led_2_Brighness(u16 puty);   //PB1  CH4
void Led_2_Off(void);

#endif























