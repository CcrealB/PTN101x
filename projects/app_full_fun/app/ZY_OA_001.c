/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 日期版本:	V16_2	2023-6-30
* 描述：
*
*
*
**********************************************************************/
#include "USER_Config.h"
#ifdef ZY_OA_001

#if defined(ZY004)
	#include "ZY_OA_004_DEF.H"
#elif defined(ZY001)
	#include "ZY_OA_001_DEF.H"
#else
#if defined(ZY003Mini)
	#include "ZY_OA_003_DEF.H"
#else
	#include "ZY_OA_003_DEF.H"
#endif
#endif

static uint8_t	WmPoSw=0;

static uint8_t	LineIn=1;
uint8_t	LineInR=3;
static uint8_t	ChargFlash=0;
static uint8_t	BtLedFlash=0;

static uint8_t  BK_DeyOnCont=0;
static uint16_t BattVn = 0x01;
static uint16_t DelayOffCont=0;
static uint16_t ms1000 = 0;
static int8_t 	EC11_Sw,EC11_SwR;

static uint16_t SysMsCont=0;

static uint8_t UartOff=2;
static uint8_t LowPowerEn=0;
static uint8_t LowPowerCont=0;		// 進入低功耗計數
#define DisplayCont_def	3		// 充電燈顯示延時(S)
static uint8_t DisplayCont=DisplayCont_def;	// 充電燈顯示計時 (S)
static uint16_t PowerDownCont=0;	// 沒聲音 30分 關機計時 (S)
static uint8_t OffPowerCont = 0;

uint8_t work_modeR=10;

static uint16_t PoKeyCont=1001;
uint16_t WMicOffCont=0;
static uint16_t WorkModeCont=0;
static uint16_t FadeInOut=0;
static uint8_t PlayIng=1;

static uint8_t Batt_FULL=0;

#if defined(ZY003) || defined(ZY004)
	static uint8_t ChargingProtocolDelay=0;
#endif
#define	LMVol	6

#ifdef RGB_EF_EN
	static uint8_t LED_n = 0;
#endif

//*****************************************************************************
uint8_t MusVolValR=20;
//								0		1		2		3		4		5		6		7
const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,-14,-12,-10,
//								8		9		10		11		12		13		14		15		16
								-9,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};
//======================================================================================================================
#ifdef ZY001
//======================================================================================================================
int16_t EG20_TAB[16]={ 120, 120, 120, 110, 110,100,  85,  80,  70,  70,  70,  60,  50,  40,  30,  20};
int16_t EG21_TAB[16]={  10,  10,  20,  40, 50,  50,	40,  35,  35,  35,  35,  30,  30,  25,  20,  10};
int16_t EG22_TAB[16]={ -70, -60, -40, -40,-40, -40, -30, -30, -30, -30, -30, -30, -30, -30, -30, -30};
int16_t EG23_TAB[16]={ -50, -40, -25, -20,-20,   0,   0,   0,  10,  15,  15,  15,  15,  15,  15,  20};

int16_t EG44_TAB[16]={ -30, -35, -45, -45, -45, -45, -35, -35, -30, -25, -25, -20, -20, -20, -20, -20};
int16_t EG45_TAB[16]={ -30, -30, -20, -20, -20, -20, -20, -20, -20, -15, -15, -15, -10, -10, -10, -10};
int16_t EG46_TAB[16]={ -60, -60, -50, -50, -50, -50, -45, -45, -40, -35, -35, -35, -30, -30, -20, -20};
int16_t EG47_TAB[16]={ -60, -60, -60, -50, -50, -50, -45, -45, -40, -35, -35, -35, -30, -30, -30, -30};

const int16_t EQ_MUS01_TAB[16]={40,40,40,40,30,20,0,0,0,0,0,0,0,0,0,0};
const int16_t EQ_ACML0_TAB[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,15};

//static uint8 EqValCchg = 0;

const ACM_SET Def_Set1 = {{
//L CH
{4,   35,  700,   0},{0,   45,  700,   0},{0,   90, 2000,  30},{0,  100, 2000,   0},{0,  160, 3000,   0},	// LCh EQ0~4
{0,  380, 3000, -20},{0,  500, 2000,   0},{0,  650,  700, -10},{0,  710,  700,   0},{0,  950, 3000,   0},	// LCh EQ5~9
{0,  800,  700,   0},{0,  850,  700,   0},{0,  950, 2000,   0},{0, 1500,  700, -15},{0, 1800,  700, -15},	// LCh EQ10~14
{0, 1620,  700, -15},{0, 2600, 2000,   0},{0, 3550, 2000,   0},{0, 4500,  700,   0},{3, 1000,  700,   0},	// LCh PEQ0~4
{0,   45,  700,  20},{0,   70,  700,  10},	// DRB Low 	EQ 0~1
{0,  230,  700, -30},{0,  180, 3000,  20},	// DRB High	EQ 0~1
//R CH
{4, 1000,  700,  15},{0,  350,  700,   0},{0,   80,  700,   0},{0,  377, 2000,-120},{0,  230,  700, -25},	// RCh EQ0~4
{0,  100,  700,   0},{0,  400,  700,   0},{0,  500, 2000,   0},{0,  800, 5000,   0},{0,  650, 3000, -15},	// RCh EQ5~9
{0, 1500,  700,   0},{0, 1620,  700, -10},{0, 1800,  700,   0},{0, 3550,  700,   0},{0, 4000, 3000, -10},	// RCh EQ10~14
{0, 4500,  700,   0},{0, 7000, 3000,  -5},{0,12500, 3000,  30},{0, 6000,  700,  -5},{0,17000,  700,   0},	// RCh PEQ0~4
{0, 6000,  700, -20},{0, 1500,  700, -10},	// DRB Low 	EQ 0~1
{0, 3500,  700, -20},{0, 4600,  700, -30}},	// DRB High	EQ 0~1
//	DrbOnOff	DrbLowGain	DrbHighGain	LR_Mix	ClassDGaie	InGaie
	1,		-30,		-10,		1,	29.5f,		0.0f};
#endif

//==================================================================
#ifdef	ZY003CG
	#define WM_PO_EN	GPIO3
	#define Boot_EN		GPIO35
	#define LED1_Bat1	GPIO7
	#define LED2_Bat2	GPIO18
	#define DcInDet		GPIO2
#else
	#define WM_PO_EN	GPIO24
	#define Boot_EN		GPIO22
	#define LED1_Bat1	GPIO2
	#define LED2_Bat2	GPIO3
	#define DcInDet		GPIO18
#endif
	#define LED3_Bat3	GPIO20
	#define LED0_Bat0	GPIO32
	#define ACM8625_EN	GPIO30



#ifdef	ZY004
	#define	fullVn			12600	// mv
	#define	P4Voln			12000
	#define	P3Voln			11250
	#define	P2Voln			10500	// LoPowerDISv 低電解除電壓
	#define	LoPowerENv		9750
	#define	LoPowerOFFv	 	9000
#else
	#define	fullVn			8400	// mv
	#define	P4Voln			8000
	#define	P3Voln			7500
	#define	P2Voln			7000
	#define	LoPowerENv		6500
	#define	LoPowerOFFv	 	6000

#endif
//***************************************************
uint16_t BattVolConvert(uint16_t AdcVol)
{
	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻 BattHR, 對地電阻 BattLR
	AdcVol = SarADC_val_cali_get(AdcVol);	// 校正轉換
	float BattVol = (float)saradc_cvt_to_voltage(AdcVol)/BattRv;
//	USER_DBG_INFO("==== AdcVol:%d  RVol:%d  BattVol:%5.0f\n",AdcVol, saradc_cvt_to_voltage(AdcVol), BattVol);
	return  BattVol;
}

//*************************************************
void BattFun(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE && hci_get_acl_link_count(sys_hdl->unit)==0 && WmPoSw ==0 && usbd_active_get()==0){	//藍芽模式且不连接状态
		if(++PowerDownCont >=1800){
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
		}
		if(PowerDownCont%60==0) USER_DBG_INFO("==== DelayPoOffCont:%d s\n", PowerDownCont);
	}else{
		PowerDownCont=0;
	}

	//===================================================================================
	uint16_t BattVol = BattVolConvert(SarAdcVal[1]);
	static uint16_t saradc_tempe_dataR;
	static uint8_t DcInDetR,Batt_FULLR;
	extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
	//--------------------------------
	fl2hex V1;
	if((abs(SarAdcValR[1]-SarAdcVal[1])>10) || (saradc_tempe_dataR != saradc_tempe_data || \
			DcInDetR != gpio_input(DcInDet) || Batt_FULLR != Batt_FULL)){
		SarAdcValR[1] =SarAdcVal[1];
		saradc_tempe_dataR = saradc_tempe_data;
		DcInDetR = gpio_input(DcInDet);
		Batt_FULLR = Batt_FULL;

		Send_Id1 = MainPid;
		Send_Id2 = 205;
		Send_Group = 1;
		V1.f = (float)BattVol/1000;
		Send_Val1 = V1.h;
		V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 轉換溫度值
		Send_Val2 = V1.h;
		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
			V1.f = app_player_get_playtime();	// PLAY_TIME
			Send_Val3 = V1.h;
		}else{
			Send_Val3 = 0;
		}
		Send_Val41 = gpio_input(DcInDet);
		Send_Val42 = Batt_FULL;
		SendUpComp(SendBuff, 32);
	}
	if(DisplayCont || gpio_input(DcInDet)){
		if(DisplayCont) USER_DBG_INFO("==== BATT ADC:%d  %d mv  DcIn:%d  Batt_FULL:%d\n",SarAdcVal[1],BattVol,gpio_input(DcInDet),Batt_FULL);
		if(DisplayCont)	DisplayCont--;
		if(ChargFlash==0){
		#ifdef ChargeFullGpio
			Batt_FULL = !gpio_input(ChargeFullGpio);
		#else
			if(BattVol>=fullVn)	Batt_FULL = 1;	// 8.375V 滿電
				else Batt_FULL =0;
		#endif
				if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
				BattVn=0x0F;
			}else if(BattVol>=P3Voln){	// 7.5	11.25
				BattVn=0x07;
			}else if(BattVol>=P2Voln){	// 7	10.5
				BattVn=0x03;
	//		}else if(SarAdcVal[1]>=2740){
	//			BattVn=0x01;
			}else{
				BattVn=1;
			}
			ChargFlash = 1;
		}else if(ChargFlash && gpio_input(DcInDet)){
			if(((BattVn>>1)&1)==0){
				BattVn=0x03;
			}else if(((BattVn>>2)&1)==0){
				BattVn=0x07;
			}else if(((BattVn>>3)&1)==0){
				BattVn=0x0F;
				ChargFlash = 0;
			}else{
				ChargFlash = 0;
			}
		}else{
			ChargFlash = 0;
		}
//		USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[1]:%d\n",BattVn,SarAdcVal[1]);
		if(DisplayCont || Batt_FULL==0){	// 顯示 或 未滿電
			gpio_output(LED0_Bat0, (BattVn&1));
			gpio_output(LED1_Bat1, ((BattVn>>1)&1));
			gpio_output(LED2_Bat2, ((BattVn>>2)&1));
			gpio_output(LED3_Bat3, ((BattVn>>3)&1));
		}else if(gpio_input(DcInDet) && Batt_FULL){
			gpio_output(LED0_Bat0, 1);
			gpio_output(LED1_Bat1, 1);
			gpio_output(LED2_Bat2, 1);
			gpio_output(LED3_Bat3, 1);
		}else{
			gpio_output(LED0_Bat0, 0);
			gpio_output(LED1_Bat1, 0);
			gpio_output(LED2_Bat2, 0);
			gpio_output(LED3_Bat3, 0);
		}
	}else{
		static uint8_t LowPowerFlash=1;
		if(BattVol<LoPowerENv){	// 6.5v	9.75
			gpio_output(LED0_Bat0, LowPowerFlash^=1);
		}else{
			gpio_output(LED0_Bat0, 0);
			gpio_output(LED1_Bat1, 0);
			gpio_output(LED2_Bat2, 0);
			gpio_output(LED3_Bat3, 0);
		}
	}

	//==== 判斷進入 LowPower Mode =======================
	if(LowPowerEn==0 && BattVol<LoPowerENv){		// 6.5v		9.75
		if(++LowPowerCont > 5){
			LowPowerEn=1;
			DelayOffCont=0;
			OffPowerCont = 0;
			USER_DBG_INFO("==== LowPower ModeT ADC:%d  %d\n",SarAdcVal[1],BattVol);
		}
	}else{
		LowPowerCont=0;
		if(LowPowerEn && SarAdcVal[1]>=P2Voln){	// 低電解除
			LowPowerEn=0;
			USER_DBG_INFO("==== UnLowPower ModeT ADC:%d  %d\n",SarAdcVal[1],BattVol);
		}
	}
	//----------------------------------------------------
	if(LowPowerEn && SysInf.MusicVol>LMVol){	// 限制功率	//yuan41
		app_handle_t sys_hdl = app_get_sys_handler();
		if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
			while(SysInf.MusicVol>LMVol){	//yuan41
				app_button_sw_action(BUTTON_BT_VOL_M);
			}
		}else{
			SysInf.MusicVol=LMVol;	//yuan41
		}
		app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
		USER_DBG_INFO("==== MusicVol:%d   %d\n",SysInf.MusicVol, LowPowerEn);	//yuan41
	}
	//===============================================================
	if(LowPowerEn){	// 低電
		if(++DelayOffCont > 300){	// 5s
			DelayOffCont=0;
			DisplayCont = DisplayCont_def;
			if(gpio_input(DcInDet)==0)	app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 充電中不報低電提示音
		}
		if(BattVol<LoPowerOFFv){	// 6v
			USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[1],BattVol, OffPowerCont);
			if(++OffPowerCont==6){
				app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
			}else if(OffPowerCont>7){
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
			}
		}else{
			OffPowerCont = 0;
		}
	}else{
		DelayOffCont=0;
	}
}
#ifdef ZY001
//*****************************************************************************
void AudioCurveAdj(uint8_t val)
{
	UpComputerRW = 0;
	if(val)	val--;
	uint8_t gain = (val/2);
	if(gain>15) gain=15;

	ACM_Set[ACM862xWId].EQ[20].gain = EG20_TAB[gain];
	ACM_Set[ACM862xWId].EQ[21].gain = EG21_TAB[gain];
	ACM_Set[ACM862xWId].EQ[22].gain = EG22_TAB[gain];
	ACM_Set[ACM862xWId].EQ[23].gain = EG23_TAB[gain];

	ACM_Set[ACM862xWId].EQ[44].gain = EG44_TAB[gain];
	ACM_Set[ACM862xWId].EQ[45].gain = EG45_TAB[gain];
	ACM_Set[ACM862xWId].EQ[46].gain = EG46_TAB[gain];
	ACM_Set[ACM862xWId].EQ[47].gain = EG47_TAB[gain];

	ACM_Set[ACM862xWId].EQ[24].gain = EQ_ACML0_TAB[gain];

	UserEq[MusicEq1].gain = EQ_MUS01_TAB[gain];
}
#endif

static uint8_t KeyId = BUTTON_BT_PLAY_PAUSE;
//*****************************************************************************
void FadeInOut_Fun()
{
	FadeInOut--;
	if(FadeInOut==199){
		if(KeyId==BUTTON_BT_PLAY_PAUSE){
			if(app_is_bt_mode() || app_is_mp3_mode()){
				if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0 && player_get_play_status()==0){	// 無播放
					app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
					FadeInOut = 51;	// 執行淡入
				}else{	// 播放中
					if(app_is_bt_mode()){
						app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
					}
					PlayIng =1;
				}
			}else if(app_is_line_mode()){	// LINE MODE
				PlayIng ^=1;
				if(PlayIng)	FadeInOut = 51;	// UMUTE
			}
		}else{
			PlayIng =1;
		}
	}else  if(FadeInOut==198){
		IOGainSetDamp(MusMstVol, -90,10);	// Fade Out
		if(PlayIng==0)	FadeInOut = 1;
	}else  if(FadeInOut==150){				// Fade Out End
		if(KeyId==BUTTON_MODE_CHG){
			system_work_mode_change_button();
			WorkModeCont = 99;
		}else if(KeyId==BUTTON_BT_PREV || KeyId==BUTTON_BT_NEXT|| KeyId==BUTTON_BT_NEXT|| KeyId==BUTTON_BT_NEXT){
			if(app_is_bt_mode() || app_is_mp3_mode()){
				app_button_sw_action(KeyId);
			}
		}else if(KeyId==BUTTON_BT_PLAY_PAUSE){
			if((app_is_mp3_mode()&&player_get_play_status())){
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			}
			if(app_is_bt_mode()==0)	FadeInOut =1;
//				else				FadeInOut =100;
		}
	}else if(FadeInOut==50){	// Fade In
		IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],5);	//yuanV41
	}else if(FadeInOut==0){
		ioGain[MusMstVol]=MusVol_TAB[(uint8_t)SysInf.MusicVol];	//yuanV41
	}
}

#if LED_RGB_NUM
	static uint8_t RgbFlash=0;
#endif
void Encoder_EC11_Init(unsigned char Set_EC11_TYPE);
char Encoder_EC11_Scan();

//********************************************************
void EC11_FUN(void)
{
	if((EC11_Sw==0) && (EC11_SwR==2)){
		if(PoKeyCont>=10){
			if(PoKeyCont>1000){
				PoKeyCont=0;
				return;
			}
			PoKeyCont=0;
			DisplayCont = DisplayCont_def;
			KeyId = BUTTON_BT_PLAY_PAUSE;
			if(FadeInOut<150)	FadeInOut = 200;
		}
		USER_DBG_INFO("====P. EC11_Sw: %d  %d  %d\n",EC11_Sw, EC11_SwR, PoKeyCont);
	}else if((EC11_Sw !=2) && (EC11_SwR != EC11_Sw)){
		EC11_SwR = EC11_Sw;
		app_handle_t sys_hdl = app_get_sys_handler();
		if(EC11_SwR==1){
			if(LowPowerEn==0 || (LowPowerEn && SysInf.MusicVol<LMVol)){	//yuanV41
				if(hci_get_acl_link_count(sys_hdl->unit))	app_button_sw_action(BUTTON_BT_VOL_P);// BT
					else	if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
				UpComputerRW = 0;	// 更新上位機
			}else{
				app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
			}
			USER_DBG_INFO("====U. EC11_Sw: %d  %3.1f\n",EC11_SwR,SysInf.MusicVol);
		}else if(EC11_SwR==-1){
			if(hci_get_acl_link_count(sys_hdl->unit))	app_button_sw_action(BUTTON_BT_VOL_M);	// BT MODE
				else		if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
			USER_DBG_INFO("====D. EC11_Sw: %d  %3.1f\n",EC11_SwR,SysInf.MusicVol);
		}
		PoKeyCont=0;
		DisplayCont = DisplayCont_def;
		UpComputerRW = 0;	// 更新上位機
	}else if(EC11_Sw==2){
		PoKeyCont++;
		if(PoKeyCont==100){	// 1s
			USER_DBG_INFO("==== EC11_Sw: %d  %d\n",EC11_Sw, PoKeyCont);
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
		}
	}
//	if(EC11_SwR != EC11_Sw) USER_DBG_INFO("==== EC11_Sw: %d  %d  %d\n",EC11_Sw, EC11_SwR, PoKeyCont);
	EC11_SwR = EC11_Sw;
}

//*****************************************************
void user_WorkModefun(void)
{
	//---- LINE DET ---------------------------
	static uint8 LineInDetCont=0;
	LineIn = gpio_input(LINE_DETECT_IO);
//	USER_DBG_INFO("====1. LineIn: %d\n",LineIn);
	if(LineIn){
		gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
		os_delay_us(500);
		if(gpio_input(LINE_DETECT_IO)) LineIn = 0;
//		USER_DBG_INFO("====2. LineIn: %d\n",LineIn);
		gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	}
	if(LineInR != LineIn){
		if(++LineInDetCont>=20){
			LineInDetCont=0;
			LineInR = LineIn;
			if(LineInR){
				if(app_is_line_mode()){
					system_work_mode_change_button();
				}
			}else{
				system_work_mode_set_button(SYS_WM_LINEIN1_MODE);
			}
//			USER_DBG_INFO("==== LineIn %d\n",LineInR);
		}
	}else{
		LineInDetCont=0;
	}

	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
#ifdef ZY004
	#define BtDefVol	0
#else
	#define BtDefVol	-5
#endif

		app_handle_t sys_hdl = app_get_sys_handler();
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				RgbLedOneColour(ModeLedAddr,3);	//B
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= BtDefVol;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				RgbLedOneColour(ModeLedAddr,3);	//B
			}else{
				if(BtLedFlash^=1)	RgbLedOneColour(ModeLedAddr,3);	//RgbLedOut(5);	// B
					else			RgbLedOneColour(ModeLedAddr,0);	//RgbLedOut(8);
			}
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				RgbLedOneColour(ModeLedAddr,4);	//W
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= BtDefVol;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			//-----------------------------------
			if(RgbFlash){	//2=閃-次		4=閃兩次
				RgbFlash--;
				if(RgbFlash%2==0)	RgbLedOneColour(ModeLedAddr,4);	// RgbLedOut(6);	// W
					else			RgbLedOneColour(ModeLedAddr,0);	// RgbLedOut(8);	// B
				WorkModeCont = 50;
			}
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				RgbLedOneColour(ModeLedAddr,2);	// G
			//	if(ioGain[Adc2_MusL] < - 15){
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -4;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
				PlayIng =1;
			}
		}
	}
}

#if (USB0_FUNC_SEL == USBD_CLS_AUDIO_HID)
	#define CHG_UART_RX_OFF
	#define CHG_UART_TX_OFF
#endif

//**** pre init ***********************
void user_init_prv(void)
{
#if defined(ZY003)||defined(ZY004)
	#define POWER_EN	GPIO31
	#define USB_SW		GPIO13
	gpio_config_new(USB_SW, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(USB_SW, 0);
#else
	#define POWER_EN	GPIO7
#endif
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(ACM8625_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(Boot_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 0);
	gpio_output(ACM8625_EN, 0);
	gpio_output(Boot_EN, 0);
	gpio_output(POWER_EN, 0);

	#define PowerKey	GPIO15
	gpio_config_new(PowerKey, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

#ifdef ChargeFullGpio
	gpio_config_new(ChargeFullGpio, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
#endif

	BK3000_set_clock(CPU_CLK_XTAL, CPU_CLK_DIV);
	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
#ifdef   CHG_UART_RX_OFF
	if(gpio_input(DcInDet)){	// 充電時關閉  UART IO 必免與 USB充電芯片 USB 接口衝突
		system_peri_mcu_irq_disable(SYS_PERI_IRQ_UART0);
		gpio_config_new(GPIO1, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);	// RX
#ifdef CHG_UART_TX_OFF
		gpio_config_new(GPIO0, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);	// TX
#endif
	}
#endif

	//===============================================================================================
	if(gpio_input(DcInDet) && gpio_input(PowerKey)){	// USB供電啟動,電源鍵 沒按下,進入假開機充電模式
		uint16_t cont=1000;
		for(uint16_t i=0; i<3000; i++){
			CLEAR_WDT; //清除看门狗，默约38.8sec
			os_delay_us(120);	//1ms 延遲 3秒 激活已保護輸出電池
			if(gpio_input(PowerKey)==0)	i=3000;
		}
		gpio_config_new(LED0_Bat0, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		gpio_config_new(LED1_Bat1, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		gpio_config_new(LED2_Bat2, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		gpio_config_new(LED3_Bat3, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		DisplayCont=10;
		//============================================
		while(1){
			CLEAR_WDT; //清除看门狗，默约38.8sec
			//---------------------------------------
			if(gpio_input(PowerKey)==0){	// 假開機充電, 按電源鍵重新啟動流程
				USER_DBG_INFO("======== 1 =========\n");
				dsp_shutdown();
				REG_SYSTEM_0x1D &= ~0x2;
				system_wdt_reset_ana_dig(1, 1);
				BK3000_wdt_reset();
				os_delay_us(3200);	//1ms
				USER_DBG_INFO("======== 2 =========\n");
				break;
			}
			os_delay_us(120);	//1ms
			if(++cont >= 1000){
				cont = 0;

				if(ChargFlash==0){
					SarAdcVal[1] = SarADC_val_cali_get(sar_adc_voltage_get(SARADC_CH_GPIO23));
					if(SarAdcVal[1]==0) SarAdcVal[1] = SarAdcValR[1];	// 防止誤讀 0 值
					SarAdcValR[1] = SarAdcVal[1];
					uint16_t BattVol = BattVolConvert(SarAdcVal[1]);
				#ifdef ChargeFullGpio
					Batt_FULL = !gpio_input(ChargeFullGpio);
				#else
					if(BattVol >= fullVn)	Batt_FULL = 1;
						else 				Batt_FULL = 0;
				//	Batt_FULL = !vusb_2v_ready();
				#endif
					if(BattVol >= fullVn && Batt_FULL){
						if(DisplayCont){
							DisplayCont--;
							BattVn=0x0F;
						}else{
							gpio_output(LED0_Bat0, 0);
							gpio_output(LED1_Bat1, 0);
							gpio_output(LED2_Bat2, 0);
							gpio_output(LED3_Bat3, 0);
							continue;
						}
						app_button_sw_action(BUTTON_BT_POWERDOWN);	// 假開機充電, 滿電後 關機
					}else if(BattVol >= P3Voln){
						BattVn=0x07;
					}else if(BattVol >= P2Voln){
						BattVn=0x03;
				//	}else if(SarAdcValR[1]>=2540){
				//		BattVn=0x01;
					}else{
						BattVn=1;
					}
					ChargFlash = 1;
				}else{
					if(((BattVn>>1)&1)==0){
						BattVn=0x03;
					}else if(((BattVn>>2)&1)==0){
						BattVn=0x07;
					}else if(((BattVn>>3)&1)==0){
						BattVn=0x0F;
						ChargFlash = 0;
					}else{
						ChargFlash = 0;
					}
				}
				gpio_output(LED0_Bat0, (BattVn&1));
				gpio_output(LED1_Bat1, ((BattVn>>1)&1));
				gpio_output(LED2_Bat2, ((BattVn>>2)&1));
				gpio_output(LED3_Bat3, ((BattVn>>3)&1));
			}
		}
	}
	PowerDownFg=0;
	EC11_SwR = EC11_Sw;
}




//**** normal init ********************
void user_init(void)
{
	gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE); // 使用GPIO2 需再設置一次, LINE設置時會被初始成 OUT 模式
	//===========================================
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	if(SysInf.MusicVol<3)SysInf.MusicVol=3;	//yuan41

	appPlayerPlayMode = SysInf.SdCardPlayMode;
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

	LineInR = gpio_input(LINE_DETECT_IO);

	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;
	USER_DBG_INFO("======== V33_01_CE user_init End MsCont:%d WorkMode:%d  LineInR:%d\n",SysMsCont, SysInf.WorkMode, LineInR);
	LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

#if LED_RGB_NUM
	app_i2s0_init(1);
	RgbLedOneColour(ModeLedAddr,0);	//RgbLedOut(8);
#endif

}

//static void key_function(KEY_CONTEXT_t *pKeyArray);
//********************************
void user_init_post(void)
{
	Hi2c_Init();
	ChipEn[0]=0;
	ChipEn[1]=0;
	WmPoSw=0;
	//============================
//	user_key_init(key_function);
	//============================
	Encoder_EC11_Init(0);

	UartOff=2;

	gpio_config_new(LED0_Bat0, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED1_Bat1, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED2_Bat2, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED3_Bat3, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//	gpio_output(LED0_Bat0, 1);
//	gpio_output(LED1_Bat1, 1);
//	gpio_output(LED2_Bat2, 1);
//	gpio_output(LED3_Bat3, 1);

}

//**********************************************
void user_loop(void)
{
    static uint32_t t_mark = 0;
    if(sys_timeout(t_mark, 10)){	//10ms task
        t_mark = sys_time_get();
        extern void user_loop_10ms(void);
        user_loop_10ms();
    }
}


//*******************************************
void tick_task_1ms(void) //timer0 1ms isr
{
	if(PowerDownFg) return;
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
	if(Tick_2ms){	// 2ms End
		user_saradc_update_trig();	// Adc 讀取觸發
	}else{
		if(EC11_SwR==EC11_Sw)	EC11_Sw = Encoder_EC11_Scan();
	}
	SysMsCont++;
}

static uint16_t Init_Cont = 0;
//**********************************************
void user_loop_10ms(void)
{
#ifdef   CHG_UART_RX_OFF
	if(UartOff !=0 && gpio_input(DcInDet)){
		UartOff=0;
		system_peri_mcu_irq_disable(SYS_PERI_IRQ_UART0);
		gpio_config_new(GPIO1, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);	// RX
#ifdef CHG_UART_TX_OFF
		gpio_config_new(GPIO0, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);	// TX
#endif
#if defined(ZY003) || defined(ZY004)
		ChargingProtocolDelay=3;
		gpio_output(USB_SW, 0);
#endif
	}else if(UartOff !=1 && gpio_input(DcInDet)==0){
		UartOff=1;
#if defined(ZY003) || defined(ZY004)
		gpio_output(USB_SW, 0);
		ChargingProtocolDelay=0;

#else
		uart_gpio_enable(); //使能串口
		system_peri_mcu_irq_enable(SYS_PERI_IRQ_UART0);
#endif
		DisplayCont = DisplayCont_def;
	}
#endif

	if(PowerDownFg)	return;
	//======================================
	if(Init_Cont == 150){
		SysMsCont=0;
#ifdef	RGB_EF_EN
		//============================
		static uint8_t led_30ms=0;
		if(++led_30ms >=3){
			led_30ms=0;
			void RGB_LED_EF_FUN(void);
			RGB_LED_EF_FUN();
		}
#endif
		//======================
		if(EF_Maim()) return;
		//======================
		if(ACM_main()) return;

		//======================
		EC11_FUN();
		//======================
		user_WorkModefun();
		//===================
		if(FadeInOut)	FadeInOut_Fun();
		//===================
		BK9532_Main();
		//======================
		user_key_scan();

		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;

		#if defined(ZY003) || defined(ZY004)
			//===============================
			if(ChargingProtocolDelay){
				ChargingProtocolDelay--;
				if(usbd_active_get()){
					ChargingProtocolDelay=0;
				}else{
					if(ChargingProtocolDelay==0)	gpio_output(USB_SW, 1);
				}
			}
		#endif
			app_handle_t sys_hdl = app_get_sys_handler();
			//==== 無線麥閒置計時關閉 20分鐘 ==================
			if(WMicOffCont){
				WMicOffCont--;
				if(WMicOffCont==0){
					WmPoSw=0;
					if(SysInf.Lang){
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_5);
					}else{
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
					}
				}
			}

			//==== 數字功放訊息報告 ====
			ACM_REPORT();

	#if 0
			//----- 沒聲音偵測  30分 關機 ------------------------
			extern uint32_t user_aud_amp_task(void);
			if(user_aud_amp_task()){
				PowerDownCont=0;
			}else{
				if(++PowerDownCont >=1800){
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
				}
				if(PowerDownCont%60==0) USER_DBG_INFO("==== DelayPoOffCont:%d  Vol:%d\n", PowerDownCont, user_aud_amp_task());
			}
	#else
			//-----  輸入音音量自動調整 ------------------------
			extern uint32_t user_aud_amp_task(void);
			static uint16_t OverVol,OverVolW;
			OverVolW = user_aud_amp_task();
			if(OverVol<OverVolW){
				OverVol = OverVolW;
			}else{
				if(OverVol)	OverVol*=0.9;
			}
			USER_DBG_INFO("====  AmpTaskVol:%d  %d\n", OverVol,OverVolW);

			if(OverVol > 7000){
				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){	// AUX
					if(ioGain[Adc2_MusL]>-15){
						ioGain[Adc2_MusL]--;
						ioGain[Adc3_MusR] = ioGain[Adc2_MusL];
					}
				}else{
		//			if(ioGain[Play_MusL]>-15){
		//				ioGain[Play_MusL]--;
		//				ioGain[Play_MusR] = ioGain[Play_MusL];
		//			}
				}
			}else if(OverVol> 10 && OverVol < 2000 && SysInf.MusicVol<=25){	//yuan41
				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){	// AUX
					if(ioGain[Adc2_MusL]<-4){
						ioGain[Adc2_MusL]++;
						ioGain[Adc3_MusR] = ioGain[Adc2_MusL];
					}
				}else{
		//			if(ioGain[Play_MusL]<-5){
		//				ioGain[Play_MusL]++;
		//				ioGain[Play_MusR] = ioGain[Play_MusL];
		//			}
				}
			}else if(OverVol> 1000 && OverVol < 2000 && SysInf.MusicVol>25){	//yuan41
				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){	// AUX
					if(ioGain[Adc2_MusL]<-4){
						ioGain[Adc2_MusL]++;
						ioGain[Adc3_MusR] = ioGain[Adc2_MusL];
					}
				}else{
		//			if(ioGain[Play_MusL]<0){
		//				ioGain[Play_MusL]++;
		//				ioGain[Play_MusR] = ioGain[Play_MusL];
		//			}
				}
			}
	//		USER_DBG_INFO("====  AmpTaskVol:%d\n", user_aud_amp_task());
	#endif
			//====================
			BattFun();
		}	//end 1s

#ifdef	ZY001
		//==== 音頻DRB曲線調整 ===============================
		if(MusVolValR != SysInf.MusicVol){	//yuan41
			MusVolValR = SysInf.MusicVol;
			AudioCurveAdj(SysInf.MusicVol);
			USER_DBG_INFO("==== MusicVol:%d db\n",MusVol_TAB[(uint8_t)SysInf.MusicVol]);
		}
#endif

		//==== BK 無線麥啟用  ================================
		if(BK_DeyOnCont){
			BK_DeyOnCont--;
			if(BK_DeyOnCont==139){
				if(SysInf.Lang){
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_9);
				}else{
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}
			}else if(BK_DeyOnCont==28){
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x00);	// Stop
		#ifdef ZY004
				ACM862xWId = 1;
				SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x00);	// Stop
				ACM862xWId = 0;
				SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		#endif
				//	ACM_Set.EQ[19].freq = 1200;
				//0	ACM_Set.EQ[44].gain = 20;
			}else if(BK_DeyOnCont==26){
				gpio_output(WM_PO_EN, WmPoSw);
			}else if(BK_DeyOnCont==6){
				BK9532_Init();
			}else if(BK_DeyOnCont==1){
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x03);	// Play
			#ifdef ZY004
				ACM862xWId = 1;
				SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
				I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x03);	// Play
				ACM862xWId = 0;
				SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
			#endif
			}
		}

		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//======================================================================
	}else{
		Init_Cont++;

		if(Init_Cont == 1){
			gpio_output(ACM8625_EN, 1);
			USER_DBG_INFO("==== 1. Init_Cont MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 16){
			ACM8625_init();
#ifdef ZY003Mini
			//======= float		L2L,	R2L, 	L2R, 	R2R  ==== -110~+48db =========
			ACM86_InputMixer(	-6,		-6,		-110,	-110);
#endif
			USER_DBG_INFO("==== 16. ACM8625_init Ok... MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 17){
			if(ACM_main()){
				Init_Cont--;
			}else{
				gpio_output(Boot_EN, 1);
				USER_DBG_INFO("==== 17. ACM.1_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
#ifdef ZY004
		}else if(Init_Cont == 18 && ACM862x_IIC_ADDR[1]){
		    ACM862xWId = 1;
		    SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    if(ACM_main()){
		    	Init_Cont--;
		    }else{
		    	ACM862xWId = 0;
		    	SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    	gpio_output(Boot_EN, 1);
		    	USER_DBG_INFO("==== 18. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
		    }
#endif
		}else if(Init_Cont == 19){
			EF_EQ_ReSend();
			EF_ModeR = EF_Mode;
			EQ_ModeR = EQ_Mode;
			USER_DBG_INFO("==== 13. EQ_ReSend Ok...MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 20){
			if(gpio_input(PowerKey)==0){
				USER_DBG_INFO("====14. Init IoGain...\n",SysMsCont);
				// 需等待 DSP 啟動完成才可開始配置 gain
				IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
				IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
				IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],64);
				IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);
				IOGainSetDamp(MusL_Out2L, -90,64);
				IOGainSetDamp(MusR_Out2R, -90,64);
				IOGainSetDamp(Play_MusL, -90,64);
				IOGainSetDamp(Play_MusR, -90,64);
				IOGainSetDamp(Adc2_MusL, -90,64);
				IOGainSetDamp(Adc3_MusR, -90,64);
				ioGain[MusL_Out1L] = 0;
				ioGain[MusR_Out1R] = 0;

				gpio_output(POWER_EN, 1);
				app_wave_file_play_start(APP_WAVE_FILE_ID_START);
				gpio_output(LED0_Bat0, 1);
				gpio_output(LED1_Bat1, 1);
				gpio_output(LED2_Bat2, 1);
				gpio_output(LED3_Bat3, 1);
				if(SysInf.WorkMode==SYS_WM_BT_MODE){
					RgbLedOneColour(ModeLedAddr,3);	//RgbLedOut(5);	// set power led color
				}else if(SysInf.WorkMode==SYS_WM_SDCARD_MODE){
					RgbLedOneColour(ModeLedAddr,4);	//RgbLedOut(6);
				}else if(SysInf.WorkMode==SYS_WM_LINEIN1_MODE){
					RgbLedOneColour(ModeLedAddr,2);	//RgbLedOut(7);
				}
			}else{
				dsp_shutdown();
				REG_SYSTEM_0x1D &= ~0x2;
				system_wdt_reset_ana_dig(1, 1);
				BK3000_wdt_reset();
				os_delay_us(3200);	//1ms
			}

		}else if(Init_Cont == 145){
			USER_DBG_INFO("================= g_mcu_log_on_fg = 0\n");
			g_dsp_log_on_fg = 0;
			g_mcu_log_on_fg = 0;
		}else if(Init_Cont == 146){
			Send_Id1 = CmdPid;
			Send_Id2 = 0x80;
			SendUpComp(SendBuff, 32);	// 請上位機連線
		}else if(Init_Cont == 147){
			g_dsp_log_on_fg = 1;
			g_mcu_log_on_fg = 1;
			USER_DBG_INFO("================= g_mcu_log_on_fg = 1\n");
			SysMsCont=0;
			EC11_SwR = EC11_Sw;

		}else if(Init_Cont == 148){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;
			system_work_mode_set_button(SysInf.WorkMode);

#ifdef	CONFIG_AUD_AMP_DET_FFT
			extern void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
			aud_mag_det_enable(1);	//audio magnitude detect enable
#endif
		}

	}
}

//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{

}

//***************************************************
void Mic_VolSet(float val)
{
	ioGain[MicMstVol] = val;
	IOGainSet(MicMstVol, val);
//	USER_DBG_INFO("============== Mic_VolSet\n");
}

//***************************************************
void Out_VolSet(float val)
{
}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_stop();
	switch (EF_Mode){
//		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		case 3:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);	break;
		case 4:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);	break;
	}
}
//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_stop();
	app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
	switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_FM_MODE); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);	break;
	}
}

//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
//	USER_DBG_INFO("====key: %d:  %d\n", pKeyArray->index, pKeyArray->event);
	UpComputerRW = 0;
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;
	case KEY_S_REL:
//		ms1000=99;
		switch (pKeyArray->index){
		case 0:	// MODE
			if(FadeInOut==0){	// MODE 切換中 按鍵失效
				if(sys_hdl->sys_work_mode==SYS_WM_BT_MODE && sd_is_attached()==0 && LineInR)	return;
				KeyId = BUTTON_MODE_CHG;
				FadeInOut = 200;
			}
			break;
		case 1:	// PRE
			if(FadeInOut<150 && (hci_get_acl_link_count(sys_hdl->unit) || app_is_mp3_mode())){
				KeyId = BUTTON_BT_PREV;
//				if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE) || player_get_play_status()){	// 0:没有播放。	yuan++
					FadeInOut = 200;
//				}
			}
			break;
		case 2:	// WM SW
			if(DisplayCont>1)	return;	// MODE 切換中 按鍵失效
			if(WmPoSw){
				if(WmPoSw==1 && usbd_active_get()){
					WmPoSw=2;	//直播模式
					ioGain[MusL_Out1L] = 0;
					ioGain[MusR_Out1R] = 0;
					if(SysInf.Lang){
						app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);
					}else{
						app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);
					}
				}else{
					WmPoSw=0;	//音樂模式
					ioGain[MusL_Out1L] = 0;
					ioGain[MusR_Out1R] = 0;
					BK_DeyOnCont = 0;
					WMicOffCont = 0;
			#ifdef ZY001
					ACM_Set[ACM862xWId].EQ[41].gain = 30;
			#endif
					if(SysInf.Lang){
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_5);
					}else{
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
					}
				}
			}else{
				WmPoSw=1;
				ioGain[MusL_Out1L] = -90;
				ioGain[MusR_Out1R] = -90;
				BK_DeyOnCont = 140;
			#ifdef ZY001
				ACM_Set[ACM862xWId].EQ[41].gain = -30;
			#endif
			}
			USER_DBG_INFO("==== WmPoSw: %d\n",WmPoSw);
			break;
		case 3:	// NEXT
			if(FadeInOut<150 && (hci_get_acl_link_count(sys_hdl->unit) || app_is_mp3_mode())){
				KeyId = BUTTON_BT_NEXT;
//				if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE) || player_get_play_status()){	// 0:没有播放。	yuan++
					FadeInOut = 200;
//				}
			}
			break;
		}
		DisplayCont = DisplayCont_def;
		break;

	//====================================================================================================
	case KEY_D_CLICK:	// 短按 2次
		break;
	case KEY_T_CLICK:
		USER_DBG_INFO("KEY_T_CLICK\n");		// 短按 3次
		if(pKeyArray->index==0){
//			WorkEqChg();
		}else if(pKeyArray->index==1){
//			Out_Eq();
		}
		break;
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:	// 常按進入
//		USER_DBG_INFO("KEY_L_PRES\n");
		switch (pKeyArray->index){
		case 0:	// MODE
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ALL_CYCLE){
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ONE;
					RgbFlash=2;
					if(SysInf.Lang){
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
					}else{
						app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);
					}
				}else{
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ALL_CYCLE;
					RgbFlash=4;
					if(SysInf.Lang){
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8);
					}else{
						app_wave_file_play_start(APP_WAVE_FILE_ID_REDIAL);
					}
				}
				SysInf.SdCardPlayMode = appPlayerPlayMode;
			}
			break;
		case 1:	// PRE
			app_player_button_prevDir();
			FadeInOut = 149;
			break;
		case 2:	// WM SW	// 消原音
			SysInf.VoiceCanEn ^=1;
			if(SysInf.VoiceCanEn){
				if(SysInf.Lang){
					app_wave_file_play_start(APP_WAVE_FILE_ID_MP3_MODE);
				}else{
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
				}
			}else{
				if(SysInf.Lang){
					app_wave_file_play_start(APP_WAVE_FILE_ID_FM_MODE);
				}else{
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_CANCEL);
				}
			}
			break;
		case 3:	// NEXT
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
#ifdef ZY004
				if(++LED_n >3)	LED_n = 0;
				if(LED_n ==0){
					RGB_MODE[0]=2;	//RGB模式 0=RGB, 1=漸變定色, 		2=聲控,	 4=幻彩
					RGB_EF[0]= 4;	//幻彩效果(1~15); 252~3		聲控模式(1~4) ===== 聲控
					RGB_SP[0]= 5;	//漸變速度(0~239),	定色(240~250),	EQ_Bank(0~8) ===== 定色 黃光
					RGB_AUTO[0]=1;	//聲控自動轉漸變(0,1)
					RGB_AEF[0]=0;
					W_Y[0]=255;
				}else if(LED_n ==1){
					RGB_MODE[0]=4;
					RGB_EF[0]= 4;
					RGB_SP[0]= 15;
					RGB_AUTO[0]=1;
					RGB_AEF[0]=1;
					W_Y[0]=255;
				}else if(LED_n ==2){    //白色
					RGB_MODE[0]=1;	// 單色配置
					RGB_EF[0]= 15;
					RGB_SP[0]= 11;	// 顏色 0~ 11
					/**********************************************
					 	0.暖白色	1.紅色	2.綠色	3.藍色	4.黃色
						5.紫色	6.青色	7.桃紅色	8.夢幻粉	9.土豪金
						10.神秘紫	11.白色	12.關燈
					 **********************************************/
				}else if(LED_n ==3){     //off
					RGB_MODE[0]=1;	// 單色配置
					RGB_EF[0]= 15;
					RGB_SP[0]= 12;	// 顏色 0~ 11
				}
#endif
			}else{
				app_player_button_nextDir();
				FadeInOut = 149;
			}
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
//		USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
		//---------------------------------------------------
		if(pKeyArray->index==1 && sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
			if(pKeyArray->keepTime==5000){
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);
			}else if(pKeyArray->keepTime==8000){
				SysInf.Lang ^=1;
				if(SysInf.Lang){
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);
				}else{
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);
				}
			}
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}
}

//********************************
void PlayWaveStart(uint16_t id)
{
#if 0
	if(id==APP_WAVE_FILE_ID_ENTER_PARING || id==APP_WAVE_FILE_ID_HFP_RING){
		USER_DBG_INFO("!!!!!!!!!!!!!! wave_start, ID:%d\n", id);
		return;
	}
#endif
	USER_DBG_INFO("==== wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);		//Music Off
	if(WmPoSw){
		IOGainSetDamp(MicMstVol, -90,32);	//Mic Off
	}
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
	//	RgbLedOut(8);
		RgbLedOneColour(ModeLedAddr,0);
		gpio_output(LED0_Bat0, 0);
		gpio_output(LED1_Bat1, 0);
		gpio_output(LED2_Bat2, 0);
		gpio_output(LED3_Bat3, 0);
		app_handle_t sys_hdl = app_get_sys_handler();
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PoKeyCont=0;
		PowerDownFg=1;

		SysInf.VoiceCanEn = 0;	// 關閉消原音
//		SysInf.MusVol32 = MusVolVal;	//yuan41
		SysInf.WorkMode = sys_hdl->sys_work_mode;
		GroupWrite(0, 2);		// 保存記憶
		break;
	case APP_WAVE_FILE_ID_DISCONN:
	case APP_WAVE_FILE_ID_BT_MODE:
	case APP_WAVE_FILE_ID_CONN:
		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{
//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		gpio_output(WM_PO_EN, 0);
		gpio_output(ACM8625_EN, 0);
		gpio_output(Boot_EN, 0);
		while(gpio_input(PowerKey)==0);
		dsp_shutdown();
		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		gpio_output(POWER_EN, 0);
		os_delay_us(3200);	//1ms
		break;
	case APP_WAVE_FILE_ID_CONN:
		break;
	case APP_WAVE_FILE_ID_BT_MODE:
		break;
	case APP_WAVE_FILE_ID_RESERVED0:	// SD Card
		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
		break;
	case APP_WAVE_FILE_ID_LINEIN_MODE:
		break;
	case APP_WAVE_FILE_ID_LOW_BATTERY:
		break;
	case APP_WAVE_FILE_ID_VOL_MAX:
	case APP_WAVE_FILE_ID_VOICE_NUM_5:
		IOGainSetDamp(MicMstVol, -90,64);
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x00);	// Stop
#ifdef ZY004
		ACM862xWId = 1;
		SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x00);	// Stop
		ACM862xWId = 0;
		SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
#endif
		ChipEn[0]=0;
		ChipEn[1]=0;
		gpio_output(WM_PO_EN, WmPoSw);
		os_delay_us(5000);
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x03);	// Play
#ifdef ZY004
		ACM862xWId = 1;
		SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x00,0x00);	// 設定頁碼
		I2C_WriteA8D8(ACM862x_IIC_ADDR[ACM862xWId],0x04,0x03);	// Play
		ACM862xWId = 0;
		SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
#endif
		//	ACM_Set.EQ[19].freq = 600;
		//	ACM_Set.EQ[44].gain = 80;
		BK_DeyOnCont = 0;
		WMicOffCont = 0;
		break;
	case APP_WAVE_FILE_ID_HFP_ACK:
//		app_button_sw_action(BUTTON_BT_PREV);
		break;
	case APP_WAVE_FILE_ID_HFP_REJECT:
//		app_button_sw_action(BUTTON_BT_NEXT);
		break;
	}
	//=========================================================
//	SwChgIng = 0;
	IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],10);	//yuan41
	if(WmPoSw){
		IOGainSetDamp(MicMstVol, 5,32);
	}
	USER_DBG_INFO("==== wave_stop, ID:%d  MusicVol:%d\n", id, MusVol_TAB[(uint8_t)SysInf.MusicVol]);
}

#endif

