/****************************************************************************
	ZY003CG	// OTG VER Chg Gpio
	原来                                						现在
	PIN51(P24)	MICPOWER(麦克风电源开关）  	P3	   	LED2  (电量指示灯3)
	PIN6(P22) 	HV_ON(升压开关）   			P35		SD_DET(TF卡插入检测）
	PIN12(P2)  	LED1（电量指示灯2）   			P7		AUX_DET（音频插入检测）
	PIN13(P3)   LED2  (电量指示灯3) 			P18	  	USB_DET（充电插入检测）
	PIN22(P18)	USB_DET（充电插入检测） 		P2	   	LED1（电量指示灯2）
	PIN21(P7)	AUX_DET（音频插入检测）  		P24	   	MICPOWER(麦克风电源开关）
	PIN26(P35)	SD_DET(TF卡插入检测） 		P22	   	HV_ON(升压开关）			ok
*****************************************************************************/
	#define	ZY_OA_001	// 方案名稱
//	#define ZY001	//青鸞一代調音
	#define ZY003	//
	#define ZY003Mini
//	#define ZY004	// 2.1CH
#ifdef	ZY003
	#define ZY003CG	// OTG VER Chg Gpio
#endif
#if defined(ZY003) || defined(ZY004)
#else
	#define ChargeFullGpio	GPIO31	// 充滿電偵測腳
#endif

	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
	#define SDA2  	GPIO14

	// 8625,8629:(10k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628:(10k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
#ifdef ZY004
	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"
//	#define ACM25pId1_CS5038_12_20
	#define ACM86xxId2	0x2D
	#define ACM8625P_LoPo_384_CS5038_1p2_13_20V_PBTL
#else
	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"
#ifdef ZY003Mini
	#define ACM25pId1_CS5038_8p4_18
	#define Acm86xxId1_8625P_LoPo_384_PBTL
#else
	#define	ACM25pId1_CS5038_8p4_18
#endif
#endif
	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	#define APPLE_IOS_VOL_SYNC_ALWAYS   //燒入時需勾選 \feature\音量同步 選項


	//==== USB =======================================================
#if defined(ZY002) || defined(ZY003) || defined(ZY004)
	//==== function set =========================================================
    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	#if (UART0_USB0_COEXIST==0)
		#define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
	#else
		#define USB0_FUNC_SEL       USB_INVALID
	#endif
#else
	#define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	#define USB0_FUNC_SEL       USB_INVALID
#endif
    #define USB1_FUNC_SEL       USB_INVALID       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
#ifdef	ZY003CG
	#define SDCARD_DETECT_IO	GPIO22	//gpio detect level
#else
	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
#endif
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		//#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif
#if defined(ZY003CG)
	#define LINE_DETECT_IO	GPIO24	// LINE IN
#elif defined(ZY003) || defined(ZY004)
	#define LINE_DETECT_IO	GPIO7	// LINE IN
#else
	#define LINE_DETECT_IO	GPIO6	// LINE IN
#endif

//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	//==== ble ==================
//    #define U_CONFIG_USE_BLE
	//===============================

//	#define CONFIG_WAV_PLAY_IN_INTR	// 提示音在中斷執行

	//=======================================================================
	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		1
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x03

	//========================================
	#define EC11_Encoder
	#define EC11_A		GPIO33
	#define EC11_B		GPIO12
	#define EC11_KEY	GPIO15
	//========================================
	#define	KeyDbg
	#define ADC_KEY_NUM		4		//
	#define SarADC_CH0	SARADC_CH_GPIO19
	#define AdcV	2950	// mv	 拉高電壓
	#define VRang	 150	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 15		// 拉高電阻(K)
	#define	KeyDnRVal	{0.1, 1, 3.6, 5.6}	//設置 拉低電阻(K)

	#define SarADC_CH1	SARADC_CH_GPIO23	// BATT_ADC
#ifdef	ZY004
	#define	BattHR	300	// (k) 電池  輸入電阻 BattHR,
	#define	BattLR	47	// (k)		對地電阻 BattLR
#else
	#define	BattHR	200
	#define	BattLR	47
#endif
	//---- ADC KeyCh  & KbonCh --------------------------
	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1}
	#define SarAdc_NUM	2

	//==== ui For RGB led diver ===============
	#define	CONFIG_AUD_AMP_DET_FFT
#ifdef ZY004
	#define LED_RGB_NUM		25
	//==== for ui efects related to audio amplitude ====
//	#define	CONFIG_AUD_AMP_DET_FFT
	#define	RGB_EF_EN
	#define	ModeLedAddr	24
#else
	#define LED_RGB_NUM		12
	#define	ModeLedAddr	0
#endif
//	#define IO_I2S0_DOUT    GPIO21	//P21,P22,P23 RGB_LED_DIN
	#define LED_RGB_IO    	21	//GPIO21	//P21,P22,P23 RGB_LED_DIN
	//==== for ui efects related to audio amplitude ====
	#define CONFIG_AUD_REC_AMP_DET

	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄
//	#define CONFIG_DBG_LOG_FLASH_OP		// dsp free=0 save LOG
//	#define SYS_TIME_STAMP_LOG
