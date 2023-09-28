/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen Yuan
* 版本:	V1.0
* 日期： 2022-8-4
* 描述：
**********************************************************************/
#include "USER_Config.h"


#ifdef A40_Effict

#define USER_DBG_INFO(fmt,...)       os_printf("[USER]"fmt, ##__VA_ARGS__)


//**** pre init ***********************
void user_init_prv(void)
{

}

//**** normal init ********************
void user_init(void)
{

}

//********************************
void user_init_post(void)
{
	//=========================
    extern void dsp_audio_init(void);
    dsp_audio_init();

    //=========================
	Hi2c_Init();

	//===============================================
//	g_dsp_log_on_fg = 0;
//	g_mcu_log_on_fg = 0;
#if USR_UART1_INIT_EN
    system_mem_clk_enable(SYS_MEM_CLK_UART1);
    system_peri_clk_enable(SYS_PERI_CLK_UART1);
    extern void uart1_initialise(u_int32 baud_rate);
    uart1_initialise(9600);	// def UART_BAUDRATE_115200 移除函數內 GPIO 設置
    system_ctrl(SYS_CTRL_CMD_UART2_GPIO_MAPPING, 0);	//0:GPIO6~7, 1:GPIO16~17
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_UART1);
    // system_peri_dsp_irq_enable(SYS_PERI_IRQ_UART1);
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_UART1);
#endif

#if USR_UART2_INIT_EN
    system_mem_clk_enable(SYS_MEM_CLK_UART2);
    system_peri_clk_enable(SYS_PERI_CLK_UART2);
    extern void uart2_initialise(u_int32 baud_rate);
    uart2_initialise(9600);	// def UART_BAUDRATE_115200
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_UART2);
    // system_peri_dsp_irq_enable(SYS_PERI_IRQ_UART1);
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_UART2);
#endif

    //==== gpio interrupt Init ===========================================
    // system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_GPIO);
    // system_peri_mcu_irq_enable(SYS_PERI_IRQ_GPIO);
#ifdef __Kt_WirelessMicRxDrv_H
	#define KT56_INTA_IO	GPIO8	//GPIO12
	#define KT56_INTB_IO	GPIO9	// ok
	gpio_config_new(KT56_INTA_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56 中斷
	gpio_config_new(KT56_INTB_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// KT56 中斷
	// 0:low level;	1:high level;  2:rise edge;	3:fall down interrup
	gpio_int_enable(KT56_INTA_IO, 3);
	gpio_int_enable(KT56_INTB_IO, 3);
	//----------------------------
	#define KT56_En_IO	GPIO23
	gpio_config_new(KT56_En_IO, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    gpio_output(KT56_En_IO, 1);
#endif
	#define LineMic_Mute	GPIO21	//有線 MIC
    gpio_config_new(LineMic_Mute, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    gpio_output(LineMic_Mute, 1);

    #define A40_Mute	GPIO18		//A40主板 MUTE OUT
    gpio_config_new(A40_Mute, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);

	#define OutOP_Mute	GPIO10		//A40主板 MUTE OUT
    gpio_config_new(OutOP_Mute, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    gpio_output(OutOP_Mute, 1);

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
#ifdef CONFIG_SYS_TICK_INT_1MS
void tick_task_1ms(void) //timer0 1ms isr
{

}
#endif


void SendTX2();
extern uint8_t  TX2_Buf_T, TX2_Buf_B;
extern void KTV_main(void);

static uint16_t Init_Cont = 0;
//********************************
void user_loop_10ms(void)
{
	//=============================================
	if(Init_Cont ==150){

		//------------------------------
		extern void usb_io_as_state_update(void);
		usb_io_as_state_update();

		//==== UART_TX_FUN ============
		if(TX2_Buf_T != TX2_Buf_B)	SendTX2();

		//---------------------
		KTV_main();
		//----------------------
		EF_Maim();

		//-----------------
#ifdef __Kt_WirelessMicRxDrv_H
		KT56_main();
#endif

	#ifdef CONFIG_AUD_REC_AMP_DET
		void user_aud_amp_task(void);
		user_aud_amp_task();
	#endif



	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){
			//-------------------------------------------
			UserFlash_Init(23);	// 設置FLASH 傳入版本參數
			//-------------------------------------------
		//	IOGainSet(Tone_DacLR, ioGain[Tone_DacLR]);	//打開提示音音頻鏈路
		//	IOGainSet(Tone_I2s2LR, ioGain[Tone_I2s2LR]);

			IOGainSet(Out1L_DacL, ioGain[Out1L_DacL]);
			IOGainSet(Out1R_DacR, ioGain[Out1R_DacR]);
			IOGainSet(Out2L_I2s2L, ioGain[Out2L_I2s2L]);
			IOGainSet(Out2R_I2s2R, ioGain[Out2R_I2s2R]);
			IOGainSet(Out2L_I2s3L, ioGain[Out2L_I2s3L]);
			IOGainSet(Out2R_I2s3R, ioGain[Out2R_I2s3R]);

		}else if(Init_Cont == 100){
	#ifdef __Kt_WirelessMicRxDrv_H
			//---------------
			KT56_Init();
	#endif

		}else if(Init_Cont == 147){
			EF_EQ_ReSend();
			USER_DBG_INFO("USER Init OK...\n");
		}
	}
}

//***************************************************
void Music_VolSet(float val)
{
	extern uint8_t PLAY_EN;
	if(PLAY_EN){
		ioGain[Adc2_MusL] = val;
		ioGain[Adc3_MusR] = val;
		IOGainSet(Adc2_MusL, ioGain[Adc2_MusL]);
		IOGainSet(Adc3_MusR, ioGain[Adc3_MusR]);
	}else{
		ioGain[Play_MusL] = val;
		ioGain[Play_MusR] = val;
		IOGainSet(Play_MusL, ioGain[Play_MusL]);
		IOGainSet(Play_MusR, ioGain[Play_MusR]);
	}
}
//***************************************************
void Mic_VolSet(float val)
{
	ioGain[Adc4_Mic] = val-6;
	ioGain[Adc5_Mic] = val-6;
	ioGain[I2s3L_Mic] = val;
	ioGain[I2s3R_Mic] = val;
	IOGainSet(Adc4_Mic, ioGain[Adc4_Mic]);
	IOGainSet(Adc5_Mic, ioGain[Adc5_Mic]);
	IOGainSet(I2s3L_Mic, ioGain[I2s3L_Mic]);
	IOGainSet(I2s3R_Mic, ioGain[I2s3R_Mic]);

}
//***************************************************
void Out_VolSet(float val)
{
	ioGain[Out1L_DacL] = SysInf.OutVol;
	ioGain[Out1R_DacR] = SysInf.OutVol;
	IOGainSet(Out1L_DacL, SysInf.OutVol);
	IOGainSet(Out1R_DacR, SysInf.OutVol);
#if 0
	ioGain[Out2L_I2s2L] = SysInf.OutVol;
	ioGain[Out2R_I2s2R] = SysInf.OutVol;
	IOGainSet(Out2L_I2s2L, SysInf.OutVol);
	IOGainSet(Out2R_I2s2R, SysInf.OutVol);

	ioGain[Out2L_I2s3L] = SysInf.OutVol;
	ioGain[Out2R_I2s3R] = SysInf.OutVol;
	IOGainSet(Out2L_I2s3L, SysInf.OutVol);
	IOGainSet(Out2R_I2s3R, SysInf.OutVol);
#endif
}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);
	switch (EF_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		case 3:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3);	break;
		case 4:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);	break;
	}

}

//***************************************************
void EQ_ClewToneGr(uint8_t Gr)
{
	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);
	switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
	}
}

//******************************************************************
void user_gpio_isr(uint32_t io0_31, uint32_t io32_39)
{
	USER_DBG_INFO("io0_31:%p, io32_39:%p\n", io0_31, io32_39);
    extern void READ_BURST();	// KT56
	if(io0_31 & 0x300)	READ_BURST();		// gpio8,9 	MicA,8
//    if(io0_31 & 0x0008)	READ_BURST();		// gpio3 	MicB
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

uint16_t Ka_delay = 3000;

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
			freqSearch(510000, 530000, 0);
			freqSearch(530000, 550000, 2);

			if(SysInfR.MicCh_A != SysInf.MicCh_A || SysInfR.MicCh_B != SysInf.MicCh_B){
				SysInfR.MicCh_A = SysInf.MicCh_A;
				SysInfR.MicCh_B = SysInf.MicCh_B;
			//	GroupWrite(0, 2);
			}
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
				ioGain[Play_MusL] = SysInf.MusicVol;
				ioGain[Play_MusR] = SysInf.MusicVol;
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
//********************************
void WaveStop()
{
//	UpDisplayReg(LedFg, 1);
//	LedFg = 0;
}

#endif
#endif
