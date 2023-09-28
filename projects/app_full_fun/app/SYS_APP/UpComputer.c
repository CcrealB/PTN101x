/*************************************************************************
 PTN101x Effect_HID  V3.3 Beta Ver:05	 2023/05/28

************************************************************************/
//#include "USER_Config.h"
#include "UpComputer.h"
#ifdef __UpComputer_H

#ifdef CONFIG_AUD_AMP_DET_FFT
//	extern	u8 W_Y[9];
	extern	u8 RGB_MODE[9];	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
	extern	u8 RGB_AUTO[9];	//聲控自動轉漸變(0,1)
	extern	u8 RGB_SP[9];	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
	extern	u8 RGB_EF[9];	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
	extern	u8 RGB_AEF[9];	//
#endif
uint8_t lf = 0;				// 接收 Buff 指標
#if IC_MODEL == IC_PTN1011
	uint8_t MainBoardInf[4] = {0x01,0xFF,0xFF,0x33};	//DSP/WMIC/DAMP/SVER
#else
	uint8_t MainBoardInf[4] = {0x02,0xFF,0xFF,0x33};
#endif

//               前導碼               機號                 功能 讀寫 長度 群組
//*************************************************************************************
uint8_t SendBuff[32] = {0xA5,0xA1,0x1A,0x5A, 0xFF,0xFF,0xFF,0xFF, 0x00,0x00,0x00,0x00,
                   0x00,0x00,0x00,0x00,   // VAL 1
                   0x00,0x00,0x00,0x00,   // VAL 2
                   0x00,0x00,0x00,0x00,   // VAL 3
                   0x00,0x00,0x00,0x00,   // VAL 4
                   0xFF,0xFF,0xFF,0xFF};  // ChkSum


//*********************************************
uint32_t crc_chksum(uint8_t *DATA, uint16_t length)
{
	uint16_t j;
	uint32_t reg_crc=0xFFFFFFFF;
	while(length--){
		reg_crc ^= *DATA++;
		for(j=0;j<8;j++){
			if(reg_crc & 0x01){ // LSB(b0)=1
				reg_crc=(reg_crc>>1) ^ 0xA0000001;
			}else{
				reg_crc=reg_crc >>1;
			}
		}
	}
	return reg_crc;
}

//***************************************************
void SendUpComp(uint8_t *Buff, uint8_t Len)
{
	Send_NO = MainBoardModel;
//	Send_NO = 0xFFFFFFFF;
	Send_ChkSum = crc_chksum(Buff,28);
	com_cmd_send_proc(Buff, 32);
}

//***************************************
uint8_t RxBuff[96];
#define Page  RxBuff[8]
#define CH  RxBuff[9]
#define RxGroup RxBuff[10]
//#define RxBuff_Val1   *(uint32_t*)&RxBuff[12]
//#define RxBuff_Val2   *(uint32_t*)&RxBuff[16]
//#define RxBuff_Val3   *(uint32_t*)&RxBuff[20]
//#define RxBuff_Val4   *(uint32_t*)&RxBuff[24]
#define RxBuff_Val41  RxBuff[24]
#define EqGrId  RxBuff[24]
#define RxBuff_Val42  RxBuff[25]
#define EqWorkId  RxBuff[25]
#define RxBuff_Val43  RxBuff[26]
#define RxBuff_Val44  RxBuff[27]
static fl2hex V1, V2, V3, V4;
uint8_t UpComputerRW = 0;
static uint8_t f, ch;
static uint8_t HidBle_Rx=0;
static uint8_t addr;
static uint8_t i;

//***************************************************
void UpComputerHid32ByeRx(void*ptr)
{
	memcpy(RxBuff, ptr, 32);
	HidBle_Rx=1;
	UpComputerRx();
}
//***************************************************
void UpComputerUart32ByeRx(uint8_t*ptr)
{
	memcpy(RxBuff, ptr, 32);
	UpComputerRx();
}
//***************************************************
void UpComputerRx()
{
	//===========================================
	uint32_t ChkSum = ((uint32_t)RxBuff[31]<<24) | ((uint32_t)RxBuff[30]<<16) | ((uint16_t)RxBuff[29]<<8) | RxBuff[28];
	uint32_t ChkSum1 = crc_chksum(RxBuff,28);
	if(RxBuff[7] != 0x33 || ChkSum != ChkSum1){	// 上位機版本驗證
		g_mcu_log_on_fg = 0;
		SendBuff[7]=0xFF;
		SendUpComp(SendBuff, 32);
		g_mcu_log_on_fg = 1;
		lf=0;
		return;
	}
#ifdef LinkPass
	if(Page==CmdPid && CH==200){
		if(memcmp(LinkPass,&RxBuff[12],6)!=0) return;
	}
#endif
	if(!HidBle_Rx){
		g_dsp_log_on_fg = 0;	// 收到上位機正確指令 關閉 UART DEGBUG 信息
		g_mcu_log_on_fg = 0;
	}

	//==========================================================
	ch = (CH-1);
	UpComputerRW = RxBuff[11];
	V1.h = ((uint32_t)RxBuff[15]<<24) | ((uint32_t)RxBuff[14]<<16) | ((uint16_t)RxBuff[13]<<8) | RxBuff[12];
	V2.h = ((uint32_t)RxBuff[19]<<24) | ((uint32_t)RxBuff[18]<<16) | ((uint16_t)RxBuff[17]<<8) | RxBuff[16];
	V3.h = ((uint32_t)RxBuff[23]<<24) | ((uint32_t)RxBuff[22]<<16) | ((uint16_t)RxBuff[21]<<8) | RxBuff[20];
	V4.h = ((uint32_t)RxBuff[27]<<24) | ((uint32_t)RxBuff[26]<<16) | ((uint16_t)RxBuff[25]<<8) | RxBuff[24];
	//======================================================================================================
	if(Page < SetupPid){
		const uint8_t	EqF[4] = {0,10,60,80};	// MusicEq*1Pagh, MicEq*5Pagh, MicEffectEq*2Pagh, OutEq*6Pagh
		const uint8_t	EqTNum[4] = {1,5,2,6};
		if(Page <= OutPid){
			//---------------------------------------
			if(ch<10){
				f = (EqF[Page]+ch);
				if(Page==1) SysInf.MicEqId =0xFF;
				if(Page==2) SysInf.MicEfEqId =0xFF;
				if(Page==3) SysInf.OutEqId =0xFF;
				for(i=0; i<EqTNum[Page]; i++){
					addr = (i*10)+f;
					if((EqGrId>>i)&1){
						if(Page==1 && SysInf.MicEqId==0xFF) SysInf.MicEqId=i;
						if(Page==2 && SysInf.MicEfEqId==0xFF) SysInf.MicEfEqId=i;
						if(Page==3 && SysInf.OutEqId==0xFF) SysInf.OutEqId=i;
						//uint8_t index, uint16_t type, float freq, float Q, float gain
						EQ_SET(addr, RxBuff[25], V1.f, V2.f, V3.f);
					}
				}
			//----------------------------------------
			}else if(CH==100){
				if(Page==1)	SysInf.MicEqId = EqWorkId;
				if(Page==2)	SysInf.MicEfEqId = EqWorkId;
				if(Page==3)	SysInf.OutEqId = EqWorkId;
				f = (EqF[Page]+(EqWorkId*10));
				for(i=f; i<(f+10); i++){
					if(Page==3){
						OutEqR[i-80].freq = 0xFFFF;
					}else{
						UserEqR[i].freq = 0xFFFF;
					}
				}
			}
		//============================================
		}else if(Page == DrePid){
			if(ch<4){
				SysInf.DreEqId =0xFF;
				for(i=0; i<8; i++){
					addr = (i*4)+ch;
					if((EqGrId>>i)&1){
						if(SysInf.DreEqId==0xFF) SysInf.DreEqId=i;
						EQ_SET(addr+140, RxBuff[25], V1.f, V2.f, V3.f);
					}
				}
			}else if(CH==100){
				SysInf.DreEqId = EqWorkId;
				f = (EqWorkId*4);
				for(i=f; i<(f+4); i++)	DreSetR.DreEq[i].freq = 0xFFFF;
			}else if(CH==101){
				DreSet.MusDreHighGain = RxBuff_Val41;
				DreSet.MusDreLowGain = RxBuff_Val42;
				DreSet.MicDreHighGain = RxBuff_Val43;
				DreSet.MicDreLowGain = RxBuff_Val44;
			}else if(CH==102){
				DreSet.MusDreOnOff = EqGrId;
			}else if(CH==103){
				DreSet.MicDreOnOff = EqGrId;
			}
		}
	}else if(Page == SetupPid){
		if(CH == 100){
			SysInf.DuckDet = RxBuff[24];
			SysInf.DuckIgain = V1.f;
			SysInf.DuckHtime = V2.f;
			SysInf.DuckFadeDB = V3.f;
		}else if(CH == 101){
			SysInf.VoiceCanGain = V1.f;
			SysInf.VoiceCanFc0 = V2.f;
			SysInf.VoiceCanFc1 = V3.f;
		}else if(CH == 102){
			SysInf.MicCompT = V1.f;
			SysInf.MicCompAT = V2.f;
			SysInf.MicCompRT = V3.f;
			SysInf.MicCompR = RxBuff[24];
			SysInf.MicCompW = RxBuff[25];
		}else if(CH == 103){
			SysInf.MusCompT = V1.f;
			SysInf.MusCompAT = V2.f;
			SysInf.MusCompRT = V3.f;
			SysInf.MusCompR = RxBuff[24];
			SysInf.MusCompW = RxBuff[25];
		}else if(CH == 104){
			SysInf.AngInGain = V4.h;
		}else if(CH == 105){
			SysInf.Out1Mix = RxBuff[24];
		}else if(CH == 106){
			SysInf.Out2Mix = RxBuff[24];
		}else if(CH == 107){
			SysInf.Out3Mix = RxBuff[24];
		}else{
			ioGain[ch] = V1.f;
		}
#ifdef ACM86xxId1
	}else if(Page == Acm86Pid){
#ifdef ZY001
		uint8_t DrbEqfg = (uint8_t)SysInf.MusicVol>>1;
		if(DrbEqfg)	DrbEqfg--;
#endif
		if(CH<=24){
			if(EqGrId&0x01){
				ACM_Set[ACM862xWId].EQ[ch].freq = (uint16_t)V1.f;
				ACM_Set[ACM862xWId].EQ[ch].Q = (float)V2.f*1000;
				ACM_Set[ACM862xWId].EQ[ch].gain = (float)V3.f*10;
				ACM_Set[ACM862xWId].EQ[ch].type = RxBuff[25];
#ifdef ZY001
				extern int16_t EG20_TAB[16];
				extern int16_t EG21_TAB[16];
				extern int16_t EG22_TAB[16];
				extern int16_t EG23_TAB[16];
				if(f==20) EG20_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f].gain;
				if(f==21) EG21_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f].gain;
				if(f==22) EG22_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f].gain;
				if(f==23) EG23_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f].gain;
#endif
			}
			if(EqGrId&0x02){
				ACM_Set[ACM862xWId].EQ[ch+24].freq = (uint16_t)V1.f;
				ACM_Set[ACM862xWId].EQ[ch+24].Q = (float)V2.f*1000;
				ACM_Set[ACM862xWId].EQ[ch+24].gain = (float)V3.f*10;
				ACM_Set[ACM862xWId].EQ[ch+24].type = RxBuff[25];
#ifdef ZY001
				extern int16_t EG44_TAB[16];
				extern int16_t EG45_TAB[16];
				extern int16_t EG46_TAB[16];
				extern int16_t EG47_TAB[16];
				if(f==20) EG44_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f+24].gain;
				if(f==21) EG45_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f+24].gain;
				if(f==22) EG46_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f+24].gain;
				if(f==23) EG47_TAB[DrbEqfg]=ACM_Set[ACM862xWId].EQ[f+24].gain;
#endif
			}
		}else if(CH==25){
			ACM_Set[ACM862xWId].DrbOnOff = RxBuff[25]&1;
			ACM_Set[ACM862xWId].LR_Mix = (RxBuff[25]>>1)&1;
			ACM_Set[ACM862xWId].DrbLowGain = RxBuff[26];
			ACM_Set[ACM862xWId].DrbHighGain = RxBuff[27];
			ACM_Set[ACM862xWId].ClassDGaie = V1.f;
			ACM_Set[ACM862xWId].InGaie = V2.f;
		}else if(CH==100){
			ACM862xWId = RxBuff_Val42;
			if(ACM862xWId>1)	ACM862xWId = 1;
			SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
//			OLED_num(0, 2, ACM862xWId, 2, 8);
			if(RxBuff_Val41==2){
				for(i=24; i<48; i++) ACM_SetR[ACM862xWId].EQ[i].freq = 0xFFFF;
			}else{
				for(i=0; i<24; i++) ACM_SetR[ACM862xWId].EQ[i].freq = 0xFFFF;
			}
			ACM_SetR[ACM862xWId].DrbOnOff = 0xFF;
			ACM_SetR[ACM862xWId].LR_Mix = 0xFF;
			ACM_SetR[ACM862xWId].DrbLowGain = 0xFF;
			ACM_SetR[ACM862xWId].DrbHighGain = 0xFF;
			ACM_SetR[ACM862xWId].ClassDGaie = 0xFFFF;
			ACM_SetR[ACM862xWId].InGaie = 0xFFFF;
		}
#endif

	}else if(Page == MainPid){
		enum
		{
			BUTTON_BT_NONE = 0,
			BUTTON_BT_PLAY_PAUSE,
			BUTTON_BT_NEXT,
			BUTTON_BT_PREV,
		};
		switch (CH) {
    		case 3:	SysInf.MusicVol = V1.f;					break;
    		case 4:	SysInf.MusNoiseGate = V1.f;				break;
    		case 5:	SysInf.KeyShift = V1.f;					break;
    		case 6:	SysInf.OutVol = V1.f;					break;
    		case 7:	UserEf.drygain = (float)V1.f/10;		break;
    		case 8:	UserEf.drygain2 = (float)V1.f/10;		break;
    		case 9:	SysInf.MicVol = V1.f;					break;
    		case 10:SysInf.MicNoiseGate = V1.f;				break;
    		case 11:UserEf.Fshift = V1.f;					break;
    		case 12:UserEf.Pitch = V1.f;					break;
    		case 13:UserEf.RevVol = (float)V1.f/10;			break;
    		case 14:UserEf.RevVol2 = (float)V1.f/10;		break;
    		case 15:UserEf.lex_PreDelay = V1.f;				break;	// REV Room
    		case 16:UserEf.RevRep = (float)V1.f/10;			break;
    		case 17:UserEf.lex_iDfus1 = (float)V1.f/100;	break;
    		case 18:UserEf.lex_iDfus2 = (float)V1.f/100;	break;
    		case 19:UserEf.lex_Excur = (float)V1.f;			break;
    		case 20:UserEf.lex_dDfus1 = (float)V1.f/100;	break;
    		case 21:UserEf.lex_dDfus2 = (float)V1.f/100;	break;
    		case 22:UserEf.lex_HFreqDamp = (float)V1.f/10;	break;
    		case 23:UserEf.EchoVol = (float)V1.f/10;		break;
    		case 24:UserEf.EchoVol2 = (float)V1.f/10;		break;
    		case 25:UserEf.EchoDeyT = V1.f;					break;
    		case 26:UserEf.EchoRep = (float)V1.f/10;		break;
    		case 27:SysInf.MicCompT = (float)V1.f;			break;
    		case 28:SysInf.MusCompT = (float)V1.f;			break;
    		case 29:SysInf.VoiceCanEn = V1.f;				break;
    		//-----------------------------------------------------
    		case 100:
    			UserFlash4K_read();
    			break;
    		case 102:
    			UserFlash4K_Write(RxBuff);
    		    break;
    		case 200:
    			// system_work_mode_change_button();
                msg_put(MSG_CHANGE_MODE);//中断中耗时太长，通过消息方式切模式
    			break;
    		case 201:
    			app_button_sw_action(BUTTON_BT_PREV);
    		   	break;
    		case 202:
		#ifdef	RECED_EN
    			user_RecPlayWork();
		#else
    			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
    	#endif
    			break;
    		case 203:
    			app_button_sw_action(BUTTON_BT_NEXT);
    			break;
		#ifdef	RECED_EN
    		case 204:	// REC
    			user_RecWork();
    			break;
		#endif
		}
	}else if(Page == CmdPid){	// 命令 ID Page
		switch(CH){
		case 0:
			EF_Mode = RxGroup;
			break;
		case 1:
			EQ_Mode = RxGroup;
			break;
		case 2:
			memcpy(&LabEf[RxGroup], &RxBuff[12], 16);
			GroupWrite(RxGroup,0);
			break;
		case 3:
			memcpy(&LabEf[RxGroup], &RxBuff[12], 16);
			GroupWrite(RxGroup,1);
			break;
		case 200:	// 上位機連線請求
			UpComputerRW = 0;
			EF_EQ_ReSend();
#ifdef ACM86xxId1
			ACMIIR_ReSend(0);
#endif
#ifdef ACM86xxId2
			ACMIIR_ReSend(1);
#endif
			break;
		case 201:	// 清除 USER FLASH
			ClearUserFlash();
			break;
		case 202:
			g_dsp_log_on_fg = 1;	// 收到上位機離線指令 打開 UART DEGBUG 信息
			g_mcu_log_on_fg = 1;
			break;
#ifdef CONFIG_AUD_AMP_DET_FFT
		case 211:	//
			RGB_MODE[0]=RxBuff[10];	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
			RGB_EF[0]=RxBuff[24];	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
			RGB_SP[0]=RxBuff[25];	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
			RGB_AUTO[0]=RxBuff[26];	//聲控自動轉漸變(0,1)
			RGB_AEF[0]=RxBuff[27];
			break;
#endif
		default:
			break;
		}
    }
	lf = 0;
}

#endif

