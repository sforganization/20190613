/*******************
LED.h

*******************/

#ifndef _FINGER_H__
#define _FINGER_H__




#define ENABLE_DEBUG			0	//是否使能debug功能，使能的话会用到一个串口输出



    
#define FINGER_POWER_RCC_CLOCKCMD RCC_APB2PeriphClockCmd
#define FINGER_POWER_RCC_CLOCKGPIO RCC_APB2Periph_GPIOA
#define FINGER_POWER_GPIO		GPIOA
#define FINGER_POWER_PIN		GPIO_Pin_1	//控制模块电源开关
#define FINGER_POWER_ON()		GPIO_SetBits(FINGER_POWER_GPIO, FINGER_POWER_PIN)
#define FINGER_POWER_OFF()		GPIO_ResetBits(FINGER_POWER_GPIO, FINGER_POWER_PIN)

#define PS_Sta   PAin(4)//读指纹模块状态引脚


#define TRUE  0x01
#define FALSE  0x00

#define DATA_START      0xf5//数据包开始
#define DATA_END      0xf5//数据包结束

#define CMD_ENROLL1     0x01//添加指纹步骤一
#define CMD_ENROLL2     0x02//添加指纹步骤二
#define CMD_ENROLL3     0x03//添加指纹步骤三
#define CMD_DELETE      0x04//删除指定编号指纹
#define CMD_CLEAR       0x05//清空所有指纹
#define CMD_USERNUMB      0x09//取用户总数
#define CMD_IDENTIFY      0x0b//1:1比对
#define CMD_SEARCH      0x0c//1:N比对
#define CMD_MODE_SET      0x2D//录入模式设置

#define ACK_SUCCESS     0x00  //操作成功
#define ACK_FAIL        0x01  //操作失败
#define ACK_FULL        0x04  //指纹数据库已满
#define ACK_NOUSER      0x05  //无此用户
#define ACK_USER_EXIST  0x07  //用户已存在
#define ACK_TIMEOUT     0x08  //采集超时



void Delay_ms(unsigned int ms)  ; 
/*******************************************************************************
**注册指纹
**输入两次指纹注册一个指纹模板
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Enroll(unsigned int u_id);

/*******************************************************************************
**验证指纹
**
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Identify(void);

/*******************************************************************************
**清空指纹
**
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Clear(void);

/*******************************************************************************
**删除指定指纹
**
**参数：UserID 指纹号
*******************************************************************************/
unsigned char Finger_Delete(unsigned int u_id);

int Finger_Read(void);

unsigned char Finger_Search(void);

int Finger_ModeSet(void);


void FingerGPIO_Init(void);


#endif

