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
#ifdef XWX_S6616

#include "XWX_S6616_DEF.H"

#ifdef CONFIG_AUD_AMP_DET_FFT
	extern	u8 W_Y[9];
	extern	u8 RGB_MODE[9];	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
	extern	u8 RGB_AUTO[9];	//聲控自動轉漸變(0,1)
	extern	u8 RGB_SP[9];	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
	extern	u8 RGB_EF[9];	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
	extern	u8 RGB_AEF[9];	//
	static uint8_t LED_n = 0;
#endif

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

//==============================================
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

static uint8_t	BtLedFlash=0;
static uint16_t WorkModeCont=0;
//************* 控制LCD显示和音源链路音量设置
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
				LCD_ShowDot(K1_BT, 1);
				LCD_ShowDot(K3_SD, 0);
				LCD_ShowDot(K2_USB, 0);
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
				LCD_ShowDot(K1_BT, 0);
				LCD_ShowDot(K3_SD, 1);
				LCD_ShowDot(K2_USB, 0);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(player_get_play_status()){


			}
		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_ShowDot(K1_BT, 0);
				LCD_ShowDot(K3_SD, 0);
				LCD_ShowDot(K2_USB, 1);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(player_get_play_status()){

			}
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				LCD_ShowDot(K1_BT, 0);
				LCD_ShowDot(K3_SD, 0);
				LCD_ShowDot(K2_USB, 0);
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

	//=============================	GPIO配置
	#define POWER_EN	GPIO35  //功放
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 1);

	#define WM_PO_EN	GPIO12	//麦克使能
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 1);

	#define BOOT_EN	GPIO3	//升压
	gpio_config_new(BOOT_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(BOOT_EN, 1);
	gpio_output(WM_PO_EN, 1);

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
		if(ACM_main()) return;
		//---------------------- 控制LCD显示和音源链路音量设置
		user_WorkModefun();

		//---------------------- 编码器旋钮
		Knob_function();

		//-------------------------
		BK9532_Main();


	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif

		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;
			ACM_REPORT();

//			USER_DBG_INFO("====player_get_play_status: %d\n",player_get_play_status());

		}

		//----------------------按键扫描
		user_key_scan();

		if(EF_Maim()) return;
		if(SysMsCont>10) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);

		//----------------------电量检测
//		Check_Charge_Vol();
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 10){
			BK9532_Init();

		}else if(Init_Cont == 11){
			ACM8625_init();
		}else if(Init_Cont == 12){
			if(ACM_main()){
				Init_Cont--;
			}else{
				//==== float L2L, R2L, L2R, R2R  -110 ~ +48db ===============
				ACM86_InputMixer(0,0,-110,-110);
				USER_DBG_INFO("==== 11. ACM.1_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
		}else if(Init_Cont == 18 && ACM862x_IIC_ADDR[1]){
			ACM862xWId = 1;
			SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
			if(ACM_main()){
				Init_Cont--;
			}else{
				//==== float L2L, R2L, L2R, R2R  -110 ~ +48db ===============
				ACM86_InputMixer(0,-110,-110,0);
				ACM862xWId = 0;
				SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
				USER_DBG_INFO("==== 12. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
		}else if(Init_Cont==20){
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
		}else if(Init_Cont == 21){
			EF_EQ_ReSend();
//			USER_DBG_INFO("===BAT_VOL===:%d:\n", SarAdcVal[3]);  //电池电量查看
		}else if(Init_Cont == 50){
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
#if 1
	#define MaxV	3600	// 最大ADC 電壓 (n/4096)
	#define CenV	1800	// 旋鈕中心值電壓
#else
	#define MaxV	4000	// 最大ADC 電壓 (n/4096)
	#define CenV	1800	// 旋鈕中心值電壓
#endif
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
	for(uint8_t i=0; i<8; i++){
//		KnobVal[i] = SarADC_val_cali_get(KnobVal[i]);	// 校正芯片誤差
		if(i==4){	// Batt_Det
			if(abs(KnobValR[i]-KnobVal[i])>=200){
				KnobValR[i]=KnobVal[i];
				USER_DBG_INFO("====SarAdcBatt: %d  %d\n",i, KnobValR[i]);

			}
		}else if(i==5){	// Chg_Det
			if(abs(KnobValR[i]-KnobVal[i])>=100){
				KnobValR[i]=KnobVal[i];
				USER_DBG_INFO("====SarAdc Chg_Det: %d  %d\n",i, KnobValR[i]);

			}
		}else{
			if(i==6) i++;	// NC
			if(abs(KnobValR[i]-KnobVal[i])>60){
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
					if(VolVar <-57) VolVar = -90;
					KeyVar =(KnobValR[i]/KeyDn6)-6;
					EqVar = (KnobValR[i]/EqDn12)-120;
					EfVar =  (KnobValR[i]/EfDn10);
				}
				//====================================================
				switch (i){
				case 3:// ECHO2 Vol
					UserEf.EchoVol = (float)EfVar/10;
					UserEf.EchoVol2 = (float)EfVar/10;
					UserEf.RevVol = (float)EfVar/20;
					UserEf.RevVol2 = (float)EfVar/20;
					LCD_ShowVol((float)(SysInf.MusicVol+61)/2.2);
					break;
				case 7:	// Music Vol
					SysInf.MusicVol = VolVar;
					VolVar = (float)(SysInf.MusicVol+61)/2.2;
					if(VolVar < 0) VolVar = 0;
					LCD_ShowVol(VolVar);
					break;
				case 1:	// Music Tre
					UserEq[MicEq3_8].gain = EqVar;
					UserEq[MicEq4_8].gain = EqVar;
					break;

				case 2:	// Mic Bass
					UserEq[MicEq3_2].gain = EqVar;
					UserEq[MicEq4_2].gain = EqVar;
					break;
					//---------------------------------------------
				case 0:	// Mic Vol
					SysInf.MicVol = VolVar;
					break;
				}
				USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], KnobVal[i], VolVar, KeyVar, EqVar, EfVar);
				KnobValR[i]=KnobVal[i];
				UpComputerRW = 0;	// 設定上位機同步更新
			}
		}
	}
}

static uint8_t tt=0;
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
	//return;
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;
	case KEY_S_REL:
		switch (pKeyArray->index){
		case 0:	// 模式切換
			system_work_mode_change_button();
			LCD_ShowNum(0,17);
			LCD_ShowNum(1,17);
			LCD_ShowNum(3,17);
			LCD_ShowNum(2,17);
//			add1=0;
//			add2=0;
//			add3=0;
//			add4=0;
			break;
		case 1:	// 光效切换
			LCD_ShowDot(tt,1);
			if(tt) LCD_ShowDot(tt-1,0);
			if(++tt>9){
				LCD_ShowDot(tt-1,0);
				LCD_ShowDot(0,1);
			}
#ifdef CONFIG_AUD_AMP_DET_FFT
			if(++LED_n >5)	LED_n = 0;
			if(LED_n ==0){
				RGB_MODE[0]=2;	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
				RGB_EF[0]= 4;	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
				RGB_SP[0]= 5;	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
				RGB_AUTO[0]=1;	//聲控自動轉漸變(0,1)
				RGB_AEF[0]=0;
				W_Y[0]=255;
			}else if(LED_n ==1){
				RGB_MODE[0]=4;	//RGB模式 0=RGB, 1=漸變定色, 2=聲控, 4=幻彩
				RGB_EF[0]= 15;	//幻彩效果(1~15); 252,3聲控模式(1~4) ===== 聲控
				RGB_SP[0]= 1;	//漸變速度(0~239),定色(240~250),EQ_Bank(0~8) ===== 定色 黃光
				RGB_AUTO[0]=1;	//聲控自動轉漸變(0,1)
				RGB_AEF[0]=0;
				W_Y[0]=255;
			}else if(LED_n ==2){
				RGB_MODE[0]=1;	// 單色配置
				RGB_EF[0]= 15;
				RGB_SP[0]= 1;	// 顏色 0~ 10
				/*
				 	0.白色(240)	//{255,160,130}
					1.紅色(241)
					2.綠色(242)
					3.藍色(243)
					4.黃色(244)
					5.紫色(245)
					6.青色(246)
					7.桃紅色(247)
					8.夢幻粉(248)
					9.土豪金(249)
					10.神秘紫(250)
				 */
			}else if(LED_n ==3){
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 2;
			}else if(LED_n ==4){
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 3;
			}else if(LED_n ==5){
				RGB_MODE[0]=1;
				RGB_EF[0]= 15;
				RGB_SP[0]= 0;
			}
#endif
			break;
		case 2:	// 均衡
			if(++EQ_Mode > 2u)	EQ_Mode = 0u;
			break;
		case 3:	// 話筒優先
			app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
			if(SysInf.DuckDet==0){
				SysInf.DuckDet = 0x18;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
			}else{
				SysInf.DuckDet = 0;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
			}
			break;
		case 4:	// 录制
			user_RecWork();
			break;
		case 5:	// 上一首
			if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
				app_button_sw_action(BUTTON_BT_PREV);
			}
			break;
		case 6:	// 播放/停止
			user_RecPlayWork();
			break;
		case 7:	// 下一首
			if((app_is_bt_mode() || app_is_udisk_mode() || app_is_sdcard_mode())){
				app_button_sw_action(BUTTON_BT_NEXT);
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
		case 0:	// MODE
			GroupWrite(0, 2);		// 保存播放記憶點

			if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
				if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ALL_CYCLE){
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ONE;
					LCD_ShowDot(K6_REP, 1);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
				}else{
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ALL_CYCLE;
					LCD_ShowDot(K6_REP, 0);
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8);
				}
				SysInf.SdCardPlayMode = appPlayerPlayMode;
			}
			break;
		case 1:	// RGB LED OFF
			RGB_MODE[0]=1;
			RGB_EF[0]= 0;
			RGB_SP[0]= 0;
			break;
		case 5:	// PRE
			app_player_button_prevDir();
			break;
		case 7:	// NEXT
			app_player_button_nextDir();
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


