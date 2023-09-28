/******************************************************************************
 * MODULE NAME:    sys_irq.h
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Hardware Interrupt Functions
 * MAINTAINER:     Tom Kerwick
 * DATE:           10 Jan 2011
 *
 * SOURCE CONTROL: $Id: sys_irq.h,v 1.1 2011/05/17 16:30:39 tomk Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
 ******************************************************************************/
#ifndef _PARTHUS_HW_IRQ_
#define _PARTHUS_HW_IRQ_

#include "sys_types.h"

#define INTR_PRI_CLS0       0
// #define INTR_PRI_CLS1       (1 << VIC_IDX_CEVA)
#define INTR_PRI_CLS_DEF    ((1 << VIC_IDX_CEVA) | (1 << VIC_IDX_USB0) | (1 << VIC_IDX_USB1))

/* Vector Interrupt Controller mast register bit macro */
#define VIC_IDX_USBPLUG                     (0 )
#define VIC_IDX_RTC                         (1 )
#define VIC_IDX_ANC                         (2 )
#define VIC_IDX_CEVA                        (3 )
#define VIC_IDX_CEC                         (4 )
#define VIC_IDX_I2S2                        (5 )
#define VIC_IDX_I2S1                        (6 )
#define VIC_IDX_SPDIF                       (7 )
#define VIC_IDX_SDIO                        (8 )
#define VIC_IDX_SADC                        (9 )
#define VIC_IDX_IRDA                        (10)
#define VIC_IDX_PWM                         (11)
#define VIC_IDX_I2S0                        (12)
#define VIC_IDX_TIMER1                      (13)
#define VIC_IDX_TIMER0                      (14)
#define VIC_IDX_I2C1                        (15)
#define VIC_IDX_I2C0                        (16)
#define VIC_IDX_SPI2                        (17)
#define VIC_IDX_SPI1                        (18)
#define VIC_IDX_SPI0                        (19)
#define VIC_IDX_UART2                       (20)
#define VIC_IDX_UART1                       (21)
#define VIC_IDX_UART0                       (22)
#define VIC_IDX_GPIO                        (23)
#define VIC_IDX_RWBT0                       (24)
#define VIC_IDX_RWBT1                       (25)
#define VIC_IDX_RWBT2                       (26)
#define VIC_IDX_BK24                        (27)
#define VIC_IDX_USB1                        (28)
#define VIC_IDX_USB0                        (29)
#define VIC_IDX_QSPI                        (30)
#define VIC_IDX_SBC                         (31)
#define VIC_IDX_FFT                         (32)
#define VIC_IDX_GENER_DMA                   (33)
#define VIC_IDX_MBX_DSP2CPU                 (34)
#define VIC_IDX_TOUCH                       (35)
#define VIC_IDX_VUSB_DLP                    (36)
// #define VIC_IDX_SWI_AUDIO                   VIC_IDX_TOUCH
#define VIC_IDX_SWI_AUDIO                   VIC_IDX_SPDIF
#define VIC_IDX_SWI_SYS                     VIC_IDX_QSPI


void vic_isr_usbplug(void);
void vic_isr_rtc(void);
void vic_isr_anc(void);
void vic_isr_ceva(void);
void vic_isr_cec(void);
void vic_isr_i2s2(void);
void vic_isr_i2s1(void);
void vic_isr_spdif(void);
void vic_isr_sdio(void);
void vic_isr_sadc(void);
void vic_isr_irda(void);
void vic_isr_pwm(void);
void vic_isr_i2s0(void);
void vic_isr_timer1(void);
void vic_isr_timer0(void);
void vic_isr_i2c1(void);
void vic_isr_i2c0(void);
void vic_isr_spi2(void);
void vic_isr_spi1(void);
void vic_isr_spi0(void);
void vic_isr_uart2(void);
void vic_isr_uart1(void);
void vic_isr_uart0(void);
void vic_isr_gpio(void);
void vic_isr_rwbt0(void);
void vic_isr_rwbt1(void);
void vic_isr_rwbt2(void);
void vic_isr_bk24(void);
void vic_isr_usb1(void);
void vic_isr_usb0(void);
void vic_isr_qspi(void);
void vic_isr_sbc(void);
void vic_isr_fft(void);
void vic_isr_gener_dma(void);
void vic_isr_mbx_dsp2cpu(void);
void vic_isr_touch(void);
void vic_isr_vusb_dlp(void);

DRAM_CODE void SYSirq_Disable_Interrupts_Save_Flags(uint32_t* flags, uint32_t* mask);
DRAM_CODE void SYSirq_Interrupts_Restore_Flags(uint32_t flags, uint32_t mask);
DRAM_CODE void SYSirq_Interrupts_Clear_Trig_Flags(void);

void SYSirq_Initialise(void);
void SYSirq_clear(void);
void SYSirq_set(void);
void SYSirq_Disable_Interrupts_Except(uint32_t *oldflags, uint32_t flags);
void SYSirq_Enable_All_Interrupts(uint32_t flags);
void SYSirq_Unmask_Interrupt(uint32_t *oldflags,uint32_t flags);
void SYSirq_enable_soft_interrupt(uint32_t id);
void SYSirq_soft_trigger_interrupt(uint32_t id);
void SYSirq_soft_clear_interrupt(uint32_t id);

typedef void (*jump_reset_func)(void);

#endif
