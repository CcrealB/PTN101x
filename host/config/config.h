/* Jungo Confidential, Copyright (c) 2012 Jungo Ltd.  http://www.jungo.com */

//#include <config_system.h>	//yuan++
//#include "config_debug.h"
//#include "version.h"

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <config_system.h>	//yuan++
#include "config_debug.h"
#include "version.h"

#define USER_KARAOK_MODE            1
#ifdef USER_KARAOK_MODE
    #define CONFIG_SYS_TICK_INT_1MS

    #define AUD_ADDA_KEEP_OPEN          1

    #define APLL_KEEP_98p304MHZ         1 //audio clock keep N*48KHz.
    #define BT_AUD_SYNC_ADJ_BY_SW       2 //audio sync adjust by 0:apll/1:mcu/2:dsp

    #if BT_AUD_SYNC_ADJ_BY_SW == 2
        #define BT_A2DP_SRC248_EN           0 // a2dp audio src to 48KHz
        #define SD_ASO_SRC248_EN            0 // sd play audio src to 48KHz
    #else
        #define BT_A2DP_SRC248_EN           1 // a2dp audio src to 48KHz
        #define SD_ASO_SRC248_EN            1 // sd play audio src to 48KHz
    #endif
    #define AUD_WAV_TONE_SEPARATE       //define to config the prompt wav play separate(wav:16k, sbc:8k/16k)

    #define DMA_ALLOC_BY_DSP

//// bt debug
	// #define BT_AUDIO_SYNC_DEBUG
    // #define DEBUG_BT_DECODE
    #ifdef DEBUG_BT_DECODE
        #define BT_DECODE_TASK_RUN(en)  REG_GPIO_0x20 = en ? 2 : 0;
        #define BT_DECODE_RUN(en)       REG_GPIO_0x1F = en ? 2 : 0;
    #else
        #define BT_DECODE_TASK_RUN(en)
        #define BT_DECODE_RUN(en)
    #endif
#endif

#define CONFIG_ARM_COMPILER                     1
#define CONFIG_BYTE_ORDER                       CPU_LITTLE_ENDIAN
#define CONFIG_PORT                             beken_no_os
#define CONFIG_SINGLE_THREADED                  1
#define CONFIG_NATIVE_UINT64                    1
#define CONFIG_JOS_MBUF                         1
#define CONFIG_JOS_BUS                          1
#define CONFIG_JOS_UTILS                        1
#define CONFIG_JOS_SECURE_PTR                   1
#define CONFIG_COMMON_STR_UTILS                 1
#define CONFIG_MEMPOOL                          1
#define CONFIG_MEMPOOL_SIZE                     (40 * 1024)
#define CONFIG_MEMPOOL_DMABLE                   1
#ifndef CONFIG_BT_FUNC_INVALID
#define CONFIG_BLUETOOTH                        1
#endif
#define CONFIG_BLUETOOTH_HCI_UART               1
#define CONFIG_BLUETOOTH_SDP_SERVER             1
#define CONFIG_BLUETOOTH_A2DP                   1
#define CONFIG_BLUETOOTH_A2DP_SINK              1
#define CONFIG_BLUETOOTH_AVRCP                  1
#define CONFIG_BLUETOOTH_AVRCP_CT               1
#define CONFIG_BLUETOOTH_AVRCP_TG               1
#define CONFIG_BLUETOOTH_AVDTP                  1
#define CONFIG_BLUETOOTH_AVDTP_SCMS_T     		1
#define CONFIG_BLUETOOTH_AVCTP                  1
// #define CONFIG_BLUETOOTH_HFP                    1
// #define CONFIG_BLUETOOTH_SDP_HFP                1
// #define CONFIG_BLUETOOTH_HSP                    1
// #define CONFIG_BLUETOOTH_SDP_HSP                1
#define CONFIG_BLUETOOTH_SCO                    1
#define CONFIG_BLUETOOTH_APP                    1
#define CONFIG_BT_USER_FLAG3                    1

#define CONFIG_BLUETOOTH_AUDIO_APP_STANDALONE   1
#define CONFIG_AUDIO_OUT_INTERFACE              1
#define CONFIG_PKG                              1
#define CONFIG_PKG_SBC                          1
#define CONFIG_FILE_LIST                        file_list_beken
#define UWVER_STR                               "4.0.33.5"
#define CONFIG_APP_ASYNC_DATA_STREAM            0
#define CONFIG_PROMPT_WAVE_AS_ALAW              0
#define CONFIG_BLUETOOTH_SSP
#define CONFIG_BLUETOOTH_HID                    0
#define CONFIG_BLUETOOTH_SPP
#define CONFIG_DRIVER_I2S                       (defined(I2S_DRIVER_CODE_OPEN))
#define CONFIG_DRIVER_DAC
#define CONFIG_DRIVER_ADC
#define CONFIG_CHARGE_EN                        1
#define CONFIG_TEMPERATURE_DETECT               1 //high&low temperature auto cali voltage & crystal frequency
#define CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE      1
#define BUTTON_DETECT_IN_SNIFF                  0
#define CONFIG_UART_IN_SNIFF					0
#define CONFIG_TX_CALIBRATION_EN                1
#define BT_ONE2MULTIPLE_AS_SCATTERNET		    0
#define CONFIG_AUDIO_DAC_ALWAYSOPEN				0
#define CONFIG_EXT_AUDIO_PA_EN   				0
#define CONFIG_EXT_PA_DIFF_EN                   0
#define SYS_CFG_BUCK_ON                         0
/*
#define AUDIO_CTRL_BY_MCU                       0
#define AUDIO_CTRL_BY_DSP                       1
#define CONFIG_AUDIO_CTRL_MODE                  AUDIO_CTRL_BY_MCU    ////  divide into  ADC_CTRL_BY_MCU / DAC_CTRL_BY_MCU
*/
#define ADC_CTRL_BY_MCU                         0
#define ADC_CTRL_BY_DSP                         1
#define CONFIG_ADC_CTRL_MODE                    ADC_CTRL_BY_MCU
#define DAC_CTRL_BY_MCU                         0
#define DAC_CTRL_BY_DSP                         1
#define CONFIG_DAC_CTRL_MODE                    DAC_CTRL_BY_MCU
#ifdef USER_KARAOK_MODE
    #undef CONFIG_ADC_CTRL_MODE
    #undef CONFIG_DAC_CTRL_MODE
    #define CONFIG_ADC_CTRL_MODE                    ADC_CTRL_BY_DSP
    #define CONFIG_DAC_CTRL_MODE                    DAC_CTRL_BY_DSP
    #define SYS_DSP_ENABLE                          1 // 0:disable/1:enable dsp core
#else
    #define SYS_DSP_ENABLE                          0 // 0:disable/1:enable dsp core
#endif

#define CONFIG_ANC_ENABLE                       0
#define CONFIG_AUDIO_SYNC_ENABLE                1

// #define CONFIG_APP_AEC
#define CONFIG_APP_EQUANLIZER
// #define CONFIG_PRE_EQ                           1
// #define CONFIG_HFP_SPK_EQ						1
// #define CONIFG_HFP_MIC_EQ						1
#define CONFIG_APP_MSBC_RESAMPLE                0
#define CONFIG_APP_ADC_VAD                      0
#define CONFIG_APP_DAC_VAD                      0
#define CONFIG_DAC_DELAY_OPERATION              0
#define CONFIG_DAC_CLOSE_IN_IDLE			    1

#define CONFIG_NOUSE_RESAMPLE 					1
#define CONFIG_TMP_ADVOID_GPIO15_REPEAT_RESET   1

#define CONFIG_CPU_CLK_OPTIMIZATION             0
#if CONFIG_CPU_CLK_OPTIMIZATION
#define CONFIG_CPU_CLK_OPTIMIZATION_LVL         2 
#else
#define CONFIG_CPU_CLK_OPTIMIZATION_LVL         0 
#endif 

//#define CONFIG_LINEIN_FUNCTION
#define CONFIG_LINEIN_ATTACH_OFF_POWER              0

/* ************************* audio transfer mode ************************* */
/* ASO_DEF_TRANS_BY_SHARE_RB     define/undefine
- define to enable audio trans by ringbuf, else by mailbox (mcu wait dsp intr)
- mode ringbuf audio stream:
    - BT exp: BT --> ringbuf wr by mcu --> AHBMEM --> ringbuf rd by dsp --> DAC
- mode mailbox audio stream:
    - BT exp: BT --> rb1 wr by mcu --> rb1 rd by dsp intr --> rb2 wr by dsp intr --> DAC
    - for mailbox mode, the rb1 in mcu, rb2 in dsp, move rb1 to rb2 in dsp intr
*/
#define ASO_DEF_TRANS_BY_SHARE_RB


#define ASI_DEF_TRANS_BY_SHARE_RB   //def asi d2m

#define ASO_USB_TRANS_BY_SHARE_RB   //usb out m2d

/** @brief select usb in audio stream use independent frame buffer or not
 * @param 
 *  #define: use independent frame buffer "aud_fra_usb_tx" in dsp side, asrc/spc func is decide by audio_asi_open(ASI_TYPE_USB)
 *  #undefine : share rec audio frame buffer "audio_rec_frame" in dsp side, asrc/spc func not suported
 * @note open define always is recommended.
 * */
#define ASI_USB_TRANS_BY_SHARE_RB   //usb in d2m

/******************** SD CARD FUNCTION  *******************************/
#ifdef SDCARD_DETECT_IO
    #define CONFIG_APP_SDCARD       //if use SDIO BK_XVR_BASEBAND_TXRX_BIT  must be 0
    #define CONFIG_SDCARD_DETECT    //pls config detect gpio in toolkit->system param(系统参数)->sd card(SD卡)
    #define SDCARD_DETECT_LVL	    0//valid detect level
    #define SDIO_CLK_CMD_DAT0_SEL8_9_10     /* def -> select gpio8/9/10/35/34/36.  undef -> select gpio37/38/39/35/34/36 */
    // #define CONFIG_APP_SDCARD_4_LINE     /* def/undef to select 4line sdio or not */

    //----sd test
    // #define SDCARD_FATFS_TEST
    // #define TEST_SDCARD_RW
#endif

/******************** USB FUNCTION	*******************************/
//types of usb class to select(define @ u_config.h)
// #define USB_INVALID             0
// #define USBD_CLS_AUDIO          1
// #define USBD_CLS_HID            2
// #define USBD_CLS_AUDIO_HID      3
// #define USBH_CLS_DISK           4

#if USB0_FUNC_SEL || USB1_FUNC_SEL
    #define CONFIG_USE_USB      1
#endif

#if (USB0_FUNC_SEL == USBD_CLS_AUDIO_HID || USB0_FUNC_SEL == USBD_CLS_AUDIO_HID)

#define USB_AUDIO_FUNC_VALID            1
// #define USB_AUDIO_TASK_SWI           //bug:if defne, then usb in no sound after a few minutes

#define CONFIG_AUD_SYNC_USBO            2 // 0:disable/1:spc by mcu/2:spc by dsp
#define CONFIG_AUD_SYNC_USBI            2 // 0:disable/1:spc by mcu/2:spc by dsp
#if (CONFIG_AUD_SYNC_USBO == 2) && (!defined(ASO_USB_TRANS_BY_SHARE_RB))
    #error "usb audio out config error!"
#endif
#if (CONFIG_AUD_SYNC_USBI == 2) && (!defined(ASI_USB_TRANS_BY_SHARE_RB))
    #error "usb audio in config error!"
#endif
#endif

// UDISK
#if (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK)
    #define CONFIG_APP_UDISK
    #define CONFIG_DRV_USB_HOST
#endif

#if (defined(SDCARD_DETECT_IO) || (USB0_FUNC_SEL == USBH_CLS_DISK) || (USB1_FUNC_SEL ==  USBH_CLS_DISK))
////player
	#define CONFIG_APP_PLAYER    //audio play from sdcard/udisk
    #define CONFIG_APP_MP3PLAYER    1
    #ifndef PLAY_TIME_DISABLE
        #define CALC_PLAY_TIME      1
    #endif
////recorder
	#define CONFIG_APP_RECORDER  2//(1:wav, 2:mp3)audio record to sdcard/udisk
    #ifdef CONFIG_REC_FMT_WAV
        #undef CONFIG_APP_RECORDER
        #define CONFIG_APP_RECORDER     1
    #endif
#else
    #define CONFIG_APP_MP3PLAYER                    0
#endif

/* ******************** SPDIF ******************** */
// #define SPDIF_GPIO      GPIO12 //pls define in user config if needed
#ifdef SPDIF_GPIO
    #define CONFIG_SPDIF_DRV
    // #define CONFIG_HDMI_CEC
    #define CONFIG_APP_SPDIF
    #define CONFIG_AUD_SYNC_SPDIF       2 // 0:disable/1:spc by mcu/2:spc by dsp
    // #define APP_SPDIF_DEBUG
#endif
/*********************PWM***********************************************/
#define CONFIG_DRIVER_PWM                           1
#if CONFIG_DRIVER_PWM
#define CONFIG_PWM_LED                              0//1
#endif
/*********************RGB LED***********************************************/
#if (LED_RGB_NUM > 0)
    #define I2S_DRIVER_CODE_OPEN
#endif
/*********************HW I2C1 Driver***********************************************/
//#define CONFIG_DRIVER_I2C1
/*********************CALI************************************************/
#define CALI_BY_PHONE_BIT                                      (1<<0)
#define CALI_BY_8852_BIT                                       (1<<1)
#define CALI_BY_JZHY_BIT                                       (1<<2)
#define CONFIG_RF_CALI_TYPE                                    (CALI_BY_PHONE_BIT)//(CALI_BY_8852_BIT|CALI_BY_JZHY_BIT)
/*********************SOFT I2C1 Driver***********************************************/
//#define CONFIG_DRIVER_I2C_SOFT
/*************************************************************************/
#define CONFIG_EAR_IN                         0

#ifdef IRDA_RX_IO
	#define CONFIG_APP_IR_RX
	#define CONFIG_DRV_IR_RX
	#define CONFIG_DRV_IR_RX_SW
#endif

/* #define WROK_AROUND_DCACHE_BUG */

// PWM not sleep when no connection
//#define CONFIG_PWM_NOT_SLEEP

/*Feature Select*/
/* #define BT_SD_MUSIC_COEXIST */

//#define CONFIG_ACTIVE_SSP
#define CONFIG_CTRL_BQB_TEST_SUPPORT            1
#define BT_MODE_1V1                             (1<<0)
#define BT_MODE_1V2                             (1<<1)
#define BT_MODE_TWS                             (1<<2)
#define BT_MODE_BLE                             (1<<3)
#define BT_MODE_DM_1V1                          (1<<4)
#define BT_MODE_DM_TWS                          (1<<5)

/********************************************BT_DUALMODE BEGIN*******************************************
 *BT_DUALMODE macro contorl,function config
 *added by yangyang, updated 2018/02/02 
 */
//#define BT_DUALMODE
#ifdef BT_DUALMODE 
    //#define LMP_LINK_L2CAL_TIMEOUT_ENABLE
    #define LE_FIRST_FULL_RX_TIMEOUT_ENBALE                     /* first_full_rx_timeout avoid BT disconn among mini LE_CONNECTION_INTERVAL */
    #define LE_SLEEP_ENABLE                                     /* DM sleep enable */
    #define LE_MUSIC_FLUENCY_ENABLE                             /* discrad BLE anchor point to ensure music palying fluency(Andriod 6.0 particularly)*/
    //#define LE_NO_PKT_RESCUE_ENABLE                             /* LE no pkt rescue with full windows RX */
#ifdef LE_SLEEP_ENABLE
    #ifndef CONFIG_CPU_CLK_OPTIMIZATION
        #define CONFIG_CPU_CLK_OPTIMIZATION         1
    #endif
    #if (CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE == 0)
        #define CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE      1
    #endif
    #if 0//def CONFIG_LINE_SD_SNIFF
        #undef CONFIG_LINE_SD_SNIFF
    #endif 
#else
    #ifdef CONFIG_CPU_CLK_OPTIMIZATION
        #undef CONFIG_CPU_CLK_OPTIMIZATION
        #define CONFIG_CPU_CLK_OPTIMIZATION 0
    #endif
    #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        #undef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    #endif 
#endif
#endif

/*******************************************BT_DUALMODE END***********************************************/

/*******************************************BT_DUALMODE_RW BEGIN******************************************/
#ifdef CONFIG_USE_BLE
    #define BT_DUALMODE_RW                              1       /* Dual mode BT(CEVA) + BLE(RW kernel), when enable this macro */
#else
    #define BT_DUALMODE_RW                              0       /* Dual mode BT(CEVA) + BLE(RW kernel), when enable this macro */
#endif

#if (BT_DUALMODE_RW == 1)
#define RW_BLE_WITH_ESCO_ENSURE_VOICE_FLUENCY           1       /* When RW_BLE conflict with esco, schedule to ensure voice fluency */
#define RW_BLE_WITH_MUSIC_ENSURE_MUSIC_FLUENCY          1       /* When RW_BLE conflict with music, schedule to ensure music fluency */
#define BT_DUALMODE_RW_SLEEP                            1       /* BT_DUALMODE_RW_SLEEP active if enbale */
#endif
/*******************************************BT_DUALMODE_RW BEGIN******************************************/

/********************************************OTA CONFIGURATION********************************************/
#define CONFIG_DRIVER_OTA                               1       /* OTA driver config */
#if (CONFIG_DRIVER_OTA == 1)
#if (BT_DUALMODE_RW == 1)
#define CONFIG_OTA_BLE                                  1       /* OTA APP data transfered by BLE */
#endif
#define CONFIG_OTA_SPP                                  1       /* OTA APP data transfered by SPP */
#define CONFIG_OTA_TWS                                  0       /* OTA APP data transparented by TWS */	

#define BEKEN_OTA                                       1       /* BEKEN OTA potocol */	
#endif

#if (!defined(DEBUG_IC_TEMPERATURE)) && (!defined(CONFIG_DBG_LOG_FLASH_OP))
    #define CONFIG_OTA_APP
#endif

/********************************************OTA CONFIGURATION********************************************/

#define CONFIG_AUTO_CONN_AB  //A ,B
//#define CONFIG_PRODUCT_TEST_INF

#define BT_MAX_AG_COUNT                         2
#define CONFIG_A2DP_PLAYING_SET_AG_FLOW		    1
#define CONFIG_AS_SLAVE_ROLE                    0   // 0: headsets as master      can open/close in SDK.
#define NEED_SNIFF_DEVICE_COUNT                 2
#define CONFIG_HFP17_MSBC_SUPPORTED             1

/*Memorize System Information into Flash, The Address based on Flash Size
    --8,   If 4Mbit Flash  4*1024*1024/8/0x10000
    --32,  If 8Mbit Flash
*/
#define FLASH_LAST_BLOCK_NUM		           (8)

#undef   ARN
#define  ARP
#undef   ALN
#define  ALP

/*********************AUD_FADE*******************************************/
#define CONFIG_AUD_FADE_IN_OUT                                 1
#if(CONFIG_AUD_FADE_IN_OUT == 1)
#define AUD_FADE_SCALE_MIN                                     (0)
#define AUD_FADE_SCALE_LEFT_SHIFT_BITS    	                   5
#define AUD_FADE_SCALE_MAX                                     (1 << AUD_FADE_SCALE_LEFT_SHIFT_BITS)
#define AUD_FADE_STEP                                          (1)     // about 640ms =  10ms * 128/2
#endif
/*************************************************************************/

//#define CONFIG_OPTIMIZING_SBC 		        1  // 20161222
//#define UPDATE_LOCAL_AFH_ENGINE_AFTER_ACK
//#define LMP_LINK_L2CAL_TIMEOUT_ENABLE
#define CONFIG_SBC_DECODE_BITS_EXTEND		    0
#define CONFIG_SBC_PROMPT      				    1
#define A2DP_SBC_DUMP_SHOW
#define CONFIG_SW_SWITCH_KEY_POWER			    0
#define AVRCP_IPHONE_VOL_EVEN                   1
#define CONFIG_VOLUME_SAVE_IN_ENV               0

/*************************************************************************
* control version upgraded.
*************************************************************************/
#define UPGRADED_VERSION    1

/*************************************************************************
* just only for pts testing, it should be closed in release version.
*************************************************************************/
#define PTS_TESTING 0

/*************************************************************************
* for a2dp-SRC <-> a2dp-SNK role switch, A2DP_ROLE_DEFAULT may be following value:

* A2DP_ROLE_AS_SRC:
    1. as a2dp source role;
    2. there is no HFP;
    3. we shall only can connect to other bluetooth earphone or voice box as a2dp-SRC role.

* A2DP_ROLE_AS_SNK:
    1. as a2dp sink role.

* A2DP_ROLE_SOURCE_CODE shall be defined as [ 1 ] when using [ A2DP_ROLE_AS_SRC & A2DP_ROLE_AS_SNK_SRC ].
*************************************************************************/
#define A2DP_ROLE_SOURCE_CODE                  0
#define A2DP_ROLE_AS_SNK                       1
#define A2DP_ROLE_AS_SRC                       2
#define A2DP_ROLE_AS_SNK_SRC                   3
#define A2DP_ROLE_DEFAULT                      A2DP_ROLE_AS_SRC

#if A2DP_ROLE_SOURCE_CODE
    #ifdef CONFIG_BLUETOOTH_HFP
    #undef CONFIG_BLUETOOTH_HFP
    #endif

    #ifdef CONFIG_BLUETOOTH_HSP
    #undef CONFIG_BLUETOOTH_HSP
    #endif

    #ifndef CONFIG_BLUETOOTH_A2DP_SOURCE
    #define CONFIG_BLUETOOTH_A2DP_SOURCE
    #endif

    #ifndef CONFIG_BLUETOOTH_A2DP_SINK
    #define CONFIG_BLUETOOTH_A2DP_SINK
    #endif

    #ifdef CONFIG_APP_AEC
    #undef CONFIG_APP_AEC
    #endif
#endif
/************************************************
* END FOR A2DP_ROLE_SOURCE_CODE
*************************************************/

#define POWERKEY_5S_PARING		               1
#ifdef BT_MAX_AG_COUNT
    #undef BT_MAX_AG_COUNT
    #define BT_MAX_AG_COUNT                    (app_check_bt_mode(BT_MODE_1V2)?2:1)
#endif

#define CONFIG_DRC	 				        0
#define CONFIG_APP_ESD				        0
#define CINFIG_CHARGER_VLCF_OFFSET	        5
#define POWER_ON_OUT_CHARGE_ENABLE        	0
#define CONFIG_TEMPERATURE_NTC			    0
#define CONFIG_OTHER_FAST_PLAY             	0
#define CONFIG_SHUTDOWN_WAKEUP_IO           0
#if (CONFIG_SHUTDOWN_WAKEUP_IO==1)
#define SECOND_WAKEUP_IO					GPIO13
#endif

#define CONFIG_A2DP_MPEG_AAC_ENABLE		       (1)
#if CONFIG_A2DP_MPEG_AAC_ENABLE
    #define A2DP_MPEG_AAC_DECODE
#endif

#define SDP_PNP_QUERY_ENABLE                0
#define CONFIG_BLUETOOTH_PBAP                   0
#if CONFIG_BLUETOOTH_PBAP
#define CONFIG_BLUETOOTH_PBAP_CLIENT            1
#define EXTRACT_VCARD
#endif



#ifdef CONFIG_ENC_ENABLE
#undef CONFIG_ENC_ENABLE
#define CONFIG_ENC_ENABLE 		                (0)
#endif
#define CONFIG_EXT_CHARGE_IO_DETECT      0
#if (CONFIG_EXT_CHARGE_IO_DETECT==1)
#define EXT_CHARGE_DETECT_IO                     GPIO30
#endif
#define CONFIG_AUD_EQS_SUPPORT              (0)
#if (CONFIG_AUD_EQS_SUPPORT==1)
#define APP_EQ_NUM         3
#endif

#define CONFIG_A2DP_CONN_DISCONN		0
#define CONFIG_HFP_CONN_DISCONN			0


#define REMOTE_RANGE_PROMPT           1 // Long distance disconnect and reconnect BT  0: disable 1:enable 
#endif
