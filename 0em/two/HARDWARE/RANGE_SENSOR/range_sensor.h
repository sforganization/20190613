#ifndef __RANGE_SENSOR_H
#define __RANGE_SENSOR_H


#include "stm32f10x.h"



//2~254(0x02~0xFE) 默认164(0xA4) bit7~bit1有效bit0=0  地址只能是双数
//#define I2C_DEVID    				0XA4

					 
void RangeSensor_Init(void); //初始化IIC

unsigned char RangeSensor_WritenByte(unsigned char *txbuff, unsigned char deviceaddr,unsigned char regaddr, unsigned char size);
unsigned char RangeSensor_ReadnByte(unsigned char *rxbuff, unsigned char deviceaddr, unsigned char regaddr, unsigned char size);


void RangeSensor_Test(void);
int RangeSensor_Event(char u8Addr, void *pVal);


#endif
















