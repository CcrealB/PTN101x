//*****************************************************************************
//  File Name: main.c
//  Function:  KT Wireless Mic Receiver Products Demoboard
//*****************************************************************************
//        Revision History
//  Version Date        Description
//  V1.0    2012-08-28  Initial draft
//  V1.7    2017-02-10  删除KT_WirelessMicRx_CheckAUXCH(辅助信道状态变化监测程序)函数
//                  修改KT_MicRX_Batter_Detecter函数，使其读取电池电量信息的时候与
//                  发射端（KT0646M）程序位置相对应
//  V1.4N    2017-04-01 根据当前是主路或者从路显示相对应的RSSI及SNR,pilot及BPSK值(Main.c)
//						上电的时候先把chipen拉低再拉高(Main.c)
//						把I2C的速度改快了(I2C.c)
//						根据最新的命名规则进行了版本号修改
//  V1.5     2017-04-21 在主循环里面一直读取导频和SNR的值，并根据最新的值决定是否mute(Main.c)
//  V1.6     2017-06-05 原来读写电池电压的寄存器为0x029,应该为0x0249
//						把rfIntCtl();pilotMuteRefresh();snrMuteRefresh();移到了驱动文件(Main.c)
//  V1.7     2017-06-28 增加了BATTERY_Display函数，用来显示接收机的电池电压(Main.c)
//  V1.8     2017-09-18 增加了根据搜台功能宏定义开关来决定一些代码
//  V1.9     2017-11-21 由于修改了KT_WirelessMicRx_SAIInit函数，所以i2sInit也修改了
//  V1.10    2018-04-08 rfIntCtlChip, pilotMuteRefreshChip,snrMuteRefreshChip函数运行前判断一下芯片版本
//*****************************************************************************

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include ".\WMIC_KT\KT_WirelessMicRxdrv.h"

#ifdef A40_Effect
#define KT56_DBG_INFO(fmt,...)      os_printf("[KT56]"fmt, ##__VA_ARGS__)

char buffer[20];
//*******************************
void __delay_ms(uint16_t val)
{
	for(uint16_t i=0; i<val; i++){
		os_delay_us(1600);
	}
}
 
//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

uint16_t refreshTimer = 0;

uint8_t LDO_CTRL;

#define LDO_CTRL_WRITE0() LDO_CTRL=0
#define LDO_CTRL_WRITE1() LDO_CTRL=1

uint16_t CHA_Data[4],CHA_DataR[4];
uint16_t CHB_Data[4],CHB_DataR[4];

//*****************************************************
void KT_I2C_Init()
{
	SDAIN_MODE = GPIO_INOUT;
	gpio_config_new(SCL_IO, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(SDA1, GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
}
void Send_Inf(uint8_t comm, uint8_t val);

//*****************************************************
void Set_CH(uint8_t val)
{
	KT_I2C_Init();
	uint8_t  channel;
	uint32_t Frequency;
	uint8_t t=0, b=4;
	if(val ==1){
		t=0;	b=2;
	}else if(val ==2){		
		t=2;	b=4;
	}

	for(chipSel=t; chipSel<b; chipSel++){
		if(Chip_E[chipSel]){
			KT_WirelessMicRx_Volume(0);	  //关闭音量
			//----- 设定 CHA/CHB 接收频率 -------------------------------------------
    		if(chipSel<chipBM){	 //A 通道的主或者从
				channel = SysInf.MicCh_A;
				Send_Inf(0x45,channel+1);
				Frequency = Freq_Top + ((uint32_t)channel*Freq_Step);
			}else{
				channel = SysInf.MicCh_B;
				Send_Inf(0x46,channel+1);
				Frequency = Freq_Top + ((uint32_t)channel*Freq_Step);
			}
    		KT_WirelessMicRx_Tune(Frequency);
			__delay_ms(20);
    		KT_WirelessMicRx_Volume(PRESET_VOL);
//			printf("ChipSel: %d  CH: %d    freq: %ld\n",(uint16_t)chipSel, (uint16_t)channel+1, (long)Frequency);
		}
	}
	CHA_DataR[1] = ~CHA_Data[1];
	CHB_DataR[1] = ~CHB_Data[1];
}

//***********************************************************
uint32_t freqSearch(uint32_t startFreq,uint32_t stopFreq, uint8_t chip)
{
	KT_I2C_Init();
	if((chip != 0) && (chip != 2))	return 0;	
	KT_I2C_Init();

	uint32_t freqTemp,rssiMaxFreq;
	uint8_t rssiMin=0xff,rssiMax=0,rssi;

	KT56_DBG_INFO("====1. chip: %d \n",chip);

	chipSel = chip;
	//-------------------------------------------------------
	KT_WirelessMicRx_Volume(0);	  //关闭音量

//	printf("============================================\n");
	for(freqTemp=startFreq; freqTemp<stopFreq; freqTemp+=Freq_Step){
//		KT56_DBG_INFO("====3. freqTemp: %d  %d\n",freqTemp,chipSel);
		KT_WirelessMicRx_FastTune(freqTemp);
	  //rssi=KT_WirelessMicRx_GetFastRSSI();
		rssi=KT_WirelessMicRx_GetRSSI();
		
//		printf("chip: %d   freq: %ld   rssi: %d  CH:%d\n",(uint16_t)chipSel, (uint32_t)freqTemp, (uint16_t)rssi, (uint16_t)((freqTemp - Freq_Top)/Freq_Step)+1);
		if(rssi<rssiMin){
			rssiMin=rssi;
//			rssiMinFreq=freqTemp;
		}
		if(rssi>rssiMax){
			rssiMax=rssi;
			rssiMaxFreq=freqTemp;
		}

		if(rssiMax > 70){
//			KT56_DBG_INFO("==== chip: %d   Max Freq: %ld   rssi: %d\n",(uint16_t)chipSel, rssiMaxFreq, (uint16_t)rssiMax);
			if(chip){
				SysInf.MicCh_B = 	((rssiMaxFreq - Freq_Top)/Freq_Step);
				SysInf.BchPair = KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
				KT56_DBG_INFO("==== Search ChB Freq: %ld  Pair: %04X\n",rssiMaxFreq, SysInf.BchPair);
				SysInf.PAIR |= 0x81;
			//	Send_Inf(0x87,((rssiMaxFreq- Freq_Top)/Freq_Step)+1);
			}else{
				SysInf.MicCh_A = 	((rssiMaxFreq - Freq_Top)/Freq_Step);
				SysInf.AchPair =  KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
				KT56_DBG_INFO("==== Search ChA Freq: %ld  Pair: %04X\n",rssiMaxFreq, SysInf.AchPair);
				SysInf.PAIR |= 0x41;
			//	Send_Inf(0x86,((rssiMaxFreq - Freq_Top)/Freq_Step)+1);
			}
			break;
		}
	}
	if(SysInfR.MicCh_A != SysInf.MicCh_A ){
		SysInfR.MicCh_A = SysInf.MicCh_A;
		Set_CH(0);
	}
	if(SysInfR.MicCh_B != SysInf.MicCh_B){
		SysInfR.MicCh_B = SysInf.MicCh_B;
		Set_CH(2);
	}
	SDAIN_MODE = SDA_MODE_R;
	return 0;
}

uint8_t RX_COMM_DATA[5][2];
uint8_t RX_COMM_T,RX_COMM_B = 0;
//***************************************************************
void READ_BURST()
{
	KT_I2C_Init();

	uint16_t BURST_RegxA=0,BURST_RegxB=0,regx=0;
	if((Chip_E[0] && Chip_E[1]) || (Chip_E[2] && Chip_E[3]))  KT_WirelessMicRx_SelectMS();

	for(chipSel=0; chipSel<4; chipSel++){ 	// 清中断
		if(Chip_E[chipSel]){
			regx = KT_Bus_Read(0x0059,chipSel);
			if(regx & 0x40){
				//	DEBUG("==== BURST  Chip:%d  Regx: %04X \r\n",(uint16_t)chipSel, (uint16_t)regx);
				KT_Bus_Write(0x0059,regx&~0x40,chipSel);
				if(chipSel <2){
					BURST_RegxA = KT_Bus_Read(0x0246,MorSSelect)<<8 | KT_Bus_Read(0x0247,MorSSelect);
				}else{
					BURST_RegxB = KT_Bus_Read(0x0246,2+MorSSelect)<<8 | KT_Bus_Read(0x0247,2+MorSSelect);
				}
			}
		}
	}
	KT56_DBG_INFO("==== BURST  ChipA: %04X  ChipB: %04X \r\n",(uint16_t)BURST_RegxA, (uint16_t)BURST_RegxB);
	if((BURST_RegxA &0xFF) == (~(BURST_RegxA>>8)&0xFF)){
		if(++RX_COMM_T >4) RX_COMM_T = 0;
		RX_COMM_DATA[RX_COMM_T][0] = (BURST_RegxA &0xFF);
		RX_COMM_DATA[RX_COMM_T][1] = 0;
//		DBG_LOG_INFO("T1==== BURST: %d  %02X \r\n",(uint16_t)RX_COMM_T, (uint16_t)RX_COMM_DATA[RX_COMM_T][0]);
	}
	if((BURST_RegxB &0xFF) == (~(BURST_RegxB>>8)&0xFF)){
		if(++RX_COMM_T >4) RX_COMM_T = 0;
		RX_COMM_DATA[RX_COMM_T][0] = (BURST_RegxB &0xFF);
		RX_COMM_DATA[RX_COMM_T][1] = 2;
//		DBG_LOG_INFO("T2==== BURST: %d  %02X \r\n",(uint16_t)RX_COMM_T, (uint16_t)RX_COMM_DATA[RX_COMM_T][0]);
	}

//	ExtCtrl_Fun(chip, RX_COMM);
	SDAIN_MODE = SDA_MODE_R;
}

#include "karacmd.h"

//extern const int MusVol_Tab[10];
//extern const int MicVol_Tab[5];
extern uint8_t PLAY_EN;



extern uint8_t EqMode_Val;
extern uint8_t EfMode_Val;
extern uint8_t Mus_Val;
extern uint8_t Mic_Val;
extern uint8_t Key_Val;

//int app_button_next_action( void );
//int app_button_prev_action( void );
//int app_button_playpause_action( void );

//***************************************************************
void READ_BURST_FUN()
{
//	uint8_t TX_IR_DATA[5]={0x81,0x05,0x7A,0x00,0x00};

	if(++RX_COMM_B>4) RX_COMM_B = 0;
//	DBG_LOG_INFO("==== BURST: %d  %02X \r\n",(uint16_t)RX_COMM_B, (uint16_t)RX_COMM_DATA[RX_COMM_B][0]);
	switch(RX_COMM_DATA[RX_COMM_B][0]){
	//-------------------------------------
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
		EfMode_Val = RX_COMM_DATA[RX_COMM_B][0]-0x80;
		break;
	//-------------------------------------
	case 0x85:	// Music++
   		if(Mus_Val<9)	Mus_Val++;
   		Send_Inf(0x5E,Mus_Val);
		break;
	//-------------------------------------
	case 0x86:	// Music--
	  	if(Mus_Val)		Mus_Val--;
	  	Send_Inf(0x5E,Mus_Val);
		break;
	//-------------------------------------
	case 0x87:
//	  	Mus_Val=0;1
//		Mic_Val=0;
		break;
	//-------------------------------------
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
		Mic_Val = RX_COMM_DATA[RX_COMM_B][0]-0x88;
		break;
	//-------------------------------------
	case 0x8D:
	case 0x8E:
	case 0x8F:
		EqMode_Val = RX_COMM_DATA[RX_COMM_B][0]-0x8D;
		break;
	//======================================
	case 0x91:	 //Rep 			// 點歌機重唱
		Send_Inf(0xA5,0x01);
//		app_button_prev_action();
		break;
	//======================================
	case 0x92:	 //Clr			// 點歌機切歌
		Send_Inf(0xA4,0x01);
//		app_button_next_action();
		break;
	//======================================
	case 0x93:	 //Pause
		Send_Inf(0xA7,0x01);
//		app_button_playpause_action();
		break;
	//======================================
	case 0x94:	 //Voc
//		if(PLAY_EN == 0 ){
//		 	if(++Voice_Clr > 1) Voice_Clr = 0;	//點歌機非播放中 消音
//			YDA174_Voice(Voice_Clr);
//		}else{
			Send_Inf(0xA6,0x01);
//		}
		break;
	//======================================
	case 0x95:	 // 美聲 ON
//		Set_AutoTune(Enable, Normal, C_Major);
		break;
   //======================================
	case 0x96:	 // 美聲 OFF
//		Set_AutoTune(Disable, Normal, C_Major);
		break;
   //======================================
	case 0x97:	 // 男調
		Key_Val = 4;	//F->M
		break;
	//======================================
	case 0x98:	 // 女調
		Key_Val = -4; 	//M->F
		break;
	//======================================
	case 0x99:	 // 原調
		Key_Val = 0;
		break;
	//======================================
	case 0x9A:	 // 升調
		if(PLAY_EN == 0 ){
			if(Key_Val < 5) Key_Val++;
		}else{
	//		TX_IR_DATA[3]=0x4A;	//點歌機升調
	//		RemoteTx(TX_IR_DATA);
			Send_Inf(0xA1,0x01);
		}

//		Send_Inf(0x5B,Key_Val_Tab[Key_Val]);
		break;
	//======================================
	case 0x9B:	 // 降調
		if(PLAY_EN == 0 ){
			if((int8_t)Key_Val > -5) Key_Val--;
		}else{
	//		TX_IR_DATA[3]=0x4C;	//點歌機降調
	//		RemoteTx(TX_IR_DATA);
			Send_Inf(0xA3,0x01);
		}
		break;
	//======================================
	case 0xA1:	 // 聲控啟動

		if(RX_COMM_DATA[RX_COMM_B][1] ==0){	// A ch
			IOGainSet(I2s3R_Mic, -90);
			IOGainSet(I2s3R_I2s3R, 0);
//			ioGain[I2s3R_Mic] = -90;
//			ioGain[I2s3R_I2s3R] = 0;
//			KT56_DBG_INFO("==== BURST  0xA1  0\n");
		}else{	//B ch
			IOGainSet(I2s3L_Mic, -90);
			IOGainSet(I2s3L_I2s3L, 0);
//			ioGain[I2s3L_Mic] = -90;
//			ioGain[I2s3L_I2s3L] = 0;
//			KT56_DBG_INFO("==== BURST  0xA1  1\n");
		}
		Send_Inf(0x40,0x01);
		break;
	//======================================
	case 0xA0:	 // 聲控關閉
//		KT56_DBG_INFO("==== BURST  0xA0\n");
		IOGainSet(I2s3L_Mic, SysInf.MicVol);
		IOGainSet(I2s3R_Mic, SysInf.MicVol);
		IOGainSet(I2s3L_I2s3L, -90);
		IOGainSet(I2s3R_I2s3R, -90);
//		ioGain[I2s3R_Mic] =SysInf.MicVol;
//		ioGain[I2s3L_Mic] = SysInf.MicVol;
//		ioGain[I2s3R_I2s3R] = -90;
//		ioGain[I2s3L_I2s3L] = -90;
		break;
	}
/*
	//==============================================================================
	if(RX_COMM_DATA[RX_COMM_B][0] >=0x80 && RX_COMM_DATA[RX_COMM_B][0] <=0x84){
		Send_Inf(0x5A,EF_Mode);
	//	Send_Inf(0x5C,Echo_Val);
	}else if(RX_COMM_DATA[RX_COMM_B][0] >=0x88 && RX_COMM_DATA[RX_COMM_B][0] <=0x8C){
		Send_Inf(0x5D,RX_COMM_DATA[RX_COMM_B][0]-0x88);
	}else if(RX_COMM_DATA[RX_COMM_B][0] >=0x8D && RX_COMM_DATA[RX_COMM_B][0] <=0x8F){
		Send_Inf(0x5D,RX_COMM_DATA[RX_COMM_B][0]-0x8D);
	}
*/
}


uint8_t int_R = 2;
uint16_t A_AA, A_AB, A_AC, A_AD;
uint16_t B_AA, B_AB, B_AC, B_AD;
uint16_t A_Tol, A_Tol_R, B_Tol, B_Tol_R;
uint8_t Rssi[4];
uint8_t snr[4];
uint8_t cAF[4];
uint8_t RB[4];
uint16_t RegB;
uint8_t AUTOMUTE, AUTOMUTE_R = 0;

uint16_t RssiA_R=0,RssiB_R=0;
extern uint16_t Ka_delay;
//*************************************************
void KT56_main(void)
{
	KT_I2C_Init();
	uint8_t SDA_MODE_R = SDAIN_MODE;
	SDAIN_MODE = GPIO_INOUT;
//	DBG_LOG_INFO("KT56_Init =========================1\r\n");
	//==========================================
	if(RX_COMM_T != RX_COMM_B)	READ_BURST_FUN();

	//==========================================
	if(refreshTimer < 10){
    	refreshTimer++;
	}else{
		refreshTimer = 0;
		KT_WirelessMicRx_Patch();	//修复一些KT0656M的bug
	}

	/***********************************************
	AUX_ADDRA	0x07	0x0248	电池电量
	AUX_ADDRB	0x12    0x024A	配对 ID
	AUX_ADDRC	0x25	0x024C	频偏
	AUX_ADDRD	0x27	0x024E	频道信息
	***********************************************/
if(refreshTimer == 0){
	//===============================================================
	chipSel = 0;
	MorSSelect = 0;
	if(Chip_E[0] && Chip_E[1])  KT_WirelessMicRx_SelectMS();
	//--------------------------------------------------------
	if(Chip_E[MorSSelect]){
		Flag_AUTOMUTE[chipSel]=KT_WirelessMicRx_Automute();
		if(Flag_AUTOMUTE[chipSel]==MUTE){
//			LED_CHA = 1;
			KT_WirelessMicRx_Volume(0);
			CHA_DataR[1] = ~CHA_Data[1];
    	}else{
//			LED_CHA = 0;
			cAF[chipSel]=KT_WirelessMicRx_GetAF();
			snr[chipSel]=KT_WirelessMicRx_GetSNR();
//			DISPLAY_AF(cAF[chipSel]/1.5, 1);

			chipSel = MorSSelect;
			CHA_Data[0] = KT_Bus_Read(0x0248,chipSel)<<8 | KT_Bus_Read(0x0249,chipSel);
			CHA_Data[1] = KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
			CHA_Data[2] = KT_Bus_Read(0x024C,chipSel)<<8 | KT_Bus_Read(0x024D,chipSel);
			CHA_Data[3] = KT_Bus_Read(0x024E,chipSel)<<8 | KT_Bus_Read(0x024F,chipSel);
			chipSel = 0;
			if(CHA_DataR[0] != CHA_Data[0]){
				CHA_DataR[0] = CHA_Data[0];
//				DISPLAY_BATT((CHA_DataR[0]-1000)/230, 1);
			}
#if 1
			if(CHA_DataR[1] != CHA_Data[1]){
				CHA_DataR[1] = CHA_Data[1];
				if(SysInf.PAIR&0x40){
					SysInf.PAIR &= (~0x40);
					SysInf.AchPair = CHA_Data[1];
					GroupWrite(0, 2);
					Send_Inf(0x45,SysInf.MicCh_A+1);
				}
				if((CHA_Data[1] == SysInf.AchPair) || SysInf.PAIR == 0){
					KT_WirelessMicRx_Volume(PRESET_VOL);
					KT56_DBG_INFO("==== ChA Pair Ok: %04X  %04X  %d\n",(uint16_t)CHA_Data[1],(uint16_t)SysInf.AchPair, PRESET_VOL);
				}else{
					KT_WirelessMicRx_Volume(0);
					KT56_DBG_INFO("xxxx ChA Pair Err: %04X  %04X\n",(uint16_t)CHA_Data[1],(uint16_t)SysInf.AchPair);
				}
			}
#else

			KT_WirelessMicRx_Volume(PRESET_VOL);
#endif
		}

	}else{
//		LED_CHA = 1;
	}

}else if(refreshTimer == 2){
	chipSel = 0;
	Rssi[chipSel]=KT_WirelessMicRx_GetRSSI();
	if(Rssi[0] <30) Rssi[0] = 0;
	if(Rssi[0] >100) Rssi[0] = 100;
	if(abs(RssiA_R - Rssi[0]) > 5 ){
		RssiA_R = Rssi[0];
		if(Ka_delay==0)	Send_Inf(0x43,RssiA_R/10);
	}
#ifdef	__OLED_H
	sprintf(buffer, "A:%2d %2d",KtSysInf.MicCh_A,Rssi[chipSel]);
	OLED_x8y16str(0,0,buffer);
#endif

//	DBG_LOG_INFO("KT56_Init =========================2\r\n");

}else if(refreshTimer == 5){
	//===============================================================
	chipSel = 2;
	MorSSelect = 0;
	if(Chip_E[2] && Chip_E[3])  KT_WirelessMicRx_SelectMS();
//	KT56_DBG_INFO("==== Chip_E: %d  %d  %d  %d\n",Chip_E[2], Chip_E[3],MorSSelect,Flag_AUTOMUTE[chipSel]);
	//--------------------------------------------------------
	if(Chip_E[2+MorSSelect]){
		Flag_AUTOMUTE[chipSel]=KT_WirelessMicRx_Automute();
    	if(Flag_AUTOMUTE[chipSel]==MUTE){
//			LED_CHB = 1;
			KT_WirelessMicRx_Volume(0);
			CHB_DataR[1] = ~CHB_Data[1];
		}else{
//			LED_CHB = 0;
			cAF[chipSel]=KT_WirelessMicRx_GetAF();
			snr[chipSel]=KT_WirelessMicRx_GetSNR();
//			DISPLAY_AF(cAF[chipSel]/1.5, 2);

			chipSel = 2+MorSSelect;
			CHB_Data[0] = KT_Bus_Read(0x0248,chipSel)<<8 | KT_Bus_Read(0x0249,chipSel);
			CHB_Data[1] = KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
			CHB_Data[2] = KT_Bus_Read(0x024C,chipSel)<<8 | KT_Bus_Read(0x024D,chipSel);
			CHB_Data[3] = KT_Bus_Read(0x024E,chipSel)<<8 | KT_Bus_Read(0x024F,chipSel);
//			printf("==== CHA AUX DATA: %04X  %04X  %04X  %04X\n",(uint16_t)CHB_Data[0],(uint16_t)CHB_Data[1],(uint16_t)CHB_Data[2],(uint16_t)CHB_Data[3]);
			chipSel = 2;
			if(CHB_DataR[0] != CHB_Data[0]){
				CHB_DataR[0] = CHB_Data[0];
//				DISPLAY_BATT((CHB_DataR[0]-1000)/230, 2);
			}
#if 1
			if(CHB_DataR[1] != CHB_Data[1]){
				CHB_DataR[1] = CHB_Data[1];
				if(SysInf.PAIR&0x80){
					SysInf.PAIR &= (~0x80);
					SysInf.BchPair = CHB_Data[1];
					GroupWrite(0, 2);
					Send_Inf(0x46,SysInf.MicCh_B+1);
				}
				if((CHB_Data[1] == SysInf.BchPair) || SysInf.PAIR == 0){
					KT_WirelessMicRx_Volume(PRESET_VOL);
					KT56_DBG_INFO("==== ChB Pair Ok: %04X  %04X  %d\n",(uint16_t)CHB_Data[1],(uint16_t)SysInf.BchPair, PRESET_VOL);
				}else{
					KT_WirelessMicRx_Volume(0);
					KT56_DBG_INFO("xxxx ChB Pair Err: %04X  %04X\n",(uint16_t)CHB_Data[1],(uint16_t)SysInf.BchPair);
				}
			}
#else
			KT_WirelessMicRx_Volume(PRESET_VOL);
#endif
		}

	}else{
//		LED_CHB = 1;
	}

}else if(refreshTimer == 7){
	chipSel = 2;
	Rssi[chipSel]=KT_WirelessMicRx_GetRSSI();
	if(Rssi[2] <30) Rssi[2] = 0;
	if(Rssi[2] >100) Rssi[2] = 100;
	if(abs(RssiB_R - Rssi[2]) > 5 ){
		RssiB_R = Rssi[2];
		if(Ka_delay==0)	Send_Inf(0x44,RssiB_R/10);
	}
#ifdef	__OLED_H
	sprintf(buffer, "B:%2d %2d",KtSysInf.MicCh_B,Rssi[chipSel]);
	OLED_x8y16str(64,0,buffer);
#endif
}

}

//**** KT56初始化 *********************************************
void KT56_Init (void)
{
	KT_I2C_Init();

	uint8_t i;
    for(i=0; i<4;i++)	Flag_AUTOMUTE[i]=1;

	__delay_ms(80);
	//=====================================================================================
	for(chipSel=0; chipSel<4; chipSel++){
		//-------------------------------------------
		Chip_E[chipSel] = KT_WirelessMicRx_PreInit();	//芯片读写是否正常, 关闭DC/DC, 关闭音量输出
	}
	DBG_LOG_INFO("==== KT56_Initok Chip_EN: %1d %1d %1d %1d\r\n",Chip_E[0],Chip_E[1],Chip_E[2],Chip_E[3]);
#ifdef	__OLED_H
	sprintf(buffer, "ChipEN: %1d %1d %1d %1d",Chip_E[0],Chip_E[1],Chip_E[2],Chip_E[3]);
	OLED_x8y16str(0,0,buffer);
#endif

//	delay_ms(200);
	//=====================================================================================
    for(chipSel=0; chipSel<4; chipSel++){
		if(Chip_E[chipSel]){

			//------------------------
			KT_WirelessMicRx_Init();

    		if(chipSel<chipBM){	 //A 通道的主或者从
				KT_WirelessMicRx_SW_Diversity(Chip_E[1]);	// 天线分集开关程序
    		}else{
				KT_WirelessMicRx_SW_Diversity(Chip_E[3]);	// 天线分集开关程序
    		}
 	
			/**** 4.18.9. INT_EN (Address 0x55) ************************************************************************************
			Bit		名称					读写方式	默认值		功能描述
			7		INT_LEVEL				R/W			1,b0		中断输出有效电平选择位：0：中断输出高电平；1：中断输出低电平。
			6		BURST_INT_EN			R/W			1,b0		辅助数字信道接收到突发数据中断使能位：0：关闭；1：使能。
			5		AUTOMUTE_INT_EN			R/W			1,b0		自动静音功能中断使能位：0：关闭；1：使能。
			4		POWER_UP_FINISH_INT_EN	R/W			1,b0		芯片上电完成中断使能位：0：关闭；1：使能。
			3:0		Reserved				R/W			4,b0000		保留位。
			************************************************************************************************************************/
			KT_Bus_Write(0x0055,0xC0,chipSel);	//开启
		}
	}
	Set_CH(0);
	Set_CH(2);
	//=================================================
	if(Chip_E[0] && Chip_E[2]){
//		printf("==== KT56_Init ok...   Time: %d =============\n",(uint16_t)ms_cont);
	}else{
//		printf("==== KT56_Init Err!!!  Time: %d =============\n",(uint16_t)ms_cont);
	}

}
#endif
