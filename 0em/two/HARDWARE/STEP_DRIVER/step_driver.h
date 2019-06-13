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
#define TIMER_FRE                   800000//��ʱ������Ƶ��800K

#define MOTO_ARR                    39   // TIMER_FRE / ��39 + 1�� = 20Khz 
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

void Moto_1_Start_CCW(u16 puty);   //��ת
void Moto_1_Start_CW(u16 puty);           //��ת
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













