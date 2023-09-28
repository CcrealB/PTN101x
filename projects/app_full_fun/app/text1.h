/****************************************************************************



*****************************************************************************/
	#define	text1	// 方案名稱

	#define APPLE_IOS_VOL_SYNC_ALWAYS   // （按键）for iphone volume sync
	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
	#define SDA2  	GPIO14
// 8625,8629:(10k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628:(10k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)

	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"

	#define ACM86xxId2	0x2D
	#define Acm86xxId2_8625P_LoPo_384_PBTL//Acm86xxId2_8628S_PTBL

	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

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
		#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
//		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	//==== LINE_IN ============
	#define LINE_EN		//
//	#define LINE2_EN	// AUX输入
	#define I2sMclk_En

//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		1	//同屏 Data IN
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x0F

	#define PromptToneMusicMute
	//-----------------------------------------
	#define GPIO_KEY_NUM	1		//
	#define	KeyGpioVal	{GPIO15}	//設置 GPIO
	//-----------------------------------------   按键
	#define ADC_KEY_NUM		7
	#define SarADC_CH0	SARADC_CH_GPIO19    //
	#define AdcV	2965	// mv	 拉高電壓
	#define VRang	 130	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// k	拉高電阻
	#define	KeyDnRVal	{0,1,2,3,3.9,5.6,6.8}	//設置拉低電阻
	//---------------------------------------------------  旋钮
//	#define	KbonNUM	0
	#define	SarADC_CH1	SARADC_CH_GPIO20  //BAT DET

	//---- ADC CH GPIO set AdcKey + AdcKbon -------------
	#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1}
	#define SarAdc_NUM	2

	//---- 电池充电 -------------
//	#define	BattHR	300
//	#define	BattLR	75
//	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄
