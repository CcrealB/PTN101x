/**********************************************************************
* 文件:
* 作者:	Chen JQ
* 版本:	V1.0
* 日期： 2023-09-26
* 描述：更新SDK,usb_desc.c  LED_EF_FUN.c

**********************************************************************/
#include "USER_Config.h"
#ifdef XFN_S930

#include "XFN_S930_DEF.H"    //徐工调的
//#include "XFN_S930_DE22F.H"  //陈总调的
//#include "XFN_S930_9.19.H"    //客户的
uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;
//extern uint8 appPlayerPlayMode;
//uint8_t work_modeR=10;

static uint16_t WorkModeCont=0;

uint8_t LineInR=0;
uint8_t LineIn2R=0;

static uint16_t SysMsCont=0;
static uint8_t	BtLedFlash=0;
uint8_t Mic_ValR=255;
uint16_t WMicOffCont=0;

//const uint8_t S_Xx[4][7]={{0x31,0x26,0x25,0x27,0xE4,0xE5,0x30},	//1~4位數字指標  a~g
//							{0x45,0x51,0x50,0x47,0x24,0x44,0x46},
//							{0x65,0x86,0x85,0x84,0x67,0x66,0x64},
//							{0xB1,0xA5,0xA6,0xA7,0x90,0x91,0xB0}};
//const uint8_t S_Nx[18]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7C,0x27,0x7F,0x67,0x77,0x7D,0x39,0x5E,0x79,0x71,0x3E,0x00};	// N0~9,A~F,U,NC 顯示位
//const uint8_t S_L3Dotx[13]={0xE6,0xE7,0xF0, 0x70,0x71, 0x87, 0xD1, 0xC4,0xC5,0xC6,0xA4,0xC7, 0xD0};



static int16_t M_LOW;  //低频音频


uint8_t MusVolValR=20;
uint32_t Key_N,Key_NR;		//存储按键值
uint32_t Key2_N,Key2_NR;	//存储按键值
//void Check_Charge_Vol();    //电池电量检测
void PLAY_bli();            //音效闪灯



static uint8_t offled = 0;
static uint16_t vol_level = 0;
//LED灯相关
uint8_t LED_TAB[63]={0x00,0x01,0x02,\
					 0x03,\
					 0x04,0x05,0x06,0x07,0x20,0x21,0x22,0x23,0x24,0x25, \
					 0x26,0x27,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,\
					 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,\
					 0xA7,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,     0xC7,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6};
const uint8_t LED_off[45] = {0x04,0x06,0x20,0x22,0x24,0x26,0x40,0x42,0x44,0x46,0x60,0x62,0x64,0x66,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6};
const uint8_t LED_back_B[14] = {0x04,0x06,0x20,0x22,0x24,0x26,0x40,0x42,0x44,0x46,0x60,0x62,0x64,0x66};   //左边按键开启灯地址 背光灯
const uint8_t LED_back[14] = {0x05,0x07,0x21,0x23,0x25,0x27,0x41,0x43,0x45,0x47,0x61,0x63,0x65,0x67};   //右边按键开启灯地址    开启灯
const uint8_t LED_Addr2[8] = {0xA7,0xC6,0xC5,0xC4,0xC3,0xC2,0xC1,0xC0};   //旋钮1 灯地址    顺转
const uint8_t LED_Addr3[8] = {0xC7,0xE6,0xE5,0xE4,0xE3,0xE2,0xE1,0xE0};   //旋钮2 灯地址
const uint8_t LED_Addr22[8] = {0xA7,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6};   //旋钮1 灯地址   逆转
const uint8_t LED_Addr33[8] = {0xC7,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6};   //旋钮2 灯地址
uint8_t BAT_LED[3]={0x00,0x01,0x02};
const uint8_t LED_AddrL[15] = {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6};    //左始底部15灯地址
const uint8_t LED_AddrR[15] = {0xA6,0xA5,0xA4,0xA3,0xA2,0xA1,0xA0,0x87,0x86,0x85,0x84,0x83,0x82,0x81,0x80};    //右始底部15灯地址
static uint8_t run_mode = 0;  	//模式
static uint8_t run_inedx = 0; 	//索引下标
static uint8_t run_count = 0;   //第一种模式换向计次
static uint8_t FF = 0;          //0是左始 1是右始

static uint8_t run_dir2 = 0;   	//0是向外，1是向内
static uint8_t front = 6; 		//头
static uint8_t rear = 8; 		//尾
static uint8_t change_run_mode = 1;
static uint8_t change_run_mode2 = 1;

static uint8_t speed_sta=10;   //变速
static uint8_t speed2_sta=10;   //变速

extern uint8_t* StbFg;
//**********************************************
//关于 充电
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

#define ChargFull	GPIO5   //充满    充满为0//#define DcInDet			GPIO5      //充电检测
#define	fullVn			12600	// 滿電壓 mv
//#define	P4Voln			12600//11700	// 亮4個燈電壓
#define	P3Voln			11800//10800	// 亮3個燈電壓
#define	P2Voln			11100//9900	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		10300	// 低電模式電壓
#define	LoPowerOFFv	 	10000	// 低電關機電壓
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
		BattVolDetMs=0;
		static uint8_t LowPowerFlash=1;
		//===================================================================================
		uint16_t BattVol = BattVolConvert(SarAdcVal[1]);
		static uint16_t saradc_tempe_dataR;
		static uint8_t Batt_FULLR;
		extern uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
		//--------------------------------
		fl2hex V1;
		if((abs(SarAdcValR[1]-SarAdcVal[1])>10) || (saradc_tempe_dataR != saradc_tempe_data  || Batt_FULLR != Batt_FULL)){
			SarAdcValR[1] =SarAdcVal[1];
			saradc_tempe_dataR = saradc_tempe_data;
	//		DcInDetR = gpio_input(DcInDet);
			Batt_FULLR = Batt_FULL;
			Send_Id1 = MainPid;
			Send_Id2 = 205;
			Send_Group = 1;
			V1.f = (float)BattVol/1000;
			Send_Val1 = V1.h;
			V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
			Send_Val2 = V1.h;
	//		Send_Val41 = gpio_input(DcInDet);
			Send_Val42 = Batt_FULL;
			SendUpComp(SendBuff, 32);
		}
		if(DisplayCont || vusb_4v_ready()){    // 未充电显示 / 充电显示
//			if(DisplayCont) USER_DBG_INFO("==== BATT ADC:%d  %d mv  DcDet:%d  CH_OK:%d\n",SarAdcVal[1],BattVol,vusb_4v_ready(),Batt_FULL);
	//		if(DisplayCont)	DisplayCont--;      //充电的时候   充电灯显示时间结束就关掉，操作机器时候就会亮充电灯
			if(ChargFlash==0){      //普通显示
			#ifdef ChargeFullDis
				if(BattVol>=fullVn)	Batt_FULL = 1;	// 8.375V 滿電
					else Batt_FULL =0;
			#else
				Batt_FULL = !gpio_input(ChargFull);      //0是在充      1是充满
			#endif
	//			if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
	//				BattVn=0x0F;
	//			}else
				if(BattVol>=P3Voln){	//9.12++ if(BattVol>=fullVn && P3Voln)
					BattVn=0x07;
				}else if(BattVol>=P2Voln){	// 7	10.5
					BattVn=0x03;
				}else{
					BattVn=1;
				}
				ChargFlash = 1;
			}else if(ChargFlash  && vusb_4v_ready()){
				if(((BattVn>>1)&1)==0){
					BattVn=0x03;
				}else if(((BattVn>>2)&1)==0){
					BattVn=0x07;
					ChargFlash = 0;
	//			}else if(((BattVn>>3)&1)==0){
	//				BattVn=0x0F;
	//				ChargFlash = 0;
				}else{
					ChargFlash = 0;
				}
			}else{
				ChargFlash = 0;
			}
	//		USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[1]:%d\n",BattVn,SarAdcVal[1]);
			if(DisplayCont || Batt_FULL==0){// 顯示 或 未滿電
				UpDisplayReg(BAT_LED[0], (BattVn&1));
				UpDisplayReg(BAT_LED[1], ((BattVn>>1)&1));
				UpDisplayReg(BAT_LED[2], ((BattVn>>2)&1));
			}else if(vusb_4v_ready() && Batt_FULL){
				UpDisplayReg(BAT_LED[0], 1);
				UpDisplayReg(BAT_LED[1], 1);
				UpDisplayReg(BAT_LED[2], 1);
			}else{   //DisplayCont为0->关灯     DisplayCont：电量显示倒计时器
				UpDisplayReg(BAT_LED[0], 0);
				UpDisplayReg(BAT_LED[1], 0);
				UpDisplayReg(BAT_LED[2], 0);
			}
			UpDisplayAll();
		}else{//DisplayCont不开启，永远到不了这
			if(BattVol<LoPowerENv){
				UpDisplayReg(BAT_LED[0], LowPowerFlash^=1);//gpio_output(LED0_Bat0, LowPowerFlash^=1);
				UpDisplayReg(BAT_LED[1], 0);
				UpDisplayReg(BAT_LED[2], 0);
			}else{   //DisplayCont为0->关灯     DisplayCont：电量显示倒计时器
				UpDisplayReg(BAT_LED[0], 0);
				UpDisplayReg(BAT_LED[1], 0);
				UpDisplayReg(BAT_LED[2], 0);
			}
			UpDisplayAll();
		}

		if(vusb_4v_ready()==0){
			if(BattVol<LoPowerENv){	// 6.5v	9.75
				UpDisplayReg(BAT_LED[0], LowPowerFlash^=1);
			}
		}

		//==== 判斷進入 LowPower Mode =======================
		if(LowPowerEn==0 && BattVol<LoPowerENv){		// 低于9.4V
			USER_DBG_INFO("==== LowPowerCont: %d\n",LowPowerCont);
			if(++LowPowerCont > 5){
				LowPowerEn=1;
				DelayOffCont=300;
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
		if(LowPowerEn){	// 低電
			if(++DelayOffCont > 300){	// 300s，5分钟播报一次
				DelayOffCont=0;
				DisplayCont = DisplayCont_def;
				if(vusb_4v_ready()==0)	app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);	// 充電中不報低電提示音
			}
			if(BattVol<LoPowerOFFv){	// 低于9v 关机
				USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[1],BattVol, OffPowerCont);
				if(++OffPowerCont==6){        //6S低电播报
					if(vusb_4v_ready()==0) app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);   // 充電中不報低電提示音
				}else if(OffPowerCont>7){     //7S关机
					if(vusb_4v_ready()==0) app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	  // 充電中不關機
				}
			}else{
				OffPowerCont = 0;
			}
		}else{
			DelayOffCont=0;
		}
	}
}



void level_display(){

	for(uint8_t w=0;w<15;w++)
	{
		UpDisplayReg(LED_AddrR[w], (vol_level>>w)&1);
	}

	UpDisplayAll();
}

static uint8_t mode2_flag = 0;
//**********************************************
void LED_DISPLAY()     //灯光效果  两种模式
{
	//变速
	static uint8_t ms100ms = 0;
	static uint16_t ms1000ms = 0;


	if(M_LOW>2000)
		speed_sta=2;
	else if(M_LOW>1000)
		speed_sta=3;
	else if(M_LOW>500)
		speed_sta=4;
	else if(M_LOW>200)
		speed_sta=5;
	else if(M_LOW>100)
		speed_sta=6;
	else if(M_LOW>100)
		speed_sta=7;
	else if(M_LOW>50)
		speed_sta=10;
	else if(M_LOW>20)
		speed_sta=11;
	else
		speed_sta=12;

	if(offled==0){
		if(++ms100ms >= speed_sta){
			ms100ms = 0;
	//======================模式二：音频跑马灯，3次换边，每边进行6次换向
			if(run_mode==1)
			{
//					//方向切换
//					if(run_inedx==15)
//					{
//						run_dir=1;
//						run_inedx=14;
//					}
//					else if(run_inedx==255)
//					{
//						run_dir=0;
//						run_inedx=0;
//						run_count++;     //run_count计到每3次进行换边
//						if(run_count%3==0) FF^=1; //每3反转一次
//					}



					//清除缓存：从模式一切模式二时候清掉显示数据，避免乱象
					if(change_run_mode2==1)
					{
						for(uint8_t j =0;j<15;j++)
						{
							UpDisplayReg(LED_AddrL[j], 0);
						}
						run_inedx=0;
					}

				if(FF==0){
					if(run_inedx==0){
						UpDisplayReg(LED_AddrL[0], 1);UpDisplayReg(LED_AddrL[1], 1);UpDisplayReg(LED_AddrL[2], 1);UpDisplayReg(LED_AddrL[3], 1);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==1){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 1);UpDisplayReg(LED_AddrL[2], 1);UpDisplayReg(LED_AddrL[3], 1);
						UpDisplayReg(LED_AddrL[4], 1);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==2){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 1);UpDisplayReg(LED_AddrL[3], 1);
						UpDisplayReg(LED_AddrL[4], 1);UpDisplayReg(LED_AddrL[5], 1);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==3){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 1);
						UpDisplayReg(LED_AddrL[4], 1);UpDisplayReg(LED_AddrL[5], 1);UpDisplayReg(LED_AddrL[6], 1);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==4){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 1);UpDisplayReg(LED_AddrL[5], 1);UpDisplayReg(LED_AddrL[6], 1);UpDisplayReg(LED_AddrL[7], 1);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==5){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 1);UpDisplayReg(LED_AddrL[6], 1);UpDisplayReg(LED_AddrL[7], 1);
						UpDisplayReg(LED_AddrL[8], 1);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==6){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 1);UpDisplayReg(LED_AddrL[7], 1);
						UpDisplayReg(LED_AddrL[8], 1);UpDisplayReg(LED_AddrL[9], 1);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==7){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 1);
						UpDisplayReg(LED_AddrL[8], 1);UpDisplayReg(LED_AddrL[9], 1);UpDisplayReg(LED_AddrL[10], 1);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==8){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 1);UpDisplayReg(LED_AddrL[9], 1);UpDisplayReg(LED_AddrL[10], 1);UpDisplayReg(LED_AddrL[11], 1);
						UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==9){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 1);UpDisplayReg(LED_AddrL[10], 1);UpDisplayReg(LED_AddrL[11], 1);
						UpDisplayReg(LED_AddrL[12], 1);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==10){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 1);UpDisplayReg(LED_AddrL[11], 1);
						UpDisplayReg(LED_AddrL[12], 1);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrL[14], 0);
					}else if(run_inedx==11){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 1);
						UpDisplayReg(LED_AddrL[12], 1);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrL[14], 1);
						run_count++;if(run_count%3==0) FF^=1; //每3反转一次
					}
					if(FF==1){
						if(run_inedx==12){
						UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 1);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrL[14], 1);
						}else if(run_inedx==13){
							UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
							UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
							UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
							UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrL[14], 1);
						}else if(run_inedx==14){
							UpDisplayReg(LED_AddrL[0], 0);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
							UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
							UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
							UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 1);
						}
					}else{
						if(run_inedx==12){
						UpDisplayReg(LED_AddrL[0], 1);UpDisplayReg(LED_AddrL[1], 0);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
						UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
						UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
						UpDisplayReg(LED_AddrL[12], 1);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrL[14], 1);
						}else if(run_inedx==13){
							UpDisplayReg(LED_AddrL[0], 1);UpDisplayReg(LED_AddrL[1], 1);UpDisplayReg(LED_AddrL[2], 0);UpDisplayReg(LED_AddrL[3], 0);
							UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
							UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
							UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrL[14], 1);
						}else if(run_inedx==14){
							UpDisplayReg(LED_AddrL[0], 1);UpDisplayReg(LED_AddrL[1], 1);UpDisplayReg(LED_AddrL[2], 1);UpDisplayReg(LED_AddrL[3], 0);
							UpDisplayReg(LED_AddrL[4], 0);UpDisplayReg(LED_AddrL[5], 0);UpDisplayReg(LED_AddrL[6], 0);UpDisplayReg(LED_AddrL[7], 0);
							UpDisplayReg(LED_AddrL[8], 0);UpDisplayReg(LED_AddrL[9], 0);UpDisplayReg(LED_AddrL[10], 0);UpDisplayReg(LED_AddrL[11], 0);
							UpDisplayReg(LED_AddrL[12], 0);UpDisplayReg(LED_AddrL[13], 0);UpDisplayReg(LED_AddrL[14], 1);
						}
					}

				}else{
					if(run_inedx==0){
						UpDisplayReg(LED_AddrR[0], 1);UpDisplayReg(LED_AddrR[1], 1);UpDisplayReg(LED_AddrR[2], 1);UpDisplayReg(LED_AddrR[3], 1);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==1){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 1);UpDisplayReg(LED_AddrR[2], 1);UpDisplayReg(LED_AddrR[3], 1);
						UpDisplayReg(LED_AddrR[4], 1);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==2){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 1);UpDisplayReg(LED_AddrR[3], 1);
						UpDisplayReg(LED_AddrR[4], 1);UpDisplayReg(LED_AddrR[5], 1);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==3){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 1);
						UpDisplayReg(LED_AddrR[4], 1);UpDisplayReg(LED_AddrR[5], 1);UpDisplayReg(LED_AddrR[6], 1);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==4){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 1);UpDisplayReg(LED_AddrR[5], 1);UpDisplayReg(LED_AddrR[6], 1);UpDisplayReg(LED_AddrR[7], 1);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==5){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 1);UpDisplayReg(LED_AddrR[6], 1);UpDisplayReg(LED_AddrR[7], 1);
						UpDisplayReg(LED_AddrR[8], 1);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==6){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 1);UpDisplayReg(LED_AddrR[7], 1);
						UpDisplayReg(LED_AddrR[8], 1);UpDisplayReg(LED_AddrR[9], 1);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==7){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 1);
						UpDisplayReg(LED_AddrR[8], 1);UpDisplayReg(LED_AddrR[9], 1);UpDisplayReg(LED_AddrR[10], 1);UpDisplayReg(LED_AddrR[11], 0);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==8){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 1);UpDisplayReg(LED_AddrR[9], 1);UpDisplayReg(LED_AddrR[10], 1);UpDisplayReg(LED_AddrR[11], 1);
						UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==9){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 1);UpDisplayReg(LED_AddrR[10], 1);UpDisplayReg(LED_AddrR[11], 1);
						UpDisplayReg(LED_AddrR[12], 1);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==10){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 1);UpDisplayReg(LED_AddrR[11], 1);
						UpDisplayReg(LED_AddrR[12], 1);UpDisplayReg(LED_AddrR[13], 1);UpDisplayReg(LED_AddrR[14], 0);
					}else if(run_inedx==11){
						UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
						UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
						UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 1);
						UpDisplayReg(LED_AddrR[12], 1);UpDisplayReg(LED_AddrL[13], 1);UpDisplayReg(LED_AddrR[14], 1);
						run_count++;if(run_count%3==0) FF^=1; //每3反转一次
					}
					if(FF==0){
						if(run_inedx==12){
							UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
							UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
							UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
							UpDisplayReg(LED_AddrR[12], 1);UpDisplayReg(LED_AddrR[13], 1);UpDisplayReg(LED_AddrR[14], 1);
						}else if(run_inedx==13){
							UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
							UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
							UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
							UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 1);UpDisplayReg(LED_AddrR[14], 1);
						}else if(run_inedx==14){
							UpDisplayReg(LED_AddrR[0], 0);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
							UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
							UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
							UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 1);
						}
					}else{
						if(run_inedx==12){
							UpDisplayReg(LED_AddrR[0], 1);UpDisplayReg(LED_AddrR[1], 0);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
							UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
							UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
							UpDisplayReg(LED_AddrR[12], 1);UpDisplayReg(LED_AddrR[13], 1);UpDisplayReg(LED_AddrR[14], 1);
						}else if(run_inedx==13){
							UpDisplayReg(LED_AddrR[0], 1);UpDisplayReg(LED_AddrR[1], 1);UpDisplayReg(LED_AddrR[2], 0);UpDisplayReg(LED_AddrR[3], 0);
							UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
							UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
							UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 1);UpDisplayReg(LED_AddrR[14], 1);
						}else if(run_inedx==14){
							UpDisplayReg(LED_AddrR[0], 1);UpDisplayReg(LED_AddrR[1], 1);UpDisplayReg(LED_AddrR[2], 1);UpDisplayReg(LED_AddrR[3], 0);
							UpDisplayReg(LED_AddrR[4], 0);UpDisplayReg(LED_AddrR[5], 0);UpDisplayReg(LED_AddrR[6], 0);UpDisplayReg(LED_AddrR[7], 0);
							UpDisplayReg(LED_AddrR[8], 0);UpDisplayReg(LED_AddrR[9], 0);UpDisplayReg(LED_AddrR[10], 0);UpDisplayReg(LED_AddrR[11], 0);
							UpDisplayReg(LED_AddrR[12], 0);UpDisplayReg(LED_AddrR[13], 0);UpDisplayReg(LED_AddrR[14], 1);
						}
					}
				}

					run_inedx++;
					if(run_inedx==15) run_inedx=0;
					UpDisplayAll();








//					if(FF==0){     //左始
//						//RUN
//						if(run_dir)   //反向
//						{
//							UpDisplayReg(LED_AddrL[run_inedx--], 0);
//						}
//						else    //正向
//						{
//							for(uint8_t z=0;z<15;z++){
//								UpDisplayReg(LED_AddrL[z], ((run_inedx<<1)>>z)&1);
//							}
//						}
//						UpDisplayAll();
//					}else if(FF==1){         //右始
//						//RUN
//						if(run_dir)   //反向
//						{
//							UpDisplayReg(LED_AddrR[run_inedx--], 0);
//						}
//						else    //正向
//						{
//							UpDisplayReg(LED_AddrR[run_inedx++], 1);
//						}
//						UpDisplayAll();
//					}

					change_run_mode=1;
					change_run_mode2=0;
					mode2_flag=0;
			}

	//======================模式一：从中间向两边扫动
			else if(run_mode==0)
			{
					//方向切换
					if(front==7||rear==7)
					{
						run_dir2=0;
						front=6;
						rear=8;
					}
					else if(front==255||rear==15)
					{
						run_dir2=1;
						front=0;
						rear=14;
					}

					//清除缓存：从模式二切模式一时候清掉显示数据，避免乱象
					if(change_run_mode==1)
					{
						for(uint8_t i =0;i<15;i++)
						{
							UpDisplayReg(LED_AddrL[i], 0);
						}
						UpDisplayReg(LED_AddrL[7], 1);
						front = 6; 	//头
						rear = 8; 	//尾
					}

					//RUN
					if(run_dir2)          //向内
					{
						UpDisplayReg(LED_AddrL[front++], 0);
						UpDisplayReg(LED_AddrL[rear--], 0);
						UpDisplayAll();        //一次显示  刷一次
					}
					else                   //向外
					{
						UpDisplayReg(LED_AddrL[front--], 1);
						UpDisplayReg(LED_AddrL[rear++], 1);
						UpDisplayAll();        //一次显示  刷一次
					}
					change_run_mode=0;
					change_run_mode2=1;
					run_count=0;
					FF=0;
					mode2_flag=0;
			}
	//======================模式三：灯效全关模式->只保留按键开启灯功能 和 电池电量 和 蓝牙状态灯
			else if(run_mode==2)
			{
				//关前排灯、旋转灯圈、背光灯
				for(uint8_t M=0;M<45;M++)
				{
					UpDisplayReg(LED_off[M], 0);
				}

				mode2_flag=1;
				RgbLedAllColour(0);	// OFF RGB LED
	//			UpDisplayAll();        //一次显示  刷一次
			}
		}
	}else if(offled){        //旋钮有动作
//		for(uint8_t j =0;j<15;j++)
//		{
//			UpDisplayReg(LED_AddrL[j], 0);
//		}
		level_display();
		if(++ms1000ms==300){    //4秒显示缓冲
			ms1000ms=0;
			offled=0;
			FF=0;
			run_count=0;
			run_inedx=0;
			if(run_mode==0){
//				UpDisplayReg(LED_AddrL[7], 1);
				front = 6; 	//头
				rear = 8; 	//尾
			}
			for(uint8_t j =0;j<15;j++)
			{
				UpDisplayReg(LED_AddrL[j], 0);
			}
			if(run_mode==0) UpDisplayReg(LED_AddrL[7], 1);
		}
	}


}

static uint8_t LR = 0;           //换向标志位    0为顺转  1为逆转
//static uint8_t ms1000ms = 0;     //换向计数器
static uint8_t run_count2 = 0;   //换向计次
//**********************************************
void LED_DISPLAY2()   //旋钮固定灯效
{
	static uint16_t s1s = 0;
	static uint8_t run2_inedx = 0; 	//索引下标

	if(M_LOW>2000)
		speed2_sta=2;
	else if(M_LOW>1000)
		speed2_sta=3;
	else if(M_LOW>500)
		speed2_sta=4;
	else if(M_LOW>200)
		speed2_sta=5;
	else if(M_LOW>100)
		speed2_sta=6;
	else if(M_LOW>50)
		speed2_sta=7;
	else if(M_LOW>30)
		speed2_sta=8;
	else if(M_LOW>10)
		speed2_sta=9;
	else
		speed2_sta=10;
	//=========================================旋转灯圈

	if(mode2_flag==0){     //模式二熄灯  旋转灯圈失效
		extern uint32_t user_aud_amp_task(void);
		if(user_aud_amp_task()==0){	    //侦测无声音时旋转灯圈全亮  否则低频律动

			for(uint8_t i=0;i<8;i++)
			{
				UpDisplayReg(LED_Addr2[i], 1);
				UpDisplayReg(LED_Addr3[i], 1);
			}
		}else{
			if(++s1s >= (speed2_sta*3)){
				s1s = 0;
				if(LR==0){       //顺转
					if(run2_inedx==0){
						UpDisplayReg(LED_Addr2[0], 0);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 0);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==1){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 0);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 0);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==2){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 0);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 0);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==3){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 0);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 0);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==4){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 0);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 0);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==5){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 0);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 0);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==6){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 0);UpDisplayReg(LED_Addr2[7], 1);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 0);UpDisplayReg(LED_Addr3[7], 1);
					}else if(run2_inedx==7){
						UpDisplayReg(LED_Addr2[0], 1);UpDisplayReg(LED_Addr2[1], 1);UpDisplayReg(LED_Addr2[2], 1);UpDisplayReg(LED_Addr2[3], 1);
						UpDisplayReg(LED_Addr2[4], 1);UpDisplayReg(LED_Addr2[5], 1);UpDisplayReg(LED_Addr2[6], 1);UpDisplayReg(LED_Addr2[7], 0);
						UpDisplayReg(LED_Addr3[0], 1);UpDisplayReg(LED_Addr3[1], 1);UpDisplayReg(LED_Addr3[2], 1);UpDisplayReg(LED_Addr3[3], 1);
						UpDisplayReg(LED_Addr3[4], 1);UpDisplayReg(LED_Addr3[5], 1);UpDisplayReg(LED_Addr3[6], 1);UpDisplayReg(LED_Addr3[7], 0);
						run_count2++;if(run_count2%3==0) LR^=1; //每3反转一次
					}
				}else{
					if(run2_inedx==0){
						UpDisplayReg(LED_Addr22[0], 0);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 0);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==1){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 0);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 0);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==2){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 0);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 0);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==3){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 0);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 0);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==4){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 0);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 0);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==5){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 0);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 0);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==6){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 0);UpDisplayReg(LED_Addr22[7], 1);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 0);UpDisplayReg(LED_Addr33[7], 1);
					}else if(run2_inedx==7){
						UpDisplayReg(LED_Addr22[0], 1);UpDisplayReg(LED_Addr22[1], 1);UpDisplayReg(LED_Addr22[2], 1);UpDisplayReg(LED_Addr22[3], 1);
						UpDisplayReg(LED_Addr22[4], 1);UpDisplayReg(LED_Addr22[5], 1);UpDisplayReg(LED_Addr22[6], 1);UpDisplayReg(LED_Addr22[7], 0);
						UpDisplayReg(LED_Addr33[0], 1);UpDisplayReg(LED_Addr33[1], 1);UpDisplayReg(LED_Addr33[2], 1);UpDisplayReg(LED_Addr33[3], 1);
						UpDisplayReg(LED_Addr33[4], 1);UpDisplayReg(LED_Addr33[5], 1);UpDisplayReg(LED_Addr33[6], 1);UpDisplayReg(LED_Addr33[7], 0);
						run_count2++;if(run_count2%3==0) LR^=1; //每3反转一次
					}
				}
				run2_inedx++;
				if(run2_inedx==8) run2_inedx=0;
//				UpDisplayAll();
			}
		}
	}
//	static uint8_t Sub = 0;
//	if(++ms1000ms==20){    //3秒换一次方向
//		ms1000ms=0;
//		//灯圈换方向
//		run_count2++;
//		if(run_count2%3==0) LR^=1;
//	}
//
//	if(M_LOW>1000)
//		Sub = 0xff;
//	else if(M_LOW>500)
//		Sub = 0x7f;
//	else if(M_LOW>200)
//		Sub = 0x3f;
//	else if(M_LOW>100)
//		Sub = 0x1f;
//	else if(M_LOW>50)
//		Sub = 0x0f;
//	else if(M_LOW>30)
//		Sub = 0x07;
//	else if(M_LOW>10)
//		Sub = 0x03;
//	else
//		Sub = 0x01;
//
//	if(LR==0)				//顺转
//	{
//		for(uint8_t i=0;i<8;i++)
//		{
//			UpDisplayReg(LED_Addr2[i], (Sub>>i)&1);
//			UpDisplayReg(LED_Addr3[i], (Sub>>i)&1);
//		}
//	}
//	else if(LR==1)				//逆转
//	{
//		for(uint8_t i=0;i<8;i++)
//		{
//			UpDisplayReg(LED_Addr22[i], (Sub>>i)&1);
//			UpDisplayReg(LED_Addr33[i], (Sub>>i)&1);
//		}
//	}
//	UpDisplayAll();
}



//**********************************************
void LED_Working()  //按键背光启动灯效
{
	for(uint8_t i=0;i<63;i++)
	{
		UpDisplayReg(LED_TAB[i], 1);
	}
	//按键开启灯 关
	for(uint8_t i = 1;i<14;i++)     //跳过唱歌灯
	{
		UpDisplayReg(LED_back[i], 0);
	}
	UpDisplayReg(LED_back[4], 1);   //电源灯开
	UpDisplayAll();        //一次显示  刷一次
}






//****************************************
void LCD_ShowDot(uint8_t n, uint8_t OnOff)
{


}

static uint8_t WModeStep =0;
//*****　插U盘会调用　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void user_udisk_ready_callback(void)
{
	if(WModeStep==4){
		system_work_mode_set_button(SYS_WM_UDISK_MODE);
		SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
	}else{
		WModeStep=1;
	}
}

//*****************************************************//控制LCD显示和音源链路音量设置
void user_WorkModefun(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){	// 1秒計時
		WorkModeCont = 0;
		if(WModeStep==2){
			system_work_mode_set_button(SYS_WM_UDISK_MODE);
			SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
			WModeStep=4;
		}
		if(WModeStep==3){
			system_work_mode_set_button(SYS_WM_BT_MODE);
			SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
			WModeStep=4;
		}

//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================//系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

//				ioGain[Play_MusL]= -2;
//				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}

			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				UpDisplayReg(LED_TAB[3],1);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)	UpDisplayReg(LED_TAB[3],1);
					else			UpDisplayReg(LED_TAB[3],0);
			}

		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

//				ioGain[Play_MusL]= -2;
//				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
			}
			if(hci_get_acl_link_count(sys_hdl->unit)==0){      //蓝牙未连接
				if(BtLedFlash^=1)	UpDisplayReg(LED_TAB[3],1);
					else			UpDisplayReg(LED_TAB[3],0);
			}
		}
	}
}


//**** pre init ***********************
void user_init_prv(void)
{
//	gpio_output(GPIO3, 1);   //先屏蔽掉TM1629寄存器残留的值，误动显示    8.31++
	//=============================	GPIO初始化
	#define SGM_MUTE	GPIO2		//监听耳机开关   高开低关
	gpio_config_new(SGM_MUTE, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(SGM_MUTE, 0);

	#define ACM_PDN		GPIO35		//ACM功放          高开低关
	gpio_config_new(ACM_PDN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(ACM_PDN, 0);

	#define WM_PO_EN	GPIO12      //WMIC     高开低关
	gpio_config_new(WM_PO_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(WM_PO_EN, 0);

	#define POWER_EN	GPIO22      //电源使能          高开低关
	gpio_config_new(POWER_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(POWER_EN, 0);

	#define Boot_EN	GPIO10      //DC       高开低关
	gpio_config_new(Boot_EN, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(Boot_EN, 1);

	#define PowerKey	GPIO15
	gpio_config_new(PowerKey, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);

	PowerDownFg=0;   //7.26-17:52++

}

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH 傳入版本參數

	if(SysInf.MusicVol<2)SysInf.MusicVol=2;

	set_aud_mode_flag(AUD_FLAG_GAME_MODE);		// 藍芽低延遲模式

	gpio_output(WM_PO_EN, 1);         //mute掉后  灯板亮不起来

//	if(vusb_4v_ready()==0){
	gpio_output(SGM_MUTE, 1);
	gpio_output(ACM_PDN, 1);

		//初始化模式
	//	app_handle_t app_h = app_get_sys_handler();
	//	app_h->sys_work_mode = SYS_WM_NULL;


#if LED_RGB_NUM
	app_i2s0_init(1);
	RgbLedAllColour(0);	// OFF RGB LED
#endif
//	}
}

//********************************
void user_init_post(void)
{
	//---------------------TM1688初始化
//	TM16xx_Init(1);
	//---------------------灯
//		LED_Working();
	for(uint8_t i=0;i<63;i++){
		UpDisplayReg(LED_TAB[i],0);
	}
	//---------------------IIC初始化
//	Hi2c_Init();
//	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
#ifndef ChargeFullDis
	gpio_config_new(ChargFull, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
#endif
}

static uint8_t wave1_stopFlash =0;		   //停止提示音1掌声
static uint8_t wave2_stopFlash =0;		   //停止提示音2笑声
static uint8_t wave11_stopFlash =0;		   //停止提示音11暖场1
static uint8_t wave22_stopFlash =0;		   //停止提示音22暖场2
static uint8_t tone = 0;                   //变音切换标志位
static uint8_t DuckDetF = 0;               //闪避
static uint8_t AMP_SW = 0;                 //AMP开关标志位
//static uint8_t OTG_SW = 0;               //OTG开关标志位
static uint8_t keycount = 0;               //按键计数器
uint32_t Key_Fun;                          //Old  key value

static uint8_t ms100=0;
static uint8_t msss500=0;
//*****************************************************
void TM16xx_main()
{

	Key_N = read_key();	//读按键值

//====================================
	if(Key_NR != Key_N){      //按下按键，开始计时
		keycount++;
	}else{                    //未按
		keycount=0;
	}

//====================================长按
	if(keycount==80){
		switch(Key_N){
			case 0x80000000 :	//暂停/播放    |  长按断开连接
				app_button_sw_action(BUTTON_BT_CONN_DISCONN);
				UpDisplayReg(LED_back[11], 0);
				UpDisplayReg(LED_back[12], 0);
				UpDisplayReg(LED_back[13], 1);
			break;

			case 0x00080000 :   //模式切换
				system_work_mode_change_button();
//				KeyId = BUTTON_MODE_CHG;
//				FadeInOut = 200;
			break;

			case 0x00008000 :	//暖场1
				wave11_stopFlash^=1;
				if(wave11_stopFlash)
				{
					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED8);//掌声
				}else{
					app_wave_file_play_stop();
				}
			break;

			case 0x00004000 :	//暖场2
				wave22_stopFlash^=1;
				if(wave22_stopFlash)
				{
					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);//掌声
				}else{
					app_wave_file_play_stop();
				}
			break;


			}
		Key_Fun =0;
	}
	if(++msss500>= 50){
		msss500=0;
		USER_DBG_INFO("====STEP1.goto TM1629_MAIN  Key_N:%d  Key_Fun:%d\n",Key_N,Key_Fun);
	}
//====================================消抖
	if(keycount==10){
		if(Key_N)	Key_Fun=Key_N;
	}

//====================================短按
	if(Key_Fun && Key_N==0){
		USER_DBG_INFO("====STEP2.goto key_Fun :%d\n",Key_Fun);
		switch (Key_Fun){
			case 0x00000008 :	//唱歌  / 音效模式
//				LED_TEST(0);
				UpDisplayReg(LED_back[0], 1);
				UpDisplayReg(LED_back[1], 0);
				UpDisplayReg(LED_back[2], 0);
//				UserEf.Pitch = 0;
				EF_Mode=1;
				tone=0;

				break;

			case 0x00000080 :	//原声模式
//				LED_TEST(1);
				UpDisplayReg(LED_back[0], 0);
				UpDisplayReg(LED_back[1], 1);
				UpDisplayReg(LED_back[2], 0);
				app_wave_file_play_stop();
//				UserEf.Pitch = 0;
				EF_Mode=0;
				tone=0;
				USER_DBG_INFO("====STEP4.goto ysmode\n");
				break;

			case 0x00000800 :	//变音
				UpDisplayReg(LED_back[0], 0);
				UpDisplayReg(LED_back[1], 0);
				UpDisplayReg(LED_back[2], 1);
                EF_Mode=2;
				app_wave_file_play_stop();
				if(tone==0){
					tone=1;	UserEf.Pitch = +4;	// 男變女    女神模式
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED1);
				}else if(tone==1){
					tone=2;	UserEf.Pitch = -4;	// 女變男    男神模式
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED2);
				}else if(tone==2){
					tone=3;	UserEf.Pitch = +12;	// 娃娃音
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED3);
				}else if(tone==3){
					tone=0;	UserEf.Pitch = -8;	// 魔音
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);
				}
				break;

			case 0x00008000 :	//掌声

				wave1_stopFlash^=1;
				if(wave1_stopFlash)
				{
					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED6);//掌声
				}else{
					app_wave_file_play_stop();
				}
				break;

			case 0x00000004 :	//闪避
				app_wave_file_play_stop();
				if(DuckDetF==0){
					DuckDetF=1;
					SysInf.DuckDet = 0x1E;//SysInf.DuckDet = 0x0E;
					app_wave_file_play_start(APP_WAVE_FILE_ID_UNMUTE_MIC); //闪避开
					UpDisplayReg(LED_back[5], 1);
				}else{
					DuckDetF=0;
					SysInf.DuckDet = 0x00;
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_CANCEL); //闪避关
					UpDisplayReg(LED_back[5], 0);
				}

				break;

			case 0x00000040 :	//消音
				app_wave_file_play_stop();
				SysInf.VoiceCanEn ^=1;
				if(SysInf.VoiceCanEn)
				{
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);//消音开
					UpDisplayReg(LED_back[6], 1);
				}
				else
				{
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_TRANSFER);//消音关
					UpDisplayReg(LED_back[6], 0);
				}

				break;

			case 0x00000400 :	//升降调
				SysInf.KeyShift++;
				if(SysInf.KeyShift==3) SysInf.KeyShift=-2;
				if(SysInf.KeyShift!=0)
					UpDisplayReg(LED_back[7], 1);
				else
					UpDisplayReg(LED_back[7], 0);
				break;

			case 0x00004000 :	//笑声
					wave2_stopFlash^=1;
					if(wave2_stopFlash)
					{
						app_wave_file_play_stop();
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED7);//笑声
					}else{
						app_wave_file_play_stop();
					}
				break;

			case 0x00040000 :	//喇叭开关
				AMP_SW^=1;
				if(AMP_SW)
				{
					UpDisplayReg(LED_back[9], 1);
					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);   //喇叭关
					DreSet.MusDreOnOff = 0;
					//在提示音结束  关功放使能脚
				}
				else
				{
					UpDisplayReg(LED_back[9], 0);
					gpio_output(ACM_PDN, 1);
					os_delay_ms(10);  			//关闭到开启  最少5ms的延时
					extern uint8_t ACM862x_IIC_ADDR[2];
					ACM862x_IIC_ADDR[0]=0x2C;
					ACM862x_IIC_ADDR[1]=0x15;
//					os_delay_ms(10);  			//关闭到开启  最少5ms的延时
					ACM8625_init();   			//cjq++    重新打开使能脚 ->>>重新初始化功放

					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);  //喇叭开
					DreSet.MusDreOnOff = 1;
				}
				break;

			case 0x00080000 :	//灯效
				UpDisplayReg(LED_back[10], 1);
				app_wave_file_play_stop();
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8);

//				run_mode++;
//				if(run_mode==3) run_mode=0;

				if(run_mode==0){
					run_mode=1;
					for(uint8_t i = 0;i<14;i++)
					{
						UpDisplayReg(LED_back_B[i], 1);
					}
				}else if(run_mode==1){
					run_mode=2;
					UpDisplayReg(LED_back[10], 0);     //灭开启灯
				}else if(run_mode==2){
					run_mode=0;
					for(uint8_t i = 0;i<14;i++)
					{
						UpDisplayReg(LED_back_B[i], 1);
					}
				}

				break;

			case 0x00800000 :	//上一曲
				UpDisplayReg(LED_back[11], 1);
				UpDisplayReg(LED_back[12], 0);
				UpDisplayReg(LED_back[13], 0);
				app_button_sw_action(BUTTON_BT_PREV);
//				KeyId = BUTTON_BT_PREV;
//				FadeInOut = 200;

//				for(uint8_t i=0;i<16;i++)
//				{
//					StbFg[i]=0;
//				}
//				UpDisplayAll();
				break;

			case 0x08000000 :	//下一曲
				UpDisplayReg(LED_back[11], 0);
				UpDisplayReg(LED_back[12], 1);
				UpDisplayReg(LED_back[13], 0);
				app_button_sw_action(BUTTON_BT_NEXT);
//				KeyId = BUTTON_BT_NEXT;
//				FadeInOut = 200;
				break;

			case 0x80000000 :	//暂停/播放    |  长按断开连接
				UpDisplayReg(LED_back[11], 0);
				UpDisplayReg(LED_back[12], 0);
				UpDisplayReg(LED_back[13], 1);
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
//				KeyId = BUTTON_BT_PLAY_PAUSE;
//				if(FadeInOut<150)	FadeInOut = 200;
				break;
			}
			USER_DBG_INFO("====STEP3.goout key_Fun :%d\n",Key_Fun);
				Key_NR=Key_N;
				Key_Fun =0;

		}
	UpDisplayAll();		// 更新显示
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
	if(PowerDownFg)	return;
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
	if(PowerDownFg)	return;
	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
#ifdef	CONFIG_AUD_AMP_DET_FFT
		//============================
		static uint8_t led_30ms=0;
		if(++led_30ms >=3){         //30ms刷新RBG灯效
			led_30ms=0;
			void RGB_LED_EF_FUN(void);
			if(run_mode!=2)
				RGB_LED_EF_FUN();
		}
#endif


		//----------------------------
		if(EF_Maim())  return;
		//----------------------AMP
		if(ACM_main()) return;
		//----------------------
		user_WorkModefun();

		//----------------------音乐渐入渐出
//		if(FadeInOut)	FadeInOut_Fun();
		//----------------------编码器旋钮
		Knob_function();
		//----------------------按键扫描
		TM16xx_main();
		//------------------------//电池电量检测
		BattFun();
		//-------------------------WMIC
		BK9532_Main();

		extern uint32_t user_aud_amp_task(void);
		M_LOW=user_aud_amp_task();
		LED_DISPLAY();     //灯效
		LED_DISPLAY2();
		//音效播放中  音效键闪灯   闪烁基数 100ms

		if(++ms100 >= 10){
			ms100 = 0;
			//灭 上一首 下一首 暂停播放 键
			UpDisplayReg(LED_back[11], 0);
			UpDisplayReg(LED_back[12], 0);
			UpDisplayReg(LED_back[13], 0);
			PLAY_bli();  //音效闪灯
		}


		static uint8_t led_50ms=0;
		if(++led_50ms >=5){         //50ms刷新RBG灯效
			led_50ms=0;
//			extern uint32_t user_aud_amp_task(void);
//			if(mode2_flag==0){     //模式二熄灯  旋转灯圈失效
//				if(user_aud_amp_task()==0){	    //侦测无声音时旋转灯圈全亮  否则低频律动
////					os_delay_ms(5);
////					if(user_aud_amp_task()==0){
//						for(uint8_t i=0;i<8;i++)
//						{
//							UpDisplayReg(LED_Addr2[i], 1);
//							UpDisplayReg(LED_Addr3[i], 1);
//						}
////					}
//				}else{
//					LED_DISPLAY2();
//				}
//			}
		}



		//==== 訊息報告 ====
		static uint8_t ms1000=0;
		if(++ms1000 >= 100){	// 1秒計時  功放报告
			ms1000 = 0;
			ACM_REPORT();   //數字功放
//			USER_DBG_INFO("====M_LOW:%d:\n", M_LOW);
//			USER_DBG_INFO("====M_MID:%d:\n", M_MID);
//			USER_DBG_INFO("====M_HIGH:%d:\n", M_HIGH);
//			USER_DBG_INFO("====M_AVG:%d:\n", M_AVG);
//			USER_DBG_INFO("====speed:%d:\n", speed_sta);
//			USER_DBG_INFO("====speed:%d:\n", speed_sta);
//			USER_DBG_INFO("====%d\n", vusb_4v_ready());
//			USER_DBG_INFO("==== 音量大小:%d\n",user_aud_amp_task());

		}
		//=====================
		user_key_scan();
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d!\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		//--------------------------------------------------
		if(Init_Cont == 1){

			//充电侦测脚在位 开启按键灯板
//			if(vusb_4v_ready()==1) TM16xx_Init(1);
//			UpDisplayOnOff(0);

		}else if(Init_Cont==6){
			static uint8_t add=0;				//判断是否开机，开机状态为1
			if(vusb_4v_ready())
				BattFun();
			if(vusb_4v_ready() && gpio_input(PowerKey))
			{
				Init_Cont--;
				add = 1;
			}else{
				if(add){
					USER_DBG_INFO("======== 1 =========\n");
					dsp_shutdown();
					REG_SYSTEM_0x1D &= ~0x2;
					system_wdt_reset_ana_dig(1, 1);
					BK3000_wdt_reset();
					os_delay_us(3200);	//1ms
					USER_DBG_INFO("======== 2 =========\n");
				}
			}
		}else if(Init_Cont == 7){
//			gpio_output(WM_PO_EN, 1);
//			gpio_output(SGM_MUTE, 1);
//			gpio_output(ACM_PDN, 1);
			gpio_output(POWER_EN, 1);
			LED_Working();
			RgbLedAllColour(1);
		}else if(Init_Cont == 13){
			BK9532_Init();       //WMIC初始化
			USER_DBG_INFO("==== 13.\n");
		}else if(Init_Cont == 14){
	#ifdef ACM86xxId1
			ACM8625_init();      //第一颗ACM初始化
		}else if(Init_Cont == 15){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 15. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}
		}else if(Init_Cont == 16 && ACM862x_IIC_ADDR[1]){    //第二颗ACM初始化
		    ACM862xWId = 1;
		    SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    if(ACM_main()){
		    	Init_Cont--;
		    }else{
		    	gpio_output(Boot_EN, 1);
		    	ACM862xWId = 0;
		    	SetI2CAddr(ACM862x_IIC_ADDR[ACM862xWId]);
		    	USER_DBG_INFO("==== 16. ACM.2_ReSend Ok...%d MsCont:%d \n",SysMsCont);
		    	USER_DBG_INFO("==== DcDet:%d  CH_OK:%d\n",vusb_4v_ready(),Batt_FULL);
		    }
	#endif
		}else if(Init_Cont==17){
			EF_EQ_ReSend();
			EF_ModeR = EF_Mode;
			EQ_ModeR = EQ_Mode;
			EF_Mode = 1;        //上电默认唱歌模式 模式1
			USER_DBG_INFO("==== 13. EQ_ReSend Ok...MsCont:%d \n",SysMsCont);
		//--------------------------------------------------
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


		}else if(Init_Cont == 45){
			USER_DBG_INFO("================= g_mcu_log_on_fg = 0\n");
			g_dsp_log_on_fg = 0;
			g_mcu_log_on_fg = 0;

		}else if(Init_Cont == 46){
			Send_Id1 = CmdPid;
			Send_Id2 = 0x80;
			SendUpComp(SendBuff, 32);	// 請上位機連線
		}else if(Init_Cont == 47){
			g_dsp_log_on_fg = 1;
			g_mcu_log_on_fg = 1;
			USER_DBG_INFO("================= g_mcu_log_on_fg = 1\n");
		}else if(Init_Cont == 48){
			app_handle_t app_h = app_get_sys_handler();
			app_h->sys_work_mode = SYS_WM_BT_MODE;
//			system_work_mode_set_button(app_h->sys_work_mode);
//			TM16xx_Init(2);

		}else if(Init_Cont == 49){
			if(EF_Maim())	Init_Cont--;
		}else if(Init_Cont == 50){
#ifdef	CONFIG_AUD_AMP_DET_FFT
			extern void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
			aud_mag_det_enable(1);	//audio magnitude detect enable
#endif

		}else if(Init_Cont == 149){
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音
			SysInf.DuckDet = 0x00;   //闪避默认关

		}
	}
}



//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
//	UpDisplayAll();//刷新LED

	extern void LCD_ShowVol(uint8_t vol);
	if(Page==MainPid){
		if(Id2==3){
//			LCD_ShowVol((uint8_t)SysInf.MusicVol+80);
		}
	}
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
//		app_wave_file_play_stop();
//		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
		switch (EF_Mode){
			case 0:	app_wave_file_play_stop();app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_0); break;
			case 1:	app_wave_file_play_stop();app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_1);	break;
			case 2:
//				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
				break;
			case 3:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);	break;
			case 4:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);	break;
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

//extern void UpDisplayOff();
//********************************
void ShowPowerOff(void)
{
//	UpDisplayOff();
//	gpio_output(POWER_EN, 0);
}

static uint8_t tiaoguo = 0;    //跳过上电的第一次进入Knob_function的误现象

//***************************************************
void Knob_function()
{

	#define MaxV	3600	// 最大ADC 電壓 (n/4096)
	#define CenV	1900	// 旋鈕中心值電壓
	#define ScopeV	(MaxV/100)	//36

	float VolUp10 = (MaxV-CenV)/10;
	float EqUp6 =	(MaxV-CenV)/60;     //上6
	float KeyUp6 = (MaxV-CenV)/6;
	float EfUp10 = (MaxV-CenV)/10;

	float VolDn60 = (CenV/60);
	float EqDn9 =	(CenV/100);    //下10
	float KeyDn6 =  (CenV/6);
	float EfDn10 = (CenV/10);


    int VolVar1;

	int VolVar;
	int KeyVar;
	int EqVar;
	float EfVar;
	//=== 處理 ADC 滑桿,旋鈕 =============
	for(uint8_t i=0; i<8; i++){

		if(i==3)	i++;   //跳过电量检测的ADC

		if(abs(KnobValR[i]-KnobVal[i])>=ScopeV){    //旋
			if(tiaoguo){
//				USER_DBG_INFO("==旋转==");
				offled=1;    //前排灯关掉 标志位
			}
			if(KnobVal[i]>MaxV){     //限
				KnobValR[i] = MaxV;
			}else if(KnobVal[i]<ScopeV){   //限
				KnobValR[i] = 0;
			}else{                //√
				KnobValR[i] = KnobVal[i];
			}
			if(KnobValR[i] >= CenV){
				VolVar = (KnobValR[i]-CenV)/VolUp10;
//				if(VolVar >0) VolVar = 0;           //CJQ++   最大0DB
				KeyVar = (KnobValR[i]-CenV)/KeyUp6;
				EqVar =  (KnobValR[i]-CenV)/EqUp6;
				EfVar =  (KnobValR[i]-CenV)/EfUp10+10;
			}else{
				VolVar =((KnobValR[i]/VolDn60)-60);
				if(VolVar <-39) VolVar = -90;
				KeyVar =(KnobValR[i]/KeyDn6)-6;
				EqVar = (KnobValR[i]/EqDn9)-100;
				EfVar =  (KnobValR[i]/EfDn10);
			}
			if(VolVar>7)
				vol_level = 0x7fff;
			else if(VolVar<=7&&VolVar>4)
				vol_level = 0x7ffe;
			else if(VolVar<=4&&VolVar>1)
				vol_level = 0x7ffc;
			else if(VolVar<=1&&VolVar>-2)
				vol_level = 0x7ff8;
			else if(VolVar<=-2&&VolVar>-5)
				vol_level = 0x7ff0;
			else if(VolVar<=-5&&VolVar>-8)
				vol_level = 0x7fe0;
			else if(VolVar<=-8&&VolVar>-11)
				vol_level = 0x7fc0;
			else if(VolVar<=-11&&VolVar>-14)
				vol_level = 0x7f80;
			else if(VolVar<=-14&&VolVar>-17)
				vol_level = 0x7f00;
			else if(VolVar<=-17&&VolVar>-20)
				vol_level = 0x7e00;
			else if(VolVar<=-20&&VolVar>-23)
				vol_level = 0x7c00;
			else if(VolVar<=-23&&VolVar>-27)
				vol_level = 0x7800;
			else if(VolVar<=-30&&VolVar>-33)
				vol_level = 0x7000;
			else if(VolVar<=-33&&VolVar>-36)
				vol_level = 0x6000;
			else if(VolVar<=-36&&VolVar>-40)
				vol_level = 0x4000;
			else if(VolVar<=-40)
				vol_level = 0;

			//====================================================
			switch (i){

			case 0:	// Mic Tre
				UserEq[MicEq4_7].gain = EqVar;
				UserEq[MicEq3_7].gain = EqVar;
				break;

			case 1:	// Mic Mid
				UserEq[MicEq4_5].gain = EqVar;
				UserEq[MicEq3_5].gain = EqVar;
				break;

			case 2:	// Mic Bass
				UserEq[MicEq4_2].gain = EqVar;
				UserEq[MicEq3_2].gain = EqVar;
				break;

			case 4:	// Rev Vol
				if(EF_Mode!=0){      //原声模式下  无动作
					UserEf.RevVol = (float)EfVar/20;
					UserEf.RevVol2 = (float)EfVar/20;
					UserEf.EchoVol = (float)EfVar/10;
					UserEf.EchoVol2 = (float)EfVar/10;
				}
				break;

			case 5:	// Music Vol
				SysInf.MusicVol = (VolVar-10==-100)?-90:VolVar-10;

				break;

			case 6:	// Mic Vol
				SysInf.MicVol = VolVar;
				break;

			case 7:	// 監聽音量+功放音量
				VolVar1=VolVar-10;
				if(VolVar1 <-50) VolVar1 = -90;
//				if(VolVar1 ==-1) VolVar1 = 0;
				ioGain[Out1L_DacL] = VolVar1;
				ioGain[Out1R_DacR] = VolVar1;
				ioGain[Out2L_I2s2L] = VolVar1;
				ioGain[Out2R_I2s2R] = VolVar1;
				break;
			}

//			USER_DBG_INFO("====VolVar1: %d  VolVar:%d  \n",VolVar1, VolVar);
			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %d  %1.3f\n",i, KnobValR[i], KnobVal[i], VolVar, KeyVar, EqVar, EfVar);
			KnobValR[i]=KnobVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
	tiaoguo=1;
}

//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
	//USER_DBG_INFO("MicVol %d\n", SysInf.MicVol);
	//app_handle_t sys_hdl = app_get_sys_handler();

//	static uint8_t OnOff=0;
	switch (pKeyArray->event){
	case KEY_S_PRES: break;
	case KEY_S_REL:	break;
	//====================================================================================================
	case KEY_D_CLICK:	USER_DBG_INFO("KEY_D_CLICK\n");	break;// 短按 2次
	case KEY_T_CLICK:	USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);break;	// 短按 3次
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
	case KEY_L_PRES:
		USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
		switch (pKeyArray->index){
		case 1:
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
//		if(pKeyArray->index==0 && pKeyArray->keepTime > 100){
//			USER_DBG_INFO("KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
//			pKeyArray->keepTime = 0;
//			switch (pKeyArray->index){
//			case 0:
//				UpDisplayReg(LED_back[4], 1);   //电源灯开
//
//
//				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机
//
//			break;
//			}
//		}
		if(pKeyArray->keepTime > 1000){
//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
			if(pKeyArray->index==0){
				pKeyArray->keepTime = 0;
				PowerDownFg=1;
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
				//关LED
				UpDisplayOnOff(0);
//				PowerDownFg=1;   //往前挪
				USER_DBG_INFO("====PowerDownFg:%d!\n", PowerDownFg);
				RgbLedAllColour(0);	// OFF RGB LED
//				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
			}
		}
		break;
	case KEY_L_REL:
		USER_DBG_INFO("KEY_L_REL\n");	// 常按放開
		break;
	default:break;
	}
}


static uint8_t zsFlash=0;    //音效1闪灯标志位
static uint8_t xsFlash=0;    //音效2闪灯标志位
static uint8_t bliflag=0;    //音效播放闪灯标志位   0:灭灯   1:音效1闪灯   2:音效2闪灯

void PLAY_bli()
{
	if(bliflag==1)
	{
		if(zsFlash^=1)	UpDisplayReg(LED_back[3], 1);
			else		UpDisplayReg(LED_back[3], 0);
	}
	else if(bliflag==2)
	{
		if(xsFlash^=1)	UpDisplayReg(LED_back[8], 1);
			else		UpDisplayReg(LED_back[8], 0);
	}
	else if(bliflag==0)
	{
		UpDisplayReg(LED_back[3], 0);
		UpDisplayReg(LED_back[8], 0);
	}
//	UpDisplayAll();
}

#ifdef PromptToneMusicMute
//********************************
void PlayWaveStart(uint16_t id)
{
	USER_DBG_INFO("wave_start, ID:%d\n", id);

    //反馈点6解决  音效播放的同时有伴奏
	if((id==APP_WAVE_FILE_ID_MP3_MODE)||(id==APP_WAVE_FILE_ID_BT_MODE))
		IOGainSetDamp(MusMstVol, -90,10);            //Music off
	switch(id){
	case APP_WAVE_FILE_ID_POWEROFF:
		SysInf.VoiceCanEn = 0;	// 關閉消原音
		for(uint8_t i=0;i<16;i++)
		{
			StbFg[i]=0;
		}
		UpDisplayAll();
		uint8_t i;
		for(i=0;i<16;i++){
			os_printf("num = %d  StbFg:%02X \n",i, StbFg[i]);
		}
		USER_DBG_INFO("====PowerDownFg:%d!\n", PowerDownFg);
//		for(uint8_t i=0;i<63;i++){
//			UpDisplayReg(LED_TAB[i],0);
//		}
		break;
	//音效播放中 灯闪
	case APP_WAVE_FILE_ID_RESERVED6://掌声
		bliflag=1;
		ioGain[Tone_DacLR]=-5;
		break;

	case APP_WAVE_FILE_ID_RESERVED7://笑声
		bliflag=2;
		ioGain[Tone_DacLR]=-5;
		break;

	case APP_WAVE_FILE_ID_RESERVED8://暖场1
		bliflag=1;
		ioGain[Tone_DacLR]=-5;
		break;

	case APP_WAVE_FILE_ID_RESERVED9://暖场2
		bliflag=2;
		ioGain[Tone_DacLR]=-5;
		break;

	case APP_WAVE_FILE_ID_DISCONN:

		break;
	}
}
#endif

//*****************************************************
void PlayWaveStop(uint16_t id)
{
	switch(id){
	case APP_WAVE_FILE_ID_START:   //开机提示音结束
		if(WModeStep==1)	WModeStep =2;//UDISK
		if(WModeStep==0)	WModeStep =3;//BT
		break;
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");


		gpio_output(SGM_MUTE, 0);	//SGM监听耳机    高开低关
		gpio_output(WM_PO_EN, 0);   //WMIC      高开低关
		gpio_output(ACM_PDN, 0);    //AMP       高开低关
		gpio_output(Boot_EN, 0);    //升压      关

//		while(gpio_input(PowerKey)==0);
//		dsp_shutdown();
//		REG_SYSTEM_0x1D &= ~0x2;
//		system_wdt_reset_ana_dig(1, 1);
//		BK3000_wdt_reset();
//		gpio_output(POWER_EN, 0);
//		os_delay_us(3200);	//1ms

		//8.31更改关机方式
		while(gpio_input(PowerKey)==0);
		dsp_shutdown();
		gpio_output(POWER_EN, 0);   //电源按键开关     高开低关
		app_powerdown();


//		GroupWrite(0, 2);		// 保存播放記憶點


		break;
	case APP_WAVE_FILE_ID_CONN:     //蓝牙已连接
		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	 //連線自動播放
		}
		break;

	case APP_WAVE_FILE_ID_HFP_ACK:  //喇叭关
		gpio_output(ACM_PDN, 0);

		break;

	//音效播放完 灯灭
	case APP_WAVE_FILE_ID_RESERVED6://掌声
		bliflag=0;
		ioGain[Tone_DacLR]=-20;
		break;
	case APP_WAVE_FILE_ID_RESERVED7://笑声
		bliflag=0;
		ioGain[Tone_DacLR]=-20;
		break;
	case APP_WAVE_FILE_ID_RESERVED8://暖场1
		bliflag=0;
		ioGain[Tone_DacLR]=-20;
		break;
	case APP_WAVE_FILE_ID_RESERVED9://暖场2
		bliflag=0;
		ioGain[Tone_DacLR]=-20;
		break;



	case APP_WAVE_FILE_ID_MP3_MODE:			// UDISK
		if(player_get_play_status()==0){		// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
		break;
	case APP_WAVE_FILE_ID_RESERVED0:			// SD
		if(player_get_play_status()==0){		// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
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
	IOGainSetDamp(MusMstVol, SysInf.MusicVol,-10);
#endif
}
#endif


