/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2023-04-20
* 描述：

**********************************************************************/
#include "USER_Config.h"
#ifdef SG_P60

#include "SG_P60_DEF.H"

uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
//extern uint8 appPlayerPlayMode;
extern void LCD_ShowMode(uint8_t mode);

static uint16_t WorkModeCont=0;
static uint16_t Check_ChargeCont=0;

static uint8_t	BatteryLedFlash=0;	//低电闪烁
static uint8_t	BtLedFlash=0;		//蓝牙闪烁
static uint8_t	Modevalu=0;			//模式储存
static uint8_t	Count=4;			//显示屏读秒
static uint8_t	CountR=4;			//跑马灯计数
//static uint16_t PowerDownCont=0;	// 沒聲音 30分 關機計時 (S)
#define DisplayCont_def	3		// 充電燈顯示延時(S)
//static uint8_t DisplayCont=DisplayCont_def;	// 充電燈顯示計時 (S)
//static uint8_t	ChargFlash=0;



//uint8_t work_modeR=10;

uint8_t LineInR=0;
uint8_t LineIn2R=0;
uint8_t EfVarR=0;

static uint16_t SysMsCont=0;


uint8_t Mic_ValR=255;
uint16_t WMicOffCont=0;


const uint8_t S_Xx[4][7]={	{0x31,0x26,0x25,0x27,0xE4,0xE5,0x30},	//1~4位數字指標  a~g
							{0x45,0x51,0x50,0x47,0x24,0x44,0x46},
							{0x65,0x86,0x85,0x84,0x67,0x66,0x64},
							{0xB1,0xA5,0xA6,0xA7,0x90,0x91,0xB0}};
const uint8_t S_Nx[19]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x27,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,0x3E,0x00,0x78};	// N0~9,A~F,U,NC 顯示位
const uint8_t S_L3Dotx[13]={0xE6,0xE7,0xF0, 0x70,0x71, 0x87, 0xD1, 0xC4,0xC5,0xC6,0xA4,0xC7, 0xD0};
uint32_t Key_N,Key_NR;		//存储按键值
uint32_t Key2_N,Key2_NR;	//存储按键值


enum SHOW_DOT
{
	Loop_Playback,	//循环
    BT,				//蓝牙
	Voi_En,			//消原音
	Colon_1,		//冒号
	Colon_2,		//冒号
	Dot,			//逗号
	Outline1,		//电池框
	Battery_1,		//电池1
	Battery_2,		//电池2
	Battery_3,		//电池3
	Battery_4,		//电池4
	Outline2,		//电池框
	Outline3		//电池框
};

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
//****************************************
void LCD_ShowVol(uint8_t vol)
{
//	USER_DBG_INFO("====SysInf.MusicVol----------------------------------: %d  \n",vol);
	if(vol<=32)
	{
		LCD_ShowNum(0,17);
		LCD_ShowNum(1,16);
		LCD_ShowNum(2,vol/10);
		LCD_ShowNum(3,vol%10);
	}
	else
	{
		LCD_ShowNum(0,17);
		LCD_ShowNum(1,16);
		LCD_ShowNum(2,0);
		LCD_ShowNum(3,0);
	}
	UpDisplayAll();
}

//****************************************
void Knob_function();

//****************************************
void LCD_ShowDot(uint8_t n, uint8_t OnOff)
{
	UpDisplayReg(S_L3Dotx[n], OnOff);
}


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

void Recharge(float vol);			//充电显示

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
				LCD_ShowDot(Battery_1, (BattVn&1));
				LCD_ShowDot(Battery_2, ((BattVn>>1)&1));
				LCD_ShowDot(Battery_3, ((BattVn>>2)&1));
				LCD_ShowDot(Battery_4, ((BattVn>>3)&1));
			}else if(DC_IN_DET && Batt_FULL){
				LCD_ShowDot(Battery_1, 1);
				LCD_ShowDot(Battery_2, 1);
				LCD_ShowDot(Battery_3, 1);
				LCD_ShowDot(Battery_4, 1);
			}else{
				LCD_ShowDot(Battery_1, 0);
				LCD_ShowDot(Battery_2, 0);
				LCD_ShowDot(Battery_3, 0);
				LCD_ShowDot(Battery_4, 0);
			}
			UpDisplayAll();
		}else{
			static uint8_t LowPowerFlash=1;
			if(BattVol<LoPowerENv){	// 6.5v	9.75
				LCD_ShowDot(Battery_1, LowPowerFlash^=1);
			}else{
				LCD_ShowDot(Battery_1, 0);
				LCD_ShowDot(Battery_2, 0);
				LCD_ShowDot(Battery_3, 0);
				LCD_ShowDot(Battery_4, 0);
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
					app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
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
void TM16xx_main()
{
	Key_N = read_key();	//读按键值
	//====================================
	if(Key_NR != Key_N){
//		if(Key_NR) UpDisplayReg(LedFg, 0);
		Key_NR = Key_N;
		if(Key_N) USER_DBG_INFO("====read_key: 0x%08X \n",Key_N);
		//==== 按键处理 =================
		if(Key_N){
			switch (Key_N){
			case 0x00000004 :	// 錄音/播放
				user_RecWork();
				break;
			case 0x00000008 :	// 藍芽斷開
				if(app_is_bt_mode()){	//判断当前是否是 BT 模式
					app_button_sw_action(BUTTON_BT_CONN_DISCONN);
				}
				break;
			case 0x00000020 :	// 消原音
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL);
				if(SysInf.VoiceCanEn){
					SysInf.VoiceCanEn = 0;
					LCD_ShowDot(Voi_En, 0);		//
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					SysInf.VoiceCanEn = 1;
					LCD_ShowDot(Voi_En, 1);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
			case 0x00000040 :	// 循環
				if(app_is_udisk_mode() || app_is_sdcard_mode()){
					if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ALL_CYCLE){
						appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ONE;
						LCD_ShowDot(Loop_Playback, 1);
						app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);
					}else{
						appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ALL_CYCLE;
						LCD_ShowDot(Loop_Playback, 0);
						app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);
					}
					SysInf.SdCardPlayMode = appPlayerPlayMode;
				}
				break;
			case 0x00000100 :	// 話筒優先
				app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
				if(SysInf.DuckDet){
					SysInf.DuckDet = 0;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
				}else{
					SysInf.DuckDet = 0x18;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				}
				break;
			case 0x00000200 :	// 上一曲

			//	LED_TEST(0);
				if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
					app_button_sw_action(BUTTON_BT_PREV);
				}
				break;
			case 0x00000800 :	// 室內/室外
				if(++EQ_Mode > 2u)	EQ_Mode = 0u;
				break;
			case 0x00001000 :	// 播放/暫停
//				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				user_RecPlayWork();
				break;
			case 0x00008000 :	// 下一曲
			//	LED_TEST(1);

				if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
					app_button_sw_action(BUTTON_BT_NEXT);
				}
				break;
			}
//
		}else{
//			UpDisplayReg(LedFg, 1);
		}
		UpDisplayAll();		// 更新显示
	}
}

//****************************************//LCD显示服务函数
//****************************************//LCD显示服务函数
void LCD_ShowMode(uint8_t mode)
{
	switch (mode){
		case SYS_WM_BT_MODE:
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,11);LCD_ShowNum(3,18);
		break;

		case SYS_WM_SDCARD_MODE:
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,18);LCD_ShowNum(3,15);
		break;

		case SYS_WM_UDISK_MODE:
			LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
			LCD_ShowNum(0,17);LCD_ShowNum(1,16);LCD_ShowNum(2,5);LCD_ShowNum(3,11);
		break;
	}
}

////==============================================
#ifdef	RECED_EN
static uint16_t RecTime = 0;
static uint8_t RecDrv = 0;
static uint16_t Rec_num;
static uint8_t RecState = 0;
static uint16_t RecPlayInx = 0;
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
		}else{
			RecState =13;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	// 開始錄音提示音
		}
		break;
	case 2:	// 停止錄音
		Send_Id1 = MainPid;
		Send_Id2 = 204;
		Send_Group = 0;
		Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
		RecPlayInx = (Rec_num-1);
		USER_DBG_INFO("====WrRecPlayInx: %d\n",RecPlayInx);
		char Lab[64];
		rec_file_name_get_by_idx(Rec_num-1, Lab);  //根据索引获取文件名
		memcpy(&SendBuff[12], Lab,16);
		SendUpComp(SendBuff, 32);
		RecState = 0;
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
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
		}else{
			if(RecPlayInx){
				if(sd_is_attached() || udisk_is_attached()){	// RecPlay
					RecState = 16;
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
			rec_file_play_by_idx(RecPlayInx);//根据索引播放录音文件
			USER_DBG_INFO("====RecPlayInx: %d\n",RecPlayInx);
			RecPlayInx = 0;
		}
		RecState = 0;
		break;
	}
}
#endif


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
		if(Count<4)Count++;
		if(Count==3)
			{
				LCD_ShowMode(Modevalu);
				UpDisplayAll();			//3秒后显示当前模式
			}
//		USER_DBG_INFO("====KnobVal[1]: %d\n", KnobVal[1]);
//		USER_DBG_INFO("====KnobVal[2]: %d\n", KnobVal[2]);
//		USER_DBG_INFO("====KnobVal[8]: %d\n", KnobVal[8]);
//		USER_DBG_INFO("====KnobVal[13]: %d\n", KnobVal[13]);
//		USER_DBG_INFO("====KnobVal[15]: %d\n", KnobVal[15]);
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================//系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				LCD_ShowDot(BT, 1);//蓝牙开
				LCD_ShowMode(SYS_WM_BT_MODE);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= 10;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				LCD_ShowDot(BT, 1);//蓝牙亮
				UpDisplayAll();
			}else{
				if(BtLedFlash^=1)
				{
					LCD_ShowDot(BT, 1);//蓝牙亮
					UpDisplayAll();
				}
				else
				{
					LCD_ShowDot(BT, 0);//蓝牙灭
					UpDisplayAll();
				}
			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_ShowDot(BT, 0);//蓝牙关
				Modevalu=SYS_WM_SDCARD_MODE;
				LCD_ShowMode(Modevalu);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= 10;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];

			}
		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_ShowMode(SYS_WM_UDISK_MODE);
				LCD_ShowDot(BT, 0);//蓝牙关
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[I2s3L_MusL]= 10;
				ioGain[I2s3R_MusR]= ioGain[I2s3L_MusL];
			}
		}
	}
}

//==============================================充电显示
void Recharge(float vol)
{
	if(++Check_ChargeCont >= 50)//1秒
	{
		BatteryLedFlash^=1;
		if(CountR<=4)
			{
				CountR++;
				if(CountR>4)CountR=0;	//4-CountR秒清零
			}
		Check_ChargeCont=0;
	}
	LCD_ShowDot(Outline1, 1);
	LCD_ShowDot(Outline2, 1);
	LCD_ShowDot(Outline3, 1);
	for(;CountR<=4;CountR++)
	{
		if((float)vol>9.4){		//大于10%
			LCD_ShowDot(Battery_1, 1);
			CountR=0;
		}
		if((float)vol>9.9){		//大于40%
			LCD_ShowDot(Battery_2, 1);
			CountR=1;
		}
		else if((float)vol<=9.7)LCD_ShowDot(Battery_2, 0);
		if((float)vol>10.8){		//大于60%
			LCD_ShowDot(Battery_3, 1);
			CountR=2;
		}
		else if((float)vol<=10.6)LCD_ShowDot(Battery_3, 0);
		if((float)vol>11.7){		//大于80%
			LCD_ShowDot(Battery_4, 1);
			CountR=3;
		}
		else if((float)vol<=11.5)LCD_ShowDot(Battery_4, 0);
	}
	switch(CountR)
	{
		case 0:LCD_ShowDot(Battery_1, 1);
		UpDisplayAll();
			break;
		case 1:LCD_ShowDot(Battery_2, 1);
		UpDisplayAll();
			break;
		case 2:LCD_ShowDot(Battery_3, 1);
		UpDisplayAll();
			break;
		case 3:LCD_ShowDot(Battery_4, 1);
		UpDisplayAll();
			break;
		default:LCD_ShowDot(Battery_1, 0);
		LCD_ShowDot(Battery_2, 0);
		LCD_ShowDot(Battery_3, 0);
		LCD_ShowDot(Battery_4, 0);
		UpDisplayAll();
		break;
	}
}


//**** pre init ***********************
void user_init_prv(void)
{
	//=============================	GPIO配置
	#define SGM_MUTE	GPIO35		//监听耳机+AMP
	gpio_config_new(SGM_MUTE, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(SGM_MUTE, 0);

	#define WM_PO_EN	GPIO12
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 0);

	#define TM_STB	GPIO3
	gpio_config_new(TM_STB, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(TM_STB, 1);

	#define CLK_TM1668	GPIO20
	gpio_config_new(CLK_TM1668, 	GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	#define DIO_TM1668	GPIO21
	gpio_config_new(DIO_TM1668, 	GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	#define C_4051	GPIO19
	gpio_config_new(C_4051, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(C_4051, 1);

	#define A_4051	GPIO22
	gpio_config_new(A_4051, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(A_4051, 1);

	#define B_4051	GPIO23
	gpio_config_new(B_4051, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(B_4051, 1);

	#define POWER_EN	GPIO2
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);

	#define PowerKey	GPIO15
	gpio_config_new(PowerKey, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);



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
    extern void key_function(KEY_CONTEXT_t *pKeyArray);
    extern void user_key_init(void* pKeyFuncCbk);
    user_key_init(key_function);

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

		//------------------------
		BattFun();
		//-------------------------
		BK9532_Main();


	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif

		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時
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
		//--------------------------------------------------
		if(Init_Cont == 1){
			TM16xx_Init(1);
			LCD_ShowDot(Outline1, 1);
			LCD_ShowDot(Outline2, 1);
			LCD_ShowDot(Outline3, 1);
		}else if(Init_Cont==6){
			//----------------------电量检测
			static uint8_t add=0;				//判断是否开机，开机计数1
		//	static uint8_t addR=1;				//计数一次
			BattFun();
			if(KnobVal[13]>1000)//Y5
			{
				Init_Cont--;
				add = 1;
			}else{
				if(add){
					for(uint8_t i=0;i<=12;i++)
					{
						LCD_ShowDot(i,0);
					}
					LCD_ShowNum(0,17);LCD_ShowNum(1,17);LCD_ShowNum(2,17);LCD_ShowNum(3,17);
					USER_DBG_INFO("======== 1 =========\n");
					dsp_shutdown();
					REG_SYSTEM_0x1D &= ~0x2;
					system_wdt_reset_ana_dig(1, 1);
					BK3000_wdt_reset();
					os_delay_us(3200);	//1ms
					USER_DBG_INFO("======== 2 =========\n");
				}
			}
		}else if(Init_Cont == 10){
			gpio_output(WM_PO_EN, 1);
			gpio_output(SGM_MUTE, 1);

		}else if(Init_Cont == 50){
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
	    	Init_Cont--;}
//	    }else{
//	    	ACM862xWId = 0;
//	    	SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
//	    	USER_DBG_INFO("==== 12. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
//	    }
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

//			TM16xx_Init(2);
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
	#define MaxV	3900	// 最大ADC 電壓 (n/4096)
	#define CenV	3500	// 旋鈕中心值電壓
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
		if(i==0 || i==3 || i==8 || i==11 || i==13) i++;
		if(i==15) break;

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
			case 1:	// 直播音量
				ioGain[Max2L_I2s3L] = VolVar;
				ioGain[Max2R_I2s3R] = VolVar;
				ioGain[Out1L_RecL] = VolVar;
				ioGain[Out1R_RecR] = VolVar;
				break;
			case 2:	// 監聽音量
				ioGain[Out1L_DacL] = VolVar/1.4;
				ioGain[Out1R_DacR] = VolVar/1.4;
				break;
			case 5:	// 吉他音量
				ioGain[	Adc4_Mic] = (VolVar+1)/1.18;
				ioGain[	Adc5_Mic] = (VolVar+1)/1.18;
				break;
			case 10:	// Music Vol
				SysInf.MusicVol = VolVar;
				LCD_ShowVol((float)(SysInf.MusicVol+60.95)/2.185);
				Count=0;
				break;
			case 14:	// Music Bass
				UserEq[MusicEq2].gain = EqVar;
				break;

			case 12:	// Music Tre
				UserEq[MusicEq8].gain = EqVar;
				break;
			//---------------------------------------------
			case 9:	// Mic Vol
				SysInf.MicVol = VolVar;

				break;
			case 6:	// Mic Bass
				UserEq[MicEq4_2].gain = EqVar;
				UserEq[MicEq3_2].gain = EqVar;
				break;
			case 4:	// Mic Tre
				UserEq[MicEq4_7].gain = EqVar;
				UserEq[MicEq3_2].gain = EqVar;
				break;
			//--------------------------------------
			case 7:	// ECHO2 Vol
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = (float)EfVar/10;
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = (float)EfVar/20;
				break;
			}
			EfVarR=EfVar;
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
//			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->index==0 && pKeyArray->keepTime > 100){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;
//			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
//			app_wave_file_play_start(APP_WAVE_FILE_ID_START);
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

		gpio_output(SGM_MUTE, 0);	//SGM监听耳机，高立体音
//		gpio_output(AMP_CRTL, 0);	//AMP_CRTL
//		gpio_output(POWER_EN, 0);	///电源按键高使能

		//app_button_sw_action(BUTTON_BT_POWERDOWN);

		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
//		GroupWrite(0, 2);		// 保存播放記憶點


		break;
	case APP_WAVE_FILE_ID_CONN:
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}
		break;
#ifdef RECED_EN
	case APP_WAVE_FILE_ID_RESERVED4:	// 開始錄音
		RecState = 3;
		break;
	case APP_WAVE_FILE_ID_RESERVED5:	// 停止錄音
		RecState = 2;
		break;
	case APP_WAVE_FILE_ID_RESERVED0:
	case APP_WAVE_FILE_ID_MP3_MODE:
		if(RecState==16){
			RecState = 6;
		}else{
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//自動播放
		}
		break;
#endif
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


