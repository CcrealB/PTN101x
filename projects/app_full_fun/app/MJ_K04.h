/****************************************************************************



*****************************************************************************/
	#define	MJ_K04	// 方案名稱

	//==== include ============================================
//	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

//	#define _Hi2c			// #include "_Hi2c.h"
//	#define SCL1	GPIO16
//	#define SDA1  	GPIO17

//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
//	#define QN8035

//	#define APPLE_IOS_VOL_SYNC_ALWAYS   // for iphone volume sync	// 燒入時需勾選 \feature\音量同步 選項
//	#define MIC_STEP32					// 麥克風 32 段步進調音
	//==== function set =========================================================
//	#define CONFIG_USE_BLE	// 燒錄軟件 需打開 BLE,編譯需開啟 BLE
//	#define TEST_USER_BLE_RX

    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
	#define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
//	#define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	#define	USR_UART1_INIT_EN
	#define USER_UART1_FUNCTION_DIRECT //uart1 recieve isr

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	// 	#define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
//	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	//==== LINE_IN ============
//	#define LINE_EN
	#define LINE2_EN
//	#define I2sMclk_En

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		0
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		1
	#define	ANC_IN_En		0x0C

	#define I2sMclk_En
//	#define CONFIG_USER_GPIO_INT	// 啟用 GPIO 中斷功能

//	#define RECED_EN

//    #define IR_TX      //IR Infrared emission，PWM+定时器实现红外发射
    #ifdef IR_TX
//      #define USE_PWM_GRP2_CH1        //PWM7(GPIO19), used for 500us timer
//      #define USE_PWM_GRP2_CH2        //PWM8(GPIO20), used for genarate 38KHz ir carrier
        #define IR_TX_PWM_TIMER     GPIO19 //use as timer, change pwm carrir wave in timer isr
        #define IR_TX_PWM_CARRI     GPIO20 //used for genarate 38KHz ir carrier wave
    #endif

//	#define	KeyDbg
	#define GPIO_KEY_NUM	0		//
//	#define	KeyGpioVal	{GPIO15}	//設置 GPIO

	#define ADC_KEY_NUM		0		//
#if (ADC_KEY_NUM)
	#define SarADC_CH1	SARADC_CH_GPIO23

	#define AdcV	2860	// mv	 拉高電壓	TL8025
	#define VRang	 150	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// 拉高電阻(K)
	#define	KeyDnRVal	{0, 2.2, 5.1}	//設置 拉低電阻(K)
#endif

	//---------------------------------------------------
	#define	KbonNUM	1
	#define	SarADC_CH0	SARADC_CH_GPIO22
	#define	KbonGpioVal	{SarADC_CH0}
	#define	Kbon4051NUM	8
	#define	Kbon4051SwGpio	{GPIO8, GPIO9, GPIO10}
	#define KbonNUMBER KbonNUM*Kbon4051NUM

	//----------------------------------------------------------
	#define SarADC_CH1	SARADC_CH_GPIO23    // 低音炮音量
	#define SarADC_CH2	SARADC_CH_GPIO20    // 直播話筒音量
	#define SarADC_CH3	SARADC_CH_GPIO21	// 樂器音量
	#define SarADC_CH4	SARADC_CH_GPIO19	// 電池電量檢測口

	#define	BattHR	270
	#define	BattLR	21

	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1,SarADC_CH2,SarADC_CH3,SarADC_CH4}
	#define SarAdc_NUM	5

	//music sdcard/udisk/spdif
    #define SPDIF_GPIO              GPIO14	// GPIO11, GPIO12, GPIO14

	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄
