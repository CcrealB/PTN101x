/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
* 2023 問題
* 1. GPIO23 ADC 讀取不穩定
* 2. GPIO32 有時會誤讀 0
*
*
*
**********************************************************************/
#include "USER_Config.h"

#if defined(PTH_X3_0602_V01)

#include "SHIDU-X3-1048-DSP-EQ-0729.H"

static uint16_t SysMsCont=0;

//**** pre init ***********************
void user_init_prv(void)
{

}

//**** normal init ********************
void user_init(void)
{
	//===========================================
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

//	LineInR = gpio_input(LINE_DETECT_IO);
//	app_handle_t app_h = app_get_sys_handler();
//	app_h->sys_work_mode = SYS_WM_NULL;
	USER_DBG_INFO("======== V01 user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

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
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
#ifdef	udrv_saradc
	if(Tick_2ms)	user_saradc_update_trig();	// Adc 讀取觸發
#endif
	SysMsCont++;
}
#endif

//********************************
void user_init_post(void)
{
#if (SYS_LOG_PORT_SEL==SYS_LOG_PORT_UART1)
    system_ctrl(SYS_CTRL_CMD_UART2_GPIO_MAPPING, 1);
    system_mem_clk_enable(SYS_MEM_CLK_UART1);
    system_peri_clk_enable(SYS_PERI_CLK_UART1);
    extern void uart1_initialise(u_int32 baud_rate);
    uart1_initialise(115200);	// def UART_BAUDRATE_115200
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_UART1);
    // system_peri_dsp_irq_enable(SYS_PERI_IRQ_UART1);
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_UART1);
#endif

	//===============================================
//	g_dsp_log_on_fg = 0;
//	g_mcu_log_on_fg = 0;

//    uint32_t sys_log_port_get(uint32_t flag)
///   void sys_log_port_set(uint32_t flag, int en)

#ifdef USR_UART1_INIT_EN
    system_mem_clk_enable(SYS_MEM_CLK_UART1);
    system_peri_clk_enable(SYS_PERI_CLK_UART1);
    extern void uart1_initialise(u_int32 baud_rate);
    uart1_initialise(115200);	// def UART_BAUDRATE_115200 移除函數內 GPIO 設置
    system_ctrl(SYS_CTRL_CMD_UART2_GPIO_MAPPING, 1);	//0:GPIO6~7, 1:GPIO16~17
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_UART1);
    // system_peri_dsp_irq_enable(SYS_PERI_IRQ_UART1);
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_UART1);
#endif

#ifdef USR_UART2_INIT_EN
    system_mem_clk_enable(SYS_MEM_CLK_UART2);
    system_peri_clk_enable(SYS_PERI_CLK_UART2);
    extern void uart2_initialise(u_int32 baud_rate);
    uart2_initialise(9600);	// def UART_BAUDRATE_115200
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_UART2);
    // system_peri_dsp_irq_enable(SYS_PERI_IRQ_UART1);
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_UART2);
#endif

}

static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	SysMsCont=0;
	//=============================================
	if(Init_Cont >=150){

		//----------------------
		if(EF_Maim())	return;

		//======================
#ifdef	udrv_saradc
		extern void Knob_function();
		Knob_function();
#endif


		if(SysMsCont>1) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){

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

		}
	}
}


//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{

}

//***************************************************
//void Mic_VolSet(float val)
//{
//	ioGain[MicMstVol] = val;
//	IOGainSet(MicMstVol, val);
//}
//***************************************************
void Out_VolSet(float val)
{
	ioGain[Out1L_DacL] = val;
	ioGain[Out1R_DacR] = val;
	ioGain[Out2L_I2s2L] = val;
	ioGain[Out2R_I2s2R] = val;
	IOGainSet(Out1L_DacL, val);
	IOGainSet(Out1R_DacR, val);
	IOGainSet(Out2L_I2s2L, val);
	IOGainSet(Out2R_I2s2R, val);
	IOGainSet(Max2L_I2s3L, val);
	IOGainSet(Max2R_I2s3R, val);
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

#ifdef udrv_saradc
//***************************************************
void Knob_function()
{
	#define MaxV	3200	// 最大ADC 電壓 (n/4096)
	#define CenV	2200	// 旋鈕中心值電壓
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
#ifdef PTN1012_DEMO_V2p2
//	SarAdcVal[3] = sar_adc_voltage_get(SARADC_CH_GPIO23);
	for(uint8_t i=1; i<5; i++){
#else
	for(uint8_t i=1; i<4; i++){
#endif
#if LED_RGB_NUM
		if(i==1) i=2;
#endif

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
	#if (LED_RGB_NUM == 0)
			case 1:	// Music Vol
				SysInf.MusicVol = VolVar;
				MenuF = 3;
				DisplayMenu();
				break;
	#endif
			case 2:	// Mic Vol
				SysInf.MicVol = VolVar;
				MenuF = 9;
				DisplayMenu();
				break;
			case 3:	// REV Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = UserEf.RevVol;
	#ifdef PTN1011_DEMO_V2p2
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = UserEf.EchoVol;
	#endif
				MenuF = 13;
				DisplayMenu();
				break;
	#ifdef PTN1012_DEMO_V2p2
			case 4:	// Echo Vol
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = UserEf.EchoVol;
				MenuF = 23;
				DisplayMenu();
				break;
	#endif
			}
			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f  %d\n",i, SarAdcVal[i], VolVar, KeyVar, EqVar, EfVar, SarAdcVal[i]);
			SarAdcValR[i]=SarAdcVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
}
#endif

#ifdef u_key
static uint8_t tone = 0;
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);

	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		UpComputerRW = 0;
		switch (pKeyArray->index){
			case 0:
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				break;
			case 1:
				if(++EF_Mode >= 5)	EF_Mode=0;
				MenuF = 1;
				DisplayMenu();
				break;
			case 2:
				system_work_mode_change_button();
				break;
			case 3:
				app_button_sw_action(BUTTON_BT_PREV);
				break;
			case 4:
				app_button_sw_action(BUTTON_BT_NEXT);
				break;
			case 5:
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				break;
		}
		break;

	//====================================================================================================
	case KEY_D_CLICK:
		USER_DBG_INFO("KEY_D_CLICK\n");	// 短按 2次
		switch (pKeyArray->index){
			case 1:	if(SysInf.KeyShift > -6) SysInf.KeyShift--;	break;
			case 2:	SysInf.KeyShift = 0;	break;
			case 3:	if(SysInf.KeyShift <  6) SysInf.KeyShift++;	break;
			case 4:	SysInf.KeyShift = -4;	break;
			case 5:	SysInf.KeyShift = +4;	break;
		}
		MenuF = 0;
		break;
	case KEY_T_CLICK:
		USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);	// 短按 3次
		switch (pKeyArray->index){
			case 1:	app_wave_file_play_stop();	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);	break;
			case 2:	app_wave_file_play_stop();	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);	break;
			case 3:	app_wave_file_play_stop();	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);	break;
			case 4:	app_wave_file_play_stop();	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);	break;
			case 5:
				app_wave_file_play_start(APP_WAVE_FILE_ID_UNMUTE_MIC);
				if(UserEf.Fshift == -6){
					UserEf.Fshift = 0;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					UserEf.Fshift = -6;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
		}
		break;
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:
		USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
		UpComputerRW = 0;
		switch (pKeyArray->index){
			case 1:	EQ_Mode = 0;	break;
			case 2:	EQ_Mode = 1;	break;
			case 3:	EQ_Mode = 2;	break;
			case 4:
				if(tone==0){
					tone=1;	UserEf.Pitch = +12;	// 男變女
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);
				}else if(tone==1){
					tone=2;	UserEf.Pitch = -12;	// 女變男
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);
				}else if(tone==2){
					tone=3;	UserEf.Pitch = +8;	// 娃娃音
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);
				}else if(tone==3){
					tone=4;	UserEf.Pitch = -8;	// 魔音
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);
				}else if(tone==4){
					tone=5;	UserEf.Pitch = +4;	// 萝莉音
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
				}else{
					tone=0;	UserEf.Pitch = 0;	// 原音
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);
				}
				break;
			case 5:	// 消原音
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
				if(SysInf.VoiceCanEn){
					SysInf.VoiceCanEn = 0;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					SysInf.VoiceCanEn = 1;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
		}
		USER_DBG_INFO("KEY_L_PRES EQ_Mode: %d\n",EQ_Mode);
		MenuF = 2;
		DisplayMenu();
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->keepTime > 300){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;

			if(pKeyArray->index==0){
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
			}
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}

}
#endif

#ifdef CONFIG_USER_GPIO_INT
//******************************************************************
void user_gpio_isr(uint32_t io0_31, uint32_t io32_39)
{
	USER_DBG_INFO("io0_31:%p, io32_39:%p\n", io0_31, io32_39);
#ifdef KT56
    extern void READ_BURST();	// KT56
    //if(io32_39 & 0x1)	READ_BURST();		// gpio14 MicA & MicB
    if(io0_31 & 0x8)	READ_BURST();		// gpio14 MicA & MicB
#endif
}
#endif

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

		Send_Id1 = CmdPid;
		Send_Id2 = 202;
		SendUpComp(SendBuff, 32);	// 請上位機離線
		break;

	case APP_WAVE_FILE_ID_DISCONN:

		break;
	}
}
//********************************
void PlayWaveStop(uint16_t id)
{
	USER_DBG_INFO("=== PlayWaveStop...id:%d\n",id);

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		GroupWrite(0, 2);		// 保存播放記憶點

//		app_powerdown();
    	app_button_sw_action(BUTTON_BT_POWERDOWN);
		break;
	case APP_WAVE_FILE_ID_CONN:
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
	case APP_WAVE_FILE_ID_BT_MODE:

		break;
	case APP_WAVE_FILE_ID_RESERVED0:	// SD Card
#ifdef SDCARD_DETECT_IO
		USER_DBG_INFO("=== SD Card Mode...%d\n",player_get_play_status());
		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
#endif
		break;
	case APP_WAVE_FILE_ID_LINEIN_MODE:
		break;

	case APP_WAVE_FILE_ID_LOW_BATTERY:
		break;
	}

#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}

#endif

