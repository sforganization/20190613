
#include "range_sensor.h" 
#include "delay.h"
#include "useri2c.h"	

#include "usart3.h"


unsigned short	length_aveval = 0;


//初始化IIC接口
void RangeSensor_Init(void)
{
	UserI2c_Init();
}


/*******************************************************************************
* Function Name  : SensorWritenByte
* Description	 : Sensor read api.
* Input 		 : *txbuff-the buffer which to write.
*				 : regaddr-the address write to
*				 : size-write data size
* Output		 : None
* Return		 : None
*******************************************************************************/
unsigned char RangeSensor_WritenByte(unsigned char * txbuff, unsigned char deviceaddr, unsigned char regaddr,
	 unsigned char size)
{
	unsigned char	i	= 0;

	UserI2c_Start();

	UserI2c_Send_Byte(deviceaddr | 0x00);

	if (1 == UserI2c_Wait_Ack())
	{
		UserI2c_Stop();
		return 1;
	}

	UserI2c_Send_Byte(regaddr & 0xff);

	if (1 == UserI2c_Wait_Ack())
	{
		UserI2c_Stop();
		return 1;
	}

    delay_us(30);
	for (i = 0; i < size; i++)
	{
		UserI2c_Send_Byte(txbuff[size - i - 1]);

		if (1 == UserI2c_Wait_Ack())
		{
			UserI2c_Stop();
			return 1;
		}
	}

	UserI2c_Stop();

	return 0;
}


/*******************************************************************************
* Function Name  : SensorReadnByte
* Description	 : Sensor read api.
* Input 		 : *rxbuff-the buffer which stores read content.
*				 : regaddr-the address read from
*				 : size-read data size
* Output		 : None
* Return		 : None
*******************************************************************************/
unsigned char RangeSensor_ReadnByte(unsigned char * rxbuff, unsigned char deviceaddr, unsigned char regaddr,
	 unsigned char size)
{
	unsigned char	i	= 0;

	UserI2c_Start();

	UserI2c_Send_Byte(deviceaddr | 0x00);

	if (1 == UserI2c_Wait_Ack())
	{
		UserI2c_Stop();
		return 1;
	}

	UserI2c_Send_Byte(regaddr & 0xff);

	if (1 == UserI2c_Wait_Ack())
	{
		UserI2c_Stop();
		return 1;
	}

	UserI2c_Stop();
    
    delay_us(30);
	UserI2c_Start();
	UserI2c_Send_Byte(deviceaddr | 0x01);

	if (1 == UserI2c_Wait_Ack())
	{
		UserI2c_Stop();
		return 1;
	}

	delay_us(30);									//30uS

	for (i = 0; i < size; i++)
	{
		rxbuff[size - i - 1] = UserI2c_Read_Byte();

		if ((i + 1) == size)
			useri2c_nack();
		else 
			useri2c_ack();

		delay_us(30);								//30uS
	}

	UserI2c_Stop();

	return 0;
}





/*******************************************************************************
*******************************************************************************/
int RangeSensor_Event(char u8Addr, void *pVal)
{
    RangeSensor_ReadnByte((unsigned char *)pVal, (unsigned char)u8Addr, 0x04, 2); //  
    return 0; 
}


/*******************************************************************************
* Function Name  : Sensor_I2C_Test
* Description	 : I2C 读写测试
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
void RangeSensor_Test(void)
{
	unsigned char	data[10];

	//	dirt_send_flag=1;
	//	SensorWritenByte((unsigned char *)&dirt_send_flag, 0x09, 1);
	//	delay_ms(100);
	//	dirt_detection_flag=0;
	//	SensorWritenByte((unsigned char *)&dirt_detection_flag, 0x08, 1);
	data[0] 			= 0;
	RangeSensor_WritenByte(data, 0XA6, 0x08, 1);	//0 ：滤波值， 1： 实时值
	delay_ms(100);

	RangeSensor_ReadnByte((unsigned char *) &length_aveval, 0XA6, 0x0f, 2); //(unsigned char *)&length_val	   
	u3_printf("------- addr  %d\n", length_aveval);

	delay_ms(100);

	while (1)
	{
		RangeSensor_ReadnByte((unsigned char *) &length_aveval, 0XA6, 0x04, 2); //0x04滤波值 
		u3_printf("~~~~~~~~~  %d\n", length_aveval);
		delay_ms(400);
	} // while		
}


