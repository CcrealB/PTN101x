/*
 * XWX_S6616.c
 *
 *  Created on: 2023年6月17日
 *      Author: 11491
 */


/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2023-04-20
* 描述：

**********************************************************************/
#include "USER_Config.h"
#ifdef COK_S8

#include "PTN1012_DEMO_DEF.H"


uint8_t LineInR=0;

static uint16_t SysMsCont=0;





//==== JW-24017-1B 带点 共楊 =======================================================================
const uint8_t LCD_PIN[7]={GPIO13, GPIO15, GPIO24, GPIO11, GPIO32, GPIO31, GPIO23};	// P1~7 GPIO
const uint8_t S_Xx[4][7]={	{0x01,0x02,0x30,0x40,0x03,0x10,0x20},	//1~4位 的 a~g段
							{0x12,0x13,0x41,0x15,0x14,0x21,0x31},
							{0x43,0x24,0x34,0x50,0x52,0x32,0x42},
							{0x65,0x56,0x45,0x53,0x35,0x54,0x46}};
const uint8_t S_Nx[19]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7C,0x27,0x7F,0x67,0x77,0x7D,0x39,0x5E,0x79,0x71,0x3E,0x00,0x40};	// N0~9,A~F,U,NC,- 段码
const uint8_t S_Dotx[10]={0x05,0x51,0x25,0x04,0x62,0x06,0x16,0x36,23,64};	// BT, SD, USB, REC, REP, B1,B1,B3,:, . // K1~K8段码

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
    K1_BT,
    K3_SD,
    K2_USB,
    K4_REC,
    K6_REP,
    A5_B1,
	A6_B2,
	A7_B3,
	K5_DOT2,
	K8_DOT,
};
//****************************************
void LCD_ShowDot(uint8_t n, uint8_t OnOff)
{
	if(n>9)	n=9;
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
////关于 充电
//#define DisplayCont_def	3		// 充電燈顯示延時(S)
//uint8_t DisplayCont = 1;
//float Battery_Cont=0;
//uint16_t Battery_ContR=1;
//static uint8_t	ChargFlash=0;
//static uint8_t Batt_FULL=0;
//static uint16_t BattVn = 0x01;
//static uint8_t LowPowerEn=0;
//static uint8_t LowPowerCont=0;		// 進入低功耗計數
//static uint16_t DelayOffCont=0;
//static uint8_t OffPowerCont = 0;
//
#define DcInDet			GPIO30      //充电检测
//#define LED3_Bat3		GPIO6
//#define LED2_Bat2		GPIO30
//#define LED1_Bat1		GPIO3
//#define LED0_Bat0		GPIO28
//
//#define	fullVn			12600	// 滿電壓 mv
//#define	P4Voln			11700	// 亮4個燈電壓
//#define	P3Voln			10800	// 亮3個燈電壓
//#define	P2Voln			9900	// 亮2個燈及解除低電模式電壓
//#define	LoPowerENv		9400	// 低電模式電壓
//#define	LoPowerOFFv	 	9000	// 低電關機電壓
////***************************************************
//uint16_t BattVolConvert(uint16_t AdcVol)
//{
//	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻(BattHR), 對地電阻 (BattLR)
//	uint16_t AdcVol1 = SarADC_val_cali_get(AdcVol);	// 校正轉換
//	uint16_t BattVol = (float)saradc_cvt_to_voltage(AdcVol1)/BattRv;
//	USER_DBG_INFO("==== AdcVol %d  %d  %d\n",AdcVol, AdcVol1, BattVol);
//	return  BattVol;
//}
//
////*************************************************
//void BattFun(void)
//{
//	//--------------------------------
//	static uint16_t BattVolDetMs = 99;
////	static uint16_t LowBattCont = 0;
//	if(++BattVolDetMs == 100){
//		BattVolDetMs=0;
//
//	//===================================================================================
//	uint16_t BattVol = BattVolConvert(SarAdcVal[6]);
//	static uint16_t saradc_tempe_dataR;
//	static uint8_t DcInDetR,Batt_FULLR;
//	extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
//	//--------------------------------
//	fl2hex V1;
//	if((abs(SarAdcValR[6]-SarAdcVal[6])>10) || (saradc_tempe_dataR != saradc_tempe_data || 	DcInDetR != gpio_input(DcInDet) || Batt_FULLR != Batt_FULL)){
//		SarAdcValR[6] =SarAdcVal[6];
//		saradc_tempe_dataR = saradc_tempe_data;
//		DcInDetR = gpio_input(DcInDet);
//		Batt_FULLR = Batt_FULL;
//		Send_Id1 = MainPid;
//		Send_Id2 = 205;
//		Send_Group = 1;
//		V1.f = (float)BattVol/1000;
//		Send_Val1 = V1.h;
//		V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
//		Send_Val2 = V1.h;
//		Send_Val41 = gpio_input(DcInDet);
//		Send_Val42 = Batt_FULL;
//		SendUpComp(SendBuff, 32);
//	}
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
//		static uint8_t LowPowerFlash=1;
//		if(BattVol<LoPowerENv){	// 6.5v	9.75
//			gpio_output(LED0_Bat0, LowPowerFlash^=1);
//		}else{
//			gpio_output(LED0_Bat0, 0);
//			gpio_output(LED1_Bat1, 0);
//			gpio_output(LED2_Bat2, 0);
//			gpio_output(LED3_Bat3, 0);
//		}
//	}
//
//	//==== 判斷進入 LowPower Mode =======================
//	if(LowPowerEn==0 && BattVol<LoPowerENv){		// 6.5v		9.75
//		if(++LowPowerCont > 5){
//			LowPowerEn=1;
//			DelayOffCont=0;
//			OffPowerCont = 0;
//			USER_DBG_INFO("==== LowPower ModeT ADC:%d  %d\n",SarAdcVal[6],BattVol);
//		}
//	}else{
//		LowPowerCont=0;
//		if(LowPowerEn && SarAdcVal[6]>=P2Voln){	// 低電解除
//			LowPowerEn=0;
//			USER_DBG_INFO("==== UnLowPower ModeT ADC:%d  %d\n",SarAdcVal[6],BattVol);
//		}
//	}
//
//	//===============================================================
//	if(LowPowerEn){	// 低電
//		if(++DelayOffCont > 300){	// 5s
//			DelayOffCont=0;
//			DisplayCont = DisplayCont_def;
//			if(!(gpio_input(DcInDet)==0))	app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 充電中不報低電提示音
//		}
//		if(BattVol<LoPowerOFFv){	// 6v
//			USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[6],BattVol, OffPowerCont);
//			if(++OffPowerCont==6){
//				app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
//			}else if(OffPowerCont>7){
//				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
//			}
//		}else{
//			OffPowerCont = 0;
//		}
//	}else{
//		DelayOffCont=0;
//	}
//
//	}
//}

static uint8_t	BtLedFlash=0;
static uint16_t WorkModeCont=0;
//************* 控制LCD显示和音源链路音量设置
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
//				LCD_ShowDot(K1_BT, 1);
//				LCD_ShowDot(K3_SD, 0);
//				LCD_ShowDot(K2_USB, 0);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				LCD_ShowDot(K1_BT, 1);
			}else{
				if(BtLedFlash^=1)	LCD_ShowDot(K1_BT, 1);
					else			LCD_ShowDot(K1_BT, 0);
			}

		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				LCD_ShowDot(K1_BT, 0);
//				LCD_ShowDot(K3_SD, 1);
//				LCD_ShowDot(K2_USB, 0);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}

		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				LCD_ShowDot(K1_BT, 0);
//				LCD_ShowDot(K3_SD, 0);
//				LCD_ShowDot(K2_USB, 1);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}

		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				LCD_ShowDot(K1_BT, 0);
//				LCD_ShowDot(K3_SD, 0);
//				LCD_ShowDot(K2_USB, 0);
			//	if(ioGain[Adc2_MusL] < - 15){
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= 0;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
//				PlayIng =1;
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
	UserFlash_Init();	// 設置FLASH
	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;


#if LED_RGB_NUM
	app_i2s0_init(1);
#endif
}


//********************************
void user_init_post(void)
{
	Hi2c_Init();
    //=============================按键初始化
//    user_key_init(key_function);	// 移至底層配置
    //---------------------旋钮初始化
//	u_SaradcKbon_Init();			// 移至底層配置
	//=============================	GPIO配置
	#define POWER_EN	GPIO27     //电源使能          高开低关
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);

	#define WM_PO_EN	GPIO12	//48v麦克使能    高开低关
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 1);

	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
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
	LCD_scan();
	//------------------------------
	static uint8_t Tick_2ms =0;
	if(++Tick_2ms>=2){
		Tick_2ms = 0;
		user_saradc_update_trig();	// Adc 讀取觸發
	}
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

		//======================

		//---------------------- 控制LCD显示和音源链路音量设置
		user_WorkModefun();

		//---------------------- 编码器旋钮
//		Knob_function();

		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;

		}

		//----------------------按键扫描
		user_key_scan();

		if(EF_Maim()) return;
		if(SysMsCont>10) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);


	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 10){



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

			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音
		//--------------------------------------------------

		}else if(Init_Cont == 15){

		}else if(Init_Cont == 20){
			EF_EQ_ReSend();
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
#ifdef	CONFIG_AUD_AMP_DET_FFT
			extern void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
			aud_mag_det_enable(1);	//audio magnitude detect enable
#endif

//			TM16xx_Init(2);
		}

	}
}


//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
//	if()




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
		switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);	break;
		}
		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
	}else{
		play_en =1;
	}
}

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
				break;
	#endif
			case 2:	// Mic Vol
				SysInf.MicVol = VolVar;
				break;
			case 3:	// REV Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = UserEf.RevVol;
	#ifdef COK_S8
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


//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
	//return;
//	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;
	case KEY_S_REL:
		switch (pKeyArray->index){
		case 0:	// 模式切換
//			system_work_mode_change_button();
//			LCD_ShowNum(0,17);
//			LCD_ShowNum(1,17);
//			LCD_ShowNum(3,17);
//			LCD_ShowNum(2,17);
//			add1=0;
//			add2=0;
//			add3=0;
//			add4=0;
			break;
		case 1:	// 光效切换
//			LCD_ShowDot(tt,1);
//			if(tt) LCD_ShowDot(tt-1,0);
//			if(++tt>9){
//				LCD_ShowDot(tt-1,0);
//				LCD_ShowDot(0,1);
//			}

			break;
		case 2:	// 均衡
//			if(++EQ_Mode > 2u)	EQ_Mode = 0u;
			break;
		case 3:	// 話筒優先
//			app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
//			if(SysInf.DuckDet==0){
//				SysInf.DuckDet = 0x18;
//				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
//			}else{
//				SysInf.DuckDet = 0;
//				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
//			}
			break;
		case 4:	// 录制
//			user_RecWork();
			break;
		case 5:	// 上一首
//			if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
//				app_button_sw_action(BUTTON_BT_PREV);
//			}
			break;
		case 6:	// 播放/停止
//			user_RecPlayWork();
			break;
		case 7:	// 下一首
//			if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
//				app_button_sw_action(BUTTON_BT_NEXT);
//			}
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
		case 0:	// MODE
//			GroupWrite(0, 2);		// 保存播放記憶點
//
//			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
//				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
//			}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
//				if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ALL_CYCLE){
//					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ONE;
//					LCD_ShowDot(K6_REP, 1);
//					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
//				}else{
//					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ALL_CYCLE;
//					LCD_ShowDot(K6_REP, 0);
//					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8);
//				}
//				SysInf.SdCardPlayMode = appPlayerPlayMode;
//			}
			break;
		case 1:	// RGB LED OFF

			break;
		case 5:	// PRE
//			app_player_button_prevDir();
			break;
		case 7:	// NEXT
//			app_player_button_nextDir();
			break;
		}
		break;

	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->keepTime > 100){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;
			switch (pKeyArray->index){
			case 3:
				break;
			case 4:
				break;
			}
		}
		break;
	case KEY_L_REL:
		USER_DBG_INFO("KEY_L_REL\n");	// 常按放開
		break;
	default:break;
	}
}



//********************************
#ifdef PromptToneMusicMute
void PlayWaveStart(uint16_t id)
{
	USER_DBG_INFO("wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);            //Music off
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
//		while(gpio_input(GPIO15)==0);
		dsp_shutdown();
		app_powerdown();

//		gpio_output(AMP_CRTL, 0);	//AMP_CRTL
//		gpio_output(POWER_EN, 0);	///电源按键高使能

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		GroupWrite(0, 2);		// 保存播放記憶點

		break;
	case APP_WAVE_FILE_ID_CONN:
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
	case APP_WAVE_FILE_ID_MP3_MODE:		// UDISK
	case APP_WAVE_FILE_ID_RESERVED0:	// SD
//		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
//			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
//		}
		break;
	case APP_WAVE_FILE_ID_BT_MODE:
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


