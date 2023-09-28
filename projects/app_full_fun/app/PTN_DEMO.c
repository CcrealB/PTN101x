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

#if defined(PTN1011_DEMO_V2p2) || defined(PTN1012_DEMO_V2p2)

#include "COK_S8_DEF.H"

const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

const int8_t MicVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

/*
uint8_t VolTableLookUp(uint8_t vol)
{
	for(int i=0; i<33; i++){
		if(vol >=MusVol_TAB[i] && vol<=MusVol_TAB[i+1]){

		}
	}
}
*/


#ifdef SDCARD_DETECT_IO
	extern uint8 appPlayerPlayMode;
#endif
//******************************************************
#if LED_RGB_NUM

#endif

uint16_t SysMsCont=0;


uint8_t work_modeR=10;
static uint16_t WorkModeCont=0;
static uint8_t	BtLedFlash=0;
uint8_t LineInR=0;
uint16_t RecTime = 0;
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
#ifdef RECED_EN
		fl2hex V1;
		//--------------------------------
		if(recorder_is_working()){
			Send_Id1 = MainPid;
			Send_Id2 = 204;
			Send_Group = 1;
			V1.f = (float)RecTime++;
			Send_Val1 = V1.h;
			SendUpComp(SendBuff, 32);
		}else{
			if(RecTime){
				RecTime=0;
				Send_Id1 = MainPid;
				Send_Id2 = 204;
				Send_Group = 0;
				uint16_t Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
//				OLED_num(40, 2, Rec_num, 2, 8);
				SysInf.RecPlayInx = (Rec_num-1);
				char Lab[64];
				rec_file_name_get_by_idx(SysInf.RecPlayInx, Lab);  //根据索引获取文件名
				memcpy(&SendBuff[12], Lab,16);
				SendUpComp(SendBuff, 32);
			}
		}
//		OLED_num(20, 2, recorder_is_working(), 2, 8);
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
#endif
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
				gpio_output(LED0, 0);
			}else{
				gpio_output(LED0, BtLedFlash^=1);
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

				ioGain[Play_MusL]= -5;
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
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
		}
	}
}

uint8_t MusVolVal;	//yuan++
//**** pre init ***********************
void user_init_prv(void)
{
//	#define LED0	GPIO18
//	gpio_config_new(LED0, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//	gpio_output(LED0, 1);
//	#define LED1	GPIO7
//	gpio_config_new(LED1, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//	gpio_output(LED1, 1);
}

//**** normal init ********************
void user_init(void)
{
	//===========================================
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

//	LineInR = gpio_input(LINE_DETECT_IO);
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;
	USER_DBG_INFO("======== V13 user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
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
	//===============================================
#ifdef USR_UART1_INIT_EN
    hal_uart1_init(GPIO16, GPIO17, 115200, 1, 1);
#endif

#ifdef USR_UART2_INIT_EN
    hal_uart2_init(9600, 1, 1);
#endif


#ifdef	_Hi2c
	Hi2c_Init();
#endif
#ifdef	OLED128x64
	OLED_Init();
#endif
#ifdef	u_key
	void key_function(KEY_CONTEXT_t *pKeyArray);
	user_key_init(key_function);
#endif
#ifdef	udrv_saradc
	u_SaradcKbon_Init();
#endif

#if LED_RGB_NUM
	app_i2s0_init(1);
#endif


    //==== gpio interrupt Init ===========================================
    // system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_GPIO);
    // system_peri_mcu_irq_enable(SYS_PERI_IRQ_GPIO);
#ifdef KT56
#if defined(PTN1011_DEMO_V2p2)
	#define KT56_INT_IO	GPIO13
	#define KT56_En_IO	GPIO37
#else
	#define KT56_INT_IO	GPIO3
	#define KT56_En_IO	GPIO2
#endif
	gpio_config_new(KT56_INT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56_INT
	// 0:low level;	1:high level;  2:rise edge;	3:fall down interrup
	gpio_int_enable(KT56_INT_IO, 3);
	//----------------------------
	gpio_config_new(KT56_En_IO, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56_EN
    gpio_output(KT56_En_IO, 1);
#endif

#ifdef ACM86xxId1
	#define ACM86_En_IO	GPIO12
	gpio_config_new(ACM86_En_IO, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(ACM86_En_IO, 1);
#else
	gpio_config_new(GPIO12, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);
#endif

#ifdef CONFIG_DBG_LOG_FLASH_OP
    extern void dbg_log_flash_load(void);
    dbg_log_flash_load();
#endif


	//=============================	GPIO配置
	#define POWER_EN	GPIO27     //电源使能          高开低关
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 0);

	#define WM_PO_EN	GPIO12	//48v麦克使能    高开低关
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 0);

	#define DcInDet		GPIO30      //充电检测
	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	#define TFT_LED_ON	GPIO28	//TFT_LED_ON  背光低开
	gpio_config_new(TFT_LED_ON, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(TFT_LED_ON, 0);
}

static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	SysMsCont=0;
	//=============================================
	if(Init_Cont >=150){
	#if LED_RGB_NUM
		static uint8_t tc=0;
		if(++Init_Cont == 200){
			Init_Cont = 150;
			RgbLedOut1();
			if(++tc >11) tc =0;
		}
	#endif

		//===================
	#ifdef ACM86xxId1
		if(ACM_main())	return;;
	#endif
		//----------------------
		if(EF_Maim())	return;

		//-----------------------------------------------------
//		if(SysInf.OtgInVol != usb_in_vol_idx_L_get()){
//			SysInf.OtgInVol = usb_in_vol_idx_L_get();
//			USER_DBG_INFO("====OTG_InVol:%d  %3.1f\n", SysInf.OtgInVol, usb_in_vol_dB_L_get());
//		}
//		if(SysInf.OtgOutVol != usb_out_vol_idx_L_get()){
//			SysInf.OtgOutVol = usb_out_vol_idx_L_get();
//			USER_DBG_INFO("====OTG_OutVol:%d  %3.1f\n", SysInf.OtgOutVol, usb_out_vol_dB_L_get());
//		}

//		static float InVol, OutVol;
//		//-----------------------------------------------------
//		if(InVol != usb_in_vol_dB_L_get()){
//			InVol = usb_in_vol_dB_L_get();
//			USER_DBG_INFO("====2 OTG_InVol:%3.1f\n", usb_in_vol_dB_L_get());
//		}
//		if(OutVol != usb_out_vol_dB_L_get()){
//			OutVol = usb_out_vol_dB_L_get();
//			USER_DBG_INFO("====2 OTG_OutVol:%3.1f\n", usb_out_vol_dB_L_get());
//		}

		//======================
		user_WorkModefun();
#ifdef	udrv_saradc
		extern void Knob_function();
		Knob_function();
#endif
		//------------------------------
		static uint16_t ms500 = 0;
		if(++ms500 == 50){
			ms500 = 0;
	#ifdef ACM86xxId1
			ACM_REPORT();
	#endif
		}

		//-----------------
#ifdef KT56
		KT56_main();
#endif
#ifdef u_key
		user_key_scan();
#endif
		if(SysMsCont>5) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 10){
#ifdef ACM86xxId1
			ACM8625_init();
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
			gpio_output(POWER_EN, 1);
			gpio_output(WM_PO_EN, 1);

			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
		//----------------------------
		}else if(Init_Cont == 100){
#ifdef KT56
			KT56_Init();
#endif
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
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;
			system_work_mode_set_button(app_h->sys_work_mode);
		}
	}
}


//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
#ifdef	OLED128x64
	const char MenuLab[28][10]={
			"Mode  Ef:","Mode  Eq:","MusicVol:","MusNGate:","MusicKEY:","Out Vol :","Dry Vol :",
			"Dry2 Vol:","Mic Vol :","MicNGate:","F  shift:","P  shift:","Rev Vol :","Rev2 Vol:",
			"PreDelay:","Rev Rep :","iDfus1  :","iDfus2  :","Excur   :","dDfus1  :","dDfus2  :",
			"HFreqAmp:","Echo Vol:","Echo2Vol:","EchoDeyT:","Echo Rep:","MicCompT:","MusCompT:"};

	uint8_t MenuF=0;
	static uint8_t MenuFR;

	if(Page==0x10 && Id2 <100){
		MenuF = Id2;
	}else if(Page==0x11 && Id2 <3){
		MenuF = Id2;
	}else{
		return;
	}

	if(MenuFR != MenuF){
		MenuFR = MenuF;
		if(MenuF<3){
			OLED_num(0, 4, MenuF, 2, 8);
			OLED_x8y16str(25,4,(char*)&MenuLab[(MenuF-1)]);
		}else{
			OLED_num(0, 6, MenuF, 2, 8);
			OLED_x8y16str(25,6,(char*)&MenuLab[(MenuF-1)]);
		}
		MenuF = Id2;
	}

	if(MenuF){
		char buffer[20] = " ";
		switch (MenuF){
//			(uint8_t column, uint8_t page, uint32_t num, uint8_t len, uint8_t size)
//			case 1:	OLED_num(0, 6, MenuF, 2, 8); OLED_num(105, 6, EF_Mode, 2, 8);	break;
			case 1:	sprintf(buffer, "%1d  ", EF_Mode);	break;
			case 2:	sprintf(buffer, "%1d  ", EQ_Mode);	break;
			case 3:	sprintf(buffer, "%3.0f",SysInf.MusicVol);	break;
			case 4: sprintf(buffer, "%3.0f",SysInf.MusNoiseGate);	break;
			case 5 :sprintf(buffer, "%2d",SysInf.KeyShift);		break;
			case 6: sprintf(buffer, "%3.0f",SysInf.OutVol);		break;
			case 7:	sprintf(buffer, "%1.1f",UserEf.drygain);	break;
		//	case 8:	sprintf(buffer, "%1.1f",UserEf.drygain2);	break;
			case 9:	sprintf(buffer, "%3.0f",SysInf.MicVol);		break;
			case 10:sprintf(buffer, "%3.0f",SysInf.MicNoiseGate);	break;
			case 11:sprintf(buffer, "%3d",(int)UserEf.Fshift);	break;
			case 12:sprintf(buffer, "%2d",(int)UserEf.Pitch);	break;
			case 13:sprintf(buffer, "%1.1f",UserEf.RevVol);		break;
		//	case 14:sprintf(buffer, "%1.1f",UserEf.RevVol2);	break;
			case 15:sprintf(buffer, "%ld",UserEf.lex_PreDelay);	break;
			case 16:sprintf(buffer, "%1.1f",UserEf.RevRep);		break;
			case 17:sprintf(buffer, "%1.2f",UserEf.lex_iDfus1);	break;
			case 18:sprintf(buffer, "%1.2f",UserEf.lex_iDfus2);	break;
			case 19:sprintf(buffer, "%2.0f",UserEf.lex_Excur);	break;
			case 20:sprintf(buffer, "%1.2f",UserEf.lex_dDfus1);	break;
			case 21:sprintf(buffer, "%1.2f",UserEf.lex_dDfus2);	break;
			case 22:sprintf(buffer, "%1.1f",UserEf.lex_HFreqDamp);break;
			case 23:sprintf(buffer, "%1.1f",UserEf.EchoVol);	break;
		//	case 24:sprintf(buffer, "%1.1f",UserEf.EchoVol2);	break;
			case 25:sprintf(buffer, "%03d",(int)UserEf.EchoDeyT);break;
			case 26:sprintf(buffer, "%1.1f",UserEf.EchoRep);	break;
			case 27:sprintf(buffer, "%3.0f",SysInf.MicCompT);	break;
			case 28:sprintf(buffer, "%3.0f",SysInf.MusCompT);	break;
		}
		if(MenuF<3){
			OLED_x8y16str(104,4,buffer);
		}else{
			OLED_x8y16str(104,6,buffer);
		}
//		USER_DBG_INFO("======== Disp:%s\n", buffer);
	}
#endif

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
	return;
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
				break;
	#endif
			case 2:	// Mic Vol
				SysInf.MicVol = VolVar;
				break;
			case 3:	// REV Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = UserEf.RevVol;
	#ifdef PTN1011_DEMO_V2p2
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = UserEf.EchoVol;
	#endif
				break;
	#ifdef PTN1012_DEMO_V2p2
			case 4:	// Echo Vol
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = UserEf.EchoVol;
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
	//***************************************************
	void key_function(KEY_CONTEXT_t *pKeyArray)
	{
		USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
		//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
		//app_handle_t sys_hdl = app_get_sys_handler();

	//	static uint8_t OnOff=0;
		switch (pKeyArray->event){
		case KEY_S_PRES: break;
		case KEY_S_REL:	break;
		//====================================================================================================
		case KEY_D_CLICK:	USER_DBG_INFO("KEY_D_CLICK\n");	break;// 短按 2次
		case KEY_T_CLICK:	USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);break;	// 短按 3次
		case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
		case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
		case KEY_L_PRES:
			USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
			switch (pKeyArray->index){
			case 1:
				break;
			}
			break;
		case KEY_L_PRESSING:	// 常按 循環

			if(pKeyArray->keepTime > 1000){
	//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
				if(pKeyArray->index==0){
					pKeyArray->keepTime = 0;
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C


				}
			}
			break;
		case KEY_L_REL:
			USER_DBG_INFO("KEY_L_REL\n");	// 常按放開
			break;
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

#ifdef ACM86xxId1
		gpio_output(ACM86_En_IO, 0);
#endif
#ifdef KT56
		gpio_output(KT56_En_IO, 0);
#endif

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

