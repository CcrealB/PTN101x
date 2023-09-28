/****************************************************************************



*****************************************************************************/
	#define	HZ_P1	// 方案名稱

	#define APPLE_IOS_VOL_SYNC_ALWAYS   //燒入時需勾選 \feature\音量同步 選項
//	#define CONFIG_WAV_PLAY_IN_INTR	// 提示音在中斷執行
	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define KT56			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V


	//==== I2C ============
	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
	#define SDA2  	GPIO14


	//==== ACM862X ============
	// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);
//8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17);
	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"

	#define ACM86xxId2	0x2D
	#define Acm86xxId2_8625P_LoPo_384_PBTL//Acm86xxId2_8628S_PTBL
//	#define ACM8625P_LoPo_384_CS5038_1p2_13_20V_PBTL

	//==== function set =========================================================
	#define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
    #define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1
	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBD_CLS_AUDIO_HID) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
	#define SDCARD_DETECT_IO	GPIO13	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
//		#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
//		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif
	//==== LINE_IN ============
	#define LINE_EN		//
//	#define LINE2_EN	// AUX输入
	//==== SPDIF ============
//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14
	//==== I2S ============
	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1   //话筒输入
	#define	I2S2_OUT_En		1   //功放输出
	#define	I2S3_IN_En		1	//同屏 Data IN(预留)
	#define	I2S3_OUT_En		1	//同屏 Data OUT
	#define	ANC_IN_En		0x0F
	#define I2sMclk_En
	#define PromptToneMusicMute
	//-----------------------------------------   GPIO按键
	#define GPIO_KEY_NUM	1
	#define	KeyGpioVal	{GPIO15}	//設置 GPIO
	//-----------------------------------------   ADC按键
	#define ADC_KEY_NUM		7
	#define SarADC_CH0	SARADC_CH_GPIO19
	#define AdcV	2965	// mv	 拉高電壓
	#define VRang	 130	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// k	拉高電阻
	#define	KeyDnRVal	{0,1,2,3,3.9,5.6,6.8}	//設置拉低電阻
	//---------------------------------------------------  4051
//	#define	KbonNUM	0     	//4051个数
//	#define	SarADC_CH0	SARADC_CH_GPIO20
//	#define	KbonGpioVal	{SarADC_CH0}
//	#define	Kbon4051NUM	8   //转换通道个数
//	#define	Kbon4051SwGpio	{GPIO8, GPIO9, GPIO19}   //A,B,C
//	#define KbonNUMBER KbonNUM*Kbon4051NUM


	#define	SarADC_CH1	SARADC_CH_GPIO20  //BAT DET
	//---- ADC CH GPIO set AdcKey + AdcKbon -------------
	#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1}
	#define SarAdc_NUM	2

	//==== BK9532 ============
	#define PAIR_ID		0x00009F00 		//对频ID
	#define WORK_ID		0x00009F01 		//默认ID
	#define PAIR_AFn	2         		//A 配对频点
	#define PAIR_BFn	0         		//B 配对频点
	#define Afreq		642000     		//A 起始频点
	#define Bfreq		657000     		//B 起始频点
	#define freqStep	1000       		//频点间隔
	#define	FreqNum	    15
//	#define PAIR_ID		0x00000510
//	#define PAIR_AFn	0
//	#define PAIR_BFn	20
//	#define WORK_ID		0x00000511
//	#define Afreq		639000
//	#define Bfreq		650000
//	#define freqStep	500
//	#define	FreqNum	22

	//==== Charge ============
	#define	BattHR	150
	#define	BattLR	33
	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄
	#define ChargeFullDis
	//==== IR ============
//	#define IRDA_RX_IO       GPIO2

	//==== ui For RGB led diver ===============
//	#define LED_RGB_NUM		48
//	#define LED_RGB_IO    	21	//P21,P22,P23 RGB_LED_DIN
//	#define	CONFIG_AUD_AMP_DET_FFT
//	#define CONFIG_AUD_REC_AMP_DET      //声音侦测

	//==== PWM ============
//	#define USER_PWM_IO      GPIO33

	//==== EC11 ============
//	#define EC11_Encoder
//	#define EC11_A		GPIO23
//	#define EC11_B		GPIO31

	//==== 录音 ============  按键功能下 调用 user_RecWork()  user_RecPlayWork()
//	#define RECED_EN

	//==== 串口接收 ============
//	#define	USR_UART1_INIT_EN
//	#define USER_UART1_FUNCTION_DIRECT //uart1 recieve isr

	//==== 串口发送 ============
//	#define	UART1_TX_SW
