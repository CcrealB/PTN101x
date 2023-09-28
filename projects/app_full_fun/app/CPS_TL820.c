/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
#include "USER_Config.h"

#ifdef CPS_TL820
#include 	".\LCD_ST7789V\ST7789V.h"

#if defined(ES260)		// 效果器
	#include "CPS_TL820_ES260_DEF.H"
#elif defined(HS2450)	// 效果器+功放
	#include "CPS_HS2450_DEF.H"
#elif defined(TL8035)	// ACM8629x2 PTBL DAMP
	#include "CPS_TL8035_DEF.H"
#elif defined(TL820)
	#include "CPS_TL820_DEF.H"
#elif defined(TL8025)||defined(TL8025v2)	//8629 TBL
	#include "CPS_TL8025_DEF.H"
#endif

#ifdef AMG8802
	void AMG82xx_Main(void);
#endif

const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

const int8_t MicVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};
//uint8_t Mus_Val=16;
//uint8_t Mic_Val=16;
uint8_t Mus_ValR=255;
uint8_t Mic_ValR=255;

static uint8_t UpDisplay = 0;

static uint16_t SysMsCont=0;

static uint16_t WorkModeCont=0;
//uint8_t work_modeR=10;
static uint8_t	BtLedFlash=0;
uint8_t LineInR=0;

static int16_t BtVolR = -5;
#if defined(TL8025) || defined(TL8025v2)
	static int16_t LineVolR = 0;
#else
	static int16_t LineVolR = 0;
#endif

#ifdef IR_TX
void pwm_set(uint32_t us, uint8_t OnOff);
void ir_tx_init(uint8_t En);
static uint16_t IrTxCont = 0;
uint8_t IrTxTime = 0;	// step 0.5s

uint8_t IrTxData[8]={0x6C,0x57,0x00,0x00,0x32,0x1C,0xE8,0x00};
//uint8_t IrTxData[4]={0x00,0xBF,0x52,0xAD};
//*********************************
void UserIrTxFun()
{
	static uint8_t TxCont=0;
	static uint8_t DataFg=0;
	static uint8_t TxBitCont=0;
	static uint8_t TxBitSt=1;
	static uint32_t Tol_Time;
	switch (TxCont){
		case 0:
			if(TxBitSt){
				pwm_set(9000, 1);
				TxBitSt=0;
			}else{
				pwm_set(4500, 0);
				Tol_Time =108000-13500;
				TxCont++;
				TxBitSt=1;
			}
			break;
        case 1:
        	if(TxBitSt){
        		pwm_set(560, 1);
        		Tol_Time-=560;
        		TxBitSt=0;
        	}else{
        		if((IrTxData[DataFg]>>TxBitCont)&1){
        			pwm_set(1690, 0);
        			Tol_Time-=1690;
        		}else{
        			pwm_set(565, 0);
        			Tol_Time-=565;
        		}
        		TxBitSt = 1;
        		if(++TxBitCont==8){
        			TxBitCont=0;
        			if(++DataFg==8)	TxCont++;
        		}
        	}
        	break;
        case 2:
        	if(TxBitSt){
        		pwm_set(560, 1);
        		Tol_Time-=560;
        		TxBitSt=0;
        	}else{
        		pwm_set(Tol_Time, 0);
        		TxCont=5;
        		TxBitSt=1;
        	}
        	break;
/*
        case 3:
        	if(TxBitSt){
        		pwm_set(9000, 1);
        		TxBitSt=0;
        	}else{
        		pwm_set(2250, 0);
        		TxCont++;
        		TxBitSt=1;
        	}
        	break;
        case 4:
        	if(TxBitSt){
        	  	pwm_set(560, 1);
        	  	TxBitSt=0;
        	}else{
        		pwm_set(96190, 0);
        		TxCont++;
        		TxBitSt=1;
        	}
        	break;
*/
        case 5:
        	ir_tx_init(0);
        	TxCont=0;
        	TxBitCont=0;
        	TxBitSt=1;
        	DataFg=0;
        	break;
	}
}

#endif

#if defined(TL8025)||defined(TL8035)||defined(TL820)||defined(TL8025v2)
static uint8_t OffPowerCont = 0;
#define	fullVn			24500	// 滿電壓 mv
#define	P4Voln			8000	// 亮4個燈電壓
#define	P3Voln			7500	// 亮3個燈電壓
#define	P2Voln			7000	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		19800	// 低電模式電壓
#define	LoPowerOFFv	 	18000	// 低電關機電壓
//***************************************************
uint16_t BattVolConvert(uint16_t AdcVol)
{
	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻(BattHR), 對地電阻 (BattLR)
	AdcVol = SarADC_val_cali_get(AdcVol);	// 校正轉換
	float BattVol = (float)saradc_cvt_to_voltage(AdcVol)/BattRv;
//	USER_DBG_INFO("==== AdcVol %d  %d  %d\n",AdcVol, saradc_cvt_to_voltage(AdcVol), BattVol);
	return  BattVol;
}

//*************************************************
void BattFun(void)
{
	//--------------------------------
	static uint16_t BattVolDetMs = 0;
	static uint16_t LowBattCont = 0;
	if(++BattVolDetMs == 100){
		BattVolDetMs = 0;
		//****25.2v, 270k,21k(1.819v)/4096=0.444, 25200/1819=13.854 19.2v警告, 18v關機  ******检测电池电量
		uint16_t BattVol = BattVolConvert(SarAdcVal[1]);
		static uint16_t saradc_tempe_dataR;
		extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。

		fl2hex V1;
		if((abs(SarAdcValR[1]-SarAdcVal[1])>10) || (saradc_tempe_dataR != saradc_tempe_data)){
			SarAdcValR[1] =SarAdcVal[1];
			saradc_tempe_dataR = saradc_tempe_data;
			USER_DBG_INFO("====SarAdcVal: %d  %d\n", SarAdcVal[1], BattVol);

			Send_Id1 = MainPid;
			Send_Id2 = 205;
			Send_Group = 1;
			V1.f = (float)BattVol/1000;
			Send_Val1 = V1.h;
			V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
			Send_Val2 = V1.h;
			SendUpComp(SendBuff, 32);

	#ifdef ST7789V
			LCD_ShowxNum(100, 214,BattVol/1000, 2, 16, 0);			//剩余电量
			LCD_ShowxNum(124, 214,(BattVol%1000)/100, 1, 16, 0);	//剩余电量
			uint16_t BattPercentage = (BattVol-LoPowerOFFv)/65;
			if(BattPercentage>100) BattPercentage = 100;
			LCD_ShowxNum(148, 214,BattPercentage, 3, 16, 0);		//剩余电量 %
	#endif
			if(BattVol<LoPowerENv){
				if(++LowBattCont > 300){	// 300 秒提示一次,直到低電關機
					LowBattCont = 0;
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 低電提示音
				}
				if(BattVol<LoPowerOFFv){
					if(++OffPowerCont==10){
						app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 低電關機
					}
				}else{
					OffPowerCont = 0;
				}
			}
		}
	}
}
#endif

//==============================================
#ifdef	RECED_EN
static uint16_t RecTime = 0;
static uint8_t RecDrv = 0;
static uint16_t Rec_num;
static uint8_t RecState = 0;
static uint16_t RecPlayInx = 0;
static uint8_t Rec10MsCont = 0;
//******************************
void user_RecWork(void)
{
	RecState = 1;
}
//******************************
void user_RecPlayWork(void)
{
	RecState = 5;
}

//**** 錄音載體設置 ********************************
void RecDrvSelect()
{
	if(player_get_play_status()){	// 2:正在播放， 0:没有播放。	yuan++
		app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		RecState = 0;	// 取消任務
		return;	// 錄音載體 播放中, 停止播放並返回
	}
	if(sd_is_attached() || udisk_is_attached()){
		if(sd_is_attached())	RecDrv = 0;
			else				RecDrv = 1;
	}
}
//******************************
void RecFun()
{
	fl2hex V1;
	switch (RecState){
	case 1:	// 錄音請求任務
		RecDrvSelect();
		USER_DBG_INFO("====RecDrv: %d\n",RecDrv);
		if(recorder_is_working()){	// 錄音中
			recorder_start(RecDrv, 0);
			RecState =12;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);	// 停止錄音提示音
		}else{
			RecState =13;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	// 開始錄音提示音
		}
		break;
	case 2:	// 停止錄音
		Send_Id1 = MainPid;
		Send_Id2 = 204;
		Send_Group = 0;
		Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
		RecPlayInx = (Rec_num-1);
		USER_DBG_INFO("====WrRecPlayInx: %d\n",RecPlayInx);
		char Lab[64];
		rec_file_name_get_by_idx(Rec_num-1, Lab);  //根据索引获取文件名
		memcpy(&SendBuff[12], Lab,16);
		SendUpComp(SendBuff, 32);
		RecState = 0;
		break;
	case 3:	// 啟動錄音
		RecTime=0;
		recorder_start(RecDrv, 1);
		RecState = 4;
		break;
	case 4:	// 錄音中傳送錄音時間碼到上位機
		if(++Rec10MsCont < 100)	return;
		Rec10MsCont = 0;
		if(recorder_is_working()){
			Send_Id1 = MainPid;
			Send_Id2 = 204;
			Send_Group = 1;
			V1.f = (float)RecTime++;
			Send_Val1 = V1.h;
			SendUpComp(SendBuff, 32);
			//	USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		}else{
			RecState = 0;
		}
		break;
	case 5:	// 錄音播放請求
		if(recorder_is_working()){	// 錄音中
			recorder_start(RecDrv, 0);	// 停止錄音
			RecState = 12;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
		}else{
			if(RecPlayInx){
				if(sd_is_attached() || udisk_is_attached()){	// RecPlay
					RecState = 16;
					if(sd_is_attached())	system_work_mode_set_button(SYS_WM_SDCARD_MODE);
						else				system_work_mode_set_button(SYS_WM_UDISK_MODE);
				}else{
					RecState = 0;
					RecPlayInx = 0;
				}
			}else{
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				RecState = 0;
			}
		}
		break;
	case 6:	// 播放最後錄音索引
		if(RecPlayInx){
			rec_file_play_by_idx(RecPlayInx);//根据索引播放录音文件
			USER_DBG_INFO("====RecPlayInx: %d\n",RecPlayInx);
			RecPlayInx = 0;
		}
		RecState = 0;
		break;
	}
}
#endif

//*****************************************************
void user_WorkModeVolChg(uint8_t Mode)
{
	if(Mode==SYS_WM_LINEIN1_MODE){
		if(ioGain[Play_MusL]==-90) ioGain[Play_MusL]=-5;
		BtVolR = ioGain[Play_MusL];
		ioGain[Play_MusL]= -90;
		ioGain[Play_MusR]= ioGain[Play_MusL];

		ioGain[Adc4_MusL]= LineVolR;
		ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
	}else{
		if(ioGain[Adc4_MusL]==-90) ioGain[Adc4_MusL]=0;
		LineVolR = ioGain[Adc4_MusL];
#ifdef HS2450
		ioGain[Adc4_MusL]= -90;
		ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
#endif
		ioGain[Play_MusL]= BtVolR;
		ioGain[Play_MusR]= ioGain[Play_MusL];
	}
}

//*****　插U盘会调用　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void user_udisk_ready_callback(void)
{
	system_work_mode_set_button(SYS_WM_UDISK_MODE);
	SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
}

//*****************************************************
void user_WorkModefun(void)
{
#ifdef RECED_EN
		RecFun();
#endif
	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_PutString(270, 214, (u8*)"BT   ", 16, POINT_COLOR, BACK_COLOR);
				user_WorkModeVolChg(SYS_WM_BT_MODE);
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				DMAimage_display(250,214,(u8*)gImage_Bluto_icon_gre);//蓝牙
			}else{
				if(BtLedFlash^=1)	DMAimage_display(250,214,(u8*)gImage_Bluto_icon_gre);//蓝牙
					else			DMAimage_display(250,214,(u8*)gImage_Bluto_icon);//蓝牙
			}
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_PutString(270, 214, (u8*)"UDisk", 16, POINT_COLOR, BACK_COLOR);
				LCD_Fill(250,214,15,15, BLACK);
				user_WorkModeVolChg(SYS_WM_UDISK_MODE);
			}
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_PutString(270, 214, (u8*)"LINE ", 16, POINT_COLOR, BACK_COLOR);
				user_WorkModeVolChg(SYS_WM_LINEIN1_MODE);
			}
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_SPDIF_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_PutString(270, 214, (u8*)"SPDIF", 16, POINT_COLOR, BACK_COLOR);
				LCD_Fill(250,214,15,15, BLACK);
				user_WorkModeVolChg(SYS_WM_SPDIF_MODE);
			}
		}
	}
}

uint8_t MenuF = 3;
uint8_t MenuFR = 3;
void DisplayMenu();

static uint16_t MenuCont = 0;
static uint16_t PoKeyCont=1001;
static int8_t 	EC11_Sw,EC11_SwR;
void Encoder_EC11_Init(unsigned char Set_EC11_TYPE);
char Encoder_EC11_Scan();

#ifdef EC11_Encoder2
	static uint16_t MenuCont2 = 0;
	static uint16_t PoKeyCont2=1001;
	static int8_t 	EC11_Sw2,EC11_SwR2;
	void Encoder_EC11_Init2(unsigned char Set_EC11_TYPE2);
	char Encoder_EC11_Scan2();
#endif

//********************************************************
void EC11_FUN(void)
{
#ifndef	EC11_Encoder2
//	app_handle_t sys_hdl = app_get_sys_handler();
#endif
	if((EC11_Sw==0) && (EC11_SwR==2)){
		if(PoKeyCont>=10){
			if(PoKeyCont>1000){
				PoKeyCont=0;
				return;
			}
			PoKeyCont=0;
			if(MenuF ==3){	// Music VOL
				MenuF = 9;
			}else if(MenuF ==9){	// Mic VOL
				MenuF = 23;
			}else if(MenuF ==23){	// ECHO VOL
				MenuF = 13;			// REV VOL
			}else{
#ifdef	EC11_Encoder2
				MenuF = 9;
#else
				MenuF = 3;
#endif
			}
			DisplayMenuChg();
			MenuCont=0;
		}
		USER_DBG_INFO("====P. EC11_Sw: %d  %d  %d\n",EC11_Sw, EC11_SwR, PoKeyCont);
	}else if((EC11_Sw !=2) && (EC11_SwR != EC11_Sw)){
		//===================================
		if(EC11_Sw==1){
#ifdef	EC11_Encoder2
			if(MenuF ==3){
				MenuF =9;
				DisplayMenuChg();
			}
#endif
			USER_DBG_INFO("=== R  %d\n", MenuF);
			switch (MenuF){
				case 1:	if(EF_Mode < 4u)	EF_Mode += 1u;	break;
				case 2:	if(EQ_Mode < 2u)	EQ_Mode += 1u;	break;
#ifndef	EC11_Encoder2
				case 3:
			//		if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
			//			app_button_sw_action(BUTTON_BT_VOL_P);
			//		}else{
						if(SysInf.MusicVol < 32) 	SysInf.MusicVol++;
			//		}
					UpComputerRW=0;
					break;
#endif
				case 4: if(SysInf.MusNoiseGate < -20) SysInf.MusNoiseGate += 1.0f;	break;
				case 5: if(SysInf.KeyShift < 6) SysInf.KeyShift += 1;			break;
				case 6: if(SysInf.OutVol < 10.0f) SysInf.OutVol += 1.0f;		break;
				case 7:	if(UserEf.drygain < 1.0f) UserEf.drygain += 0.1f;		break;
				case 8:	if(UserEf.drygain2 < 1.0f) UserEf.drygain2 += 0.1f;		break;
				case 9:	if(SysInf.MicVol < 32) 	SysInf.MicVol++;				break;
				case 10:if(SysInf.MicNoiseGate < -20) SysInf.MicNoiseGate += 1.0f;	break;
				case 11:
					if(UserEf.Fshift < 12) UserEf.Fshift += 1;
					if(UserEf.Fshift==-2){
						UserEf.Fshift =0;
					}else if(UserEf.Fshift==1){
						UserEf.Fshift =3;
					}
					break;
				case 12:if(UserEf.Pitch < 12) UserEf.Pitch += 1;				break;
				case 13:if(UserEf.RevVol < 1.0f) UserEf.RevVol += 0.1f;	UserEf.RevVol2=UserEf.RevVol; break;
				case 14:if(UserEf.RevVol2 < 1.0f) UserEf.RevVol2 += 0.1f;		break;
				case 15:if(UserEf.lex_PreDelay <100) UserEf.lex_PreDelay += 1;	break;
				case 16:if(UserEf.RevRep < 1.0f) UserEf.RevRep += 0.1f;			break;
				case 17:if(UserEf.lex_iDfus1 < 95) UserEf.lex_iDfus1 += 1;		break;
				case 18:if(UserEf.lex_iDfus2 < 95) UserEf.lex_iDfus2 += 1;		break;
				case 19:if(UserEf.lex_Excur < 16) UserEf.lex_Excur += 1;		break;
				case 20:if(UserEf.lex_dDfus1 < 95) UserEf.lex_dDfus1 += 1;		break;
				case 21:if(UserEf.lex_dDfus2 < 95) UserEf.lex_dDfus2 += 1;		break;
				case 22:if(UserEf.lex_HFreqDamp < 10) UserEf.lex_HFreqDamp += 1;break;
				case 23:if(UserEf.EchoVol < 2.0f) UserEf.EchoVol += 0.1f; UserEf.EchoVol2=UserEf.EchoVol; break;
				case 24:if(UserEf.EchoVol2 < 2.0f) UserEf.EchoVol2 += 0.1f;		break;
				case 25:if(UserEf.EchoDeyT < 300)	UserEf.EchoDeyT += 1;		break;
				case 26:if(UserEf.EchoRep < 1.0f) UserEf.EchoRep += 0.1f;		break;
				case 27:if(SysInf.MicCompT < 0) SysInf.MicCompT += 1;			break;
				case 28:if(SysInf.MusCompT < 0) SysInf.MusCompT += 1;			break;
				case 29: SysInf.VoiceCanEn = 1;									break;
				case 30: if(SysInf.DuckFadeDB < 0.0f) SysInf.DuckFadeDB += 1.0f;break;
			}
			UpDisplay = 1;
			MenuCont=0;
		}else if(EC11_Sw==-1){
#ifdef	EC11_Encoder2
			if(MenuF ==3){
				MenuF =9;
				DisplayMenuChg();
			}
#endif
			USER_DBG_INFO("=== L  %d\n", MenuF);
			switch (MenuF){
				case 1:	if(EF_Mode > 0u)	EF_Mode -= 1u;	break;
				case 2:	if(EQ_Mode > 0u)	EQ_Mode -= 1u;	break;
#ifndef	EC11_Encoder2
				case 3:
			//		if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
			//			app_button_sw_action(BUTTON_BT_VOL_M);
			//		}else{
						if(SysInf.MusicVol)	SysInf.MusicVol--;
			//		}
					UpComputerRW=0;
					break;
#endif
				case 4: if(SysInf.MusNoiseGate > -60) SysInf.MusNoiseGate -= 1.0f;	break;
				case 5: if(SysInf.KeyShift > -6) SysInf.KeyShift -= 1;		break;
				case 6: if(SysInf.OutVol > -60.0f) SysInf.OutVol -= 1.0f;	break;
				case 7:	if(UserEf.drygain > 0.01f) UserEf.drygain -= 0.1f;	break;
				case 8:	if(UserEf.drygain2 > 0.01f) UserEf.drygain2 -= 0.1f;break;
				case 9:	if(SysInf.MicVol) 		SysInf.MicVol--;			break;
				case 10:if(SysInf.MicNoiseGate > -60) SysInf.MicNoiseGate -= 1.0f;	break;
				case 11:
				if(UserEf.Fshift > -12)	UserEf.Fshift -= 1;
				if(UserEf.Fshift==2){
					UserEf.Fshift =0;
				}else if(UserEf.Fshift==-1){
					UserEf.Fshift =-3;
				}
				break;
				case 12:if(UserEf.Pitch > -12) UserEf.Pitch -= 1;				break;
				case 13:if(UserEf.RevVol > 0.01f) UserEf.RevVol -= 0.1f; UserEf.RevVol2=UserEf.RevVol;	break;
				case 14:if(UserEf.RevVol2 > 0.01f) UserEf.RevVol2 -= 0.1f; 		break;
				case 15:if(UserEf.lex_PreDelay > 0) UserEf.lex_PreDelay -= 1;	break;
				case 16:if(UserEf.RevRep > 0.01f) UserEf.RevRep -= 0.1f;		break;
				case 17:if(UserEf.lex_iDfus1 > 0) UserEf.lex_iDfus1 -= 1;		break;
				case 18:if(UserEf.lex_iDfus2 > 0) UserEf.lex_iDfus2 -= 1;		break;
				case 19:if(UserEf.lex_Excur > 0) UserEf.lex_Excur -= 1;			break;
				case 20:if(UserEf.lex_dDfus1 > 0) UserEf.lex_dDfus1 -= 1;		break;
				case 21:if(UserEf.lex_dDfus2 > 0) UserEf.lex_dDfus2 -= 1;		break;
				case 22:if(UserEf.lex_HFreqDamp > 0) UserEf.lex_HFreqDamp -= 1;	break;
				case 23:if(UserEf.EchoVol > 0.01f) UserEf.EchoVol -= 0.1f; UserEf.EchoVol2=UserEf.EchoVol;	break;
				case 24:if(UserEf.EchoVol2 > 0.01f) UserEf.EchoVol2 -= 0.1f;	break;
				case 25:if(UserEf.EchoDeyT > 0)	UserEf.EchoDeyT -= 1;			break;
				case 26:if(UserEf.EchoRep > 0.01f) UserEf.EchoRep -= 0.1f;		break;
				case 27:if(SysInf.MicCompT > -36.0f) SysInf.MicCompT -= 1.0f;	break;
				case 28:if(SysInf.MusCompT > -36.0f) SysInf.MusCompT -= 1.0f;	break;
				case 29: SysInf.VoiceCanEn = 0;									break;
				case 30:if(SysInf.DuckFadeDB > -60.0f) SysInf.DuckFadeDB -= 1.0f;break;
			}
			UpDisplay = 1;
			MenuCont=0;
		}
	}else if(EC11_Sw==2){
		PoKeyCont++;
#if defined(TL8025)||defined(TL8035)||defined(TL820)||defined(TL8025v2)
		if(PoKeyCont==100){	// 1s
			USER_DBG_INFO("==== EC11_Sw: %d  %d\n",EC11_Sw, PoKeyCont);
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
		}
#else
		if(PoKeyCont==100){	// 1s
			GroupWrite(0, 2);
		}
#endif
	}

	EC11_SwR = EC11_Sw;
}

#ifdef EC11_Encoder2
//********************************************************
void EC11_FUN2(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	if((EC11_Sw2==0) && (EC11_SwR2==2)){
		if(PoKeyCont2>=10){
			if(PoKeyCont2>1000){
				PoKeyCont2=0;
				return;
			}
			PoKeyCont2=0;
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
		//	if(hci_get_acl_link_count(sys_hdl->unit) || ){	//如果是连接状态
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			}
			MenuCont2=0;
		}
		USER_DBG_INFO("====P. EC11_Sw: %d  %d  %d\n",EC11_Sw2, EC11_SwR2, PoKeyCont2);
	}else if((EC11_Sw2 !=2) && (EC11_SwR2 != EC11_Sw2)){
		if(MenuF !=3){
			MenuF =3;
			DisplayMenuChg();
			MenuCont2=0;
		}
		//===================================
		if(EC11_Sw2==1){
			USER_DBG_INFO("=== R  %d\n", MenuF);
			if(MenuF==3){
				if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
					app_button_sw_action(BUTTON_BT_VOL_P);
				}else{
					if(SysInf.MusicVol < 32) 	SysInf.MusicVol++;
				}
				UpComputerRW=0;
			}
			UpDisplay = 1;
			MenuCont2=0;
		}else if(EC11_Sw2==-1){
			if(MenuF !=3){
				MenuF =3;
				DisplayMenuChg();
			}
			USER_DBG_INFO("=== L  %d\n", MenuF);
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				app_button_sw_action(BUTTON_BT_VOL_M);
			}else{
				if(SysInf.MusicVol)	SysInf.MusicVol--;
			}
			UpComputerRW=0;
			UpDisplay = 1;
			MenuCont2=0;
		}
	}else if(EC11_Sw2==2){
		PoKeyCont2++;
		if(PoKeyCont2==100){	// 1s
			USER_DBG_INFO("==== EC11_Sw: %d  %d\n",EC11_Sw2, PoKeyCont2);
//			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
			GroupWrite(0, 2);
		}
	}

	EC11_SwR2 = EC11_Sw2;
}
#endif

//**** pre init ***********************
void user_init_prv(void)
{
	//==== GPIO SET =======================
#ifdef HS2450
	#define	IR_AMP_EN	GPIO11
    gpio_config_new(IR_AMP_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(IR_AMP_EN, 1);
#endif
	#define KT56_INTAB_IO	GPIO23
//	#define KT56_INTAB_IO	GPIO29
//	#define	IR_TX		GPIO12
//	gpio_config_new(IR_TX, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//  gpio_output(IR_TX, 0);

	gpio_config_new(KT56_INTAB_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56 中斷
	// 0:low level;	1:high level;  2:rise edge;	3:fall down interrup
	gpio_int_enable(KT56_INTAB_IO, 3);
	//----------------------------
	#define KT56_En_IO	GPIO35
	gpio_config_new(KT56_En_IO, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    gpio_output(KT56_En_IO, 1);

	#define	AMP_EN		GPIO14	//20230328 ADD
    gpio_config_new(AMP_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(AMP_EN, 0);

	#define	POWER_EN		GPIO30
    gpio_config_new(POWER_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(POWER_EN, 1);

	USER_DBG_INFO("====ST7789V_Init 1\n");
}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	appPlayerPlayMode = SysInf.SdCardPlayMode;
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;

	USER_DBG_INFO("======== TL820_V01 user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
	LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);
#if LED_RGB_NUM
	app_i2s0_init(1);
#endif
}

//********************************
void user_init_post(void)
{
    //=========================
	Hi2c_Init();
#ifdef AMG8802
	extern void APP_Init(void);
	APP_Init();
#endif
	//============================
	Encoder_EC11_Init(1);
#ifdef	EC11_Encoder2
	sys_log_port_set(SYS_LOG_PORT_UART0, 0);
	Encoder_EC11_Init2(1);
#endif
#if defined(TL8025)||defined(TL8035)||defined(TL820)||defined(TL8025v2)
	if(gpio_input(GPIO15)){
		//	gpio_output(POWER_EN, 0);	// 電源鍵按下時間太短
		//	while(1);
		app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
	}
#endif
}

//**********************************************
void user_loop(void)
{
//    mailbox_mcu_isr();
    static uint32_t t_mark = 0;
    if(sys_timeout(t_mark, 10)){	//10ms task
        t_mark = sys_time_get();
        extern void user_loop_10ms(void);
        user_loop_10ms();
    }
}

static uint8_t Tick_2ms =0;
//*******************************************
void tick_task_1ms(void) //timer0 1ms isr
{
//	extern int8_t a2dp_get_volume(void);
//	if(SysInf.MusicVol != a2dp_get_volume()){
//		USER_DBG_INFO("=== a2dp_get_volume:%d\n",a2dp_get_volume());
//		SysInf.MusicVol = a2dp_get_volume();
//	}

	if(PowerDownFg) return;
	//=============================================
	Tick_2ms++;
	if(Tick_2ms==1){
		user_saradc_update_trig();	// Adc 讀取觸發
		if(EC11_SwR==EC11_Sw || EC11_Sw==0)	EC11_Sw = Encoder_EC11_Scan();
	}else if(Tick_2ms>=2){
		Tick_2ms =0;
#ifdef	EC11_Encoder2
		if(EC11_SwR2==EC11_Sw2 || EC11_Sw2==0)	EC11_Sw2 = Encoder_EC11_Scan2();
#endif
	}
	SysMsCont++;
}

//**********************************************
void user_loop_10ms(void)
{
#if 1	// link test
	static uint8_t ll,llR,a2dp,a2dpR,ppR,pp;
	app_handle_t sys_hdl = app_get_sys_handler();
	ll = hci_get_acl_link_count(sys_hdl->unit); 	// BT 是否連線
	a2dp = a2dp_has_connection(); 	//
	pp = a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE);

	if(llR !=ll || a2dpR !=a2dp || ppR !=pp){
		llR=ll;
		a2dpR =a2dp;
		ppR =pp;
		USER_DBG_INFO("xxxxxx ll:%d  A2dp:%d  pp:%d\n", ll,a2dp,pp);

	}
#endif
	if(PowerDownFg) return;
	//==========================
#ifdef LCD_ST7789_EN
	if(ST7789V_main()==0){
		//---------------------
		if(MenuF!=3){
			MenuCont++;
			if(MenuCont==500){
				MenuF=3;
				MenuCont=0;
				DisplayMenuChg();
			}
		}
		//---------------------
		if(UpDisplay){
			UpDisplay = 0;
			DisplayMenu();
		}
	}
#endif
	static uint16_t Init_Cont = 0;
	//======================================
	if(Init_Cont == 150){
		SysMsCont=0;
		//---------------------------------
		if(EF_Maim())	return;
	#ifdef ACM86xxId1
		if(ACM_main())	return;;
		//------------------------------
		static uint16_t ms500 = 0;
		if(++ms500 == 100){
			ms500 = 0;
			ACM_REPORT();
		}
	#endif
	#ifdef	CONFIG_AUD_AMP_DET_FFT
		//============================
		static uint8_t led_30ms=0;
		if(++led_30ms >=3){
			led_30ms=0;
			void RGB_LED_EF_FUN(void);
			RGB_LED_EF_FUN();
		}
	#endif
	#if defined(TL8025)||defined(TL8035)||defined(TL820)||defined(TL8025v2)
		BattFun();
	#endif
		//======================
		EC11_FUN();
#ifdef EC11_Encoder2
		EC11_FUN2();
#endif
		//-------------------
		user_WorkModefun();

		//=====================================
#ifdef AMG8802
		AMG82xx_Main();
#endif
#ifdef __Kt_WirelessMicRxDrv_H
		//-----------------
		KT56_main();
		//-----------------
		void READ_BURST_FUN();
		if(RX_COMM_T != RX_COMM_B)	READ_BURST_FUN();
#endif

#ifdef HwSpiTest
		//==== HW SPI TEST ============
		extern int user_spi_is_busy(void);
		extern int user_spi_transfer(uint8_t* tx_buf, int tx_sz, uint8_t* rx_buf, int rx_sz, uint8_t tx_addr_fixed);
        while(user_spi_is_busy()){};
        //user_spi_read((uint8_t*)spi_rx_buf, sizeof(spi_rx_buf));
        //user_spi_write((uint8_t*)spi_tx_buf, sizeof(spi_tx_buf), 0);
		user_spi_transfer((uint8_t*)spi_tx_buf, 1, NULL, 0, 0);
     // user_spi_transfer((uint8_t*)spi_tx_buf, sizeof(spi_tx_buf), (uint8_t*)spi_rx_buf, sizeof(spi_rx_buf), 0);
#endif

		//--------------------
		user_key_scan();

#ifdef IR_TX
		if(++IrTxCont>50 && IrTxTime){
			IrTxCont = 0;
			IrTxTime--;
			extern void ir_tx_init(uint8_t En);
			ir_tx_init(1);
		}
#endif
		if(SysMsCont>8) USER_DBG_INFO("====SysMsCont:%d\n", SysMsCont);
	}else{
		Init_Cont++;
		//======================================================================
		if(Init_Cont == 1){
#ifdef ACM86xxId1
			gpio_output(AMP_EN, 1);
		}else if(Init_Cont == 10){
			ACM8625_init();
		}else if(Init_Cont == 11){
			if(ACM_main()){
				Init_Cont--;
			}else{
		#ifdef TL8035
				//==== float L2L, R2L, L2R, R2R  -110 ~ +48db ===============
				ACM86_InputMixer(0,-110,-110,-110);
		#endif
				USER_DBG_INFO("==== 11. ACM.1_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
		}else if(Init_Cont == 18 && ACM862x_IIC_ADDR[1]){
			ACM862xWId = 1;
			SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
			if(ACM_main()){
				Init_Cont--;
			}else{
		#ifdef TL8035
				//==== float L2L, R2L, L2R, R2R  -110 ~ +48db ===============
				ACM86_InputMixer(-110,0,-110,-110);
		#endif
				ACM862xWId = 0;
				SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
				USER_DBG_INFO("==== 12. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
		}else if(Init_Cont == 19){
			EF_EQ_ReSend();
			EF_ModeR = EF_Mode;
			EQ_ModeR = EQ_Mode;
			USER_DBG_INFO("==== 13. EQ_ReSend Ok...MsCont:%d \n",SysMsCont);
#endif
		}else if(Init_Cont == 20){
	#ifdef KT56
			KT56_Init();
	#endif
			gpio_output(AMP_EN, 1);
		}else if(Init_Cont == 21){
			USER_DBG_INFO("====13. Init IoGain...\n",SysMsCont);
			// 需等待 DSP 啟動完成才可開始配置 gain
			IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
			IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
			IOGainSetDamp(Out1L_DacL, ioGain[Out1L_DacL],64);
			IOGainSetDamp(Out1R_DacR, ioGain[Out1R_DacR],64);
			IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],64);
			IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);

//			IOGainSetDamp(MusL_Out2L, 0,64);
//			IOGainSetDamp(MusR_Out2R, 0,64);
//			IOGainSetDamp(Play_MusL, 0,64);
//			IOGainSetDamp(Play_MusR, 0,64);

		}else if(Init_Cont == 50){
	#ifdef HS2450
			gpio_output(IR_AMP_EN, 0);
	#endif
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
		}else if(Init_Cont == 101){
			EF_EQ_ReSend();
			EQ_ModeR = EQ_Mode;
		//----------------------------
		}else if(Init_Cont == 102){
			g_dsp_log_on_fg = 0;
			g_mcu_log_on_fg = 0;
		}else if(Init_Cont == 103){
			Send_Id1 = CmdPid;
			Send_Id2 = 0x80;
			SendUpComp(SendBuff, 32);	// 請上位機連線
		}else if(Init_Cont == 104){
			g_dsp_log_on_fg = 1;
			g_mcu_log_on_fg = 1;
		}else if(Init_Cont == 148){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;

			if(SysInf.WorkMode != SYS_WM_UDISK_MODE){
				system_work_mode_set_button(SysInf.WorkMode);
				SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
			}
#ifdef PCM1865
			extern void PCM1865_Init();
			PCM1865_Init();
#endif
#ifdef	CONFIG_AUD_AMP_DET_FFT
			extern void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
			aud_mag_det_enable(1);	//audio magnitude detect enable
#endif
		}
	}
}

//***************************************************
void Out_VolSet(float val)
{

}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	static uint8_t play_en =0;
	if(play_en){
		app_wave_file_play_start(APP_WAVE_FILE_ID_REDIAL);
		switch (Gr){
			case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
			case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
			case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
			case 3:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);	break;
			case 4:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);	break;
		}
	}else{
		play_en =1;
	}
	LCD_ShowxNum(233, 214, EF_Mode , 1, 16, 0);
	SysInf.EfGruup = Gr;
}
//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
#if 0
	static uint8_t play_en =0;
	if(play_en){
		app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		switch (Gr){
			case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
			case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
			case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		}
	}else{
		play_en =1;
	}
#endif
	SysInf.EqGruup = Gr;
}

//***********************************************************************************
void DisplayMenu()
{
#ifdef LCD_ST7789_EN
	if(MenuF==3){//显示数字
		ShowNum(35, 90, (uint8_t)SysInf.MusicVol,2,GREEN, BACK_COLOR);
	}else if(MenuF==9){
		ShowNum(107, 90, SysInf.MicVol,2,GREEN, BACK_COLOR);
	}else if(MenuF==23){
		ShowNum(179, 90, (UserEf.EchoVol*10),2,GREEN, BACK_COLOR);
	}else if(MenuF==13){
		ShowNum(253, 90, (UserEf.RevVol*10),2,GREEN, BACK_COLOR);
	}
#endif
}
//***********************************************************************************
void DisplayMenuChg()//显示方框颜色
{
#ifdef LCD_ST7789_EN
	if(MenuFR != MenuF){
		if(MenuF==3){//显示数字
			ShowNum(35, 90, SysInf.MusicVol,2,GREEN, BACK_COLOR);
			LCD_PutString(33, 133, (u8*)"MUSIC", 16, GREEN, BACK_COLOR);
			DMAimage_display(45,155,(u8*)gImage_Mus_icon_gre);//音乐
		}else if(MenuF==9){
			ShowNum(107, 90, SysInf.MicVol,2,GREEN, BACK_COLOR);
			LCD_PutString(112, 133, (u8*)"MIC", 16, GREEN, BACK_COLOR);
			DMAimage_display(118,157,(u8*)gImage_voice_tube_gre);
		}else if(MenuF==23){
			ShowNum(179, 90, (UserEf.EchoVol*10),2,GREEN, BACK_COLOR);
			LCD_PutString(181, 133, (u8*)"EHCO", 16, GREEN, BACK_COLOR);
			DMAimage_display(189,160,(u8*)gImage_ehco_icon_gre);// EF MODE
		}else if(MenuF==13){
			ShowNum(253, 90, (UserEf.RevVol*10),2,GREEN, BACK_COLOR);
			LCD_PutString(246, 133, (u8*)"REVERB", 16, GREEN, BACK_COLOR);
			DMAimage_display(262,158,(u8*)gImage_rev_icon_gre);// WORK MODE
		}
		//======================================================================
		if(MenuFR==3){//显示数字
			ShowNum(35, 90, SysInf.MusicVol,2,WHITE, BACK_COLOR);
			LCD_PutString(33, 133, (u8*)"MUSIC", 16, WHITE, BACK_COLOR);
			DMAimage_display(45,155,(u8*)gImage_Mus_icon);//音乐
		}else if(MenuFR==9){
			ShowNum(107, 90, SysInf.MicVol,2,WHITE, BACK_COLOR);
			LCD_PutString(112, 133, (u8*)"MIC", 16, WHITE, BACK_COLOR);
			DMAimage_display(118,157,(u8*)gImage_voice_tube);
		}else if(MenuFR==23){
			ShowNum(179, 90, (UserEf.EchoVol*10),2,WHITE, BACK_COLOR);
			LCD_PutString(181, 133, (u8*)"EHCO", 16, WHITE, BACK_COLOR);
			DMAimage_display(189,160,(u8*)gImage_ehco_icon);	// ECHO
		}else if(MenuFR==13){
			ShowNum(253, 90, (UserEf.RevVol*10),2,WHITE, BACK_COLOR);
			LCD_PutString(246, 133, (u8*)"REVERB", 16, WHITE, BACK_COLOR);
			DMAimage_display(262,158,(u8*)gImage_rev_icon);// REV
		}
		MenuFR = MenuF;
	}

#endif
}

//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
	switch(Id2){
	case 3:
		if(MenuF==3)	ShowNum(35, 90, SysInf.MusicVol,2,GREEN, BACK_COLOR);
			else		ShowNum(35, 90, SysInf.MusicVol,2,WHITE, BACK_COLOR);
		break;
	case 9:
		if(MenuF==9)	ShowNum(107, 90, SysInf.MicVol,2,GREEN, BACK_COLOR);
			else		ShowNum(107, 90, SysInf.MicVol,2,WHITE, BACK_COLOR);
		break;
	case 13:
		if(MenuF==13)	ShowNum(253, 90, (UserEf.RevVol*10),2,GREEN, BACK_COLOR);
			else		ShowNum(253, 90, (UserEf.RevVol*10),2,WHITE, BACK_COLOR);
		break;
	case 23:
		if(MenuF==23)	ShowNum(179, 90, (UserEf.EchoVol*10),2,GREEN, BACK_COLOR);
			else		ShowNum(179, 90, (UserEf.EchoVol*10),2,WHITE, BACK_COLOR);
		break;
	}
}

extern void KT56_IrPair(uint8_t AB);
extern void KT56_IrPair_Freq(uint8_t UpDn);
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
#ifdef	KeyDbg
	USER_DBG_INFO("====key: %d:  %d\n", pKeyArray->index, pKeyArray->event);
	return;
#endif
	switch (pKeyArray->event){
	case KEY_S_PRES: break;
	case KEY_S_REL:
		UpComputerRW = 0;
		switch (pKeyArray->index){
		case 0:	//
			if(IrTxTime){
				KT56_IrPair_Freq(1);	// ch +1
			}else{
				if((app_is_bt_mode() || app_is_udisk_mode())){
					app_button_sw_action(BUTTON_BT_PREV);
				}
			}
			break;
		case 1:	//
			if(IrTxTime){
				KT56_IrPair_Freq(0);	// ch -1
			}else{
				if((app_is_bt_mode() || app_is_udisk_mode())){
					app_button_sw_action(BUTTON_BT_NEXT);
				}
			}
			break;
		case 2:	// 輸入選擇
			system_work_mode_change_button();
			break;
		case 3:	// EF MODE
			if(++EF_Mode > 4u)	EF_Mode = 0u;
			break;
		}
		break;
	//====================================================================================================
	case KEY_D_CLICK:	// 短按 2次


		break;
	case KEY_T_CLICK:	DBG_LOG_INFO("KEY_T_CLICK\n");	break;	// 短按 3次
	case KEY_Q_CLICK:	DBG_LOG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	DBG_LOG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:
		DBG_LOG_INFO("KEY_L_PRES\n");
		switch (pKeyArray->index){	// 常按進入
			case 0:	//
				KT56_IrPair(1);
				IrTxTime = 60;
				break;
			case 1:	//
				KT56_IrPair(0);
				IrTxTime = 60;
				break;
		#ifdef RECED_EN
			case 2:	//
				user_RecPlayWork();
				break;
			case 3:	//
				user_RecWork();
				break;
		#endif
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->keepTime==3000){
			DBG_LOG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
//			PlayWaveStop(APP_WAVE_FILE_ID_POWEROFF);


		}
		break;
	case KEY_L_REL:	DBG_LOG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}

}

#ifdef KT56
//***************************************************************
void READ_BURST_FUN()
{
//	uint8_t TX_IR_DATA[5]={0x81,0x05,0x7A,0x00,0x00};

	if(++RX_COMM_B>4) RX_COMM_B = 0;
//	DBG_LOG_INFO("==== BURST: %d  %02X \r\n",(uint16_t)RX_COMM_B, (uint16_t)RX_COMM_DATA[RX_COMM_B][0]);
	switch(RX_COMM_DATA[RX_COMM_B][0]){
	//==== EF_MODE ===================
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
		EF_Mode = RX_COMM_DATA[RX_COMM_B][0]-0x80;	//	EfMode_Val = RX_COMM_DATA[RX_COMM_B][0]-0x80;
		break;
	//==== MUSIC++ ===================
	case 0x85:
		if(SysInf.MusicVol<32)	SysInf.MusicVol++;
//		DisPlay_UpData(MainPid,3);
		break;
	//==== Music-- ===================
	case 0x86:	//
		if(SysInf.MusicVol)	SysInf.MusicVol--;
//		DisPlay_UpData(MainPid,9);
		break;
	//================================
	case 0x87:
		break;
	//==== MIC VOL 0~ 4 ==============
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
		SysInf.MicVol= (RX_COMM_DATA[RX_COMM_B][0]-0x88)*8;
		MenuF=9;
		UpDisplay = 1;
//		SysInf.MicVol = MicVol_Tab[SysInf.MicVol32];
		break;
	//==== EQ_MODE ===================
	case 0x8D:
	case 0x8E:
	case 0x8F:
		EQ_Mode = RX_COMM_DATA[RX_COMM_B][0]-0x8D;	//	EqMode_Val = RX_COMM_DATA[RX_COMM_B][0]-0x8D;
		break;
	//======================================
	case 0x91:	 //Rep	點歌機重唱
		app_button_sw_action(BUTTON_BT_PREV);
		break;
	//======================================
	case 0x92:	 //Next	點歌機切歌
		app_button_sw_action(BUTTON_BT_NEXT);
		break;
	//======================================
	case 0x93:	 //Pause
		app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		break;
	//======================================
	case 0x94:	 //Voc
		SysInf.VoiceCanEn ^= 1;
		break;
	//======================================
//	case 0x95:	 // 美聲 ON
//		break;
   //======================================
//	case 0x96:	 // 美聲 OFF
//		break;
   //======================================
	case 0x97:	 // 男調	F->M
		SysInf.KeyShift = 4;
		break;
	//======================================
	case 0x98:	 // 女調	M->F
		SysInf.KeyShift = -4;
		break;
	//======================================
	case 0x99:	 // 原調
		SysInf.KeyShift = 0;
		break;
	//======================================
	case 0x9A:	 // 升調
		if(SysInf.KeyShift < 5) SysInf.KeyShift++;
		break;
	//======================================
	case 0x9B:	 // 降調
		if(SysInf.KeyShift > -5) SysInf.KeyShift--;
		break;
	//======================================
//	case 0xA1:	 // 聲控啟動
//		break;
	//======================================
//	case 0xA0:	 // 聲控關閉
//		break;
	}
}
#endif

//******************************************************************
void user_gpio_isr(uint32_t io0_31, uint32_t io32_39)
{
//	USER_DBG_INFO("io0_31:%p, io32_39:%p\n", io0_31, io32_39);
#ifdef __Kt_WirelessMicRxDrv_H
	extern uint8_t KT56_INT;
	if(io0_31 & 0x800000)	KT56_INT=1; 	// gpio14 MicA & MicB
#endif
}

//********************************
void PlayWaveStart(uint16_t id)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	USER_DBG_INFO("wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PowerDownFg=1;
		SysInf.VoiceCanEn = 0;	// 關閉消原音
		SysInf.WorkMode = sys_hdl->sys_work_mode;
		ST7789V_BL_EN(0);
		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{
	//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		SysInf.EfGruup = EF_Mode;

		gpio_output(AMP_EN, 0);
#ifdef HS2450
		gpio_output(GPIO11, 0);	//DAMP ON
#endif

		os_delay_us(160000);
	//	GroupWrite(EF_Mode, 0);		// 保存記憶
		GroupWrite(EF_Mode, 2);		// 保存記憶
		os_delay_us(160000);
		dsp_shutdown();

		gpio_output(POWER_EN, 0);
		gpio_config_new(POWER_EN, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);

		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF... %d  %d\n",SysInf.EfGruup,EF_Mode);
		break;
	case APP_WAVE_FILE_ID_CONN:
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
#ifdef REC_EN
	case APP_WAVE_FILE_ID_RESERVED4:	// 開始錄音
		RecState = 3;
		break;
	case APP_WAVE_FILE_ID_RESERVED5:	// 停止錄音
		RecState = 2;
		break;
#endif
	case APP_WAVE_FILE_ID_RESERVED0:
	case APP_WAVE_FILE_ID_MP3_MODE:
#ifdef REC_EN
		if(RecState==16){
			RecState = 6;
		}else
#endif
		{
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//自動播放
		}
		break;
	}
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}

#endif

