
//#include	"USER_Config.h"
#include	".\RGB_LED\LED_EF_FUN.h"

#ifdef CONFIG_AUD_AMP_DET_FFT

#define RGB_DBG_INFO(fmt,...)       os_printf("[RGB]"fmt, ##__VA_ARGS__)

u16	CH_LW[7];
//	0	1		2		3		4		5		6		7
//	NC	63Hz	160Hz 	400Hz 	1000Hz	2500Hz	6250Hz	16000Hz

u8 CCPRL_R[161];		// UBUS 160回 燈光狀態
u8 MODE_SW=0;
u8 Ext_ID=0;				// 本機 ID

#define Max_CH	1

//==============================================================================================
// ----------	  1   2	  3	  4	  5   6   7   8   9
u8 LED_TYPE[9]={  5,  5,  5,  5,  5,  5,  5,  5,  5};		//燈條模式(0~3) EQ TEST
#ifdef	ZY004	// 最後一個是狀態燈需減1
	u16	LED_LEN[9]={ LED_RGB_NUM-1, 90, 65, 65, 58, 58, 25, 25, 25};	//燈條長度		EQ TEST
	u8 W_Y[9]={	    150,100,100,100,100,100,100,100,100};
#else
	u16	LED_LEN[9]={ LED_RGB_NUM, 90, 65, 65, 58, 58, 25, 25, 25};		//燈條長度		EQ TEST
	u8 W_Y[9]={	    250,100,100,100,100,100,100,100,100};
#endif

u8	LED_FR[9]={   0,  0,  0,  0,  0,  0,  1,  1,  0};		//燈條顯示方向(0=正向,1=反向)
u8 RGB_MODE[9]={  2,  1,  1,  1,  1,  1,  1,  1,  1};		//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
u8 RGB_AUTO[9]={  1,  0,  0,  0,  0,  0,  0,  0,  0};		//聲控自動轉漸變(0,1)
u8 RGB_EF[9]={    4, 15, 15, 15, 15, 15, 15, 15, 15};		//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
u8 RGB_SP[9]={    4,  4,  4,  4,  4,  4,  4,  4,  4};		//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
u8	RGB_AEF[9]={  0,  0,  0,  0,  0,  0,  0,  0,  0};

#if defined(CPS_TL820)
	const u8 AudiOffEf[5]={	1,	15,	1,	0,	150};	// 音樂停止效果
#elif defined (XFN_S930)
	const u8 AudiOffEf[5]={	1,	4,	15,	0,	150};	// 音樂停止效果
#elif defined (MZ_200K)
	const u8 AudiOffEf[5]={	1,	4,	15,	0,	150};	// 音樂停止效果
#else
//						  MODE	EF	SP	AEF	W_Y
	const u8 AudiOffEf[5]={	4,	7,	15,	0,	150};	// 音樂停止效果
#endif

u8	ef_cont[9];

u8	RGB_MODE_R[9];
u8	RGB_SP_R[9];
u8	RGB_EF_R[9];
u8	RGB_AEF_R[9];
u8	W_Y_R[9];		// 亮度 暫存

u8	LED_RR[9];		// RGB 顯示 暫存
u8	LED_GG[9];
u8	LED_BB[9];

u32	RGB_TIME[9];		// 顯示 計時

u16	LED_LEN_Tol;
u8	EF_Pause=0;		// 暫停 效果滾動指標(1 =暫停)

u8	LED_F=0;			// 功能指標
u8	LED_S=0;			// 設定指標

u32	Tempo_1ms_cont=0;


u16	CH_Level[13];

const u8	RGB_EF_n[6][9]={{1,2,3,1,2,3,1,2,3},
							{2,3,4,2,3,4,2,3,4},
							{3,4,5,3,4,1,3,4,1},
							{4,5,6,4,1,2,4,1,2},
							{5,6,1,1,2,3,1,2,3},
							{6,1,2,2,3,4,2,3,4}
};

const u8 RGB_Color[15][3]={	//LEN 48
		{255,160,130},	//0.暖白色(240)	//{255,160,130}
		{255,  0,  0},	//1.紅色(241)
		{  0,160,  0},	//2.綠色(242)
		{  0,  0,255},	//3.藍色(243)
		{255, 255, 0},	//4.黃色(244)
		{255,  0,255},	//5.紫色(245)
		{  0,160,255},	//6.青色(246)
		{255, 20,  0},	//7.桃紅色(247)
		{255, 20, 80},	//8.夢幻粉(248)
		{255, 50,  0},	//9.土豪金(249)
		{150,  0,255},	//10.神秘紫(250)
		{255,255,255},	//11.白色(251)
		{  0,  0,  0},	//12.未定(252)
		{  0,  0,  0},	//13.未定(253)
		{  0,  0,  0},	//14.未定(254)
};

RGB_EF_INF_TAB RGB_EF_INF;
/*****************************************************
 EF=0 	R,RG,G,GB,B,BR 6色漸變20階 20x3	彩色漸變流星
 EF=1	R,G,B,RG,RB,GB 流星10階 6x10  	 	6色流星
 EF=2	白色流星 10階 	 	 				白色流星
 EF=3	R,G,B,RG,RB,GB 單點色 6x10			單點6色

 EF=15	EQ Tab
******************************************************/
#define LCN	1
#define MCN	3
#define HCN	5
//********************************************************************
void RGB_LED_EF_FUN(void)
{
	u8 i;
	// == 讀取音頻 EQ 音量值 ================
	extern uint32_t user_aud_amp_task(void);
	extern int16_t aud_mag_get_freq_low(void) ;
	extern int16_t aud_mag_get_freq_mid(void) ;
	extern int16_t aud_mag_get_freq_high(void);

	CH_Level[LCN] = aud_mag_get_freq_low();
	CH_Level[MCN] = aud_mag_get_freq_mid()*2;
	CH_Level[HCN] = aud_mag_get_freq_high()*4;

	AUDIO_TEMPO_FUN(CH_Level, sys_time_get());

	//各迴路 處理 =========================================================
//	ms_contR=ms_cont;
	for(i=0; i<Max_CH; i++){
		RGB_EF_INF.LedChF = i;
		RGB_EF_INF.RGB_AUTO = &RGB_AUTO[i];
		RGB_EF_INF.W_Y = &W_Y[i];
		RGB_EF_INF.W_Y_R = &W_Y_R[i];
		RGB_EF_INF.RGB_MODE = &RGB_MODE[i];
		RGB_EF_INF.RGB_MODE_R = &RGB_MODE_R[i];
		RGB_EF_INF.RGB_SP = &RGB_SP[i];
		RGB_EF_INF.RGB_SP_R = &RGB_SP_R[i];
		RGB_EF_INF.RGB_EF = &RGB_EF[i];
		RGB_EF_INF.RGB_EF_R = &RGB_EF_R[i];
		RGB_EF_INF.RGB_AEF = &RGB_AEF[i];
		RGB_EF_INF.RGB_AEF_R = &RGB_AEF_R[i];
		RGB_EF_INF.LED_RR = &LED_RR[i];
		RGB_EF_INF.LED_GG = &LED_GG[i];
		RGB_EF_INF.LED_BB = &LED_BB[i];
		RGB_EF_INF.LED_FR = &LED_FR[i]; // @suppress("Field cannot be resolved")
		RGB_EF_INF.RGB_EF_N = (u8*)&RGB_EF_n[i][0];
		RGB_EF_INF.LED_LEN = &LED_LEN[i];
		RGB_EF_INF.RGB_TIME = &RGB_TIME[i];
		RGB_EF_INF.ef_cont = &ef_cont[i];
		RGB_EF_INF.LED_RGB = (u8*)&LED_RGB;
		RGB_EF_INF.CH_Level = (u8*)&CH_Level[RGB_SP[i]];
		RGB_EF_INF.RGB_Color = (u8*)RGB_Color;
		RGB_EF_INF.AudiOffEf = AudiOffEf;
		RGB_EF_INF.Tempo_ms_cont = sys_time_get;

		RGB_LED_PROG_FUN(&RGB_EF_INF);
	}//for(i = 0; i < Max_CH ; i++)


	//===== 白光頻閃時間 =====
//	Subflash(sys_time_get());

	//===== 同步 效果,速度一樣的 CH ===========================
	for(i = 0; i < Max_CH ; i++){
		if(i && RGB_EF[i]==RGB_EF[i-1] && RGB_SP[i]==RGB_SP[i-1] && LED_FR[i]==LED_FR[i-1] && LED_LEN[i]==LED_LEN[i-1] ){
			ef_cont[i]=ef_cont[i-1];
		}
		if(RGB_MODE[i]!=4){
			for(uint8_t j=0; j<LED_LEN[i]; j++){
		#ifdef	CPS_TL820	// 方案名稱
				LED_RGB[j][0] = LED_RR[i];
				LED_RGB[j][1] = LED_RR[i];
				LED_RGB[j][2] = LED_RR[i];
		#else
				LED_RGB[j][0] = LED_RR[i];
				LED_RGB[j][1] = LED_GG[i];
				LED_RGB[j][2] = LED_BB[i];
#endif
			}
		}else{
//			RGB_DBG_INFO("========%d\n",RGB_MODE[i]);
		}
		RgbLedShow();
	}
}

#endif
