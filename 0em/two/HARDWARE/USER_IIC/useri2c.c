
/* Includes ------------------------------------------------------------------*/
#include "useri2c.h"	
#include "delay.h"

/*******************************************************************************
* Function Name  : UserI2c_Start
* Description    : 时钟脚为高，数据脚高变低
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
	USERI2C_SCL=0;			//钳住I2C总线，准备发送或接收数据 
}	  
/*******************************************************************************
* Function Name  : UserI2c_Stop
* Description    : 时钟脚为高，数据脚低变高
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
	USERI2C_SDA=1;		//发送I2C总线结束信号
	
}
/*******************************************************************************
* Function Name  : UserI2c_Wait_Ack
* Description    : 第9个时钟上升沿读取ack信号
* Input          : None
* Output         : None
* Return         : =0有ack
*								 : =1无ack
*******************************************************************************/
unsigned char UserI2c_Wait_Ack(void)
{
	unsigned int ucErrTime=0;
  unsigned char RetValue;
	
	USERI2C_SDA_IN();      //SDA设置为输入   
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
	USERI2C_SCL=0;//时钟输出0 
 	delay_us(5);	
	USERI2C_SDA_OUT();//sda线输出 
	USERI2C_SDA=1; 	
	return RetValue;  
} 
/*******************************************************************************
* Function Name  : useri2c_ack
* Description    : 第9个时钟上升沿发送低电平ack
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
* Description    : 第9个时钟上升沿发送高电平nack
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
* Description    : 写1字节数据到i2c总线
* Input          : dat-要发送的数据
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
* Description    : 从i2c总线读1字节数据
* Input          : None
* Output         : None
* Return         : 读到的1字节数据
*******************************************************************************/ 
unsigned char UserI2c_Read_Byte(void)
{
	unsigned char i,receive=0;
	
	USERI2C_SDA_IN();//SDA设置为输入
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
	USERI2C_SDA_OUT();//sda线输出 
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
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_PORT_I2C, &GPIO_InitStructure);
	GPIO_SetBits(GPIO_PORT_I2C,I2C_SCL_PIN|I2C_SDA_PIN); 	//输出高
}




/************************END OF FILE*************************/
