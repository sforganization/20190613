/**
  ******************************************************************************
  * @file    useri2c.h
  * @author  kevin_guo
  * @version V1.0.0
  * @date    12-15-2018
  * @brief   This file contains definitions for useri2c.c
  ******************************************************************************
  * @attention
  ******************************************************************************  
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USERI2C_H
#define __USERI2C_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "sys.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
//IO��������

#define USERI2C_SDA_IN()  {GPIOA->CRL&=0XFFFFF0FF;GPIOA->CRL|=(u32)8 << 8;GPIOA->BSRR |=1<<2;}   //CRL ���� ����0��7 io  CRH 8 ��15
#define USERI2C_SDA_OUT() {GPIOA->CRL&=0XFFFFF0FF;GPIOA->CRL|=(u32)3 << 8;}

#define GPIO_PORT_I2C	  GPIOA			                /* GPIO�˿� */
#define RCC_I2C_PORT 	  RCC_APB2Periph_GPIOA		    /* GPIO�˿�ʱ�� */
#define I2C_SCL_PIN		  GPIO_Pin_1			        /* ���ӵ�SCLʱ���ߵ�GPIO */
#define I2C_SDA_PIN		  GPIO_Pin_2			        /* ���ӵ�SDA�����ߵ�GPIO */

//IO��������	 
#define USERI2C_SCL    			PAout(1) //SCL
#define USERI2C_SDA    			PAout(2) //SDA	 
#define USERI2C_READ_SDA   	    PAin(2)  //����SDA 



/* Exported functions ------------------------------------------------------- */
//IIC���в�������
void UserI2c_Start(void);  
void UserI2c_Stop(void);
unsigned char UserI2c_Wait_Ack(void);
void useri2c_ack(void); 
void useri2c_nack(void);			  
void UserI2c_Send_Byte(unsigned char dat);
unsigned char UserI2c_Read_Byte(void);
void UserI2c_Init(void);



#endif /* __USERI2C_H */

/************************END OF FILE*************************/

