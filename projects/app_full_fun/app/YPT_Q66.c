/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen JQ
* 版本:	V1.0
* 日期： 2023-04-20
* 描述：	1，	电池检测不正常        OK
        2，	GPIO28脚作为灯4，某一时刻被改为输入模式？  加一行输出模式就正常了
        3,	充电检测不正常，原因在于IO口的输入配置输入不能放在Init_Pre和Init,放在Init_Post可以。。。输出放在Init_Pre可以
        4,  关机充电 DC插上去喇叭有po声，功放芯片工作之后把DSP关了而产生。应在未打开AMP之前关掉DSP 。   经DEBUG测试DCINDET在系统初始化的130ms之前都被置高，之后读取的才是正确的。
        5,  开机充电后，关机保留DC充电，喇叭有po声。原因是功放芯片工作之后把DSP关了而产生。应先关闭AMP再关掉DSP
**********************************************************************/
#include "USER_Config.h"

#ifdef YPT_Q66

#include "YPT_Q66_DEF.H"

const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

const int8_t MicVol_Tab[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};


uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
//extern uint8 appPlayerPlayMode;
//static uint8_t DuckDetF = 0;
static uint16_t FadeInOut=0;      //fade++
static uint8_t PlayIng=1;         //fade++
static uint16_t WorkModeCont=0;
//uint8_t work_modeR=10;
static uint8_t	BtLedFlash=0;
uint8_t LineInR=0;
uint8_t LineIn2R=0;

static uint8_t	LineIn=3;
static uint16_t SysMsCont=0;



//uint8_t Mus_Val=16;
//uint8_t Mic_Val=16;
uint8_t Mus_ValR=255;
uint8_t Mic_ValR=255;
uint16_t WMicOffCont=0;


//关于 充电
#define DisplayCont_def	3		// 充電燈顯示延時(S)
uint8_t DisplayCont = 1;
float Battery_Cont=0;
uint16_t Battery_ContR=1;
static uint8_t	ChargFlash=0;
static uint8_t Batt_FULL=0;
static uint16_t BattVn = 0x01;
static uint8_t LowPowerEn=0;
static uint8_t LowPowerCont=0;		// 進入低功耗計數
static uint16_t DelayOffCont=0;
static uint8_t OffPowerCont = 0;

#define DcInDet			GPIO14      //充电检测
#define LED3_Bat3		GPIO6
#define LED2_Bat2		GPIO30
#define LED1_Bat1		GPIO3
#define LED0_Bat0		GPIO28

#define	fullVn			12600	// 滿電壓 mv
#define	P4Voln			11700	// 亮4個燈電壓
#define	P3Voln			10800	// 亮3個燈電壓
#define	P2Voln			9900	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		9400	// 低電模式電壓
#define	LoPowerOFFv	 	9000	// 低電關機電壓
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
	static uint16_t BattVolDetMs = 99;
//	static uint16_t LowBattCont = 0;
	if(++BattVolDetMs == 100){
		BattVolDetMs=0;

	//===================================================================================
	uint16_t BattVol = BattVolConvert(SarAdcVal[6]);
	static uint16_t saradc_tempe_dataR;
	static uint8_t DcInDetR,Batt_FULLR;
	extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
	//--------------------------------
	fl2hex V1;
	if((abs(SarAdcValR[6]-SarAdcVal[6])>10) || (saradc_tempe_dataR != saradc_tempe_data || \
			DcInDetR != gpio_input(DcInDet) || Batt_FULLR != Batt_FULL)){
		SarAdcValR[6] =SarAdcVal[6];
		saradc_tempe_dataR = saradc_tempe_data;
		DcInDetR = gpio_input(DcInDet);
		Batt_FULLR = Batt_FULL;
		Send_Id1 = MainPid;
		Send_Id2 = 205;
		Send_Group = 1;
		V1.f = (float)BattVol/1000;
		Send_Val1 = V1.h;
		V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
		Send_Val2 = V1.h;
		Send_Val41 = gpio_input(DcInDet);
		Send_Val42 = Batt_FULL;
		SendUpComp(SendBuff, 32);
	}
	if(DisplayCont || !gpio_input(DcInDet)){
		if(DisplayCont) USER_DBG_INFO("==== BATT ADC:%d  %d mv  DcIn:%d  Batt_FULL:%d\n",SarAdcVal[6],BattVol,gpio_input(DcInDet),Batt_FULL);
//		if(DisplayCont)	DisplayCont--;      //充电的时候   充电灯显示时间结束就关掉，操作机器时候就会亮充电灯
		if(ChargFlash==0){
		#ifdef ChargeFullDis
			if(BattVol>=fullVn)	Batt_FULL = 1;	// 8.375V 滿電
				else Batt_FULL =0;
		#else
			Batt_FULL = !gpio_input(ChargFull);
		#endif
			if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
				BattVn=0x0F;
			}else if(BattVol>=P3Voln){	// 7.5	11.25
				BattVn=0x07;
			}else if(BattVol>=P2Voln){	// 7	10.5
				BattVn=0x03;
	//		}else if(SarAdcVal[1]>=2740){
	//			BattVn=0x01;
			}else{
				BattVn=1;
			}
			ChargFlash = 1;
		}else if(ChargFlash && Batt_FULL==0 && !gpio_input(DcInDet)){
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
		}else{
			ChargFlash = 0;
		}
//		USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[1]:%d\n",BattVn,SarAdcVal[1]);
		if(DisplayCont || Batt_FULL==0){	// 顯示 或 未滿電
			gpio_output(LED0_Bat0, (BattVn&1));
			gpio_output(LED1_Bat1, ((BattVn>>1)&1));
			gpio_output(LED2_Bat2, ((BattVn>>2)&1));
			gpio_output(LED3_Bat3, ((BattVn>>3)&1));
		}else if(!gpio_input(DcInDet) && Batt_FULL){
			gpio_output(LED0_Bat0, 1);
			gpio_output(LED1_Bat1, 1);
			gpio_output(LED2_Bat2, 1);
			gpio_output(LED3_Bat3, 1);
		}else{
			gpio_output(LED0_Bat0, 0);
			gpio_output(LED1_Bat1, 0);
			gpio_output(LED2_Bat2, 0);
			gpio_output(LED3_Bat3, 0);
		}
	}else{
		static uint8_t LowPowerFlash=1;
		if(BattVol<LoPowerENv){	// 6.5v	9.75
			gpio_output(LED0_Bat0, LowPowerFlash^=1);
		}else{
			gpio_output(LED0_Bat0, 0);
			gpio_output(LED1_Bat1, 0);
			gpio_output(LED2_Bat2, 0);
			gpio_output(LED3_Bat3, 0);
		}
	}

	//==== 判斷進入 LowPower Mode =======================
	if(LowPowerEn==0 && BattVol<LoPowerENv){		// 6.5v		9.75
		if(++LowPowerCont > 5){
			LowPowerEn=1;
			DelayOffCont=0;
			OffPowerCont = 0;
			USER_DBG_INFO("==== LowPower ModeT ADC:%d  %d\n",SarAdcVal[6],BattVol);
		}
	}else{
		LowPowerCont=0;
		if(LowPowerEn && SarAdcVal[6]>=P2Voln){	// 低電解除
			LowPowerEn=0;
			USER_DBG_INFO("==== UnLowPower ModeT ADC:%d  %d\n",SarAdcVal[6],BattVol);
		}
	}

	//===============================================================
	if(LowPowerEn){	// 低電
		if(++DelayOffCont > 300){	// 5s
			DelayOffCont=0;
			DisplayCont = DisplayCont_def;
			if(!(gpio_input(DcInDet)==0))	app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 充電中不報低電提示音
		}
		if(BattVol<LoPowerOFFv){	// 6v
			USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[6],BattVol, OffPowerCont);
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

//**** pre init ***********************
void user_init_prv(void)
{
	//=============================	GPIO配置

	#define LINE_DETECT_IO	GPIO12      //LINEIN检测

	#define PowerKey		GPIO24      //开机检测脚

	#define ACM_PDN		GPIO35		//ACM功放+监听耳机   改 检测口
	#define LED_EN2		GPIO21      //led灯珠
	gpio_config_new(LED_EN2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(ACM_PDN, 	GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LED_EN2, 0);

	//LED灯

	#define LED_BT	        GPIO2      	//蓝牙闪烁指示灯
	gpio_config_new(LED0_Bat0, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED1_Bat1, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED2_Bat2, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED3_Bat3, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED_BT, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LED0_Bat0, 0);
	gpio_output(LED1_Bat1, 0);
	gpio_output(LED2_Bat2, 0);
	gpio_output(LED3_Bat3, 0);
	gpio_output(LED_BT, 0);

	USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));

//	if(gpio_input(PowerKey)==0){
//		USER_DBG_INFO("======== 1 =========\n");
//		dsp_shutdown();
//		REG_SYSTEM_0x1D &= ~0x2;
//		system_wdt_reset_ana_dig(1, 1);
//		BK3000_wdt_reset();
//		os_delay_us(3200);	//1ms
//		USER_DBG_INFO("======== 2 =========\n");
//	}
//	BK3000_set_clock(CPU_CLK_XTAL, CPU_CLK_DIV);   //ref ZY

//	//==========================================关机充电
//	gpio_config_new(PowerKey, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
//	USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
////	uint16_t BattVol = BattVolChg(SarAdcVal[6]);
//	uint16_t BattVol = BattVolConvert(SarAdcVal[6]);
//		if(gpio_input(DcInDet)==0 && gpio_input(PowerKey)==0){      //DC ON 且  开关OFF
//		USER_DBG_INFO("========  off power charge =========\n");
//			uint16_t cont=1000;
//			for(uint16_t i=0; i<3000; i++){
//				CLEAR_WDT; //清除看门狗，默约38.8sec
//				os_delay_us(120);	//1ms 延遲 3秒 激活已保護輸出電池
//				if(gpio_input(PowerKey)==0)	i=3000;
//			}
//			USER_DBG_INFO("========  off power charge two step=========\n");
////	//		DisplayCont=10;
////			//============================================
//			while(1){
//				CLEAR_WDT; //清除看门狗，默约38.8sec
//				USER_DBG_INFO("========  off power charge third step=========\n");
//				USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
//				//---------------------------------------
//				if(gpio_input(PowerKey)==1){
//					gpio_output(LED0_Bat0, 1);
//					gpio_output(LED1_Bat1, 1);
//					gpio_output(LED2_Bat2, 1);
//					gpio_output(LED3_Bat3, 1);
//					goto here;
//				}
////				os_delay_us(120);	//1ms
//				if(++cont >= 1000){
//					cont = 0;
//						//充电
//					//充电
//					if(gpio_input(DcInDet)==0){
//						if(ChargFlash==0){        //充电里面的一个功能标志位
//							//电量显示
//
//							if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
//								BattVn=0x0F;
//							}else if(BattVol>=P3Voln){	// 7.5	11.25
//								BattVn=0x07;
//							}else if(BattVol>=P2Voln){	// 7	10.5
//								BattVn=0x03;
//							}else{
//								BattVn=1;
//							}
//							ChargFlash = 1;
//						}
//						else if(ChargFlash && Batt_FULL==0){    //未满电
//							if(((BattVn>>1)&1)==0){
//								BattVn=0x03;
//
//							}else if(((BattVn>>2)&1)==0){
//								BattVn=0x07;
//
//							}else if(((BattVn>>3)&1)==0){
//								BattVn=0x0F;
//								ChargFlash = 0;
//							}else{
//								ChargFlash = 0;
//							}
//						}
//						else if(ChargFlash && Batt_FULL){       //满电
//							ChargFlash = 0;
//						}
//						gpio_output(LED0_Bat0, (BattVn&1));
//						gpio_output(LED1_Bat1, ((BattVn>>1)&1));
//						gpio_output(LED2_Bat2, ((BattVn>>2)&1));
//						gpio_output(LED3_Bat3, ((BattVn>>3)&1));
//			}
//				}
//			}
//	}
//		here:
//		USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數
//	if(SysInf.MusicVol<2)SysInf.MusicVol=2;


	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;




#if LED_RGB_NUM
	app_i2s0_init(1);
//	RgbLedOneColour(7,4);
	//RgbLedOut(8);
	RgbLedAllColour(0xff);

#endif

	USER_DBG_INFO("======== V33_01_CE user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);
}

//static void key_function(KEY_CONTEXT_t *pKeyArray);
//********************************
void user_init_post(void)
{
    //---------------------
	Hi2c_Init();
    //---------------------
	extern void key_function(KEY_CONTEXT_t *pKeyArray);
    extern void user_key_init(void* pKeyFuncCbk);
    user_key_init(key_function);
    //---------------------
	u_SaradcKbon_Init();

	gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(PowerKey, 	GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED0_Bat0, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED1_Bat1, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED2_Bat2, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(LED3_Bat3, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
#ifdef LED_DRIVER_BY_I2S0_TX
    extern void user_app_i2s0_init(uint8_t en);
	user_app_i2s0_init(1);
#endif
	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
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
void tick_task_1ms(void) //timer0 1ms isr
{
//	if(PowerDownFg) return;
	//------------------------------
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
	if(Tick_2ms)	user_saradc_update_trig();	// Adc 讀取觸發
	SysMsCont++;
#ifdef	CONFIG_AUD_AMP_DET_FFT
	extern u32	Tempo_ms_cont;
	Tempo_ms_cont++;
#endif
}


static uint8_t KeyId = BUTTON_BT_PLAY_PAUSE;

void FadeInOut_Fun()
{
	FadeInOut--;
	if(FadeInOut==199){
		if(KeyId==BUTTON_BT_PLAY_PAUSE){    //播放|暂停 动作
			if(app_is_bt_mode() || app_is_mp3_mode()){     //蓝牙|SD卡 模式
				if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0 && player_get_play_status()==0){	//蓝牙和SD卡無播放
					app_button_sw_action(BUTTON_BT_PLAY_PAUSE);//播放
					FadeInOut = 51;	// Fade In
				}else{	//播放中
					if(app_is_bt_mode()){
						app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
					}
					PlayIng =1;
				}
			}else if(app_is_line_mode()){	// LINE MODE
				PlayIng ^=1;
				if(PlayIng)	FadeInOut = 51;	// 未播放 ->播放 淡入
			}
		}else{
			PlayIng = 1;  //播放标志位
		}
	}else  if(FadeInOut==198){				// Fade Out
      IOGainSetDamp(MusMstVol, -90,10);	// Fade Out
		if(PlayIng==0)	FadeInOut = 1;      // 进LINE MODE，静音保持
	}else  if(FadeInOut==150){				// Fade Out End
		if(KeyId==BUTTON_MODE_CHG){         // 模式改变标志位
			system_work_mode_change_button();
			WorkModeCont = 99;
		}
		else if(KeyId==BUTTON_BT_PREV || KeyId==BUTTON_BT_NEXT){   //上一首还是下一首
			if(app_is_bt_mode() || app_is_mp3_mode()){
				app_button_sw_action(KeyId);
//				FadeInOut = 198;	// 執行淡出
			}
		}else if(KeyId==BUTTON_BT_PLAY_PAUSE){      //播放|暂停
			if((app_is_mp3_mode()&&player_get_play_status())){      //SD卡模式且在播放
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			}
			if(app_is_bt_mode()==0)	FadeInOut =1;
		}
	}else if(FadeInOut==50){	// Fade In
		IOGainSetDamp(MusMstVol,SysInf.MusicVol,5);
	}else if(FadeInOut==0){     // 原音
		ioGain[MusMstVol]=SysInf.MusicVol;
	}
}

static uint8_t WModeStep =0;
//*****　插U盘会调用　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void user_udisk_ready_callback(void)
{
	if(WModeStep==4){
		system_work_mode_set_button(SYS_WM_UDISK_MODE);
		SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
	}else{
		WModeStep=1;
	}
}


//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{

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
	if(LineIn2R != LineIn){
		if(++LineInDetCont>=20){
			LineInDetCont=0;
			LineIn2R = LineIn;
			if(LineIn2R){
				if(app_is_line_mode()){
					system_work_mode_change_button();
				}
			}else{
				system_work_mode_set_button(SYS_WM_LINEIN2_MODE);
			}
			USER_DBG_INFO("==== AuxInDet %d\n",LineIn2R);
		}
	}else{
		LineInDetCont=0;
	}

	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
		if(WModeStep==2){
			system_work_mode_set_button(SYS_WM_UDISK_MODE);
			SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
			WModeStep=4;
		}
		if(WModeStep==3){
			system_work_mode_set_button(SYS_WM_BT_MODE);
			SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
			WModeStep=4;
		}
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================//系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				gpio_output(LED_BT, 1);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)		gpio_output(LED_BT, 1);
					else				gpio_output(LED_BT, 0);
			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				gpio_output(LED_BT, 1);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)		gpio_output(LED_BT, 1);
					else				gpio_output(LED_BT, 0);
			}

		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}

			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				gpio_output(LED_BT, 1);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)		gpio_output(LED_BT, 1);
					else				gpio_output(LED_BT, 0);
			}

		//==================================================//系统工作模式：AUX模式
//		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE || sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

//				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 0); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}else{
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 1); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}

				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -4;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				gpio_output(LED_BT, 1);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)		gpio_output(LED_BT, 1);
					else				gpio_output(LED_BT, 0);
			}
//			PlayIng = 1;     //播放标志位
		}
	}
}







void Knob_function();
static uint16_t Init_Cont = 0;
static uint8_t LED_SW=0;           //LED切换
//static uint16_t BattVolR =0;
//static uint8_t tee=0;
//********************************
void user_loop_10ms(void)
{
//	if(PowerDownFg)	return;  //ref ZY
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

		//------------------------
		BattFun();

		//-------------------------WMIC
//		BK9532_Main();

//		//----------------------淡入淡出
//		if(FadeInOut)	FadeInOut_Fun();

		//----------------------按键扫描
		user_key_scan();

		static uint8_t led_100ms=0;
		if(++led_100ms >=100){         //30ms刷新RBG灯效
			led_100ms=0;
			USER_DBG_INFO("==================================ACM_PDN:%d\n", gpio_input(ACM_PDN));
			USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
		}
		static uint8_t acm_sw=0;
		static uint8_t decount=0;

		if(gpio_input(ACM_PDN)){      //拔出监听耳机
			decount++;
		}else{
			decount=0;
			acm_sw=1;
		}

		if(decount==50){
			decount=0;
			if(acm_sw)
			{
				extern uint8_t ACM862x_IIC_ADDR[2];
				ACM862x_IIC_ADDR[0]=0x2C;
				os_delay_ms(15);
				ACM8625_init();   			//cjq++    重新打开使能脚 ->>>重新初始化功放
			}
			acm_sw=0;
		}



		//开机后充电 拔掉充电口
		if(gpio_input(PowerKey)==0)
		{
			IOGainSetDamp(MusMstVol, -90,64);
			IOGainSet(Out2L_I2s2L, -90);
			IOGainSet(Out2R_I2s2R, -90);
			os_delay_us(3200);	//1ms
			USER_DBG_INFO("======== 1 =========\n");USER_DBG_INFO("======== 1 =========\n");USER_DBG_INFO("======== 1 =========\n");USER_DBG_INFO("======== 1 =========\n");
//			ACM862x_IIC_ADDR[0]=0;   //
//			USER_DBG_INFO("======== ACM862x_IIC_ADDR[0] =========%X02\n",ACM862x_IIC_ADDR[0]);
//			os_delay_ms(6);
//			ACM8625_init();   			//cjq++    重新打开使能脚 ->>>重新初始化功放
			dsp_shutdown();
			REG_SYSTEM_0x1D &= ~0x2;
			system_wdt_reset_ana_dig(1, 1);
			BK3000_wdt_reset();
			os_delay_us(3200);	//1ms
			USER_DBG_INFO("======== 2 =========\n");
		}


//RGB灯光控制相关
//	#ifdef CONFIG_AUD_REC_AMP_DET
//		void user_aud_amp_task(void);
//		user_aud_amp_task();
//	#endif



//		static uint8_t led_30ms=0;
//		if(++led_30ms >=3){         //30ms刷新RBG灯效
//			led_30ms=0;
//				void RGB_LED_EF_FUN(void);
//				RGB_LED_EF_FUN();
//		}

//		USER_DBG_INFO("====SD:%d:\n", SysMsContsd_is_attached());     //SD卡在位 test
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		if (Init_Cont == 5){
		USER_DBG_INFO("======== 5.DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
		}else if(Init_Cont == 13){
			USER_DBG_INFO("======== 13.DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
//			gpio_output(ACM_PDN, 1);     //功放监听耳机一起控制
			//----------------------电量检测
//			static uint8_t add=0;				//判断是否开机，开机计数1
			if(gpio_input(DcInDet)==0)
				BattFun();
			if(gpio_input(DcInDet)==0 && gpio_input(PowerKey)==0)	 //DC ON 且  开关OFF
			{
				Init_Cont--;
//				add = 1;
			}else{
//				if(add){
//					for(uint8_t i=0;i<=12;i++)
//					{
//						LCD_ShowDot(i,0);
//					}
//					LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);

//					USER_DBG_INFO("======== 1 =========\n");
//					dsp_shutdown();
//					REG_SYSTEM_0x1D &= ~0x2;
//					system_wdt_reset_ana_dig(1, 1);
//					BK3000_wdt_reset();
//					os_delay_us(3200);	//1ms
//					USER_DBG_INFO("======== 2 =========\n");
//				}
			}

//		USER_DBG_INFO("======== DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
//		uint16_t BattVol = BattVolChg(SarAdcVal[6]);
//		if(gpio_input(DcInDet)==0 && gpio_input(PowerKey)==0){      //DC ON 且  开关OFF
//		//充电
//		if(gpio_input(DcInDet)==0){
//			if(ChargFlash==0){        //充电里面的一个功能标志位
//				//电量显示
//
//				if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
//					BattVn=0x0F;
//				}else if(BattVol>=P3Voln){	// 7.5	11.25
//					BattVn=0x07;
//				}else if(BattVol>=P2Voln){	// 7	10.5
//					BattVn=0x03;
//				}else{
//					BattVn=1;
//				}
//				ChargFlash = 1;
//			}
//			else if(ChargFlash && Batt_FULL==0){    //未满电
//				if(((BattVn>>1)&1)==0){
//					BattVn=0x03;
//				}else if(((BattVn>>2)&1)==0){
//					BattVn=0x07;
//				}else if(((BattVn>>3)&1)==0){
//					BattVn=0x0F;
//					ChargFlash = 0;
//				}else{
//					ChargFlash = 0;
//				}
//			}
//			else if(ChargFlash && Batt_FULL){       //满电
//				ChargFlash = 0;
//			}
//			gpio_output(LED1_BAT, (BattVn&1));
//			gpio_output(LED2_BAT, ((BattVn>>1)&1));
//			gpio_output(LED3_BAT, ((BattVn>>2)&1));
//			gpio_output(LED4_BAT, ((BattVn>>3)&1));
//			}
//		}
//		if(gpio_input(DcInDet)==0 && gpio_input(PowerKey)==0)
//		{
//			Init_Cont--;
//		}

		//--------------------------------------------------
		}else if(Init_Cont == 7){	USER_DBG_INFO("======== 7.DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
		}else if(Init_Cont == 8){	USER_DBG_INFO("======== 8.DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
		}else if(Init_Cont == 9){
//			BK9532_Init();       //WMIC初始化

		}else if(Init_Cont == 15){

			ACM8625_init();      //ACM初始化

		}else if(Init_Cont == 16){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 16. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}

		}else if(Init_Cont==18){    //13

			EF_EQ_ReSend();
//			EF_ModeR = EF_Mode;
//			EQ_ModeR = EQ_Mode;

			USER_DBG_INFO("==== 18. EQ_ReSend Ok...MsCont:%d \n",SysMsCont);
			USER_DBG_INFO("======== 18.DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));
		//--------------------------------------------------

		}else if(Init_Cont == 20){
			USER_DBG_INFO("======== 20.DcInDet: %d ========= PowerKey: %d =========\n",gpio_input(DcInDet),gpio_input(PowerKey));

			if(gpio_input(PowerKey)){
							USER_DBG_INFO("====14. Init IoGain...\n",SysMsCont);
							// 需等待 DSP 啟動完成才可開始配置 gain
							IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
							IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
							IOGainSetDamp(Out2L_I2s2L, ioGain[Out2L_I2s2L],64);
							IOGainSetDamp(Out2R_I2s2R, ioGain[Out2R_I2s2R],64);
							IOGainSetDamp(MusL_Out2L, -90,64);
							IOGainSetDamp(MusR_Out2R, -90,64);
							IOGainSetDamp(Play_MusL, -90,64);
							IOGainSetDamp(Play_MusR, -90,64);
							IOGainSetDamp(Adc2_MusL, -90,64);
							IOGainSetDamp(Adc3_MusR, -90,64);
//							gpio_output(POWER_EN, 1);

							gpio_output(LED0_Bat0, 1);
							gpio_output(LED1_Bat1, 1);
							gpio_output(LED2_Bat2, 1);
							gpio_output(LED3_Bat3, 1);
//							if(SysInf.WorkMode==SYS_WM_BT_MODE){
//								RgbLedOneColour(ModeLedAddr,3);	//RgbLedOut(5);	// set power led color
//							}else if(SysInf.WorkMode==SYS_WM_SDCARD_MODE){
//								RgbLedOneColour(ModeLedAddr,4);	//RgbLedOut(6);
//							}else if(SysInf.WorkMode==SYS_WM_LINEIN1_MODE){
//								RgbLedOneColour(ModeLedAddr,2);	//RgbLedOut(7);
//							}

						}else{
							IOGainSetDamp(MusMstVol, -90,64);
							IOGainSet(Out2L_I2s2L, -90);
							IOGainSet(Out2R_I2s2R, -90);
							os_delay_us(3200);	//1ms
							dsp_shutdown();
							REG_SYSTEM_0x1D &= ~0x2;
							system_wdt_reset_ana_dig(1, 1);
							BK3000_wdt_reset();
							os_delay_us(3200);	//1ms
						}



		}else if(Init_Cont == 25){
			USER_DBG_INFO("================= g_mcu_log_on_fg = 0\n");
			g_dsp_log_on_fg = 0;
			g_mcu_log_on_fg = 0;
		}else if(Init_Cont == 26){
			Send_Id1 = CmdPid;
			Send_Id2 = 0x80;
			SendUpComp(SendBuff, 32);	// 請上位機連線
		}else if(Init_Cont == 27){
			g_dsp_log_on_fg = 1;
			g_mcu_log_on_fg = 1;
			USER_DBG_INFO("================= g_mcu_log_on_fg = 1\n");
		}else if(Init_Cont == 28){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;           //一上电设置为蓝牙模式
//			system_work_mode_set_button(app_h->sys_work_mode);
		}else if(Init_Cont == 149){
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
			USER_DBG_INFO("===EQ_Mode:%d  ===EQ_ModeR:%d \n",EQ_Mode,EQ_ModeR);
		}
	}
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
		USER_DBG_INFO("===11EQ_Mode:%d\n",EQ_Mode);
//		app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		switch (EQ_Mode){
		case 0:	USER_DBG_INFO("===22EQ_Mode:%d\n",EQ_Mode);app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	USER_DBG_INFO("===33EQ_Mode:%d\n",EQ_Mode);app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
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
	#define MaxV	3900	// 最大ADC 電壓 (n/4096)
	#define CenV	3400	// 旋鈕中心值電壓
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
	for(uint8_t i=1; i<6; i++){    //2个旋钮。i=1时，SarAdcChVal[1] i=2时，SarAdcChVal[2]，1和2都是旋钮  。 i=0时，SarAdcChVal[0]是按键
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
			if(EqVar>120) EqVar=120;
			if(EqVar<-120) EqVar=-120;
			//====================================================
			switch (i){
			case 1:	// Music Vol
				SysInf.MusicVol = VolVar;
				break;
			case 5:	// Mic Vol
				SysInf.MicVol = VolVar;
				break;
			case 4: // Rec Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = (float)EfVar/20;
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = (float)EfVar/10;
				break;
			case 2: // BASS Vol

				UserEq[MusicEq2].gain = EqVar;
				break;
			case 3: // Tre Vol

				UserEq[MusicEq8].gain = EqVar;
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
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){

		case 1:	// LED ON/OFF
			LED_SW^=1;
			if(LED_SW==1)
				gpio_output(LED_EN2, 0);
			else
				gpio_output(LED_EN2, 1);
			break;
		case 2:	// EQ

			if(++EQ_Mode >= 2u)	EQ_Mode=0;
			USER_DBG_INFO("===EQ_Mode:%d\n",EQ_Mode);
			break;
		case 3:	// 上一首
			app_button_sw_action(BUTTON_BT_PREV);
//			KeyId = BUTTON_BT_PREV;
//			FadeInOut = 200;
			break;
		case 4:	// 下一首
			app_button_sw_action(BUTTON_BT_NEXT);
//			KeyId = BUTTON_BT_NEXT;
//			FadeInOut = 200;
			break;
		case 5:	//播放&暂停
			if(app_is_bt_mode()||app_is_sdcard_mode()||app_is_udisk_mode())
			{
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			}
//			KeyId = BUTTON_BT_PLAY_PAUSE;
//			if(FadeInOut<150)	FadeInOut = 200;
			break;
		case 6:	//模式切换
			system_work_mode_change_button();
//			KeyId = BUTTON_MODE_CHG;
//			FadeInOut = 200;
			break;
		}
		break;
	//====================================================================================================
	case KEY_D_CLICK:	USER_DBG_INFO("KEY_D_CLICK\n");	break;// 短按 2次
	case KEY_T_CLICK:	USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);break;	// 短按 3次
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:    USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
			if(pKeyArray->index==5 && sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			}
		break;
	case KEY_L_PRESSING:	// 常按 循環

		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	USER_DBG_INFO("MusicVol %d\n", SysInf.MusicVol); break;	// 常按放開

	default:break;
	}
}



//********************************
#ifdef PromptToneMusicMute
void PlayWaveStart(uint16_t id)
{
	USER_DBG_INFO("wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);      //Music Off

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
//		app_handle_t sys_hdl = app_get_sys_handler();

//		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
//			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
//		}


//		SysInf.WorkMode = sys_hdl->sys_work_mode;  //模式
//		SysInf.VoiceCanEn = 0;	// 關閉消原音
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

	case APP_WAVE_FILE_ID_START:
		gpio_output(LED_EN2, 1);
		if(WModeStep==1)	WModeStep =2;//UDISK
		if(WModeStep==0)	WModeStep =3;//BT
		break;
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");

//		gpio_output(ACM_PDN, 0);	//AMP_CRTL

//		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		app_powerdown();
//		gpio_output(POWER_EN, 0);	///POWER_EN
		os_delay_us(3200);	//1ms
		//app_button_sw_action(BUTTON_BT_POWERDOWN);
		PowerDownFg=1;
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
//		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
//			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
//		}
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


