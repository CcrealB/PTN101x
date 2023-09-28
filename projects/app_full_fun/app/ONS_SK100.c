/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 	2022-8-4
* 描述：        蓝牙、TF卡、USB 三种模式
* 问题：         交流220V供电，TF模式卡死
**********************************************************************/
#include "USER_Config.h"

#ifdef ONS_SK100

#include "ONS_01_DEF.H"

static uint16_t WorkModeCont=0;     //计1秒 ， 放缓蓝牙闪烁速度
extern uint8 appPlayerPlayMode;     //用于TF卡切歌播放模式
static uint8_t DuckDetF = 0;
static uint8_t	BtLedFlash=0;  //Q++
static uint8_t	play_pause=0;  //Q++
static uint16_t SysMsCont=0;

uint8_t LineInR=0;
uint8_t LineIn2R=0;

//==== JW-24017-1B 带点 共楊 =======================================================================
const uint8_t LCD_PIN[7]={GPIO26, GPIO24, GPIO15, GPIO14, GPIO27, GPIO29, GPIO30};	// P1~7 GPIO
const uint8_t S_Xx[4][7]={	{0x01,0x02,0x30,0x40,0x03,0x10,0x20},	//1~4位數字指標  a~g
							{0x12,0x13,0x41,0x15,0x14,0x21,0x31},
							{0x43,0x24,0x34,0x50,0x52,0x32,0x42},
							{0x65,0x56,0x45,0x53,0x35,0x54,0x46}};
const uint8_t S_Nx[18]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7C,0x27,0x7F,0x67,0x77,0x7D,0x39,0x5E,0x79,0x71,0x3E,0x00};	// N0~9,A~F,U,NC 顯示位
const uint8_t S_Dotx[8]={0x05,0x25,0x51,0x04,0x23,0x62,0x26,0x64};		// PLAY, PAUSE, USB, SD, :, 蓝牙,SPDIF, . 	// K1~K8指標
uint8_t LCD[7]={0x00,0x00,0x00,0x00,0x00,0x00,0x00};
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

//****************************************
void LCD_ShowDot(uint8_t n, uint8_t OnOff)
{
	if(OnOff)	LCD[(S_Dotx[n]>>4)&0xF] |= (1<<(S_Dotx[n]&0xF));
		else	LCD[(S_Dotx[n]>>4)&0xF] &= ~(1<<(S_Dotx[n]&0xF));
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

enum SHOW_DOT
{
    K1_Play,
    K2_Pause,
    K3_USB,
    K4_TF,
    K5_DOT2,
    K6_B1,     //蓝牙
    K7_B2,
	K8_DOT,
};


void LCD_ShowMode(uint8_t mode)
{
	LCD_ShowDot(K1_Play, 0);
	LCD_ShowDot(K2_Pause, 0);
	LCD_ShowDot(K3_USB, 0);
	LCD_ShowDot(K4_TF, 0);
	LCD_ShowDot(K5_DOT2, 0);
	LCD_ShowDot(K6_B1, 0);
	LCD_ShowDot(K7_B2, 0);
	LCD_ShowDot(K8_DOT, 0);
	switch(mode)
	{
		case SYS_WM_BT_MODE:LCD_ShowDot(K6_B1, 1);break;
		case SYS_WM_SDCARD_MODE:LCD_ShowDot(K4_TF, 1);LCD_ShowDot(K1_Play, 1);LCD_ShowDot(K2_Pause, 0);break;
		case SYS_WM_UDISK_MODE:LCD_ShowDot(K3_USB, 1);break;
	}
}


//***************************************************** 模式显示和音频链路
void user_WorkModefun(void)
{

	app_handle_t sys_hdl = app_get_sys_handler();
	//=============================================
	if(++WorkModeCont>=100)   //一秒进
	{
		WorkModeCont = 0;
		//=============================================   //系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
	//			RgbLedOut(1);	// B
				LCD_ShowMode(work_modeR);    //Q++

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
	//Q++
	#if 0
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
	//			RgbLedOut(1);	// B
			}else{
	//			if(BtLedFlash^=1)	RgbLedOut(1);	// B
	//				else			RgbLedOut(8);
			}
	#endif
	//Q test
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
					LCD_ShowMode(SYS_WM_BT_MODE);
					LCD_ShowDot(K1_Play, 1);LCD_ShowDot(K2_Pause, 0);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
					if(BtLedFlash^=1)	LCD_ShowMode(work_modeR);
					else			    LCD_ShowMode(SYS_WM_NULL);
			}

		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
	//			RgbLedOut(2);	// W
				LCD_ShowMode(work_modeR);       //Q++

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			//-----------------------------------
	//		if(RgbFlash){	//2=閃-次		4=閃兩次
	//			RgbFlash--;
	//			if(RgbFlash%2==0)	RgbLedOut(2);	// W
	//				else			RgbLedOut(8);
	//			ms1000 = 50;
	//		}

		//=====================================================//系统工作模式：USB模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				LCD_ShowMode(work_modeR);

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}

		//==================================================//？？？
//		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
//			if(work_modeR != sys_hdl->sys_work_mode){
//				work_modeR = sys_hdl->sys_work_mode;
//
//				ioGain[Play_MusL]= -90;
//				ioGain[Play_MusR]= ioGain[Play_MusL];
//				ioGain[Adc2_MusL]= -4;
//				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
//			}
		}
	}
}

//******************************************************************************************
//**** pre init ***********************
void user_init_prv(void)
{



}

//**** normal init ********************
void user_init(void)
{
	//===========================================
	UserFlash_Init(22);	// 設置FLASH 傳入版本參數

//	MusVolVal = SysInf.PlayInf[15];
//	if(MusVolVal<3) MusVolVal=3;

	appPlayerPlayMode = SysInf.SdCardPlayMode;	//
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

//	LineInR = gpio_input(LINE_DETECT_IO);
//	app_handle_t app_h = app_get_sys_handler();
//	app_h->sys_work_mode = SYS_WM_NULL;

	USER_DBG_INFO("======== V20_3 user_init End MsCont:%d  WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

}

//static void key_function(KEY_CONTEXT_t *pKeyArray);
//********************************
void user_init_post(void)
{
    Hi2c_Init();

	LCD_ShowNum(0,8);
	LCD_ShowNum(1,8);
	LCD_ShowNum(2,8);
	LCD_ShowNum(3,8);

//	for(uint8_t i=0; i<8; i++) LCD_ShowDot(i, 1);

    //=============================
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
}
#endif


void Knob_function();
static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	//=============================================
	if(Init_Cont ==150){

		//----------------------
		if(EF_Maim()) return;

		//-------------------
		user_WorkModefun();

		//----------------------
		Knob_function();

		//----------------------
		user_key_scan();


	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif

	//================================================================
	}else{
		Init_Cont++;
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
			if(SysInf.WorkMode == SYS_WM_UDISK_MODE){
//			    udisk_mode_auto_sw_set(1);
			}else{
				system_work_mode_set_button(SysInf.WorkMode);
			}
		}
	}
}


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
//	ioGain[Adc4_Mic] = val;
	ioGain[Adc5_Mic] = val;
//	IOGainSet(Adc4_Mic, val);
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
	ioGain[Out1L_DacL] = val;
	ioGain[Out1R_DacR] = val;
	ioGain[Out2L_I2s2L] = val;
	ioGain[Out2R_I2s2R] = val;
	IOGainSet(Out1L_DacL, val);
	IOGainSet(Out1R_DacR, val);
	IOGainSet(Out2L_I2s2L, val);
	IOGainSet(Out2R_I2s2R, val);
#if defined(PTN1012_TEST)
	IOGainSet(Out2L_I2s3L, val);
	IOGainSet(Out2R_I2s3R, val);
#endif
}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_stop();
	app_wave_file_play_start(APP_WAVE_FILE_ID_REDIAL);
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
	app_wave_file_play_stop();
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

uint16_t SarAdcValW[SarAdc_NUM];
//***************************************************
void Knob_function()
{
	#define MaxV	3630	// 最大ADC 電壓 (n/4096)
	#define CenV	3350	// 旋鈕中心值電壓
	#define ScopeV	(MaxV/100)	//36

	float VolUp10 = (MaxV-CenV)/10;
	float EqUp12 =	(MaxV-CenV)/120;
	float KeyUp6 = (MaxV-CenV)/6;

	float VolDn60 = (CenV/60);
	float EqDn12 =	(CenV/120);
	float KeyDn6 =  (CenV/6);
	float EfUpDn20 = (MaxV/20);

	int VolVar;
	int KeyVar;
	int EqVar;
	float EfVar;

//	float EfVar;
//	float Rtemp;

	//=== 處理 ADC 滑桿,旋鈕 ===========
	for(uint8_t i=2; i<6; i++){
		SarAdcValW[i] = SarAdcVal[i];
		if(abs(SarAdcValR[i]-SarAdcValW[i])>=(ScopeV+20)){
			if(SarAdcValW[i]>MaxV){
				SarAdcValR[i] = MaxV;
			}else if(SarAdcValW[i]<ScopeV){
				SarAdcValR[i] = 0;
			}else{
				SarAdcValR[i] = SarAdcValW[i];
			}
			if(SarAdcValR[i] >= CenV){
				VolVar = (SarAdcValR[i]-CenV)/VolUp10;
				KeyVar = (SarAdcValR[i]-CenV)/KeyUp6;
				EqVar =  (SarAdcValR[i]-CenV)/EqUp12;
			}else{
				VolVar =(SarAdcValR[i]/VolDn60)-60;
				if(VolVar <-59) VolVar = -90;
				KeyVar =(SarAdcValR[i]/KeyDn6)-6;
				EqVar = (SarAdcValR[i]/EqDn12)-120;
			}
			EfVar =  ((SarAdcValR[i]-CenV)/EfUpDn20);
		//	Rtemp = (float)VolVar20/10;
			//====================================================
			switch (i){
			case 2:	//音乐音量
				SysInf.MusicVol = VolVar; 	LCD_ShowVol(VolVar+60);	break;
			case 3:	//低音调节
				UserEq[MusicEq2].gain = EqVar;	break;
			case 4:	//音乐混响
				UserEq[MusicEq8].gain = EqVar;	break;
			case 5: //混响音量
				UserEf.RevVol = (float)EfVar/20; break;
		//	case 13:	UserEf.EchoVol2 =(float)EfVar/10;break;
			}
			USER_DBG_INFO("====Knob:%d  Val:%d  Vol:%d  Key:%d  EQ%d  EfVar:%2.1f\n",i, SarAdcValR[i], VolVar, KeyVar, EqVar, EfVar);
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
#if 1
//	static uint8_t ll=0;
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		if(pKeyArray->index>=8)	app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	system_work_mode_change_button();			break;

		case 1:	//上一首
			if(hci_get_acl_link_count(sys_hdl->unit) || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			app_button_sw_action(BUTTON_BT_PREV);}
			break;

		case 2:	//播放&暂停
			if(hci_get_acl_link_count(sys_hdl->unit) || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				if(play_pause^=1){                  							//切换显示符号
					LCD_ShowDot(K1_Play, 0);LCD_ShowDot(K2_Pause, 1);
				}
				else{
					LCD_ShowDot(K1_Play, 1);LCD_ShowDot(K2_Pause, 0);
				}
			}
			break;
		case 3:	//下一首
			if(hci_get_acl_link_count(sys_hdl->unit) || sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			app_button_sw_action(BUTTON_BT_NEXT);}
			break;
		case 4:	// 循環
			if(SysInf.WorkMode==2){	//判断当前是否是 SD 模式
				//APP_PLAYER_MODE_PLAY_ALL_CYCLE = 0,
				//APP_PLAYER_MODE_PLAY_ONE = 2,      
				//APP_PLAYER_MODE_PLAY_RANDOM = 3,  
				if(appPlayerPlayMode==0){
					appPlayerPlayMode=2;
				//	RgbFlash=2;
					set_app_player_play_mode(APP_PLAYER_MODE_PLAY_ONE);    		//顺序播放
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);
				}else{
					appPlayerPlayMode=0;
				//	RgbFlash=4;
					set_app_player_play_mode(APP_PLAYER_MODE_PLAY_ALL_CYCLE);  //全部循环
					app_wave_file_play_start(APP_WAVE_FILE_ID_REDIAL);
				}
			}
			break;
		case 5:// 話筒優先

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

		case 14:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);	UserEf.Pitch = -8; break;	// 女變男
		case 15:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);	UserEf.Pitch = +8; break;	// 男變女
		case 16:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);	UserEf.Pitch = +12; break;	// 兒童
		case 17:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	UserEf.Pitch = -12; break;	// 老人
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
	case KEY_T_CLICK:   USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index); break;	// 短按 3次
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:    USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index); break;	// 常按進入
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

//********************************
void PlayWaveStart(uint16_t id)
{
	app_handle_t sys_hdl = app_get_sys_handler();

	USER_DBG_INFO("wave_start, ID:%d\n", id);

	IOGainSetDamp(MusMstVol, -90,10);            //Music off

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态，断开蓝牙
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PowerDownFg=1;
		SysInf.VoiceCanEn = 0;	// 开闭消原音
		SysInf.WorkMode = sys_hdl->sys_work_mode;

		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		os_delay_us(3200);	 //1ms
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		break;
	case APP_WAVE_FILE_ID_CONN:
//		app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		break;

	case APP_WAVE_FILE_ID_BT_MODE:

		break;
	case APP_WAVE_FILE_ID_RESERVED0:	// SD Card

		break;
	case APP_WAVE_FILE_ID_LINEIN_MODE:

		break;

	case APP_WAVE_FILE_ID_LOW_BATTERY:

		break;
	}
//	//yusn++
//#ifdef PromptToneMusicMute
//	ioGainR[MusL_Out2L] = -90;
//	ioGainR[MusR_Out2R] = -90;
//#endif
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}
#endif


