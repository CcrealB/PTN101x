/****************************************************************************



*****************************************************************************/
	#define	SK_460	// 方案名稱

	//==== include ============================================
	#define u_key			// #include "u_key.h"
//	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17

	#define TAS5760M		//
//	#define ACM86xx			// #include "ACM86xx.h"
//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	//==== function set =========================================================
   #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
    #define USB1_FUNC_SEL       USB_INVALID        //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
//	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		//#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

//	#define LINE_EN		// AUX
//	#define LINE2_EN	// FM RADIO

	#define LineInExchange  0	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		1
	#define	ANC_IN_En		0x0F

	#define I2sMclk_En

	#define PromptToneMusicMute

	//music sdcard/udisk/spdif
    #define SPDIF_GPIO		GPIO12	// GPIO11, GPIO12, GPIO14

	//-----------------------------------------
	#define GPIO_KEY_NUM	0		//

#ifdef udrv_saradc
	//-----------------------------------------
	#define ADC_KEY_NUM		5		//
	#define SarADC_CH0	SARADC_CH_GPIO23
	#define AdcV	2970	// mv	 拉高電壓
	#define VRang	 100	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// k	拉高電阻
	#define	KeyDnRVal	{0,2.2,4.7,6.8,10}	//設置 拉低電阻
	//---------------------------------------------------
	#define	KbonNUM	1
	#define	SarADC_CH1	SARADC_CH_GPIO22
	#define	KbonGpioVal	{SarADC_CH1}
	#define	Kbon4051NUM	8
	#define	Kbon4051SwGpio	{GPIO10, GPIO23, GPIO32}
	#define KbonNUMBER KbonNUM*Kbon4051NUM

	//---- ADC CH GPIO set AdcKey + AdcKbon -------------
	#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1}
	#define SarAdc_NUM	2
#endif
