
/* Includes ------------------------------------------------------------------*/
#include "useri2c.h"	
#include "delay.h"

/*******************************************************************************
* Function Name  : UserI2c_Start
* Description    : ʱ�ӽ�Ϊ�ߣ����ݽŸ߱��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UserI2c_Start(void)
{
	USERI2C_SDA=1;	  	  
	USERI2C_SCL=1;
	delay_us(30);
 	USERI2C_SDA=0;			
	delay_us(30);
	USERI2C_SCL=0;			//ǯסI2C���ߣ�׼�����ͻ�������� 
}	  
/*******************************************************************************
* Function Name  : UserI2c_Stop
* Description    : ʱ�ӽ�Ϊ�ߣ����ݽŵͱ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UserI2c_Stop(void)
{
	USERI2C_SCL=0;
	USERI2C_SDA=0;
 	delay_us(30);
	USERI2C_SCL=1;
 	delay_us(30);
	USERI2C_SDA=1;		//����I2C���߽����ź�
	
}
/*******************************************************************************
* Function Name  : UserI2c_Wait_Ack
* Description    : ��9��ʱ�������ض�ȡack�ź�
* Input          : None
* Output         : None
* Return         : =0��ack
*								 : =1��ack
*******************************************************************************/
unsigned char UserI2c_Wait_Ack(void)
{
	unsigned int ucErrTime=0;
  unsigned char RetValue;
	
	USERI2C_SDA_IN();      //SDA����Ϊ����   
	USERI2C_SDA=1;  
	USERI2C_SCL=0;
 	delay_us(5);
	USERI2C_SCL=1; 
  ucErrTime = 10000;
  while( ucErrTime-- > 0 )
  {
    if(USERI2C_READ_SDA )
    {
      RetValue = 1;     			
    }
    else
    {
      RetValue = 0;
			break;
    }
  }
	USERI2C_SCL=0;//ʱ�����0 
 	delay_us(5);	
	USERI2C_SDA_OUT();//sda����� 
	USERI2C_SDA=1; 	
	return RetValue;  
} 
/*******************************************************************************
* Function Name  : useri2c_ack
* Description    : ��9��ʱ�������ط��͵͵�ƽack
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void useri2c_ack(void)
{
	USERI2C_SCL=0;
	USERI2C_SDA=0;
	delay_us(5);
	USERI2C_SCL=1;
	delay_us(5);
	USERI2C_SCL=0;
}

/*******************************************************************************
* Function Name  : useri2c_nack
* Description    : ��9��ʱ�������ط��͸ߵ�ƽnack
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/   
void useri2c_nack(void)
{
	USERI2C_SCL=0;
	USERI2C_SDA=1;
	delay_us(5);
	USERI2C_SCL=1;
	delay_us(5);
	USERI2C_SCL=0;
}					
 
/*******************************************************************************
* Function Name  : UserI2c_Send_Byte
* Description    : д1�ֽ����ݵ�i2c����
* Input          : dat-Ҫ���͵�����
* Output         : None
* Return         : None
*******************************************************************************/	  
void UserI2c_Send_Byte(unsigned char dat)
{                        
	unsigned char i; 
  	    
	USERI2C_SCL=0; 	 
	delay_us(5); 
	for(i=0;i<8;i++)
	{         
		if((dat&0x80)>>0)
			USERI2C_SDA=1;
		else
			USERI2C_SDA=0;
		dat<<=1; 	   
		USERI2C_SCL=1;
		delay_us(5);
		USERI2C_SCL=0; 	 
		delay_us(5); 
	}	 
} 	 
/*******************************************************************************
* Function Name  : UserI2c_Read_Byte
* Description    : ��i2c���߶�1�ֽ�����
* Input          : None
* Output         : None
* Return         : ������1�ֽ�����
*******************************************************************************/ 
unsigned char UserI2c_Read_Byte(void)
{
	unsigned char i,receive=0;
	
	USERI2C_SDA_IN();//SDA����Ϊ����
	USERI2C_SDA=1;
	delay_us(5);
	for(i=0;i<8;i++ )
	{ 
		USERI2C_SCL=0;
		delay_us(5);
		USERI2C_SCL=1; 
		delay_us(5);
		receive<<=1; 
		if(USERI2C_READ_SDA)
			receive++;  
	}				
	USERI2C_SCL=0;   
	USERI2C_SDA_OUT();//sda����� 
	return receive;
}


/*******************************************************************************
* Function Name  : UserI2c_Init
* Description    : config i2c driver gpio
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UserI2c_Init(void)
{					     

	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_I2C_PORT, ENABLE );	//
	   
	GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN|I2C_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_PORT_I2C, &GPIO_InitStructure);
	GPIO_SetBits(GPIO_PORT_I2C,I2C_SCL_PIN|I2C_SDA_PIN); 	//�����
}




/************************END OF FILE*************************/
