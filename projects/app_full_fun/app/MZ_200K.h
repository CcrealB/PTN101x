/****************************************************************************



*****************************************************************************/
	#define	MZ_200K	// 方案名稱

	#define	BattHR	300
	#define	BattLR	75
	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄
	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
//	#define SDA2  	GPIO14

	// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
	#define ACM86xxId1	0x2C	// #include "ACM86xx.h"
	#define ACM8625P_LoPo_384_CS5038_1p2_13_20V_PBTL	//Acm86xxId1_8625_TBL

//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
	#define KT56			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	#define APPLE_IOS_VOL_SYNC_ALWAYS   //燒入時需勾選 \feature\音量同步 選項

	//==== function set =========================================================
    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
	#define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
    #define USB1_FUNC_SEL       USB_INVALID       //will undefine if config PTN1011
// 	#define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 /log输出到UART1

	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
	//==== sdcard ============
//	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	#define LINE_DETECT_IO	GPIO4	// LINE IN
//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	//==== ble ==================
//    #define U_CONFIG_USE_BLE
	//===============================

//	#define CONFIG_WAV_PLAY_IN_INTR	// 提示音在中斷執行

	//=======================================================================
	#define LineInExchange  0	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	I2S2_IN_En		0
	#define	I2S2_OUT_En		1
	#define	I2S3_IN_En		1
	#define	I2S3_OUT_En		0
	#define	ANC_IN_En		0x0F

	//=================================================
	#define	KeyDbg	// 打印 Key 信息
	#define GPIO_KEY_NUM	1		// 設定數量後 需在 key.h & udrv_saradc.h 設定相關配置
	#define	KeyGpioVal	{GPIO15}	//設置 GPIO
	//========================================
	#define ADC_KEY_NUM		5		//
	#define SarADC_CH0	SARADC_CH_GPIO20
	#define AdcV	2830	// mv	 拉高電壓
	#define VRang	 100	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 22		// 拉高電阻(K)
	#define	KeyDnRVal	{0, 5.1, 10, 15, 22}	//設置 拉低電阻(K)

	#define SarADC_CH1	SARADC_CH_GPIO21	// BATT_ADC
	//---- ADC KeyCh  & KbonCh --------------------------
	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1}
	#define SarAdc_NUM	2

	//==== ui For RGB led diver ===============
	#define LED_RGB_NUM		20
	#define LED_RGB_IO    	GPIO23	//GPIO23	//P21,P22,P23 RGB_LED_DIN
	#define	CONFIG_AUD_AMP_DET_FFT

	//==== for ui efects related to audio amplitude ====
	#define CONFIG_AUD_REC_AMP_DET
