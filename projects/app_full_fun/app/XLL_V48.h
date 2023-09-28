/****************************************************************************



*****************************************************************************/
	#define	XLL_V48	// 方案名稱

	//==== include ============================================
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

	#define _Hi2c			// #include "_Hi2c.h"
	#define SCL1	GPIO16
	#define SDA1  	GPIO17

	#define ACM86xx			// #include "ACM86xx.h"
	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

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
//	#define SDCARD_DETECT_IO	GPIO35	//gpio detect level
	#ifdef	SDCARD_DETECT_IO
		//#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
		//#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
	#endif

	#define CONFIG_BT_FUNC_INVALID	// 關閉藍芽功能

//	#define LINE_EN

//	#define SPDIF_GPIO		GPIO14	// GPIO11, GPIO12, GPIO14

	#define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
	#define	ADC0_IN_En		0	// 40&48Pin ADC0 0=關閉, PTN1012 56Pin 無ADC0
	#define	ANC_IN_En		0x0F
	#define	I2S2_IN_En		1	// 9532 WMIC
	#define	I2S2_OUT_En		1	// DAC to Low Spk
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0

	#define I2sMclk_En
//	#define CONFIG_USER_GPIO_INT	// 啟用 GPIO 中斷功能

	#define GPIO_KEY_NUM	0		//

	#define ADC_KEY_NUM		7		//
	#define SarADC_CH0	SARADC_CH_GPIO19
	#define AdcV	2930	// mv	 拉高電壓
	#define VRang	 100	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 10		// 拉高電阻(K)
	#define	KeyDnRVal	{0,1,2,3,3.9,5.6,6.8}	//設置 拉低電阻(K)
	#define	SarAdcChVal	{SarADC_CH0}
	#define SarAdc_NUM	1

