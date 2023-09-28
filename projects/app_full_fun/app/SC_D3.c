/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2023-04-20
* 描述：

**********************************************************************/
#include "USER_Config.h"
#ifdef SC_D3

#include "SC_D3_DEF.H"

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
//extern uint8 appPlayerPlayMode;


static uint16_t WorkModeCont=0;


//static uint16_t PowerDownCont=0;	// 沒聲音 30分 關機計時 (S)

//static uint8_t DisplayCont=DisplayCont_def;	// 充電燈顯示計時 (S)
//static uint8_t	ChargFlash=0;
uint16_t power_off=2800;
//uint16_t power_off=1500;

//uint8_t work_modeR=10;

uint8_t LineInR=3;
uint8_t LineIn2R=0;
uint8_t EfVarR=0;

static uint16_t SysMsCont=0;


uint8_t Mic_ValR=255;
uint16_t WMicOffCont=0;

const uint8_t LED_Xx[4][5]={
	{0x05,0x25,0x45,0x65,0x85},
	{0xA5,0xC5,0xE4,0xE6,0xC6},
	{0xA6,0x86,0x66,0x46,0x26},
	{0x06,0x00,0x00,0x00,0x00}};

const uint8_t LED_KEY[10][2]={
		{0x30,0x27},{0x16,0x15},{0x14,0x13},{0x12,0x11},{0x10,0x07},
		{0x32,0x31},{0x33,0x34},{0x35,0x36},{0x00,0x00},{0x00,0x00},
};


const uint8_t S_Xx[4][7]={	{0x31,0x26,0x25,0x27,0xE4,0xE5,0x30},	//1~4位數字指標  a~g
							{0x45,0x51,0x50,0x47,0x24,0x44,0x46},
							{0x65,0x86,0x85,0x84,0x67,0x66,0x64},
							{0xB1,0xA5,0xA6,0xA7,0x90,0x91,0xB0}};
const uint8_t S_Nx[19]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x27,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,0x3E,0x00,0x78};	// N0~9,A~F,U,NC 顯示位
uint32_t Key_N,Key_NR;		//存储按键值
uint32_t Key2_N,Key2_NR;	//存储按键值




void LCD_ShowClear(void)
{
	for(uint16 i=0;i<=255;i++)
	{
		UpDisplayReg(i,0);
	}
	UpDisplayAll();
}
void LCD_ShowAll(void)
{
	for(uint16 i=0;i<255 ;i++)
	{
		UpDisplayReg(i,1);
	}
	UpDisplayAll();
}

void LCD_KeyDot(uint8_t n, uint8_t OnOff)
{
	UpDisplayReg(LED_KEY[n][0], OnOff);
	UpDisplayReg(LED_KEY[n][1], OnOff);
}
void LCD_KeyDotClear(void)
{
	for(uint8 n=0;n<10;n++)
	{
		UpDisplayReg(LED_KEY[n][0], 0);
		UpDisplayReg(LED_KEY[n][1], 0);
	}
}

void LCD_ShowDot(uint8_t n, uint8_t OnOff)
{
	UpDisplayReg(LED_Xx[n][0], OnOff);
	UpDisplayReg(LED_KEY[n][1], OnOff);
}
//**********************************************
void LCD_ShowNum(uint8_t Xx,uint8_t Nx)
{
	for(uint8_t i=0; i<7; i++){
		if((S_Nx[Nx]>>i)&1){
			UpDisplayReg(S_Xx[Xx][i], 1);
		//	StbFg[(S_Xx[Xx][i]>>4)&0xF] |= (1<<(S_Xx[Xx][i]&0xF));
		}else{
			UpDisplayReg(S_Xx[Xx][i], 0);
		//	StbFg[(S_Xx[Xx][i]>>4)&0xF] &= ~(1<<(S_Xx[Xx][i]&0xF));
		}
	}
}

extern uint8_t DispBuff[16];
//**********************************************
void LCD_ShowNum1(uint8_t Xx,uint8_t Nx)
{
	uint8_t rt;
	for(uint8_t i=0; i<7; i++){
		rt = ((i*2)<<4) | Xx;
		os_printf("%02X ",rt);
		if((S_Nx[Nx]>>i)&1){
			UpDisplayReg(rt, 1);
		}else{
			UpDisplayReg(rt, 0);
		}
	}
	USER_DBG_INFO(" \n%02X %02X %02X %02X %02X %02X %02X \n",DispBuff[0],DispBuff[2],DispBuff[4],DispBuff[6],DispBuff[8],DispBuff[10],DispBuff[12]);
}


//****************************************

void LCD_ShowVol(uint8_t vol)
{
		LCD_ShowNum(0,17);
		LCD_ShowNum(1,16);
		LCD_ShowNum(2,vol/10);
		LCD_ShowNum(3,vol%10);

		LCD_ShowNum(0,17);
		LCD_ShowNum(1,16);
		LCD_ShowNum(2,0);
		LCD_ShowNum(3,0);
	UpDisplayAll();
}

//****************************************
void Knob_function();

//****************************************


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


//void Recharge(float vol);			//充电显示

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
		BattVolDetMs = 0;
		//****25.2v, 270k,21k(1.819v)/4096=0.444, 25200/1819=13.854 19.2v警告, 18v關機  ******检测电池电量
		uint16_t BattVol = BattVolConvert(KnobVal[15]);
		static uint16_t saradc_tempe_dataR;
		extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
		static uint8_t DC_IN_DET;
		if(KnobVal[13]>1000 || KnobVal[8]>1000)	DC_IN_DET=1;
			else								DC_IN_DET=0;

		fl2hex V1;
		if((abs(KnobValR[15]-KnobVal[15])>10) || (saradc_tempe_dataR != saradc_tempe_data)){
			KnobValR[15] =KnobVal[15];
//			USER_DBG_INFO("====KnobVal[15]:%d  KnobVal[13]:%d  KnobVal[8]:%d  \n",KnobVal[15], KnobVal[13],KnobVal[8]);
			saradc_tempe_dataR = saradc_tempe_data;
			USER_DBG_INFO("====SarAdcVal: %d  %5.0f\n", KnobVal[15], BattVol);//[USER]====SarAdcVal: 3046  7844

			Send_Id1 = MainPid;
			Send_Id2 = 205;
			Send_Group = 1;
			V1.f = (float)BattVol/1000;
			Send_Val1 = V1.h;
			V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
			Send_Val2 = V1.h;
			Send_Val41 = DC_IN_DET;
			Send_Val42 = Batt_FULL;
			SendUpComp(SendBuff, 32);
		}
		if(DisplayCont || DC_IN_DET){	//判断是否为充电状态
			USER_DBG_INFO("==== BATT ADC:%d  %d mv  Batt_FULL:%d\n",KnobVal[15], BattVol ,Batt_FULL);
		//	if(DisplayCont)	DisplayCont--;
			if(ChargFlash==0){

			if(BattVol>=fullVn)	Batt_FULL = 1;	// 8.375V 滿電
				else Batt_FULL =0;
			//	Batt_FULL = !gpio_input(ChargFull);
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
			}else if(ChargFlash && Batt_FULL==0 && DC_IN_DET){
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
//			USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[1]:%d\n",BattVn,SarAdcVal[1]);
			if(DisplayCont || Batt_FULL==0){	// 顯示 或 未滿電
//				LCD_ShowDot(1, (BattVn&1));
//				LCD_ShowDot(2, ((BattVn>>1)&1));
//				LCD_ShowDot(3, ((BattVn>>2)&1));
//				LCD_ShowDot(4, ((BattVn>>3)&1));
			}else if(DC_IN_DET && Batt_FULL){
//				LCD_ShowDot(1, 1);
//				LCD_ShowDot(2, 1);
//				LCD_ShowDot(3, 1);
//				LCD_ShowDot(4, 1);
			}else{
//				LCD_ShowDot(1, 0);
//				LCD_ShowDot(2, 0);
//				LCD_ShowDot(3, 0);
//				LCD_ShowDot(4, 0);
			}
			UpDisplayAll();
		}else{
//			static uint8_t LowPowerFlash=1;
			if(BattVol<LoPowerENv){	// 6.5v	9.75
//				LCD_ShowDot(1, LowPowerFlash^=1);
			}else{
//				LCD_ShowDot(1, 0);
//				LCD_ShowDot(2, 0);
//				LCD_ShowDot(3, 0);
//				LCD_ShowDot(4, 0);
			}
			UpDisplayAll();
		}
		//==== 判斷進入 LowPower Mode =======================
		if(LowPowerEn==0 && BattVol<LoPowerENv){
			if(++LowPowerCont > 5){
				LowPowerEn=1;
				DelayOffCont=0;
				OffPowerCont = 0;
				USER_DBG_INFO("==== LowPower ModeT ADC:%d  %d\n",SarAdcVal[1],BattVol);
			}
		}else{
			LowPowerCont=0;
			if(LowPowerEn && SarAdcVal[1]>=P2Voln){	// 低電解除
				LowPowerEn=0;
				USER_DBG_INFO("==== UnLowPower ModeT ADC:%d  %d\n",SarAdcVal[1],BattVol);
			}
		}
		//===============================================================
		if(LowPowerEn){	// 低電 6.5V && 不充電
	//		USER_DBG_INFO("==== SarAdcValR[1]<2700:%d\n",SarAdcVal[1]);
			if(++DelayOffCont > 300){	// 5s
				DelayOffCont=0;
				DisplayCont = DisplayCont_def;
				if(DC_IN_DET==0) app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 充電中不報低電提示音
			}
			if(BattVol<LoPowerOFFv){	// 6v	9
				USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[1],BattVol, OffPowerCont);
				if(++OffPowerCont==6){
					app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
				}else if(OffPowerCont>7){
//					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
				}
			}else{
				OffPowerCont = 0;
			}
		}else{
			DelayOffCont=0;
		}
	}	//end 1s
}


//**************************************
uint8 AMP_En=1;
uint8 Mic_En=0;
uint8 ii=0;
uint16 adda=0x00;
static uint8_t tone = 0;
static uint8_t keycount = 0;               //按键计数器
//static uint8_t Out_indoor = 0;
void TM16xx_main()
{
	Key_N = read_key();	//读按键值
//	USER_DBG_INFO("====Key_N: %d \n",keycount);
	//====================================
//	app_handle_t sys_hdl = app_get_sys_handler();

	if(Key_NR != Key_N){
		keycount++;
	}else{                    //未按
				keycount=0;
			}
//	USER_DBG_INFO("====keycount: %d \n",keycount);
	if(Key_NR != Key_N){

//		if(Key_NR) UpDisplayReg(LedFg, 0);
		Key_NR = Key_N;
//		if(Key_N) USER_DBG_INFO("====read_key: 0x%08X \n",Key_N);
		//==== 按键处理 =================
		if(Key_N){

			switch (Key_N){
			case 0x00000004 :	// 下一曲
			//	LCD_ShowClear();
				UpDisplayOnOff(0xFE);

				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);
				if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
					app_button_sw_action(BUTTON_BT_NEXT);
				}
				break;
			case 0x00000008 :	// 静音
				LED_TEST(1);
				adda=adda+0x01;
//				UpDisplayReg(adda,1);
//				UpDisplayLCD();
				USER_DBG_INFO("====adda====: 0x%x \n",adda);

				if(AMP_En)
				{
//					LCD_KeyDot(4, 1);
					ioGain[Out2L_I2s2L] = -90;
					ioGain[Out2R_I2s2R] = -90;
					AMP_En=0;
//					UpDisplayReg(0x3F,1);
				}
				else
				{
//					LCD_KeyDot(4, 0);
					ioGain[Out2L_I2s2L] = 0;
					ioGain[Out2R_I2s2R] = 0;
					AMP_En=1;
//					UpDisplayReg(0x3F,0);
				}
				break;
			case 0x00000040 :	// 暂停
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);

				break;
			case 0x00000080 :	// 咪开关
				LED_TEST(0);
				if(Mic_En)
				{
//					LCD_KeyDot(3, 0);
					ioGain[I2s2L_Mic] = 0;//输出无线麦音量
					ioGain[I2s2R_Mic] = 0;//输出无线麦音量
					Mic_En=0;
//					UpDisplayReg(0x3F, 1);
//					UpDisplayReg(0x31, 0);
				}
				else
				{
//					LCD_KeyDot(3, 1);
					ioGain[I2s2L_Mic] = -90;//输出无线麦音量
					ioGain[I2s2R_Mic] = -90;//输出无线麦音量
					Mic_En=1;
//					UpDisplayReg(0x32, 0);
//					UpDisplayReg(0x3F, 1);
				}
				break;
			case 0x00000400 :	// 上一曲
//				LCD_ShowAll();
				LCD_ShowNum1(0,0);
				LCD_ShowNum1(1,18);
				LCD_ShowNum1(2,18);
				LCD_ShowNum1(3,18);
				LCD_ShowNum1(4,18);

				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);
				if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
								app_button_sw_action(BUTTON_BT_PREV);
							}
				break;
			case 0x00000800 :	// 话筒优先
				app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
				if(SysInf.DuckDet){
					SysInf.DuckDet = 0;
					LCD_KeyDot(2, 0);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					SysInf.DuckDet = 0x18;
					LCD_KeyDot(2, 1);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
			case 0x00004000 :	// 室內/室外
				if(++EQ_Mode > 1u)
					{
						EQ_Mode = 0u;
						LCD_KeyDot(6, 0);
					}
				else LCD_KeyDot(6, 1);
				break;
			case 0x00008000 :	// 消原音
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
				if(SysInf.VoiceCanEn){
					SysInf.VoiceCanEn = 0;
					LCD_KeyDot(1, 0);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					SysInf.VoiceCanEn = 1;
					LCD_KeyDot(1, 1);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
			case 0x00040000 ://模式
				if(keycount==0)
				{
					LCD_KeyDotClear();
				}
				system_work_mode_change_button();
				break;
			case 0x00080000 ://话筒模式
				if(tone==0){
					LCD_KeyDot(0, 1);
				tone=1;	//UserEf.Pitch = +12;	// 男變女
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);
			}else if(tone==1){
				tone=2;	//UserEf.Pitch = -12;	// 女變男
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);
			}else if(tone==2){
				tone=3;	//UserEf.Pitch = +8;	// 娃娃音
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
			}else if(tone==3){
				tone=4;	//UserEf.Pitch = -8;	// 魔音
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);
			}else{
				tone=0;	//UserEf.Pitch = 0;	// 原音
				LCD_KeyDot(0, 0);
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);
			}

				break;
				break;
			}

		}else{
//			UpDisplayReg(LedFg, 1);
		}
		UpDisplayAll();		// 更新显示
	}

}




//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{

	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
		for(int i=0;i<16;i++)
		{
			if(i/2==0)
			{
				BattFun();
			}
//			USER_DBG_INFO("====KnobValR[%d]: %d  \n",i,KnobVal[i]);
		}
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态

		app_handle_t sys_hdl = app_get_sys_handler();
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				gpio_output(LED0, 0);
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= -4;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
//				gpio_output(LED0, 0);
			}else{
//				gpio_output(LED0, BtLedFlash^=1);
			}
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				RgbLedOut(2);	// W
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= 0;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= -90;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			//-----------------------------------
		//=====================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				//	if(ioGain[Play_MusL] < -15){
					ioGain[Play_MusL]= -5;
					ioGain[Play_MusR]= ioGain[Play_MusL];
					ioGain[I2s3L_MusL]= -90;
					ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
		//==================================================
		}else if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
//				RgbLedOut(3);	// G
			//	if(ioGain[Adc2_MusL] < - 15){
				ioGain[Play_MusL]= 0;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= -90;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
//				PlayIng =1;
			}
		}
	}
}



//**** pre init ***********************
void user_init_prv(void)
{
	//=============================	GPIO配置
	#define SGM_MUTE	GPIO35		//AMP升压
	gpio_config_new(SGM_MUTE, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(SGM_MUTE, 1);

	#define MCLK	GPIO24		//AUDIO
	gpio_config_new(MCLK, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(MCLK, 1);

	#define POWER_EN	GPIO22	//开关机自锁
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);


	#define LED_DIN	GPIO21			//LED
	gpio_config_new(LED_DIN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LED_DIN, 1);

	#define CE	GPIO13			//麦克
	gpio_config_new(CE, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(CE, 1);

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數

}


//********************************
void user_init_post(void)
{
	Hi2c_Init();

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
	if(++Tick_2ms>=2){
		Tick_2ms = 0;
		user_saradc_update_trig();	// Adc 讀取觸發
	}
	SysMsCont++;
}
#endif

static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
		//----------------------------
		if(EF_Maim()) return;

		//======================
		if(ACM_main()) return;


		//----------------------控制LCD显示和音源链路音量设置
		user_WorkModefun();
		//----------------------编码器旋钮
		Knob_function();
		//-----------------------电池显示
//		BattFun();
		//-------------------------
		BK9532_Main();
//		USER_DBG_INFO("====KnobVal[15]:%d  KnobVal[13]:%d  KnobVal[8]:%d  \n",KnobVal[15], KnobVal[13],KnobVal[8]);

/*
	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif
*/

		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時
			//----- 沒聲音偵測  15分 關機 ------------------------
//			extern uint32_t user_aud_amp_task(void);
//			if(user_aud_amp_task()){
//				PowerDownCont=0;
//			}else{
//				if(++PowerDownCont >=900){
//						app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
//				}
//				if(PowerDownCont%60==0) USER_DBG_INFO("==== DelayPoOffCont:%d  Vol:%d\n", PowerDownCont, user_aud_amp_task());
//2			}
			ms1000 = 0;
//			ACM_REPORT();
		}
		//----------------------按键扫描
		user_key_scan();
		TM16xx_main();


		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);

	//================================================================
	}else{
		Init_Cont++;
		if(Init_Cont == 1){
			TM16xx_Init(1);
		if(Init_Cont == 6){
			if(KnobVal[8]<power_off&&gpio_input(GPIO15)){
								USER_DBG_INFO("====KnobVal[15]:%d  KnobVal[13]:%d  KnobVal[8]:%d  \n",KnobVal[15], KnobVal[13],KnobVal[8]);
								Init_Cont--;
							}
		}
		//--------------------------------------------------


		}else if(Init_Cont==6){
			//----------------------电量检测
			BattFun();
		}else if(Init_Cont == 10){
			gpio_output(SGM_MUTE, 1);
			//----------------------编码器旋钮
			Knob_function();
		}else if(Init_Cont == 11){
			BK9532_Init();
#ifdef ACM86xxId1
	ACM8625_init();
}else if(Init_Cont == 51){
		if(ACM_main()){
			Init_Cont--;
		}else{
			USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
		}
}else if(Init_Cont == 52 && ACM862x_IIC_ADDR[1]){    //第二颗ACM初始化
ACM862xWId = 1;
SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
if(ACM_main()){
	Init_Cont--;
	    }else{
	    	ACM862xWId = 0;
	    	SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
	    	USER_DBG_INFO("==== 12. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
	    }
#endif
		}else if(Init_Cont==53){
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

		}else if(Init_Cont == 54){

//			BK9532_Init();

		}else if(Init_Cont == 55){
			EF_EQ_ReSend();
//			USER_DBG_INFO("===BAT_VOL===:%d:\n", SarAdcVal[3]);  //电池电量查看
		}else if(Init_Cont == 56){
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音
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


//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
//	if()
	extern void LCD_ShowVol(uint8_t vol);



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
	#define CenV	1900	// 旋鈕中心值電壓
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
		if(i>7) i++;
		if(i==11||i==9) break;

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
				if(VolVar <=-57) VolVar = -90;
				KeyVar =(KnobValR[i]/KeyDn6)-6;
				EqVar = (KnobValR[i]/EqDn12)-120;
				EfVar =  (KnobValR[i]/EfDn10);
			}
			//====================================================

			switch (i){

			case 6:	// 混响
				UserEf.RevVol = (float)EfVar/40;
				UserEf.RevVol2 = (float)EfVar/40;
				break;
			case 3:	// Music Vol
//				SysInf.MusicVol = VolVar;
//				LCD_ShowVol((float)(SysInf.MusicVol+60.95)/2.185);
				ioGain[MusR_Out2R] = VolVar;//输出总音量
				ioGain[MusL_Out2L] = VolVar;//输出总音量


				ioGain[Tone_DacLR] = (VolVar/2)-20;//提示音
//				ioGain[Tone_DacLR] = (VolVar-20)/2;//提示音

				break;
			case 1:	// Music Bass
				UserEq[MusicEq2].gain = EqVar/2;
				break;

			case 0:	// Music Tre
				UserEq[MusicEq8].gain = EqVar/2;
				break;
			//---------------------------------------------
			case 7:	// Mic Vol
				SysInf.MicVol = VolVar;


				break;
			case 5:	// Mic Bass
				UserEq[MicEq4_2].gain = EqVar/2;
				UserEq[MicEq3_2].gain = EqVar/2;
				break;
			case 4:	// Mic Tre
				UserEq[MicEq4_7].gain = EqVar/2;
				UserEq[MicEq3_7].gain = EqVar/2;
				break;
			//--------------------------------------
			case 2:	// ECHO2 Vol
				UserEf.EchoVol = (float)EfVar/30;
				UserEf.EchoVol2 = (float)EfVar/30;

				break;
			}

			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], KnobVal[i], VolVar, KeyVar, EqVar, EfVar);
			KnobValR[i]=KnobVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
}

//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 0:	// 模式切換
			if(sys_hdl->sys_work_mode==SYS_WM_BT_MODE && sd_is_attached()==0 && udisk_is_attached()==0)	return;
			system_work_mode_change_button();
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
		case 0:
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
//			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->index==0 && pKeyArray->keepTime > 100){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
//			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
		}else if(pKeyArray->keepTime==800){
			SysInf.Lang ^=1;
			if(SysInf.Lang){
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);
			}else{
				app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);
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
uint16_t TimeVol=0;
void PlayWaveStop(uint16_t id)
{
	//app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:

		dsp_shutdown();
//		gpio_output(POWER_EN, 0);
		app_powerdown();
	//	app_button_sw_action(BUTTON_BT_POWERDOWN);
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		break;
	case APP_WAVE_FILE_ID_CONN:
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
	case APP_WAVE_FILE_ID_RESERVED0:	// SD Card
		if(player_get_play_status()==0){		// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
		break;
	case APP_WAVE_FILE_ID_MP3_MODE:		// USB DISK
		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
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


