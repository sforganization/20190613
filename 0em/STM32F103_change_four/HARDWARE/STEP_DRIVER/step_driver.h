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
#define TIMER_FRE                   80000//定时器计数频率80K

#define MOTO_ARR                    39   // 80000 / （39 + 1） = 2Khz
#define MOTO_PUTY_99                (MOTO_ARR * 99 / 100)     
#define MOTO_PUTY_10                (MOTO_ARR * 10 / 100)     
#define MOTO_PUTY_30                (MOTO_ARR * 30 / 100)   
#define MOTO_PUTY_40                (MOTO_ARR * 40 / 100)     

#define DEC_STEP_NUM               100  //找到零点，往前走几步
#define DEBOUNCH_STEP_NUM          5    //从检测到零点再过几步到正中心位置

#define STEP_DIR_RCC_CLOCKCMD       RCC_APB2PeriphClockCmd
#define STEP_DIR_RCC_CLOCKGPIO      RCC_APB2Periph_GPIOD
#define STEP_DIR_GPIO               GPIOD

#define STEP_DOOR_DIR_PIN_L         GPIO_Pin_0    //
#define STEP_DOOR_DIR_PIN_R         GPIO_Pin_1    //
#define STEP_RISE_DIR_PIN           GPIO_Pin_2    //
#define STEP_CENTER_DIR_PIN         GPIO_Pin_3    //

//默认DIR + 接了3.3v, 要光耦导通才会反转，dir- 接0
#define SET_DOOR_L_DIR_P()              GPIO_SetBits(STEP_DIR_GPIO, STEP_DOOR_DIR_PIN_L);
#define SET_DOOR_L_DIR_N()              GPIO_ResetBits(STEP_DIR_GPIO, STEP_DOOR_DIR_PIN_L);
#define SET_DOOR_R_DIR_P()              GPIO_SetBits(STEP_DIR_GPIO, STEP_DOOR_DIR_PIN_R);
#define SET_DOOR_R_DIR_N()              GPIO_ResetBits(STEP_DIR_GPIO, STEP_DOOR_DIR_PIN_R);
#define SET_RISE_STEP_DIR_P()           GPIO_SetBits(STEP_DIR_GPIO, STEP_RISE_DIR_PIN);
#define SET_RISE_STEP_DIR_N()           GPIO_ResetBits(STEP_DIR_GPIO, STEP_RISE_DIR_PIN);
#define SET_CENTER_STEP_DIR_P()         GPIO_SetBits(STEP_DIR_GPIO, STEP_CENTER_DIR_PIN);
#define SET_CENTER_STEP_DIR_N()         GPIO_ResetBits(STEP_DIR_GPIO, STEP_CENTER_DIR_PIN);

typedef enum 
{
   ACCEL = 0, // 加减速曲线状态：减速阶段
   RUN,       // 加减速曲线状态：匀速阶段
   DECEL,     // 加减速曲线状态：减速阶段
   STOP,      // 加减速曲线状态：停止
} StepMotoState_T;


typedef enum 
{
    STEP_STATE_INIT = 0, 
    STEP_STATE_WAIT,  
    STEP_STATE_EXIT, 
    STEP_STATE_DEF = 0xFF, 
} StepEventState_T;    
    
typedef enum 
{
    STEP_SUB_STATE_INIT = 0, 
    STEP_SUB_STATE_ZERO_L,  //寻找零点 
    STEP_SUB_STATE_DEC,   //减速
    STEP_SUB_STATE_ZERO_R,  //寻找零点 
    STEP_SUB_STATE_DEBOUNCH,  //消抖，反回去走两步 
    STEP_SUB_STATE_DEF = 0xFF, 
} StepEventSubState_T;    

typedef enum 
{
    STEP_FIND_ZERO = 0,
    STEP_RUN,    
} EventOpt_T;   




void CenterMotoInit(u16 arr,u16 psc);

void CenterMotoRun(u16 step);

void CenterMotoStop(void);

int CenterMoto_Event(char opt, void * data);




void Door_L_MotoStart(u16 puty);
void Door_L_MotoStop(void);

void Door_R_MotoStart(u16 puty);
void Door_R_MotoStop(void);

//
void Rise_MotoStart(u16 puty);
void Rise_MotoStop(void);

#endif













