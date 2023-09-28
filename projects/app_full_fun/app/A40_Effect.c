/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
#include "USER_Config.h"

#ifdef A40_Effect

#include "A40_Effect_DEF.H"

uint16_t SysMsCont=0;

uint8_t MusVolValR=20;
const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	static uint16_t WorkModeCont=0;
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
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
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];

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
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
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
	#define DAmp_Po_EN	GPIO13
	gpio_config_new(DAmp_Po_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(DAmp_Po_EN, 0);

	#define A40_Po_EN	GPIO14
	gpio_config_new(A40_Po_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(A40_Po_EN, 0);
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
	USER_DBG_INFO("======== V13 user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);
}

//********************************
void user_init_post(void)
{
	//===============================================
#ifdef USR_UART1_INIT_EN
#ifdef DCC303
    hal_uart1_init(GPIO16, GPIO17, 9600, 1, 1);
#else
    hal_uart1_init(GPIO6, GPIO7, 9600, 1, 1);
#endif
#endif

#ifdef USR_UART2_INIT_EN
    hal_uart2_init(9600, 1, 1);
#endif

#ifdef	_Hi2c
	Hi2c_Init();
#endif

#if LED_RGB_NUM
	app_i2s0_init(1);
#endif


    //==== gpio interrupt Init ===========================================
    // system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_GPIO);
    // system_peri_mcu_irq_enable(SYS_PERI_IRQ_GPIO);
#ifdef KT56
	#define KT56_INTA_IO	GPIO8	//GPIO12
	#define KT56_INTB_IO	GPIO9	// ok
	gpio_config_new(KT56_INTA_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56 中斷
	gpio_config_new(KT56_INTB_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56 中斷
	// 0:low level;	1:high level;  2:rise edge;	3:fall down interrup
	gpio_int_enable(KT56_INTA_IO, 3);
	gpio_int_enable(KT56_INTB_IO, 3);
	//----------------------------
	#define KT56_En_IO	GPIO23
	gpio_config_new(KT56_En_IO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(KT56_En_IO, 1);
#endif

#ifdef DCC303
//	#define DAmp_Po_EN	GPIO13
//	gpio_config_new(DAmp_Po_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(DAmp_Po_EN, 1);

    #define A40_Mute	GPIO3		//A40主板 MUTE OUT
    gpio_config_new(A40_Mute, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);

//	#define A40_Po_EN	GPIO14
//	gpio_config_new(A40_Po_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(A40_Po_EN, 1);

	#define System_LED	GPIO2
    gpio_config_new(System_LED, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(System_LED, 0);

	#define Po_LED	GPIO35
    gpio_config_new(Po_LED, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(Po_LED, 0);

	#define ACM86xx_EN	GPIO12
    gpio_config_new(ACM86xx_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(ACM86xx_EN, 0);

	#define Boot_EN	GPIO10
    gpio_config_new(Boot_EN, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
    gpio_output(Boot_EN, 0);

	#define USB_SW		GPIO4
	gpio_config_new(USB_SW, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(USB_SW, 0);
#else
	#define System_LED	GPIO20
	gpio_config_new(System_LED, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(System_LED, 0);

	#define LineMic_Mute	GPIO21	//有線 MIC
    gpio_config_new(LineMic_Mute, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    gpio_output(LineMic_Mute, 1);

    #define A40_Mute	GPIO18		//A40主板 MUTE OUT
    gpio_config_new(A40_Mute, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);

	#define OutOP_Mute	GPIO10		//A40主板 MUTE OUT
    gpio_config_new(OutOP_Mute, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    gpio_output(OutOP_Mute, 1);

#endif
}

//**********************************************
void user_loop(void)
{
//    main_loop_cnt++;
    mailbox_mcu_isr();
    static uint32_t t_mark = 0;
    if(sys_timeout(t_mark, 10))//10ms task
    {
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
#ifdef	udrv_saradc
		user_saradc_update_trig();	// Adc 讀取觸發
#endif
	}
	SysMsCont++;
#ifdef	CONFIG_AUD_AMP_DET_FFT
	extern u32	Tempo_ms_cont;
	Tempo_ms_cont++;
#endif

}

void SendTX2();
extern uint8_t  TX2_Buf_T, TX2_Buf_B;
extern void KTV_main(void);

static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	if(PowerDownFg) return;
	//=============================================
	if(Init_Cont ==150){
		SysMsCont = 0;
		//==== UART_TX_FUN ============
		if(TX2_Buf_T != TX2_Buf_B)	SendTX2();
		//======================
#ifdef	ACM86xxId1
		if(ACM_main()) return;
#endif
		if(EF_Maim()) return;

		//----------------------控制LCD显示和音源链路音量设置
//		user_WorkModefun();

		//-------------------------
		KT56_main();

		extern void KTV_main(void);
		KTV_main();


	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif

		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時
			ms1000 = 0;
#ifdef	ACM86xxId1
			ACM_REPORT();
#endif
			static uint8_t SysLed = 0;
			gpio_output(System_LED, SysLed^=1);
		}
#ifdef		u_key
		//----------------------按键扫描
		user_key_scan();
#endif

		if(SysMsCont>10) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);

		//----------------------电量检测
//		Check_Charge_Vol();
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){
			KT56_Init();
	#ifdef ACM86xxId1
		}else if(Init_Cont == 2){
			gpio_output(ACM86xx_EN, 1);

		}else if(Init_Cont == 16){
			ACM8625_init();
			USER_DBG_INFO("==== 10. ACM8625_init Ok... MsCont:%d \n",SysMsCont);
		}else if(Init_Cont == 17){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM.1_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
	#endif
	#ifdef ACM86xxId2
		}else if(Init_Cont == 18 && ACM862x_IIC_ADDR[1]){
		    ACM862xWId = 1;
		    SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    if(ACM_main()){
		    	Init_Cont--;
		    }else{
		    	gpio_output(Boot_EN, 1);
		    	ACM862xWId = 0;
		    	SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    	USER_DBG_INFO("==== 12. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
		    }
	#endif
		}else if(Init_Cont==19){
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
		}else if(Init_Cont == 20){
			EF_EQ_ReSend();
			ioGain[Play_MusL] = -90;
			ioGain[Play_MusR] = -90;
//			USER_DBG_INFO("===BAT_VOL===:%d:\n", SarAdcVal[3]);  //电池电量查看

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
//			system_work_mode_set_button(app_h->sys_work_mode);
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

}

//***************************************************
void Out_VolSet(float val)
{
	ioGain[Out1L_DacL] = SysInf.OutVol;
	ioGain[Out1R_DacR] = SysInf.OutVol;
	IOGainSet(Out1L_DacL, SysInf.OutVol);
	IOGainSet(Out1R_DacR, SysInf.OutVol);
}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
#if 0
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
#endif
}
//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
#if 0
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
#endif
}
#ifdef		u_key
//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
//	return;
	app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
	switch (pKeyArray->index){
		case 0:	// 模式切換
			system_work_mode_change_button();

			break;
		case 1:	// 光效切换
#ifdef CONFIG_AUD_AMP_DET_FFT
			if(++LED_n >3)	LED_n = 0;
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
				RGB_AUTO[0]=1;	//聲控自動轉漸變(0,1)
			}else if(LED_n ==3){
				RGB_AUTO[0]=1;	//聲控自動轉漸變(0,1)

			}
#endif
			break;
		case 2:	// 均衡
			if(++EQ_Mode > 2u)	EQ_Mode = 0u;
			break;
		case 3:	// 麦

//			LCD_ShowNum(0,add1++);
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
			break;
		case 3:

				if(SysInf.MusicVol) 	SysInf.MusicVol--;
			break;
		case 4:

				if(SysInf.MusicVol < 32) 	SysInf.MusicVol++;
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		//		USER_DBG_INFO("1.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
		//---------------------------------------------------
		if(pKeyArray->keepTime%100==0){
//			USER_DBG_INFO("2.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			switch (pKeyArray->index){
			case 2:	// VOL +
				if(SysInf.MusicVol<32)	SysInf.MusicVol++;// SD, AUX MODE	//yuan41
				break;
			case 3:	// VOL -
				if(SysInf.MusicVol)	SysInf.MusicVol--;	// SD, AUX MODE
				break;
			case 4:	// MODE SW
				if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	//判断当前是否是 BT 模式
					app_button_sw_action(BUTTON_BT_CONN_DISCONN);
				}
				break;
			}
		}else if(pKeyArray->keepTime > 1000){
//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			pKeyArray->keepTime = 10;
			if(pKeyArray->index==0){
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
			}
		}
		break;

	case KEY_L_REL:
		USER_DBG_INFO("KEY_L_REL\n");	// 常按放開
		break;
	default:break;
	}
}
#endif

//********************************
void PlayWaveStart(uint16_t id)
{
	USER_DBG_INFO("wave_start, ID:%d\n", id);
//	IOGainSetDamp(MusMstVol, -90,10);            //Music off
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		PowerDownFg = 1;
		GroupWrite(0, 2);		// 保存播放記憶點
#ifdef DCC303
		gpio_output(Po_LED, 1);	// 1 = OFF
		gpio_output(A40_Po_EN, 0);
#endif
		break;

	case APP_WAVE_FILE_ID_DISCONN:

		break;
	}
}

//********************************
void PlayWaveStop(uint16_t id)
{
	//	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:

		while(gpio_input(GPIO15)==0){
			CLEAR_WDT; //清除看门狗，默约38.8sec
		}

		dsp_shutdown();
//		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		gpio_output(DAmp_Po_EN, 0);
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		os_delay_us(160000);	//10ms
		break;
	case APP_WAVE_FILE_ID_CONN:
//		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
//		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
//			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
//		}
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

//******************************************************************
void user_gpio_isr(uint32_t io0_31, uint32_t io32_39)
{
	USER_DBG_INFO("io0_31:%p, io32_39:%p\n", io0_31, io32_39);
    extern void READ_BURST();	// KT56
#ifdef DCC303	
	if(io0_31 & 0x100)	READ_BURST();		// gpio8,9 	MicA,8
    if(io0_31 & 0x0008)	READ_BURST();		// gpio3 	MicB
#else
	if(io0_31 & 0x300)	READ_BURST();		// gpio8,9 	MicA,8
//    if(io0_31 & 0x0008)	READ_BURST();		// gpio3 	MicB
#endif	
}

//*******************************************************************************0
/**** KTV A40 通信協議  UART 9600，8，N，1
[I]com2_cmd_recv_proc_direct: D1<LF>
[I]com2_cmd_recv_proc_direct: 51<LF>
[I]com2_cmd_recv_proc_direct: 00<LF>
[I]com2_cmd_recv_proc_direct: 2F<LF>
[I]com2_cmd_recv_proc_direct: 51<LF>
 **********************************************************************************/
uint8_t Uart2Rx_lf = 0;				// 接收 Buff 指標
uint8_t Uart2Rx_Buff[10];
uint8_t  TX2_Buf[20][2];
uint8_t  TX2_Buf_T=0;
uint8_t  TX2_Buf_B=0;

//const int MusVol_Tab[10] = {0, 2, 5, 9, 14, 20, 25, 28, 30, 32};
const int MusVol_Tab[10] = {-90, -30, -20, -15, -10, -7, -5, -3, -1, 0};
const int MicVol_Tab[5] = {-90, -10, -6, -3, 0};
const float EchoVol_Tab[5] = {0.f, 0.2f, 0.4f, 1.0f, 1.5f};
const float RevVol_Tab[5] = {0.f, 0.2f, 0.4f, 0.6f, 0.8f};

uint8_t	Mus_Val=5;
uint8_t	Mus_ValR=255;
uint8_t	Mic_Val=3;
uint8_t	Mic_ValR=255;
uint8_t	Echo_Val=2;
uint8_t	Echo_ValR=255;
uint8_t	Rev_Val=2;
uint8_t	Rev_ValR=255;
uint8_t	EfMode_Val=2;
uint8_t	EfMode_ValR=255;
uint8_t	EF_Mode_AR=2;
uint8_t	EqMode_Val=1;
uint8_t	EqMode_ValR=255;
int8_t	Key_Val=0;
int8_t	Key_ValR=255;
uint8_t	AvOut_Val=24;
uint8_t	AvOut_ValR=255;

uint8_t  Voice_Clr = 0;

uint8_t	BT_EN_R = 2;
uint8_t PLAY_EN = 1;
uint8_t PLAY_EN_R = 2;

uint8_t	IN_TYPE = 0;
uint16_t IN_Cont = 0;

uint8_t	VoiceCtrl=0;
uint8_t	AutoVoice=0;

uint16_t Ka_delay = 5000;

//***************************************************
void Uart1_CmdRx(uint8_t rdata)
{
	//Rx_Buff5[5]={0xD1,id,val,0x2F,ChkSum};
	//===========================================
	Uart2Rx_Buff[Uart2Rx_lf++] = 	rdata;
	if(Uart2Rx_lf==1 && Uart2Rx_Buff[0] != 0xD1){
		Uart2Rx_lf = 0;
		return;
	}
	if(Uart2Rx_lf==4 && Uart2Rx_Buff[3] != 0x2F){
		Uart2Rx_lf = 0;
		return;
	}
	//===========================================
	if(Uart2Rx_lf <5) return;
	Ka_delay = 0;
	USER_DBG_INFO("Uart2Rx_Buff: %02X  %02X\n", Uart2Rx_Buff[1], Uart2Rx_Buff[2]);
	switch (Uart2Rx_Buff[1]){
		case 0x51:	// SPK ON/OFF
	//		EEROM_R[SPK_OnOff] = Uart2Rx_Buff[2];
			break;
		case 0x52: // AUTO VOICE ON/OFF
			if(Uart2Rx_Buff[2] == 2){
				AutoVoice = 1;
			}else{
				if(VoiceCtrl == 1) AutoVoice = 0;
			}
			break;
		case 0x53:	// ECHO VOL 0~4
			if(Uart2Rx_Buff[2] <5)	Echo_Val = Uart2Rx_Buff[2];
			break;
		case 0x54:	// REV VOL  0~4
			if(Uart2Rx_Buff[2] <5)	Rev_Val = Uart2Rx_Buff[2];
			break;
		case 0x5A:	// EQ MODE 0~2
			if(Uart2Rx_Buff[2] <3)	EqMode_Val = Uart2Rx_Buff[2];
			break;
		case 0x5B:	// Music KEY
			if((int8_t)Uart2Rx_Buff[2] >= -5 && (int8_t)Uart2Rx_Buff[2] <= 5)	Key_Val = (int8_t)Uart2Rx_Buff[2];
			break;
		case 0x5C:	// EF MODE
			if(Uart2Rx_Buff[2] <5)	EfMode_Val = Uart2Rx_Buff[2];
			break;
		case 0x5D:	// Mic VOL
			if(Uart2Rx_Buff[2]<5)	Mic_Val = Uart2Rx_Buff[2];
			break;
		case 0x5E:	// Music VOL
			if(Uart2Rx_Buff[2] <10)	Mus_Val = Uart2Rx_Buff[2];
			break;
		case 0x5F:	// AvOut VOL
			if(Uart2Rx_Buff[2]<10)	SysInf.OutVol = Uart2Rx_Buff[2];
			break;
		case 0x70:	// AUTO EF ON/OFF
			if(Uart2Rx_Buff[2]){
				EF_Mode = EF_Mode_AR;
				PLAY_EN = 1;
			}else{
//				EF_Mode = 0;
				Key_Val = 0;	//KEY RESET
				PLAY_EN = 0;
			}
			break;
		case 0x71: // EF_EQ_ReSend
			if(Uart2Rx_Buff[2]==1)	EF_EQ_ReSend();
			break;
		case 0x72:	// POWER OFF
//			if(Uart2Rx_Buff[2]==5){
//				EEROM_R[POWER_R]=0;
//				goto	CHG_LOOP;
//			}
			break;
		case 0x73:	// VoiceCan
			SysInf.VoiceCanEn = Uart2Rx_Buff[2];
			break;
		case 0x74:	// HOW 0~1
			UserEf.Fshift =  Uart2Rx_Buff[2];
			break;
		case 0x75:	// WMic Pair
#ifndef	DCC303		
			freqSearch(510000, 530000, 0);
			freqSearch(530000, 550000, 2);
#endif
			break;
		}
	Uart2Rx_lf = 0;
}

void com1_cmd_send_proc(uint8_t *buf, uint8_t len);
//void com2_cmd_send_proc(uint8_t *buf, uint8_t len);
// 0x55,0x54,0x89,0x6F,0x00,0x00,comm,val,0x00,chkSum(cmmm+chkval)
uint8_t Uart2Tx_Buff[10]={0x55,0x54,0x89,0x6F,0x00,0x00,0xFF,0xFF,0x00,0xFF};
uint8_t Uart2Tx_Buff5[5]={0xD1,0xFF,0xFF,0x2F,0xFF};
//**** MCU -> KTV_UART ******************************************************
void SendTX2()
{
	if(Ka_delay) return;
	if(++TX2_Buf_B >=20) TX2_Buf_B = 0;
	if(TX2_Buf[TX2_Buf_B][0] >= 0x80){
		Uart2Tx_Buff[6]=TX2_Buf[TX2_Buf_B][0];
		Uart2Tx_Buff[7]=TX2_Buf[TX2_Buf_B][1];
		Uart2Tx_Buff[9] = TX2_Buf[TX2_Buf_B][0]+TX2_Buf[TX2_Buf_B][1];
		com1_cmd_send_proc(Uart2Tx_Buff, 10);
		USER_DBG_INFO("TX2_Buf10: %02X  %02X   %02X %d \n", TX2_Buf[TX2_Buf_B][0], TX2_Buf[TX2_Buf_B][1], Uart2Tx_Buff[9], TX2_Buf_B);
	}else{
		Uart2Tx_Buff5[1]=TX2_Buf[TX2_Buf_B][0];
		Uart2Tx_Buff5[2]=TX2_Buf[TX2_Buf_B][1];
		Uart2Tx_Buff5[4] = TX2_Buf[TX2_Buf_B][0]+TX2_Buf[TX2_Buf_B][1];
		com1_cmd_send_proc(Uart2Tx_Buff5, 5);
		USER_DBG_INFO("TX2_Buf5: %02X  %02X   %02X %d \n", TX2_Buf[TX2_Buf_B][0], TX2_Buf[TX2_Buf_B][1], Uart2Tx_Buff5[4],TX2_Buf_B);
	}
}

//*********************************************
void Send_Inf(uint8_t comm, uint8_t val)
{
	if(++TX2_Buf_T >=20) TX2_Buf_T = 0;
	TX2_Buf[TX2_Buf_T][0] = comm;
	TX2_Buf[TX2_Buf_T][1] = val;
}



#if 1

//*************************************************************************************
//*************************************************************************************
void KTV_main(void)
{
	static  uint8_t PLAY_TYPE = 0;
	static  uint8_t	BT_Cont = 0;
//	static  uint8_t	MusVol_Bak=2;
//	static  uint8_t	BtVol_Bak=5;
//	u16	BT_val=0;
//	u16 KA_val=0;
	if(Ka_delay) Ka_delay--;
#if 1
	//---- Input Gate Conter ---------------------------------
	if(PLAY_EN == 0){	//BT ON	  //20190917 ADD
		if(PLAY_TYPE == 0){
			if(BT_Cont == 0 ){
				BT_Cont = 200;
//				PLAY_EN_R = PLAY_EN;
//				MusVol_Bak = Mus_Val;
//				Mus_Val = BtVol_Bak;
				ioGain[Play_MusL] = 0;
				ioGain[Play_MusR] = 0;
				PLAY_TYPE = 1;
				USER_DBG_INFO("PLAY_EN: %d\n", PLAY_EN);
			}else{
				BT_Cont--;
				USER_DBG_INFO("====BT_Cont: %d\n", BT_Cont);
			}
		}
	}else{	//BT OFF
		if(PLAY_TYPE == 1){
			BT_Cont = 200;
//			PLAY_EN_R = PLAY_EN;
//			BtVol_Bak = Mus_Val;
//			Mus_Val = MusVol_Bak;
			ioGain[Play_MusL] = -90;
			ioGain[Play_MusR] = -90;
			PLAY_TYPE = 0;
			USER_DBG_INFO("PLAY_EN: %d\n", PLAY_EN);
		}
	}
#endif
	//---- vol cont --------------------------------------
	if(Mus_ValR != Mus_Val){
		Mus_ValR = Mus_Val;
		SysInf.MusicVol = MusVol_Tab[Mus_Val];
		Send_Inf(0x5E,Mus_Val);
	}
	if(Mic_ValR != Mic_Val){
		Mic_ValR = Mic_Val;
		SysInf.MicVol = MicVol_Tab[Mic_Val];
		Send_Inf(0x5D,Mic_Val);
	}
	if(Echo_ValR != Echo_Val){
		Echo_ValR = Echo_Val;
		UserEf.EchoVol = EchoVol_Tab[Echo_Val];
	//	UserEf.EchoVol2 = EchoVol_Tab[Echo_Val];
		Send_Inf(0x53,Echo_Val);
	}
	if(Rev_ValR != Rev_Val){
		Rev_ValR = Rev_Val;
		UserEf.RevVol = RevVol_Tab[Rev_Val];
		UserEf.RevVol2 = RevVol_Tab[Rev_Val];
		Send_Inf(0x54,Rev_Val);
	}
	if(EfMode_ValR != EfMode_Val){
		EfMode_ValR = EfMode_Val;
		EF_Mode = EfMode_Val;
		EF_Mode_AR = EF_Mode;
		Send_Inf(0x5C,EfMode_Val);
	}
	if(EqMode_ValR != EqMode_Val){
		EqMode_ValR = EqMode_Val;
		EQ_Mode = EqMode_Val;
		Send_Inf(0x5A,EqMode_Val);
	}

	if(Key_ValR != Key_Val){
		Key_ValR = Key_Val;
		SysInf.KeyShift = (int8_t)Key_Val;
		Send_Inf(0x5B,Key_Val);
	}
	if(AvOut_ValR != AvOut_Val){
		AvOut_ValR = AvOut_Val;
//		Send_Inf(0xnn,AvOut);
	}

	//---- SPK ON/OFF ---------------------------------
//	if(SpkOnOff_ValR != EEROM_R[SPK_OnOff]){
//		SpkOnOff_ValR = EEROM_R[SPK_OnOff];
//		YDA174_SPK_OnOff(SpkOnOff_ValR);
//		ADI_Spk_Sw(SpkOnOff_ValR);
//		Send_Inf(0x51,SpkOnOff_ValR);
//	}

}

#endif
#endif
