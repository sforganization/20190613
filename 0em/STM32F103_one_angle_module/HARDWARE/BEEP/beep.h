#ifndef __BEEP_H
#define __BEEP_H
#include "sys.h"

//定义低音音名C
#define CL1 262
#define CL2 294
#define CL3 330
#define CL4 349
#define CL5 392
#define CL6 440
#define CL7 494

//定义中音音名C
#define CM1 523
#define CM2 587
#define CM3 659
#define CM4 698
#define CM5 784
#define CM6 880
#define CM7 988

//定义高音音名C
#define CH1 1047
#define CH2 1175
#define CH3 1319
#define CH4 1397
#define CH5 1568
#define CH6 1760
#define CH7 1976

//定义低音音名D
#define DL1 294
#define DL2 330
#define DL3 370
#define DL4 392
#define DL5 440
#define DL6 494
#define DL7 554

//定义中音音名D
#define DM1 587
#define DM2 659
#define DM3 740
#define DM4 784
#define DM5 880
#define DM6 988
#define DM7 1109

//定义高音音名D
#define DH1 1175
#define DH2 1319
#define DH3 1480
#define DH4 1568
#define DH5 1760
#define DH6 1976
#define DH7 2217

//定义低音音名E
#define EL1 330
#define EL2 370
#define EL3 415
#define EL4 440
#define EL5 494
#define EL6 554
#define EL65 587
#define EL7 622

//定义中音音名E
#define EM1 659
#define EM2 740
#define EM3 831
#define EM4 880
#define EM5 988
#define EM6 1109
#define EM7 1245

//定义高音音名E
#define EH1 1319
#define EH2 1480
#define EH3 1661
#define EH4 1760
#define EH5 1976

//定义低音音名F (单位是Hz)
#define FL1 349
#define FL2 392
#define FL3 440
#define FL4 466
#define FL5 523
#define FL6 587
#define FL7 659

//定义中音音名F
#define FM1 698
#define FM2 784
#define FM3 880
#define FM4 932
#define FM5 1047
#define FM6 1175
#define FM7 1319
	
//定义高音音名F
#define FH1 1397
#define FH2 1568
#define FH3 1760
#define FH4 1865


//定义时值单�?决定演奏的速度 ms为单�?2000为佳
#define TT 2000   
#define TTS 1000  
#define TT1 300  
extern u16 CL[7];
extern u16 CM[7];
extern u16 CH[7];
extern u16 DL[7];
extern u16 DM[7];
extern u16 DH[7];
extern u16 EL[7];
extern u16 EM[7];
extern u16 EH[7];
extern u16 FL[7];
extern u16 FM[7];
extern u16 FH[7];
extern u8 BGM ;
extern u8 BGM_change_flg;
extern u8 BGM_volum;

void BeepQuiet(void);
void BeepInit(u16 arr,u16 psc);
void BeepSound(unsigned short usFraq,unsigned int volume_level);
void BeepMusicPlay(int length,unsigned char volume_level);

#define BAD_BGM   1
#define GOOD_BGM   2
#define BEGIN_BGM 3
#define LIFE_BGM  4
#define LEVEL_BGM  5

#endif

