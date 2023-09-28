/****************************************************************************

*****************************************************************************/
	#define	ZY_BEEMIC2p4	// 方案名稱

	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO30
	#define SDA1  	GPIO11
//	#define SDA2  	GPIO14

	// 8625,8629:(10k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628:(10k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"
	#define	ACM25pId1_CS5038_8p4_18

//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	#define APPLE_IOS_VOL_SYNC_ALWAYS   //燒入時需勾選 \feature\音量同步 選項

	//==== function set =========================================================
    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
	#define USB0_FUNC_SEL       USB_INVALID
    #define USB1_FUNC_SEL       USB_INVALID       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
	#define SDCARD_DETECT_IO	7	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif
	#define LINE_DETECT_IO	GPIO4	// LINE IN

//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	//==== ble ==================
//    #define U_CONFIG_USE_BLE
	//===============================

//	#define CONFIG_WAV_PLAY_IN_INTR	// 提示音在中斷執行

	//=======================================================================
	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		0
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x03

	//========================================
	#define EC11_Encoder
	#define EC11_A		GPIO24
	#define EC11_B		GPIO14
	#define EC11_KEY	GPIO15
	//========================================
	#define	KeyDbg
	#define ADC_KEY_NUM		4		//
	#define SarADC_CH0	SARADC_CH_GPIO32
	#define AdcV	2950	// mv	 拉高電壓
	#define VRang	 150	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 15		// 拉高電阻(K)
	#define	KeyDnRVal	{0.1, 1, 3.6, 5.6}	//設置 拉低電阻(K)

	#define SarADC_CH1	SARADC_CH_GPIO21	// BATT_ADC
	#define	BattHR	1000	// k
	#define	BattLR	750		// k
	//---- ADC KeyCh  & KbonCh --------------------------
	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1}
	#define SarAdc_NUM	2

//==== ui For RGB led diver ===============
	#define LED_RGB_NUM		12

//	#define IO_I2S0_DOUT    GPIO21	//P21,P22,P23 RGB_LED_DIN
	#define LED_RGB_IO    	23	//GPIO21	//P21,P22,P23 RGB_LED_DIN
	//==== for ui efects related to audio amplitude ====
	#define CONFIG_AUD_REC_AMP_DET
	#define	ModeLedAddr	0


	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄
//	#define CONFIG_DBG_LOG_FLASH_OP		// dsp free=0 save LOG
//	#define SYS_TIME_STAMP_LOG
