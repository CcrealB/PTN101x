/****************************************************************************



*****************************************************************************/
	#define	YPT_Q66	// 方案名稱

//	#define APPLE_IOS_VOL_SYNC_ALWAYS   // （按键）for iphone volume sync
	#define ChargeFullDis	// 無充滿電偵測腳
	#define	BattHR	1600
	#define	BattLR	240
	//灯光
	#define CONFIG_AUD_REC_AMP_DET
//	#define	CONFIG_AUD_AMP_DET_FFT
	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
//	#define SDA2  	GPIO14

	#define ACM86xxId1	0x2C		// #include "ACM86xx.h"

//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	//==== function set =========================================================
	#define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USB_INVALID
    #define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBD_CLS_AUDIO_HID) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
//	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
//		#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
	#define SDCARD_DETECT_IO	GPIO13	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
//		#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
//		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	//==== LINE_IN ============
//	#define LINE_EN		//
	#define LINE2_EN	// AUX输入

//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x0F

	#define PromptToneMusicMute
	//-----------------------------------------
	#define GPIO_KEY_NUM	1		//
	#define	KeyGpioVal	{GPIO15}	//設置 GPIO
	//-----------------------------------------   按键
	#define ADC_KEY_NUM		6
	#define SarADC_CH0	SARADC_CH_GPIO20    //
	#define AdcV	3200	// mv	 拉高電壓
	#define VRang	 100	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// k	拉高電阻
	#define	KeyDnRVal	{0,1,2.2,3.6,5.6,6.8}	//設置拉低電阻
	//---------------------------------------------------  旋钮
//	#define	KbonNUM	0

	#define	SarADC_CH1	SARADC_CH_GPIO23  //Music Vol
	#define	SarADC_CH2	SARADC_CH_GPIO32  //BASS Vol
	#define	SarADC_CH3	SARADC_CH_GPIO31  //HIGH Vol
	#define	SarADC_CH4	SARADC_CH_GPIO19  //Rev Vol
	#define	SarADC_CH5	SARADC_CH_GPIO22  //Mic Vol
	#define	SarADC_CH6	SARADC_CH_GPIO11  //BAT DET

	//---- ADC CH GPIO set AdcKey + AdcKbon -------------
	#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1, SarADC_CH2,SarADC_CH3,SarADC_CH4,SarADC_CH5,SarADC_CH6}
	#define SarAdc_NUM	7

	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄

	//==== ui For RGB led diver ===============
//	#define LED_RGB_NUM		48
//	#define LED_RGB_IO    	21	//P21,P22,P23 RGB_LED_DIN
