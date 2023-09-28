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
//						把I2C的速度改快了(I2C.c)5
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
#include "USER_Config.h"

#ifdef MI_0656

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
				Frequency = Freq_Top + ((uint32_t)channel*Freq_Step);
			}else{
				channel = SysInf.MicCh_B;
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
//	uint32_t freqTemp,rssiMinFreq,rssiMaxFreq;
	uint32_t freqTemp,rssiMaxFreq;
	uint8_t rssiMin=0xff,rssiMax=0,rssi;

	if(chip != 0 && chip != 2)	return;

	chipSel = chip;
	//-------------------------------------------------------
	KT_WirelessMicRx_Volume(0);	  //关闭音量
//	printf("============================================\n");
	for(freqTemp=startFreq;freqTemp<stopFreq;freqTemp+=Freq_Step){
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
	}
	if(rssiMax > 40){
//		printf("==== chip: %d   Max Freq: %ld   rssi: %d\n",(uint16_t)chipSel, rssiMaxFreq, (uint16_t)rssiMax);
		if(chip){
			SysInf.MicCh_B = 	((rssiMaxFreq - Freq_Top)/Freq_Step);
			Set_CH(1);
		}else{
			SysInf.MicCh_A = 	((rssiMaxFreq - Freq_Top)/Freq_Step);
			Set_CH(2);
		}
	}else{
//		printf("==== chip: %d   Min Freq: %ld   rssi: %d\n",(uint16_t)chipSel, rssiMinFreq, (uint16_t)rssiMin);
		if(chip){  //BCH
//			Send_Inf(0x87,((rssiMin- Freq_Top)/Freq_Step)+1);
//			if(MIC_SW==0)	EEROM_R[MIC_CH2] = 	((rssiMinFreq - 510000)/500);	
		}else{	   //ACH
//			Send_Inf(0x86,((rssiMin - Freq_Top)/Freq_Step)+1);
//			if(MIC_SW==0)	EEROM_R[MIC_CH1] = 	((rssiMinFreq - 510000)/500);
		}
	}
//	printf("============================================\n");
}

uint8_t RX_COMM_DATA[5][2];
uint8_t RX_COMM_T,RX_COMM_B = 0;
//***************************************************************
void READ_BURST()
{
	KT_I2C_Init();
	uint16_t BURST_RegxA,BURST_RegxB,regx;
	if((Chip_E[0] && Chip_E[1]) || (Chip_E[2] && Chip_E[3]))  KT_WirelessMicRx_SelectMS();
	if(Chip_E[0])	BURST_RegxA = KT_Bus_Read(0x0246,MorSSelect)<<8 | KT_Bus_Read(0x0247,MorSSelect);
	if(Chip_E[2])	BURST_RegxB = KT_Bus_Read(0x0246,2+MorSSelect)<<8 | KT_Bus_Read(0x0247,2+MorSSelect);

	for(chipSel=0; chipSel<4; chipSel++){ 	// 清中断
		if(Chip_E[chipSel]){
			regx = KT_Bus_Read(0x0059,chipSel);
	//		DBG_LOG_INFO("==== BURST  Chip:%d  Regx: %04X \r\n",(uint16_t)chipSel, (uint16_t)regx);
			KT_Bus_Write(0x0059,regx&~0x40,chipSel);
		}
	}
	DBG_LOG_INFO("==== BURST  ChipA: %04X  ChipB: %04X \r\n",(uint16_t)BURST_RegxA, (uint16_t)BURST_RegxB);
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
}

//***************************************************************
void READ_BURST_FUN()
{
//	uint8_t TX_IR_DATA[5]={0x81,0x05,0x7A,0x00,0x00};

	if(++RX_COMM_B>4) RX_COMM_B = 0;
//	DBG_LOG_INFO("==== BURST: %d  %02X \r\n",(uint16_t)RX_COMM_B, (uint16_t)RX_COMM_DATA[RX_COMM_B][0]);
	switch(RX_COMM_DATA[RX_COMM_B][0]){
	//---- Ef Mode ------------------------
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
		break;
	//---- Music++ ------------------------
	case 0x85:

		break;
	//---- Music-- ------------------------
	case 0x86:

		break;
	//-------------------------------------
	case 0x87:	// Mic Vol 0

		break;
	//---- Mic Vol 1~4 -------------------
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
		break;
	//---- EQ Mode ------------------------
	case 0x8D:
	case 0x8E:
	case 0x8F:

		break;
	//======================================
	case 0x91:	 //Rep 			// 點歌機重唱

		break;
	//======================================
	case 0x92:	 //Clr			// 點歌機切歌

		break;
	//======================================
	case 0x93:	 //Pause

		break;
	//======================================
	case 0x94:	 //Voc

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
//		Key_Val = 4;	//F->M
		break;
	//======================================
	case 0x98:	 // 女調
		break;
	//======================================
	case 0x99:	 // 原調
		break;
	//======================================
	case 0x9A:	 // 升調
		break;
	//======================================
	case 0x9B:	 // 降調
		break;
	//======================================
	case 0xA1:	 // 聲控啟動

		break;
	//======================================
	case 0xA0:	 // 聲控關閉
//	case 0:

		break;
	}

}


const int8 MicVol[16]={-90,-14,-13,-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0};
const int8 MicRev[16]=	{0,1,2,3,4,5,6,6,6,6,6,6,6,6,6,6};
const int8 MicEcho[16]=	{0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9};

//=================================
void CommFun(uint16_t comm, uint16_t pair, uint8_t AB)
{
	uint8_t type = (comm>>8)&0xFF;
	uint8_t val =  comm&0xFF;
	if(type==0x88){			// SET ChA
		SysInf.MicCh_A = val-1;
	}else if(type==0x89){	// SET ChB
		SysInf.MicCh_B = val+19;
	}else if(type==0xE0){	// 配對
		if(AB==0){
			SysInf.AchPair = pair;
			DBG_LOG_INFO("====== SysInf.AchPair:%08X\n", SysInf.AchPair);
		}else{
			SysInf.BchPair = pair;
			DBG_LOG_INFO("====== SysInf.BchPair:%08X\n", SysInf.BchPair);
		}
	}else if(type==0xE1){	// 跳頻
		if(AB==0){
			SysInf.MicCh_A = val-1;
			Set_CH(1);
			DBG_LOG_INFO("====== SysInf.MicCh_A:%d\n", SysInf.AchPair);
		}else{
			SysInf.MicCh_B = val-1;
			Set_CH(2);
			DBG_LOG_INFO("====== SysInf.MicCh_B:%d\n", SysInf.BchPair);
		}
	}
}

//=================================
void CtrlFun(uint16_t ctrl)
{
	uint8_t type = (ctrl>>12)&0xF;
	uint8_t val =  (ctrl>>8)&0xF;
	if(type==1){
		UserEf.RevVol = (float)MicRev[val]/10;
		UserEf.EchoVol = (float)MicEcho[val]/10;
	}else if(type==2){
		SysInf.MicVol = (float)MicVol[val];
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

extern uint16_t Ka_delay;
extern uint8_t LED_CHA,LED_CHB;
extern uint8_t ShowCh;
extern void UpDataInf(uint8_t ch);
uint8_t ChA_w = 0,ChB_w = 20;
uint8_t ChAScanDelay=0, ChBScanDelay=0;
//*************************************************
void KT56_main(void)
{
	KT_I2C_Init();
#if 1
if(refreshTimer == 0){
	if(ChAScanDelay)	ChAScanDelay--;
	//===========================================
	if(Flag_AUTOMUTE[0]==MUTE && ChAScanDelay==0){
		chipSel = 0;
		uint32_t freqTemp = Freq_Top + ((uint32_t)ChA_w*Freq_Step);
		KT_WirelessMicRx_FastTune(freqTemp);
		uint8 rssi = KT_WirelessMicRx_GetRSSI();
		if(rssi <40){
//			DBG_LOG_INFO("CHA:%d  rssi: %d\n", ChA_w, rssi );
			if(++ChA_w >19) ChA_w =0;
		}else{
			ShowCh = 0;
			UpDataInf(ShowCh);
			SysInf.MicCh_A = ChA_w;
			if(Chip_E[1]){
				chipSel = 1;
				KT_WirelessMicRx_FastTune(freqTemp);
			}
			ChAScanDelay = 50;
		}
	}

}else if(refreshTimer == 5){

	if(ChBScanDelay)	ChBScanDelay--;
	//===========================================
	if(Flag_AUTOMUTE[2]==MUTE && ChBScanDelay==0){
		chipSel = 2;
		uint32_t freqTemp = Freq_Top + ((uint32_t)ChB_w*Freq_Step);
		KT_WirelessMicRx_FastTune(freqTemp);
		uint8 rssi = KT_WirelessMicRx_GetRSSI();
		if(rssi <40){
//			DBG_LOG_INFO("CHB:%d  rssi: %d\n", ChB_w,rssi );
			if(++ChB_w >39) ChB_w =20;
		}else{
			ShowCh = 1;
			UpDataInf(ShowCh);
			SysInf.MicCh_B = ChB_w;
			if(Chip_E[3]){
				chipSel = 3;
				KT_WirelessMicRx_FastTune(freqTemp);
			}
			ChBScanDelay = 50;
		}
	}
}
#endif
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
		AUX_ADDRA	0x07	0x0248	电池电量   	ID
		AUX_ADDRB	0x12    0x024A	配对 ID	高位 0x88:ChA, 0x89ChB, 0xE0配對, 0xE1跳頻/ 低位:CH
		AUX_ADDRC	0x25	0x024C	频偏		高位 4it 1:ECHO ,2:VOL
		AUX_ADDRD	0x27	0x024E	频道信息	不使用
		***********************************************/
	if(refreshTimer == 1){
		//===============================================================
		chipSel = 0;
		MorSSelect = 0;
		if(Chip_E[0] && Chip_E[1])  KT_WirelessMicRx_SelectMS();
		//--------------------------------------------------------
		if(Chip_E[0]){
			Flag_AUTOMUTE[0]=KT_WirelessMicRx_Automute();
			if(Flag_AUTOMUTE[0]==MUTE){
				KT_WirelessMicRx_Volume(0);
	    	}else{
	    		if(ChAScanDelay){
	    			ChAScanDelay=0;
	    		//	GroupWrite(0, 2);
	    		}
	//			cAF[chipSel]=KT_WirelessMicRx_GetAF();
	//			snr[chipSel]=KT_WirelessMicRx_GetSNR();

				chipSel = MorSSelect;
	//			CHA_Data[0] = KT_Bus_Read(0x0248,0)<<8 | KT_Bus_Read(0x0249,chipSel);
				CHA_Data[1] = KT_Bus_Read(0x024A,0)<<8 | KT_Bus_Read(0x024B,chipSel);
				CHA_Data[2] = KT_Bus_Read(0x024C,0)<<8 | KT_Bus_Read(0x024D,chipSel);
	//			CHA_Data[3] = KT_Bus_Read(0x024E,0)<<8 | KT_Bus_Read(0x024F,chipSel);

				if(CHA_DataR[1] != CHA_Data[1]){
					CHA_DataR[1] = CHA_Data[1];
					CHA_Data[0] = KT_Bus_Read(0x0248,0)<<8 | KT_Bus_Read(0x0249,chipSel);
					DBG_LOG_INFO("ChAAUX_DATA[1]:%04X   DATA[0]:%04X\n", CHA_Data[1], CHA_Data[0]);
					CommFun(CHA_Data[1], CHA_Data[0],0);
				}
				if(CHA_DataR[2] != CHA_Data[2]){
					CHA_DataR[2] = CHA_Data[2];
					DBG_LOG_INFO("ChAAUX_DATA[2]:%04X\n", CHA_Data[2]);
					CtrlFun(CHA_Data[2]);
				}

				chipSel = 0;
		#if 1
				if((uint16_t)SysInf.AchPair == CHA_Data[0]){
					KT_WirelessMicRx_Volume(PRESET_VOL);
				}else{
					KT_WirelessMicRx_Volume(0);
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
		}

	}else if(refreshTimer == 4){
		//===============================================================
		chipSel = 2;
		MorSSelect = 0;
		if(Chip_E[2] && Chip_E[3])  KT_WirelessMicRx_SelectMS();
		//--------------------------------------------------------
		if(Chip_E[2]){
			Flag_AUTOMUTE[2]=KT_WirelessMicRx_Automute();
	    	if(Flag_AUTOMUTE[2]==MUTE){
    			KT_WirelessMicRx_Volume(0);
			}else{
				if(ChBScanDelay){
					ChBScanDelay=0;
				//	GroupWrite(0, 2);
				}

		//		cAF[chipSel]=KT_WirelessMicRx_GetAF();
		//		snr[chipSel]=KT_WirelessMicRx_GetSNR();

				chipSel = 2+MorSSelect;
			//	CHB_Data[0] = KT_Bus_Read(0x0248,2)<<8 | KT_Bus_Read(0x0249,chipSel);
				CHB_Data[1] = KT_Bus_Read(0x024A,2)<<8 | KT_Bus_Read(0x024B,chipSel);
				CHB_Data[2] = KT_Bus_Read(0x024C,2)<<8 | KT_Bus_Read(0x024D,chipSel);
			//	CHB_Data[3] = KT_Bus_Read(0x024E,2)<<8 | KT_Bus_Read(0x024F,chipSel);

				if(CHB_DataR[1] != CHB_Data[1]){
					CHB_DataR[1] = CHB_Data[1];
					CHB_Data[0] = KT_Bus_Read(0x0248,0)<<8 | KT_Bus_Read(0x0249,chipSel);
					DBG_LOG_INFO("ChBAUX_DATA[1]:%04X   DATA[0]:%04X\n", CHB_Data[1], CHB_Data[0]);
					CommFun(CHB_Data[1], CHB_Data[0],1);
				}
				if(CHB_DataR[2] != CHB_Data[2]){
					CHB_DataR[2] = CHB_Data[2];
					DBG_LOG_INFO("ChBAUX_DATA[2]:%04X\n", CHB_Data[2]);
					CtrlFun(CHB_Data[2]);
				}

				chipSel = 2;
		#if 1
				if((uint16_t)SysInf.BchPair == CHB_Data[0]){
					KT_WirelessMicRx_Volume(PRESET_VOL);
				}else{
					KT_WirelessMicRx_Volume(0);
				}
		#else
				KT_WirelessMicRx_Volume(PRESET_VOL);
		#endif
			}
		}else{
	//		LED_CHB = 1;
		}

	}else if(refreshTimer == 6){
		chipSel = 2;
		Rssi[chipSel]=KT_WirelessMicRx_GetRSSI();
		if(Rssi[2] <30) Rssi[2] = 0;
		if(Rssi[2] >100) Rssi[2] = 100;
		if(abs(RssiB_R - Rssi[2]) > 5 ){
			RssiB_R = Rssi[2];
		}
	}
}

//**** KT56初始化 *********************************************
void KT56_Init (void)
{
    uint8_t i;

//	printf("==== KT56 Init ============================\n");
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
	Set_CH(1);
	Set_CH(2);
	//=================================================
}
#endif
