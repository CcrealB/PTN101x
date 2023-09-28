#ifndef _DRIVER_BEKEN_INCLUDES_H_
#define _DRIVER_BEKEN_INCLUDES_H_

#include <stddef.h>
#include <stdint.h>
#include <config/config.h>
#include "excutil.h"
#include "board.h"
#include "spr_defs.h"
#include "bautil.h"
#include "port.h"
#include "msg_pub.h"
#include "msg.h"
#include "bkreg.h"
#include "sys_types.h"
#include "driver_ringbuff.h"
#include "drv_ring_buffer.h"
#include "drv_msg.h"
#include "driver_icu.h"
#include "driver_serio.h"
#include "driver_audio.h"
#include "driver_gpio.h"
#include "driver_saradc.h"
#include "driver_flash.h"
#include "driver_dma.h"
#include "drv_spi.h"
#include "drv_audio.h"
#include "drv_system.h"
#include "drv_mailbox.h"
#include "drv_timer.h"
#include "target.h"
#include "timer.h"
#include "sys_irq.h"
#include "driver_efuse.h"
#include "types.h"
#include "driver_sdcard.h"
//#include "driver_usb.h"
#include "driver_vusb.h"
#include "driver_vusb_dlp.h"
#if CONFIG_AUDIO_SYNC_ENABLE
#include "audio_sync.h"
#endif
#ifdef CONFIG_DRIVER_I2C1
#include "Driver_i2c1.h"
#endif
#if CONFIG_DRIVER_PWM
#include "driver_pwm.h"
#endif
#if 1//(CONFIG_DRIVER_I2S)
#include "driver_i2s.h"
#endif

#define A2DP_STREAM_ON        0x100
#define A2DP_STREAM_OFF       0x200
#define HF_INCOMING_CALL      0x400
#define HF_CALL_OVER          0x800
#define HF_OUTGOING_CALL      0x1000
#define A2DP_CONNECTED        0x2000

/****software interrupt flag**********/
extern volatile uint32_t sleep_tick;
extern volatile uint32_t pwdown_tick;
extern volatile uint16_t sniffmode_wakeup_dly;
extern volatile uint16_t adcmute_cnt;
#define CLEAR_SLEEP_TICK     do{sleep_tick = 0;}while(0)
#define INC_SLEEP_TICK       do{sleep_tick++;}while(0)
#define SLEEP_TICK_CHECK     1000
#define CLEAR_PWDOWN_TICK    do{pwdown_tick = 0;}while(0)
#define INC_PWDOWN_TICK(step)do{pwdown_tick += step;}while(0)
#define POWER_DOWN_CHECK     -1

#define ATOMIC_OR(variable, bit) do{\
        uint32_t info, mask;    \
        SYSirq_Disable_Interrupts_Save_Flags(&info,&mask);  \
        variable |= (bit);                                  \
        SYSirq_Interrupts_Restore_Flags(info,mask);       \
                                    }while(0)

#define ATOMIC_AND(variable, bit) do{                     \
        uint32_t info,mask;    \
        SYSirq_Disable_Interrupts_Save_Flags(&info,&mask);  \
        variable &= (bit);                                  \
        SYSirq_Interrupts_Restore_Flags(info,mask);       \
                                    }while(0)
#if 0
void global_event_set(uint32_t flag);
void global_event_clear(uint32_t flag);
#endif

#endif
