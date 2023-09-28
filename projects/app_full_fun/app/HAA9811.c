/**********************************************************************
* 文件:   user_ui.c
* 作者:   Chen Yuan
* 版本:   V1.0
* 日期:   2023-04-10
* 描述：
* 話筒音量,  混響, 回音,  音樂音量
*
*	 	燈模式				EF MODE
* 上曲				電源/PLAY					下曲
* 		原唱					WORK MODE
* 		EQ MODE
*
**********************************************************************/
#include "USER_Config.h"
#ifdef HAA9811
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

uint8_t PlayType=2, PlayTypeR=0;
uint8_t LED_OnOff=0;
static uint16_t SysMsCont=0;

static uint16_t WorkModeCont=0;
uint8_t work_modeR=10;
//static uint8_t	BtLedFlash=0;


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
//				LCD_ShowMode(work_modeR);
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
//				LCD_ShowMode(work_modeR);
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
//				LCD_ShowMode(work_modeR);
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
	#define POWER_EN	GPIO24
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);

	#define DAMP_PO_EN	GPIO35
	gpio_config_new(DAMP_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(DAMP_PO_EN, 0);

	#define LED_PO_EN	GPIO12
	gpio_config_new(LED_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LED_PO_EN, LED_OnOff);

	#define EPHONE_MUTE_EN	GPIO16
	gpio_config_new(EPHONE_MUTE_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(EPHONE_MUTE_EN, 0);
}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init(18);	// 設置FLASH 傳入版本參數
}

static void key_function(KEY_CONTEXT_t *pKeyArray);
//********************************
void user_init_post(void)
{
    //===============================
    user_key_init(key_function);

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
		//----------------------
		if(EF_Maim()) return;

		//----------------------
		user_WorkModefun();

		//----------------------
		Knob_function();

		//----------------------
		user_key_scan();


		if(SysMsCont>5) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 10){
			gpio_output(DAMP_PO_EN, 1);
			gpio_output(EPHONE_MUTE_EN, 1);

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
			app_h->sys_work_mode = SYS_WM_BT_MODE;//蓝牙模式
			system_work_mode_set_button(SysInf.WorkMode);
		}
	}
}

//***************************************************
void Audio_Def()
{
	SysInf.MusicVol = 0;
	SysInf.MicVol = 0;
	SysInf.MicCompT = -10;
	SysInf.MusCompT = -10;

	ioGain[Tone_I2s2LR] = -30;
	ioGain[Tone_DacLR] = -30;

	ioGain[I2s2L_Mic] = 3;
	ioGain[I2s2R_Mic] = 3;

	ioGain[Out2L_RecL] = -10;
	ioGain[Out2R_RecR] = -10;

	ioGain[Out1L_DacL] = 5;
	ioGain[Out1R_DacR] = 5;
	ioGain[Out2L_I2s2L] = 0;
	ioGain[Out2R_I2s2R] = 0;

//	ioGain[MusL_Out2L] = 0;
//	ioGain[MusR_Out2R] = 0;

	ioGain[Adc2_MusL] = 0;
	ioGain[Adc3_MusR] = 0;
//	ioGain[Out2L_RecL] = 0;
//	ioGain[Out2R_RecR] = 0;

	//--------------------------------
	for(uint8_t i=MicEq0_0; i<(MicEq4_0); i+=10){
		UserEq[i].type = 2;
		UserEq[i].freq = 100;
		UserEq[i+2].freq = 125;
		UserEq[i+2].gain = 20;
		UserEq[i+7].Q = 400;
		UserEq[i+7].gain = 40;
		UserEq[i+8].gain = 20;
		UserEq[i+9].type = 1;
		UserEq[i+9].freq = 9000;
	}

	UserEq[OutEq0_0].type = eq_HPF;
	UserEq[OutEq0_0].freq = 400;
	UserEq[OutEq0_0].Q = 700;
	UserEq[OutEq0_0].gain = 0;

	UserEq[OutEq1_9].type = eq_LPF;
	UserEq[OutEq1_9].freq = 400;
	UserEq[OutEq1_9].Q = 700;
	UserEq[OutEq1_9].gain = 0;

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
	ioGain[Adc4_Mic] = val;
	ioGain[Adc5_Mic] = val;
	IOGainSet(Adc4_Mic, val);
	IOGainSet(Adc5_Mic, val);
	ioGain[I2s2L_Mic] = val;
	ioGain[I2s2R_Mic] = val;
	IOGainSet(I2s2L_Mic, val);
	IOGainSet(I2s2R_Mic, val);
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
	#define MaxV	4000	// 最大ADC 電壓 (n/4096)
	#define CenV	3300	// 旋鈕中心值電壓
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
	for(uint8_t i=1; i<5; i++){
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
			//====================================================
			switch (i){
			case 1:	// Mic Vol
				SysInf.MicVol = VolVar;
				break;
			case 2:	// REV Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = UserEf.RevVol;
				break;
			case 3:	// Echo Vol
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = UserEf.EchoVol;
				break;
			case 4:	// Music Vol
				SysInf.MusicVol = VolVar;
				break;
			}
			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f\n",i, SarAdcVal[i], VolVar, KeyVar, EqVar, EfVar);
			SarAdcValR[i]=SarAdcVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
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
		case 0:	// PLAY/PAUSE
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	break;
			break;
		case 1:	// PRE
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				app_button_sw_action(BUTTON_BT_PREV);
			}
			break;
		case 2:	// NEXT
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				app_button_sw_action(BUTTON_BT_NEXT);
			}
			break;
		case 3:	// LED ON/OFF
			LED_OnOff ^= 1;
			gpio_output(LED_PO_EN, LED_OnOff);
			break;
		case 4:	// EF MODE
			if(++EF_Mode > 4u)	EF_Mode = 0u;
			break;
		case 5:	// EQ MODE
//			if(++EQ_Mode > 2u)	EQ_Mode = 0u;
			break;
		case 6:	// WORK MODE
			system_work_mode_change_button();
			USER_DBG_INFO("==== 3.PlayType: %d\n", PlayType);
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
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->index==0 && pKeyArray->keepTime > 100){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}
}

//********************************
void PlayWaveStart(uint16_t id)
{
	app_handle_t sys_hdl = app_get_sys_handler();

	USER_DBG_INFO("wave_start, ID:%d\n", id);

	IOGainSetDamp(MusL_Out1L, -90,10);
	IOGainSetDamp(MusR_Out1R, -90,10);
	IOGainSetDamp(MusL_Out2L, -90,10);
	IOGainSetDamp(MusR_Out2R, -90,10);

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PowerDownFg=1;
		SysInf.VoiceCanEn = 0;	// 關閉消原音
//		SysInf.MusVol32 = MusVolVal;
		SysInf.WorkMode = sys_hdl->sys_work_mode;
		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{
	//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:

		dsp_shutdown();
		gpio_output(DAMP_PO_EN, 0);
		gpio_output(POWER_EN, 0);

		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		break;
	}
	IOGainSetDamp(MusL_Out1L, SysInf.MusicVol,10);
	IOGainSetDamp(MusR_Out1R, SysInf.MusicVol,10);
	IOGainSetDamp(MusL_Out2L, SysInf.MusicVol,10);
	IOGainSetDamp(MusR_Out2R, SysInf.MusicVol,10);
}
#endif


