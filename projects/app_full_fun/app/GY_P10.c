/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2023-04-20
* 描述：

**********************************************************************/
#include "USER_Config.h"

#ifdef GY_P10

#include "P10_DEF.H"

const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

const int8_t MicVol_Tab[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

//static uint16_t ms1000 = 0;

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
//extern uint8 appPlayerPlayMode;
//static uint8_t DuckDetF = 0;
//static uint8_t	BtLedFlash=0;
//static uint16_t FadeInOut=0;      //fade++
//static uint8_t PlayIng=1;         //fade++
static uint16_t WorkModeCont=0;
//uint8_t work_modeR=10;
//
//uint8_t LineInR=0;
uint8_t LineIn2R=0;


static uint16_t SysMsCont=0;



//uint8_t Mus_Val=16;
//uint8_t Mic_Val=16;
uint8_t Mus_ValR=255;
uint8_t Mic_ValR=255;
uint16_t WMicOffCont=0;
void Check_Charge_Vol();
//**** pre init ***********************
void user_init_prv(void)
{
	//=============================	PWM
//	pwm_peri_init();   //激活PWM外设
//  hal_pwm_wave_init(GPIO35,1,2600,2600,NULL);
//  hal_pwm_enable(GPIO35,1);

	//=============================	GPIO配置
	#define SGM_MUTE	GPIO11		//SGM监听耳机         高开低关
	gpio_config_new(SGM_MUTE, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(SGM_MUTE, 0);

	#define POWER_LOW	GPIO3      //低电量LED指示灯，高开低关
	gpio_config_new(POWER_LOW, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(POWER_LOW, 0);

	#define LED_PLAY	GPIO24      //播放LED指示灯     高开低关
	gpio_config_new(LED_PLAY, 	GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
	gpio_output(LED_PLAY, 0);

	#define POWER_EN	GPIO30      //电源按键使能          高开低关
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 0);

	#define OTG_EN	GPIO4      		//OTG关闭充电  	     高关低开
	gpio_config_new(OTG_EN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(OTG_EN, 0);

	#define WM_PO_EN	GPIO12      //WMIC        高开低关
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 0);

	#define ACM_PDN		GPIO35		//ACM功放                 高开低关
	gpio_config_new(ACM_PDN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(ACM_PDN, 0);

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;

	gpio_output(SGM_MUTE, 1);
	gpio_output(ACM_PDN, 1);
	gpio_output(WM_PO_EN, 1);

	USER_DBG_INFO("======== V33_01_CE user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);
}

//********************************
void user_init_post(void)
{
    //---------------------
	Hi2c_Init();
    //---------------------


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

//static uint8_t KeyId = BUTTON_BT_PLAY_PAUSE;

//void FadeInOut_Fun()
//{
//	FadeInOut--;
//	if(FadeInOut==199){
//		if(KeyId==BUTTON_BT_PLAY_PAUSE){    //播放|暂停 动作
//			if(app_is_bt_mode() || app_is_mp3_mode()){     //蓝牙|SD卡 模式
//				if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0 && player_get_play_status()==0){	//蓝牙和SD卡無播放
//					app_button_sw_action(BUTTON_BT_PLAY_PAUSE);//播放
//					FadeInOut = 51;	// Fade In
//				}else{	//播放中
//					app_button_sw_action(BUTTON_BT_PLAY_PAUSE);//暂停
//					FadeInOut = 198;	// Fade Out
//					PlayIng = 1;     //播放标志位
//				}
//			}else if(app_is_line_mode()){	// LINE MODE
//				PlayIng ^=1;
//				if(PlayIng)	FadeInOut = 51;	// 未播放 ->播放 淡入
//			}
//		}else{
//			PlayIng = 1;  //播放标志位
//		}
//	}else  if(FadeInOut==198){				// Fade Out
//      IOGainSetDamp(MusMstVol, -90,10);	// Fade Out
//		if(PlayIng==0)	FadeInOut = 1;      // 进LINE MODE，静音保持
//	}else  if(FadeInOut==150){				// Fade Out End
//		if(KeyId==BUTTON_MODE_CHG){         // 模式改变标志位
//			system_work_mode_change_button();
//			WorkModeCont = 99;
//		}
//		else if(KeyId==BUTTON_BT_PREV || KeyId==BUTTON_BT_NEXT){   //上一首还是下一首
//			if(app_is_bt_mode() || app_is_mp3_mode()){
//				app_button_sw_action(KeyId);
//				FadeInOut = 198;	// 執行淡出
//			}
//		}else if(KeyId==BUTTON_BT_PLAY_PAUSE){      //播放|暂停
//			if((app_is_mp3_mode()&&player_get_play_status())){      //SD卡模式且在播放
//				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
//			}
//		}
//	}else if(FadeInOut==50){	// Fade In
//		IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],5);
//	}else if(FadeInOut==0){     // 原音
//		ioGain[MusMstVol]=MusVol_TAB[(uint8_t)SysInf.MusicVol];
//	}
//}
//*****　插U盘会调用　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void user_udisk_ready_callback(void)
{
	system_work_mode_set_button(SYS_WM_UDISK_MODE);
	SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
}
//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================//系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc4_MusL]= -90;
				ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
			}
			if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
				gpio_output(LED_PLAY, 0);
			}else{	// 播放中
				gpio_output(LED_PLAY, 1);
			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc4_MusL]= -90;
				ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
			}
			if(player_get_play_status()==0){	// 無播放
				gpio_output(LED_PLAY, 0);
			}else{	// 播放中
				gpio_output(LED_PLAY, 1);
			}
		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc4_MusL]= -90;
				ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
			}
			if(player_get_play_status()==0){	// 無播放
				gpio_output(LED_PLAY, 0);
			}else{	// 播放中
				gpio_output(LED_PLAY, 1);
			}
		//==================================================//系统工作模式：AUX模式
//		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE || sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

//				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 0); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}else{
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 1); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}

				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc4_MusL]= -4;
				ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
			}
//			if(player_get_play_status()==0){	// 無播放
//				gpio_output(LED_PLAY, 0);
//			}else{	// 播放中
//				gpio_output(LED_PLAY, 1);
//			}
//			PlayIng = 1;     //播放标志位
		}
	}
}




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
		//======================ACM
		if(ACM_main()) return;
		//----------------------控制LCD显示和音源链路音量设置
		user_WorkModefun();

		//----------------------编码器旋钮
		Knob_function();

		//-------------------------WMIC
		BK9532_Main();

//		//----------------------淡入淡出
//		if(FadeInOut)	FadeInOut_Fun();



		//----------------------电池电量检测
//		Check_Charge_Vol();

		//----------------------按键扫描
		user_key_scan();

	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif

//		static uint8_t ms1000=0;
//		if(++ms1000 >= 100){	// 1秒計時
//			ms1000 = 0;
//			ACM_REPORT();
//		}

//		USER_DBG_INFO("====SD:%d:\n", SysMsContsd_is_attached());     //SD卡在位 test
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 9){
			BK9532_Init();       //WMIC初始化

		}else if(Init_Cont == 10){

			ACM8625_init();      //ACM初始化
//			ACMIIR_ReSend();
		}else if(Init_Cont == 11){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}

		}else if(Init_Cont==13){
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

		}else if(Init_Cont == 15){

			gpio_output(POWER_EN, 1);      //长按2秒后开机，开机使能滞后且能消除开机POPO声
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音

		}else if(Init_Cont == 20){
			EF_EQ_ReSend();
			USER_DBG_INFO("===BAT_VOL===:%d\n", SarAdcVal[3]);  //电池电量查看
		}else if(Init_Cont == 50){

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

//***************************************************检测电池电量
void Check_Charge_Vol()
{
	if(SarAdcVal[3]==0) 	SarAdcVal[3] = SarAdcValR[3];	// 防止误读0
	SarAdcValR[3] = SarAdcVal[3];
//	USER_DBG_INFO("====SarAdcValR: %d\n", SarAdcVal[3]);
	SarAdcVal[3] = SarADC_val_cali_get(SarAdcVal[3]);	// SarADC_val_cali_get 校正 ADC 偏差值
	if(SarAdcVal[3]<2750)
		gpio_output(POWER_LOW, 1);
	else
		gpio_output(POWER_LOW, 0);
	if(SarAdcVal[3]<2550)
	app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
}

//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
	extern void LCD_ShowVol(uint8_t vol);
	if(Page==MainPid){
		if(Id2==3){
//			LCD_ShowVol((uint8_t)SysInf.MusicVol+80);
		}
	}
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
//		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
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
	#define MaxV	4095	// 最大ADC 電壓 (n/4096)
	#define CenV	2100	// 旋鈕中心值電壓
	#define ScopeV	(MaxV/36)	//100

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
	for(uint8_t i=1; i<3; i++){    //2个旋钮。i=1时，SarAdcChVal[1] i=2时，SarAdcChVal[2]，1和2都是旋钮  。 i=0时，SarAdcChVal[0]是按键
		SarAdcVal[i] = SarADC_val_cali_get(SarAdcVal[i]);	// SarADC_val_cali_get 校正 ADC 偏差值
//		SarAdcVal[i] = (SarAdcVal[i]+SarAdcValR[i])>>1; //取平均值
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
			case 1:	// Echo Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = (float)EfVar/20;
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = (float)EfVar/10;
				break;
			case 2:	// Mic Vol
				SysInf.MicVol = VolVar;
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
//	return;
	//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
	//app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 1:	// 音效模式
			EF_Mode++;
			if(EF_Mode==4)
			{
				EF_Mode=0;
			}
			USER_DBG_INFO("====EF_Mode:%d\n", EF_Mode);
			break;
		case 2:	// 输入选择 模式切换
//				if(FadeInOut==0){	// MODE 切換中 按鍵失效
//					KeyId = BUTTON_MODE_CHG;
//					FadeInOut = 200;
//				}
			system_work_mode_change_button();
			break;
		case 3:	//播放&暂停
			if(app_is_bt_mode()||app_is_sdcard_mode()||app_is_udisk_mode())
			{
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			}
			break;
		case 4:	// 上一首
//			KeyId = BUTTON_BT_PREV;
			app_button_sw_action(BUTTON_BT_PREV);
//			FadeInOut = 200;
			break;
		case 5:	// 下一首
			app_button_sw_action(BUTTON_BT_NEXT);
//			KeyId = BUTTON_BT_NEXT;
//			FadeInOut = 200;
			break;
		}
		break;
	//====================================================================================================
	case KEY_D_CLICK:	USER_DBG_INFO("KEY_D_CLICK\n");	break;// 短按 2次
	case KEY_T_CLICK:	USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);break;	// 短按 3次
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:
		USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
		switch (pKeyArray->index){
		case 1:	// 消原音
			SysInf.VoiceCanEn ^=1;
			app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
			if(SysInf.VoiceCanEn)	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				else				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
			break;

		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
//		if(pKeyArray->index==0 && pKeyArray->keepTime > 100){
//			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
//			pKeyArray->keepTime = 0;
//			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
//		}
		if(pKeyArray->keepTime > 100){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;
			switch (pKeyArray->index){
			case 0:  //关机
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
				break;
			case 1:  //消原音
				SysInf.VoiceCanEn ^=1;
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
				if(SysInf.VoiceCanEn){
					SysInf.VoiceCanEn = 0;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					SysInf.VoiceCanEn = 1;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
			case 4:  //音量-
				if(SysInf.MusicVol) 	SysInf.MusicVol--;
				break;
			case 5:  //音量+
				if(SysInf.MusicVol < 32) 	SysInf.MusicVol++;
				break;
			}
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	USER_DBG_INFO("MusicVol %d\n", SysInf.MusicVol); break;	// 常按放開

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
	IOGainSetDamp(MusMstVol, -90,10);      //Music Off

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
//		app_handle_t sys_hdl = app_get_sys_handler();

//		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
//			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
//		}

		gpio_output(POWER_LOW, 0);	//低电量LED指示灯，高点亮
		gpio_output(LED_PLAY, 0);	//播放LED指示灯，高点亮
//		SysInf.WorkMode = sys_hdl->sys_work_mode;  //模式
		SysInf.VoiceCanEn = 0;	// 關閉消原音
//		SysInf.NC = MusVolVal;  //CJQ++
		GroupWrite(0, 2);		// 保存播放記憶點
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
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		gpio_output(SGM_MUTE, 0);	//SGM
		gpio_output(WM_PO_EN, 0);	///WMIC
		gpio_output(ACM_PDN, 0);	//AMP_CRTL
		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		app_powerdown();
		gpio_output(POWER_EN, 0);	///POWER_EN
		os_delay_us(3200);	//1ms
		//app_button_sw_action(BUTTON_BT_POWERDOWN);

		break;
	case APP_WAVE_FILE_ID_CONN:
#ifdef SDCARD_DETECT_IO
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
#endif
		break;
	case APP_WAVE_FILE_ID_BT_MODE:
		break;
	case APP_WAVE_FILE_ID_RESERVED0:	// SD Card
//		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
//			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
//		}
		break;
	case APP_WAVE_FILE_ID_LINEIN_MODE:
		break;

	}
	//=========================================================
//	SwChgIng = 0;
	if(id < INVALID_WAVE_EVENT){
		IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
	}
}
#endif


