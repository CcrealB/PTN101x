/****************************************************************************



*****************************************************************************/
	#define	PTN1011_DEMO_V2p2	// 方案名稱
//	#define	PTN1012_DEMO_V2p2	// 方案名稱

	#define LinkPass	"000000"	// 六位數連線密碼 0~Z

//	#define APPLE_IOS_VOL_SYNC_ALWAYS   // for iphone volume sync
	#define	USB_OUT_VOL_AT_DSP
	#define USB_IN_VOL_AT_DSP

	//==== include ============================================
#if 1
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"
	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
//	#define OLED128x64
//	#define ACM86xxId1	0x2D	// #include "ACM86xx.h"
//	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
#endif
//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V
#ifdef	ACM86xxId1
	#define CONFIG_USER_GPIO_INT	// 啟用 GPIO 中斷功能
#endif
	//==== function set =========================================================
#if defined(PTN1011_DEMO_V2p2) || defined(PTN1012_DEMO_V2p2)
	//---------------------------
	#define PromptToneMusicMute
//	#define RECED_EN
//	#define I2sMclk_En
//	#define SPDIF_GPIO		14	// GPIO11, GPIO12, GPIO14

#ifdef PTN1012_DEMO_V2p2

    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USB_INVALID
    #define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		//#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	//==== ui For RGB led diver ===============
//	#define LED_RGB_NUM		12

	#define LINE_EN

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	ANC_IN_En		0x0F
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		1
	#define	I2S3_OUT_En		1
#else
	#undef IC_MODEL
	#define IC_MODEL		IC_PTN1011
	#define FLASH_SIZE		FLASH_INFO_8M   // 1=4M;2=8M;3=16M;4=32M

//	#define CONFIG_BT_FUNC_INVALID	// 關閉藍芽功能
//	#define CONFIG_USE_BLE	  // 燒錄軟件 需打開 BLE,編譯需開啟 BLE
//	#define TEST_USER_BLE_RX
//	#define SYS_LOG_PORT_SEL	SYS_LOG_PORT_UART1
//	#define USR_UART1_INIT_EN
//	#define USER_UART1_FUNCTION_DIRECT
	//==== USB =======================================================
    #define USB0_FUNC_SEL	USBD_CLS_AUDIO_HID

	#define	ADC0_IN_En		1
	#define	ANC_IN_En		0
	#define	I2S2_IN_En		0
	#define	I2S2_OUT_En		0
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
#endif

#ifdef	u_key
	//=================================================
	#define GPIO_KEY_NUM	1		// 設定數量後 需在 key.h & udrv_saradc.h 設定相關配置
	#define	KeyGpioVal	{GPIO15}	//設置 GPIO

	//=================================================
	#define ADC_KEY_NUM		5		//
	#ifdef PTN1012_DEMO_V2p2
		#define SarADC_CH0	SARADC_CH_GPIO19
	#else
		#define SarADC_CH0	SARADC_CH_GPIO11
	#endif
	#define AdcV	2900	// mv 拉高電壓
	#define VRang	 100	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// k 拉高電阻
	#define	KeyDnRVal	{0,2.2,4.7,6.8,10}	//設置 拉低電阻
#endif

#ifdef udrv_saradc
	#ifdef PTN1012_DEMO_V2p2
		#if LED_RGB_NUM
			#define SarADC_CH1     SARADC_CH_GPIO20	// LED TX
		#else
			#define SarADC_CH1     SARADC_CH_GPIO21	// Music Vol
		#endif
		#define SarADC_CH2     SARADC_CH_GPIO20	// Mic Vol
		#define SarADC_CH3     SARADC_CH_GPIO23	// REV Vol
		#define SarADC_CH4     SARADC_CH_GPIO22	// Echo Vol
		//---- ADC KeyCh  & KbonCh --------------------------
		#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1, SarADC_CH2, SarADC_CH3, SarADC_CH4}
		#define SarAdc_NUM	5
	#else
		#define SarADC_CH1     SARADC_CH_GPIO31	// Music Vol
		#define SarADC_CH2     SARADC_CH_GPIO32	// Mic Vol
		#define SarADC_CH3     SARADC_CH_GPIO37	// REV Vol & Echo Vol
		//---- ADC KeyCh  & KbonCh --------------------------
		#define	SarAdcChVal	{SarADC_CH0, SarADC_CH1, SarADC_CH2, SarADC_CH3}
		#define SarAdc_NUM	4
	#endif
#endif

////dbg
 //#define CONFIG_DBG_LOG_FLASH_OP
#endif
