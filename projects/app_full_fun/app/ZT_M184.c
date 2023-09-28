/**********************************************************************
* 文件:	user_ui.c
* 作者:	Chen JQ
* 版本:	V1.0
* 日期： 2023-04-20
* 描述：	8.30   u盘枚举完成后，才能识别到U盘，在记忆模式下，不同的优盘的枚举完成在程序上的位置不定，现象一：枚举完成时间很后，一直在等待的情况下，被切到BT（未定位到问题点）

**********************************************************************/
#include "USER_Config.h"

#ifdef ZT_M184

#include "ZT_M184_DEF.H"


const int8_t MusVol_TAB[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};

const int8_t MicVol_Tab[33]={	-90,-50,-45,-40,-36,-32,-28,-26,-24,-22,-20,-18,-16,\
						-14,-12,-10,-8,-7.5,-7,-6.5,-6,-5.5,-5,-4.5,-4,-3.5,-3,-2.5,-2,-1.5,-1,-0.5,0};
#define A1 0x80
#define B1 0x20
#define C1 0X08
#define D1 0X02
#define E1 0X01
#define F1 0X40
#define G1 0X10
#define H1 0X04
const uint8_t Num_Tab[40]=
{
	A1|B1|C1|D1|E1|F1|H1|0//第一个数码管显示0，电量指示灯点亮
	,A1|B1|C1|D1|E1|F1|0|0//第一个数码管显示0，电量指示灯点亮
	,B1|C1|H1|0|0|0|0|0//1
	,B1|C1|0|0|0|0|0|0//1
	,A1|B1|G1|E1|D1|H1|0|0//2
	,A1|B1|G1|E1|D1|0|0|0//2
	,A1|B1|G1|C1|D1|H1|0|0//3
	,A1|B1|G1|C1|D1|0|0|0//3
	,F1|G1|B1|C1|H1|0|0|0//4
	,F1|G1|B1|C1|0|0|0|0//4
	,A1|F1|G1|C1|D1|H1|0|0//5
	,A1|F1|G1|C1|D1|0|0|0//5
	,A1|F1|G1|C1|D1|E1|H1|0//6
	,A1|F1|G1|C1|D1|E1|0|0//6
	,A1|B1|C1|H1|0|0|0|0//7
	,A1|B1|C1|0|0|0|0|0//7
	,A1|F1|G1|C1|D1|E1|B1|H1//8
	,A1|F1|G1|C1|D1|E1|B1|0//8
	,A1|B1|C1|D1|F1|G1|H1|0//9
	,A1|B1|C1|D1|F1|G1|0|0//9
	,F1|E1|D1|C1|G1|H1|0|0//b
	,F1|E1|D1|C1|G1|0|0|0//b
	,B1|C1|D1|E1|G1|H1|0|0//d
	,B1|C1|D1|E1|G1|0|0|0//d
	,A1|B1|C1|H1|F1|E1|0|0//n
	,A1|B1|C1|F1|E1|0|0|0//n
	,G1|E1|H1|0|0|0|0|0//r
	,G1|E1|0|0|0|0|0|0//r
	,A1|F1|E1|D1|H1|0|0|0//C
	,A1|F1|E1|D1|0|0|0|0//C
	,A1|F1|E1|G1|D1|H1|0|0//E
	,A1|F1|E1|G1|D1|0|0|0//E
	,F1|E1|D1|H1|0|0|0|0//L
	,F1|E1|D1|0|0|0|0|0//L
	,A1|F1|G1|C1|D1|H1|0|0//S
	,A1|F1|G1|C1|D1|0|0|0//S
	,F1|E1|D1|C1|B1|H1|0|0//U
	,F1|E1|D1|C1|B1|0|0|0//U
	,H1|0|0|0|0|0|0|0//U
	,0
};
uint8_t MusVolValR=20;
uint8_t U_vol_index=4;    //伴奏音量 显示      标志位
uint8_t E_vol_index=4;    //混响音量 显示      标志位
uint8_t n_vol_index=4;    //麦克风音量 显示  标志位
uint8_t SD_start_index=6;
uint8_t Rec_index=0;
uint8_t L_index=4;

uint8_t L_version=0x04;
uint8_t bat_index_1=0;
uint8_t bat_index_2=0;
uint8_t bat_index_3=0;
uint8_t bat_index_4=0;
static uint8_t	ChargFlash=0;

uint8_t Bass_led_flag=0;
uint8_t POWER_ON_OFF=0;

//关于 充电
//static uint16_t Check_ChargeCont=0;
//static uint8_t add=0;				//判断是否开机，开机计数1
//static uint8_t addR=1;				//计数一次
//static uint8_t	Count=4;			//显示屏读秒
//static uint8_t	CountR=4;			//跑马灯计数
//static uint8_t	BatteryLedFlash=0;	//低电闪烁
#define DisplayCont_def	3		// 充電燈顯示延時(S)
uint16_t Battery_Cont=0;
uint16_t Battery_ContR=1;
static uint8_t DuckDetF = 0;               //闪避
static int8_t 	EC11_Sw,EC11_SwR;		//EC11 转动标志
static uint16_t PoKeyCont=1001;
uint8_t bat_flag=0;
//static uint16_t ms1000 = 0;
static int8_t PlayIng1 = 0;
uint8_t PlayType=2, PlayTypeR=0;
uint8_t exit_work_mode_En = 0;

#if (USB0_FUNC_SEL ==  USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
    uint8_t Usb_St_Det_Cont=0;
#endif

//extern uint8 appPlayerPlayMode;


static uint16_t WorkModeCont=0;
static uint8_t	BtLedFlash=0;
uint8_t LineInR=255;    //0为开line in1
uint8_t LineIn2R=255;   //0为开line in2
//static uint8_t	LineIn=3;
static uint16_t SysMsCont=0;


uint8_t Mus_ValR=255;
uint8_t Mic_ValR=255;
uint16_t WMicOffCont=0;
//uint8_t Mus_Val=16;
//uint8_t Mic_Val=16;


uint8_t MicDet_flag=0;
uint8_t PHONE_DETECT_falg=1;
uint8_t POW_W_OFF=0;
uint8_t ACM_PDN_Flag=1;

//********************************************************
uint8_t I2C_WriteD16(uint16_t Sdata)  //ZT_M184++
{
    uint8_t temp2;
    SET_SDA_IO(1);
	Start();					//发送起始命令
   // SendData((id<<1)&0xFE);		//发送设备地址+写命令
    //RecvACK();
   // SendData(addr);				//发送存储地址
   // RecvACK();
	temp2 = (uint8_t)(Sdata>>8);
	SendData(temp2);				// DATA_H
    RecvACK();
	temp2 = (uint8_t)(Sdata&0x00FF);
	SendData(temp2);				// DATA_L
    RecvACK();
    Stop();						//发送停止命令
    SET_SDA_IO(0);
//    USER_DBG_INFO("3.IIC   %d\n", Ack_Flag);
    return Ack_Flag;
}
uint8_t low_index=0;

uint16_t old_BattVol[10]={0};
uint16_t BattVolConvert(uint16_t AdcVol)
{
	float BattRv = ((float)BattLR/(BattLR+BattHR));	// 電池  輸入電阻(BattHR), 對地電阻 (BattLR)
	uint16_t AdcVol1 = SarADC_val_cali_get(AdcVol);	// 校正轉換
	uint16_t BattVol = (float)saradc_cvt_to_voltage(AdcVol1)/BattRv;
//	USER_DBG_INFO("==== AdcVol %d  %d  %d\n",AdcVol, AdcVol1, BattVol);
	return  BattVol;
}



//**** pre init ***********************
void user_init_prv(void)
{
	PowerDownFg=0;
	EC11_SwR=EC11_Sw;

	//低电开机限制
	SarAdcVal[2] = SarADC_val_cali_get(sar_adc_voltage_get(SARADC_CH_GPIO21));
	uint16_t BattVolConvert(uint16_t AdcVol);
	uint16_t BattVol = BattVolConvert(SarAdcVal[2]);
	USER_DBG_INFO("======== BattVol:%d\n",BattVol);
	uint16_t offset =400;   //偏移量
	if((BattVol-offset)<6000){   //   <6V
		USER_DBG_INFO("======== 1 =========\n");
		dsp_shutdown();
		REG_SYSTEM_0x1D &= ~0x2;
		system_wdt_reset_ana_dig(1, 1);
		BK3000_wdt_reset();
		os_delay_us(3200);	//1ms
		USER_DBG_INFO("======== 2 =========\n");
	}
}

void BattFun(void);

//**** normal init ********************
void user_init(void)
{
	//-------------------------------------------
	UserFlash_Init();	// 設置FLASH

	//方式一：强制切换
//	SysInf.WorkMode = SYS_WM_UDISK_MODE;	// 開機強制切換固定模式
	//方式二：记忆模式 （起始位置挂在蓝牙）
	app_handle_t app_h = app_get_sys_handler();
	app_h->sys_work_mode = SYS_WM_NULL;
	SysInfR.WorkMode = SYS_WM_BT_MODE; //	SysInfR.WorkMode = SYS_WM_NULL;

	set_aud_mode_flag(AUD_FLAG_GAME_MODE);	// 藍芽低延遲模式

#if LED_RGB_NUM
	app_i2s0_init(1);
//	RgbLedOneColour(7,4);
	//RgbLedOut(8);
	RgbLedAllColour(0xff);

#endif
//	user_saradc_update_trig();

	USER_DBG_INFO("======== V33_01_CE user_init End MsCont:%d WorkMode:%d\n",SysMsCont, SysInf.WorkMode);
    LOG_I(APP,"APP %s Build Time: %s, %s\n",__FILE__, __DATE__,__TIME__);
}


//********************************
void user_init_post(void)
{
	//WmPoSw=0;
    //---------------------
	Hi2c_Init();
	I2C_WriteD16(0X4801);
	I2C_WriteD16(0X4841);
	//=============================	GPIO配置

	//检测
	#define MicDet			GPIO30      //话筒检测
	#define PHONE_DETECT_IO	GPIO4      	//耳放检测
	#define DcInDet			GPIO32      //充电检测
	//使能
	#define PHONE_EN 		GPIO35      //监听耳机          高开低关
	#define ACM_PDN			GPIO3		//ACM功放 +升压使能               	高开低关
	#define UMic_CE			GPIO24      //U麦使能

	//短按开机
	#define SYS_EN			GPIO13		//系统供电使能      高开低自锁
	gpio_config_new(SYS_EN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_output(SYS_EN, 1);

	//LED灯
	#define LED_BT	        GPIO5      	//蓝牙闪烁指示灯
	#define POW_W			GPIO28      //电源白灯
	gpio_config_new(LED_BT, 	GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(POW_W, 	    GPIO_OUTPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_output(LED_BT, 0);
	gpio_output(POW_W, 0);



	//============================EC11 初始化配置
	Encoder_EC11_Init(0);

	gpio_config_new(PHONE_DETECT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(DcInDet, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(MicDet, GPIO_INPUT, GPIO_PULL_NONE, GPIO_PERI_NONE);
	gpio_config_new(GPIO2, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);     //红外遥控重新配置IO口。在MAIN进程 中 的InitPOST之前一直被拉低

	gpio_config_new(PHONE_EN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(ACM_PDN, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(UMic_CE, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
	gpio_config_new(GPIO19, 	GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_NONE);

	gpio_output(GPIO19,1);
	gpio_output(PHONE_EN, 1);
	gpio_output(ACM_PDN, 1);
	gpio_output(UMic_CE, 1);

//	SysInf.MicVol=7;    //初始化   麦克风音量为0
//	SysInf.MusicVol=15;

#ifdef LED_DRIVER_BY_I2S0_TX
    extern void user_app_i2s0_init(uint8_t en);
	user_app_i2s0_init(1);
#endif

	//============================PWM 初始化配置
#ifdef	USER_PWM_IO
	hal_pwm_wave_init(USER_PWM_IO, 1, 0, 260000, NULL);
	hal_pwm_enable(USER_PWM_IO, 1);
	//hal_pwm_duty_set(USER_PWM_IO, 0);
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
void EC11_FUN(void);
static uint8_t udisk_os = 1;    //开机后计时开关  Usb_St_Det_Cont只计算一次，作为开机优盘的枚举时间的阈值
//*******************************************
#ifdef CONFIG_SYS_TICK_INT_1MS
void tick_task_1ms(void) //timer0 1ms isr
{
	if(udisk_os)
		Usb_St_Det_Cont++;
	if(Usb_St_Det_Cont==1){
		udisk_os=0;
		Usb_St_Det_Cont=1;
	}
	//------------------------------
	static uint8_t Tick_2ms =0;
	Tick_2ms ^= 1;
	if(Tick_2ms){	// 2ms End
		user_saradc_update_trig();	// Adc 讀取觸發
	}else{
		if(EC11_SwR==EC11_Sw) {
			EC11_Sw = Encoder_EC11_Scan();	//EC11扫描动作
			EC11_FUN();                     //EC11执行动作
		}
	}
	SysMsCont++;
}
#endif

uint8_t led_en=1;
uint16_t temp;
uint8_t key_scan_time=0;
uint8_t key_scan_time_flag=0;
uint16_t max_vol_idnex=400;
//********************************************************
void EC11_FUN(void)
{
//	if((EC11_Sw==1))
//	{
//		key_scan_time_flag=1;
//	}
//	if((EC11_Sw==-1))
//	{
//		key_scan_time_flag=2;
//	}
//	if((key_scan_time_flag==1)&&(key_scan_time<10))
//	{
//		key_scan_time++;
//	}
//
//	if((key_scan_time_flag==2)&&(key_scan_time>0))
//	{
//		key_scan_time--;
//	}

	if((EC11_Sw==0) && (EC11_SwR==2)){
		if(PoKeyCont>=10){
			if(PoKeyCont>1000){
				PoKeyCont=0;
				return;
			}
			PoKeyCont=0;
//			DisplayCont = DisplayCont_def;
//			KeyId = BUTTON_BT_PLAY_PAUSE;
//			if(FadeInOut<150)	FadeInOut = 200;
		}
		USER_DBG_INFO("====P. EC11_Sw: %d  %d  %d\n",EC11_Sw, EC11_SwR, PoKeyCont);
	}else if((EC11_Sw !=2)&&(EC11_SwR != EC11_Sw )){
		EC11_SwR = EC11_Sw;
		if(EC11_SwR==1){
				if(SysInf.MusicVol<32)	SysInf.MusicVol++;

				if(SysInf.MusicVol==32&&max_vol_idnex>300)
				{
					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_9); //音量已最大
					max_vol_idnex=0;    //开始计时
				}
//				if(SysInf.MusicVol!=32)
//					max_vol_idnex=301;  //停止计时
				U_vol_index=0;
				if(led_en==1)
				{
					temp=SysInf.MusicVol;
					I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);
					I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
					if((temp/10)*2==0)
						I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
					else
						I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
					if((temp%10)*2==0)
						I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
					else
						I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
				}
				UpComputerRW = 0;	// 更新上位機
			USER_DBG_INFO("====U. EC11_Sw: %d  %3.1f\n",EC11_SwR,SysInf.MusicVol);
		}else if(EC11_Sw==-1){
			if(SysInf.MusicVol)	SysInf.MusicVol--;
			temp=SysInf.MusicVol;
			if(SysInf.MusicVol!=32)
				max_vol_idnex=301;
			U_vol_index=0;
			if(led_en==1)
			{
				I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);
				I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
				if((temp/10)*2==0)
					I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
				else
					I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
				if((temp%10)*2==0)
					I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
				else
					I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
				}
			//	USER_DBG_INFO("3.IIC   %d\n", temp);
			//	USER_DBG_INFO("====my_iic: %d  %3.1f\n",EC11_SwR,temp);
//			if(SysInf.MusicVol<32)	SysInf.MusicVol=SysInf.MusicVol-5;
			USER_DBG_INFO("====D. EC11_Sw: %d  %3.1f\n",EC11_SwR,SysInf.MusicVol);
		}
		PoKeyCont=0;
//		DisplayCont = DisplayCont_def;
		UpComputerRW = 0;	// 更新上位機
	}else if(EC11_Sw==2){
		PoKeyCont++;
		if(PoKeyCont==100){	// 1s
			USER_DBG_INFO("==== EC11_Sw: %d  %d\n",EC11_Sw, PoKeyCont);
//			app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
		}
	}
//	if(EC11_SwR != EC11_Sw) USER_DBG_INFO("==== EC11_Sw: %d  %d  %d\n",EC11_Sw, EC11_SwR, PoKeyCont);
	EC11_SwR = EC11_Sw;
}

uint16_t IR_time_index=0;       //红外接收数据处理计时器
uint16_t IR_time_index1=0;      //点歌数据计时器
uint8_t IR_buff[4]={0,0,0,0};   //红外接收BUF
uint8_t IR_buff_time=0;         //BUF下标索引
void user_ir_rx_callback(uint8_t *buff, int size)
{

	USER_DBG_INFO("ir rx buff[%d](HEX): %02X %02X %02X %02X\n", size, buff[0], buff[1], buff[2], buff[3]);
	if(buff[0]==0x00&&buff[1]==0xFF&&IR_time_index>10)    //地址码 00  地址反码FF   100ms接收一次(按键按下到松手不到100ms)
	{
		IR_time_index=0;
		switch (buff[2])
		{
			case 0x45://静音
				if(ioGain[Play_MusL]==-90)
				{
					ioGain[Play_MusL]=-4;
					ioGain[Play_MusR]=ioGain[Play_MusL];
				}
				else
				{
					ioGain[Play_MusL]=-90;
					ioGain[Play_MusR]=ioGain[Play_MusL];
				}
				break;
			case 0x44://数码管开关
				led_en^=1;
				break;
			case 0x46://循环
				if(appPlayerPlayMode==APP_PLAYER_MODE_PLAY_ALL_CYCLE){
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ONE;
					//app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);
				}else{
					appPlayerPlayMode=APP_PLAYER_MODE_PLAY_ALL_CYCLE;
					//app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);
				}
				//SysInf.SdCardPlayMode = appPlayerPlayMode;    //SD播放模式记忆
				break;
			case 0x47://模式
				system_work_mode_change_button();
				break;
			case 0x19://音量+
				if(SysInf.MusicVol<32)	SysInf.MusicVol++;
				if(SysInf.MusicVol==32&&max_vol_idnex>300)
				{
					max_vol_idnex=0;
					app_wave_file_play_stop();
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_9); //闪避开
				}
//				if(SysInf.MusicVol!=32)
//					max_vol_idnex=301;

				U_vol_index=0;
				if(led_en==1)
				{
					temp=SysInf.MusicVol;
					I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);
					I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
					if((temp/10)*2==0)
						I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
					else
						I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
					if((temp%10)*2==0)
						I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
					else
						I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
				}
				break;
			case 0x09://播放暂停
				if(((SysInf.WorkMode == SYS_WM_SDCARD_MODE)|(SysInf.WorkMode==SYS_WM_UDISK_MODE)))
				{	// 無播放
					if(PlayIng1==0)
						PlayIng1=1;
					else
						PlayIng1=0;
					if(PlayIng1==1)
						SD_start_index=0;
				}
				app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
				break;
			case 0x16://音量-
				if(SysInf.MusicVol) SysInf.MusicVol--;
				temp=SysInf.MusicVol;    //暂存音量
				if(SysInf.MusicVol!=32)
					max_vol_idnex=301;
				U_vol_index=0;
				if(led_en==1)
				{
					I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);   //U字符
					I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);   //灭
					if((temp/10)*2==0)
						I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
					else
						I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
					if((temp%10)*2==0)
						I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
					else
						I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
				}
				break;
			case 0x07://上一曲
				app_button_sw_action(BUTTON_BT_PREV);
				if((SysInf.WorkMode == SYS_WM_SDCARD_MODE)|(SysInf.WorkMode==SYS_WM_UDISK_MODE))
				{	// 無播放
					SD_start_index=0;
				}
				break;
			case 0x15://下一曲
				app_button_sw_action(BUTTON_BT_NEXT);
				if((SysInf.WorkMode == SYS_WM_SDCARD_MODE)|(SysInf.WorkMode==SYS_WM_UDISK_MODE))
				{	// 無播放
					SD_start_index=0;
				}
				break;
			case 0x40://话筒优先
				app_wave_file_play_stop();
				if(DuckDetF==0){
					DuckDetF=1;
					SysInf.DuckDet = 0x18;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7); //闪避开
				}else{
					DuckDetF=0;
					SysInf.DuckDet = 0x00;
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8); //闪避关
				}
				break;
			case 0x43://伴唱
				app_wave_file_play_stop();
				SysInf.VoiceCanEn ^=1;
				//app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL); //闪避开
				if(SysInf.VoiceCanEn)
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);//消音开
				else
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);//消音关
				break;
			case 0x0C://1
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=1;
				IR_buff_time++;
				break;
			case 0x18://2
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=2;
				IR_buff_time++;
				break;
			case 0x5E://3
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=3;
				IR_buff_time++;
				break;
			case 0X08://4
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=4;
				IR_buff_time++;
				break;
			case 0X1C://5
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=5;
				IR_buff_time++;
				break;
			case 0X5A://6
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=6;
				IR_buff_time++;
				break;
			case 0X42://7
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=7;
				IR_buff_time++;
				break;
			case 0X52://8
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=8;
				IR_buff_time++;
				break;
			case 0X4A://9
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=9;
				IR_buff_time++;
				break;
			case 0X0D://0
				IR_time_index1=0;
				if(IR_buff_time>3)
				{
					IR_buff_time=0;
				}
				IR_buff[IR_buff_time]=0;
				IR_buff_time++;
				break;
		}
	}
}






//==============================================
#ifdef	RECED_EN
static uint16_t RecTime = 0;
static uint8_t RecDrv = 0;
static uint16_t Rec_num;
static uint8_t RecState = 0;
static uint16_t RecPlayInx = 0;
static uint16_t RecPlayInxR = 0;
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
			Rec_index=0;
		}else{
			//recorder_start(RecDrv,1);
			RecState =13;
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	// 開始錄音提示音
			Rec_index=1;
		}
		RecPlayInxR = 1;
		break;
	case 2:	// 停止錄音
		Send_Id1 = MainPid;
		Send_Id2 = 204;
		Send_Group = 0;
		Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
		RecPlayInx = Rec_num;   //9.4更改
		USER_DBG_INFO("====WrRecPlayInx: %d\n",RecPlayInx);
		char Lab[64];
		rec_file_name_get_by_idx(Rec_num-1, Lab);  //根据索引获取文件名
		memcpy(&SendBuff[12], Lab,16);
		SendUpComp(SendBuff, 32);
		if(RecPlayInxR==1){
			RecState = 0;
			USER_DBG_INFO("====00+++++++++++++++++++\n");
		}else{
			RecState = 5;
			USER_DBG_INFO("====11+++++++++++++++++++\n");
		}
		RecPlayInxR=0;
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
			RecPlayInxR=0;  //8.21cjq++
			app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED5);
			USER_DBG_INFO("====44+++++++++++++++++++\n");
		}else{
			Rec_num = rec_file_total_num_get();  //获取总的录音文件数量（需要初始化文件系统）
			RecPlayInx = Rec_num;   //9.4更改
			USER_DBG_INFO("====222+++++++++++++++++++：%d \n",RecPlayInx);



			if(RecPlayInx){
				USER_DBG_INFO("====77+++++++++++++++++++\n");
				if(sd_is_attached() || udisk_is_attached()){	// RecPlay
					RecState = 16;
					USER_DBG_INFO("====33+++++++++++++++++++\n");
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
			rec_file_play_by_idx(RecPlayInx-1);//根据索引播放录音文件    //9.4更改
			USER_DBG_INFO("====RecPlayInx: %d\n",RecPlayInx);
			RecPlayInx = 0;
		}
		RecState = 0;
		break;
	}
}
#endif

static uint8_t flag_usb = 0;      //记忆在蓝牙和踢夫卡时跳过  优调切优模式  在进入循环5秒后flag_usb 恢复优调切优模式
//*****　插U盘会调用　＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
void user_udisk_ready_callback(void)
{
	USER_DBG_INFO("====66666666666666\n");
	USER_DBG_INFO("====66666666666666\n");
	USER_DBG_INFO("====Usb_St_Det_Cont:%d\n",Usb_St_Det_Cont);
	USER_DBG_INFO("====SysInf.WorkMode:%d\n",SysInf.WorkMode);

	if(1&&Usb_St_Det_Cont==1){
		if(flag_usb==0){
			system_work_mode_set_button(SYS_WM_UDISK_MODE);
			SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
//			Usb_St_Det_Cont=0;
		}
	}
}



uint8_t my_temp;
uint16_t old_temp=0;
uint8_t charg_index=0;
uint8_t Mic_index=0;
uint8_t Phone_index=01;
extern uint16_t my_tf_index;
//#define USER_PWM_FREQ_Hz        38000   //Hz
//#define USER_PWM_DUTY_ON        50      //percent
//
//#define PWM_REG_VAL_CYCLE       ((uint32_t)(26000000 / USER_PWM_FREQ_Hz))
//#define PWM_REG_VAL_DUTY        ((uint32_t)(PWM_REG_VAL_CYCLE * USER_PWM_DUTY_ON / 100))
//*****************************************************//控制LCD显示和音源链路音量设置
uint8_t mic_open_time=0;
void user_WorkModefun(void)
{
#ifdef RECED_EN
		RecFun();
#endif
		if(usbd_active_get())
			gpio_output(GPIO19,0);
		else
			gpio_output(GPIO19,1);

		if(gpio_input(DcInDet)==0)
		{
			ChargFlash=1;
			charg_index=0;
		}
		else if(gpio_input(DcInDet)==1)
		{
			if(charg_index<100)
			{
				charg_index++;
			}
			if(charg_index==100)
			{
				ChargFlash=0;
			}

		}
		if(SysInf.MicVol<=-30)
		{
			ioGain[Adc4_Mic]= -90;
		}
		else
		{
			ioGain[Adc4_Mic]= 0;
		}
		if((gpio_input(MicDet)==0)&&(MicDet_flag==0))
		{
			POW_W_OFF=1;
			mic_open_time=0;//
		}
		if(mic_open_time<20)
		{
			mic_open_time++;
		}
		if(mic_open_time==15)
		{
			ioGain[Adc4_Mic]= 0;
		}
		if(gpio_input(MicDet)==0)
		{
			MicDet_flag=1;
			Mic_index=0;
		}
		else if(gpio_input(MicDet)==1)
		{
			if(Mic_index<10)
			{
				Mic_index++;
			}
			//if(Mic_index==2)
			{
				MicDet_flag=0;
				ioGain[Adc4_Mic]= -90;
			}
		}

		//当 插入耳机  关闭功放
		if((gpio_input(PHONE_DETECT_IO)==0)&&(PHONE_DETECT_falg==0))
		{
			Phone_index=0;
			PHONE_DETECT_falg=1;
//			gpio_output(ACM_PDN, 0);
			ACM_PDN_Flag=0;
		}else if(gpio_input(PHONE_DETECT_IO)==1){
			if(Phone_index<10)
				Phone_index++;
			if(Phone_index==10)   		//100ms消抖 打开功放
			{
				Phone_index=101;        //挂空 防止二次进入
				PHONE_DETECT_falg=0;    //开机之后  才置0  |拔掉耳机之后 置0
				if(ACM_PDN_Flag==0)
					ACM_PDN_Flag=2;     //打开功放
			}
		}

//	//---- LINE DET ---------------------------
//	static uint8 LineInDetCont=0;
//	LineIn = gpio_input(LINE_DETECT_IO);
////	USER_DBG_INFO("====1. AuxIn: %d\n",AuxIn);
//	if(LineIn){
//		gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_PERI_NONE);
//		os_delay_us(500);
//		if(gpio_input(LINE_DETECT_IO)) LineIn = 0;
////		USER_DBG_INFO("====2. AuxIn: %d\n",AuxIn);
//		gpio_config_new(LINE_DETECT_IO, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
//	}
//	if(LineInR != LineIn){
//		if(++LineInDetCont>=20){
//			LineInDetCont=0;
//			LineInR = LineIn;
//			if(LineInR){
//				if(app_is_line_mode()){
//					system_work_mode_change_button();
//				}
//			}else{
//				system_work_mode_set_button(SYS_WM_LINEIN1_MODE);
//			}
//			USER_DBG_INFO("==== AuxInDet %d\n",LineInR);
//		}
//	}else{
//		LineInDetCont=0;
//	}

	app_handle_t sys_hdl = app_get_sys_handler();
	//=======================================================
	if(++WorkModeCont >= 100){
		if(POW_W_OFF==1)
		{
			POW_W_OFF=0;
			gpio_output(POW_W, 0);
		}
		else if(U_vol_index!=6)
		{
			gpio_output(POW_W, 1);
		}

		if(U_vol_index<4&&led_en==1)
		{
			U_vol_index++;
			temp=SysInf.MusicVol;
			I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
			{
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			}
			else
			{
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			}
			if((temp%10)*2==0)
			{
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			}
			else
			{
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			}
		}
		if(E_vol_index<4&&led_en==1)
		{
			E_vol_index++;
			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
			{
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			}
			else
			{
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			}
			if((temp%10)*2==0)
			{
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			}
			else
			{
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			}
		}
		if(n_vol_index<4&&led_en==1)
		{
			n_vol_index++;
		}

		temp=get_cur_music_gFileIdx();//获取当前播放的全局文件索引         //在蓝牙下该函数返回值，串口信息一直会打印错误信息
		if(((SD_start_index<6)|(temp!=old_temp))&&led_en==1&&((sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE)|(sys_hdl->sys_work_mode==SYS_WM_UDISK_MODE)))
		{
			SD_start_index++;
			//temp=get_cur_music_DirIdx();
			USER_DBG_INFO("====2. SD_Index0: %d\n",temp);
			if(temp==-1)
			{
				temp = my_tf_index;
			}
			//temp=get_cur_music_dFileIdx();//获取当前播放的目录中的文件索引
			//USER_DBG_INFO("====2. SD_Index1: %d\n",temp);

			old_temp=temp;
			temp++;
			USER_DBG_INFO("================temp: %d  %d \n",temp,get_cur_music_gFileIdx());
			USER_DBG_INFO("====2. LED: %d %d %d %d \n",(temp/1000),((temp/100)%10),((temp/10)%10),(temp%10));
			//USER_DBG_INFO("====2. SD_Index2: %d\n",temp);
			//I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			//I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/1000)*2==0)
			{
				I2C_WriteD16(0X6800|Num_Tab[0+bat_index_4]);
			}
			else
			{
				I2C_WriteD16(0X6800|Num_Tab[(temp/1000)*2+bat_index_4]);
			}
			if(((temp/100)%10)*2==0)
			{
				I2C_WriteD16(0X6A00|Num_Tab[0+bat_index_3]);
			}
			else
			{
				I2C_WriteD16(0X6A00|Num_Tab[((temp/100)%10)*2+bat_index_3]);
			}
			if(((temp/10)%10)*2==0)
			{
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			}
			else
			{
				I2C_WriteD16(0X6C00|Num_Tab[((temp/10)%10)*2+bat_index_2]);
			}
			if((temp%10)*2==0)
			{
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			}
			else
			{
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			}
		}
		else
		{
			SD_start_index=6;
		}
		if(E_vol_index==4&&led_en==1&&U_vol_index==4&&n_vol_index==4&&SD_start_index==6&&Rec_index==1)
		{
			I2C_WriteD16(0X6800|Num_Tab[26+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[30+bat_index_3]);
			I2C_WriteD16(0X6C00|Num_Tab[28+bat_index_2]);
			I2C_WriteD16(0X6E00|Num_Tab[38+bat_index_1]);
		}
		if(L_index<4&&led_en==1)
		{
			L_index++;
			I2C_WriteD16(0X6800|Num_Tab[32+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			I2C_WriteD16(0X6C00|Num_Tab[38+bat_index_2]);
			I2C_WriteD16(0X6E00|Num_Tab[L_version*2+bat_index_1]);
		}
		if((U_vol_index==6)|(led_en==0))
		{
			I2C_WriteD16(0X6800|Num_Tab[39]);
			I2C_WriteD16(0X6A00|Num_Tab[39]);
			I2C_WriteD16(0X6C00|Num_Tab[39]);
			I2C_WriteD16(0X6E00|Num_Tab[39]);
		}
		// 1秒計時

//		USER_DBG_INFO("==1== SysInf.WorkMode: %d %d %d\n",SysInf.WorkMode, SysInfR.WorkMode,sys_hdl->sys_work_mode);
		WorkModeCont = 0;
//		USER_DBG_INFO("==== bt_mode:%d  %d   Sd_Momd:%d  %d    %d\n",app_is_bt_mode(),a2dp_has_music(),app_is_mp3_mode(),player_get_play_status(),a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));//如果是播放状态
		//=============================================//系统工作模式：蓝牙模式
		if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				USER_DBG_INFO("==== SysInfR.WorkMode: %d %d %d\n",SysInf.WorkMode, SysInfR.WorkMode,sys_hdl->sys_work_mode);
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];

				USER_DBG_INFO("====31. SysInf.WorkMode: %d  %d\n",SysInf.WorkMode, sys_hdl->sys_work_mode);
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				gpio_output(LED_BT, 1);
				if(E_vol_index==4&&U_vol_index==4&&n_vol_index==4&&SD_start_index==6&&Rec_index==0&&L_index==4)
				{
					I2C_WriteD16(0X6800|Num_Tab[20+bat_index_4]);
					I2C_WriteD16(0X6A00|Num_Tab[32+bat_index_3]);
					I2C_WriteD16(0X6C00|Num_Tab[36+bat_index_2]);
					I2C_WriteD16(0X6E00|Num_Tab[30+bat_index_1]);
				}
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)
				{
					gpio_output(LED_BT, 1);
					if(E_vol_index==4&&led_en==1&&U_vol_index==4&&n_vol_index==4&&SD_start_index==6&&Rec_index==0&&L_index==4)
					{
						I2C_WriteD16(0X6800|Num_Tab[20+bat_index_4]);
						I2C_WriteD16(0X6A00|Num_Tab[32+bat_index_3]);
						I2C_WriteD16(0X6C00|Num_Tab[36+bat_index_2]);
						I2C_WriteD16(0X6E00|Num_Tab[30+bat_index_1]);
					}
				}
				else
				{
					gpio_output(LED_BT, 0);
					if(E_vol_index==4&&led_en==1&&U_vol_index==4&&n_vol_index==4&&SD_start_index==6&&Rec_index==0&&L_index==4)
					{
						I2C_WriteD16(0X6800|Num_Tab[38+bat_index_4]);
						I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
						I2C_WriteD16(0X6C00|Num_Tab[38+bat_index_2]);
						I2C_WriteD16(0X6E00|Num_Tab[38+bat_index_1]);
					}
				}
			}
		//=====================================================//系统工作模式：TF卡模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_SDCARD_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				SD_start_index=0;
				PlayIng1=1;
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
				USER_DBG_INFO("====32. SysInf.WorkMode: %d  %d\n",SysInf.WorkMode, sys_hdl->sys_work_mode);
			}
			if(E_vol_index==4&&led_en==1&&U_vol_index==4&&n_vol_index==4&&SD_start_index==6&&Rec_index==0&&L_index==4)
			{
				I2C_WriteD16(0X6800|Num_Tab[38+bat_index_4]);
				I2C_WriteD16(0X6A00|Num_Tab[34+bat_index_3]);
				I2C_WriteD16(0X6C00|Num_Tab[22+bat_index_2]);
				I2C_WriteD16(0X6E00|Num_Tab[38+bat_index_1]);
			}
				gpio_output(LED_BT, 0);

		//=====================================================//系统工作模式:U盘模式
		}else if(sys_hdl->sys_work_mode == SYS_WM_UDISK_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;
				//SD_start_index=0;
				PlayIng1=1;
				ioGain[Play_MusL]= -5;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -90;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
				//USER_DBG_INFO("====32. SysInf.WorkMode: %d  %d\n",SysInf.WorkMode, sys_hdl->sys_work_mode);
			}
			if(E_vol_index==4&&led_en==1&&U_vol_index==4&&n_vol_index==4&&SD_start_index==6&&Rec_index==0&&L_index==4)
			{
				I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);
				I2C_WriteD16(0X6A00|Num_Tab[34+bat_index_3]);
				I2C_WriteD16(0X6C00|Num_Tab[20+bat_index_2]);
				I2C_WriteD16(0X6E00|Num_Tab[38+bat_index_1]);
			}
				gpio_output(LED_BT, 0);

		//==================================================//系统工作模式：AUX模式
//		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE || sys_hdl->sys_work_mode == SYS_WM_LINEIN2_MODE){
		}else if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
			if(SysInf.WorkMode != sys_hdl->sys_work_mode){
				SysInf.WorkMode = sys_hdl->sys_work_mode;

//				if(sys_hdl->sys_work_mode == SYS_WM_LINEIN1_MODE){
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 0); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}else{
//					audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, 1); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
//				}

				ioGain[Play_MusL]= -90;
				ioGain[Play_MusR]= ioGain[Play_MusL];
				ioGain[Adc2_MusL]= -4;
				ioGain[Adc3_MusR]= ioGain[Adc2_MusL];
				USER_DBG_INFO("====33. SysInf.WorkMode: %d  %d\n",SysInf.WorkMode, sys_hdl->sys_work_mode);
			}
			if(hci_get_acl_link_count(sys_hdl->unit)){	         //如果是连接状态
				gpio_output(LED_BT, 0);
			}else{                                               //如果不是连接状态，蓝色灯闪烁
				if(BtLedFlash^=1)		gpio_output(LED_BT, 1);
					else				gpio_output(LED_BT, 0);
			}
		}
	}
}




void Knob_function();
static uint16_t Init_Cont = 0;
static uint32_t PowerDownCont=0;	// 15分 關機計時 (S)
static uint32_t PowerDownCont1=0;
//static uint16_t PowerDownCont2=0;	// 15分 關機計時 (S)
uint8_t usd_state=0;
uint32_t muise_index=0;
//********************************
void user_loop_10ms(void)
{

	//=============================================
	if(Init_Cont ==150){
		SysMsCont=0;
		//==== 開機切換 udisk mode 等 udisk 枚舉完畢  ====
//		if(Usb_St_Det_Cont){
//			Usb_St_Det_Cont--;
//			if(Usb_St_Det_Cont==0) system_work_mode_set_button(SysInf.WorkMode);
//		}

//		extern int udisk_is_enumerated();
//		if(udisk_is_enumerated()){
//			app_wave_file_play_start(APP_WAVE_FILE_ID_MP3_MODE);
//			USER_DBG_INFO("====udisk_is_enumerated \n");
//		}

		static uint8_t led_30ms=0;
		if(++led_30ms >=3){         //30ms刷新RBG灯效
			led_30ms=0;
//				void RGB_LED_EF_FUN(void);
//				RGB_LED_EF_FUN();
		}
		//音量达到MAX时，开始计时3秒
		if(max_vol_idnex<310)
		{
			max_vol_idnex++;
		}
		//红外接收 数据处理计时
		if(IR_time_index<55)
		{
			IR_time_index++;
		}
		//红外接收 点歌数据处理计时
		if(IR_time_index1<55)
		{
			IR_time_index1++;
		}
		//点歌数据显示  到达500ms暂显一次
		if(IR_time_index1==50)
		{
			if(IR_buff_time==1)
			{
				muise_index=IR_buff[0];//*1000+IR_buff[1]*100+IR_buff[2]*10+IR_buff[2]
			}
			else if(IR_buff_time==2)
			{
				muise_index=IR_buff[0]*10+IR_buff[1];//*1000+IR_buff[1]*100+IR_buff[2]*10+IR_buff[2]
			}
			else if(IR_buff_time==3)
			{
				muise_index=IR_buff[0]*100+IR_buff[1]*10+IR_buff[2];//*1000+IR_buff[1]*100+IR_buff[2]*10+IR_buff[2]
			}
			else if(IR_buff_time==4)
			{
				muise_index=IR_buff[0]*1000+IR_buff[1]*100+IR_buff[2]*10+IR_buff[3];//*1000+IR_buff[1]*100+IR_buff[2]*10+IR_buff[2]
			}
			if(muise_index!=0)
			{
				app_player_select_music(muise_index-1);		//点歌
			}
			USER_DBG_INFO("==== muise_index:%d\n",muise_index);
			IR_buff[0]=0;
			IR_buff[1]=0;
			IR_buff[2]=0;
			IR_buff[3]=0;
			IR_buff_time=0;
		}
		//----------------------------效果器
		if(EF_Maim()) return;
		//----------------------------功放
		if(ACM_main()) return;
		//----------------------------控制LCD显示和音源链路音量设置
		user_WorkModefun();
		//----------------------------KT麦克风
		KT56_main();
		//----------------------------电池电量检测
		BattFun();
		//----------------------按键扫描
		user_key_scan();
		//===================
//		EC11_FUN();          //挪到1ms计时间器

		app_handle_t sys_hdl = app_get_sys_handler();
		//==== 訊息報告 ====
		static uint16_t ms1000=0;
		if(++ms1000 >= 500){	// 5秒計時  功放报告
			ms1000 = 0;
			flag_usb = 0;       //复位usb_flag  U盘可正常回调

//			USER_DBG_INFO("==== muise_num:%d\n",get_music_gFileNumTotal());
		}

		//ACM_PDN_Flag   =0 关闭功放   =2 打开功放  =1常态
		if(ACM_PDN_Flag==0)
		{
			gpio_output(ACM_PDN, 0);
			ACM_PDN_Flag=1;
		}
		else if(ACM_PDN_Flag==2)
		{
			gpio_output(ACM_PDN, 1);
			extern uint8_t ACM862x_IIC_ADDR[2];
			ACM862x_IIC_ADDR[0]=0x2C;
			os_delay_ms(15);     //功放使能脚打开后需要间隔至少10ms才重新初始化功放配置
			ACM8625_init();      //cjq++    重新打开使能脚 ->>>重新初始化功放
			ACM_PDN_Flag=1;
		}



		//麦克风开和接入时，不做自动给关机    尚未写入
		//15分钟自动关机
		extern uint32_t user_aud_amp_task(void);
		if(user_aud_amp_task()==0)
		{
			if(++PowerDownCont1 >=90000){
				PowerDownCont1=0;
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
			}
		}
		else
		{
			PowerDownCont1=0;
//			if(ACM_PDN_Flag==3)
//			{
//				ACM_PDN_Flag=2;
//			}
		}

		//（藍芽模式并且蓝牙不连接状态）  |没声音| （ 非蓝牙模式下无播放）、、      15分钟关机
		if(((sys_hdl->sys_work_mode == SYS_WM_BT_MODE && hci_get_acl_link_count(sys_hdl->unit)==0)|(user_aud_amp_task()==0)|(sys_hdl->sys_work_mode != SYS_WM_BT_MODE && player_get_play_status()==0))&&(PHONE_DETECT_falg==0)){
			if(++PowerDownCont >=90000){
				PowerDownCont=0;
				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
			}
//			if(PowerDownCont==1000)
//			{
//				gpio_output(ACM_PDN, 0);
//				ACM_PDN_Flag=3;
//				USER_DBG_INFO("==== 10S:%d s\n", PowerDownCont);
//			}

			if(PowerDownCont%1000==0) USER_DBG_INFO("==== DelayPoOffCont:%d s\n", PowerDownCont/100);    //s
		}else{
			PowerDownCont=0;
		}

//		extern uint32_t user_aud_amp_task(void);
//		if(sys_hdl->sys_work_mode != SYS_WM_BT_MODE && user_aud_amp_task()){
//			PowerDownCont2=0;
//		}else if(sys_hdl->sys_work_mode != SYS_WM_BT_MODE && user_aud_amp_task()==0){//沒聲音偵測  15分 關機
//			if(++PowerDownCont2 >=900){
//				app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);	// 關機
//			}
//			if(PowerDownCont2%60==0) USER_DBG_INFO("==== DelayPoOffCont:%d  Vol:%d\n", PowerDownCont2, user_aud_amp_task());
//		}

//		USER_DBG_INFO("====SD:%d:\n", SysMsContsd_is_attached());     //SD卡在位 test
		if(SysMsCont>11) USER_DBG_INFO("====SysMsCont:%d:\n", SysMsCont);
	//================================================================
	}else{
		Init_Cont++;
		if(Init_Cont == 9){

//			BK9532_Init();       //WMIC初始化
			KT56_Init();
		}else if(Init_Cont == 10){

			ACM8625_init();      //ACM初始化
		}else if(Init_Cont == 11){
			if(ACM_main()){
				Init_Cont--;
			}else{
				USER_DBG_INFO("==== 11. ACM_ReSend Ok...%d MsCont:%d \n",SysMsCont);
			}

		}else if(Init_Cont == 12){
			EF_EQ_ReSend();
			EF_ModeR = EF_Mode;
			EQ_ModeR = EQ_Mode;
			USER_DBG_INFO("==== 12. EQ_ReSend Ok...MsCont:%d \n",SysMsCont);

//			KT56_Init();

		}else if(Init_Cont==13){
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

		}
		else if(Init_Cont==14)
		{
			EQ_Mode=SysInf.EqGruup;
			if(EQ_Mode==0)
			{
				hal_pwm_duty_set(USER_PWM_IO, 260000);
			}
			else if(EQ_Mode==1)
			{
				hal_pwm_duty_set(USER_PWM_IO, 10000);
			}
			else
			{
				hal_pwm_duty_set(USER_PWM_IO, 0);
			}
			gpio_output(LED_BT, 1);
			gpio_output(POW_W, 1);

			bat_index_4=1;
			bat_index_3=1;
			bat_index_2=1;
			bat_index_1=1;

			if(SysInf.WorkMode == SYS_WM_BT_MODE)
			{
				I2C_WriteD16(0X6800|Num_Tab[20+bat_index_4]);
				I2C_WriteD16(0X6A00|Num_Tab[32+bat_index_3]);
				I2C_WriteD16(0X6C00|Num_Tab[36+bat_index_2]);
				I2C_WriteD16(0X6E00|Num_Tab[30+bat_index_1]);
			}
			else if(SysInf.WorkMode == SYS_WM_SDCARD_MODE)
			{
				I2C_WriteD16(0X6800|Num_Tab[38+bat_index_4]);
				I2C_WriteD16(0X6A00|Num_Tab[34+bat_index_3]);
				I2C_WriteD16(0X6C00|Num_Tab[22+bat_index_2]);
				I2C_WriteD16(0X6E00|Num_Tab[38+bat_index_1]);
			}
			else
			{
				I2C_WriteD16(0X6800|Num_Tab[36+bat_index_4]);
				I2C_WriteD16(0X6A00|Num_Tab[34+bat_index_3]);
				I2C_WriteD16(0X6C00|Num_Tab[20+bat_index_2]);
				I2C_WriteD16(0X6E00|Num_Tab[38+bat_index_1]);
			}
		}
		else if(Init_Cont == 15){
			gpio_output(SYS_EN, 1);     //低为自锁
			app_wave_file_play_start(APP_WAVE_FILE_ID_START);    //开机提示音
			SysInf.DuckDet = 0x00;   //闪避默认关
		}else if(Init_Cont == 20){
			EF_EQ_ReSend();
			USER_DBG_INFO("===BAT_VOL===:%d\n", SarAdcVal[3]);  //电池电量查看
		}else if(Init_Cont == 50){

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
			EC11_SwR = EC11_Sw;
		}else if(Init_Cont == 148){

			U_vol_index=4;
			E_vol_index=4;
			n_vol_index=4;
			SD_start_index=6;
			Rec_index=0;

//			if(SysInf.MicVol %2!=0)
//			{
//					SysInf.MicVol--;
//			}
			if(SysInf.WorkMode == SYS_WM_UDISK_MODE){
//				app_handle_t app_h = app_get_sys_handler();

//				if(app_h->sys_work_mode==SYS_WM_NULL)	Usb_St_Det_Cont=20;

			}else{
				system_work_mode_set_button(SysInf.WorkMode);
				SysInf.WorkMode = SYS_WM_UDISK_MODE;  //预设U盘状态
				flag_usb=1;
//				SysInf.WorkMode = 255;	// 確保會跑一次切換顯示
			}
		}
	}
}

//static uint8_t ton=0;


//********************************************
void DisPlay_UpData(uint8_t Page, uint8_t Id2)
{
	extern void LCD_ShowVol(uint8_t vol);
	if(Page==MainPid){
		if(Id2==3){
//			LCD_ShowVol((uint8_t)SysInf.MusicVol+80);
		}
	}
}
////***************************************************    //新加，根据CPS
//void Mic_VolSet(float val)
//{
//	UpComputerRW =0;
//	SysInf.MicVol = val;
//	ioGain[MicMstVol] = val;
//	IOGainSet(MicMstVol, val);
//}
//***************************************************
void Out_VolSet(float val)
{

}

//***************************************************
void EF_ClewToneGr(uint8_t Gr)
{
	static uint8_t play_en =0;
	if(play_en){
//		app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_REJECT);
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
//		app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);
		switch (EQ_Mode){
		case 0:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_3); break;
		case 1:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_2);	break;
		case 2:	app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_4);	break;
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
	#define MaxV	4100	// 最大ADC 電壓 (n/4096)
	#define CenV	3400	// 旋鈕中心值電壓
	#define ScopeV	(MaxV/36)	//100

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
	for(uint8_t i=1; i<3; i++){    //2个旋钮。i=1时，SarAdcChVal[1] i=2时，SarAdcChVal[2]，1和2都是旋钮  。 i=0时，SarAdcChVal[0]是按键
		SarAdcVal[i] = SarADC_val_cali_get(SarAdcVal[i]);	// SarADC_val_cali_get 校正 ADC 偏差值
//		SarAdcVal[i] = (SarAdcVal[i]+SarAdcValR[i])>>1; //取平均值
		if(abs(SarAdcValR[i]-SarAdcVal[i])>=ScopeV){
			if(SarAdcVal[i]>MaxV){
				SarAdcValR[i] = MaxV;
			}else if(SarAdcVal[i]<ScopeV){
				SarAdcValR[i] = 0;
			}else{
				SarAdcValR[i] = SarAdcVal[i];
			}
			if(SarAdcValR[i] >= CenV){
				VolVar = (SarAdcValR[i]-CenV)/VolUp10;
				KeyVar = (SarAdcValR[i]-CenV)/KeyUp6;
				EqVar =  (SarAdcValR[i]-CenV)/EqUp12;
				EfVar =  (SarAdcValR[i]-CenV)/EfUp10+10;
			}else{
				VolVar =(SarAdcValR[i]/VolDn60)-60;
				if(VolVar <-59) VolVar = -90;
				KeyVar =(SarAdcValR[i]/KeyDn6)-6;
				EqVar = (SarAdcValR[i]/EqDn12)-120;
				EfVar =  (SarAdcValR[i]/EfDn10);
			}
			//====================================================
			switch (i){
			case 1:	// Music Vol
				SysInf.MusicVol = VolVar;
				break;
			case 5:	// Mic Vol
				SysInf.MicVol = VolVar;
				break;
			case 4: // Rec Vol
				UserEf.RevVol = (float)EfVar/20;
				UserEf.RevVol2 = (float)EfVar/20;
				UserEf.EchoVol = (float)EfVar/10;
				UserEf.EchoVol2 = (float)EfVar/10;
				break;
			case 2: // BASS Vol
				UserEq[MicEq3_2].gain = EqVar;
				UserEq[MicEq2_2].gain = EqVar;
				UserEq[MicEq1_2].gain = EqVar;
				break;
			case 3: // Tre Vol
				UserEq[MicEq3_6].gain = EqVar;
				UserEq[MicEq2_6].gain = EqVar;
				UserEq[MicEq1_6].gain = EqVar;
				break;
			}
			USER_DBG_INFO("====SarAdcValR: %d  %d  %d  %d  %d  %1.3f\n",i, SarAdcVal[i], VolVar, KeyVar, EqVar, EfVar);
			SarAdcValR[i]=SarAdcVal[i];
			UpComputerRW = 0;	// 設定上位機同步更新
		}
	}
}

//***************************************************
void key_function(KEY_CONTEXT_t *pKeyArray)
{
	USER_DBG_INFO("====index:%d:  event:%d\n", pKeyArray->index, pKeyArray->event);
//	return;
//	app_handle_t sys_hdl = app_get_sys_handler();
	//app_handle_t sys_hdl = app_get_sys_handler();
	switch (pKeyArray->event){
	case KEY_S_PRES: break;

	case KEY_S_REL:
		app_wave_file_play_stop();
		switch (pKeyArray->index){

		case 0:	// 模式切换

			system_work_mode_change_button();
			break;

		case 9:	// 上一首

			app_button_sw_action(BUTTON_BT_PREV);
			if((SysInf.WorkMode == SYS_WM_SDCARD_MODE)|(SysInf.WorkMode==SYS_WM_UDISK_MODE))
			{	// 無播放
				SD_start_index=0;
			}

			break;
		case 3:	// 下一首

			app_button_sw_action(BUTTON_BT_NEXT);
			if((SysInf.WorkMode == SYS_WM_SDCARD_MODE)|(SysInf.WorkMode==SYS_WM_UDISK_MODE))
			{	// 無播放
				SD_start_index=0;
			}

			break;
		case 8:	// MIC+
			USER_DBG_INFO("MicVol %f\n", SysInf.MicVol);
			if(SysInf.MicVol < -10) 	SysInf.MicVol=SysInf.MicVol+2;
			n_vol_index=0;
			temp=SysInf.MicVol;
			temp=temp/2+15;
			I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 2:	// MIC-
			if(SysInf.MicVol>-30) 	SysInf.MicVol=SysInf.MicVol-2;
			n_vol_index=0;
			temp=SysInf.MicVol;
			temp=temp/2+15;
			I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 5:	// EQ
			if(++EQ_Mode >= 3)	EQ_Mode=0;
			if(EQ_Mode==0)
				hal_pwm_duty_set(USER_PWM_IO, 260000);
			else if(EQ_Mode==1)
				hal_pwm_duty_set(USER_PWM_IO, 10000);
			else
				hal_pwm_duty_set(USER_PWM_IO, 0);
			break;
		case 1:	//消原音   伴唱
			app_wave_file_play_stop();
			POW_W_OFF=1;
			SysInf.VoiceCanEn ^=1;
			if(SysInf.VoiceCanEn)
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);//消音开
			else
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);//消音关
			break;
		case 7:	// 话筒优先
			app_wave_file_play_stop();
			POW_W_OFF=1;
			if(DuckDetF==0){
				DuckDetF=1;
				SysInf.DuckDet = 0x18;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7); //闪避开
			}else{
				DuckDetF=0;
				SysInf.DuckDet = 0x00;
				app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8); //闪避关
			}
			break;
		case 10:	// 混响+
			if(UserEf.RevVol < 1.0f||UserEf.EchoVol < 2.0f){
				UserEf.RevVol += 0.1f;
				UserEf.RevVol2=UserEf.RevVol;
				UserEf.EchoVol += 0.2f;
				UserEf.EchoVol2=UserEf.EchoVol;
				E_vol_index=0;
			}

			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 4:	// 混响-
			if(UserEf.RevVol >0.09f||UserEf.EchoVol>0.09f){
				UserEf.RevVol -= 0.1f;
				UserEf.RevVol2=UserEf.RevVol;
				UserEf.EchoVol -= 0.2f;
				UserEf.EchoVol2=UserEf.EchoVol;
				E_vol_index=0;
			}
			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 6:// BT
		//	if(FadeInOut<150)	FadeInOut = 200;
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
			break;
		case 11:// 录音
			//当没有插入U盘和SD卡的时候，就报“请插入U盘或者TF卡后录音”
			if(sd_is_attached()==0 && udisk_is_attached()==0){
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);	// “请插入U盘或者TF卡后录音”
			}else{
				user_RecWork();
			}
//						break;
			break;
		case 12:// 播放|暂停
			//if(FadeInOut<150)	FadeInOut = 200;
			if(((SysInf.WorkMode == SYS_WM_SDCARD_MODE)|(SysInf.WorkMode==SYS_WM_UDISK_MODE))){	// 無播放
				if(PlayIng1==0)
					PlayIng1=1;
				else
					PlayIng1=0;
				if(PlayIng1==1)
					SD_start_index=0;
			}
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
			break;
		}
		break;
	//====================================================================================================
	case KEY_D_CLICK:
		app_wave_file_play_stop();
		switch (pKeyArray->index){
		case 11:// 停止录音，保存好录音，并播放录音文件，新录入的文件最优先播放，依次播放已存录音文件，
			if(sd_is_attached()==0 && udisk_is_attached()==0)
				app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_ACK);	// “请插入U盘或者TF卡后录音”
			else
				user_RecPlayWork();
			break;
		case 7:
			if(EF_Mode==0)
				EF_Mode=1;
			else if(EF_Mode==1)
				EF_Mode=2;
			else
				EF_Mode=0;
			break;
		case 8:	// MIC+
			USER_DBG_INFO("MicVol %f\n", SysInf.MicVol);
			if(SysInf.MicVol < 10) 	SysInf.MicVol=SysInf.MicVol+2;
			n_vol_index=0;
			temp=SysInf.MicVol;
			temp=temp/2+5;
			I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);

			USER_DBG_INFO("MicVol %f\n", SysInf.MicVol);
			break;
		case 2:	// MIC-
			if(SysInf.MicVol>-10) 	SysInf.MicVol=SysInf.MicVol-2;
			n_vol_index=0;
			temp=SysInf.MicVol;
			temp=temp/2+5;
			I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 10:	// 混响+
			if(UserEf.RevVol < 1.0f||UserEf.EchoVol < 2.0f){
				UserEf.RevVol += 0.1f;
				UserEf.RevVol2=UserEf.RevVol;
				UserEf.EchoVol += 0.2f;
				UserEf.EchoVol2=UserEf.EchoVol;
				E_vol_index=0;
			}

			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);

			break;
		case 4:	// 混响-
			if(UserEf.RevVol >0.09f||UserEf.EchoVol>0.09f){
				UserEf.RevVol -= 0.1f;
				UserEf.RevVol2=UserEf.RevVol;
				UserEf.EchoVol -= 0.2f;
				UserEf.EchoVol2=UserEf.EchoVol;
				E_vol_index=0;
			}
			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		}
		break;// 短按 2次

	case KEY_T_CLICK:
		switch(pKeyArray->index)
		{
		case 8:	// MIC+
			USER_DBG_INFO("MicVol %f\n", SysInf.MicVol);
			if(SysInf.MicVol < 10) 	SysInf.MicVol=SysInf.MicVol+2;
			n_vol_index=0;
			temp=SysInf.MicVol;
			temp=temp/2+5;
			I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);

			USER_DBG_INFO("MicVol %f\n", SysInf.MicVol);
			break;
		case 2:	// MIC-
			if(SysInf.MicVol>-10) 	SysInf.MicVol=SysInf.MicVol-2;
			n_vol_index=0;
			temp=SysInf.MicVol;
			temp=temp/2+5;
			I2C_WriteD16(0X6800|Num_Tab[24+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 10:	// 混响+
			if(UserEf.RevVol < 1.0f||UserEf.EchoVol < 2.0f){
				UserEf.RevVol += 0.1f;
				UserEf.RevVol2=UserEf.RevVol;
				UserEf.EchoVol += 0.2f;
				UserEf.EchoVol2=UserEf.EchoVol;
				E_vol_index=0;
			}

			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		case 4:	// 混响-
			if(UserEf.RevVol >0.09f||UserEf.EchoVol>0.09f){
				UserEf.RevVol -= 0.1f;
				UserEf.RevVol2=UserEf.RevVol;
				UserEf.EchoVol -= 0.2f;
				UserEf.EchoVol2=UserEf.EchoVol;
				E_vol_index=0;
			}
			temp=UserEf.RevVol*10;
			I2C_WriteD16(0X6800|Num_Tab[30+bat_index_4]);
			I2C_WriteD16(0X6A00|Num_Tab[38+bat_index_3]);
			if((temp/10)*2==0)
				I2C_WriteD16(0X6C00|Num_Tab[0+bat_index_2]);
			else
				I2C_WriteD16(0X6C00|Num_Tab[(temp/10)*2+bat_index_2]);
			if((temp%10)*2==0)
				I2C_WriteD16(0X6E00|Num_Tab[0+bat_index_1]);
			else
				I2C_WriteD16(0X6E00|Num_Tab[(temp%10)*2+bat_index_1]);
			break;
		}

		USER_DBG_INFO("KEY_T_CLICK index:%d\n",pKeyArray->index);break;	// 短按 3次
	case KEY_Q_CLICK:	USER_DBG_INFO("KEY_Q_CLICK\n");	break;	// 短按 4次
	case KEY_5_CLICK:	//USER_DBG_INFO("KEY_5_CLICK\n");	break;	// 短按 5次
		switch (pKeyArray->index){
		case 12:
			app_wave_file_play_stop();
			app_wave_file_play_start(APP_WAVE_FILE_ID_CLEAR_MEMORY);//L_index=0;
			break;
		}
		break;// 短按 2次
	case KEY_L_PRES:
		USER_DBG_INFO("KEY_L_PRES Index: %d\n",pKeyArray->index);// 常按進入
		switch (pKeyArray->index){
		case 1:
			break;
		}
		break;
	case KEY_L_PRESSING:	// 常按 循環
		if(pKeyArray->keepTime%100==0){
		//			USER_DBG_INFO("2.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
					switch (pKeyArray->index){
					case 2:
						break;
					case 3:
						break;
					case 4:
						break;
					}
				}else if(pKeyArray->keepTime > 2000){
		//			USER_DBG_INFO("3.KEY_L_PRESSING %d   %d\n", pKeyArray->clickCnt, pKeyArray->keepTime);
					//pKeyArray->keepTime = 10;
					if(pKeyArray->index==0&&pKeyArray->keepTime >2000&&pKeyArray->keepTime <2200){
						pKeyArray->keepTime = 2200;
						U_vol_index=6;
						app_wave_file_play_stop();

						app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
					}
					if(pKeyArray->index==12&&pKeyArray->keepTime >2000&&pKeyArray->keepTime <2200&&PHONE_DETECT_falg==1){
						pKeyArray->keepTime = 2200;
						if(ACM_PDN_Flag==1)
							ACM_PDN_Flag=0;
						else
							ACM_PDN_Flag=2;
					}
					if(pKeyArray->index==6&&pKeyArray->keepTime>5000&&pKeyArray->keepTime>5200){
						pKeyArray->keepTime=5200;
						app_button_sw_action(BUTTON_BT_CLEAR_MEMORY);
						app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_6);
					}
				}
		break;
	case KEY_L_REL:	USER_DBG_INFO("KEY_L_REL\n");	USER_DBG_INFO("MusicVol %d\n", SysInf.MusicVol); break;	// 常按放開

	default:break;
	}
}



//********************************
#ifdef PromptToneMusicMute
void PlayWaveStart(uint16_t id)
{
	short temp;
	float tem;
#if 0
	if(id==APP_WAVE_FILE_ID_ENTER_PARING || id==APP_WAVE_FILE_ID_HFP_RING){
		USER_DBG_INFO("!!!!!!!!!!!!!! wave_start, ID:%d\n", id);
		return;
	}
#endif
	USER_DBG_INFO("wave_start, ID:%d\n", id);
	IOGainSetDamp(MusMstVol, -90,10);      //Music Off
	app_handle_t sys_hdl = app_get_sys_handler();
	switch(id){

	case APP_WAVE_FILE_ID_POWEROFF:

		PoKeyCont=0;
		PowerDownFg=1;
		SysInf.VoiceCanEn = 0;	// 關閉消原音

		if(hci_get_acl_link_count(sys_hdl->unit)){ //如果是连接状态
			app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		}

		temp=SysInf.MicVol;
		tem=SysInf.MusicVol;
		SysInf.WorkMode = sys_hdl->sys_work_mode;  //记忆模式
		SysInf.EqGruup=EQ_Mode; //记忆均衡
		SysInf.MicVol=temp;    	//记忆麦克风音量
		SysInf.MusicVol=tem;    //记忆音乐音量

		GroupWrite(0, 2);		// 保存播放記憶點
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
#ifdef	RECED_EN
	case APP_WAVE_FILE_ID_RESERVED4:	// 開始錄音
		RecState = 3;
		break;
	case APP_WAVE_FILE_ID_RESERVED5:	// 停止錄音
		RecState = 2;
		break;
#endif
	case APP_WAVE_FILE_ID_START:
		break;
	case APP_WAVE_FILE_ID_POWEROFF:
		USER_DBG_INFO("=== PowerOffWaveStop Po OFF...\n");
		gpio_output(PHONE_EN, 0);
		gpio_output(ACM_PDN, 0);
		//关灯，关显示
		hal_pwm_duty_set(USER_PWM_IO, 0);
		gpio_output(LED_BT, 0);
		gpio_output(POW_W, 0);
		I2C_WriteD16(0X6800|Num_Tab[39]);
		I2C_WriteD16(0X6A00|Num_Tab[39]);
		I2C_WriteD16(0X6C00|Num_Tab[39]);
		I2C_WriteD16(0X6E00|Num_Tab[39]);

		while(gpio_input(GPIO15)==0);
		app_button_sw_action(BUTTON_BT_CONN_DISCONN);
		dsp_shutdown();
		app_powerdown();
		gpio_output(SYS_EN, 0);	///POWER_EN



		//app_button_sw_action(BUTTON_BT_POWERDOWN);

		break;
	case APP_WAVE_FILE_ID_CONN:

		USER_DBG_INFO("=== BT Mode...%d\n",a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE));
		if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE)==0){	// 無播放
			SD_start_index=0;
			//app_button_sw_action(BUTTON_BT_PLAY_PAUSE);	//連線自動播放
		}

		break;
	case APP_WAVE_FILE_ID_BT_MODE:
		break;

	case APP_WAVE_FILE_ID_MP3_MODE:			// UDISK
	case APP_WAVE_FILE_ID_RESERVED0:			// SD
		if(RecState==16){
			RecState = 6;       //播放最新录音
		}
		if(player_get_play_status()==0){		// 1:正在播放， 0:没有播放。	yuan++
			app_button_sw_action(BUTTON_BT_PLAY_PAUSE);
		}
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




//==== 关于 充电 =========================================
#define DisplayCont_def	3		// 充電燈顯示延時(S)
//uint8_t DisplayCont = 1;
//float Battery_Cont=0;
//static uint8_t Batt_FULL=0;
//static uint16_t BattVn = 0x01;
static uint8_t LowPowerEn=0;
static uint8_t LowPowerCont=0;		// 進入低功耗計數
static uint16_t DelayOffCont=0;
static uint16_t DisplayCont=0;
static uint8_t OffPowerCont = 0;


#define Batt_FULL	0x55
#define	fullVn			8300	// 滿電壓 mv
#define	P4Voln			8000	// 亮4個燈電壓
#define	P3Voln			7600	// 亮3個燈電壓
#define	P2Voln			7200	// 亮2個燈及解除低電模式電壓
#define	LoPowerENv		6800	// 低電模式電壓
#define	LoPowerOFFv	 	6400	// 低電關機電壓

//#define	fullVn			12600	// 滿電壓 mv
//#define	P4Voln			11700	// 亮4個燈電壓
//#define	P3Voln			10800	// 亮3個燈電壓
//#define	P2Voln			9900	// 亮2個燈及解除低電模式電壓
//#define	LoPowerENv		9400	// 低電模式電壓
//#define	LoPowerOFFv	 	9000	// 低電關機電壓
//***************************************************

//*************************************************
void BattFun(void)
{
	//--------------------------------
	static uint16_t BattVolDetMs = 99;
//	static uint16_t LowBattCont = 0;
	if(++BattVolDetMs >= 100){
		BattVolDetMs=0;
		//===================================================================================
		uint16_t BattVol = BattVolConvert(SarAdcVal[2]);
		if(old_BattVol[0]==0)
		{	for(uint8_t i=0;i<10;i++)
			{
				old_BattVol[i]=BattVol;
			}
		}

		for(uint8_t i=1;i<10;i++)
		{
			old_BattVol[i-1]=old_BattVol[i];
		}
		old_BattVol[9]=BattVol;
		int bat_temp=0;
		for(uint8_t i=0;i<10;i++)
		{
			bat_temp+=old_BattVol[i];
		}
		BattVol=bat_temp/10;
	//	BattVol=8300;
		static uint16_t saradc_tempe_dataR;
		static uint8_t Batt_FULLR;
		 uint16_t saradc_tempe_data;//温度传感器更新的SarADC数字值，10s更新一次。
		//--------------------------------
		fl2hex V1;
		if((abs(SarAdcValR[2]-SarAdcVal[2])>10) || (saradc_tempe_dataR != saradc_tempe_data  || Batt_FULLR != Batt_FULL)){
			SarAdcValR[2] =SarAdcVal[2];
			saradc_tempe_dataR = saradc_tempe_data;
			Batt_FULLR = Batt_FULL;
			Send_Id1 = MainPid;
			Send_Id2 = 205;
			Send_Group = 1;
			V1.f = (float)BattVol/1000;
			Send_Val1 = V1.h;
			V1.f = (float)((float)1640-saradc_tempe_data)/4;	// 紀錄轉換溫度值
			Send_Val2 = V1.h;
			Send_Val42 = Batt_FULL;
			SendUpComp(SendBuff, 32);
		}
//	if(DisplayCont || !gpio_input(DcInDet)){
//		if(DisplayCont) USER_DBG_INFO("==== BATT ADC:%d  %d mv  DcIn:%d  Batt_FULL:%d\n",SarAdcVal[6],BattVol,gpio_input(DcInDet),Batt_FULL);
////		if(DisplayCont)	DisplayCont--;      //充电的时候   充电灯显示时间结束就关掉，操作机器时候就会亮充电灯
//		if(ChargFlash==0){
//		#ifdef ChargeFullDis
//			if(BattVol>=fullVn)	Batt_FULL = 1;	// 8.375V 滿電
//				else Batt_FULL =0;
//		#else
//			Batt_FULL = !gpio_input(ChargFull);
//		#endif
//			if(BattVol>=fullVn && P4Voln){		// 8V & 滿電
//				BattVn=0x0F;
//			}else if(BattVol>=P3Voln){	// 7.5	11.25
//				BattVn=0x07;
//			}else if(BattVol>=P2Voln){	// 7	10.5
//				BattVn=0x03;
//	//		}else if(SarAdcVal[1]>=2740){
//	//			BattVn=0x01;
//			}else{
//				BattVn=1;
//			}
//			ChargFlash = 1;
//		}else if(ChargFlash && Batt_FULL==0 && !gpio_input(DcInDet)){
//			if(((BattVn>>1)&1)==0){
//				BattVn=0x03;
//			}else if(((BattVn>>2)&1)==0){
//				BattVn=0x07;
//			}else if(((BattVn>>3)&1)==0){
//				BattVn=0x0F;
//				ChargFlash = 0;
//			}else{
//				ChargFlash = 0;
//			}
//		}else{
//			ChargFlash = 0;
//		}
////		USER_DBG_INFO("==== BattVn:%02X  SarAdcVal[1]:%d\n",BattVn,SarAdcVal[1]);
//		if(DisplayCont || Batt_FULL==0){	// 顯示 或 未滿電
//			gpio_output(LED0_Bat0, (BattVn&1));
//			gpio_output(LED1_Bat1, ((BattVn>>1)&1));
//			gpio_output(LED2_Bat2, ((BattVn>>2)&1));
//			gpio_output(LED3_Bat3, ((BattVn>>3)&1));
//		}else if(!gpio_input(DcInDet) && Batt_FULL){
//			gpio_output(LED0_Bat0, 1);
//			gpio_output(LED1_Bat1, 1);
//			gpio_output(LED2_Bat2, 1);
//			gpio_output(LED3_Bat3, 1);
//		}else{
//			gpio_output(LED0_Bat0, 0);
//			gpio_output(LED1_Bat1, 0);
//			gpio_output(LED2_Bat2, 0);
//			gpio_output(LED3_Bat3, 0);
//		}
//	}else{
//
//		if(BattVol<LoPowerENv){	// 6.5v	9.75
//			gpio_output(LED0_Bat0, LowPowerFlash^=1);
//		}else{
//			gpio_output(LED0_Bat0, 0);
//			gpio_output(LED1_Bat1, 0);
//			gpio_output(LED2_Bat2, 0);
//			gpio_output(LED3_Bat3, 0);
//		}
//	}
		///app_handle_t app_h = app_get_sys_handler();
		if(BattVol<P2Voln){	// 6.5v	9.75
			bat_flag=1;//gpio_output(LED2_AUX, LowPowerFlash^=1);
//			SysInf.NC1=1;
		}
		/*else if(BattVol<P2Voln&&bat_flag!=2){	// 6.5v	9.75
			bat_flag=2;//gpio_output(LED2_AUX, LowPowerFlash^=1);
		}*/
		else if(BattVol<P3Voln){	// 6.5v	9.75
			bat_flag=2;//gpio_output(LED2_AUX, LowPowerFlash^=1);
//			SysInf.NC1=2;
		}
		else if(BattVol<P4Voln){	// 6.5v	9.75
			bat_flag=3;//gpio_output(LED2_AUX, LowPowerFlash^=1);
//			SysInf.NC1=3;
		}
		else if(BattVol<fullVn){	// 6.5v	9.75
			bat_flag=4;//gpio_output(LED2_AUX, LowPowerFlash^=1);
//			SysInf.NC1=4;
		}
		if(BattVol>fullVn)
		{
			bat_flag=5;
//			SysInf.NC1=5;
		}

		//ChargFlash=1;
		if(ChargFlash)
		{
			switch(bat_flag)
			{
			case 1:
				if(bat_index_1==0)
				{
					bat_index_1=1;
				}
				else
				{
					bat_index_1=0;
				}
				bat_index_2=1;
				bat_index_3=1;
				bat_index_4=1;
				break;
			case 2:
				bat_index_1=0;
				if(bat_index_2==0)
				{
					bat_index_2=1;
				}
				else
				{
					bat_index_2=0;
				}
				bat_index_3=1;
				bat_index_4=1;
				break;
			case 3:
				bat_index_1=0;
				bat_index_2=0;
				if(bat_index_3==0)
				{
					bat_index_3=1;
				}
				else
				{
					bat_index_3=0;
				}
				bat_index_4=1;
				break;
			case 4:
				bat_index_1=0;
				bat_index_2=0;
				bat_index_3=0;
				if(bat_index_4==0)
				{
					bat_index_4=1;
				}
				else
				{
					bat_index_4=0;
				}
				break;
			case 5:
				bat_index_1=0;
				bat_index_2=0;
				bat_index_3=0;
				bat_index_4=0;
				break;
			   default:
				   bat_index_1=0;
				   bat_index_2=0;
				   bat_index_3=0;
				   bat_index_4=0;
				break;
			}
		}
		else
		{
			switch(bat_flag)
			{
			case 1:
				bat_index_1=0;
				bat_index_2=1;
				bat_index_3=1;
				bat_index_4=1;
				break;
			case 2:
				bat_index_1=0;
				bat_index_2=0;
				bat_index_3=1;
				bat_index_4=1;
				break;
			case 3:
				bat_index_1=0;
				bat_index_2=0;
				bat_index_3=0;
				bat_index_4=1;
				break;
			case 4:
				bat_index_1=0;
				bat_index_2=0;
				bat_index_3=0;
				bat_index_4=0;
				break;
			case 5:
				bat_index_1=0;
				bat_index_2=0;
				bat_index_3=0;
				bat_index_4=0;
				break;
			   default:
				   bat_index_1=0;
				   bat_index_2=0;
				   bat_index_3=0;
				   bat_index_4=0;
				break;
			}
		}
		//USER_DBG_INFO("==== LowPower Mode ADC:%d  %d mv\n",bat_index_2,BattVol);
		//==== 判斷進入 LowPower Mode =======================
		if(LowPowerEn==0 && BattVol<LoPowerENv){		// 6.5v		9.75
			USER_DBG_INFO("==== LowPowerCont: %d\n",LowPowerCont);
			if(++LowPowerCont > 5){
				LowPowerEn=1;
				DelayOffCont=300;
				OffPowerCont = 0;
				USER_DBG_INFO("==== LowPower Mode ADC:%d  %d mv\n",SarAdcVal[2],BattVol);
			}
		}else{
			LowPowerCont=0;
			//USER_DBG_INFO("==== UnLowPower Mode ADC:%d  %d mv\n",ChargFlash,BattVol);
			if(LowPowerEn && BattVol>LoPowerENv){	// 低電解除
				LowPowerEn=0;
				USER_DBG_INFO("==== UnLowPower Mode ADC:%d  %d mv\n",SarAdcVal[2],BattVol);
			}
		}

		//===============================================================
		if(LowPowerEn){	// 低電
			if(SysInf.MusicVol > 24){
				SysInf.MusicVol = 24;
				DelayOffCont = 300;	// 立即播報一次低電提示音
			}
			if(++DelayOffCont > 300){	// 5分鐘播報一次低電提示音
				DelayOffCont=0;
				low_index++;
				DisplayCont = DisplayCont_def;	// 計時顯示
				if(low_index<4)
				{
					if(SysInf.Lang)	{
						//app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
						//app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
					}else{
						//app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
						//app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
						app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
					}
				}
				else
				{
					if(SysInf.Lang)	app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);    //关机E
											else		app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
				}
			}
			if(BattVol<LoPowerOFFv){	// 低電關機
				USER_DBG_INFO("==== LowBatt ADC:%d  %1.2fv  %d\n",SarAdcVal[2],BattVol, OffPowerCont);
				if(++OffPowerCont==6&&low_index<4){	// 低電關機計時 播報一次低電提示音
					low_index++;
					if(SysInf.Lang)	{
						//app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
						//app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
						app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED9);
					}else{
						//app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
						//app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
						app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);

					}
				}
				else if(OffPowerCont==9)
				{	// 低電關機計時 關機
					if(SysInf.Lang)	app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);    //关机E
						else		app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
				}
				else if(low_index==4)
				{
					if(SysInf.Lang)	app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);    //关机E
											else		app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);    //关机C
				}
			}else{
				OffPowerCont = 0;
			}
		}else{
			DelayOffCont=0;
			low_index=0;
		}
	}
}
#endif


