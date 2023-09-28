/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
#include "USER_Config.h"
#ifdef NW_TRX_01
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

const int8_t MicVol_Tab[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};
uint8_t Mus_ValR=255;
uint8_t Mic_ValR=255;

static uint16_t SysMsCont=0;

static uint16_t WorkModeCont=0;
uint8_t work_modeR=10;
//static uint8_t	BtLedFlash=0;

uint8_t LineInR=0;
uint8_t LineIn2R=0;

//*****************************************************
void user_WorkModefun(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
				ioGain[I2s2L_MusL]= -90;
				ioGain[I2s2R_MusR]= ioGain[I2s2L_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
//				DMAimage_display(250,214,(u8*)gImage_Bluto_icon_gre);//蓝牙
			}else{
//				if(BtLedFlash^=1)	DMAimage_display(250,214,(u8*)gImage_Bluto_icon_gre);//蓝牙
//					else			DMAimage_display(250,214,(u8*)gImage_Bluto_icon);//蓝牙
			}
#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
#endif
		//==================================================//系统工作模式：FM卡模式和AUX模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE || sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
					ioGain[Play_MusL]= -90;
					ioGain[Play_MusR]= ioGain[Play_MusL];
					ioGain[Adc2_MusL]= -4;
					ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
					ioGain[I2s2L_MusL]= -4;
					ioGain[I2s2R_MusR]= ioGain[I2s2L_MusL];
				}
			}
#ifdef	SPDIF_GPIO
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_SPDIF_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
			}
#endif
		}
	}
}

//**** pre init ***********************
void user_init_prv(void)
{
	#define POWER_EN	GPIO35
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);

	#define MUTE_EN	GPIO22
	gpio_config_new(MUTE_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(MUTE_EN, 1);
}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init(18);	// 設置FLASH 傳入版本參數
}

//********************************
void user_init_post(void)
{
	//=========================
    Hi2c_Init();


    //=============================
    extern void key_function(KEY_CONTEXT_t *pKeyArray);
    extern void user_key_init(void* pKeyFuncCbk);
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
//	if(Tick_2ms)	user_saradc_update_trig();	// Adc 讀取觸發
	SysMsCont++;
}
#endif


static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{

	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
		//----------------------------
		if(EF_Maim()) return;

		//-------------------
		user_WorkModefun();

		//----------------------
		user_key_scan();

		//================================================
		if(Mus_ValR != SysInf.MusVol32){
			Mus_ValR = SysInf.MusVol32;
			Music_VolSet(MusVol_TAB[SysInf.MusVol32]);
		}
		if(Mic_ValR != SysInf.MicVol32){
			Mic_ValR = SysInf.MicVol32;
			Mic_VolSet(MusVol_TAB[SysInf.MicVol32]);
		}

		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){

		}else if(Init_Cont == 13){

//			gpio_output(AMP_EN, 1);
		}else if(Init_Cont == 14){
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
		}else if(Init_Cont == 15){
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
			system_work_mode_set_button(SysInf.WorkMode);
		}

	}
}

//***************************************************
void Audio_Def()
{
	SysInf.MusicVol = -10;
	SysInf.MicVol = 0;

	ioGain[Tone_I2s2LR] = -20;
	ioGain[Tone_DacLR] = -20;
//	ioGain[Adc2_MusL] = 0;
//	ioGain[Adc3_MusR] = 0;

//	ioGain[Play_MusL] = -5;
//	ioGain[Play_MusR] = -5;

	ioGain[Out2L_RecL] = -10;
	ioGain[Out2R_RecR] = -10;

	ioGain[Out1L_DacL] = 0;
	ioGain[Out1R_DacR] = 0;
	ioGain[Out2L_I2s2L] = 0;
	ioGain[Out2R_I2s2R] = 0;

	ioGain[MusL_Out2L] = 0;
	ioGain[MusR_Out2R] = 0;

//	ioGain[Adc2_MusL] = 0;
//	ioGain[Adc3_MusR] = 0;
//	ioGain[Out2L_RecL] = 0;
//	ioGain[Out2R_RecR] = 0;



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
	if(val<=-60)	val = -90;
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
	if(val<=-60)	val = -90;
//	ioGain[Adc4_Mic] = val;
//	ioGain[Adc5_Mic] = val;
//	IOGainSet(Adc4_Mic, val);
//	IOGainSet(Adc5_Mic, val);
//	ioGain[I2s2L_Mic] = val;
//	ioGain[I2s2R_Mic] = val;
//	IOGainSet(I2s2L_Mic, val);
//	IOGainSet(I2s2R_Mic, val);
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
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
#if 1
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
//		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	// 輸入選擇 模式切換
			system_work_mode_change_button();
			break;
		case 1:	//V+
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE)	app_button_sw_action(BUTTON_BT_VOL_P);// BT
				else	if(SysInf.MusVol32 < 32) 	SysInf.MusVol32++;
			UpComputerRW = 0;	// 更新上位機
			break;
		case 2:	//V-
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE)	app_button_sw_action(BUTTON_BT_VOL_M);	// BT MODE
				else	if(SysInf.MusVol32) 		SysInf.MusVol32--;
			UpComputerRW = 0;	// 更新上位機
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
		switch (pKeyArray->index){
		case 0:	// 輸入選擇 模式切換
			if(pKeyArray->keepTime > 100){
				USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
				pKeyArray->keepTime = 0;
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
			}
			break;
		case 1:	//V+
			if(pKeyArray->keepTime > 80){
				pKeyArray->keepTime = 0;
				app_button_sw_action(BUTTON_BT_VOL_P);	// BT MODE
				UpComputerRW = 0;	// 更新上位機
			}
			break;
		case 2:	//V-
			if(pKeyArray->keepTime > 80){
				pKeyArray->keepTime = 0;
				app_button_sw_action(BUTTON_BT_VOL_M);	// BT MODE
				UpComputerRW = 0;	// 更新上位機
			}
			break;
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}
#endif

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
		GroupWrite(0, 2);		// 保存記憶

		dsp_shutdown();
		gpio_output(POWER_EN, 0);

		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		break;
	case APP_WAVE_FILE_ID_CONN:
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
	}
	IOGainSetDamp(MusL_Out1L, SysInf.MusicVol,10);
	IOGainSetDamp(MusR_Out1R, SysInf.MusicVol,10);
	IOGainSetDamp(MusL_Out2L, SysInf.MusicVol,10);
	IOGainSetDamp(MusR_Out2R, SysInf.MusicVol,10);
}
#endif


