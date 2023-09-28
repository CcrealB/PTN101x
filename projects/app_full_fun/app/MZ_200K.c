/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 日期版本:	V16_2	2023-2-4

*
*更新SDK: usb_desc.c    LED_EF_FUN.c
*
**********************************************************************/
#include "USER_Config.h"
#ifdef MZ_200K

#include "MZ_200K_DEF.H"

#ifdef CONFIG_AUD_AMP_DET_FFT
	extern	u8 W_Y[9];
	extern	u8 RGB_MODE[9];	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
	extern	u8 RGB_AUTO[9];	//聲控自動轉漸變(0,1)
	extern	u8 RGB_SP[9];	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
	extern	u8 RGB_EF[9];	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
	extern	u8 RGB_AEF[9];	//
	static uint8_t LED_n = 0;
#endif



static uint8_t	WmPoSw=0;

static uint8_t	LineIn=1;
uint8_t	LineInR=3;

static uint8_t	ChargFlash=0;
static uint8_t Batt_FULL=0;
static uint16_t BattVn = 0x01;
//static uint8_t UartOff=2;
//static uint8_t LowPowerEn=0;
//static uint8_t LowPowerCont=0;		// 進入低功耗計數

static uint8_t	BtLedFlash=0;

//static uint8_t  BK_DeyOnCont=0;

//static uint16_t DelayOffCont=0;
static uint16_t ms1000 = 0;

static uint16_t SysMsCont=0;

#define DisplayCont_def	3		// 充電燈顯示延時(S)
static uint8_t DisplayCont=DisplayCont_def;	// 充電燈顯示計時 (S)
//static uint16_t PowerDownCont=0;	// 沒聲音 30分 關機計時 (S)
//static uint8_t OffPowerCont = 0;
static uint16_t PoKeyCont=1001;
//uint16_t WMicOffCont=0;


uint8_t work_modeR=10;
static uint16_t WorkModeCont=0;
static uint16_t FadeInOut=0;
static uint8_t PlayIng=1;

//*****************************************************************************
//uint8_t MusVolVal;
uint8_t MusVolValR=20;
const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

//==== 关于 充电 =========================================
#define DisplayCont_def	3		// 充電燈顯示延時(S)
//uint8_t DisplayCont = 1;
float Battery_Cont=0;
uint16_t Battery_ContR=1;
//static uint8_t	ChargFlash=0;
//static uint8_t Batt_FULL=0;
//static uint16_t BattVn = 0x01;
static uint8_t LowPowerEn=0;
static uint8_t LowPowerCont=0;		// 進入低功耗計數
static uint16_t DelayOffCont=0;
static uint8_t OffPowerCont = 0;


#define LED2_AUX	GPIO9
#define	fullVn			8400	// 滿電壓 mv
#define	P4Voln			7800	// 亮4個燈電壓
#define	P3Voln			7200	// 亮3個燈電壓
#define	P2Voln			6600	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		6300	// 低電模式電壓
#define	LoPowerOFFv	 	6000	// 低電關機電壓

//#define	fullVn			12600	// 滿電壓 mv
//#define	P4Voln			11700	// 亮4個燈電壓
//#define	P3Voln			10800	// 亮3個燈電壓
//#define	P2Voln			9900	// 亮2個燈及解除低電模式電壓
//#define	LoPowerENv		9400	// 低電模式電壓
//#define	LoPowerOFFv	 	9000	// 低電關機電壓
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
	if(++BattVolDetMs >= 100){
		BattVolDetMs=0;

		//===================================================================================
		uint16_t BattVol = BattVolConvert(SarAdcVal[1]);
		static uint16_t saradc_tempe_dataR;
		static uint8_t Batt_FULLR;
		extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
		//--------------------------------
		fl2hex V1;
		if((abs(SarAdcValR[1]-SarAdcVal[1])>10) || (saradc_tempe_dataR != saradc_tempe_data  || Batt_FULLR != Batt_FULL)){
			SarAdcValR[1] =SarAdcVal[1];
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
//		if(DisplayCont) USER_DBG_INFO("==== BATT ADC:%d  %d mv  DcIn:%d  Batt_FULL:%d\n",SarAdcVal[6],BattVol,gpio_input(DcInDet),Batt_FULL);
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
//	//		}else if(SarAdcVal[1]>=2740){
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
////		USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[1]:%d\n",BattVn,SarAdcVal[1]);
//		if(DisplayCont || Batt_FULL==0){	// 顯示 或 未滿電
//			gpio_output(LED0_Bat0, (BattVn&1));
//			gpio_output(LED1_Bat1, ((BattVn>>1)&1));
//			gpio_output(LED2_Bat2, ((BattVn>>2)&1));
//			gpio_output(LED3_Bat3, ((BattVn>>3)&1));
//		}else if(!gpio_input(DcInDet) && Batt_FULL){
//			gpio_output(LED0_Bat0, 1);
//			gpio_output(LED1_Bat1, 1);
//			gpio_output(LED2_Bat2, 1);
//			gpio_output(LED3_Bat3, 1);
//		}else{
//			gpio_output(LED0_Bat0, 0);
//			gpio_output(LED1_Bat1, 0);
//			gpio_output(LED2_Bat2, 0);
//			gpio_output(LED3_Bat3, 0);
//		}
//	}else{
//
//		if(BattVol<LoPowerENv){	// 6.5v	9.75
//			gpio_output(LED0_Bat0, LowPowerFlash^=1);
//		}else{
//			gpio_output(LED0_Bat0, 0);
//			gpio_output(LED1_Bat1, 0);
//			gpio_output(LED2_Bat2, 0);
//			gpio_output(LED3_Bat3, 0);
//		}
//	}
		static uint8_t LowPowerFlash=1;
		if(BattVol<LoPowerENv){	// 6.5v	9.75
			gpio_output(LED2_AUX, LowPowerFlash^=1);
		}
		//==== 判斷進入 LowPower Mode =======================
		if(LowPowerEn==0 && BattVol<LoPowerENv){		// 6.5v		9.75
			USER_DBG_INFO("==== LowPowerCont: %d\n",LowPowerCont);
			if(++LowPowerCont > 5){
				LowPowerEn=1;
				DelayOffCont=300;
				OffPowerCont = 0;
				USER_DBG_INFO("==== LowPower Mode ADC:%d  %d mv\n",SarAdcVal[1],BattVol);
			}
		}else{
			LowPowerCont=0;
			if(LowPowerEn && BattVol>P2Voln){	// 低電解除
				LowPowerEn=0;
				USER_DBG_INFO("==== UnLowPower Mode ADC:%d  %d mv\n",SarAdcVal[1],BattVol);
			}
		}

		//===============================================================
		if(LowPowerEn){	// 低電
			if(SysInf.MusicVol > 24){
				SysInf.MusicVol = 24;
				DelayOffCont = 300;	// 立即播報一次低電提示音
			}
			if(++DelayOffCont > 300){	// 5分鐘播報一次低電提示音
				DelayOffCont=0;
				DisplayCont = DisplayCont_def;	// 計時顯示
				if(SysInf.Lang)	{
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
				}else{
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
				}
			}
			if(BattVol<LoPowerOFFv){	// 低電關機
				USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[1],BattVol, OffPowerCont);
				if(++OffPowerCont==6){	// 低電關機計時 播報一次低電提示音
					if(SysInf.Lang)	{
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
					}else{
						app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
						app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
						app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);

					}
				}else if(OffPowerCont>9){	// 低電關機計時 關機
					if(SysInf.Lang)	app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);    //关机E
						else		app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
				}
			}else{
				OffPowerCont = 0;
			}
		}else{
			DelayOffCont=0;
		}
	}
}

static uint8_t KeyId = BUTTON_BT_PLAY_PAUSE;
//********************************************************
void FadeInOut_Fun()
{
	FadeInOut--;
	if(FadeInOut==199){
		if(KeyId==BUTTON_BT_PLAY_PAUSE){
			if(app_is_bt_mode()){
			//	if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0 && player_get_play_status()==0){	// 無播放
			//		app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			//		FadeInOut = 51;	// 執行淡入
			//	}else{	// 播放中
					if(app_is_bt_mode()){
						app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
					}
					PlayIng =1;
			//	}
			}else if(app_is_line_mode()){	// LINE MODE
				PlayIng ^=1;
				if(PlayIng)	FadeInOut = 51;	// UMUTE
			}
		}else{
			PlayIng =1;
		}
	}else  if(FadeInOut==198){
		IOGainSetDamp(MusMstVol, -90,10);	// Fade Out
		if(PlayIng==0)	FadeInOut = 1;
	}else  if(FadeInOut==150){				// Fade Out End
		if(KeyId==BUTTON_MODE_CHG){
			system_work_mode_change_button();
			WorkModeCont = 99;
		}else if(KeyId==BUTTON_BT_PREV || KeyId==BUTTON_BT_NEXT|| KeyId==BUTTON_BT_NEXT|| KeyId==BUTTON_BT_NEXT){
			if(app_is_bt_mode()){
				app_button_sw_action(KeyId);
			}
		}else if(KeyId==BUTTON_BT_PLAY_PAUSE){
	//		if((app_is_mp3_mode()&&player_get_play_status())){
	//			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
	//		}
			if(app_is_bt_mode()==0)	FadeInOut =1;
//				else				FadeInOut =100;
		}
	}else if(FadeInOut==50){	// Fade In
		IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],5);	//yuanV41
	}else if(FadeInOut==0){
		ioGain[MusMstVol]=MusVol_TAB[(uint8_t)SysInf.MusicVol];	//yuanV41
	}
}


//**** pre init ***********************
void user_init_prv(void)
{
	//==== 充電檢測假開機 =============================
	BK3000_set_clock(CPU_CLK_XTAL, CPU_CLK_DIV);
//	#define DcInDet		GPIO18
//	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
#ifdef   CHG_UART_RX_OFF
	if(gpio_input(DcInDet)){	// 充電時關閉  UART IO 必免與 USB充電芯片 USB 接口衝突
		system_peri_mcu_irq_disable(SYS_PERI_IRQ_UART0);
		gpio_config_new(GPIO1, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);	// RX
#ifdef CHG_UART_TX_OFF
		gpio_config_new(GPIO0, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);	// TX
#endif
	}
#endif

	#define WM_PO_EN	GPIO12
	#define ACM8625_EN	GPIO35
	#define POWER_EN	GPIO13

	#define LED1_BT		GPIO8
//	#define LED2_AUX	GPIO9
	#define LED3_MIC	GPIO10
	#define LED4_USB	GPIO22
	gpio_config_new(LED1_BT, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED2_AUX, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED3_MIC, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED4_USB, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LED1_BT, 0);
	gpio_output(LED2_AUX, 0);
	gpio_output(LED3_MIC, 0);
	gpio_output(LED4_USB, 0);

	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(ACM8625_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 1);
	gpio_output(ACM8625_EN, 1);
	gpio_output(POWER_EN, 0);

//	#define ChargFullGpio	GPIO31
//	gpio_config_new(ChargFull, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	#define PowerKey	GPIO15
	gpio_config_new(PowerKey, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

//	#define ChargFull	GPIO31
//	gpio_config_new(ChargFull, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	//==========================================
	if(vusb_4v_ready() && gpio_input(PowerKey)){	// USB供電啟動,電源鍵為按下,進入假開機充電模式
		uint16_t cont=1000;
		for(uint16_t i=0; i<3000; i++){
			CLEAR_WDT; //清除看门狗，默约38.8sec
			os_delay_us(120);	//1ms 延遲 3秒 激活已保護輸出電池
			if(gpio_input(PowerKey)==0)	i=3000;
		}
//		gpio_config_new(LED0_Bat0, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//		gpio_config_new(LED1_Bat1, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//		gpio_config_new(LED2_Bat2, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//		gpio_config_new(LED3_Bat3, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
		DisplayCont=10;
		//============================================
		while(1){
			CLEAR_WDT; //清除看门狗，默约38.8sec
			//---------------------------------------
			if(gpio_input(PowerKey)==0){
				USER_DBG_INFO("======== 1 =========\n");
				dsp_shutdown();
				REG_SYSTEM_0x1D &= ~0x2;
				system_wdt_reset_ana_dig(1, 1);
				BK3000_wdt_reset();
				os_delay_us(3200);	//1ms
				USER_DBG_INFO("======== 2 =========\n");
				break;
			}
			os_delay_us(120);	//1ms
			if(++cont >= 1000){
				cont = 0;
				if(ChargFlash==0){
					SarAdcVal[1] = SarADC_val_cali_get(sar_adc_voltage_get(SARADC_CH_GPIO21));
					if(SarAdcVal[1]==0) SarAdcVal[1] = SarAdcValR[1];	// 防止誤讀 0 值
					SarAdcValR[1] = SarAdcVal[1];
				#ifdef ChargFullGpio
					Batt_FULL = !gpio_input(ChargFull);
				#else
					//8.4	3460	3420	3380	3340
					if(SarAdcVal[1]>=3450)	Batt_FULL = 1;	// 8.375V 滿電
						else Batt_FULL =0;
				//	Batt_FULL = !vusb_2v_ready();
				#endif
					if(SarAdcValR[1]>=3340 && Batt_FULL){
						if(DisplayCont){
							DisplayCont--;
							BattVn=0x0F;
						}else{
//							gpio_output(LED0_Bat0, 0);
//							gpio_output(LED1_Bat1, 0);
//							gpio_output(LED2_Bat2, 0);
//							gpio_output(LED3_Bat3, 0);
							continue;
						}
			//		#ifndef ChargeFullDis
						app_button_sw_action(BUTTON_BT_POWERDOWN);	// 8.3V
			//		#endif
					}else if(SarAdcValR[1]>=3180){
						BattVn=0x07;
					}else if(SarAdcValR[1]>=2940){
						BattVn=0x03;
				//	}else if(SarAdcValR[1]>=2540){
				//		BattVn=0x01;
					}else{
						BattVn=1;
					}
					ChargFlash = 1;
				}else{
					if(((BattVn>>1)&1)==0){
						BattVn=0x03;
					}else if(((BattVn>>2)&1)==0){
						BattVn=0x07;
					}else if(((BattVn>>3)&1)==0){
						BattVn=0x0F;
						ChargFlash = 0;
					}else{
						ChargFlash = 0;
					}
				}
//				gpio_output(LED0_Bat0, (BattVn&1));
//				gpio_output(LED1_Bat1, ((BattVn>>1)&1));
//				gpio_output(LED2_Bat2, ((BattVn>>2)&1));
//				gpio_output(LED3_Bat3, ((BattVn>>3)&1));
			}
		}
	}

	//====================================================================
	gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);

	PowerDownFg=0;

}

//*****************************************************
void user_WorkModefun(void)
{
#if 0
	//---- SD DET ---------------------------
	static uint8_t	SdCareDetR=1;
//	app_sd_scanning();
	if(SdCareDetR != !sd_is_attached()){
		SdCareDetR = !sd_is_attached();
		if(SdCareDetR==0){
			system_work_mode_set_button(SYS_WM_SDCARD_MODE);
		}else{
			if(app_is_mp3_mode()){
				system_work_mode_change_button();
			}
		}
	}
#endif
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
//			USER_DBG_INFO("++++++++++++++++++++++++++++++++++++++++++++++++ AuxInDet %d\n",LineInR);
		}
	}else{
		LineInDetCont=0;
	}

	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态

		app_handle_t sys_hdl = app_get_sys_handler();
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				gpio_output(LED1_BT, 1);
				gpio_output(LED2_AUX, 0);
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= 0;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];

			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				gpio_output(LED1_BT, 1);
				gpio_output(LED2_AUX, 0);
			}else{
				if(BtLedFlash^=1)	gpio_output(LED1_BT, 1);
					else			gpio_output(LED1_BT, 0);
			}
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				gpio_output(LED1_BT, 0);
				gpio_output(LED2_AUX, 1);
			//	if(ioGain[Adc2_MusL] < - 15){
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= 0;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
				PlayIng =1;
			}
		}
		//===============================
		if(usbd_active_get()){
			gpio_output(LED4_USB, 1);
		}else{
			gpio_output(LED4_USB, 0);
		}
	}
}


//**** normal init ********************
void user_init(void)
{

	UserFlash_Init();	// 設置FLASH 傳入版本參數

//	appPlayerPlayMode = SysInf.SdCardPlayMode;
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

	LineInR = gpio_input(LINE_DETECT_IO);
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;

	USER_DBG_INFO("======== V33_01_CE user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

#if LED_RGB_NUM
	app_i2s0_init(1);
#endif
}

//********************************
void user_init_post(void)
{
	//============================
	Hi2c_Init();


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
	SysMsCont++;
	TbTime++;
}
#endif

static uint32_t PowerDownCont=0;	// 15分 關機計時 (S)
static uint16_t Init_Cont = 0;
//**********************************************
void user_loop_10ms(void)
{
	if(PowerDownFg)	return;

	//======================================
	if(Init_Cont == 150){
		SysMsCont=0;
#ifdef	CONFIG_AUD_AMP_DET_FFT
		//============================
		static uint8_t led_30ms=0;
		if(++led_30ms >=3){
			led_30ms=0;
			void RGB_LED_EF_FUN(void);
			RGB_LED_EF_FUN();
		}
#endif
		//======================
		if(EF_Maim()) return;
		//======================
		if(ACM_main()) return;

		//======================
		user_WorkModefun();
		//===================
		if(FadeInOut)	FadeInOut_Fun();
		//===================
		KT56_main();
		//======================
		user_key_scan();
		//======================
		BattFun();

		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;
			//==== 數字功放訊息報告 ====
			ACM_REPORT();
		}

		//8.26++ 2小时无蓝牙连接自动关机
		app_handle_t sys_hdl = app_get_sys_handler();
		if(hci_get_acl_link_count(sys_hdl->unit)){ //蓝牙连接
			PowerDownCont=0;
		}else{
			if(++PowerDownCont>=720000){
				if(SysInf.Lang)
					app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);    //关机E
				else
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
			}
			if(PowerDownCont%6000==0) USER_DBG_INFO("==== DelayPoOffCont:%d  isconnect:%d\n", PowerDownCont, hci_get_acl_link_count(sys_hdl->unit));
		}

		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//======================================================================
	}else{
		Init_Cont++;

		if(Init_Cont == 1){
			gpio_output(ACM8625_EN, 1);
			USER_DBG_INFO("==== 1. Init_Cont MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 10){
			ACM8625_init();
			USER_DBG_INFO("==== 10. ACM8625_init Ok... MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 11){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
		}else if(Init_Cont == 12){
			EF_EQ_ReSend();
			EF_ModeR = EF_Mode;
			EQ_ModeR = EQ_Mode;
			USER_DBG_INFO("==== 12. EQ_ReSend Ok...MsCont:%d \n",SysMsCont);

		}else if(Init_Cont == 13){
			USER_DBG_INFO("====13. Init IoGain...\n",SysMsCont);
			// 需等待 DSP 啟動完成才可開始配置 gain
			IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
			IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
			IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],64);
			IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);
			IOGainSetDamp(MusL_Out2L, -90,64);
			IOGainSetDamp(MusR_Out2R, -90,64);

			gpio_output(POWER_EN, 1);
			gpio_output(LED3_MIC, 1);
			gpio_output(LED1_BT, 1);
			RgbLedAllColour(1);      //8.14++
			if(SysInf.Lang)
				app_wave_file_play_start(APP_WAVE_FILE_ID_UNMUTE_MIC);   //开机E
			else
				app_wave_file_play_start(APP_WAVE_FILE_ID_START);        //开机C

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
			SysMsCont=0;
		}else if(Init_Cont == 148){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;
//			system_work_mode_set_button(SysInf.WorkMode);    //SysInf.WorkMode记忆点
			system_work_mode_set_button(app_h->sys_work_mode);
#ifdef	CONFIG_AUD_AMP_DET_FFT
			extern void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
			aud_mag_det_enable(1);	//audio magnitude detect enable
#endif
		}else if(Init_Cont == 149){
			KT56_Init();            //开机默认打开麦克风
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
	app_wave_file_play_stop();
	switch (EF_Mode){
		case 0:
			SysInf.VoiceCanEn = 0;//消原音关
			if(SysInf.Lang)
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);  //KTV E
			else
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0);  //KTV

			break;
		case 1:
			SysInf.VoiceCanEn = 1;//消原音开
			if(SysInf.Lang)
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2); //消原音E
			else
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);	    //消原音

			break;
		case 2:
			SysInf.VoiceCanEn = 0;//消原音关
			if(SysInf.Lang)
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3); //主持人E
			else
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2); //主持人

			break;
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
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_FM_MODE); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);	break;
	}
}
//8.26++
static uint8_t MusicVolR=0;
static uint8_t MR_flag=0;
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
//	USER_DBG_INFO("====key: %d:  %d\n", pKeyArray->index, pKeyArray->event);
	extern uint32_t user_aud_amp_task(void);
	UpComputerRW = 0;	// 更新上位機
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;
	case KEY_S_REL:
//		ms1000=99;
		switch (pKeyArray->index){
		case 0:	// MODE
		case 1:
			DisplayCont = DisplayCont_def;
			KeyId = BUTTON_BT_PLAY_PAUSE;
			if(FadeInOut<150)	FadeInOut = 200;
			break;
#if 1
		case 2:	// NEXT
			if(FadeInOut<150 && (app_is_bt_mode() || app_is_mp3_mode())){
				KeyId = BUTTON_BT_NEXT;
				FadeInOut = 200;
			}
			break;
		case 3:	// PRE
			if(FadeInOut<150 && (app_is_bt_mode() || app_is_mp3_mode())){
				KeyId = BUTTON_BT_PREV;
				FadeInOut = 200;
			}
			break;
#else
		case 2:	// VOL +
			if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
			break;
		case 3:	// VOL -
			if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
			break;
#endif
		case 4:
			if(FadeInOut==0){	// MODE 切換中 按鍵失效
				if(sys_hdl->sys_work_mode==SYS_WM_BT_MODE && sd_is_attached()==0 && LineInR)	return;
				KeyId = BUTTON_MODE_CHG;
				FadeInOut = 200;
			}
			break;
		case 5:
#ifdef CONFIG_AUD_AMP_DET_FFT
			if(++LED_n >8)	LED_n = 0;

			if(LED_n ==0){
				RGB_MODE[0]=2;	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
				RGB_EF[0]= 4;	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
				RGB_SP[0]= 5;	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
				RGB_AUTO[0]=1;	//聲控自動轉漸變(0,1)
				RGB_AEF[0]=0;
				W_Y[0]=255;
			}else if(LED_n ==1){
				RGB_MODE[0]=4;	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
				RGB_EF[0]= 4;	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
				RGB_SP[0]= 4;	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
				RGB_AUTO[0]=0;	//聲控自動轉漸變(0,1)
				RGB_AEF[0]=0;
				W_Y[0]=255;
			}else if(LED_n ==2){    //白色
				RGB_MODE[0]=1;	// 單色配置
				RGB_EF[0]= 15;
				RGB_SP[0]= 11;	// 顏色 0~ 11
				/**********************************************
				 	0.暖白色	1.紅色	2.綠色	3.藍色	4.黃色
					5.紫色	6.青色	7.桃紅色	8.夢幻粉	9.土豪金
					10.神秘紫	11.白色	12.關燈
				 **********************************************/
			}else if(LED_n ==3){     //黃色
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 4;
			}else if(LED_n ==4){     //深蓝
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 3;
			}else if(LED_n ==5){     //绿色
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 2;
			}else if(LED_n ==6){     //紫色
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 10;
			}else if(LED_n ==7){     //红色
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 1;
			}else if(LED_n ==8){     //粉色
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 8;
			}

#endif
			USER_DBG_INFO("====LED_n: %d\n", LED_n);
			break;
		}
		DisplayCont = DisplayCont_def;
		break;

	//====================================================================================================
	case KEY_D_CLICK:	// 短按 2次
		switch (pKeyArray->index){
			case 4:	// WMIC ON/OFF
				WmPoSw^=1;
				if(WmPoSw){
					if(SysInf.Lang)
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);   //MIC OFF
					else
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);   //关闭麦克风
					gpio_output(WM_PO_EN, 0);
					Chip_E[0]=0;
					Chip_E[1]=0;
					gpio_output(LED3_MIC, 0);

				}else{
					if(SysInf.Lang)
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);   //MIC ON
					else
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);   //打开麦克风
					gpio_output(WM_PO_EN, 1);
					gpio_output(LED3_MIC, 1);
					KT56_Init();
				}
				break;
			case 5:	  //双击关灯
				RGB_MODE[0]=1;
				RGB_EF[0]= 0;
				RGB_SP[0]= 0;
				LED_n -= 1;
				break;
		}
		break;
	case KEY_T_CLICK:
		USER_DBG_INFO("KEY_T_CLICK\n");		// 短按 3次
		break;
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:	// 常按進入
//		USER_DBG_INFO("KEY_L_PRES\n");
		switch (pKeyArray->index){
		case 2:	// VOL +
			if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)||user_aud_amp_task()){      //8.14++
				if(LowPowerEn){	// 低電
					if(SysInf.MusicVol<24){
						SysInf.MusicVol++;// SD, AUX MODE	//yuan41
					}else{
						DelayOffCont = 300;	// 立即播報一次低電提示音
					}
				}else{
					if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
				}
			}
			break;
		case 3:	// VOL -
			if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
			break;
		case 4:	  //蓝牙
			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			}
			break;
		case 5:	  //长按关灯
			RGB_MODE[0]=1;
			RGB_EF[0]= 0;
			RGB_SP[0]= 0;
			LED_n -= 1;
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		USER_DBG_INFO("1.KEY_L_PRESSING %d   %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime,user_aud_amp_task());
		//---------------------------------------------------
		if(pKeyArray->keepTime>5000){
			pKeyArray->keepTime=0;
			if(pKeyArray->index==2){
				USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);

				if(user_aud_amp_task()==0){	    //侦测无声音
					SysInf.Lang ^=1;
					if(SysInf.Lang)
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);    //英文
					else
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);    //中文


					//8.26++
					SysInf.MusicVol = MusicVolR;
					MR_flag=0;

				}
			}
		}else if(pKeyArray->keepTime%100==0){
			//USER_DBG_INFO("2.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			switch (pKeyArray->index){
			case 2:	// VOL +
				if(MR_flag==0){   //8.26++
					MusicVolR=SysInf.MusicVol;
					MR_flag=1;
				}
				if(LowPowerEn){	// 低電
					if(SysInf.MusicVol<24)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
				}else{
					if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
				}
				break;
			case 3:	// VOL -
				if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
				break;
			}
		}else if(pKeyArray->keepTime > 1000){
//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			if(pKeyArray->index==0){
				pKeyArray->keepTime = 0;
				if(SysInf.Lang)
					app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);    //关机E
				else
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
			}
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}

}

//********************************
void PlayWaveStart(uint16_t id)
{
	USER_DBG_INFO("==== wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);		//Music Off
	app_handle_t sys_hdl = app_get_sys_handler();       //8.14++
	if(WmPoSw){
		IOGainSetDamp(MicMstVol, -90,32);	//Mic Off
	}
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:

		RgbLedAllColour(0);
//		app_handle_t sys_hdl = app_get_sys_handler();       //8.14--
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PoKeyCont=0;
		PowerDownFg=1;

		SysInf.VoiceCanEn = 0;	// 關閉消原音
//		SysInf.MusVol32 = MusVolVal;	//yuan41
//		SysInf.WorkMode = sys_hdl->sys_work_mode;
		GroupWrite(0, 2);		// 保存記憶
		break;
	case APP_WAVE_FILE_ID_MUTE_MIC:
		RgbLedAllColour(0);
		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}
		PoKeyCont=0;
		PowerDownFg=1;

		SysInf.VoiceCanEn = 0;	// 關閉消原音
//		SysInf.MusVol32 = MusVolVal;	//yuan41
//		SysInf.WorkMode = sys_hdl->sys_work_mode;
		GroupWrite(0, 2);		// 保存記憶      C++//
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
//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		gpio_output(WM_PO_EN, 0);
		gpio_output(ACM8625_EN, 0);
		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		gpio_output(POWER_EN, 0);
		os_delay_us(3200);	//1ms
		break;
	case APP_WAVE_FILE_ID_MUTE_MIC:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		gpio_output(WM_PO_EN, 0);
		gpio_output(ACM8625_EN, 0);
		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		gpio_output(POWER_EN, 0);
		os_delay_us(3200);	//1ms
		break;
	case APP_WAVE_FILE_ID_CONN:
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
	case APP_WAVE_FILE_ID_LOW_BATTERY:
		break;
	case APP_WAVE_FILE_ID_VOL_MAX:
	case APP_WAVE_FILE_ID_VOICE_NUM_5:

		break;
	case APP_WAVE_FILE_ID_HFP_ACK:
//		app_button_sw_action(BUTTON_BT_PREV);
		break;
	case APP_WAVE_FILE_ID_HFP_REJECT:
//		app_button_sw_action(BUTTON_BT_NEXT);
		break;
	}
	//=========================================================
//	SwChgIng = 0;
	IOGainSetDamp(MusMstVol,MusVol_TAB[(uint8_t)SysInf.MusicVol],10);	//yuan41
	if(WmPoSw){
		IOGainSetDamp(MicMstVol, 5,32);
	}
	USER_DBG_INFO("==== wave_stop, ID:%d  MusicVol:%d\n", id, MusVol_TAB[(uint8_t)SysInf.MusicVol]);
}

#endif

