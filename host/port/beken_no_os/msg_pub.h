#ifndef _MSG_PUB_H_
#define _MSG_PUB_H_

#include <stddef.h>
#include <stdint.h>
#include "config.h"

#define DELAY_MSG_PUT                (1)
#define OS_TICK_MSEC                 (10)

#define MSG_SUCCESS_RET              (0)
#define MSG_FAILURE_RET              (-1)

typedef void (*msg_process)(uint32_t arg,uint32_t arg2);

typedef struct
{
    uint32_t id;

    void *hdl;
    uint32_t arg;
    uint32_t arg2;
}MSG_T, *MSG_PTR;

/* public api*/
extern void msg_init(void);
extern void msg_dump(void);
extern void msg_uninit(void);
extern void msg_put(uint32_t msg);
extern void msg_clear_all(void);
extern int msg_priority_put(MSG_T *msg);

#ifdef DELAY_MSG_PUT
extern int msg_delay_put(uint32_t os_tick, MSG_T *msg);
extern void msg_delay_put_handle(uint32_t os_tick_count, uint32_t id);
#endif

/* message table*/
#define KEY_MSG_GP                      (0x0000FF00)         /* Attention: special msg*/
#define BLUETOOTH_MSG_GP                (0x10000000)
#define SDMUSIC_MSG_GP                  (0x20000000)
#define UDISK_MSG_GP                    (0x30000000)
#define FM_MSG_GP                       (0x40000000)
#define LINEIN_MSG_GP                   (0x50000000)
#define OTHER_MSG_GP                    (0x60000000)
#define ENV_MSG_GP                      (0x70000000)
#define USB_MSG_GP                      (0x80000000)

/* Name format: MSG_module_messageInformation
   assume: message number is less than 65535 at one module
*/
enum
{
    MSG_NULL                         = 0,

    /* Attention: special msg for key press, from 0x0000ff00--0x0000ffff*/
    MSG_KEYPRESS                     = KEY_MSG_GP       + 0x00,
        
    /*BLK0: bluetooth msg*/
    MSG_BT_INIT                      = BLUETOOTH_MSG_GP + 0x0000,
    
    /*BLK1: sd music msg*/
    MSG_SD_INIT                      = SDMUSIC_MSG_GP   + 0x0000,
    MSG_SD_NOTIFY,
    
    /*BLK2: usb disk msg*/
    MSG_UDISK_INIT                   = UDISK_MSG_GP     + 0x0000,
    /*BLK2: usb msg*/
    /* host msg */
    MSG_USB_HOST_INIT                = USB_MSG_GP       + 0x0000,
	MSG_USB_HOST_ATTACH_DEVICE,
	MSG_USB_HOST_DETACH_DEVICE,
	MSG_USB_HOST_ACTIVELY_ATTACH,
	MSG_USB_HOST_ACTIVELY_DETATCH,
	MSG_USB_HOST_ATTACH_UDISK_OK,
	MSG_USB_HOST_ATTACH_UDISK_ERROR,
	MSG_USB_HOST_DETACH_UDISK,
	MSG_USB_HOST_ATTACH_OTHER_DEVICE_OK,
	MSG_USB_HOST_ATTACH_OTHER_DEVICE_ERROR,
    /* device msg */
    MSG_USB_DEVICE_INIT              = USB_MSG_GP       + 0x8000,
	MSG_USB_DEVICE_ENUM_START,
	MSG_USB_DEVICE_ENUM_OVER,
    
    /*BLK3: fm msg*/
    MSG_FM_INIT                      = FM_MSG_GP        + 0x0000,
    MSG_FM_SEEK_CONTIUE,
    MSG_FM_MEMERIZE,
    MSG_FM_RESTORE,
    MSG_FM_INSTALL_START,
    MSG_FM_INSTALL_HW,
    MSG_FM_INSTALL_CHANNEL,
    MSG_FM_UNINSTALL,
    MSG_FM_SEEK_PREV_CHANNEL,
    MSG_FM_SEEK_NEXT_CHANNEL,
    MSG_FM_AUTO_SEEK_START,
    MSG_FM_CHANNEL_SEEK_START,
    MSG_FM_CHANNEL_TUNE_CONTINUE,                  /* 0xc*/
    MSG_FM_CHANNEL_SEEK_CONFIG,
    MSG_FM_CHANNEL_TUNE_OVER,
    MSG_FM_TUNE_SUCCESS_CONTINUE,
    MSG_FM_TUNE_FAILURE_CONTINUE,
    MSG_FM_AUTO_SEEK_OVER,
    MSG_FM_DISABLE_MUTE,
    MSG_FM_ENABLE_MUTE,
    
    
    /*BLK4: linein msg*/
    MSG_LINEIN_INIT                  = LINEIN_MSG_GP  + 0x0000,
    
    /*BLK5: other msg*/   
    MSG_LED_INIT                     = OTHER_MSG_GP   + 0x0000,
    MSG_SD_ATTACH_CHANGE,            /* sd attach or detach*/
    MSG_SD_READ_ERR,                 /* mp3-mode,SD read Err */
    MSG_SDADC,                       /* sdadc*/
    MSG_POWER_DOWN,
    MSG_POWER_UP,
    MSG_FLASH_WRITE,
    MSG_IRDA_RX,
    MSG_USER_SarADC_UPDT,
    MSG_LOWPOWER_DETECT,
    MSG_TEMPERATURE_DETECT,
    MSG_CY_TEST,
    MSG_TEMPRATURE_NTC_DETECT,
    MSG_TEMPRATURE_APLL_TOGGLE,
    MSG_TEMPRATURE_DPLL_TOGGLE,
    MSG_ENV_WRITE_ACTION            = ENV_MSG_GP + 0x0000,
    
    MSG_LINEIN_ATTACH,
    MSG_LINEIN_DETACH,

    MSG_TIMER_PWM1_PT2_HDL,
    MSG_SAVE_VOLUME,
    
#if(POWERKEY_5S_PARING == 1)
    MSG_ENTER_MATCH_STATE,
    MSG_POWER_ON_START_CONN,
#endif

	MSG_SWITCH_POWER_DOWN,
	MSG_CHARGE_FAKE_POWER_DOWN,
	MSG_VUSB_DETECT,
	MSG_VUSB_DETECT_JITTER,
#if (CONFIG_DRIVER_QSPI == 1)
	MSG_QSPI_FLASH_DEBUG,
	MSG_QSPI_LCD_DEBUG,
#endif
	MSG_CLEAT_MEMORY,
	MSG_CHANGE_MODE,
#if (CONFIG_EAR_IN == 1)
	MSG_EAR_ATTACH,
	MSG_EAR_DETACH,
	MSG_EAR_ATTACH_WAVEFILE,
	MSG_EAR_ATTACH_SEND_STATUS,
	MSG_EAR_DETACH_SEND_STATUS,	
#endif
    MSG_USB_SPK_OPEN,
    MSG_USB_SPK_CLOSE,
    MSG_USB_MIC_OPEN,
    MSG_USB_MIC_CLOSE,
};

#endif
// EOF
