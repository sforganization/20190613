#ifndef __STEP_DRIVER_H
#define __STEP_DRIVER_H	 
#include "sys.h"  




/* S型加速参数 */

//驱动器设置6400 个脉冲一圉
//1600 为1/4圈


#define CIRCLE_STEP_PULSE           6400 //一圈的步数


#define ACCELERATED_SPEED_LENGTH    (CIRCLE_STEP_PULSE / 4 / 2) //800 //定义加速度的点数（其实也是800个细分步的意思），调这个参数改变加速点
#define CIRCLE_STEP_LENGTH          (CIRCLE_STEP_PULSE - ACCELERATED_SPEED_LENGTH * 2)  //一圈的要匀速走的步数

#define FRE_MIN 				    100  //最低的运行频率，调这个参数调节最低运行速度
#define FRE_MAX 				    2500 //最高的运行频率，调这个参数调节匀速时的最高速度3000
#define FIEXIBLE                   6    //flexible: flexible value. adjust the S curves, 数值越大S曲线起始越慢,起始频率越低
#define TIMER_FRE                   800000//定时器计数频率800K

#define MOTO_ARR                    39   // TIMER_FRE / （39 + 1） = 20Khz 
#define MOTO_PUTY_10                (MOTO_ARR * 10 / 100)     
#define MOTO_PUTY_30                (MOTO_ARR * 30 / 100)   
#define MOTO_PUTY_40                (MOTO_ARR * 40 / 100)     
#define MOTO_PUTY_50                (MOTO_ARR * 50 / 100)     
#define MOTO_PUTY_60                (MOTO_ARR * 60 / 100)     
#define MOTO_PUTY_70                (MOTO_ARR * 70 / 100)     
#define MOTO_PUTY_80                (MOTO_ARR * 80 / 100)   
#define MOTO_PUTY_90                (MOTO_ARR * 90 / 100)   
#define MOTO_PUTY_99                (MOTO_ARR * 99 / 100)   



#define MOTO_PUTY_65                (MOTO_ARR * 65 / 100)      


#define MOTO_PUTY                   MOTO_PUTY_70
#define MOTO_PUTY_B                 MOTO_PUTY_65



void MotoPWMInit(u16 arr,u16 psc);

void Moto_1_Start_CCW(u16 puty);   //正转
void Moto_1_Start_CW(u16 puty);           //反转
void Moto_1_Stop(void);

void Moto_2_Start_CCW(u16 puty);
void Moto_2_Start_CW(u16 puty);
void Moto_2_Stop(void);

void Moto_3_Start_CCW(u16 puty);
void Moto_3_Start_CW(u16 puty);
void Moto_3_Stop(void);

void Moto_4_Start_CCW(u16 puty);
void Moto_4_Start_CW(u16 puty);
void Moto_4_Stop(void);

#endif













