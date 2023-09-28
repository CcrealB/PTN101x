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
#ifdef ONS_01

#include "PTN1012_DEMO_DEF.H"
//static uint16_t ms1000 = 0;

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
extern uint8 appPlayerPlayMode;
static uint8_t DuckDetF = 0;
static uint8_t	BtLedFlash=0;

static uint16_t WorkModeCont=0;


uint8_t LineInR=0;
uint8_t LineIn2R=0;

static uint16_t SysMsCont=0;

uint8_t work_modeR=10;
//==== JW-24017-1B 带点 共楊 =======================================================================
const uint8_t LCD_PIN[7]={GPIO26, GPIO24, GPIO15, GPIO14, GPIO27, GPIO28, GPIO7};	// P1~7 GPIO
const uint8_t S_Xx[4][7]={	{0x01,0x02,0x30,0x40,0x03,0x10,0x20},	//1~4位 的 a~g段
							{0x12,0x13,0x41,0x15,0x14,0x21,0x31},
							{0x43,0x24,0x34,0x50,0x52,0x32,0x42},
							{0x65,0x56,0x45,0x53,0x35,0x54,0x46}};
const uint8_t S_Nx[19]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7C,0x27,0x7F,0x67,0x77,0x7D,0x39,0x5E,0x79,0x71,0x3E,0x00,0x40};	// N0~9,A~F,U,NC,- 段码
const uint8_t S_Dotx[8]={0x05,0x25,0x51,0x04,0x23,0x62,0x26,0x64};	// TF, FM, USB, AUX, :, B1, B2,. 	// K1~K8段码

uint8_t LCD[7]={0x00,0x00,0x00,0x00,0x00,0x00,0x00};    //显示缓存
//************************************
void LCD_ShowNum(uint8_t Xx,uint8_t Nx)
{
	for(uint8_t i=0; i<7; i++){
		if((S_Nx[Nx]>>i)&1){
			LCD[(S_Xx[Xx][i]>>4)&0xF] |= (1<<(S_Xx[Xx][i]&0xF));
		}else{
			LCD[(S_Xx[Xx][i]>>4)&0xF] &= ~(1<<(S_Xx[Xx][i]&0xF));
		}
	}
}
//****************************************
void LCD_ShowVol(uint8_t vol)
{
	LCD_ShowNum(0,17);
	LCD_ShowNum(1,16);
	LCD_ShowNum(2,vol/10);
	LCD_ShowNum(3,vol%10);
}
enum SHOW_DOT
{
    K1_TF,
    K2_FM,
    K3_USB,
    K4_AUX,
    K5_DOT2,
    K6_B1,
	K7_B2,
	K8_DOT,
};
//****************************************
void LCD_ShowDot(uint8_t n, uint8_t OnOff)
{
	if(OnOff)	LCD[(S_Dotx[n]>>4)&0xF] |= (1<<(S_Dotx[n]&0xF));
		else	LCD[(S_Dotx[n]>>4)&0xF] &= ~(1<<(S_Dotx[n]&0xF));
}

//****************************************//LCD显示服务函数
void LCD_ShowMode(uint8_t mode)
{
	LCD_ShowDot(K6_B1, 0);
	LCD_ShowDot(K1_TF, 0);
	LCD_ShowDot(K3_USB, 0);
	LCD_ShowDot(K4_AUX, 0);
	LCD_ShowDot(K2_FM, 0);
	LCD_ShowDot(K7_B2, 0);
	switch (mode){
		case SYS_WM_BT_MODE:
			LCD_ShowDot(K6_B1, 1);
			LCD_ShowDot(K5_DOT2, 1);
			LCD_ShowDot(K8_DOT, 0);   //关闭小数点
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
		break;

		case SYS_WM_SDCARD_MODE:
			LCD_ShowDot(K1_TF, 1);
			LCD_ShowDot(K5_DOT2, 1);
			LCD_ShowDot(K8_DOT, 0);   //关闭小数点
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
		break;

		case SYS_WM_UDISK_MODE:
			LCD_ShowDot(K3_USB, 1);
			LCD_ShowDot(K5_DOT2, 1);
			LCD_ShowDot(K8_DOT, 0);   //关闭小数点
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
		break;

		case SYS_WM_LINEIN1_MODE:
			LCD_ShowDot(K4_AUX, 1);
			LCD_ShowDot(K5_DOT2, 1);
			LCD_ShowDot(K8_DOT, 0);   //关闭小数点
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
		break;

		case SYS_WM_LINEIN2_MODE:
			LCD_ShowDot(K2_FM, 1);
			LCD_ShowDot(K5_DOT2, 0);
			LCD_ShowDot(K8_DOT, 1);   //打开小数点
			Show_Freq();
			break;

		case SYS_WM_SPDIF_MODE:
			LCD_ShowDot(K7_B2, 1);
			LCD_ShowDot(K5_DOT2, 1);
			LCD_ShowDot(K8_DOT, 0);   //关闭小数点
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
		break;
	}
}
//*****************************
void LCD_scan()
{
	static uint8_t ScanIndex = 0;
	for(uint8_t i=0; i<7; i++){
		if(ScanIndex==i){
			gpio_config_new(LCD_PIN[i], GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
			gpio_output(LCD_PIN[i],1);
			gpio_config_capacity(LCD_PIN[i], GPIO_DRV_20mA);
		}else{
			if((LCD[ScanIndex]>>i)&1){
				gpio_config_new(LCD_PIN[i], GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
				gpio_output(LCD_PIN[i],0);
				gpio_config_capacity(LCD_PIN[i], GPIO_DRV_5mA);
			}else{
				gpio_config_new(LCD_PIN[i], GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
			}
		}
	}
	if(++ScanIndex == 7)	ScanIndex = 0;
}

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
				LCD_ShowMode(work_modeR);
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				LCD_ShowMode(SYS_WM_BT_MODE);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)	LCD_ShowMode(work_modeR);
					else			LCD_ShowMode(SYS_WM_NULL);
			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				LCD_ShowMode(work_modeR);
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
				LCD_ShowMode(work_modeR);
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
				LCD_ShowMode(work_modeR);
			//	if(ioGain[Adc2_MusL] < - 15){
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -4;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
		}
	}
}

//**** pre init ***********************
void user_init_prv(void)
{

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init(17);	// 設置FLASH 傳入版本參數

}

//********************************
void user_init_post(void)
{
	//=========================
    Hi2c_Init();

	LCD_ShowNum(0,8);
	LCD_ShowNum(1,8);
	LCD_ShowNum(2,8);
	LCD_ShowNum(3,8);

//	for(uint8_t i=0; i<8; i++) LCD_ShowDot(i, 1);

    //=============================
//    extern void key_function(KEY_CONTEXT_t *pKeyArray);
//    extern void user_key_init(void* pKeyFuncCbk);
//    user_key_init(key_function);
    //---------------------
//	u_SaradcKbon_Init();


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
	//----------------
	LCD_scan();
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

		//-------------------QN8035
		QN8035_Main();


	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 13){
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
			//--------------------------------------------------

		}else if(Init_Cont == 20){

			EF_EQ_ReSend();
			//=====================
			SysMsCont=0;
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音

			QN8035_Init();
			USER_DBG_INFO("====QN8035_Init time:%d ms\n", SysMsCont);
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
			app_h->sys_work_mode = SYS_WM_BT_MODE;           //一上电设置为蓝牙模式
			system_work_mode_set_button(app_h->sys_work_mode);
		}

	}
}

//***************************************************

//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
//	if()
	extern void LCD_ShowVol(uint8_t vol);



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
#if 1
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
void Knob_function()
{
	#define MaxV	3600	// 最大ADC 電壓 (n/4096)
	#define CenV	3240	// 旋鈕中心值電壓
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
	for(uint8_t i=0; i<16; i++){
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
			case 3:	// Music Vol
				SysInf.MusicVol = VolVar;
	//				MenuF = 3;
	//				DisplayMenu();
				break;
			case 0:	// Music Bass
				UserEq[MusicEq2].gain = EqVar;
				break;
			case 1:	// Music MID
				UserEq[MusicEq4].gain = EqVar;
				break;
			case 2:	// Music Tre
				UserEq[MusicEq8].gain = EqVar;
				break;
			//---------------------------------------------
			case 5:	// Mic Vol
				SysInf.MicVol = VolVar;
	//				MenuF = 3;
	//				DisplayMenu();
				break;
			case 7:	// Mic Bass
				UserEq[MicEq4_2].gain = EqVar;
				break;
			case 6:	// Mic MID
				UserEq[MicEq4_4].gain = EqVar;
				break;
			case 4:	// Mic Tre
				UserEq[MicEq4_8].gain = EqVar;
				break;
			//--------------------------------------
			case 11:	// ECHO Vol
				UserEf.EchoVol = (float)EfVar/10;
				break;
			case 8:	// ECHO2 Vol
				UserEf.EchoVol2 = (float)EfVar/10;
				break;
			case 9:	// ECHO Delay
				UserEf.EchoDeyT = EfVar*15;
				break;
			case 10:// ECHO Rep
				UserEf.EchoRep = (float)EfVar/30;
				break;
			//--------------------------------------
			case 13:	// REV Vol
				UserEf.RevVol = (float)EfVar/20;
				break;
			case 15:	// REV2 Vol
				UserEf.RevVol2 = (float)EfVar/20;
				break;
			case 14:	// REV Delay
				UserEf.lex_PreDelay = EfVar*5;
				break;
			case 12:	// REV Rep
				UserEf.RevRep = (float)EfVar/30;
				break;
			}
			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], VolVar, KeyVar, EqVar, EfVar);
			KnobValR[i]=KnobVal[i];
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
		case 0:	// 輸入選擇 模式切換
			system_work_mode_change_button();
			USER_DBG_INFO("==== 3.PlayType: %d\n", PlayType);
			break;
		case 1:	// PRE
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE || sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
				app_button_sw_action(BUTTON_BT_PREV);
			}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
				//if(Fre_data >8750) Fre_data-=10;
				Fre_data-=10;
				if(Fre_data==8740) Fre_data=10800;
				Show_Freq();
				QND_TuneToCH(Fre_data);
			}
			break;
		case 2:	app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	break;
		case 3:	// NEXT
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE || sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
				app_button_sw_action(BUTTON_BT_NEXT);
			}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
				//if(Fre_data < 10800) Fre_data+=10;
				Fre_data+=10;
				if(Fre_data==10810) Fre_data=8750;
				Show_Freq();
				QND_TuneToCH(Fre_data);
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
				LCD_ShowNum(0,18);        //搜台时0123----
				LCD_ShowNum(1,18);
				LCD_ShowNum(2,18);
				LCD_ShowNum(3,18);
				LCD_ShowDot(K8_DOT, 0);   //关闭小数点
				QN8035_RXSeekFreq(1);     //从下往上搜
			}
			break;
		case 3:
			if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE)
			{
				LCD_ShowNum(0,18);        //搜台时0123----
				LCD_ShowNum(1,18);
				LCD_ShowNum(2,18);
				LCD_ShowNum(3,18);
				LCD_ShowDot(K8_DOT, 0);   //关闭小数点
				QN8035_RXSeekFreqdown(0); //从上往下搜
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
#if 0
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
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}
#endif


