#ifndef __KARACMD_H
#define __KARACMD_H

#include "USER_Config.h"

#define DspSoftVer	0x33	// 軟件版本號

typedef enum {
	eq_APF,	// all pass
	eq_LPF,	// low pass
	eq_HPF,	// high pass
	eq_LSF,	// low shelf
	eq_HSF,	// high shelf
	eq_BEL	// bell shape
}eqFiltType;	// EQ TYPE 20220930 Add

//*****************************************************
typedef enum {
	K_bypasscontrol = 1,
//	K_bypass_mic,	//mic bypass
//	K_bypass_mus,	//mus bypass
	K_igain,	// input gain
	K_ogain,	// output gain
	K_ingate,//input noise gate
	K_musingate,//music channel input noise gate
	//==== mic input eq
	K_Miceq0,
	K_Miceq1,
	K_Miceq2,
	K_Miceq3,
	K_Miceq4,
	K_epr_eq,		// echo and reverb eq
	K_epri2s_eq,	// i2s echo and reverb eq
	K_Museq,		// music eq
	//==== output eq
	K_Oeq0,
	K_Oeq1,
	K_Oeq2,
	K_lexrev,
	K_echo,
	K_pshift,		//fft pitch shift
	K_fshift,		//freq shifting howling suppression
	K_keyshifit,	//key shift of music
	//==== reberb and echo wet dry gain
	K_drygain,		// int type  q13
	K_echogain,		// int type  q13
	K_revgain,		// int type  q13
	K_drygainI2S,	// int type  q13
	K_echogainI2S,	// int type  q13
	K_revgainI2S,	// int type  q13
	K_duck_flag,	// select ducking mic input, int type only 0or1  0:closed,1: selected, default: all mic inputs are selected
	K_mduck,		// ducking
	K_SVS,			// singing voice separation
	//==== drc	20220914 Add
	K_micDRC,
	K_musDRC,
	//==== dc filt 20221014 Add
	K_micDCfilt,
	K_musDCfilt,
	//gain damping
	K_igain_damp,// int type; range : 1~64; default:64(20~30ms)	1=(1260~1890ms)
	K_ogain_damp,// int type; range : 1~64; the smaller it is the longer it takes to reach the target gain; default : 64
//====================================
	K_DRB,		// 20230404 add V3.0
	K_micDRB,	// 20220417 add V3.1
	K_mgain,		// main stream gain Sid:mic=0 ,mus=1~2
	K_mgain_damp,	// Sid:mic=0 ,mus=1~2 int type; range : 1~64; the smaller it is the longer it takes to reach the target gain; default : 64
	//output eq
	K_Oeq0_LRSW,// 1 or 0 : on off
	K_Oeq1_LRSW,// 1 or 0 : on off
	K_Oeq2_LRSW,// 1 or 0 : on off

	//output eq		// 20230423 add V3.2
	K_Oeq0L,
	K_Oeq1L,
	K_Oeq2L,
	K_Oeq0R,
	K_Oeq1R,
	K_Oeq2R,
	//i2s3 gain
	K_i2s3gain,//sub idx 0~16(ps:0~15)

}karaMID;	//module ID

typedef enum {
//==iGain ==============
	Adc1_Mic,
	Adc2_Mic,
	Adc3_Mic,
	Adc4_Mic,
	Adc5_Mic,
	I2s2L_Mic,
	I2s2R_Mic,
	I2s3L_Mic,
	I2s3R_Mic,
	Adc2_MusL,
	Adc3_MusR,
	Adc4_MusL,
	Adc5_MusR,
	I2s2L_MusL,
	I2s2R_MusR,
	I2s3L_MusL,
	I2s3R_MusR,
	Play_MusL,
	Play_MusR,
	Tone_I2s2LR,
	Tone_DacLR,
	Usb_MusL,
	Usb_MusR,	//22 V12 add
//==oGain ==============
	Out1L_RecL,
	Out1R_RecR,
	Out2L_RecL,
	Out2R_RecR,
	MusL_Out1L,
	MusR_Out1R,
	MusL_Out2L,
	MusR_Out2R,
	Out1L_DacL,
	Out1R_DacR,
	Out2L_I2s2L,
	Out2R_I2s2R,	// 34
//===========================
	Adc2_I2s3L,		// V30 edit
	Adc4_I2s3L,
	I2s2L_I2s3L,
	I2s3L_I2s3L,
	Adc3_I2s3R,
	Adc5_I2s3R,
	I2s2R_I2s3R,
	I2s3R_I2s3R,
	Max2L_I2s3L,
	Max2R_I2s3R,
//===========================
	Play_I2S3L,
	Play_I2S3R,
	OTG_I2S3L,
	OTG_I2S3R,
	I2s3L_Max12L,
	I2s3R_Max12R,
//===========================
	MicMstVol,
	MusMstVol,
	EndNum
}karaIOGain;	//53
extern int16_t	ioGain[EndNum];
extern int16_t	ioGainR[EndNum];

//***************************************************
typedef struct
{
	uint16_t	type;	// eqFiltType
    uint16_t 	freq;	// 1Hz step
    uint16_t 	Q;		// 0.001 step
	int16_t		gain;	// 0.1db step
} EQ_TAB;		// 8 Byte	==== 變量宣告長度最小占用 uint16_t =====
/*
typedef enum {
	eq_APF,	// all pass
	eq_LPF,	// low pass
	eq_HPF,	// high pass
	eq_LSF,	// low shelf
	eq_HSF,	// high shelf
	eq_BEL	// bell shape
}eqFiltType;	// EQ TYPE 20220930 Add
*/

typedef struct
{
	EQ_TAB	DreEq[32];	//32*8 = 256 + 8 = 264
	uint8_t	MusDreOnOff;
	int8_t	MusDreLowGain;
	int8_t	MusDreHighGain;
	int8_t	NC1;
	uint8_t	MicDreOnOff;
	int8_t	MicDreLowGain;
	int8_t	MicDreHighGain;
	int8_t	NC2;
} DRE_SET;
extern DRE_SET	DreSet;
extern DRE_SET	DreSetR;


extern EQ_TAB	UserEq[80];
extern EQ_TAB	UserEqR[80];
extern EQ_TAB	OutEq[60];
extern EQ_TAB	OutEqR[60];

//***************************************************
typedef struct
{
	float	RevVol;
	float	RevVol2;
	float	EchoVol;
	float	EchoVol2;
	float	drygain;
	float	drygain2;
	float	RevRep;
	int32_t	lex_PreDelay;
	float	lex_iDfus1; 		// q11 feedback gain of first two input allpass filt ramge : 	0~0.95	default: 0.7
	float	lex_iDfus2;			// q11 feedback gain of last two input allpass filt ramge : 	0~0.95	default: 0.5
	float	lex_dDfus1;			// q11 feedback gain of first tank allpass filt ramge : 		0~0.95	default: 0.75
	float	lex_dDfus2;			// q11 feedback gain of second tank allpass filt ramge : 		0~0.95	default: 0.625
	float	lex_Excur;			// q0 LFO amp of first tank allpass's delay line  ramge : 		0~16 	default:8
	float	lex_HFreqDamp;		// q14 High-frequency damping : 								0~1		default: 0.1	高频阻尼
	float	EchoRep;
	int32_t	EchoDeyT;
	int32_t	Fshift;
	int32_t	Pitch;
} EF_SET;	//Len 18*4 = 72
extern EF_SET	UserEf;
extern EF_SET	UserEfR;

typedef struct
{
	uint8_t	Ver;
	uint8_t	StGruup;
	uint8_t	EfGruup;
	uint8_t	EqGruup;
	float	MusicVol;
	float	MicVol;
	float	OutVol;
	float	MicNoiseGate;
	float	MusNoiseGate;
	int8_t	KeyShift;

	float	 VoiceCanGain;
	uint16_t VoiceCanFc0;
	uint16_t VoiceCanFc1;

	uint8_t  VoiceCanEn;
	uint8_t  DuckDet;
	float	 DuckIgain;
	float	 DuckFadeDB;
	uint16_t DuckHtime;

	float	 MicCompT;
	uint8_t	 MicCompR;
	uint8_t	 MicCompW;
	uint16_t MicCompAT;
	uint16_t MicCompRT;
	float	 MusCompT;
	uint8_t	 MusCompR;
	uint8_t	 MusCompW;
	uint16_t MusCompAT;
	uint16_t MusCompRT;
	uint32_t AngInGain;

	uint8_t	MicCh_A;
	uint8_t	MicCh_B;
	uint8_t	PAIR;
	uint8_t	WorkMode;
	uint32_t AchPair;
	uint32_t BchPair;
	uint8_t  PlayInf[16];
	uint8_t  SdCardPlayMode;
	uint8_t  EqChg;
	uint8_t  MusVol32_NC;
	uint8_t  Lang;
	uint8_t  MicVol32_NC;
	uint8_t  MicEqId;
	uint8_t  MicEfEqId;
	uint8_t  OutEqId;
	uint8_t  DreEqId;
	uint8_t  Out1Mix;
	uint8_t  Out2Mix;
	uint8_t  Out3Mix;
	uint8_t  OtgInVol;
	uint8_t  OtgOutVol;
	uint16_t  NC1;
}SYS_INF;	// Len 124

typedef struct
{
	uint8_t	MicCh_A;
	uint8_t	MicCh_B;
	uint8_t	PAIR_En;
	uint8_t CH_Num;
	uint32_t PairId_A;
	uint32_t PairId_B;
	uint32_t PAIR_FREQ_A;
	uint32_t ST_FREQ_A;
	uint32_t PAIR_FREQ_B;
	uint32_t ST_FREQ_B;
}WMIC_INF;	// Len 28

extern char	 LabEf[5][16];	// Ef Lab 80
extern char	 LabEq[3][16];	// Eq Lab 48
extern SYS_INF	SysInf;
extern SYS_INF	SysInfR;


extern uint8_t EF_Mode, EF_ModeR;
extern uint8_t EQ_Mode, EQ_ModeR;
extern uint8_t EF_Busy;

//extern SYS_INF	SysInfR;
//extern EF_SET	UserEfR;

uint8_t EF_Maim();
void EF_EQ_ReSend();
void IOGainSet(uint8_t sid, float gain);
void IOGainSetDamp(uint8_t sid, float gain, uint8_t time);
void EQ_SET(uint8_t index, uint16_t type, float freq, float Q, float gain);

#endif  

