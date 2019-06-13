#ifndef __MACHINE_H__
#define __MACHINE_H__

/* 包含头文件 ----------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <stm32f10x.h>

/* 类型定义 ------------------------------------------------------------------*/


//定义回调函数类型
typedef  int    (*pfnCallBack)(char cmd, void *data);
//int Finger_Event(char cmd, void *data)

typedef struct 
{
    bool   bEnable;  //状态机是否使能
    bool   bFinish;       //是否完成
    u16    u16RetVal;     //运行结果返回值
    vu32   u32StartTime;  //开始进入machine 状态机时间
    vu32   u32LastTime;   //最后进入状态机时间
    u32    u32RunCnt;
}MachineRun_t;

typedef struct 
{
    u16   u16InterValTime;   //间隔时间，状态机频率 （u32SysTimes - u32LastTime > , 后期精确延时考虑时间溢出情况u32大概49天才会溢出）
    pfnCallBack CallBack;   //回调函数
    u8    cmd;
    vu32  u32Timeout;        //超时时间
    void *pData;            //数据
}MachineCtrl_t;

typedef struct 
{
    MachineRun_t    MachineRun;
    MachineCtrl_t   MachineCtrl;
    bool            bInit; 
    bool            bMutex;
}PollEvent_t;

typedef struct 
{
    u8              u8FingerID;
}FingerData_t;

typedef enum 
{   
   EVENT_FAILED = 0, 
   EVENT_SUCCESS, 
   
   EVENT_TIME_OUT = 0XFE,
   EVENT_RET_DEF = 0XFF, 
}PollEvent_e;


/* 宏定义 --------------------------------------------------------------------*/
/*
 */

#endif  //

