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

#if defined(CPS_TL820)

#define KT56_DBG_INFO(fmt,...)      os_printf("[KT56]"fmt, ##__VA_ARGS__)

char buffer[20];
uint16_t MicA_battery;
uint16_t MicB_battery;
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

//void Send_Inf(uint8_t comm, uint8_t val);
uint8_t KT56_INT=0;
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
//				Send_Inf(0x45,channel+1);
			#ifdef LCD_ST7789_EN
				LCD_ShowxNum(47, 48, channel+1, 2, 16, 0);//// 频道CH
			#endif
				Frequency = Freq_Top + ((uint32_t)channel*Freq_Step);
			}else{
				channel = SysInf.MicCh_B;
//				Send_Inf(0x46,channel+1);
			#ifdef LCD_ST7789_EN
				LCD_ShowxNum(211,  48, channel+1, 2, 16, 0);//// 频道CH
			#endif
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
	uint32_t freqTemp,rssiMinFreq;
//	uint32_t freqTemp,rssiMaxFreq;
	uint8_t rssiMin=0xff, rssi;//,rssiMax=0;

	KT56_DBG_INFO("====1. chip: %d \n",chip);
	if((chip != 0) && (chip != 2))	return 0;
	KT56_DBG_INFO("====2. chip: %d \n",chip);

	chipSel = chip;
	//-------------------------------------------------------
	KT_WirelessMicRx_Volume(0);	  //关闭音量
//	KT56_DBG_INFO("============================================\n");
	for(freqTemp=startFreq;freqTemp<stopFreq;freqTemp+=Freq_Step){
		KT_WirelessMicRx_FastTune(freqTemp);
		//rssi=KT_WirelessMicRx_GetFastRSSI();
		rssi=KT_WirelessMicRx_GetRSSI();
//		KT56_DBG_INFO("chip: %d   freq: %ld   rssi: %d  CH:%d\n",(uint16_t)chipSel, (uint32_t)freqTemp, (uint16_t)rssi, (uint16_t)((freqTemp - Freq_Top)/Freq_Step)+1);
		if(rssi<rssiMin){
			rssiMin=rssi;
			rssiMinFreq=freqTemp;
		}
//		if(rssi>rssiMax){
//			rssiMax=rssi;
//			rssiMaxFreq=freqTemp;
//		}
	}

/*
		if(rssiMax > 70){
//			KT56_DBG_INFO("==== chip: %d   Max Freq: %ld   rssi: %d\n",(uint16_t)chipSel, rssiMaxFreq, (uint16_t)rssiMax);
			if(chip){
				SysInf.MicCh_B = 	((rssiMaxFreq - Freq_Top)/Freq_Step);
				SysInf.BchPair = KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
				KT56_DBG_INFO("==== Search ChB Freq: %ld  Pair: %04X\n",rssiMaxFreq, SysInf.BchPair);
				SysInf.PAIR |= 0x81;
				CHB_DataR[1] = ~CHB_Data[1];
			//	Send_Inf(0x87,((rssiMaxFreq- Freq_Top)/Freq_Step)+1);
			#ifdef LCD_ST7789_EN
				LCD_ShowxNum(220,  5, ((rssiMaxFreq- Freq_Top)/Freq_Step)+1, 2, 24, 0);
			#endif
			}else{
				SysInf.MicCh_A = 	((rssiMaxFreq - Freq_Top)/Freq_Step);
				SysInf.AchPair =  KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
				KT56_DBG_INFO("==== Search ChA Freq: %ld  Pair: %04X\n",rssiMaxFreq, SysInf.AchPair);
				SysInf.PAIR |= 0x41;
				CHA_DataR[1] = ~CHA_Data[1];
			//	Send_Inf(0x86,((rssiMaxFreq - Freq_Top)/Freq_Step)+1);
			#ifdef LCD_ST7789_EN
				LCD_ShowxNum(65, 5, ((rssiMaxFreq - Freq_Top)/Freq_Step)+1, 2, 24, 0);
			#endif
			}
			return;
		}
*/
		KT56_DBG_INFO("==== chip:%d  Freq:%ld  Min:%ld rssi:%d\n",(uint16_t)chipSel, freqTemp, rssiMinFreq, (uint16_t)rssiMin);


	return rssiMinFreq;
}

extern uint8_t IrTxTime;	// step 0.5s
extern uint8_t IrTxData[8];
#define UserId	IrTxData[2]	//用户ID
#define CH_AB	IrTxData[3]	//A/B话筒	 0：A话筒（左路），1：B话筒（右路）
#define CH_NUM  IrTxData[4]	//通道值（范围为1-200）
#define CH_FreqH  IrTxData[5]
#define CH_FreqL  IrTxData[6]
//****************************************
void KT56_IrPair(uint8_t AB)
{
	KT_I2C_Init();
	uint16_t FreqK;
	uint32_t FreqW;
	if(AB){	// CHB
		FreqW = freqSearch(530000, 550000, 2);
		FreqK = (FreqW/100);
		CH_FreqL = FreqK&0xFF;
		CH_FreqH = (FreqK>>8)&0xFF;
		CH_AB = 1;
		CH_NUM = ((FreqW- Freq_Top)/Freq_Step)+1;
		SysInf.MicCh_B = CH_NUM-1;
		SysInf.PAIR |= 0x80;
		Set_CH(2);
	}else{
		FreqW = freqSearch(510000, 530000, 0);
		FreqK = (FreqW/100);
		CH_FreqL = FreqK&0xFF;
		CH_FreqH = (FreqK>>8)&0xFF;
		CH_AB = 0;
		CH_NUM = ((FreqW- Freq_Top)/Freq_Step)+1;
		SysInf.MicCh_A = CH_NUM-1;
		SysInf.PAIR |= 0x40;
		Set_CH(1);
	}
	IrTxData[7] = IrTxData[0]^IrTxData[1]^IrTxData[2]^IrTxData[3]^IrTxData[4]^IrTxData[5]^IrTxData[6];
	USER_DBG_INFO("TX_DATA:%02X %02X %02X %02X %02X %02X %02X %02X\n",
		IrTxData[0],IrTxData[1],IrTxData[2],IrTxData[3],IrTxData[4],IrTxData[5],IrTxData[6],IrTxData[7]);
}

//****************************************
void KT56_IrPair_Freq(uint8_t UpDn)
{
	KT_I2C_Init();

	uint16_t FreqK;
	uint32_t FreqW;
	if(CH_AB){	// CHB
		if(UpDn){
			if(SysInf.MicCh_B<79)	SysInf.MicCh_B++;
		}else{
			if(SysInf.MicCh_B>40)	SysInf.MicCh_B--;
		}
		FreqW = Freq_Top + ((uint32_t)SysInf.MicCh_B*Freq_Step);
		FreqK = (FreqW/100);
		CH_FreqL = FreqK&0xFF;
		CH_FreqH = (FreqK>>8)&0xFF;
		CH_AB = 1;
		CH_NUM = SysInf.MicCh_B+1;
		Set_CH(2);
	}else{
		if(UpDn){
			if(SysInf.MicCh_A<39)	SysInf.MicCh_A++;
		}else{
			if(SysInf.MicCh_A)		SysInf.MicCh_A--;
		}
		FreqW = Freq_Top + ((uint32_t)SysInf.MicCh_A*Freq_Step);
		FreqK = (FreqW/100);
		CH_FreqL = FreqK&0xFF;
		CH_FreqH = (FreqK>>8)&0xFF;
		CH_AB = 0;
		CH_NUM = SysInf.MicCh_A+1;
		Set_CH(1);
	}
	IrTxData[7] = IrTxData[0]^IrTxData[1]^IrTxData[2]^IrTxData[3]^IrTxData[4]^IrTxData[5]^IrTxData[6];
	USER_DBG_INFO("TX_DATA:%02X %02X %02X %02X %02X %02X %02X %02X\n",
		IrTxData[0],IrTxData[1],IrTxData[2],IrTxData[3],IrTxData[4],IrTxData[5],IrTxData[6],IrTxData[7]);
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

//*************************************************
void KT56_main(void)
{
	KT_I2C_Init();

	if(Chip_E[0]==0 && Chip_E[2]==0)	return;
	if(KT56_INT){
		KT56_INT=0;
		READ_BURST();
	}
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

	Tx端通过以下辅助通道传输一些关键信息，对应的Rx端可以读取这些寄存器.
	1.使用AUX_ADDRA传输电池电压量化值val, (范围0~2048)，
	计算实际电池电压的公式Vbat = val/2048*1.2*4(V).
	***********************************************/
	switch(refreshTimer){
	//-------------------------------------
	case 0:
		//===============================================================
		chipSel = 0;
		MorSSelect = 0;
		if(Chip_E[0] && Chip_E[1])  KT_WirelessMicRx_SelectMS();
		//--------------------------------------------------------
		if(Chip_E[MorSSelect]){
			Flag_AUTOMUTE[chipSel]=KT_WirelessMicRx_Automute();
			if(Flag_AUTOMUTE[chipSel]==MUTE){
//				LED_CHA = 1;
				KT_WirelessMicRx_Volume(0);
			#ifdef LCD_ST7789_EN
				RssiA_R =0;
				MicA_battery=0;
				ShowRssiAch(RssiA_R);
				ShowBattAch(MicA_battery);
			#endif
				CHA_DataR[0] = ~CHA_Data[0];
				CHA_DataR[1] = ~CHA_Data[1];
			}else{
//				LED_CHA = 0;
				cAF[chipSel]=KT_WirelessMicRx_GetAF();
				snr[chipSel]=KT_WirelessMicRx_GetSNR();
//				DISPLAY_AF(cAF[chipSel]/1.5, 1);

				chipSel = MorSSelect;
				CHA_Data[0] = KT_Bus_Read(0x0248,chipSel)<<8 | KT_Bus_Read(0x0249,chipSel);
				CHA_Data[1] = KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
				CHA_Data[2] = KT_Bus_Read(0x024C,chipSel)<<8 | KT_Bus_Read(0x024D,chipSel);
				CHA_Data[3] = KT_Bus_Read(0x024E,chipSel)<<8 | KT_Bus_Read(0x024F,chipSel);
				chipSel = 0;
				//-------------------------------------------------------------
				if(CHA_DataR[0] != CHA_Data[0]){
					CHA_DataR[0] = CHA_Data[0];
					if((CHA_Data[3]>>8)+(CHA_Data[3]&0xFF)==0xFF){
						// n=1.725 n1160=2v  n1740=3v	1740-1160=580
						MicA_battery=(CHA_DataR[0]-1160)/5.8;
					}else{
						// n=2.344 n1280=3v  n1792=4.2v		1792-1280=512
						MicA_battery=(CHA_DataR[0]-1280)/5.12;
					}
					KT56_DBG_INFO("=============MicA_RData: %04X %04X %04X %04X  bat:%d\n",CHA_Data[0],CHA_Data[1],CHA_Data[2],CHA_Data[3],MicA_battery);
					ShowBattAch(MicA_battery);
				}
				//-------------------------------------------------------------
				if(CHA_DataR[1] != CHA_Data[1]){
					CHA_DataR[1] = CHA_Data[1];
					if(SysInf.PAIR&0x40){
						SysInf.PAIR &= (~0x40);
						SysInf.AchPair = CHA_Data[1];
						GroupWrite(0, 2);
//						Send_Inf(0x45,SysInf.MicCh_A+1);
					}
					if((CHA_Data[1] == SysInf.AchPair) || SysInf.PAIR == 0){
						if(CH_AB==0) IrTxTime = 0;
						KT_WirelessMicRx_Volume(PRESET_VOL);
						KT56_DBG_INFO("==== ChA Pair Ok: Rid:%04X Pid:%04X PEn:%d Vol:%d\n",(uint16_t)CHA_Data[1],(uint16_t)SysInf.AchPair, SysInf.PAIR, PRESET_VOL);
					}else{
						KT_WirelessMicRx_Volume(0);
						KT56_DBG_INFO("xxxx ChA Pair Err: Rid:%04X  Pid:%04X\n",(uint16_t)CHA_Data[1],(uint16_t)SysInf.AchPair);
					}
				}
			}

		}else{
//			LED_CHA = 1;
		}
		break;
	case 2:
		if(Flag_AUTOMUTE[0]!=MUTE){
			chipSel = 0;
			Rssi[chipSel]=KT_WirelessMicRx_GetRSSI();
			if(Rssi[0] <30) Rssi[0] = 0;
			if(Rssi[0] >100) Rssi[0] = 100;
			if(abs(RssiA_R - Rssi[0]) > 5 ){
				RssiA_R = Rssi[0];
//				if(Ka_delay==0)	Send_Inf(0x43,RssiA_R/10);
				KT56_DBG_INFO("=================RssiA_R=%d\n",RssiA_R);
			#ifdef LCD_ST7789_EN
				ShowRssiAch(RssiA_R);
			#endif
			}
		}
		break;
	case 5:
		//===============================================================
		chipSel = 2;
		MorSSelect = 0;
		if(Chip_E[2] && Chip_E[3])  KT_WirelessMicRx_SelectMS();
		//--------------------------------------------------------
		if(Chip_E[2+MorSSelect]){
			Flag_AUTOMUTE[chipSel]=KT_WirelessMicRx_Automute();
			if(Flag_AUTOMUTE[chipSel]==MUTE){
//				LED_CHB = 1;
				KT_WirelessMicRx_Volume(0);
			#ifdef LCD_ST7789_EN
				RssiB_R =0;
				MicB_battery=0;
				ShowRssiBch(RssiB_R);
				ShowBattBch(MicB_battery);
			#endif
				CHB_DataR[0] = ~CHB_Data[0];
				CHB_DataR[1] = ~CHB_Data[1];
			}else{
//				LED_CHB = 0;
				cAF[chipSel]=KT_WirelessMicRx_GetAF();
				snr[chipSel]=KT_WirelessMicRx_GetSNR();
//				DISPLAY_AF(cAF[chipSel]/1.5, 2);

				chipSel = 2+MorSSelect;
				CHB_Data[0] = KT_Bus_Read(0x0248,chipSel)<<8 | KT_Bus_Read(0x0249,chipSel);
				CHB_Data[1] = KT_Bus_Read(0x024A,chipSel)<<8 | KT_Bus_Read(0x024B,chipSel);
				CHB_Data[2] = KT_Bus_Read(0x024C,chipSel)<<8 | KT_Bus_Read(0x024D,chipSel);
				CHB_Data[3] = KT_Bus_Read(0x024E,chipSel)<<8 | KT_Bus_Read(0x024F,chipSel);
//				printf("==== CHA AUX DATA: %04X  %04X  %04X  %04X\n",(uint16_t)CHB_Data[0],(uint16_t)CHB_Data[1],(uint16_t)CHB_Data[2],(uint16_t)CHB_Data[3]);
				//-------------------------------------------------------
				chipSel = 2;
				if(CHB_DataR[0] != CHB_Data[0]){
					CHB_DataR[0] = CHB_Data[0];
					if((CHB_Data[3]>>8)+(CHB_Data[3]&0xFF)==0xFF){
						// n=1.725 n1160=2v  n1740=3v	1740-1160=580
						MicB_battery=(CHB_DataR[0]-1160)/5.8;
					}else{
						// n=2.344 n1280=3v  n1792=4.2v		1792-1280=512
						MicB_battery=(CHB_DataR[0]-1280)/5.12;
					}
					KT56_DBG_INFO("=============MicB_RData: %04X %04X %04X %04X  bat:%d\n",CHB_Data[0],CHB_Data[1],CHB_Data[2],CHB_Data[3],MicB_battery);
					ShowBattBch(MicB_battery);

				}
				//-------------------------------------------------------
				if(CHB_DataR[1] != CHB_Data[1]){
					CHB_DataR[1] = CHB_Data[1];
					if(SysInf.PAIR&0x80){
						SysInf.PAIR &= (~0x80);
						SysInf.BchPair = CHB_Data[1];
						GroupWrite(0, 2);
//						Send_Inf(0x46,SysInf.MicCh_B+1);
					}
					if((CHB_Data[1] == SysInf.BchPair) || SysInf.PAIR == 0){
						if(CH_AB) IrTxTime = 0;
						KT_WirelessMicRx_Volume(PRESET_VOL);
						KT56_DBG_INFO("==== ChB Pair Ok: Rid:%04X Pid:%04X PEn:%d Vol:%d\n",(uint16_t)CHB_Data[1],(uint16_t)SysInf.BchPair, SysInf.PAIR, PRESET_VOL);
					}else{
						KT_WirelessMicRx_Volume(0);
						KT56_DBG_INFO("xxxx ChB Pair Err: Rid:%04X  Pid:%04X\n",(uint16_t)CHB_Data[1],(uint16_t)SysInf.BchPair);
					}
				}
			}
		}else{
//			LED_CHB = 1;
		}
		break;
	case 7:
		if(Flag_AUTOMUTE[2]!=MUTE){
			chipSel = 2;
			Rssi[chipSel]=KT_WirelessMicRx_GetRSSI();
			if(Rssi[2] <30) Rssi[2] = 0;
			if(Rssi[2] >100) Rssi[2] = 100;
			if(abs(RssiB_R - Rssi[2]) > 5 ){
				RssiB_R = Rssi[2];
//				if(Ka_delay==0)	Send_Inf(0x44,RssiB_R/10);
			#ifdef LCD_ST7789_EN
				ShowRssiBch(RssiB_R);
			#endif
			}
		}
		break;
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
	KT56_DBG_INFO("==== KT56_Initok Chip_EN: %1d %1d %1d %1d\r\n",Chip_E[0],Chip_E[1],Chip_E[2],Chip_E[3]);
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
	Set_CH(1);
	Set_CH(2);
	//=================================================
}
#endif
