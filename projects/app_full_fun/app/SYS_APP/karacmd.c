/**********************************************************************
* 文件:	karacmd.h
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
//#include "USER_Config.h"
#include "karacmd.h"
#ifdef __KARACMD_H
#define CMD_DBG_INFO(fmt,...)	//os_printf("[CMD]"fmt, ##__VA_ARGS__)

/*************************************
* 混响参数
* 全部为int32整形
* 范围和量化精度参考注释
**************************************/
typedef enum {
	lex_PreDelay,			// predelay : 0~100(ms); it is useless now ;
	lex_HighCutFrequency,	// q0 cutoff freq of input one-pole high pass filter : 0~0.5*samplerate

	lex_iDfus1, 	// q11 feedback gain of first two input allpass filt ramge : 	0~0.95
	lex_iDfus2,		// q11 feedback gain of last two input allpass filt ramge : 	0~0.95
	lex_dDfus1,		// q11 feedback gain of first tank allpass filt ramge : 		0~0.95
	lex_dDfus2,		// q11 feedback gain of second tank allpass filt ramge : 		0~0.95
	lex_Excur,		// q0 LFO amp of first tank allpass's delay line  ramge : 		0~16 	default:8

	lex_DecayFactor,			// q15 Decay factor of reverb tail : 	0~1
	lex_HighFrequencyDamping,	// q14 High-frequency damping : 		0~1
	lex_WetDryMix,				// q15 wet dry percentage: 				0~1

}lexRevPN;//REV parameter names

/*********************************************
* MCU to DSP 目前命令的格式：命令长度 12Bytes
*		1. 四字节头 FF FF FF FF
*		2. 两字节 模块id
*		3. 两字节 子模块id
*		4. 四字节 参数内容
*参见main函数例子
*********************************************/
//**** Mcu to Dsp Send CMD struct(def All ByPass)*****************************
typedef struct _eft_cmd_stu_t {
    uint32_t    Head;   // Head
    int16_t     MidF;   // M_ID
    int16_t     SidF;   // S_ID
    union               // VAL
    {
        int32_t ValiF;
        float   ValfF;
    };
} eft_cmd_stu_t __attribute__((aligned(4)));

uint8_t EF_Mode = 0u, EF_ModeR = 10;
uint8_t EQ_Mode = 0u, EQ_ModeR = 10;

uint8_t EF_Busy = 1;

int16_t	ioGain[EndNum];
int16_t	ioGainR[EndNum];

EF_SET	UserEfR;
EF_SET	UserEf ={
		//	RevVol,	RevVol2,	EchoVol,	EchoVol2,	drygain,	drygain2,	RevRep,	lex_PreDelay	lex_iDfus1	lex_iDfus2	lex_dDfus1	lex_dDfus2	lex_Excur	lex_HFreqDamp
			0.f, 	0.f,		0.f,		0.f,		1.0f,		1.0f,		0.4f, 	10u,			0.7f,		0.5f,		0.75f,		0.625f,		8.0f,		0.1f,	\
		/*	EchoRep,	EchoDeyT,		Fshift,	Pitch	*/
			0.4f, 		200u, 		 	0u, 	0u,};

EQ_TAB	UserEqR[80];
EQ_TAB	UserEq[80] ={
			{2,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},

			{2,   20,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1,20000,700,   0},
			{2,   20,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1,20000,700,   0},
			{2,   20,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1,20000,700,   0},
			{2,   20,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1,20000,700,   0},
			{2,   20,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1,20000,700,   0},

			{2,  100,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  280,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1, 4500,700,   0},
			{2,  100,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  280,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{1, 5500,700,   0},
};	// 8*80Bank=640

EQ_TAB	OutEqR[60];
EQ_TAB	OutEq[60] ={
			{5,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},
			{5,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},
			{5,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},
			{5,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},
			{5,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},
			{5,   32,700,   0},{5,   62,700,  0},{5,  125,700,  0},{5,  250,700,  0},{5,  500,700,   0},
			{5, 1000,700,   0},{5, 2000,700,  0},{5, 4000,700,  0},{5, 8000,700,  0},{5,16000,700,   0},
};	// 8*60Bank=480


DRE_SET	DreSetR;
DRE_SET	DreSet ={
       {{5,   62,700,  120},{5,  248,700,-60},{5, 3125,700,-60},{5,12500,400, 80},	// Low Level Music_L
    	{5,   62,700,  120},{5,  248,700,-60},{5, 3125,700,-60},{5,12500,400, 80},	// Low Level Music_R
		{5,   62,700,    0},{5,  248,700,  0},{5, 3125,700,  0},{5,12500,400,  0},	// Detect
		{5,   62,700,    0},{5,  248,700,  0},{5, 3125,700,  0},{5,12500,400,  0},	// High Level Music_L
		{5,   62,700,    0},{5,  248,700,  0},{5, 3125,700,  0},{5,12500,400,  0},	// High Level Music_R
		{5,   62,700,  100},{5,  248,700,  0},{5, 3125,700,  0},{5,12500,400, 80},	// Low Level	Mic
		{5,   62,700,    0},{5,  248,700,  0},{5, 3125,700,  0},{5,12500,400,  0},	// Detect
		{5,   62,700,    0},{5,  248,700,  0},{5, 3125,700,  0},{5,12500,400,  0}},	// High Level
		//	DrbOnOff	DrbLowGain	DrbHighGain	NC
			1,			-20,		0,		0,
		//	DrbOnOff	DrbLowGain	DrbHighGain	NC
			1,			-20,		0,		0,
};	// 8*12Bank=96+4=100*2=200Byte

SYS_INF	SysInfR;
				//	Ver	StGr	EfGr	EqGr	MusVol	MicVol	OutVol	MicNoise	MusNoise	KeyShift	VoCanGain	VoCanFc1	VoCanFc2	VoCanEn	DuckDet	DuckIgain	DuckFadeDB	DuckHtime
SYS_INF	SysInf= { 	1u, 0u,		0u,		0u,		0.0f,	-60.0f,	0.0f,	-60.0f,		-60.0f,		0,			-5.0f,		300u,		3400u,		0u,		0u,		-30.0f,		-20.0f,		2500u, \
				/*	MicCompT	MicCompR	MicCompW	MicCompAT	MicCompRT	MusCompT	MusCompR	MusCompW	MusCompAT	MusCompRT	AngInGain */
					-15.0f,		7u,			5u,			200u,		50u,		-15.0f,		7u,			5u,			200u,		50u, 		0x05050000,\
				/*	MicA		MicB		Pair 	WorkMode	AchPair			BchPair			PlayInf								SdPlayMode	EqChg 	MusVol32	Lang	MicVol32*/
					0,			40,			0,		0,			0x00000701,		0x00000701,		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},	1,			0,		3,			0,		3, \
				/*	MicEqId		MicEfEqId	OutEqId		DreEqID	Out1Mix	Out2Mix	Out3Mix	OtgInVol	OtgOutVol	NC*/
					1,			1,			1,			1,		0,		0,		0,		0,			0,			0,
};

char LabEf[5][16]={"Effect_0        ","Effect_1        ","Effect_2        ","Effect_3        ","Effect_4        "};	// Ef Lab 80
char LabEq[3][16]={"EQ_0            ","EQ_1            ","EQ_2            "};	// Eq Lab 48

uint8_t free_num = 8;
uint8_t CmdWriteEn = 1;
uint8_t CmdFreeCont = 0;
//****************************************************************
void aud_eft_param_set(eft_cmd_stu_t *p_cmd)
{
    extern int mbx_cmd_mcu2dsp_set_single(uint32_t *p_cmd, uint8_t cmd_sz, uint8_t fast_en);
    p_cmd->Head = 0xFFFFFFFF;
	CmdWriteEn = 0;
#if 1
    uint32_t interrupts_info, mask;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    int ret = mbx_cmd_mcu2dsp_set_single((uint32_t*)p_cmd, sizeof(eft_cmd_stu_t), 0);
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    if(ret <= 10) {
    	DBG_LOG_ERR("!!!!!!!!!!!! 1.ret:%d  %d\n", ret, CmdFreeCont);
    }
#else
    int ret = mbx_cmd_mcu2dsp_set_single((uint32_t*)cmd_buff, sizeof(cmd_buff), 1);
#endif
    if(ret <= 0) {
//        extern void sys_debug_show(uint8_t redirect);
//        sys_debug_show(1);
        os_delay_us(8000);	// 5ms
        void PlayWaveStop(uint16_t id);
        if(++CmdFreeCont>10) PlayWaveStop(APP_WAVE_FILE_ID_POWEROFF);
        DBG_LOG_ERR("!!!!!!!!!!!! 2.ret:%d  %d\n", ret, CmdFreeCont);
    }
    CmdWriteEn = 1;
}

//***********************************************************
void EF_EQ_ReSend()
{
//	ByPass(1, K_micDCfilt);
//	ByPass(1, K_micDCfilt);

	EF_ModeR = 0xFF;
	EQ_ModeR = 0xFF;

	SysInfR.MicVol =0xFFFF;
	UserEfR.RevVol =0xFFFF;
	UserEfR.RevVol2 =0xFFFF;

	UserEfR.EchoVol =0xFFFF;
	UserEfR.EchoVol2 =0xFFFF;
	UserEfR.drygain =0xFFFF;
	UserEfR.drygain2 =0xFFFF;

	UserEfR.RevRep =0xFFFF;
	UserEfR.lex_PreDelay =0xFFFF;
	UserEfR.lex_iDfus1 =0xFFFF;
	UserEfR.lex_iDfus2 =0xFFFF;
	UserEfR.lex_dDfus1 =0xFFFF;
	UserEfR.lex_dDfus2 =0xFFFF;
	UserEfR.lex_Excur =0xFFFF;
	UserEfR.lex_HFreqDamp =0xFFFF;

	UserEfR.EchoRep =0xFFFF;
	UserEfR.EchoDeyT =0xFFFF;
	UserEfR.Fshift =0xFFFF;
	UserEfR.Pitch =0xFFFF;

	SysInfR.MusicVol =0xFFFF;
	SysInfR.OutVol =0xFFFF;
	SysInfR.MusNoiseGate =0xFFFF;
	SysInfR.MicNoiseGate =0xFFFF;

	SysInfR.KeyShift = 0xFF;
	SysInfR.VoiceCanEn = SysInf.VoiceCanEn;
	SysInfR.VoiceCanGain = 0xFFFF;
	SysInfR.VoiceCanFc0 = 0xFFFF;
	SysInfR.VoiceCanFc1 = 0xFFFF;
	SysInfR.DuckDet = 0xFF;
	SysInfR.DuckIgain = 0xFFFF;
	SysInfR.DuckFadeDB = 0xFFFF;
	SysInfR.DuckHtime = 0xFFFF;

	SysInfR.MusCompT = 0xFFFF;
	SysInfR.MusCompR = 0xFF;
	SysInfR.MusCompW = 0xFF;
	SysInfR.MusCompAT = 0xFFFF;
	SysInfR.MusCompRT = 0xFFFF;

	SysInfR.MicCompT = 0xFFFF;
	SysInfR.MicCompR = 0xFF;
	SysInfR.MicCompW = 0xFF;
	SysInfR.MicCompAT = 0xFFFF;
	SysInfR.MicCompRT = 0xFFFF;
	SysInfR.AngInGain = 0xFFFF;
//	SysInfR.PlayType = 0xFF;
	DreSetR.MicDreHighGain=0;
	DreSetR.MicDreLowGain=0;
	DreSetR.MusDreHighGain=0;
	DreSetR.MusDreLowGain=0;
	DreSetR.MicDreOnOff=2;
	DreSetR.MusDreOnOff=2;

	SysInfR.Out1Mix = 2;
	SysInfR.Out2Mix = 2;
	SysInfR.Out3Mix = 2;
	SysInfR.WorkMode = 10;

	uint8_t i;
	for(i=0; i<80; i++)	UserEqR[i].freq=0xFFFF;
	for(i=0; i<60; i++)	OutEqR[i].freq=0xFFFF;
	for(i=0; i<32; i++)	DreSetR.DreEq[i].freq=0xFFFF;
	for(i=0; i<EndNum; i++)	ioGainR[i]=0xFFFF;
	UpComputerRW = 0;	// 更新上位機
}

/************************************************
  	index = Eq Id  參考 UComputeer.h EqIndex枚舉
  	type = eq_BEL  參考 karacmd.h eqFiltType枚舉
    freq = 20 ~ 20000 Hz
    Q = 0.7 ~ 20
	gain = -12 ~ 12 db
************************************************/
void EQ_SET(uint8_t index, uint16_t type, float freq, float Q, float gain)
{
	if(index <OutEq0L_0){
		UserEq[index].type = type;
		UserEq[index].freq = freq;
		UserEq[index].Q = (float)Q*1000;
		UserEq[index].gain = (float)gain*10;
	}else if(index <DreEqLL_0){
		index -= OutEq0L_0;
		OutEq[index].type= type;
		OutEq[index].freq = freq;
		OutEq[index].Q = (float)Q*1000;
		OutEq[index].gain = (float)gain*10;
	}else if(index <EQEnd){
		index -= DreEqLL_0;
		DreSet.DreEq[index].type = type;
		DreSet.DreEq[index].freq = freq;
		DreSet.DreEq[index].Q = (float)Q*1000;
		DreSet.DreEq[index].gain = (float)gain*10;
	}
}
#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS
int8_t GetSysMusicVol(void)
{
	return (int8_t)SysInf.MusicVol;
}
#endif

//***********************************************************
uint8_t EF_Maim()
{
	extern int8_t a2dp_get_volume(void);
	extern uint32_t avrcp_has_connection(void);
	extern void current_send_volume_change_response(uint8_t volume_value);
	extern void a2dp_volume_init(int8_t aud_volume);
//	CMD_DBG_INFO("=============/n");
	static float gainO, QO;
	static uint8_t pageR,chR,chR1;
	static fl2hex V1,V2,V3;
	Send_Group = EF_Mode;
	Send_RW = UpComputerRW;
	Send_Id1 = MainPid;
	Send_Id2 = 0;
	V2.f = 0;
	V3.f = 0;
	EF_Busy = 0;
	//=============================================
	if(EF_ModeR != EF_Mode){
		EF_ModeR = EF_Mode;
		EF_ClewToneGr(EF_Mode);	// 提示音
		GroupRead(EF_Mode,0);
		Send_Id1 = CmdPid;
		Send_Id2 = 0;
		Send_Group = EF_Mode;
		memcpy(&SendBuff[12], &LabEf[Send_Group], 16);
		SendUpComp(SendBuff, 32);
		DisPlay_UpData(Send_Id1, 1);	// 顯示更新回調
		EF_Busy = 1;
		return EF_Busy;
	}else if(EQ_ModeR != EQ_Mode){
		EQ_ModeR = EQ_Mode;
		EQ_ClewToneGr(EQ_Mode);	// 提示音
		GroupRead(EQ_Mode,1);
		Send_Id1 = CmdPid;
		Send_Id2 = 1;
		Send_Group = EQ_Mode;
		memcpy(&SendBuff[12], &LabEq[Send_Group], 16);
		SendUpComp(SendBuff, 32);
		DisPlay_UpData(Send_Id1, 2);	// 顯示更新回調
		EF_Busy = 1;
		return EF_Busy;
	//=============================================
	}else if(	SysInfR.VoiceCanGain != SysInf.VoiceCanGain	\
			||	SysInfR.VoiceCanFc0 != SysInf.VoiceCanFc0	\
			||	SysInfR.VoiceCanFc1 != SysInf.VoiceCanFc1){
		SysInfR.VoiceCanGain = SysInf.VoiceCanGain;
		SysInfR.VoiceCanFc0 = SysInf.VoiceCanFc0;
		SysInfR.VoiceCanFc1 = SysInf.VoiceCanFc1;
		VoiceCanSet(SysInf.VoiceCanGain, SysInf.VoiceCanFc0, SysInf.VoiceCanFc1);
		Send_Id1 = SetupPid;
		Send_Id2 = 101;
		V1.f = SysInf.VoiceCanGain;
		V2.f = SysInf.VoiceCanFc0;
		V3.f = SysInf.VoiceCanFc1;
	//=============================================
	}else if(SysInfR.DuckDet != SysInf.DuckDet	\
			||	SysInfR.DuckIgain != SysInf.DuckIgain	\
			||	SysInfR.DuckHtime != SysInf.DuckHtime	\
			||	SysInfR.DuckFadeDB != SysInf.DuckFadeDB){
		SysInfR.DuckDet = SysInf.DuckDet;
		SysInfR.DuckIgain = SysInf.DuckIgain;
		SysInfR.DuckHtime = SysInf.DuckHtime;
		SysInfR.DuckFadeDB = SysInf.DuckFadeDB;

		DuckingSet(SysInf.DuckDet, SysInf.DuckIgain, ((float)SysInf.DuckHtime/1000), SysInf.DuckFadeDB);
		Send_Id1 = SetupPid;
		Send_Id2 = 100;
		V1.f = SysInf.DuckIgain;
		V2.f = SysInf.DuckHtime;
		V3.f = SysInf.DuckFadeDB;
		Send_Val4 = 0;
		Send_Val41 = SysInf.DuckDet;
	//=============================================
	}else if(SysInfR.MicCompR != SysInf.MicCompR	\
			||	SysInfR.MicCompW != SysInf.MicCompW	\
			||	SysInfR.MicCompAT != SysInf.MicCompAT	\
			||	SysInfR.MicCompRT != SysInf.MicCompRT){
		SysInfR.MicCompR = SysInf.MicCompR;
		SysInfR.MicCompW = SysInf.MicCompW;
		SysInfR.MicCompAT = SysInf.MicCompAT;
		SysInfR.MicCompRT = SysInf.MicCompRT;
		MicDrcSet(SysInf.MicCompR, SysInf.MicCompW,(float)SysInf.MicCompAT/1000,(float)SysInf.MicCompRT/1000);
		Send_Id1 = SetupPid;
		Send_Id2 = 102;
		V1.f = SysInf.MicCompT;
		V2.f = SysInf.MicCompAT;
		V3.f = SysInf.MicCompRT;
		Send_Val4 = 0;
		Send_Val41 = SysInf.MicCompR;
		Send_Val42 = SysInf.MicCompW;
	//=============================================
	}else if(SysInfR.MicCompT != SysInf.MicCompT){
		SysInfR.MicCompT = SysInf.MicCompT;
		MicDrcCompTSet(SysInf.MicCompT);
		Send_Id1 = SetupPid;
		Send_Id2 = 102;
		V1.f = SysInf.MicCompT;
		V2.f = SysInf.MicCompAT;
		V3.f = SysInf.MicCompRT;
		Send_Val4 = 0;
		Send_Val41 = SysInf.MicCompR;
		Send_Val42 = SysInf.MicCompW;
	//=============================================
	}else if(SysInfR.MusCompR != SysInf.MusCompR	\
			||	SysInfR.MusCompW != SysInf.MusCompW	\
			||	SysInfR.MusCompAT != SysInf.MusCompAT	\
			||	SysInfR.MusCompRT != SysInf.MusCompRT){
		SysInfR.MusCompR = SysInf.MusCompR;
		SysInfR.MusCompW = SysInf.MusCompW;
		SysInfR.MusCompAT = SysInf.MusCompAT;
		SysInfR.MusCompRT = SysInf.MusCompRT;
		MusDrcSet(SysInf.MusCompR, SysInf.MusCompW,(float)SysInf.MusCompAT/1000,(float)SysInf.MusCompRT/1000);
		Send_Id1 = SetupPid;
		Send_Id2 = 103;
		V1.f = SysInf.MusCompT;
		V2.f = SysInf.MusCompAT;
		V3.f = SysInf.MusCompRT;
		Send_Val4 = 0;
		Send_Val41 = SysInf.MusCompR;
		Send_Val42 = SysInf.MusCompW;
	//=============================================
	}else if(SysInfR.MusCompT != SysInf.MusCompT){
		SysInfR.MusCompT = SysInf.MusCompT;
		MusDrcCompTSet(SysInf.MusCompT);
		Send_Id1 = SetupPid;
		Send_Id2 = 103;
		V1.f = SysInf.MusCompT;
		V2.f = SysInf.MusCompAT;
		V3.f = SysInf.MusCompRT;
		Send_Val4 = 0;
		Send_Val41 = SysInf.MusCompR;
		Send_Val42 = SysInf.MusCompW;
	//=============================================
	}else if(SysInfR.AngInGain != SysInf.AngInGain){
		SysInfR.AngInGain = SysInf.AngInGain;
		AngInOutGainSet(SysInf.AngInGain);
		Send_Id1 = SetupPid;
		Send_Id2 = 104;
		Send_Val4 = SysInf.AngInGain;
	//=============================================
	}else if(SysInfR.Out1Mix != SysInf.Out1Mix){
		SysInfR.Out1Mix = SysInf.Out1Mix;
	//	K_OutEqMixSet(K_Oeq0_LRSW, SysInf.Out1Mix);
		Send_Id1 = SetupPid;
		Send_Id2 = 105;
		Send_Val41 = SysInf.Out1Mix;
	//=============================================
	}else if(SysInfR.Out2Mix != SysInf.Out2Mix){
		SysInfR.Out2Mix = SysInf.Out2Mix;
		K_OutEqMixSet(K_Oeq1_LRSW, SysInf.Out2Mix);
		Send_Id1 = SetupPid;
		Send_Id2 = 106;
		Send_Val41 = SysInf.Out2Mix;
	//=============================================
	}else if(SysInfR.Out3Mix != SysInf.Out3Mix){
		SysInfR.Out3Mix = SysInf.Out3Mix;
		K_OutEqMixSet(K_Oeq2_LRSW, SysInf.Out3Mix);
		Send_Id1 = SetupPid;
		Send_Id2 = 107;
		Send_Val41 = SysInf.Out3Mix;
	//=============================================
	}else if(SysInfR.WorkMode != SysInf.WorkMode){
		SysInfR.WorkMode = SysInf.WorkMode;
		Send_Id2 = 200;
		Send_Val41 = SysInf.WorkMode;
	//=============================================
	}else if(DreSetR.MicDreHighGain != DreSet.MicDreHighGain || \
			 DreSetR.MicDreLowGain != DreSet.MicDreLowGain || \
			 DreSetR.MusDreHighGain != DreSet.MusDreHighGain || \
			 DreSetR.MusDreLowGain != DreSet.MusDreLowGain){
		DreSetR.MicDreHighGain = DreSet.MicDreHighGain;
		DreSetR.MicDreLowGain = DreSet.MicDreLowGain;
		DreSetR.MusDreHighGain = DreSet.MusDreHighGain;
		DreSetR.MusDreLowGain = DreSet.MusDreLowGain;
//		K_DreSet(float Atime, float Rtime, float Low_lev, float High_lev)
		K_DreSet(K_DRB, 0.2f, 0.2f, DreSet.MusDreLowGain, DreSet.MusDreHighGain);
		K_DreSet(K_micDRB, 0.2f, 0.2f, DreSet.MicDreLowGain, DreSet.MicDreHighGain);
		Send_Val41 = DreSet.MusDreHighGain;
		Send_Val42 = DreSet.MusDreLowGain;
		Send_Val43 = DreSet.MicDreHighGain;
		Send_Val44 = DreSet.MicDreLowGain;
		Send_Id1 = DrePid;
		Send_Id2 = 101;
	//=============================================
	}else if(DreSetR.MusDreOnOff != DreSet.MusDreOnOff){
		DreSetR.MusDreOnOff = DreSet.MusDreOnOff;
		ByPass((DreSet.MusDreOnOff^1), K_DRB);
		Send_Id1 = DrePid;
		Send_Id2 = 102;
		Send_Val41 = DreSet.MusDreOnOff;
	//=============================================
	}else if(DreSetR.MicDreOnOff != DreSet.MicDreOnOff){
		DreSetR.MicDreOnOff = DreSet.MicDreOnOff;
		ByPass((DreSet.MicDreOnOff^1), K_micDRB);
		Send_Id1 = DrePid;
		Send_Id2 = 103;
		Send_Val41 = DreSet.MicDreOnOff;
	//====================================================
	}else if(SysInfR.MusicVol != SysInf.MusicVol){
		SysInfR.MusicVol = SysInf.MusicVol;
#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS	//iosVolSync
		Send_Val41 = 32;
		ioGain[MusMstVol] = MusVol_TAB[(uint8_t)SysInf.MusicVol];
		if(avrcp_has_connection() /*&& app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_VOLUME_SYNC)*/){
			a2dp_volume_init(SysInf.MusicVol);
			uint8_t vol = (uint8_t)SysInf.MusicVol*4;
			if(vol>=0x80) vol = 0x7F;
			current_send_volume_change_response(vol);
		}
#else
		Send_Val41 = 0;
		ioGain[MusMstVol] = SysInf.MusicVol;
#endif
		IOGainSetDamp(MusMstVol, ioGain[MusMstVol],64);
		Send_Id2 = 3;
		V1.f = SysInf.MusicVol;
#ifdef	RECED_EN
		if(SysInf.MusicVol>=22){
			ioGain[Out1L_RecL] = 10-(SysInf.MusicVol-22);
			ioGain[Out1R_RecR] = ioGain[Out1L_RecL];
		}else{
			ioGain[Out1L_RecL] = 10;
			ioGain[Out1R_RecR] = ioGain[Out1L_RecL];
		}
#endif
	//====================================================
#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS	//iosVolSync
	}else if(SysInf.MusicVol != a2dp_get_volume() && a2dp_has_connection()){
//	}else if(SysInf.MusicVol != a2dp_get_volume() && a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)){
		uint8_t VolB = a2dp_get_volume();
		USER_DBG_INFO("=== a2dp_get_volume:%d\n",VolB);
//		if(abs(VolB-SysInf.MusicVol)>6){
//			a2dp_volume_init(SysInf.MusicVol);
//			uint8_t vol = (uint8_t)SysInf.MusicVol*4;
//			if(vol>=0x80) vol = 0x7F;
//			current_send_volume_change_response(vol);
//		}else{
			SysInf.MusicVol = VolB;
			SysInfR.MusicVol = SysInf.MusicVol;
			Send_Val41 = 32;
			ioGain[MusMstVol] = MusVol_TAB[(uint8_t)SysInf.MusicVol];
			IOGainSetDamp(MusMstVol, ioGain[MusMstVol],64);
			Send_Id2 = 3;
			Send_RW = 0;	// 同步上位機音量
			V1.f = SysInf.MusicVol;
//		}


#ifdef	RECED_EN
		if(SysInf.MusicVol>=22){
			ioGain[Out1L_RecL] = 10-(SysInf.MusicVol-22);
			ioGain[Out1R_RecR] = ioGain[Out1L_RecL];
		}else{
			ioGain[Out1L_RecL] = 10;
			ioGain[Out1R_RecR] = ioGain[Out1L_RecL];
		}
#endif
#endif
	//=============================================
	}else if(SysInfR.MusNoiseGate != SysInf.MusNoiseGate){
		SysInfR.MusNoiseGate = SysInf.MusNoiseGate;
		K_MusNoiseGateSet(SysInf.MusNoiseGate);
		Send_Id2 = 4;
		V1.f = SysInf.MusNoiseGate;
	//=============================================
	}else if(SysInfR.KeyShift != SysInf.KeyShift){
		SysInfR.KeyShift = SysInf.KeyShift;
		KeyShiftSet(SysInf.KeyShift);	// 伴奏升降調
		Send_Id2 = 5;
		V1.f = SysInf.KeyShift;
	//=============================================
	}else if(SysInfR.OutVol != SysInf.OutVol){
		SysInfR.OutVol = SysInf.OutVol;
		Out_VolSet(SysInf.OutVol);
		Send_Id2 = 6;
		V1.f = SysInf.OutVol;
	//=============================================
	}else if(UserEfR.drygain != UserEf.drygain){
		UserEfR.drygain = UserEf.drygain;
		MicDryVolSet(UserEf.drygain, K_drygain);
		Send_Id2 = 7;
		V1.f = ((float)UserEf.drygain*10);
	//=============================================
	}else if(UserEfR.drygain2 != UserEf.drygain2){
		UserEfR.drygain2 = UserEf.drygain2;
		MicDryVolSet(UserEf.drygain2, K_drygainI2S);
		Send_Id2 = 8;
		V1.f = ((float)UserEf.drygain2*10);
	}else if(SysInfR.MicVol != SysInf.MicVol){
		SysInfR.MicVol = SysInf.MicVol;
#ifdef	MIC_STEP32
		if(SysInf.MicVol<0 || SysInf.MicVol>32)	SysInf.MicVol = 16;
		ioGain[MicMstVol] = MicVol_TAB[(uint8_t)SysInf.MicVol];
		Send_Val41 = 32;
#else
		Send_Val41 = 0;
		ioGain[MicMstVol] = SysInf.MicVol;
#endif
		IOGainSet(MicMstVol, ioGain[MicMstVol]);
		Send_Id2 = 9;
		V1.f = SysInf.MicVol;
	//=============================================
	}else if(SysInfR.MicNoiseGate != SysInf.MicNoiseGate){
		SysInfR.MicNoiseGate = SysInf.MicNoiseGate;
		K_MicNoiseGateSet(SysInf.MicNoiseGate);
		Send_Id2 = 10;
		V1.f = SysInf.MicNoiseGate;
	//=============================================
	}else if(UserEfR.Fshift != UserEf.Fshift){
		UserEfR.Fshift = UserEf.Fshift;
		FreqshiftSet(UserEf.Fshift);	// 频偏防啸叫 6~12Hz
		Send_Id2 = 11;
		V1.f = UserEf.Fshift;
	//=============================================
	}else if(UserEfR.Pitch != UserEf.Pitch){
		UserEfR.Pitch = UserEf.Pitch;
		K_pshiftSet(UserEf.Pitch);
		Send_Id2 = 12;
		V1.f = UserEf.Pitch;
	//=============================================
	}else if(UserEfR.RevVol != UserEf.RevVol){
		UserEfR.RevVol = UserEf.RevVol;
		RevVolSet(UserEf.RevVol, K_revgain);
		Send_Id2 = 13;
		V1.f = ((float)UserEf.RevVol*10);
	//--------------------------------------------
	}else if(UserEfR.RevVol2 != UserEf.RevVol2){
		UserEfR.RevVol2 = UserEf.RevVol2;
		RevVolSet(UserEf.RevVol2,K_revgainI2S);
		Send_Id2 = 14;
		V1.f = ((float)UserEf.RevVol2*10);
	//--------------------------------------------
	}else if(UserEfR.lex_PreDelay != UserEf.lex_PreDelay){
		UserEfR.lex_PreDelay = UserEf.lex_PreDelay;
		RevPreDelaySet(UserEf.lex_PreDelay);
		Send_Id2 = 15;
		V1.f = UserEf.lex_PreDelay;
	//--------------------------------------------
	}else if(UserEfR.RevRep != UserEf.RevRep){
		UserEfR.RevRep = UserEf.RevRep;
		RevRepSet(UserEf.RevRep);
		Send_Id2 = 16;
		V1.f = ((float)UserEf.RevRep*10);
	//--------------------------------------------
	}else if(UserEfR.lex_iDfus1 != UserEf.lex_iDfus1){
		UserEfR.lex_iDfus1 = UserEf.lex_iDfus1;
		RevDfusSet(lex_iDfus1, UserEf.lex_iDfus1);
		Send_Id2 = 17;
		V1.f = ((float)UserEf.lex_iDfus1*100);
	//--------------------------------------------
	}else if(UserEfR.lex_iDfus2 != UserEf.lex_iDfus2){
		UserEfR.lex_iDfus2 = UserEf.lex_iDfus2;
		RevDfusSet(lex_iDfus2, UserEf.lex_iDfus2);
		Send_Id2 = 18;
		V1.f = ((float)UserEf.lex_iDfus2*100);
	//--------------------------------------------
	}else if(UserEfR.lex_Excur !=UserEf.lex_Excur){
		UserEfR.lex_Excur = UserEf.lex_Excur;
		RevExcurSet(UserEf.lex_Excur);
		Send_Id2 = 19;
		V1.f = UserEf.lex_Excur;
	//--------------------------------------------
	}else if(UserEfR.lex_dDfus1 !=UserEf.lex_dDfus1){
		UserEfR.lex_dDfus1 = UserEf.lex_dDfus1;
		RevDfusSet(lex_dDfus1, UserEf.lex_dDfus1);
		Send_Id2 = 20;
		V1.f = ((float)UserEf.lex_dDfus1*100);
	//--------------------------------------------
	}else if(UserEfR.lex_dDfus2 !=UserEf.lex_dDfus2){
		UserEfR.lex_dDfus2 = UserEf.lex_dDfus2;
		RevDfusSet(lex_dDfus2, UserEf.lex_dDfus2);
		Send_Id2 = 21;
		V1.f = ((float)UserEf.lex_dDfus2*100);
	//--------------------------------------------
	}else if(UserEfR.lex_HFreqDamp !=UserEf.lex_HFreqDamp){
		UserEfR.lex_HFreqDamp = UserEf.lex_HFreqDamp;
		RevHFreqDampSet(UserEf.lex_HFreqDamp);
		Send_Id2 = 22;
		V1.f = ((float)UserEf.lex_HFreqDamp*10);
	//=============================================
	}else if(UserEfR.EchoVol != UserEf.EchoVol){
		UserEfR.EchoVol = UserEf.EchoVol;
		EchoVolSet(UserEf.EchoVol, K_echogain);
		Send_Id2 = 23;
		V1.f = ((float)UserEf.EchoVol*10);
	//=============================================
	}else if(UserEfR.EchoVol2 != UserEf.EchoVol2){
		UserEfR.EchoVol2 = UserEf.EchoVol2;
		EchoVolSet(UserEf.EchoVol2, K_echogainI2S);
		Send_Id2 = 24;
		V1.f = ((float)UserEf.EchoVol2*10);
	//=============================================
	}else if(UserEfR.EchoDeyT != UserEf.EchoDeyT){
		UserEfR.EchoDeyT = UserEf.EchoDeyT;
		K_EchoDelayTimeSet(UserEf.EchoDeyT);
		Send_Id2 = 25;
		V1.f = UserEf.EchoDeyT;
	//=============================================
	}else if(UserEfR.EchoRep != UserEf.EchoRep){
		UserEfR.EchoRep = UserEf.EchoRep;
		K_EchoRepSet(UserEf.EchoRep);
		Send_Id2 = 26;
		V1.f = ((float)UserEf.EchoRep*10);
	//===== Id27,28 由 102 103 發送  ===================
	}else if(SysInfR.VoiceCanEn != SysInf.VoiceCanEn){
		SysInfR.VoiceCanEn = SysInf.VoiceCanEn;
		VoiceCan(SysInf.VoiceCanEn);
		Send_Id2 = 29;
		V1.f = SysInf.VoiceCanEn;
	}else{
		//**** EQ SET　*********************************************
		uint8_t i=172;	// userEq:80,OutEq:60ch,DrbMusEq:20,DrbMicEq:12ch
		static uint8_t f=172;
		const uint8_t pageRF[8]={K_Museq,K_Miceq0,K_Miceq1,K_Miceq2,K_Miceq3,K_Miceq4,K_epr_eq,K_epri2s_eq};
		const uint8_t pageRF1[6]={K_Oeq0L,K_Oeq0R,K_Oeq1L,K_Oeq1R,K_Oeq2L,K_Oeq2R};
		const uint8_t pageRF2[8]={K_DRB,K_DRB,K_DRB,K_DRB,K_DRB,K_micDRB,K_micDRB,K_micDRB};
		Send_Group = EQ_Mode;
		while(i--){
			if(++f >=172) f=0;
//			CMD_DBG_INFO("====== f: %d\n", f);
			if(f<80){
				if(UserEqR[f].type != UserEq[f].type || UserEqR[f].freq != UserEq[f].freq || \
						UserEqR[f].Q != UserEq[f].Q || UserEqR[f].gain != UserEq[f].gain){
//test				if(UserEq[f].type&0x10)	UserEq[f].type=0;
//test				UserEq[f].type |= 0x10;
					UserEqR[f].type = UserEq[f].type;
					UserEqR[f].freq = UserEq[f].freq;
					UserEqR[f].Q = 	 UserEq[f].Q;
					UserEqR[f].gain = UserEq[f].gain;
					pageR = (uint8_t)(f/10);
					chR = (uint8_t)(f%10);
					if(chR > 9)	chR = 9;
					gainO = ((float)UserEq[f].gain/10);
					QO = ((float)UserEq[f].Q/1000);
					K_EqSet(pageRF[pageR], chR, gainO, QO, UserEq[f].freq, UserEq[f].type);

					V1.f = ((float)UserEq[f].freq);
					V2.f = QO;	// Q
					V3.f = gainO;  // gain
					Send_Val42 = UserEq[f].type;
					if(pageR == 0){
						Send_Id1 = MusicPid;
						Send_Val41 = 1;
						Send_Id2 = (chR+1);
					}else if(pageR>=1 && pageR<=5){
						Send_Id1 = MicPid;
						Send_Val41 = (1<<(pageR-1));
						if(SysInf.MicEqId==(pageR-1)) Send_Id2 = (chR+1);
					}else if(pageR>=6 && pageR<=7){
						Send_Id1 = EfPid;
						Send_Val41 = (1<<(pageR-6));
						if(SysInf.MicEfEqId==(pageR-6)) Send_Id2 = (chR+1);
					}
//					DBG_LOG_INFO("f:%5.0f  q:%2.3f  g:%2.1f  t:%d  p:%d\n",V1.f,V2.f,V3.f, Send_Val42,pageR);
					break;	// 更新一筆 跳出
				}
			}else if(f<140){	// OutEq
				uint8_t f1 = (f-80);
				if(OutEqR[f1].type != OutEq[f1].type || OutEqR[f1].freq != OutEq[f1].freq || \
						OutEqR[f1].Q != OutEq[f1].Q || OutEqR[f1].gain != OutEq[f1].gain){
					OutEqR[f1].type = OutEq[f1].type;
					OutEqR[f1].freq = OutEq[f1].freq;
					OutEqR[f1].Q = 	 OutEq[f1].Q;
					OutEqR[f1].gain = OutEq[f1].gain;
					pageR = (uint8_t)(f1/10);
					chR = (uint8_t)(f1%10);
					if(chR > 9)	chR = 9;
					gainO = ((float)OutEq[f1].gain/10);
					QO = ((float)OutEq[f1].Q/1000);
#ifdef ZY_OA_001
//					if(f1==21 || f1==31 || f1==22 || f1==32) OutEq[f1].type = 0;
#endif
					K_EqSet(pageRF1[pageR], chR, gainO, QO, OutEq[f1].freq, OutEq[f1].type);
					V1.f = ((float)OutEq[f1].freq);
					V2.f = QO;	// Q
					V3.f = gainO;  // gain
					Send_Val42 = OutEq[f1].type;
					Send_Id1 = OutPid;
					Send_Val41 = (1<<pageR);
					if(SysInf.OutEqId==pageR) Send_Id2 = (chR+1);
				//	DBG_LOG_INFO("f:%5.0f  q:%2.3f  g:%2.1f  t:%d  p:%d\n",V1.f,V2.f,V3.f, Send_Val42,pageR);
					break;	// 更新一筆 跳出
				}
			}else if(f<172){	//DrbMusEq
				uint8_t f2 = f-140;
				if(DreSetR.DreEq[f2].type != DreSet.DreEq[f2].type || DreSetR.DreEq[f2].freq != DreSet.DreEq[f2].freq || \
						DreSetR.DreEq[f2].Q != DreSet.DreEq[f2].Q || DreSetR.DreEq[f2].gain != DreSet.DreEq[f2].gain){
					DreSetR.DreEq[f2].type = DreSet.DreEq[f2].type;
					DreSetR.DreEq[f2].freq = DreSet.DreEq[f2].freq;
					DreSetR.DreEq[f2].Q = 	DreSet.DreEq[f2].Q;
					DreSetR.DreEq[f2].gain = DreSet.DreEq[f2].gain;
					pageR = (uint8_t)(f2/4);
					chR = (uint8_t)(f2%4);
					if(chR > 3)	chR = 3;
					gainO = ((float)DreSet.DreEq[f2].gain/10);
					QO = ((float)DreSet.DreEq[f2].Q/1000);
					if(f2<20){
						chR1 = f2+1;
					}else{
						chR1 = f2-19;
						if(chR1==5)	chR1 = 9;
					}
					K_EqSet(pageRF2[pageR], chR1, gainO, QO, DreSet.DreEq[f2].freq, DreSet.DreEq[f2].type);
					V1.f = ((float)DreSet.DreEq[f2].freq);
					V2.f = QO;	// Q
					V3.f = gainO;  // gain
					Send_Val42 = DreSet.DreEq[f2].type;
					Send_Id1 = DrePid;
					Send_Val41 = (1<<pageR);
					if(SysInf.DreEqId==pageR) Send_Id2 = (chR+1);
				//	DBG_LOG_INFO("f:%5.0f  q:%2.3f  g:%2.1f  t:%d  p:%d\n",V1.f,V2.f,V3.f, Send_Val42,pageR);
					break;	// 更新一筆 跳出
				}
			}
		}
		//**** IOGain SET　*********************************************
		uint8_t ii=EndNum;
		static uint8_t ff=EndNum;
		if(Send_Id2==0){
			while(ii--){
				if(++ff >=EndNum) ff=0;
				if(ioGainR[ff] != ioGain[ff]){
					ioGainR[ff] = ioGain[ff];
					Send_RW = 0;
					Send_Id1 = SetupPid;
					Send_Id2 = ff+1;
					V1.f = (float)ioGain[ff];
					V2.f = 0;
					V3.f = 0;
					IOGainSet(ff, V1.f);
					break;
				}
			}
		}
	}
	//a5a11a5affffffff 04 64 02 00 ffffffff00ff7f47ffffffffff00000015e50c82
	//======================================
	if(Send_Id2){
		Send_Val1 = V1.h;
		Send_Val2 = V2.h;
		Send_Val3 = V3.h;
		SendUpComp(SendBuff, 32);
		DisPlay_UpData(Send_Id1, Send_Id2);	// 顯示更新回調
		EF_Busy = 1;
	}
#if 0
	static uint8_t EF_BusyR;
	if(EF_BusyR != EF_Busy){
		EF_BusyR = EF_Busy;
		DBG_LOG_INFO("==== EF_Busy: %d\n",EF_Busy);
	}
#endif
	return EF_Busy;
}

#endif



