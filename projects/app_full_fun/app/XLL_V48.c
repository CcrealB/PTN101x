/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
#include "USER_Config.h"
#ifdef XLL_V48
#include "PTN1012_DEMO_DEF.H"


//static uint8_t  BK_DeyOnCont=0;
static uint8_t	WmPoSw=0;
uint16_t WMicOffCont=0;

static uint16_t SysMsCont=0;

//**** pre init ***********************
void user_init_prv(void)
{
	#define WM_PO_EN	GPIO11
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 1);
}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH
}

static void key_function(KEY_CONTEXT_t *pKeyArray);
//********************************
void user_init_post(void)
{
	//=========================
    Hi2c_Init();
    BK9532_Init();

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


static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
		//----------------------------
		if(EF_Maim()) return;


		//===================
		BK9532_Main();

		//----------------------
		user_key_scan();

//		static uint8_t ms1000 = 0;
//		if(ms1000 >= 100){
//			ms1000 = 0;
//			Hi2c_Init();
//		}else{
//			ms1000++;
//		}
/*
		//==== BK 無線麥啟用  ================================
		if(BK_DeyOnCont){
			BK_DeyOnCont--;
			if(BK_DeyOnCont==139){
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_9);
			}else if(BK_DeyOnCont==28){

			}else if(BK_DeyOnCont==26){
				gpio_output(WM_PO_EN, WmPoSw);
			}else if(BK_DeyOnCont==6){
				BK9532_Init();
			}else if(BK_DeyOnCont==1){
				// UMUTE
			}
		}
*/
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

		//----------------------------
		}else if(Init_Cont == 101){
			EF_EQ_ReSend();
		//----------------------------
		}else if(Init_Cont == 102){
			g_dsp_log_on_fg = 0;
			g_mcu_log_on_fg = 0;
		}else if(Init_Cont == 103){
			Send_Id1 = CmdPid;
			Send_Id2 = 200;
			SendUpComp(SendBuff, 32);	// 請上位機連線
		}else if(Init_Cont == 104){
			g_dsp_log_on_fg = 1;
			g_mcu_log_on_fg = 1;
		}else if(Init_Cont == 148){
//			app_handle_t app_h = app_get_sys_handler();
//			app_h->sys_work_mode = SYS_WM_BT_MODE;
//			system_work_mode_set_button(SysInf.WorkMode);
		}
	}
}

//********************************************
void DisPlay_UpData(uint8_t Id2)
{

}
//***************************************************
void Music32_VolSet(uint8_t val)
{
	SysInf.MusicVol = val;
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
#if 1
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	// WMIC SW
/*
			if(WmPoSw){
				WmPoSw=0;
				BK_DeyOnCont = 0;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_5);
			}else{
				WmPoSw=1;
				BK_DeyOnCont = 140;
			}
*/
			USER_DBG_INFO("==== WmPoSw: %d\n",WmPoSw);
			break;
		case 1:	// MODE
			if(++EF_Mode > 4u)	EF_Mode = 0u;
			break;
		case 2:	// VOICE On/Off

			break;
		case 3:	// EF-

			break;
		case 4:	// EF +

			break;
		case 5:	// MIC VOL-

			break;
		case 6:	// MIC VOL+

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
//		if(pKeyArray->index==0 && pKeyArray->keepTime > 100){
//			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
//			pKeyArray->keepTime = 0;
//			PlayWaveStop(APP_WAVE_FILE_ID_POWEROFF);
//		}
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
	IOGainSetDamp(MusMstVol, -90,10);

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
//		gpio_output(POWER_EN, 0);

		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		break;
	}
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}
#endif


