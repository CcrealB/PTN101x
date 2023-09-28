/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2023-03-20
* 描述：
*
* 旋鈕功能:
* Music Vol		BASS 		MID		TREBLE
* Mc Vol		BASS 		MID		TREBLE
* REV1 Vol		REV2 Vol 	DELAY	REPEAT
* ECHO1 Vol		ECHO2 Vol 	DELAY	REPEAT
*
*按鍵功能:
*	变调复位		魔音
*	模拟DL加		娃娃音
*	模拟DL减		咪女变男
*	话筒优先		咪男变女
*	循环按键		鄙视
*	下一曲		烏鴉
*	播放暂停		笑聲
*	上一曲		掌声
*	输入选择		感謝
**********************************************************************/
#include "USER_Config.h"
#ifdef SY_K59
#include "driver_beken_includes.h"    //bk  片上外设硬件
#include "app_beken_includes.h"       //bk  应用层

//static uint16_t ms1000 = 0;

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
extern uint8 appPlayerPlayMode;
static uint8_t DuckDetF = 0;
//static uint8_t	BtLedFlash=0;

static uint16_t WorkModeCont=0;
uint8_t work_modeR=10;

uint8_t LineInR=0;
uint8_t LineIn2R=0;

static uint16_t SysMsCont=0;

//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{
/*
	//---- LINE DET ---------------------------
	static uint8 LineInDetCont=0;
	LineIn = gpio_input(LINE_DETECT_IO);
//	USER_DBG_INFO("====1. AuxIn: %d\n",AuxIn);
	if(LineIn){
		gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
		os_delay_us(500);
		if(gpio_input(LINE_DETECT_IO)) LineIn = 0;
//		USER_DBG_INFO("====2. AuxIn: %d\n",AuxIn);
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
			USER_DBG_INFO("==== AuxInDet %d\n",LineInR);
		}
	}else{
		LineInDetCont=0;
	}
*/
	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================//系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;

			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
//			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
//				LCD_ShowMode(SYS_WM_BT_MODE);
//			}else{                                               //如果不是连接状态，蓝色灯闪烁
//				if(BtLedFlash^=1)	LCD_ShowMode(work_modeR);
//					else			LCD_ShowMode(SYS_WM_NULL);
//			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;

			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			//-----------------------------------
//			if(RgbFlash){	//2=閃-次		4=閃兩次
//				RgbFlash--;
//				if(RgbFlash%2==0)	RgbLedOut(2);	// W
//					else			RgbLedOut(8);
//				WorkModeCont = 50;
//			}
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;


				//	if(ioGain[Play_MusL] < -15){
					ioGain[Play_MusL]= -5;
					ioGain[Play_MusR]= ioGain[Play_MusL];
					ioGain[Adc2_MusL]= -90;
					ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
		//==================================================//系统工作模式：FM卡模式和AUX模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE || sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;

				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 0); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
				}else{
					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 1); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
				}

			//	if(ioGain[Adc2_MusL] < - 15){
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -4;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
//				PlayIng =1;
			}
		//==================================================//系统工作模式：SPDIF模式
//		}else if(sys_hdl->sys_work_mode == SYS_WM_SPDIF_MODE){
//			if(work_modeR != sys_hdl->sys_work_mode){
//				work_modeR = sys_hdl->sys_work_mode;

//				LCD_ShowMode(work_modeR);
//				ioGain[Play_MusL]= -5;
//				ioGain[Play_MusR]= ioGain[Play_MusL];
//				ioGain[Adc2_MusL]= -90;
//				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
//			}
		}

	}
}

//**** pre init ***********************
void user_init_prv(void)
{
//	#define POWER_EN	GPIO24
//	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//	gpio_output(POWER_EN, 1);

	#define DAMP_PO_EN	GPIO35
	gpio_config_new(DAMP_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(DAMP_PO_EN, 0);	// 0 = SHDOWM

	#define DAMP_MUTE	GPIO30
	gpio_config_new(DAMP_MUTE, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(DAMP_MUTE, 1);	// 1= MUTE

	#define EPHONE_MUTE_EN	GPIO12
	gpio_config_new(EPHONE_MUTE_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(EPHONE_MUTE_EN, 0);	// 0=MUTE

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init(17);	// 設置FLASH 傳入版本參數

	appPlayerPlayMode = SysInf.SdCardPlayMode;
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;

	USER_DBG_INFO("======== SY_K59_V01 WorkMode:%d\n",SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

}

//********************************
void user_init_post(void)
{
    //=============================
    extern void key_function(KEY_CONTEXT_t *pKeyArray);
    extern void user_key_init(void* pKeyFuncCbk);
    user_key_init(key_function);
    //---------------------
	u_SaradcKbon_Init();

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
#ifdef CONFIG_SYS_TICK_INT_1MS
void tick_task_1ms(void) //timer0 1ms isr
{
	//------------------------------
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
	if(Tick_2ms)	user_saradc_update_trig();	// Adc 讀取觸發
	SysMsCont++;
}
#endif


void Knob_function();
static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
		//----------------------------
		if(EF_Maim()) return;

		//-------------------控制LCD显示和音源链路音量设置
		user_WorkModefun();

		//----------------------编码器旋钮
		Knob_function();

		//----------------------按键扫描
		user_key_scan();

		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 10){
			gpio_output(DAMP_PO_EN, 1);	// 0 = SHDOWM
			gpio_output(DAMP_MUTE, 0);	// 1= MUTE
			gpio_output(EPHONE_MUTE_EN, 1);	// 0=MUTE

		}else if(Init_Cont == 13){
			USER_DBG_INFO("====13. Init IoGain...\n",SysMsCont);
			// 需等待 DSP 啟動完成才可開始配置 gain
			IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
			IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
			IOGainSetDamp(Out1L_DacL, ioGain[Out1L_DacL],64);
			IOGainSetDamp(Out1R_DacR, ioGain[Out1R_DacR],64);
			IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],64);
			IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);

			IOGainSetDamp(MusL_Out2L, 0,64);
			IOGainSetDamp(MusR_Out2R, 0,64);
			IOGainSetDamp(Play_MusL, 0,64);
			IOGainSetDamp(Play_MusR, 0,64);

		}else if(Init_Cont == 50){
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
			//----------------------------
		}else if(Init_Cont == 101){
			EF_EQ_ReSend();

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


		}else if(Init_Cont == 148){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_LINEIN2_MODE;           //一上电设置为FM模式
			system_work_mode_set_button(app_h->sys_work_mode);
		}

	}
}

//***************************************************
void Audio_Def()
{
	SysInf.MusicVol = -20;
	SysInf.MicVol = 0;

	ioGain[Tone_I2s2LR] = -30;
	ioGain[Tone_DacLR] = -30;

	ioGain[Play_MusL] = -5;
	ioGain[Play_MusR] = -5;
	ioGain[Usb_MusL] = -5;
	ioGain[Usb_MusR] = -5;

//	ioGain[Out1L_RecL] = -10;
//	ioGain[Out1R_RecR] = -10;

//	ioGain[Out1L_DacL] = 0;
//	ioGain[Out1R_DacR] = 0;

}
//********************************************
void DisPlay_UpData(uint8_t Id2)
{

}
//***************************************************
void Music32_VolSet(uint8_t val)
{
	SysInf.MusVol32 = val;
}
//***************************************************
void Music_VolSet(float val)
{
	ioGain[MusL_Out1L] = val;
	ioGain[MusR_Out1R] = val;
	IOGainSet(MusL_Out1L, val);
	IOGainSet(MusR_Out1R, val);
	ioGain[MusL_Out2L] = val;
	ioGain[MusR_Out2R] = val;
	IOGainSet(MusL_Out2L, val);
	IOGainSet(MusR_Out2R, val);
}
//***************************************************
void Mic_VolSet(float val)
{
#if 0
	ioGain[Adc4_Mic] = val;
	ioGain[Adc5_Mic] = val;
	IOGainSet(Adc4_Mic, val);
	IOGainSet(Adc5_Mic, val);
//	ioGain[I2s2L_Mic] = val;
//	ioGain[I2s2R_Mic] = val;
//	IOGainSet(I2s2L_Mic, val);
//	IOGainSet(I2s2R_Mic, val);
#else
	ioGain[Adc1_Mic] = val;
	IOGainSet(Adc1_Mic, val);
#endif
}
//***************************************************
void Out_VolSet(float val)
{

}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
	switch (EF_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		case 3:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);	break;
		case 4:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);	break;
	}

}
//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
	switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
	}
}

extern void UpDisplayOff();
//********************************
void ShowPowerOff(void)
{
//	UpDisplayOff();
//	gpio_output(POWER_EN, 0);
}

//***************************************************
void Knob_function()
{
#if 1
	#define MaxV	4095	// 最大ADC 電壓 (n/4096)
	#define CenV	2048	// 旋鈕中心值電壓
	#define ScopeV	(MaxV/100)	//36

	float VolUp10 = (MaxV-CenV)/10;
	float EqUp12 =	(MaxV-CenV)/120;
	float KeyUp6 = (MaxV-CenV)/6;
	float EfUp10 = (MaxV-CenV)/10;

	float VolDn60 = (CenV/60);
	float EqDn12 =	(CenV/120);
	float KeyDn6 =  (CenV/6);
	float EfDn10 = (CenV/10);

	int VolVar;
	int KeyVar;
	int EqVar;
	float EfVar;
	//=== 處理 ADC 滑桿,旋鈕 ===========
	for(uint8_t i=0; i<8; i++){
//		USER_DBG_INFO("==== test KnobValR[%d]: %d   %d   %d\n",i, KnobVal[i],KnobValR[i],abs(KnobValR[i]-KnobVal[i]));
		if(abs(KnobValR[i]-KnobVal[i])>=ScopeV){
			if(KnobVal[i]>MaxV){
				KnobValR[i] = MaxV;
			}else if(KnobVal[i]<ScopeV){
				KnobValR[i] = 0;
			}else{
				KnobValR[i] = KnobVal[i];
			}
			if(KnobValR[i] >= CenV){
				VolVar = (KnobValR[i]-CenV)/VolUp10;
				KeyVar = (KnobValR[i]-CenV)/KeyUp6;
				EqVar =  (KnobValR[i]-CenV)/EqUp12;
				EfVar =  (KnobValR[i]-CenV)/EfUp10+10;
			}else{
				VolVar =(KnobValR[i]/VolDn60)-60;
				if(VolVar <-59) VolVar = -90;
				KeyVar =(KnobValR[i]/KeyDn6)-6;
				EqVar = (KnobValR[i]/EqDn12)-120;
				EfVar =  (KnobValR[i]/EfDn10);
			}
			//====================================================
			switch (i){
			case 0:	// 直播  Vol
				ioGain[Out2L_I2s2L] = VolVar;
				ioGain[Out2R_I2s2R] = VolVar;
				ioGain[Out2L_RecL] = VolVar;
				ioGain[Out2R_RecR] = VolVar;
				break;
			case 1:	// 樂器 REV
				UserEf.RevVol2 = (float)EfVar/20;
				break;
			case 2:	// 樂器 V0l
				ioGain[Adc5_Mic] = VolVar;
				break;
			case 3:	// 監聽 Vol
				ioGain[Out1L_DacL] = VolVar;
				ioGain[Out1R_DacR] = VolVar;
				break;
			//---------------------------------------------
			case 4:	// Mic Vol
				ioGain[Adc4_Mic] = VolVar;
				break;
			case 5:	// Mic Bass
				UserEq[MicEq3_2].gain = EqVar;
				break;
			case 6:	// MIC REV
				UserEf.RevVol = (float)EfVar/20;
				UserEf.EchoVol = (float)EfVar/10;
				break;
			case 7:	// Mic Tre
				UserEq[MicEq3_8].gain = EqVar;
				break;
			}
			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], VolVar, KeyVar, EqVar, EfVar);
			KnobValR[i]=KnobVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
	if(abs(SarAdcValR[5]-SarAdcVal[5])>=ScopeV){
		if(SarAdcVal[5]>MaxV){
			SarAdcValR[5] = MaxV;
		}else if(SarAdcVal[5]<ScopeV){
			SarAdcValR[5] = 0;
		}else{
			SarAdcValR[5] = SarAdcVal[5];
		}
		if(SarAdcValR[5] >= CenV){
			VolVar = (SarAdcValR[5]-CenV)/VolUp10;
		}else{
			VolVar =(SarAdcValR[5]/VolDn60)-60;
			if(VolVar <-59) VolVar = -90;
		}
		SysInf.MusicVol = VolVar;
		USER_DBG_INFO("====SarAdcValR5: %d %d\n",SarAdcValR[5], VolVar);
	}
#endif
}

//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	// Music Bass-
			if(UserEq[MusicEq2].gain > -120) UserEq[MusicEq2].gain-=10;
			break;
		case 1:	// Music Bass+
			if(UserEq[MusicEq2].gain > -120) UserEq[MusicEq2].gain+=10;
			break;
		case 2:	// Music Tre-
			if(UserEq[MusicEq8].gain > -120) UserEq[MusicEq8].gain-=10;
			break;
		case 3:	// Music Tre+
			if(UserEq[MusicEq8].gain > -120) UserEq[MusicEq8].gain+=10;
			break;
		case 4:
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			break;
		case 5:	// PRE
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				app_button_sw_action(BUTTON_BT_PREV);
			}
			break;
		case 6:	// NEXT
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				app_button_sw_action(BUTTON_BT_NEXT);
			}
			break;
		case 7:	// 輸入選擇 模式切換
			system_work_mode_change_button();
			USER_DBG_INFO("==== 3.PlayType: %d\n", PlayType);
			break;
		case 8:		app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);	break;	// 感謝
		case 9:		app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);	break;	// 掌聲
		case 10:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);	break;	// 笑聲
		case 11:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);	break;	// 烏鴉
		case 12:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);	break;	// 鄙視
		case 13:	// 藍芽斷開
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			}
			break;
		case 14:	// WM SW	// 消原音
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
		case 15:	// 話筒優先
			app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
			if(DuckDetF==0){
				DuckDetF=1;
				SysInf.DuckDet = 0x18;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
			}else{
				DuckDetF=0;
				SysInf.DuckDet = 0x18;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
			}
			break;
		case 16:	// 錄音回放 /刪除錄音
			break;
		case 17:	// 錄音
			break;

		}
		break;
	//====================================================================================================
	case KEY_D_CLICK:
		USER_DBG_INFO("KEY_D_CLICK\n");	// 短按 2次

		break;
	case KEY_T_CLICK:
		USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);	// 短按 3次

		break;
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:
		USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
		switch (pKeyArray->index){
		case 8:	// 消原音
			SysInf.VoiceCanEn ^=1;
			app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
			if(SysInf.VoiceCanEn)	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				else				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
			break;
		case 1:

			break;
		case 3:

			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->keepTime > 100){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}
}

//********************************
#ifdef PromptToneMusicMute
void PlayWaveStart(uint16_t id)
{
#if 0
	if(id==APP_WAVE_FILE_ID_ENTER_PARING || id==APP_WAVE_FILE_ID_HFP_RING){
		USER_DBG_INFO("!!!!!!!!!!!!!! wave_start, ID:%d\n", id);
		return;
	}
#endif
	USER_DBG_INFO("wave_start, ID:%d\n", id);
//	IOGainSetDamp(MusL_Out1L, (SysInf.MusicVol-10),10);		//Music Off
//	IOGainSetDamp(MusR_Out1R, (SysInf.MusicVol-10),10);
	IOGainSetDamp(MusL_Out1L, (-90),10);		//Music Off
	IOGainSetDamp(MusR_Out1R, (-90),10);
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:

		break;

	case APP_WAVE_FILE_ID_DISCONN:

		break;
	}
}
#endif

//********************************
void PlayWaveStop(uint16_t id)
{
	//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:

		dsp_shutdown();
		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		break;
	case APP_WAVE_FILE_ID_CONN:
		app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
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

	}
	//=========================================================
//	SwChgIng = 0;
	if(id < INVALID_WAVE_EVENT){
		IOGainSetDamp(MusL_Out1L,SysInf.MusicVol,10);
		IOGainSetDamp(MusR_Out1R,SysInf.MusicVol,10);
		USER_DBG_INFO("==== wave_stop, ID:%d\n", id);
	}
}
#endif


