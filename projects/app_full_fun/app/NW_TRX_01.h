/****************************************************************************



*****************************************************************************/
	#define	NW_TRX_01	// 方案名稱

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
	#define	NW_TX	//no define = RX

//	#define CONFIG_BT_FUNC_INVALID	// 關閉藍芽功能
    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
    #define USB1_FUNC_SEL       USB_INVALID       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    //#define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
//	#define SDCARD_DETECT_IO	GPIO13	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		//#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	#define LINE_EN		// AUX
//	#define LINE2_EN	// I2S ADC


	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
#ifdef NW_TX
	#define	I2S2_IN_En		1	// for 5.8G TX
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0	// ADC AK5720
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x03
#else
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		0
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x00
#endif

	#define I2sMclk_En

	#define PromptToneMusicMute

	//-----------------------------------------
	#define GPIO_KEY_NUM	3		//
	#define	KeyGpioVal	{GPIO15,GPIO19,GPIO20}	// POWER, V+, V-

