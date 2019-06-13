#ifndef __STEP_DRIVER_H
#define __STEP_DRIVER_H	 
#include "sys.h"  




/* S�ͼ��ٲ��� */

//����������6400 ������һ��
//1600 Ϊ1/4Ȧ


#define CIRCLE_STEP_PULSE           6400 //һȦ�Ĳ���


#define ACCELERATED_SPEED_LENGTH    (CIRCLE_STEP_PULSE / 4 / 2) //800 //������ٶȵĵ�������ʵҲ��800��ϸ�ֲ�����˼��������������ı���ٵ�
#define CIRCLE_STEP_LENGTH          (CIRCLE_STEP_PULSE - ACCELERATED_SPEED_LENGTH * 2)  //һȦ��Ҫ�����ߵĲ���

#define FRE_MIN 				    100  //��͵�����Ƶ�ʣ����������������������ٶ�
#define FRE_MAX 				    2500 //��ߵ�����Ƶ�ʣ������������������ʱ������ٶ�3000
#define FIEXIBLE                   6    //flexible: flexible value. adjust the S curves, ��ֵԽ��S������ʼԽ��,��ʼƵ��Խ��
#define TIMER_FRE                   80000//��ʱ������Ƶ��80K

#define MOTO_ARR                    39   // 80000 / ��39 + 1�� = 2Khz
#define MOTO_PUTY_99                (MOTO_ARR * 99 / 100)     
#define MOTO_PUTY_10                (MOTO_ARR * 10 / 100)     
#define MOTO_PUTY_30                (MOTO_ARR * 30 / 100)   
#define MOTO_PUTY_40                (MOTO_ARR * 40 / 100)     

#define DEC_STEP_NUM               100  //�ҵ���㣬��ǰ�߼���
#define DEBOUNCH_STEP_NUM          5    //�Ӽ�⵽����ٹ�������������λ��

#define STEP_DIR_RCC_CLOCKCMD       RCC_APB2PeriphClockCmd
#define STEP_DIR_RCC_CLOCKGPIO      RCC_APB2Periph_GPIOD
#define STEP_DIR_GPIO               GPIOD

#define STEP_DOOR_DIR_PIN_L         GPIO_Pin_0    //
#define STEP_DOOR_DIR_PIN_R         GPIO_Pin_1    //
#define STEP_RISE_DIR_PIN           GPIO_Pin_2    //
#define STEP_CENTER_DIR_PIN         GPIO_Pin_3    //

//Ĭ��DIR + ����3.3v, Ҫ���ͨ�Żᷴת��dir- ��0
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
   ACCEL = 0, // �Ӽ�������״̬�����ٽ׶�
   RUN,       // �Ӽ�������״̬�����ٽ׶�
   DECEL,     // �Ӽ�������״̬�����ٽ׶�
   STOP,      // �Ӽ�������״̬��ֹͣ
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
    STEP_SUB_STATE_ZERO_L,  //Ѱ����� 
    STEP_SUB_STATE_DEC,   //����
    STEP_SUB_STATE_ZERO_R,  //Ѱ����� 
    STEP_SUB_STATE_DEBOUNCH,  //����������ȥ������ 
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













