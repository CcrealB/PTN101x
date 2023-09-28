/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2023-03-20
* 描述：
*
**********************************************************************/
#include "USER_Config.h"
#ifdef DY_DT_K800
#include "PTN1012_DEMO_DEF.H"

//static uint16_t ms1000 = 0;

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
extern uint8 appPlayerPlayMode;
static uint8_t DuckDetF = 0;
//static uint8_t	BtLedFlash=0;

static uint16_t WorkModeCont=0;
uint8_t work_modeR=10;

uint8_t LineInR=0;

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
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
//				LCD_ShowMode(SYS_WM_BT_MODE);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
//				if(BtLedFlash^=1)	LCD_ShowMode(work_modeR);
//					else			LCD_ShowMode(SYS_WM_NULL);
			}
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
	//==== GPIO SET =======================
	#define	ACM86_EN	GPIO35
    gpio_config_new(ACM86_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(ACM86_EN, 1);

	#define	POWER_EN	GPIO21
    gpio_config_new(POWER_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(POWER_EN, 1);

	#define	MCU_EN	GPIO22	// DisPlay
    gpio_config_new(MCU_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(MCU_EN, 1);

	#define	BOOT_EN	GPIO30	// DisPlay
    gpio_config_new(BOOT_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(BOOT_EN, 1);


	USER_DBG_INFO("====GPIO_Init 1\n");



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


//	for(uint8_t i=0; i<8; i++) LCD_ShowDot(i, 1);

    //=============================
    extern void key_function(KEY_CONTEXT_t *pKeyArray);
    extern void user_key_init(void* pKeyFuncCbk);
    user_key_init(key_function);
    //---------------------
	u_SaradcKbon_Init();


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

		//----------------------按键扫描
		user_key_scan();

		//-------------------QN8035



	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){
#ifdef ACM86xx
			ACM8625_init();
			ACMIIR_ReSend();
		}else if(Init_Cont == 11){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
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
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
			//----------------------------
		}else if(Init_Cont == 100){
			EF_EQ_ReSend();
			//=====================
			SysMsCont=0;

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

//********************************************
void DisPlay_UpData(uint8_t Id2)
{

}

//***************************************************
void Mic_VolSet(float val)
{
	ioGain[MicMstVol] = val;
	IOGainSet(MicMstVol, val);
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
		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
		switch (EF_Mode){
			case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
			case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
			case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
			case 3:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);	break;
			case 4:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);	break;
		}
	}else{
		play_en =1;
	}
}
//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
	static uint8_t play_en =0;
	if(play_en){
		app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		}
	}else{
		play_en =1;
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
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	// 輸入選擇 模式切換
			system_work_mode_change_button();
			USER_DBG_INFO("==== 3.PlayType: %d\n", PlayType);
			break;
		case 1:	// PRE
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				app_button_sw_action(BUTTON_BT_PREV);
			}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
				//if(Fre_data >8750) Fre_data-=10;


			}
			break;
		case 2:	app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	break;
		case 3:	// NEXT
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				app_button_sw_action(BUTTON_BT_NEXT);
			}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
				//if(Fre_data < 10800) Fre_data+=10;

			//	USER_DBG_INFO("==== QND_RXSeekCHAll:%d\n",QND_RXSeekCHAll(qnd_CH_START, qnd_CH_STOP, qnd_CH_STEP, 0, 1));
			}

			break;
		case 4:	// 循環
			if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ALL_CYCLE){
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ONE;
		//			RgbFlash=2;
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);
				}else{
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ALL_CYCLE;
		//			RgbFlash=4;
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);
				}
				SysInf.SdCardPlayMode = appPlayerPlayMode;
			}
			break;
		case 5:	// 話筒優先
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
		case 6:	USER_DBG_INFO("QFM %d\n",I2C_ReadA8D8(0x10, 0x06)); break;// Delay Time -
		case 7:
			//while(qn8035_rx_sch()==0);

			break;// Delay Time +
		case 8:		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);	UserEf.Pitch = 0; break;	// 原音
		case 9:		app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);	break;	// 感謝
		case 10:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);	break;	// 掌聲
		case 11:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);	break;	// 笑聲
		case 12:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);	break;	// 烏鴉
		case 13:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);	break;	// 鄙視
		case 14:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);	UserEf.Pitch = +8; break;	// 女變男
		case 15:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);	UserEf.Pitch = -8; break;	// 男變女
		case 16:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);	UserEf.Pitch = +12; break;	// 娃娃音
		case 17:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	UserEf.Pitch = -12; break;	// 魔音
//			if(ll==0)	LCD_ShowDot(7, 0);
//				else	LCD_ShowDot(ll-1, 0);
//			LCD_ShowDot(ll, 1);
//			if(++ll > 7) ll=0;
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
			if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE)
			{

			}
			break;
		case 3:
			if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE)
			{

			}
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
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}

#endif


