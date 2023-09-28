/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 日期版本:	V16_2	2023-2-4

*
*更新SDK: usb_desc.c    LED_EF_FUN.c
*
**********************************************************************/
#include "USER_Config.h"
#ifdef HZ_P1

#include "HZ_P1_DEF.H"

#ifdef	RECED_EN
void user_RecWork(void);
void user_RecPlayWork(void);
void RecDrvSelect();
#endif

#ifdef	BattHR
void BattFun(void);
#endif


static uint8_t	WmPoSw=0;    //U段麦开关 标志位
uint8_t	LineInR=0;    //LineIN1开关 标志位   0：开   其他：关
uint8_t	LineIn2R=255;    //LineIN2开关 标志位   0：开   其他：关


static uint16_t SysMsCont=0;    //系统用时 计时器
static uint16_t PoKeyCont=1001;   //用于EC11 计时器
static uint16_t WorkModeCont=0;   //进入WorkMode()计时器


//伴奏音量大小 32段
const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

#ifdef	BattHR
//==== 关于 充电 =========================================
#define DisplayCont_def	3		// 充電燈顯示延時(S)
uint8_t DisplayCont = 1;
float Battery_Cont=0;
uint16_t Battery_ContR=1;
//static uint8_t	ChargFlash=0;
static uint8_t Batt_FULL=0;
//static uint16_t BattVn = 0x01;
static uint8_t LowPowerEn=0;
static uint8_t LowPowerCont=0;		// 進入低功耗計數
static uint16_t DelayOffCont=0;
static uint8_t OffPowerCont = 0;

//#define DcInDet			GPIO14      //充电检测
#define Channel_x       1           //AD采样通道号

#define	fullVn			8300	// 滿電壓 mv
#define	P4Voln			7800	// 亮4個燈電壓
#define	P3Voln			7200	// 亮3個燈電壓
#define	P2Voln			7000	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		6800	// 低電模式電壓
#define	LoPowerOFFv	 	6400	// 低電關機電壓

//#define	fullVn			12600	// 滿電壓 mv
//#define	P4Voln			11700	// 亮4個燈電壓
//#define	P3Voln			10800	// 亮3個燈電壓
//#define	P2Voln			9900	// 亮2個燈及解除低電模式電壓
//#define	LoPowerENv		9400	// 低電模式電壓
//#define	LoPowerOFFv	 	9000	// 低電關機電壓
//==== 关于 充电 =========================================
#endif



//**** pre init ***********************
void user_init_prv(void)
{
	PowerDownFg=0;	//打开 tick_task_1ms中断服务 和  user_loop_10ms计时任务
	//==============================================================
	#define POWER_EN	GPIO35      //开机/耳放/升压使能脚         高开低关
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);
}



//**** normal init ********************
void user_init(void)
{

	UserFlash_Init();	// 設置FLASH 傳入版本參數

//	appPlayerPlayMode = SysInf.SdCardPlayMode;
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式


	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;

	USER_DBG_INFO("======== V33_01_CE user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

#if LED_RGB_NUM
	app_i2s0_init(1);
	RgbLedAllColour(0);	// OFF RGB LED
#endif
}

//********************************
void user_init_post(void)
{
	//=============================	GPIO配置


	#define LED_AJ	GPIO22		//按键灯         上电拉高
	gpio_config_new(LED_AJ, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(LED_AJ, 0);

	#define WM_PO_EN	GPIO12      //WMIC     高开低关
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 1);

	#define ACM_PDN		GPIO5		//ACM功放          高开低关
	gpio_config_new(ACM_PDN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(ACM_PDN, 1);

	#define TP_PWR_EN		GPIO23		//同屏模式使能          高开低关
	gpio_config_new(TP_PWR_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(TP_PWR_EN, 0);

	#define	PHONE_DET		GPIO4     //耳放插入检测
	gpio_config_new(PHONE_DET, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);

	//==============PWM 初始化配置==============
#ifdef	USER_PWM_IO
	hal_pwm_wave_init(USER_PWM_IO, 1, 0, 260000, NULL);   //PWM波初始化
	hal_pwm_enable(USER_PWM_IO, 1);         //使能PWM
	//hal_pwm_duty_set(USER_PWM_IO, 0);     //占空比设置
#endif

	//==============IR红外 初始化配置==============
#ifdef IRDA_RX_IO
	gpio_config_new(IRDA_RX_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);     //红外遥控重新配置IO口。在MAIN进程 中 的InitPOST之前一直被拉低
#endif

	//=============UART1串口 初始化配置===============
#ifdef USR_UART1_INIT_EN
    hal_uart1_init(GPIO16, GPIO17, 115200, 1, 1);
#endif

}
//*****　插U盘会调用　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void user_udisk_ready_callback(void)
{
	system_work_mode_set_button(SYS_WM_UDISK_MODE);
	SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
}
//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{
#ifdef RECED_EN
		RecFun();
#endif
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
				ioGain[I2s3L_MusL]= -90;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			gpio_output(TP_PWR_EN, 0);
//			if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 蓝牙無播放
//				gpio_output(LED_PLAY, 0);
//			}else{	// 播放中
//				gpio_output(LED_PLAY, 1);
//			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= -90;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			gpio_output(TP_PWR_EN, 0);
//			if(player_get_play_status()==0){	// TF卡無播放
//				gpio_output(LED_PLAY, 0);
//			}else{	// 播放中
//				gpio_output(LED_PLAY, 1);
//			}
		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= -90;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			gpio_output(TP_PWR_EN, 0);
//			if(player_get_play_status()==0){	// U盘無播放
//				gpio_output(LED_PLAY, 0);
//			}else{	// 播放中
//				gpio_output(LED_PLAY, 1);
//			}
		//==================================================//系统工作模式：同屏模式
//		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE || sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

//				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 0); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}else{
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 1); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}

//				ioGain[Play_MusL]= -90;
//				ioGain[Play_MusR]= ioGain[Play_MusL];
//				ioGain[Adc4_MusL]= -4;
//				ioGain[Adc5_MusR]= ioGain[Adc4_MusL];
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= -5;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			gpio_output(TP_PWR_EN, 1);
		}
	}
}

//**********************************************
void user_loop(void)
{
//    mailbox_mcu_isr();
    static uint32_t t_mark = 0;
    if(sys_timeout(t_mark, 10)){	//10ms task
        t_mark = sys_time_get();
        extern void user_loop_10ms(void);
        user_loop_10ms();
    }
}

static uint16_t TbTime=0;
//*******************************************
#ifdef CONFIG_SYS_TICK_INT_1MS
void tick_task_1ms(void) //timer0 1ms isr
{
	if(PowerDownFg) return;
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
	if(Tick_2ms){
		user_saradc_update_trig();	// Adc 讀取觸發
	}// 2ms End
#ifdef EC11_Encoder
	else{
		if(EC11_SwR==EC11_Sw) {
			EC11_Sw = Encoder_EC11_Scan();	//EC11扫描动作
			EC11_FUN();                     //EC11执行动作
		}
	}
#endif
	SysMsCont++;
	TbTime++;
}
#endif


static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	if(PowerDownFg) return;
	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;

#ifdef	CONFIG_AUD_AMP_DET_FFT
		//============================
		static uint8_t led_30ms=0;
		if(++led_30ms >=3){         //30ms刷新RBG灯效
			led_30ms=0;
			void RGB_LED_EF_FUN(void);
			if(run_mode!=2)
				RGB_LED_EF_FUN();
		}
#endif

		//----------------------------
		if(EF_Maim()) return;
		//======================ACM
		if(ACM_main()) return;
		//----------------------控制LCD显示和音源链路音量设置
		user_WorkModefun();

		//----------------------编码器旋钮
//		Knob_function();

		//-------------------------WMIC
		BK9532_Main();

		//----------------------电池电量检测
#ifdef	BattHR
		BattFun();
#endif
		//----------------------按键扫描
		user_key_scan();

	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif

		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;
//			ACM_REPORT();
			USER_DBG_INFO("==== PHONE_DET:%d \n",gpio_input(PHONE_DET));
		}

//		USER_DBG_INFO("====SD:%d:\n", SysMsContsd_is_attached());     //SD卡在位 test
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 5){//WMIC初始化
			BK9532_Init();

		}else if(Init_Cont == 10){//ACM初始化
			ACM8625_init();
			USER_DBG_INFO("==== 10. ACM8625_init Ok... MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 11){//第一颗ACM
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
#ifdef ACM86xxId2
		}else if(Init_Cont == 12){//第二颗ACM
		    ACM862xWId = 1;
		    SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    if(ACM_main()){
		    	Init_Cont--;
		    }else{
//		    	gpio_output(Boot_EN, 1);     //功放升压
		    	ACM862xWId = 0;
		    	SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    	USER_DBG_INFO("==== 12. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
		    }
#endif
		}else if(Init_Cont==15){
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
			gpio_output(LED_AJ, 1);       //灯板亮
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音

		//--------------------------------------------------

		}else if(Init_Cont == 18){



		}else if(Init_Cont == 20){
			EF_EQ_ReSend();
			EF_ModeR = EF_Mode;
			EQ_ModeR = EQ_Mode;
		}else if(Init_Cont == 50){
//			gpio_output(POWER_EN, 1);      //升压/开机/耳机，三合一

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
		}else if(Init_Cont == 149){
#ifdef	CONFIG_AUD_AMP_DET_FFT
			extern void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
			aud_mag_det_enable(1);	//audio magnitude detect enable
#endif
		}
	}
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
//		app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		}
	}else{
		play_en =1;
	}
}

//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	//app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){

		case 0:	//播放&暂停
			if(app_is_bt_mode()||app_is_sdcard_mode()||app_is_udisk_mode())
			{
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			}
			break;

		case 1:	// PREV
			app_button_sw_action(BUTTON_BT_PREV);
			break;

		case 2:	// NEXT
			app_button_sw_action(BUTTON_BT_NEXT);
			break;

		case 3:	// VOL +
			if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
			break;

		case 4:	// VOL -
			if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
			break;

		case 5:	// SW MODE
			system_work_mode_change_button();
			break;

		case 6:	// VoiceCanEn
			app_wave_file_play_stop();
			SysInf.VoiceCanEn ^=1;
			if(SysInf.VoiceCanEn){
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);      //开
			}else{
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_CANCEL);      //关
			}
			break;

		case 7:	// WMIC ON/OFF
			WmPoSw^=1;
			if(WmPoSw){
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);   //MIC OFF
				gpio_output(WM_PO_EN, 0);
				ChipEn[0]=0;
				ChipEn[1]=0;
			}else{
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);   //MIC ON
				gpio_output(WM_PO_EN, 1);
				BK9532_Init();       //WMIC初始化
			}
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
			case 3:	// VOL +
				if(SysInf.MusicVol<32)	SysInf.MusicVol++;
				break;
			case 4:	// VOL -
				if(SysInf.MusicVol)	SysInf.MusicVol--;
				break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		USER_DBG_INFO("KEY_L_PRESSING\n");
		USER_DBG_INFO("keepTime:%d\n",pKeyArray->keepTime);
		if(pKeyArray->keepTime%100==0){
//			USER_DBG_INFO("2.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			switch (pKeyArray->index){
			case 3:	// VOL +
				if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
				break;
			case 4:	// VOL -
				if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
				break;
			}
		}else if(pKeyArray->keepTime > 1000){
//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			if(pKeyArray->index==0){
				pKeyArray->keepTime = 0;
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
			}
		}
//		if(pKeyArray->keepTime > 1000){
//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
//			if(pKeyArray->index==0){
//				pKeyArray->keepTime = 0;
//				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
//				USER_DBG_INFO("====PowerDownFg:%d!\n", PowerDownFg);
//			}
//		}

		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	USER_DBG_INFO("MusicVol %d\n", SysInf.MusicVol); break;	// 常按放開

	default:break;
	}
}


//********************************
void PlayWaveStart(uint16_t id)
{
	USER_DBG_INFO("==== wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);		//Music Off
	app_handle_t sys_hdl = app_get_sys_handler();
	if(WmPoSw){
		IOGainSetDamp(MicMstVol, -90,32);	//Mic Off
	}
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PoKeyCont=0;
		PowerDownFg=1;  //关闭 tick_task_1ms中断服务 和  user_loop_10ms计时任务
		SysInf.VoiceCanEn = 0;	// 關閉消原音

		//**************记忆功能(伴奏音量/麦克风音量/工作模式/EQ)******************
//		short temp;
//		float tem;
//		temp=SysInf.MicVol;
//		tem=SysInf.MusicVol;
//		SysInf.WorkMode = sys_hdl->sys_work_mode;  //记忆模式
//		SysInf.EqGruup=EQ_Mode; //记忆均衡
//		SysInf.MicVol=temp;    	//记忆麦克风音量
//		SysInf.MusicVol=tem;    //记忆音乐音量
//		GroupWrite(0, 2);		// 保存播放記憶點

		break;

	case APP_WAVE_FILE_ID_DISCONN:
	case APP_WAVE_FILE_ID_BT_MODE:
	case APP_WAVE_FILE_ID_CONN:
		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{

	switch(id){
#ifdef	RECED_EN
	case APP_WAVE_FILE_ID_RESERVED4:	// 開始錄音
		RecState = 3;
		break;
	case APP_WAVE_FILE_ID_RESERVED5:	// 停止錄音
		RecState = 2;
		break;
#endif
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		gpio_output(WM_PO_EN, 0);
		gpio_output(LED_AJ, 0);
		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		gpio_output(POWER_EN, 0);
		os_delay_us(3200);	//1ms
		break;

	case APP_WAVE_FILE_ID_CONN:

		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}

		break;
	case APP_WAVE_FILE_ID_BT_MODE:
		break;

	case APP_WAVE_FILE_ID_MP3_MODE:			// UDISK
	case APP_WAVE_FILE_ID_RESERVED0:			// SD
		if(player_get_play_status()==0){		// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
		break;
	case APP_WAVE_FILE_ID_LINEIN_MODE:
		break;
	}
	//=========================================================

	IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],10);	//yuan41
	if(WmPoSw){
		IOGainSetDamp(MicMstVol, 5,32);
	}
	USER_DBG_INFO("==== wave_stop, ID:%d  MusicVol:%d\n", id, MusVol_TAB[(uint8_t)SysInf.MusicVol]);
}

#ifdef	BattHR
//***************************************************
uint16_t BattVolConvert(uint16_t AdcVol)
{
	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻(BattHR), 對地電阻 (BattLR)
	uint16_t AdcVol1 = SarADC_val_cali_get(AdcVol);	// 校正轉換
	uint16_t BattVol = (float)saradc_cvt_to_voltage(AdcVol1)/BattRv;
//	USER_DBG_INFO("==== AdcVol %d  %d  %d\n",AdcVol, AdcVol1, BattVol);
	return  BattVol;
}

//*************************************************
void BattFun(void)
{
	//--------------------------------
	static uint16_t BattVolDetMs = 99;
//	static uint16_t LowBattCont = 0;
	if(++BattVolDetMs == 100){
		BattVolDetMs=0;

	//===================================================================================
	uint16_t BattVol = BattVolConvert(SarAdcVal[Channel_x]);
	static uint16_t saradc_tempe_dataR;
	static uint8_t Batt_FULLR;
	extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
	//--------------------------------
	fl2hex V1;
	if((abs(SarAdcValR[Channel_x]-SarAdcVal[Channel_x])>10) || (saradc_tempe_dataR != saradc_tempe_data  || Batt_FULLR != Batt_FULL)){
		SarAdcValR[Channel_x] =SarAdcVal[Channel_x];
		saradc_tempe_dataR = saradc_tempe_data;

		Batt_FULLR = Batt_FULL;
		Send_Id1 = MainPid;
		Send_Id2 = 205;
		Send_Group = 1;
		V1.f = (float)BattVol/1000;
		Send_Val1 = V1.h;
		V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
		Send_Val2 = V1.h;
		Send_Val42 = Batt_FULL;
		SendUpComp(SendBuff, 32);
	}
//	if(DisplayCont || !gpio_input(DcInDet)){
////		if(DisplayCont) USER_DBG_INFO("==== BATT ADC:%d  %d mv  DcIn:%d  Batt_FULL:%d\n",SarAdcVal[Channel_x],BattVol,gpio_input(DcInDet),Batt_FULL);
////		if(DisplayCont)	DisplayCont--;      //充电的时候   充电灯显示时间结束就关掉，操作机器时候就会亮充电灯
//		if(ChargFlash==0){
//		#ifdef ChargeFullDis
//			if(BattVol>=fullVn)	Batt_FULL = 1;	// 8.375V 滿電
//				else Batt_FULL =0;
//		#else
//			Batt_FULL = !gpio_input(ChargFull);
//		#endif
//			if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
//				BattVn=0x0F;
//			}else if(BattVol>=P3Voln){	// 7.5	11.25
//				BattVn=0x07;
//			}else if(BattVol>=P2Voln){	// 7	10.5
//				BattVn=0x03;
//	//		}else if(SarAdcVal[Channel_x]>=2740){
//	//			BattVn=0x01;
//			}else{
//				BattVn=1;
//			}
//			ChargFlash = 1;
//		}else if(ChargFlash && Batt_FULL==0 && !gpio_input(DcInDet)){
//			if(((BattVn>>1)&1)==0){
//				BattVn=0x03;
//			}else if(((BattVn>>2)&1)==0){
//				BattVn=0x07;
//			}else if(((BattVn>>3)&1)==0){
//				BattVn=0x0F;
//				ChargFlash = 0;
//			}else{
//				ChargFlash = 0;
//			}
//		}else{
//			ChargFlash = 0;
//		}
////		USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[Channel_x]:%d\n",BattVn,SarAdcVal[Channel_x]);
//		if(DisplayCont || Batt_FULL==0){	// 顯示 或 未滿電
////			gpio_output(LED0_Bat0, (BattVn&1));
////			gpio_output(LED1_Bat1, ((BattVn>>1)&1));
////			gpio_output(LED2_Bat2, ((BattVn>>2)&1));
////			gpio_output(LED3_Bat3, ((BattVn>>3)&1));
//		}else if(!gpio_input(DcInDet) && Batt_FULL){
////			gpio_output(LED0_Bat0, 1);
////			gpio_output(LED1_Bat1, 1);
////			gpio_output(LED2_Bat2, 1);
////			gpio_output(LED3_Bat3, 1);
//		}else{
////			gpio_output(LED0_Bat0, 0);
////			gpio_output(LED1_Bat1, 0);
////			gpio_output(LED2_Bat2, 0);
////			gpio_output(LED3_Bat3, 0);
//		}
//	}else{
////		static uint8_t LowPowerFlash=1;
//		if(BattVol<LoPowerENv){	// 6.5v	9.75
////			gpio_output(LED0_Bat0, LowPowerFlash^=1);
//		}else{
////			gpio_output(LED0_Bat0, 0);
////			gpio_output(LED1_Bat1, 0);
////			gpio_output(LED2_Bat2, 0);
////			gpio_output(LED3_Bat3, 0);
//		}
//	}

		//==== 判斷進入 LowPower Mode =======================
		if(LowPowerEn==0 && BattVol<LoPowerENv){		// 6.5v		9.75
			if(++LowPowerCont > 5){
				LowPowerEn=1;
				DelayOffCont=300;	// 立即播報一次低電提示音
				OffPowerCont = 0;
				USER_DBG_INFO("==== LowPower ModeT ADC:%d  %d\n",SarAdcVal[Channel_x],BattVol);
			}
		}else{
			LowPowerCont=0;
			if(LowPowerEn && SarAdcVal[Channel_x]>=P2Voln){	// 低電解除
				LowPowerEn=0;
				USER_DBG_INFO("==== UnLowPower ModeT ADC:%d  %d\n",SarAdcVal[Channel_x],BattVol);
			}
		}

		//===============================================================
		if(LowPowerEn){	// 低電
			if(++DelayOffCont > 300){	// 5分鐘播報一次低電提示音
				DelayOffCont=0;
				DisplayCont = DisplayCont_def;
				app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
			}
			if(BattVol<LoPowerOFFv){	// 6v
				USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[Channel_x],BattVol, OffPowerCont);
				if(++OffPowerCont==6){
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
				}else if(OffPowerCont>7){
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
				}
			}else{
				OffPowerCont = 0;
			}
		}else{
			DelayOffCont=0;
		}
	}
}
#endif

/*==============================================
UART1串口发送信息
使用方法：打开相关宏后        完善下列功能函数
参考：MJ_K04
===============================================*/
#ifdef UART1_TX_SW
void com1_cmd_send_proc(uint8_t *buf, uint8_t len);
uint8_t Uart2Tx_Buff[4]={0xFF,0xFF,0xFF,0xFF};     //接收BUF
uint8_t  TX2_Buf_B=0;                              //BUF索引
void Send_Inf(uint8_t comm)
{
	Uart2Tx_Buff[0] = 0XAA;
	Uart2Tx_Buff[1] = 0X55;
	Uart2Tx_Buff[2] = comm;
	Uart2Tx_Buff[3] = 0XEF;
	com1_cmd_send_proc(Uart2Tx_Buff, 4);
	USER_DBG_INFO("Uart2Tx_Buff: %02X  %02X  %02X  %02X \n", Uart2Tx_Buff[0],Uart2Tx_Buff[1], Uart2Tx_Buff[2], Uart2Tx_Buff[3]);
}
#endif
/*==============================================
UART1串口接收回调函数     MAIN()完成UART1串口接收相关配置     底层回调该函数
使用方法：打开相关宏后        完善下列功能函数        并在SYS_APP下的u_com.c文件下的    com1_cmd_recv_proc_direct()内声明该函数
参考：MJ_K04
===============================================*/
#ifdef USR_UART1_INIT_EN
uint8_t Uart2Rx_lf = 0;				// 接收 Buff 指標
uint8_t Uart2Rx_Buff[10];
void Uart1_CmdRx(uint8_t rdata){
	??????
}
#endif
/*==============================================
红外回调函数     MAIN()完成IR初始化动作  并且在   user_init_post中重新配置IRDA_RX_IO    底层回调该函数
使用方法：打开相关宏后         完善下列功能函数
参考：ZT_M184
===============================================*/
#ifdef IRDA_RX_IO
void user_ir_rx_callback(uint8_t *buff, int size){
	??????
}
#endif

/*==============================================
EC11编码功能函数     在tick_task_1ms完成扫描动作和功能执行
使用方法：打开相关宏后         完善下列功能函数
参考：ZT_M184
===============================================*/
#ifdef EC11_Encoder
void EC11_FUN(void){
	??????
}
#endif

/*==============================================
录音机制：
	user_RecWork():开始录音
	user_RecPlayWork():停止录音，保存好录音，并播放录音文件，新录入的文件最优先播放
使用方法：打开相关宏后              按键功能下 调用 user_RecWork()  user_RecPlayWork()
参考：ZT_M184
===============================================*/
#ifdef	RECED_EN
static uint16_t RecTime = 0;
static uint8_t RecDrv = 0;
static uint16_t Rec_num;
static uint8_t RecState = 0;
static uint16_t RecPlayInx = 0;
static uint16_t RecPlayInxR = 0;
static uint8_t Rec10MsCont = 0;
//******************************
void user_RecWork(void)
{
	RecState = 1;
}
//******************************
void user_RecPlayWork(void)
{
	RecState = 5;
}

//**** 錄音載體設置 ********************************
void RecDrvSelect()
{
	if(player_get_play_status()){	// 2:正在播放， 0:没有播放。	yuan++
		app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		RecState = 0;	// 取消任務
		return;	// 錄音載體 播放中, 停止播放並返回
	}
	if(sd_is_attached() || udisk_is_attached()){
		if(sd_is_attached())	RecDrv = 0;
			else				RecDrv = 1;
	}
}
//******************************
void RecFun()
{
	fl2hex V1;
	switch (RecState){
	case 1:	// 錄音請求任務
		RecDrvSelect();
		USER_DBG_INFO("====RecDrv: %d\n",RecDrv);
		if(recorder_is_working()){	// 錄音中
			recorder_start(RecDrv, 0);
			RecState =12;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);	// 停止錄音提示音
			Rec_index=0;
		}else{
			//recorder_start(RecDrv,1);
			RecState =13;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	// 開始錄音提示音
			Rec_index=1;
		}
		RecPlayInxR = 1;
		break;
	case 2:	// 停止錄音
		Send_Id1 = MainPid;
		Send_Id2 = 204;
		Send_Group = 0;
		Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
		RecPlayInx = Rec_num;   //9.4更改
		USER_DBG_INFO("====WrRecPlayInx: %d\n",RecPlayInx);
		char Lab[64];
		rec_file_name_get_by_idx(Rec_num-1, Lab);  //根据索引获取文件名
		memcpy(&SendBuff[12], Lab,16);
		SendUpComp(SendBuff, 32);
		if(RecPlayInxR==1){
			RecState = 0;
			USER_DBG_INFO("====00+++++++++++++++++++\n");
		}else{
			RecState = 5;
			USER_DBG_INFO("====11+++++++++++++++++++\n");
		}
		RecPlayInxR=0;
		break;
	case 3:	// 啟動錄音
		RecTime=0;
		recorder_start(RecDrv, 1);
		RecState = 4;

		break;
	case 4:	// 錄音中傳送錄音時間碼到上位機
		if(++Rec10MsCont < 100)	return;
		Rec10MsCont = 0;
		if(recorder_is_working()){
			Send_Id1 = MainPid;
			Send_Id2 = 204;
			Send_Group = 1;
			V1.f = (float)RecTime++;
			Send_Val1 = V1.h;
			SendUpComp(SendBuff, 32);
			//	USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		}else{
			RecState = 0;
		}
		break;
	case 5:	// 錄音播放請求
		if(recorder_is_working()){	// 錄音中
			recorder_start(RecDrv, 0);	// 停止錄音
			RecState = 12;
			RecPlayInxR=0;  //8.21cjq++
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
			USER_DBG_INFO("====44+++++++++++++++++++\n");
		}else{
			Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
			RecPlayInx = Rec_num;   //9.4更改
			USER_DBG_INFO("====222+++++++++++++++++++：%d \n",RecPlayInx);



			if(RecPlayInx){
				USER_DBG_INFO("====77+++++++++++++++++++\n");
				if(sd_is_attached() || udisk_is_attached()){	// RecPlay
					RecState = 16;
					USER_DBG_INFO("====33+++++++++++++++++++\n");
					if(sd_is_attached())	system_work_mode_set_button(SYS_WM_SDCARD_MODE);
						else				system_work_mode_set_button(SYS_WM_UDISK_MODE);
				}else{
					RecState = 0;
					RecPlayInx = 0;
				}
			}else{
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				RecState = 0;
			}
		}
		break;
	case 6:	// 播放最後錄音索引
		if(RecPlayInx){
			rec_file_play_by_idx(RecPlayInx-1);//根据索引播放录音文件    //9.4更改
			USER_DBG_INFO("====RecPlayInx: %d\n",RecPlayInx);
			RecPlayInx = 0;
		}
		RecState = 0;
		break;
	}
}
#endif

#endif

