/****************************************************************************



*****************************************************************************/
	#define	SY_K59	// 方案名稱

	//==== include ============================================
	#define u_key			// #include "u_key.h"
//	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17

//	#define ACM86xx			// #include "ACM86xx.h"
//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	//==== function set =========================================================
    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USB_INVALID	//USBD_CLS_AUDIO_HID
    #define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
	#define SDCARD_DETECT_IO	GPIO13	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		//#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	#define LINE_EN		// AUX
//	#define LINE2_EN	// FM RADIO
//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		0
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x0F

	#define PromptToneMusicMute

	//-----------------------------------------
	#define GPIO_KEY_NUM	0		//
	//-----------------------------------------
	#define ADC_KEY_NUM		8		//
	#define SarADC_CH0	SARADC_CH_GPIO19
	#define AdcV	2870	// mv	 拉高電壓
	#define VRang	 120	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// k	拉高電阻
	#define	KeyDnRVal	{0,1,2,3,3.9,5.6,6.8,9.1}	//設置拉低電阻
	//-----------------------------------------
	#define ADC_KEY2_NUM	8		//
	#define SarADC_CH1	SARADC_CH_GPIO20
	#define AdcV2	2690	// mv	 拉高電壓
	#define VRang2	 120	// 按鍵電壓(mv)上下範圍值
	#define Key2UpR	 10		// k	拉高電阻
	#define	Key2DnRVal	{0,1,2,3,3.9,5.6,6.8,9.1}	//設置拉低電阻
	//-----------------------------------------
	#define ADC_KEY3_NUM	2		//
	#define SarADC_CH2	SARADC_CH_GPIO21
	#define AdcV3	2870	// mv	 拉高電壓
	#define VRang3	 120	// 按鍵電壓(mv)上下範圍值
	#define Key3UpR	 10		// k	拉高電阻
	#define	Key3DnRVal	{0,1}	//設置拉低電阻

	//---------------------------------------------------
	#define	KbonNUM	2
	#define	SarADC_CH3	SARADC_CH_GPIO23
	#define	SarADC_CH4	SARADC_CH_GPIO22
	#define	KbonGpioVal	{SarADC_CH2, SarADC_CH3}

	#define	Kbon4052NUM	4
	#define	Kbon4052SwGpio	{GPIO16, GPIO17}
	#define KbonNUMBER KbonNUM*Kbon4052NUM

//	#define	Kbon4051NUM	8
//	#define	Kbon4051SwGpio	{GPIO33, GPIO32, GPIO31}
//	#define KbonNUMBER KbonNUM*Kbon4051NUM

	//---- ADC CH GPIO set AdcKey + AdcKbon -------------
	#define	SarADC_CH5	SARADC_CH_GPIO31	// MAIN VOL
	#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1, SarADC_CH2, SarADC_CH3, SarADC_CH4, SarADC_CH5}
	#define SarAdc_NUM	6
