/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述： 直播和低音炮 左右通道对调。    底噪麦克风出来，硬件改善。
* 		更新SDK动usr_saradc_val_update_isr()4051旋钮和com1_cmd_recv_proc_direct()UART_RX;
* 		GPIO32 有時會誤讀 0
**********************************************************************/
#include "USER_Config.h"

#ifdef MJ_K04

#include "MJ_K04_DEF.H"

static uint16_t SysMsCont=0;

static uint16_t WorkModeCont=0;
//uint8_t work_modeR=10;
//static uint8_t	BtLedFlash=0;
//uint8_t LineInR=0;
uint8_t LineIn2R=0;

void Send_Inf(uint8_t comm);
static uint8_t DuckDetF = 0;               //闪避
//**********************************************
//关于 充电
#define DisplayCont_def	3		// 充電燈顯示延時(S)
uint8_t DisplayCont = 1;
float Battery_Cont=0;
uint16_t Battery_ContR=1;


static uint8_t LowPowerEn=0;
static uint8_t LowPowerCont=0;		// 進入低功耗計數
static uint16_t DelayOffCont=0;
static uint8_t OffPowerCont = 0;

//static int16_t BtVolR = 0;


#define	fullVn			25200	// 滿電壓 mv
#define	P5Voln			24500	// 亮5個燈電壓
#define	P4Voln			22500	// 亮4個燈電壓
#define	P3Voln			21500	// 亮3個燈電壓
#define	P2Voln			20000	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		19200	// 低電模式電壓
#define	LoPowerOFFv	 	18000	// 低電關機電壓
////***************************************************
//uint16_t BattVolConvert(uint16_t AdcVol)
//{
//	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻(BattHR), 對地電阻 (BattLR)
//	AdcVol = SarADC_val_cali_get(AdcVol);	// 校正轉換
//	float BattVol = (float)saradc_cvt_to_voltage(AdcVol)/BattRv;
////	USER_DBG_INFO("==== AdcVol %d  %d  %d\n",AdcVol, saradc_cvt_to_voltage(AdcVol), BattVol);
//	return  BattVol;
//}
//***************************************************
uint16_t BattVolConvert(uint16_t AdcVol)
{
	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻(BattHR), 對地電阻 (BattLR)
	uint16_t AdcVol1 = SarADC_val_cali_get(AdcVol);	// 校正轉換
	uint16_t BattVol = (float)saradc_cvt_to_voltage(AdcVol1)/BattRv;
	USER_DBG_INFO("==== AdcVol %d  %d  %d\n",AdcVol, AdcVol1, BattVol);
	return  BattVol;
}

//*************************************************
void BattFun(void)
{
	//--------------------------------
//	static uint16_t BattVolDetMs = 0;
	static uint16_t BattVolDetMs = 99;
//	static uint16_t LowBattCont = 0;
	if(++BattVolDetMs == 100){
		BattVolDetMs = 0;
		//****25.2v, 270k,21k(1.819v)/4096=0.444, 25200/1819=13.854 19.2v警告, 18v關機  ******检测电池电量
		uint16_t BattVol = BattVolConvert(SarAdcVal[4]);
		static uint16_t saradc_tempe_dataR;
		extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。

		fl2hex V1;
		if((abs(SarAdcValR[4]-SarAdcVal[4])>10) || (saradc_tempe_dataR != saradc_tempe_data)){
			SarAdcValR[4] =SarAdcVal[4];
			saradc_tempe_dataR = saradc_tempe_data;
			USER_DBG_INFO("====SarAdcVal: %d  %d\n", SarAdcVal[4], BattVol);

			Send_Id1 = MainPid;
			Send_Id2 = 205;
			Send_Group = 1;
			V1.f = (float)BattVol/1000;
			Send_Val1 = V1.h;
			V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
			Send_Val2 = V1.h;
			SendUpComp(SendBuff, 32);
		}

		//电量显示
		Send_Inf(0X01);  		 //第一格电量
		if(BattVol>=P2Voln){
			Send_Inf(0X02);  	 //第二格电量
		}
		if(BattVol>=P3Voln){
			Send_Inf(0X03);   	 //第三格电量
		}
		if(BattVol>=P4Voln){
			Send_Inf(0X04);   	 //第四格电量
		}
		if(BattVol>=fullVn && BattVol>=P5Voln){
			Send_Inf(0X05);   	 //第五格电量（满电）
		}


		//==== 判斷進入 LowPower Mode =======================
		if(LowPowerEn==0 && BattVol<LoPowerENv){
			USER_DBG_INFO("==== LowPowerCont: %d\n",LowPowerCont);
			if(++LowPowerCont > 5){
				LowPowerEn=1;
				DelayOffCont=300;
				OffPowerCont = 0;
				USER_DBG_INFO("==== LowPower ModeT ADC:%d  %d\n",SarAdcVal[4],BattVol);
			}
		}else{
			LowPowerCont=0;
			if(LowPowerEn && SarAdcVal[4]>=P2Voln){	// 低電解除
				LowPowerEn=0;
				USER_DBG_INFO("==== UnLowPower ModeT ADC:%d  %d\n",SarAdcVal[4],BattVol);
			}
		}

		//===============================================================
		if(LowPowerEn){	// 低電
			if(++DelayOffCont > 300){	// 300s，5分钟播报一次
				DelayOffCont=0;
				DisplayCont = DisplayCont_def;
				app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 報低電提示音
				Send_Inf(0X06);   //低电提示弹窗
			}
			if(BattVol<LoPowerOFFv){	// 低于9v 关机
				USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[4],BattVol, OffPowerCont);
				Send_Inf(0X0A);   //倒计时关机
				if(++OffPowerCont==6){        //6S低电播报
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);   //報低電提示音
				}else if(OffPowerCont>7){     //7S关机
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	  //關機
				}
			}else{
				OffPowerCont = 0;
			}
		}else{
			DelayOffCont=0;
		}
	}
}

////*****************************************************
//void user_WorkModeVolChg(uint8_t Mode)
//{
//	if(Mode==SYS_WM_LINEIN1_MODE){
//		BtVolR = ioGain[Play_MusL];
//		ioGain[Play_MusL]= -90;
//		ioGain[Play_MusR]= ioGain[Play_MusL];
////		ioGain[Adc4_MusL]= LineVolR;
////		ioGain[Adc5_MusR]= ioGain[Adc2_MusL];
//	}else{
//		ioGain[Play_MusL]= BtVolR;
//		ioGain[Play_MusR]= ioGain[Play_MusL];
////		LineVolR = ioGain[Adc2_MusL];
//		ioGain[Adc2_MusL]= -90;
//		ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
//	}
//}

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
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				user_WorkModeVolChg(SYS_WM_BT_MODE);
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
//				DMAimage_display(250,214,(u8*)gImage_Bluto_icon_gre);//蓝牙
			}else{
//				if(BtLedFlash^=1)	DMAimage_display(250,214,(u8*)gImage_Bluto_icon_gre);//蓝牙
//					else			DMAimage_display(250,214,(u8*)gImage_Bluto_icon);//蓝牙
			}
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				user_WorkModeVolChg(SYS_WM_UDISK_MODE);
			}
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
				if(SysInf.WorkMode != sys_hdl->sys_work_mode){
					SysInf.WorkMode = sys_hdl->sys_work_mode;
//					user_WorkModeVolChg(SYS_WM_LINEIN1_MODE);
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






//**** pre init ***********************
void user_init_prv(void)
{


}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數

//	appPlayerPlayMode = SysInf.SdCardPlayMode;
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;

	USER_DBG_INFO("======== TL820_V01 user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
	LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);
}

//********************************
void user_init_post(void)
{
	//==== GPIO SET =======================
	#define	AMP_MUTE		GPIO2      //功放mute  1mute 0开
    gpio_config_new(AMP_MUTE, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(AMP_MUTE, 1);

	#define	POWER_EN		GPIO3      //12V电源控制      开机2S后输出高电平
    gpio_config_new(POWER_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(POWER_EN, 0);

	#define	PHONE_DET		GPIO28     //耳放插入检测
    gpio_config_new(PHONE_DET, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	#define	PHONE_EN		GPIO35     //耳放使能
    gpio_config_new(PHONE_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(PHONE_EN, 0);

	#define	MIC_DET		    GPIO32     //话筒插入检测
	gpio_config_new(MIC_DET, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);

	#define	TEMP_DET		GPIO13     //48脚温度检测
	gpio_config_new(TEMP_DET, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	#define	TEMP_ACT		GPIO15     //48脚检测到低电平，输出高电平1分钟
    gpio_config_new(TEMP_ACT, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(TEMP_ACT, 0);

#ifdef USR_UART1_INIT_EN
    hal_uart1_init(GPIO16, GPIO17, 115200, 1, 1);
#endif

#ifdef USR_UART2_INIT_EN
    hal_uart2_init(115200, 1, 1);
#endif

	if(gpio_input(PHONE_DET))
			gpio_output(PHONE_EN, 1);
}

static uint8_t sw_flag2 = 0;
static uint16_t Count1min = 0;
void temp_det_act()
{

	if(gpio_input(TEMP_DET)==0){
		if(sw_flag2==0){
			gpio_output(TEMP_ACT, 1);
			USER_DBG_INFO("TEMP_ON!!!\n");
			sw_flag2=1;
		}
		Count1min++;
	}else if(gpio_input(TEMP_DET)){
		Count1min=0;
		sw_flag2=0;
	}
	if(Count1min>=6000){
		if(sw_flag2){
			gpio_output(TEMP_ACT, 0);
			sw_flag2=0;
		}
		Count1min=0;
		USER_DBG_INFO("TEMP_Off!!!\n");
	}
}

static uint8_t sw_flag = 0;
void phone_det_en()
{
	if(gpio_input(PHONE_DET)){
		if(sw_flag==0){
			gpio_output(PHONE_EN, 1);
			USER_DBG_INFO("PHONE_ON\n");
			sw_flag=1;
		}
	}else{
		if(sw_flag){
			gpio_output(PHONE_EN, 0);
			USER_DBG_INFO("PHONE_OFF\n");
			sw_flag=0;
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
void Knob_function();
static uint8_t Tick_2ms =0;
//*******************************************
void tick_task_1ms(void) //timer0 1ms isr
{
	if(PowerDownFg) return;
	//=============================================
	Tick_2ms++;
	if(Tick_2ms==1){
		user_saradc_update_trig();	// Adc 讀取觸發
	}else if(Tick_2ms>=2){
		Tick_2ms =0;
	}
	SysMsCont++;
#ifdef	CONFIG_AUD_AMP_DET_FFT
	extern u32	Tempo_ms_cont;
	Tempo_ms_cont++;
#endif
}

//**********************************************
void user_loop_10ms(void)
{
	if(PowerDownFg) return;
	static uint16_t Init_Cont = 0;

	//======================================
	if(Init_Cont == 150){
		SysMsCont=0;
		//---------------------------------
		if(EF_Maim())	return;
		//=====================
//		BattFun();

		//-------------------
		user_WorkModefun();
		//----------------------编码器旋钮
		Knob_function();
		//-------------------
		temp_det_act();
		phone_det_en();



		//话筒检测
//		if(gpio_input(MIC_DET)==0){
//			ioGain[Adc4_Mic] = -10;
//		}else{
//			ioGain[Adc4_Mic] = -90;
//		}

		//USB声卡检测
		static uint8_t timecountt =0;
		static uint8_t usb_Sw = 0;
		if(usbd_active_get())    //插入OTG
		{
			timecountt++;
		}
		if(timecountt==50)
		{
			timecountt=0;
			if(usb_Sw==0){
				Send_Inf(0x70);
				usb_Sw=1;
			}
		}
		if(usbd_active_get()==0)                     //退出OTG
		{
			if(usb_Sw==1){
				Send_Inf(0x71);
				usb_Sw=0;
			}
		}

		//==== 訊息報告 ====
		static uint16_t ms5000=0;
		if(++ms5000 >= 500){	// 5秒計時  功放报告
			ms5000 = 0;
//			USER_DBG_INFO("==== 3:%d\n",SarAdcVal[3]);
//			USER_DBG_INFO("==== 电池检测:%d\n",SarAdcVal[4]);
//			USER_DBG_INFO("==== OTG:%d\n",usbd_active_get());
//			USER_DBG_INFO("==== 5:%d\n",SarAdcVal[5]);
			USER_DBG_INFO("==== 话筒检测:%d\n",gpio_input(MIC_DET));
			USER_DBG_INFO("==== 温度检测:%d\n",gpio_input(TEMP_DET));
		}


		if(SysMsCont>5) USER_DBG_INFO("====SysMsCont:%d\n", SysMsCont);
	}else{
		Init_Cont++;
		//======================================================================
		if(Init_Cont == 1){

		}else if(Init_Cont == 20){

			gpio_output(AMP_MUTE, 0);
//			gpio_output(PHONE_EN, 1);
			SysInf.MicVol=0;
		}else if(Init_Cont == 21){
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
		}else if(Init_Cont == 101){
			EF_EQ_ReSend();
			EQ_ModeR = EQ_Mode;
			EF_Mode=1;
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
//			if(SysInf.WorkMode == SYS_WM_UDISK_MODE){
//			    udisk_mode_auto_sw_set(1);
//			}else{
//				system_work_mode_set_button(SysInf.WorkMode);
			system_work_mode_set_button(SYS_WM_SPDIF_MODE);
				SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
//			}
			gpio_output(POWER_EN, 1);  //开机两秒后输出高电平
		}
	}
}

//***************************************************
void Knob_function()
{
	#define MaxV	4076	// 最大ADC 電壓 (n/4096)
	#define CenV	2000	// 旋鈕中心值電壓
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
	int EqVar2;
	float EfVar;
	//=== 處理 ADC 滑桿,旋鈕 ===========
	for(uint8_t i=0; i<8; i++){
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
			EqVar2 = (KnobValR[i]/(4095/120))-120;
			if(EqVar>0) EqVar=0;

			if(VolVar>0) VolVar=0;
			//====================================================
			switch (i){

			case 0:	// 音乐低音
				UserEq[MusicEq1].gain = EqVar2;
				break;
			case 1:	// 混响
				if(EF_Mode!=0){      //原声模式下  无动作
					UserEf.RevVol = (float)EfVar/20;
					UserEf.RevVol2 = (float)EfVar/20;
					UserEf.EchoVol = (float)EfVar/10;
					UserEf.EchoVol2 = (float)EfVar/10;
				}
				break;
			case 2:	// 音乐音量
				SysInf.MusicVol = VolVar;
				break;
			case 3:	// 话筒高音
				UserEq[MicEq3_2].gain = EqVar2;
				break;
			//---------------------------------------------
			case 4:	// 音乐高音
				UserEq[MusicEq2].gain = EqVar2;
				break;
			case 5:	// 麦克风音量
//				ioGain[Adc4_Mic] = (((VolVar+1)/1.18)<-60)?-90:((VolVar+1)/1.18);
				ioGain[Adc5_Mic] = (((VolVar+1)/1.18)<-60)?-90:((VolVar+1)/1.18);
				break;
			case 7:	// 话筒低音
				UserEq[MicEq3_1].gain = EqVar2;
				break;
			case 6:	// 直播音乐音量
				ioGain[Max2L_I2s3L] = VolVar;
				break;
			}
			USER_DBG_INFO("====KnobValR: %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], VolVar, KeyVar, EqVar, EfVar);
			KnobValR[i]=KnobVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}

	//=== 處理 ADC 滑桿,旋鈕 ===========
		for(uint8_t i=1; i<4; i++){    //2个旋钮。i=1时，SarAdcChVal[1] i=2时，SarAdcChVal[2]，1和2都是旋钮  。 i=0时，SarAdcChVal[0]是按键
//			SarAdcVal[i] = SarADC_val_cali_get(SarAdcVal[i]);	// SarADC_val_cali_get 校正 ADC 偏差值
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

				if(VolVar>0) VolVar=0;
				//====================================================
				switch (i){
				case 1:	// 低音炮音量
					ioGain[Max2R_I2s3R] = VolVar;
					break;
				case 2:	// 直播话筒音量
					ioGain[Adc4_I2s3L] = VolVar;
					break;
				case 3:	// 乐器音量
//					ioGain[Adc5_Mic] = (((VolVar+1)/1.18)<-60)?-90:((VolVar+1)/1.18);
					ioGain[Adc4_Mic] = (((VolVar+1)/1.18)<-60)?-90:((VolVar+1)/1.18);
					break;
				}
				USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f\n",i, SarAdcVal[i], VolVar, KeyVar, EqVar, EfVar);
				SarAdcValR[i]=SarAdcVal[i];
				UpComputerRW = 0;	// 設定上位機同步更新
			}
		}
}
uint8_t Uart2Rx_lf = 0;				// 接收 Buff 指標
uint8_t Uart2Rx_Buff[10];
static uint8_t tone = 0;
static uint8_t mute_sw = 0;
static uint8_t charg_sta = 0;
static uint8_t Neulu_Sw= 0;
static uint8_t Jili_Sw = 0;
//***************************************************
void Uart1_CmdRx(uint8_t rdata)
{

	//Rx_Buff5[5]={0xD1,id,val,0x2F,ChkSum};
	//===========================================
	Uart2Rx_Buff[Uart2Rx_lf++] = 	rdata;
	USER_DBG_INFO("====UART_RX_ING====%d  %02X\n",Uart2Rx_lf,rdata);
	if(Uart2Rx_lf==1 && Uart2Rx_Buff[0] != 0xAA){
		Uart2Rx_lf = 0;
		return;
	}
	if(Uart2Rx_lf==2 && Uart2Rx_Buff[1] != 0x55){
		Uart2Rx_lf = 0;
		return;
	}
	if(Uart2Rx_lf==4 && Uart2Rx_Buff[3] != 0xEF){
		Uart2Rx_lf = 0;
		return;
	}
	//===========================================
	if(Uart2Rx_lf <4) return;
	USER_DBG_INFO("Uart2Rx_Buff: %02X  %02X\n", Uart2Rx_Buff[1], Uart2Rx_Buff[2]);
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (Uart2Rx_Buff[2]){

//--------------蓝牙-------------
		case 0X00:	// 蓝牙暂停/播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			break;
		case 0X01:	// 下一曲
			app_button_sw_action(BUTTON_BT_NEXT);
			break;
		case 0X02:	// 上一曲
			app_button_sw_action(BUTTON_BT_PREV);
			break;
		case 0X03:	// 打开蓝牙
			system_work_mode_set_button(SYS_WM_BT_MODE);
			break;
		case 0X04:	// 退出蓝牙
			system_work_mode_set_button(SYS_WM_SPDIF_MODE);
			break;
		case 0X4D:	// 断开蓝牙
			if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			}
			break;
//--------------声音-------------
		case 0X0B:	// 静音
			ioGain[Out1L_DacL] = -90;
			ioGain[Out1R_DacR] = -90;
			ioGain[Out2L_I2s2L] = -90;
			ioGain[Out2R_I2s2R] = -90;
			break;
		case 0X0C:	// 放音
			ioGain[Out1L_DacL] = 0;
			ioGain[Out1R_DacR] = 0;
			ioGain[Out2L_I2s2L] = 0;
			ioGain[Out2R_I2s2R] = 0;

			break;
//--------------话筒-------------
		case 0X09:	// 话筒优先开
			app_wave_file_play_stop();
			if(DuckDetF==0){
				DuckDetF=1;
				SysInf.DuckDet = 0x18;
//				app_wave_file_play_start(APP_WAVE_FILE_ID_UNMUTE_MIC); //闪避开
			}
			break;
		case 0X0A:	// 话筒优先关
			app_wave_file_play_stop();
			if(DuckDetF){
				DuckDetF=0;
				SysInf.DuckDet = 0x00;
//				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_CANCEL); //闪避关
			}
			break;
//--------------面板按键-------------
		case 0X10:	// 室内/室外
			if(++EQ_Mode >= 2)	EQ_Mode=0;
			break;
		case 0X11:	// 鼓掌
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);
			break;
		case 0X12:	// 欢呼
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
			break;
		case 0X13:	// 鄙视
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
			break;
		case 0X14:	// 尴尬
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);
			break;
		case 0X15:	// 笑
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);
			break;
		case 0X16:	// 么么哒
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_9);
			break;
		case 0X17:	// 开场
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);
			break;
		case 0X18:	// 贼拉拉
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_5);
			break;
		case 0X1B:	// 男变女/女变男/原音
			app_wave_file_play_stop();
			if(tone==0){
				tone=1;	UserEf.Pitch = +4;	// 男變女    女神模式
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);
			}else if(tone==1){
				tone=2;	UserEf.Pitch = -4;	// 女變男    男神模式
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);
			}else if(tone==2){
				tone=0;	UserEf.Pitch = 0;	// 原音
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);
			}
			break;
		case 0X1C:	// 演唱
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
			EF_Mode=1;
			break;
		case 0X1D:	// 美声
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);
			EF_Mode=2;
			break;
		case 0X1F:	// 原声
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);
			EF_Mode=0;
			break;
		case 0X1E:	// 传统/专业    混响模式
			app_wave_file_play_stop();
			if(EF_Mode==0||EF_Mode==1||EF_Mode==2||EF_Mode==4){
				EF_Mode=3;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);
			}
			else if(EF_Mode==3){
				EF_Mode=4;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);
			}
			break;
		case 0X42:	// 功放静音/解除静音
			mute_sw^=1;
			if(mute_sw)
				gpio_output(AMP_MUTE, 1);
			else
				gpio_output(AMP_MUTE, 0);
			break;
		case 0X51:	// 鼓掌
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);
			break;
		case 0X52:	// 欢呼
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
			break;
		case 0X53:	// 鄙视
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
			break;
		case 0X54:	// 尴尬
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);
			break;
		case 0X55:	// 笑
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);
			break;
		case 0X56:	// 么么哒
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_9);
			break;
		case 0X57:	// 开场
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);
			break;
		case 0X58:	// 贼拉拉
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_5);
			break;
		case 0X59:	// 混响按键
			app_wave_file_play_stop();
			if(EF_Mode==0||EF_Mode==1||EF_Mode==2||EF_Mode==4){
				EF_Mode=3;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);
			}
			else if(EF_Mode==3){
				EF_Mode=4;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);
			}
			break;
		case 0X43:	// 内录 默认开

			Neulu_Sw^=1;
			if(Neulu_Sw){
				ioGain[Usb_MusL]=-90;
				ioGain[Usb_MusR]=-90;
			}else{
				ioGain[Usb_MusL]=0;
				ioGain[Usb_MusR]=0;
			}

			break;
		case 0X45:	// 直播间静音
			ioGain[Max2L_I2s3L] = -90;
			break;
		case 0X44:	// 直播间静音关   默认关
			ioGain[Max2L_I2s3L] = 0;
			break;
		case 0X47:	// 话筒激励开
			ioGain[Adc4_Mic] = ((ioGain[Adc4_Mic]+6)>10)?10:ioGain[Adc4_Mic]+6;
			Jili_Sw=1;
			break;
		case 0X46:	// 话筒激励关       默认关
			if(Jili_Sw){
				ioGain[Adc4_Mic] = ioGain[Adc4_Mic]-6;
				Jili_Sw=0;
			}
			break;
		case 0XA0:	// 未充电
			charg_sta = 0;
			break;
		case 0XA3:	// 充电
			charg_sta = 1;
			break;
		case 0XEA:	// 开机发送，DSP收到回传电量
//			BattFun();
			break;
		case 0X49:	// 进入同轴输入
			system_work_mode_set_button(SYS_WM_SPDIF_MODE);
			break;
//		case 0X04:	// 退出同轴输入
//			system_work_mode_set_button(SYS_WM_BT_MODE);
//			break;
		}
	Uart2Rx_lf = 0;
}

void com1_cmd_send_proc(uint8_t *buf, uint8_t len);
uint8_t Uart2Tx_Buff[4]={0xFF,0xFF,0xFF,0xFF};     //接收BUF
uint8_t  TX2_Buf_B=0;                              //BUF索引
//蓝牙已连接,蓝牙已断开,,,,,,,,,,,,,,,,,
uint8_t COMM_Buff[]={0X7A,0X7C,0X80,0X81,0X82,0X83,0X84,0X00,0X01,0X02,0X03,0X04,0X05,0X06,0X0A,0X91,0X9A,0X93,0X70,0X71};
//*********************************************
void Send_Inf(uint8_t comm)
{
	Uart2Tx_Buff[0] = 0XAA;
	Uart2Tx_Buff[1] = 0X55;
	Uart2Tx_Buff[2] = comm;
	Uart2Tx_Buff[3] = 0XEF;
	com1_cmd_send_proc(Uart2Tx_Buff, 4);
	USER_DBG_INFO("Uart2Tx_Buff: %02X  %02X  %02X  %02X \n", Uart2Tx_Buff[0],Uart2Tx_Buff[1], Uart2Tx_Buff[2], Uart2Tx_Buff[3]);
}

//***************************************************
void Out_VolSet(float val)
{
//	ioGain[Out1L_DacL] = SysInf.OutVol;
//	ioGain[Out1R_DacR] = SysInf.OutVol;

//	ioGain[Out2L_I2s2L] = SysInf.OutVol;
//	ioGain[Out2R_I2s2R] = SysInf.OutVol;
}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	static uint8_t play_en =0;
	if(play_en){
//		app_wave_file_play_start(APP_WAVE_FILE_ID_REDIAL);
		switch (Gr){
			case 0:
//				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);
				break;
			case 1:
//				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
				break;
			case 2:
//				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);
				break;
			case 3:
//				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);
				break;
			case 4:
				//app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);
				break;
		}
	}else{
		play_en =1;
	}

	SysInf.EfGruup = Gr;
}
//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
	static uint8_t play_en =0;
	if(play_en){
		app_wave_file_play_stop();
//		app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		switch (Gr){
			case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
			case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
			case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		}
	}else{
		play_en =1;
	}
	SysInf.EqGruup = Gr;
}

//***********************************************************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{


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
		SysInf.WorkMode = sys_hdl->sys_work_mode;

		break;
	case APP_WAVE_FILE_ID_MUTE_MIC: //同轴输入

//		Send_Inf(0X9a);  //同轴输入初始化应答
		break;
	case APP_WAVE_FILE_ID_CONN: //蓝牙已连接

		Send_Inf(0X91);  //蓝牙初始化应答

		break;
	case APP_WAVE_FILE_ID_DISCONN: //蓝牙已断开
		Send_Inf(0X7C);
		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{
	//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		SysInf.EfGruup = EF_Mode;

		gpio_output(AMP_MUTE, 1);


		os_delay_us(160000);
	//	GroupWrite(EF_Mode, 0);		// 保存記憶
		GroupWrite(EF_Mode, 2);		// 保存記憶
		os_delay_us(160000);
		dsp_shutdown();

		gpio_output(POWER_EN, 0);
		gpio_config_new(POWER_EN, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);

		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF... %d  %d\n",SysInf.EfGruup,EF_Mode);
		break;
	case APP_WAVE_FILE_ID_CONN:
		Send_Inf(0X7A);  //进入蓝牙
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
//			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
#ifdef REC_EN
	case APP_WAVE_FILE_ID_RESERVED4:	// 開始錄音
		RecState = 3;
		break;
	case APP_WAVE_FILE_ID_RESERVED5:	// 停止錄音
		RecState = 2;
		break;
#endif
	case APP_WAVE_FILE_ID_RESERVED0:
	case APP_WAVE_FILE_ID_MP3_MODE:
#ifdef REC_EN
		if(RecState==16){
			RecState = 6;
		}else
#endif
		{
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//自動播放
		}
		break;
	}
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}

#endif

