/*
 * XWX_S6616.h
 *
 *  Created on: 2023年6月17日
 *      Author: 11491
 */

#ifndef APP_XWX_S6616_H_
#define APP_XWX_S6616_H_

/****************************************************************************



*****************************************************************************/
	#define	XWX_S6616	// 方案名稱

//	#define APPLE_IOS_VOL_SYNC_ALWAYS   // for iphone volume sync
	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
	#define SDA2  	GPIO14

	// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
#if 0
	#define ACM86xxId1	0x14	// #include "ACM86xx.h"
	#define Acm86xxId1_8628S_PTBL
#else
	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"
	#define Acm86xxId1_8625P_LoPo_384_PBTL
	#define ACM86xxId2	0x14
	#define ACM86xxId2_28_BTL
#endif
	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
	#define PAIR_ID		0x12345678		//
	#define PAIR_AFn	13
	#define PAIR_BFn	13
	#define PAIR_AFreq	695100
	#define PAIR_BFreq	662100
	#define WORK_ID		0x55555555
	#define Afreq		925000
	#define Bfreq		931500
	#define freqStep	500
	#define	FreqNum		14

//	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V
//	#define TM1668

	//==== function set =========================================================
//	#define CONFIG_USE_BLE	// 燒錄軟件 需打開 BLE,編譯需開啟 BLE
//	#define TEST_USER_BLE_RX
	#define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USBH_CLS_DISK
	#define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
	#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
    #define USB1_FUNC_SEL       USB_INVALID       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBD_CLS_AUDIO_HID) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    //#define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
	#define SDCARD_DETECT_IO	GPIO30	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
	//	#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	//==== LINE_IN ============
	#define LINE_EN		//
	#define I2sMclk_En

	#define RECED_EN

//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1   //WMIC
	#define	I2S2_OUT_En		1   //ACM8628 AMP
	#define	I2S3_IN_En		0   //AUX IN
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x0F

	#define PromptToneMusicMute
	//-----------------------------------------
	#define GPIO_KEY_NUM	0		//
//	#define	KeyGpioVal	{GPIO15}	//設置 GPIO
	//-----------------------------------------   按键
	#define ADC_KEY_NUM		8		//
	#define SarADC_CH0	SARADC_CH_GPIO20 //AD按键
	#define AdcV	3180	// mv	 拉高電壓
	#define VRang	 130	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// 拉高電阻(K)
	#define	KeyDnRVal	{0,1,2,3,3.9,5.6,6.8,9.1}	//設置 拉低電阻(K)
	//---------------------------------------------------  旋钮
	//---------------------------------------------------
	#define	KbonNUM	1
	#define	SarADC_CH1	SARADC_CH_GPIO22
//	#define	SarADC_CH1	SARADC_CH_GPIO21	//BAT DET电源检测
	#define	KbonGpioVal	{SarADC_CH1}
	#define	Kbon4051NUM	8
	#define	Kbon4051SwGpio	{GPIO7,GPIO18,GPIO19}
	#define KbonNUMBER KbonNUM*Kbon4051NUM

	//---- ADC CH GPIO set AdcKey + AdcKbon -------------
	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1}
	#define SarAdc_NUM	2

	//==== ui For RGB led diver ===============
	#define LED_RGB_NUM		26
	#define LED_RGB_IO    	21//GPIO23	//P21,P22,P23 RGB_LED_DIN
	#define	CONFIG_AUD_AMP_DET_FFT


#endif /* APP_XWX_S6616_H_ */
