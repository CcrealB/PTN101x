

#ifndef _PRJ_TEST_H_
#define _PRJ_TEST_H_

    #define PTN101x_PRJ_TEST

//// dsp effect
    #define CONFIG_DSP_EFFECT_EN        1

//// redefine ic model to PTN1011
    // #undef IC_MODEL
    // #define IC_MODEL        IC_PTN1011
    #if IC_MODEL == IC_PTN1011
        #define FLASH_SIZE              FLASH_INFO_8M//1011有8Mbit flash的版本，根据实际型号定义
    #endif

////sys
    // #define USR_UART1_INIT_EN //uart1
    // #define USER_UART1_FUNCTION_DIRECT //uart1 recieve isr
    #define CONFIG_CHARGE_INNER     //define to enable user charge only func

////usb
    #if IC_MODEL == IC_PTN1012
        #define UART0_USB0_COEXIST      1 /* UART0和USB0：1=引脚分开  0=引脚复用并联(!!无调试信息) */
        #define USB0_FUNC_SEL       USBD_CLS_AUDIO_HID
        #define USB1_FUNC_SEL       USBH_CLS_DISK       //will undefine if config IC_PTN1011
        #if UART0_USB0_COEXIST == 0
            #define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 //log输出到 UART1
        #endif
    #else
        #define USB0_FUNC_SEL       USB_INVALID //USBD_CLS_AUDIO_HID
        #if USB0_FUNC_SEL != USB_INVALID
            #define SYS_LOG_PORT_SEL    SYS_LOG_PORT_UART1 //log输出到 UART1
        #endif
    #endif

//// udisk
	#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
	    #define CONFIG_UDISK_AUTO_MOD_SW_DIS    //disable auto switch to udisk mode when udisk inserted
		//#define CONFIG_UDISK_AUTO_PLAY_DIS      //disable auto play when switch to udisk mode
	#endif

//// sdcard
    #if IC_MODEL == IC_PTN1012
        #define SDCARD_DETECT_IO	    GPIO35	//gpio detect
        #ifdef	SDCARD_DETECT_IO
            //#define CONFIG_SD_AUTO_MOD_SW_DIS    //disable auto switch to sdcard mode when sdcard inserted
            //#define CONFIG_SD_AUTO_PLAY_DIS      //disable auto play when switch to sd mode
        #endif
    #endif

//// spdif
    #define SPDIF_GPIO              GPIO12//GPIO11, GPIO12, GPIO14

////ble
    // #define CONFIG_USE_BLE
    // #define TEST_USER_BLE_TX
    // #define TEST_USER_BLE_RX

////bt
    // #define CONFIG_BT_FUNC_INVALID

////audio
    #define LineInExchange  1	// Ch:0->Gpio2/3   Ch:1->Gpio4/5
    #define	ADC0_IN_En		1	// 40&48Pin ADC0 0=close, PTN1012 56Pin have no ADC0
    #define	ANC_IN_En		0x0F// PTN1012 56Pin
    #define	I2S2_IN_En		1
    #define	I2S2_OUT_En		1
    #define	I2S3_IN_En		0
    #define	I2S3_OUT_En		0

//// gpio intrrupt
	// #define CONFIG_USER_GPIO_INT	// GPIO interrupt enable

//// gpio key
	#define GPIO_KEY_NUM	1	        // refer to key.h
	#define	KeyGpioVal	    { GPIO15 }	//config GPIO key

//// adc key
	// #define ADC_KEY_NUM		4	// refer to key.h
    #if ADC_KEY_NUM
        // #define ADC_V2D(mv)         ((float)(mv) * 4096 / 1800.0f)  //cvt adc voltage to digital value, adc full [anaVolta:~1800mV]->[digVal:4096]
        // #define keyDigVal(DnR)      ((uint16_t)ADC_V2D(AdcV * DnR / (DnR + UpR)))

        #define SarADC_CH0      SARADC_CH_GPIO23
        #define AdcV            2900.0f // the voltage(unit:mv) pull up
        #define VRang           200     // adc digtal val(0~4095), adc val err range[-VRang ~ +VRang]
        #define KeyUpR          10.0f   // the resistance(unit:kohm) pull up
        #define	KeyDnRVal       {0.0f, 2.2f, 5.1f, 10.0f}	// the resistance(unit:kohm) pull down of Key1~4
        #define	SarAdcChVal     {SarADC_CH0};
        #define SarAdc_NUM      1
    #endif

//// saradc debug(may need to close usb1)
	#define DEBUG_SarADC_NUM        6	// refer to key.h
    #if DEBUG_SarADC_NUM
        #define	SarAdcChVal        {SARADC_CH_GPIO20, \
                                    SARADC_CH_GPIO21, \
                                    SARADC_CH_GPIO22, \
                                    SARADC_CH_GPIO23, \
                                    SARADC_CH_GPIO31, \
                                    SARADC_CH_GPIO32 }
        #define DBG_SarADC_MOD_SOFT 0
        #define SarAdc_NUM          (DBG_SarADC_MOD_SOFT ? 0 : 6)
    #endif

//// spi test
    // #define CONFIG_USER_SPI_FUNC    (1 << 0)//spi:0/1/2, if(1<<3), meas spi2_fun4

//// ir_tx, pwm demo
    // #define IR_TX      //IR Infrared emission，PWM+定时器实现红外发射
    #ifdef IR_TX
        #define IR_TX_PWM_TIMER     GPIO19 //use as timer, change pwm carrir wave in timer isr
        #define IR_TX_PWM_CARRI     GPIO20 //used for genarate 38KHz ir carrier wave
    #endif

////audio detect for LED or LCD ui
    // #define CONFIG_AUD_AMP_DET_FFT  //define/undef to set audio amp detect func, api refer to app_aud_amp_det.h

////RGB_LED
    #define LED_RGB_NUM         0
    #ifdef LED_RGB_NUM
        #define LED_RGB_IO      GPIO21
    #endif

//// debug
    // #define DEBUG_IC_TEMPERATURE
    // #define FFT_RUN_TEST

#endif /* _PRJ_TEST_H_ */

//EOF
