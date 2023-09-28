/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
* 2023 問題
* 1. GPIO23 ADC 讀取不穩定
* 2. GPIO32 有時會誤讀 0
*
*
*
**********************************************************************/
#include "USER_Config.h"

#if defined(ZY_Spark_Pop)

#include "ZY_Spark_Pop_DEF.H"

static uint16_t SysMsCont=0;

//*****************************************************************************
uint8_t MusVolValR=20;
//								0		1		2		3		4		5		6		7
const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,-14,-12,-10,
//								8		9		10		11		12		13		14		15		16
								-9,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

#define	PA_MUTE		GPIO27
#define	PowerKey	GPIO15

#define	B_LED		GPIO29	//H=on
#define	R_LED		GPIO30	//H=on

#define	fullVn			4200	// mv
#define	P4Voln			4000
#define	P3Voln			3700
#define	P2Voln			3500
#define	LoPowerENv		3200
#define	LoPowerOFFv	 	3000

//*************************************************
void BattFun(void)
{
	static uint8_t Batt_ms100=0;
	if(++Batt_ms100 <100) return;
	Batt_ms100 = 0;

	static uint8_t LowPowerEn=0;
	static uint8_t LowPowerCont=0;		// 進入低功耗計數
//	static uint16_t PowerDownCont=0;	// 沒聲音 30分 關機計時 (S)

	static uint16_t DelayOffCont=0;
	static uint8_t OffPowerCont = 0;
/*
	app_handle_t sys_hdl = app_get_sys_handler();
	if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE && hci_get_acl_link_count(sys_hdl->unit)==0 && usbd_active_get()==0){	//藍芽模式且不连接状态
		if(++PowerDownCont >=1800){
			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
		}
		if(PowerDownCont%60==0) USER_DBG_INFO("==== DelayPoOffCont:%d s\n", PowerDownCont);
	}else{
		PowerDownCont=0;
	}
*/
//	uint16_t saradc_transf_adc_to_vol(uint16_t data)
	//===================================================================================
	uint16_t BattVol = saradc_transf_adc_to_vol(SarADC_val_cali_get(SarAdcVal[1]));
	static uint16_t saradc_tempe_dataR;
	static uint8_t Batt_FULL=0;
	static uint8_t DcInDet;
	extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
	//--------------------------------
	fl2hex V1;
	if((abs(SarAdcValR[1]-SarAdcVal[1])>10) || (saradc_tempe_dataR != saradc_tempe_data)){
		saradc_tempe_dataR = saradc_tempe_data;
//		USER_DBG_INFO("==== BATT ADC:%d  %d mv\n",SarAdcVal[1],BattVol);

		Send_Id1 = MainPid;
		Send_Id2 = 205;
		Send_Group = 1;
		V1.f = (float)BattVol/1000;
		Send_Val1 = V1.h;
		V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 轉換溫度值
		Send_Val2 = V1.h;
//		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
//			V1.f = app_player_get_playtime();	// PLAY_TIME
//			Send_Val3 = V1.h;
//		}else{
//			Send_Val3 = 0;
//		}

		Send_Val41 = DcInDet;
		Send_Val42 = Batt_FULL;
		SendUpComp(SendBuff, 32);
	}

//	DcInDet = vusb_2v_ready();
	if(SarAdcVal[2]>3000)	DcInDet=1;
		else	DcInDet=0;
//	USER_DBG_INFO("==== SarAdcVal[2]:%d\n",SarAdcVal[2]);
	if(DcInDet){
		if(BattVol>=fullVn)	Batt_FULL = 1;	// 滿電
			else Batt_FULL =0;
		gpio_output(R_LED, DcInDet);
//		USER_DBG_INFO("====1. BATT ADC:%d  %d mv  DcIn:%d  Batt_FULL:%d\n",SarAdcVal[1],BattVol,DcInDet,Batt_FULL);
	}else{
		static uint8_t LowPowerFlash=1;
		if(BattVol<LoPowerENv){	// 6.5v	9.75
			gpio_output(R_LED, LowPowerFlash^=1);
		}else{
			gpio_output(R_LED, 0);
		}
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
/*
	//----------------------------------------------------
	if(LowPowerEn && SysInf.MusicVol>LMVol){	// 限制功率	//yuan41
		app_handle_t sys_hdl = app_get_sys_handler();
		if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
			while(SysInf.MusicVol>LMVol){	//yuan41
				app_button_sw_action(BUTTON_BT_VOL_M);
			}
		}else{
			SysInf.MusicVol=LMVol;	//yuan41
		}
		app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
		USER_DBG_INFO("==== MusicVol:%d   %d\n",SysInf.MusicVol, LowPowerEn);	//yuan41
	}
*/
	//===============================================================
	if(LowPowerEn){	// 低電
		if(++DelayOffCont > 300){	// 5s
			DelayOffCont=0;
			if(DcInDet==0)	app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 充電中不報低電提示音
		}
		if(BattVol<LoPowerOFFv){
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
}

static int8_t 	EC11_Sw,EC11_SwR;
void Encoder_EC11_Init(unsigned char Set_EC11_TYPE);
char Encoder_EC11_Scan();
//********************************************************
void EC11_FUN(void)
{
	if(EC11_SwR != EC11_Sw){
		EC11_SwR = EC11_Sw;
		if(EC11_SwR==1){
//			if(LowPowerEn==0 || (LowPowerEn && SysInf.MusicVol<LMVol)){	//yuanV41
				if(SysInf.MusicVol<32)	SysInf.MusicVol++;
				UpComputerRW = 0;	// 更新上位機
//			}else{
//				app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
//			}
			USER_DBG_INFO("====U. EC11_Sw: %d  %3.1f\n",EC11_SwR,SysInf.MusicVol);
		}else if(EC11_SwR==-1){
			if(SysInf.MusicVol)	SysInf.MusicVol--;
			USER_DBG_INFO("====D. EC11_Sw: %d  %3.1f\n",EC11_SwR,SysInf.MusicVol);
		}
		UpComputerRW = 0;	// 更新上位機
	}
//	if(EC11_SwR != EC11_Sw) USER_DBG_INFO("==== EC11_Sw: %d  %d  %d\n",EC11_Sw, EC11_SwR, PoKeyCont);
	EC11_SwR = EC11_Sw;
}

static uint16_t WorkModeCont=0;
static uint8_t	BtLedFlash=0;
uint8_t work_modeR=10;
//*****************************************************
void user_WorkModefun(void)
{
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
	#define BtDefVol	-2

		app_handle_t sys_hdl = app_get_sys_handler();
		//=============================================
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(work_modeR != sys_hdl->sys_work_mode){
				work_modeR = sys_hdl->sys_work_mode;
				gpio_output(B_LED, 1);
			//	if(ioGain[Play_MusL] < -15){
				ioGain[Play_MusL]= BtDefVol;
				ioGain[Play_MusR]= ioGain[Play_MusL];
//				ioGain[Adc2_MusL]= -90;
//				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	//如果是连接状态
				gpio_output(B_LED, 1);
			}else{
				if(BtLedFlash^=1)	gpio_output(B_LED, 1);	// B
					else			gpio_output(B_LED, 0);	//
			}
		}
	}
}


//**** pre init ***********************
void user_init_prv(void)
{
	gpio_config_new(PowerKey,GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	gpio_config_new(PA_MUTE,GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(PA_MUTE, 0);

	gpio_config_new(R_LED, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(B_LED, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);


	SarAdcVal[2] = SarADC_val_cali_get(sar_adc_voltage_get(SARADC_CH_VUSB));
	USER_DBG_INFO("=================== SarAdcVal[2]:%d  PoKey:%d\n",SarAdcVal[2],gpio_input(PowerKey));

	BK3000_set_clock(CPU_CLK_XTAL, CPU_CLK_DIV);
	//===============================================================================================
	if((SarAdcVal[2]>3000) && gpio_input(PowerKey)){	// USB供電啟動,電源鍵 沒按下,進入假開機充電模式
		uint16_t cont=1000;
		USER_DBG_INFO("======== 0 =========\n");

//		for(uint16_t i=0; i<3000; i++){
//			CLEAR_WDT; //清除看门狗，默约38.8sec
//			os_delay_us(120);	//1ms 延遲 3秒 激活已保護輸出電池
//			if(gpio_input(PowerKey)==0)	i=3000;
//		}
//		DisplayCont=10;

		uint16_t BattVol;
		//============================================
		while(1){
			CLEAR_WDT; //清除看门狗，默约38.8sec
			//---------------------------------------
			if(gpio_input(PowerKey)==0){	// 假開機充電, 按電源鍵重新啟動流程
				USER_DBG_INFO("======== 1 =========\n");
				dsp_shutdown();
				system_wdt_reset_ana_dig(1, 1);
				BK3000_wdt_reset();
				os_delay_us(3200);	//1ms
				USER_DBG_INFO("======== 2 =========\n");
				break;
			}
			os_delay_us(120);	//1ms
			if(++cont >= 1000){
				cont = 0;
				BattVol = saradc_transf_adc_to_vol(SarADC_val_cali_get(sar_adc_voltage_get(SARADC_CH_VBAT)));
				SarAdcVal[2] = SarADC_val_cali_get(sar_adc_voltage_get(SARADC_CH_VUSB));
				if((BattVol>=fullVn) || (SarAdcVal[2]<3000)){
					USER_DBG_INFO("====1.BUTTON_BT_POWERDOWN !!!\n");
					BK3000_icu_sw_powerdown(GPIO15,BUTTON_BT_POWERDOWN);
					USER_DBG_INFO("====2.BUTTON_BT_POWERDOWN !!!\n");
				}else{
					gpio_output(R_LED, 1);
					USER_DBG_INFO("====ChgIng... BattVol: %d mv\n",BattVol);
				}
			}
		}
	}else if(gpio_input(PowerKey)){
		USER_DBG_INFO("====1.BUTTON_BT_POWERDOWN !!!\n");
		BK3000_icu_sw_powerdown(GPIO15,BUTTON_BT_POWERDOWN);
		USER_DBG_INFO("====2.BUTTON_BT_POWERDOWN !!!\n");
	}
	PowerDownFg=0;
	EC11_SwR = EC11_Sw;
}

//**** normal init ********************
void user_init(void)
{

	//===========================================
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

//	LineInR = gpio_input(LINE_DETECT_IO);
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;
	USER_DBG_INFO("======== V01 user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);

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
	if(PowerDownFg) return;
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
	if(Tick_2ms){	// 2ms End
		user_saradc_update_trig();	// Adc 讀取觸發
	}else{
		if(EC11_SwR==EC11_Sw)	EC11_Sw = Encoder_EC11_Scan();
	}
	SysMsCont++;
}
//********************************
void user_init_post(void)
{
//    gpio_config_new(GPIO15,GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
    //======================
    Encoder_EC11_Init(1);
}

static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	static uint8_t ms1000=0;
	SysMsCont=0;
	//=============================================
	if(Init_Cont >=150){

		//----------------------
		if(EF_Maim())	return;
		//======================
		EC11_FUN();
		//======================
		user_WorkModefun();
		//=====================
		BattFun();
		//===================
		user_key_scan();
		//===================
		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;
#if 1
			//-----  輸入音音量自動調整 ------------------------
			extern uint32_t user_aud_amp_task(void);
			static uint16_t OverVol,OverVolW;
			OverVolW = user_aud_amp_task();
			if(OverVol<OverVolW){
				OverVol = OverVolW;
			}else{
				if(OverVol)	OverVol*=0.95;
			}
//			USER_DBG_INFO("====  AmpTaskVol:%d  %d\n", OverVol,OverVolW);
			if(OverVol > 4800){
				if(ioGain[Play_MusL]>-5){
					ioGain[Play_MusL]--;
					ioGain[Play_MusR] = ioGain[Play_MusL];
					if(OverVol)	OverVol*=0.9;
					USER_DBG_INFO("==== AmpTaskVol:%d %d  Vol:%d\n",OverVol,OverVolW,ioGain[Play_MusL]);
				}
			}else if((OverVol > 500) && (OverVol < 4000)){
				if(ioGain[Play_MusL]<0){
					ioGain[Play_MusL]++;
					ioGain[Play_MusR] = ioGain[Play_MusL];
					USER_DBG_INFO("==== AmpTaskVol:%d %d  Vol:%d\n",OverVol,OverVolW,ioGain[Play_MusL]);
				}
			}
#endif
		}
		if(SysMsCont>1) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){

		}else if(Init_Cont == 13){
			USER_DBG_INFO("====13. Init IoGain...\n",SysMsCont);
			// 需等待 DSP 啟動完成才可開始配置 gain
			IOGainSetDamp(Tone_DacLR, ioGain[Tone_DacLR],64);	//打開提示音音頻鏈路
			IOGainSetDamp(Tone_I2s2LR, ioGain[Tone_I2s2LR],64);
			IOGainSetDamp(Out1L_DacL, ioGain[Out1L_DacL],64);
			IOGainSetDamp(Out1R_DacR, ioGain[Out1R_DacR],64);

			if(gpio_input(PowerKey)==0){
				gpio_output(B_LED, 1);
				gpio_output(PA_MUTE, 1);
				app_wave_file_play_start(APP_WAVE_FILE_ID_START);
			}else{
				USER_DBG_INFO("====21.BUTTON_BT_POWERDOWN !!!\n");
				dsp_shutdown();
				system_wdt_reset_ana_dig(1, 1);
				BK3000_wdt_reset();
				USER_DBG_INFO("====22.BUTTON_BT_POWERDOWN !!!\n");
			}
		//----------------------------
		}else if(Init_Cont == 100){

		//----------------------------
		}else if(Init_Cont == 101){
			EF_EQ_ReSend();
		//----------------------------
		}else if(Init_Cont == 102){
			g_dsp_log_on_fg = 0;
			g_mcu_log_on_fg = 0;
		}else if(Init_Cont == 103){
			Send_Id1 = CmdPid;
			Send_Id2 = 200;
			SendUpComp(SendBuff, 32);	// 請上位機連線
		}else if(Init_Cont == 104){
			g_dsp_log_on_fg = 1;
			g_mcu_log_on_fg = 1;
		}else if(Init_Cont == 148){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;
			system_work_mode_set_button(SYS_WM_BT_MODE);
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

#ifdef u_key
//static uint8_t tone = 0;
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
#ifdef KeyDbg
	return;
#endif
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		UpComputerRW = 0;
		switch (pKeyArray->index){
			case 0:

				break;
			case 1:
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				break;
			case 2:
				app_button_sw_action(BUTTON_BT_PREV);
				break;
			case 3:
				app_button_sw_action(BUTTON_BT_NEXT);
				break;
		}
		break;

	//====================================================================================================
	case KEY_D_CLICK:
		USER_DBG_INFO("KEY_D_CLICK\n");	// 短按 2次

		break;
	case KEY_T_CLICK:
		USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);	// 短按 3次

	case KEY_Q_CLICK:	break;	// 短按 4次
	case KEY_5_CLICK:	break;	// 短按 5次
	case KEY_L_PRES:
		USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->keepTime > 300){
			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 0;

			if(pKeyArray->index==0){
				gpio_output(B_LED, 0);
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
			}
		}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	break;	// 常按放開
	default:break;
	}

}
#endif

#ifdef CONFIG_USER_GPIO_INT
//******************************************************************
void user_gpio_isr(uint32_t io0_31, uint32_t io32_39)
{
	USER_DBG_INFO("io0_31:%p, io32_39:%p\n", io0_31, io32_39);
#ifdef KT56
    extern void READ_BURST();	// KT56
    //if(io32_39 & 0x1)	READ_BURST();		// gpio14 MicA & MicB
    if(io0_31 & 0x8)	READ_BURST();		// gpio14 MicA & MicB
#endif
}
#endif

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

		Send_Id1 = CmdPid;
		Send_Id2 = 202;
		SendUpComp(SendBuff, 32);	// 請上位機離線
		break;

	case APP_WAVE_FILE_ID_DISCONN:

		break;
	}
}
//********************************
void PlayWaveStop(uint16_t id)
{
	USER_DBG_INFO("=== PlayWaveStop...id:%d\n",id);

	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		GroupWrite(0, 2);		// 保存播放記憶點
		gpio_output(PA_MUTE, 0);

		while(gpio_input(PowerKey)==0);
		dsp_shutdown();
		USER_DBG_INFO("====31.BUTTON_BT_POWERDOWN !!!\n");
//		BK3000_icu_sw_powerdown(GPIO15,BUTTON_BT_POWERDOWN);
		dsp_shutdown();
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		os_delay_us(3200);	//1ms
		USER_DBG_INFO("====32.BUTTON_BT_POWERDOWN !!!\n");
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
#ifdef SDCARD_DETECT_IO
		USER_DBG_INFO("=== SD Card Mode...%d\n",player_get_play_status());
		if(player_get_play_status()==0){	// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
#endif
		break;
	case APP_WAVE_FILE_ID_LINEIN_MODE:
		break;

	case APP_WAVE_FILE_ID_LOW_BATTERY:
		break;
	}

#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
	IOGainSetDamp(MusMstVol, MusVol_TAB[(uint8_t)SysInf.MusicVol],10);
#else
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,10);
#endif
}

#endif

