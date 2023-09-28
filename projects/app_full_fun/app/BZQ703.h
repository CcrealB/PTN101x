/****************************************************************************



*****************************************************************************/
	#define	BZQ703	// 方案名稱

	#define APPLE_IOS_VOL_SYNC_ALWAYS   // for iphone volume sync

	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17
	#define SDA2  	GPIO14

	// 8625:(4.7k=0x2C)(15k=0x2D)(47k=0x2E)(120k=0x2F);	8628,8629:(4.7k=0x14)(15k=0x15)(47k=0x16)(120k=0x17)
	#define ACM86xxId1	0x14	// #include "ACM86xx.h"
	#define Acm86xxId1_8628S_PTBL


	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

	//==== function set =========================================================
    #define UART0_USB0_COEXIST      1 // UART0和USB0：1=引腳不並聯  0=並聯(!!無調試信息)
	//==== USB =======================================================
    #define USB0_FUNC_SEL       USB_INVALID
    #define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config PTN1011
	//==== UDisk ============
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif
    #if IC_MODEL == IC_PTN1011 //如果是1011， log打印切到 UART1
        #if (USB0_FUNC_SEL != USB_INVALID)
            #define CONFIG_LOG_TO_UART1  //define to seletct log port to uart1, for ptn1011 USB & UART0 is mutex
        #endif
    #endif

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	ANC_IN_En		0x0F
	#define	I2S2_IN_En		1	// BK9532 WMIC
	#define	I2S2_OUT_En		1	// ACM8628 AMP
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0

	#define LINE_DETECT_IO	GPIO13	// LINE IN DET
	#define LINE2_EN

//    #define SPDIF_GPIO              GPIO12	// GPIO11, GPIO12, GPIO14

	#define I2sMclk_En
//	#define CONFIG_USER_GPIO_INT	// 啟用 GPIO 中斷功能

	#define GPIO_KEY_NUM	1		// 設定數量後 需在 key.h & udrv_saradc.h 設定相關配置
	#define	KeyGpioVal	{GPIO15}	//設置 GPIO

	#define ADC_KEY_NUM		4		//
	#define SarADC_CH0	SARADC_CH_GPIO21
	#define AdcV	2860	// mv	 拉高電壓
	#define VRang	 120	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// 拉高電阻(K)
	#define	KeyDnRVal	{0,2,3.9,5.6}	//設置 拉低電阻(K)

	#define SarADC_CH1	SARADC_CH_GPIO20	// BATT_ADC
	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1}
	#define SarAdc_NUM	2

