#ifndef __UpComputer_H
#define __UpComputer_H
#include "USER_Config.h"

typedef enum{
    MusicPid=0,
    MicPid,
    EfPid,
    OutPid,
    DrePid,
    SetupPid,	//5
    Acm86Pid,
    WMicPid,
    MainPid=0x10,
    CmdPid=0x11,
}PageID;

typedef union{
	float 	f;
	int32_t h;
} fl2hex;

typedef enum{
	MusicEq0, MusicEq1, MusicEq2, MusicEq3, MusicEq4, MusicEq5, MusicEq6, MusicEq7, MusicEq8, MusicEq9,
	MicEq0_0, MicEq0_1,	MicEq0_2, MicEq0_3,	MicEq0_4, MicEq0_5,	MicEq0_6, MicEq0_7,	MicEq0_8, MicEq0_9,
	MicEq1_0, MicEq1_1,	MicEq1_2, MicEq1_3,	MicEq1_4, MicEq1_5,	MicEq1_6, MicEq1_7,	MicEq1_8, MicEq1_9,
	MicEq2_0, MicEq2_1,	MicEq2_2, MicEq2_3,	MicEq2_4, MicEq2_5,	MicEq2_6, MicEq2_7,	MicEq2_8, MicEq2_9,
	MicEq3_0, MicEq3_1,	MicEq3_2, MicEq3_3,	MicEq3_4, MicEq3_5,	MicEq3_6, MicEq3_7,	MicEq3_8, MicEq3_9,
	MicEq4_0, MicEq4_1,	MicEq4_2, MicEq4_3,	MicEq4_4, MicEq4_5,	MicEq4_6, MicEq4_7,	MicEq4_8, MicEq4_9,
	EfEq0_0,  EfEq0_1,	EfEq0_2,  EfEq0_3,	EfEq0_4,  EfEq0_5,	EfEq0_6,  EfEq0_7,	EfEq0_8,  EfEq0_9,
	EfEq1_0,  EfEq1_1,	EfEq1_2,  EfEq1_3,	EfEq1_4,  EfEq1_5,	EfEq1_6,  EfEq1_7,	EfEq1_8,  EfEq1_9,

	OutEq0L_0, OutEq0L_1,	OutEq0L_2, OutEq0L_3,	OutEq0L_4, OutEq0L_5,	OutEq0L_6, OutEq0L_7,	OutEq0L_8, OutEq0L_9,
	OutEq0R_0, OutEq0R_1,	OutEq0R_2, OutEq0R_3,	OutEq0R_4, OutEq0R_5,	OutEq0R_6, OutEq0R_7,	OutEq0R_8, OutEq0R_9,
	OutEq1L_0, OutEq1L_1,	OutEq1L_2, OutEq1L_3,	OutEq1L_4, OutEq1L_5,	OutEq1L_6, OutEq1L_7,	OutEq1L_8, OutEq1L_9,
	OutEq1R_0, OutEq1R_1,	OutEq1R_2, OutEq1R_3,	OutEq1R_4, OutEq1R_5,	OutEq1R_6, OutEq1R_7,	OutEq1R_8, OutEq1R_9,
	OutEq2L_0, OutEq2L_1,	OutEq2L_2, OutEq2L_3,	OutEq2L_4, OutEq2L_5,	OutEq2L_6, OutEq2L_7,	OutEq2L_8, OutEq2L_9,
	OutEq2R_0, OutEq2R_1,	OutEq2R_2, OutEq2R_3,	OutEq2R_4, OutEq2R_5,	OutEq2R_6, OutEq2R_7,	OutEq2R_8, OutEq2E_9,

	DreEqLL_0, 	DreEqLL_1,	DreEqLL_2, 	DreEqLL_3,	//LCh LowLevel
	DreEqLR_0, 	DreEqLR_1,	DreEqLR_2, 	DreEqLR_3,	//RCh LowhLevel
	DreEqDet_0,	DreEqDet_1,	DreEqDet_2,	DreEqDet_3,//Detection Level
	DreEqHL_0, 	DreEqHL_1,	DreEqHL_2, 	DreEqHL_3,	//LCh HighLevel
	DreEqHR_0, 	DreEqHR_1,	DreEqHR_2, 	DreEqHR_3,	//RCh HighLevel
	mDreEqL_0, 	mDreEqL_1,	mDreEqL_2, mDreEqL_3,	//LCh LowLevel
	mDreEqH_0, 	mDreEqH_1,	mDreEqH_2, mDreEqH_3,	//RCh LowhLevel
	mDreEqDet_0,mDreEqDet_1,mDreEqDet_2,mDreEqDet_3,//Detection Level
	EQEnd
} EqIndex;
//{0,10,60,80};	// MusicEq*1Pagh, MicEq*5Pagh, MicEffectEq*2Pagh, OutEq*3Pagh

//***************************************************
/*
typedef struct
{
	uint8_t   Inv 	:8;		// 高位反向=0, 低位靜音=1
	int	 vol 	:16;	// 0.1db step
} VOL_TAB;	// 4 Bye
*/

#define Send_NO     *(uint32_t*)&SendBuff[4]
#define Send_Id1    SendBuff[8]
#define Send_Id2    SendBuff[9]
#define Send_Group  SendBuff[10]
#define Send_RW     SendBuff[11]
#define Send_Val1   *(uint32_t*)&SendBuff[12]
#define Send_Val2   *(uint32_t*)&SendBuff[16]
#define Send_Val3   *(uint32_t*)&SendBuff[20]
#define Send_Val4   *(uint32_t*)&SendBuff[24]
#define Send_Val41  SendBuff[24]
#define Send_Val42  SendBuff[25]
#define Send_Val43  SendBuff[26]
#define Send_Val44  SendBuff[27]
#define Send_ChkSum *(uint32_t*)&SendBuff[28]

extern uint8_t SendBuff[32];

extern	uint8_t WriteData_EN;	// 保存資料致能

extern uint8_t UpComputerRW;

extern uint8_t MainBoardInf[4];	// SVer/DAMP/WMIC/DSP
#define	MainBoardModel *(uint32_t*)MainBoardInf
#define	MainBoardDsp  MainBoardInf[0]
#define	MainBoardWmic MainBoardInf[1]
#define	MainBoardDAmp MainBoardInf[2]
#define	MainBoardSVer MainBoardInf[3]

void SendUpComp(uint8_t *Buff, uint8_t Len);	// 發送資料到上位機
//void UpComputerRx(uint8_t rdata);		// 讀取 UART 資料
//void UpComputerHidRx(void*ptr);
extern uint8_t lf;
void UpComputerRx();
void UpComputerHid32ByeRx(void*ptr);
void UpComputerUart32ByeRx(uint8_t*ptr);

#endif  

