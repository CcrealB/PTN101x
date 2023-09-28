/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
#include "USER_Config.h"
#ifdef SK_460

#include "PTN1012_DEMO_DEF.H"

//static uint16_t ms1000 = 0;

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
//static uint8_t SdCardPlayMode=0;
//static uint8_t DuckDetF = 0;
//static uint8_t	SdCareDetR;

static uint16_t WorkModeCont=0;
//uint8_t work_modeR=10;
static uint16_t SysMsCont=0;

uint8_t	LineInR=0;

//*****************************************************
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

	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态

		app_handle_t sys_hdl = app_get_sys_handler();
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				gpio_output(LED0, 0);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
//				gpio_output(LED0, 0);
			}else{
//				gpio_output(LED0, BtLedFlash^=1);
			}
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				RgbLedOut(2);	// W
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
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= 0;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				RgbLedOut(3);	// G
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -4;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
//				PlayIng =1;
			}
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_SPDIF_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				ioGain[Play_MusL]= 0;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
		}

	}
}


//**** pre init ***********************
void user_init_prv(void)
{

	gpio_config_new(GPIO14, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(GPIO14, 0);

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數
}

//********************************
void user_init_post(void)
{
	//=========================
    Hi2c_Init();

#ifdef LED_DRIVER_BY_I2S0_TX
    extern void user_app_i2s0_init(uint8_t en);
	user_app_i2s0_init(1);
#endif
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
#ifdef udrv_saradc
	static uint8_t Tick_cont =0;
	if(++Tick_cont>2)	user_saradc_update_trig();	// Adc 讀取觸發
#endif
}


void Knob_function();
static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
		//===================
	#ifdef ACM86xxId1
		if(ACM_main())	return;;
	#endif
		//----------------------------
		if(EF_Maim()) return;

		//-------------------
		user_WorkModefun();

		//----------------------
//		Knob_function();

		//----------------------
//		user_key_scan();

		//------------------------------
		static uint16_t ms500 = 0;
		if(++ms500 == 50){
			ms500 = 0;
	#ifdef ACM86xxId1
			ACM_REPORT();
	#endif
		}


	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 10){
	#ifdef ACM86xxId1
			ACM8625_init();
			ACMIIR_ReSend();
		}else if(Init_Cont == 11){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
	#endif
	#ifdef TAS5760M
			extern void Tas5760M_init();
			Tas5760M_init();
			USER_DBG_INFO("==== 11. TAS5760M_init Ok...%d MsCont:%d \n",SysMsCont);
	#endif

		}else if(Init_Cont == 13){
			USER_DBG_INFO("====13. Init IoGain...\n",SysMsCont);
			// 需等待 DSP 啟動完成才可開始配置 gain
			IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
			IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
			IOGainSetDamp(Out1L_DacL, ioGain[Out1L_DacL],64);
			IOGainSetDamp(Out1R_DacR, ioGain[Out1R_DacR],64);
			IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],64);
			IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);
			IOGainSetDamp(Max2L_I2s3L, ioGain[Max2L_I2s3L],64);
			IOGainSetDamp(Max2R_I2s3R, ioGain[Max2R_I2s3R],64);
		}else if(Init_Cont == 50){
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
		//----------------------------
		}else if(Init_Cont == 101){
			EF_EQ_ReSend();
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
			app_h->sys_work_mode = SYS_WM_BT_MODE;	//蓝牙模式
			system_work_mode_set_button(SysInf.WorkMode);	// 開機設定存儲的模式
		//	system_work_mode_set_button(SYS_WM_SPDIF_MODE);	// 開機設定模式
		}

	}
}

//***************************************************
void Audio_Def()
{
	SysInf.MusicVol = -10;
	SysInf.MicVol = 0;
	SysInf.AngInGain = 0;

	ioGain[Tone_I2s2LR] = -30;
	ioGain[Tone_DacLR] = -30;
	ioGain[Out1L_DacL] = 0;
	ioGain[Out1R_DacR] = 0;
	ioGain[Out2L_I2s2L] = 0;
	ioGain[Out2R_I2s2R] = 0;
	ioGain[Max2L_I2s3L] = 0;
	ioGain[Max2R_I2s3R] = 0;

	ioGain[MusL_Out1L] = 0;
	ioGain[MusR_Out1R] = 0;
	ioGain[MusL_Out2L] = 0;
	ioGain[MusR_Out2R] = 0;


	ioGain[I2s2L_Mic] = -10;	// D MIC
	ioGain[I2s2R_Mic] = -10;

	ioGain[Adc2_MusL] = -10;	// LINE
	ioGain[Adc3_MusR] = -10;

	ioGain[Play_MusL] = 0;
	ioGain[Play_MusR] = 0;
	ioGain[Usb_MusL] = 0;
	ioGain[Usb_MusR] = 0;

	ioGain[Out2L_RecL] = 0;
	ioGain[Out2R_RecR] = 0;

}
//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{

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

#if (KbonNUM)
//***************************************************
void Knob_function()
{
	#define MaxV	3800	// 最大ADC 電壓 (n/4096)
	#define CenV	3400	// 旋鈕中心值電壓
	#define ScopeV	(MaxV/100)	//36
/*
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
*/
#ifndef PROJECT_V310
		return;
#endif
		/*
	//=== 處理 ADC 滑桿,旋鈕 ===========
	for(uint8_t i=0; i<7; i++){
		if(abs(SarAdcValR[i]-SarAdcVal[i])>=ScopeV){
			if(SarAdcVal[i]>MaxV){
				SarAdcValR[i] = MaxV;
			}else if(SarAdcVal[i]<ScopeV){
				SarAdcValR[i] = 0;
			}else{
				SarAdcValR[i] = SarAdcVal[i];
			}
			if(SarAdcValR[i] >= CenV){
				VolVar = (SarAdcValR[i]-CenV)/VolUp10;
				KeyVar = (SarAdcValR[i]-CenV)/KeyUp6;
				EqVar =  (SarAdcValR[i]-CenV)/EqUp12;
				EfVar =  (SarAdcValR[i]-CenV)/EfUp10+10;
			}else{
				VolVar =(SarAdcValR[i]/VolDn60)-60;
				if(VolVar <-59) VolVar = -90;
				KeyVar =(SarAdcValR[i]/KeyDn6)-6;
				EqVar = (SarAdcValR[i]/EqDn12)-120;
				EfVar =  (SarAdcValR[i]/EfDn10);
			}


	for(uint8_t i=0; i<8; i++){
		if(abs(KnobValR[i]-KnobVal[i])>=ScopeV){
			temp_cnt++;
			if(KnobVal[i]>MaxV){
				USER_DBG_INFO("====SarAdc MaxV:: %d \n",KnobVal[i]);
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

			// if(EqVar>0)
			// 	EqVar	= 0;
			// else if(EqVar<-2)
			// 	EqVar	= -2;	
			//====================================================
			switch (i){
#ifdef PROJECT_V310
			case 1:	// Mic Vol
				ioGain[Adc4_Mic] = VolVar-10;
				break;

			case 2:	// Mic Bass
				UserEq[MicEq4_2].gain = EqVar;
				break;

			case 3:	// Mic Tre
				UserEq[MicEq4_8].gain = EqVar;
				break;

			case 4:	// ECHO 
				// UserEf.RevVol = (float)EfVar/20;
				UserEf.EchoVol = (float)EfVar/10;
				// UserEf.RevVol2 = UserEf.RevVol;
				UserEf.EchoVol2 = UserEf.EchoVol;
				break;

			case 0:	// DELAY
				UserEf.EchoDeyT = EfVar*15;
				UserEf.lex_PreDelay = EfVar*5;
				break;

			case 5:	//  REV
				UserEf.RevVol = (float)EfVar/20;
				// UserEf.EchoVol = (float)EfVar/10;
				UserEf.RevVol2 = UserEf.RevVol;
				// UserEf.EchoVol2 = UserEf.EchoVol;
				// SysInf.MusicVol = VolVar;
				break;
#else
			case 0:	// Music Bass
				UserEq[MusicEq2].gain = EqVar;
				break;
			case 1:	// Music Tre
				UserEq[MusicEq8].gain = EqVar;
				break;
			case 2:	// ECHO & REV
				UserEf.RevVol = (float)EfVar/20;
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.RevVol2 = UserEf.RevVol;
				UserEf.EchoVol2 = UserEf.EchoVol;
				break;
			case 3:	// Mic Tre
				UserEq[MicEq4_8].gain = EqVar;
				break;
			case 4:	// Music Vol
				SysInf.MusicVol = VolVar;
				break;
			case 5:	// Mic Bass
				UserEq[MicEq4_2].gain = EqVar;
				break;
			case 6:	// jt Vol
				ioGain[Adc5_Mic] = VolVar-10;
				break;
			case 7:	// Mic Vol
				ioGain[Adc4_Mic] = VolVar-10;
				break;
#endif				
			}

			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], VolVar, KeyVar, EqVar, EfVar);
			KnobValR[i]=KnobVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
*/
}
#endif


#if (GPIO_KEY_NUM || ADC_KEY_NUM)
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);

#if 1
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
//		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	app_button_sw_action(BUTTON_BT_PREV);	break;
		case 1:	app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	break;
		case 2:	app_button_sw_action(BUTTON_BT_NEXT);	break;

		case 3:	// EF MODE

			break;
		case 4:	// 輸入選擇 模式切換
			system_work_mode_change_button();
			break;
/*
		case 5:	// 循環
			if(PlayType==2){	//判断当前是否是 SD 模式
				//APP_PLAYER_MODE_PLAY_ALL_CYCLE = 0,
				//APP_PLAYER_MODE_PLAY_ONE = 2,      
				//APP_PLAYER_MODE_PLAY_RANDOM = 3,  
				if(SdCardPlayMode==0){
					SdCardPlayMode=2;
				//	RgbFlash=2;
					set_app_player_play_mode(APP_PLAYER_MODE_PLAY_ONE);
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);
				}else{
					SdCardPlayMode=0;
				//	RgbFlash=4;
					set_app_player_play_mode(APP_PLAYER_MODE_PLAY_ALL_CYCLE);
					app_wave_file_play_start(APP_WAVE_FILE_ID_REDIAL);
				}
			}
			break;
		case 5:

			if(DuckDetF==0){
				DuckDetF=1;
				SysInf.DuckDet = 0x18;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
			}else{
				DuckDetF=0;
				SysInf.DuckDet = 0x18;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
			}
			break;// 話筒優先
		case 6:	break;// Delay Time -
		case 7:	break;// Delay Time +
		case 8:		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_CANCEL);	UserEf.Pitch = +4; break;	// 機器人

		case 9:		app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);	break;
		case 10:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);	break;
		case 11:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);	break;
		case 12:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);	break;
		case 13:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);	break;

		case 14:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);	UserEf.Pitch = +8; break;	// 女變男
		case 15:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);	UserEf.Pitch = -8; break;	// 男變女
		case 16:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);	UserEf.Pitch = +12; break;	// 兒童
		case 17:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	UserEf.Pitch = -12; break;	// 老人
//			if(ll==0)	LCD_ShowDot(7, 0);
//				else	LCD_ShowDot(ll-1, 0);
//			LCD_ShowDot(ll, 1);
//			if(++ll > 7) ll=0;

 */
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
		case 0:	// MODE

			break;
		case 3:	// 消原音
			SysInf.VoiceCanEn ^=1;
			app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
			if(SysInf.VoiceCanEn)	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				else				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
			break;
		case 8:	app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);	UserEf.Pitch = 0; break;	// 原音

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
#endif

}
#endif


//********************************
#ifdef PromptToneMusicMute
void PlayWaveStart(uint16_t id)
{
#if 1
	if(id==APP_WAVE_FILE_ID_ENTER_PARING || id==APP_WAVE_FILE_ID_HFP_RING){
		USER_DBG_INFO("!!!!!!!!!!!!!! wave_start, ID:%d\n", id);
		return;
	}
#endif
	USER_DBG_INFO("wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);
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
	}
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
}
#endif


