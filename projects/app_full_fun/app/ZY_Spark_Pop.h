/****************************************************************************



*****************************************************************************/
	#define	ZY_Spark_Pop	// 方案名稱

	#define APPLE_IOS_VOL_SYNC_ALWAYS   // for iphone volume sync

	//==== include ============================================
//	#define	KeyDbg
	#define u_key			// #include "u_key.h"
	#define udrv_saradc		// #include "udrv_saradc.h"

//	#define _Hi2c			// #include "_Hi2c.h"
//	#define SCL1	GPIO16
//	#define SDA1  	GPIO17

//	#define OLED128x64
//	#define ACM86xx			// #include "ACM86xx.h"
//	#define KT56			// #include ".\WMIC_KT\KT_WirelessMicRxdrv.h"
//	#define BK9532			// #include ".\WMIC_BK\BK9532.h"
//	#define QN8035
//	#define drv_spi
//	#define ST7789V

//	#define CONFIG_USER_GPIO_INT	// 啟用 GPIO 中斷功能

	//==== function set =========================================================
	//---------------------------
	#define PromptToneMusicMute

//	#define RECED_EN

	#undef IC_MODEL
	#define IC_MODEL		IC_PTN1011
	#define FLASH_SIZE		FLASH_INFO_16M   // 1=4M;2=8M;3=16M;4=32M

//	#define CONFIG_BT_FUNC_INVALID	// 關閉藍芽功能

//	#define SYS_LOG_PORT_SEL	SYS_LOG_PORT_UART1
//	#define USR_UART1_INIT_EN
//	#define USER_UART1_FUNCTION_DIRECT
	//==== USB =======================================================
    #define USB0_FUNC_SEL	USB_INVALID	//USBD_CLS_AUDIO_HID

	#define	ADC0_IN_En		1
	#define	ANC_IN_En		0
	#define	I2S2_IN_En		0
	#define	I2S2_OUT_En		0
	#define	I2S3_IN_En		0
	#define	I2S3_OUT_En		0

//	#define I2sMclk_En
	//========================================
	#define EC11_Encoder
	#define EC11_A		GPIO13
	#define EC11_B		GPIO14
	#define EC11_KEY	GPIO15

	#define	BattHR	300	// (k) 電池  輸入電阻 BattHR,
	#define	BattLR	47	// (k)		對地電阻 BattLR

#ifdef	u_key
	//=================================================
	#define GPIO_KEY_NUM	0		// 設定數量後 需在 key.h & udrv_saradc.h 設定相關配置
//	#define	KeyGpioVal	{GPIO15}	//設置 GPIO

	//=================================================
	#define ADC_KEY_NUM		4		//
	#define SarADC_CH0	SARADC_CH_GPIO11
	#define AdcV	2960	// mv 拉高電壓
	#define VRang	 100	// 按鍵電壓(mv)上下範圍值
	#define KeyUpR	 33		// k 拉高電阻
	#define	KeyDnRVal	{0,13.4,22.2,39.2}	//設置 拉低電阻 0, 15, 22, 33
#endif
	#define SarADC_CH1	SARADC_CH_VBAT
	#define SarADC_CH2	SARADC_CH_VUSB

#ifdef udrv_saradc
//	#define SarADC_CH1     SARADC_CH_GPIO31	// Music Vol
	//---- ADC KeyCh  & KbonCh --------------------------
	#define	SarAdcChVal	{SarADC_CH0,SarADC_CH1,SarADC_CH2}
	#define SarAdc_NUM	3
#endif

	//==== for ui efects related to audio amplitude ====
	#define CONFIG_AUD_REC_AMP_DET

	#define DEBUG_IC_TEMPERATURE		// 芯片溫度讀取紀錄

