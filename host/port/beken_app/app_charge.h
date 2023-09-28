/**
 * @file   tlv.c
 * @author
 *
 * @brief
 *
 * (c) 2018 BEKEN Corporation, Ltd. All Rights Reserved
 */

#ifndef __CHARGE_H__
#define __CHARGE_H__

//#include "stdint.h"
#define CONFIG_RECHARGER_VLCF_OFFSET        3
#define CONFIG_CHARGER_VLCF_OFFSET			5
#define CONFIG_AUTO_SWITCH_JITTER_CNT                   10 // 500ms   pulse jitter
#define CONFIG_CHARGE_JITTER_CNT                        50 // 500ms   pulse jitter
#define CONFIG_CHARGE_PREPARE_CNT                    CONFIG_CHARGE_JITTER_CNT + 100 // 500ms(pulse jitter) + 1.5s   enter charge avtive timer 
#define POWERON_VBAT_THRESHOLD      3000
#define POWERON_VBAT_OFFSET          600
#define POWERON_VBAT_LOWER_OFFSET    300
#define DEFAULT_CHARGE_CURRENT          3   // 1 = 30mA,2=40mA,3=50mA,4 = 60mA
#define CONFIG_MINI_CHARGE_DEEPSLEEP    0

typedef enum
{
    CHARGE_PREPARE_NONE = 0,
    CHARGE_PREPARING    = 1,
    CHARGE_PREPARED     = 2
}t_battery_prepare;

typedef enum
{
    BATTERY_CHARGE_PREPARE= 0,
    BATTERY_CHARGE_ACTIVE,
    BATTERY_CHARGING,
    BATTERY_CHARGE_FULL,
    BATTERY_CHARGE_FINISHED,
    BATTERY_CHARGE_ABORTED,
    BATTERY_CHARGE_FULL_POWER
}t_battery_charge_state;

#define VUSB_IS_CHARGE_MODE         (vusb_get_mode() == VUSB_CHARGE_MODE)
#define IS_USB_CHARGE_FULL                   (REG_SYSTEM_0x31&MSK_SYSTEM_0x31_CHARGE_FULL)
#define IS_USB_CHARGE_CON_VOLT          (REG_SYSTEM_0x31&MSK_SYSTEM_0x31_CHARGE_CON_VOL)
#define IS_USB_CHARGE_ENABLE              (REG_SYSTEM_0x44&MSK_SYSTEM_0x44_CHARGE_ENABLE)
#define IS_USB_CHARGE_EXT                    (REG_SYSTEM_0x44&MSK_SYSTEM_0x44_EXT_CHARGE)
#define mSET_USB_CHARGE_CURRENT(current)     (REG_SYSTEM_0x43 = ((REG_SYSTEM_0x43 & ~(MSK_SYSTEM_0x43_CHARGE_CURRENT)) | ((current) << SFT_SYSTEM_0x43_CHARGE_CURRENT)))
#define mSET_USB_CHARGE_VLCF(vlcf)                  (REG_SYSTEM_0x43 = ((REG_SYSTEM_0x43 & ~(MSK_SYSTEM_0x43_VLCF)) | ((vlcf) << SFT_SYSTEM_0x43_VLCF)))
#define mSET_USB_CHARGE_ICP(icp)                     (REG_SYSTEM_0x59 = ((REG_SYSTEM_0x59 & ~(MSK_SYSTEM_0x59_ICP)) | ((icp) << SFT_SYSTEM_0x59_ICP)))
#define mSET_USB_CHARGE_VCV(vcv)                    (REG_SYSTEM_0x44 = ((REG_SYSTEM_0x44 & ~(MSK_SYSTEM_0x44_VCV)) | ((vcv) << SFT_SYSTEM_0x44_VCV)))
#define mSET_USB_CHARGE_ENABLE(enable)        (REG_SYSTEM_0x44 = ((REG_SYSTEM_0x44 & ~(MSK_SYSTEM_0x44_CHARGE_ENABLE)) | ((enable) << SFT_SYSTEM_0x44_CHARGE_ENABLE)))
#define mSET_USB_DETECT_ENABLE(enable)         (REG_SYSTEM_0x44 = ((REG_SYSTEM_0x44 & ~(MSK_SYSTEM_0x44_USB_DETECT_EN)) | ((enable) << SFT_SYSTEM_0x44_USB_DETECT_EN)))
#define mSET_USB_CHARGE_EXT(enable)               (REG_SYSTEM_0x44 = ((REG_SYSTEM_0x44 & ~(MSK_SYSTEM_0x44_EXT_CHARGE)) | ((enable) << SFT_SYSTEM_0x44_EXT_CHARGE)))
#define mSET_USB_CHARGE_SMALL_CURRENT(small)    (REG_SYSTEM_0x5A = ((REG_SYSTEM_0x5A & ~(MSK_SYSTEM_0x5A_SMALL_CURRENT)) | ((small) << SFT_SYSTEM_0x5A_SMALL_CURRENT)))

void app_charge_init(void);
t_battery_charge_state get_Charge_state(void);
uint8_t app_charge_powerOn_handler(void);
void app_charge_handler(uint32_t step );
uint8_t app_charge_powerOff_handler(void);
void app_charge_button_handle(void);
uint8_t app_charge_is_powerdown( void );
void app_charge_reset_usb_plug_in(void);
void app_charge_disable_usb_plug_detect(void);
uint8_t app_charge_is_usb_plug_in(void);
void app_poweroff_battery_charge_wakeup(void);
uint8_t get_charge_is_preparing(void);
void app_charge_fake_powerdown( void );
void app_battery_prepare_charge(void);
void app_charge_process(void);
uint16 app_charge_check_vbat(void);
void app_charge_mini_active(void);
void app_charge_mini_sched(void);

void app_charge_save_flag_before_Wakeup(void);
int app_charge_is_wakeup_for_charge_full(void);

#endif//__tlv_H__

