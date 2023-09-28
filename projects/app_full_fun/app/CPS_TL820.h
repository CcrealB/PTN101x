/****************************************************************************



*****************************************************************************/
	#define	CPS_TL820	// 方案名稱

//	#define	ES260	// 效果器
//	#define	HS2450	// 效果器+功放
//	#define	TL820	// 方形木音箱
//	#define	TL8025	// ACM8629 DAMP 圓筒小音箱
//	#define	TL8025v2// ACM8629 DAMP 圓筒小音箱
	#define	TL8035	// ACM8629x2 PTBL DAMP 圓筒大音箱

	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
#if defined(TL8025)||defined(TL8025v2)
	// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
	#define ACM86xxId1	0x14	// #include "ACM86xx.h"
	#define Acm86xxId1_8628S_TBL
#endif
#if defined(TL8035)
	// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
	#define ACM86xxId1	0x14	// #include "ACM86xx.h"
	#define Acm86xxId1_8628S_PTBL
	#define ACM86xxId2	0x15	// #include "ACM86xx.h"
	#define Acm86xxId2_8628S_PTBL
#endif
//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
//	#define QN8035
	#define drv_spi
	#define ST7789V
#if defined(ES260) || defined(HS2450)
	#define PCM1865
#endif

#if 0 //defined(ES260)
	#define AMG8802
	#define AMG_DBG_INFO(fmt,...)       os_printf("[AMG]"fmt, ##__VA_ARGS__)
#endif

	#define APPLE_IOS_VOL_SYNC_ALWAYS   // for iphone volume sync	// 燒入時需勾選 \feature\音量同步 選項
	#define MIC_STEP32					// 麥克風 32 段步進調音
	//==== function set =========================================================
//	#define CONFIG_USE_BLE	// 燒錄軟件 需打開 BLE,編譯需開啟 BLE
//	#define TEST_USER_BLE_RX

    #define UART0_USB0_COEXIST      0 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
#if (UART0_USB0_COEXIST ==1)
	#define USB0_FUNC_SEL       USB_INVALID
#else
	#define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
#endif
	#define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	//	#define CONFIG_UDISK_AUTO_MOD_SW_DIS   	//disable auto switch to udisk mode when udisk in first 5000ms inserted
		#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
//	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

//#define TL820_V02

//	#define LineInExchange  0	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		1
#if defined(ES260)
	#define	I2S3_IN_En		1
	#define	I2S3_OUT_En		1
#elif defined(HS2450)
	#define	I2S3_IN_En		1
	#define	I2S3_OUT_En		0
#else
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
#endif
	#define	ANC_IN_En		0x0C

#if defined(HS2450)
	#define LINE_EN		// LINE IN
#endif

	#define I2sMclk_En
	#define CONFIG_USER_GPIO_INT	// 啟用 GPIO 中斷功能

//	#define RECED_EN

    #define IR_TX      //IR Infrared emission，PWM+定时器实现红外发射
    #ifdef IR_TX
//      #define USE_PWM_GRP2_CH1        //PWM7(GPIO19), used for 500us timer
//      #define USE_PWM_GRP2_CH2        //PWM8(GPIO20), used for genarate 38KHz ir carrier
        #define IR_TX_PWM_TIMER     GPIO19 //use as timer, change pwm carrir wave in timer isr
        #define IR_TX_PWM_CARRI     GPIO20 //used for genarate 38KHz ir carrier wave
    #endif
	//========================================
	#define EC11_Encoder
	#define EC11_A		GPIO9
	#define EC11_B		GPIO8
	#define EC11_KEY	GPIO15
#if defined(ES260) || defined(HS2450)
	#define EC11_Encoder2
	#define EC11_A2		GPIO1
	#define EC11_B2		GPIO0
	#define EC11_KEY2	GPIO14
#endif

//	#define	KeyDbg
	#define GPIO_KEY_NUM	0		//
//	#define	KeyGpioVal	{GPIO15}	//設置 GPIO

	#define ADC_KEY_NUM		4		//
	#define SarADC_CH0	SARADC_CH_GPIO21
#if defined(TL8025)||defined(TL8025v2)
	#define AdcV	2860	// mv	 拉高電壓	TL8025
#else
	#define AdcV	2960	// mv	 拉高電壓 TL820
#endif
	#define VRang	 150	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// 拉高電阻(K)
	#define	KeyDnRVal	{0, 2.2, 5.1, 10}	//設置 拉低電阻(K)

	#define SarADC_CH1	SARADC_CH_GPIO19	// 電池電量檢測口
#ifdef	TL8035
	#define	BattHR	820	// (k) 電池  輸入電阻 BattHR,
	#define	BattLR	62	// (k)	對地電阻 BattLR
#else
	#define	BattHR	270
	#define	BattLR	21
#endif

	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1}
	#define SarAdc_NUM	2

	//music sdcard/udisk/spdif
    #define SPDIF_GPIO              GPIO12	// GPIO11, GPIO12, GPIO14

	#define CONFIG_USER_SPI_FUNC    (1 << 0)	//spi:0/1/2, if(1<<3), meas spi2_fun4
	#define LCD_ST7789_EN

	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄

	//==== ui For RGB led diver ===============
#if defined(TL8035) || defined(TL8025v2)
	#define LED_RGB_NUM		12
	//==== for ui efects related to audio amplitude ====
	#define	CONFIG_AUD_AMP_DET_FFT
//	#define IO_I2S0_DOUT    GPIO21	//P21,P22,P23 RGB_LED_DIN
	#define LED_RGB_IO    	22	//GPIO21	//P21,P22,P23 RGB_LED_DIN
#endif
