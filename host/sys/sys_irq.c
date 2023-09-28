/***********************************************************************
 *
 * MODULE NAME:    sys_irq.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Hardware Interrupt Interface
 * MAINTAINER:     Tom Kerwick
 * CREATION DATE:  10 Jan 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
 ***********************************************************************/

#include "bkreg.h"
#include "sys_config.h"
#include "config.h"
#include "sys_types.h"
#include "sys_irq.h"
#include "sys_irq_impl.h"
#include "bautil.h"
#include "lslc_irq.h"
#include "hw_leds.h"
#include "beken_external.h"
//#include "driver_usb.h"
#include "drv_spi.h"
#ifdef CONFIG_DRIVER_I2C
#include "driver_i2c.h"
#endif
#if (BT_DUALMODE_RW == 1)
#include "rwip.h"
#include "rwble.h"
#include "rwbt.h"
#endif
#include "driver_beken_includes.h"

static uint32_t int_disable_count = 0;
static uint32_t int_restore_count = 0;

extern void uart_handler(void);
extern void uart1_handler(void);	//yuan++
extern void gpio_isr(void);
extern void vusb_isr(void);
extern void vusb_dlp_isr(void);
extern void saradc_isr(void);
#ifdef CONFIG_DRIVER_I2C1
extern void i2c1_isr(void);
#endif
/******************************************************************************
 *
 * FUNCTION:  SYSirq_Disable_Interrupts_Save_Flags
 * PURPOSE:   Disables ARM IRQ and FIQ Interrupts, saves previous PSR
 *
 ******************************************************************************/
void SYSirq_Disable_Interrupts_Save_Flags(uint32_t *flags, uint32_t *mask) 
{
    *flags = get_spr(SPR_SR);
    cpu_set_interrupts_enabled(0);
    *mask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), 0x00);

    int_disable_count++;
}

void SYSirq_Disable_Interrupts_Except(uint32_t *oldflags, uint32_t flags)
{
    cpu_set_interrupts_enabled(0);
    *oldflags = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), flags);
    cpu_set_interrupts_enabled(1);
}

void SYSirq_Enable_All_Interrupts(uint32_t flags)
{
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), flags);
    cpu_set_interrupts_enabled(1);
 
}

void SYSirq_Unmask_Interrupt(uint32_t *oldflags, uint32_t flags)
{
    cpu_set_interrupts_enabled(0);
    *oldflags = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), (*oldflags) & (~flags));
    cpu_set_interrupts_enabled(1);
}

void SYSirq_enable_soft_interrupt(uint32_t id)
{
    uint32_t val;
    if(id < 32)
    {
        val = get_spr(SPR_VICMR(0));
        set_spr(SPR_VICMR(0), (val | (1 << id)));   
    }
    else
    {
        id -= 32;
        val = get_spr(SPR_VICMR(1));
        set_spr(SPR_VICMR(1), (val | (1 << id)));   
    }
}

void SYSirq_soft_trigger_interrupt(uint32_t id)
{
    uint32_t val;
    if(id < 32)
    {
        val = get_spr(SPR_VICTR0);
        set_spr(SPR_VICTR0, (val | (1 << id)));
    }
    else
    {
        id -= 32;
        val = get_spr(SPR_VICTR1);
        set_spr(SPR_VICTR1, (val | (1 << id)));
    }
}

void SYSirq_soft_clear_interrupt(uint32_t id)
{
    uint32_t val;
    if(id < 32)
    {
        val = get_spr(SPR_VICTR0);
        set_spr(SPR_VICTR0, (val & (~(1 << id))));
    }
    else
    {
        id -= 32;
        val = get_spr(SPR_VICTR1);
        set_spr(SPR_VICTR1, (val & (~(1 << id))));
    }
}

/******************************************************************************
 *
 * FUNCTION:  SYSirq_Interrupts_Restore_Flags
 * PURPOSE:   Restores previously saved previous PSR
 *
 ******************************************************************************/
void SYSirq_Interrupts_Clear_Trig_Flags(void)
{
    if(get_spr(SPR_VICTR(0)))
    {
        set_spr(SPR_VICTR(0), 0);    
    }
}

void SYSirq_Interrupts_Restore_Flags(uint32_t flags, uint32_t mask) 
{
    int_restore_count++;

    if (int_disable_count > int_restore_count)
    {
        return;
    }
    else
    {
        int_disable_count = 0;
        int_restore_count = 0;
    }

    if (mask != 0)
    {
        set_spr(SPR_VICTR(0), get_spr(SPR_VICTR(0)) &(~(1 << VIC_IDX_CEVA)));
        set_spr(SPR_VICMR(0), mask);
    }

    set_spr(SPR_SR, flags);

}

inline void SYSirq_entry_intr_mask(uint32_t *oldmask, uint32_t mask, uint32_t *oldmask_H, uint32_t mask_H)
{
    *oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), (*oldmask) & mask);
    if(oldmask_H) {
        *oldmask_H = get_spr(SPR_VICMR(1));
        set_spr(SPR_VICMR(1), (*oldmask_H) & mask_H);
    }
    cpu_set_interrupts_enabled(1);
}

inline void SYSirq_exit_intr_mask(uint32_t *oldmask, uint32_t *oldmask_H)
{
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), *oldmask);
    if(oldmask_H) { set_spr(SPR_VICMR(0), *oldmask_H); }
}
/******************************************************************************
 *
 * FUNCTION:  SYSirq_Initialise
 * PURPOSE:   Initialize Interrupt Requests
 *
 ******************************************************************************/
void SYSirq_Initialise(void)
{
    // clear all interrupts
    set_spr(SPR_VICTR(0), 0x00000000);
    set_spr(SPR_VICTR(1), 0x00000000);

    // enable all interrupts
    set_spr(SPR_VICMR(0), 0xFFFFFFFF);
    set_spr(SPR_VICMR(1), 0xFFFFFFFF);

    // set interrupt handler
    set_vic_handler(SPR_VICVA(VIC_IDX_USBPLUG    ), vic_isr_usbplug    );
    set_vic_handler(SPR_VICVA(VIC_IDX_RTC        ), vic_isr_rtc        );
    set_vic_handler(SPR_VICVA(VIC_IDX_ANC        ), vic_isr_anc        );
    set_vic_handler(SPR_VICVA(VIC_IDX_CEVA       ), vic_isr_ceva       );
    set_vic_handler(SPR_VICVA(VIC_IDX_CEC        ), vic_isr_cec        );
    set_vic_handler(SPR_VICVA(VIC_IDX_I2S2       ), vic_isr_i2s2       );
    set_vic_handler(SPR_VICVA(VIC_IDX_I2S1       ), vic_isr_i2s1       );
    set_vic_handler(SPR_VICVA(VIC_IDX_SPDIF      ), vic_isr_spdif      );
    set_vic_handler(SPR_VICVA(VIC_IDX_SDIO       ), vic_isr_sdio       );
    set_vic_handler(SPR_VICVA(VIC_IDX_SADC       ), vic_isr_sadc       );
    set_vic_handler(SPR_VICVA(VIC_IDX_IRDA       ), vic_isr_irda       );
    set_vic_handler(SPR_VICVA(VIC_IDX_PWM        ), vic_isr_pwm        );
    set_vic_handler(SPR_VICVA(VIC_IDX_I2S0       ), vic_isr_i2s0       );
    set_vic_handler(SPR_VICVA(VIC_IDX_TIMER1     ), vic_isr_timer1     );
    set_vic_handler(SPR_VICVA(VIC_IDX_TIMER0     ), vic_isr_timer0     );
    set_vic_handler(SPR_VICVA(VIC_IDX_I2C1       ), vic_isr_i2c1       );
    set_vic_handler(SPR_VICVA(VIC_IDX_I2C0       ), vic_isr_i2c0       );
    set_vic_handler(SPR_VICVA(VIC_IDX_SPI2       ), vic_isr_spi2       );
    set_vic_handler(SPR_VICVA(VIC_IDX_SPI1       ), vic_isr_spi1       );
    set_vic_handler(SPR_VICVA(VIC_IDX_SPI0       ), vic_isr_spi0       );
    set_vic_handler(SPR_VICVA(VIC_IDX_UART2      ), vic_isr_uart2      );
    set_vic_handler(SPR_VICVA(VIC_IDX_UART1      ), vic_isr_uart1      );
    set_vic_handler(SPR_VICVA(VIC_IDX_UART0      ), vic_isr_uart0      );
    set_vic_handler(SPR_VICVA(VIC_IDX_GPIO       ), vic_isr_gpio       );
    set_vic_handler(SPR_VICVA(VIC_IDX_RWBT0      ), vic_isr_rwbt0      );
    set_vic_handler(SPR_VICVA(VIC_IDX_RWBT1      ), vic_isr_rwbt1      );
    set_vic_handler(SPR_VICVA(VIC_IDX_RWBT2      ), vic_isr_rwbt2      );
    set_vic_handler(SPR_VICVA(VIC_IDX_BK24       ), vic_isr_bk24       );
    set_vic_handler(SPR_VICVA(VIC_IDX_USB1       ), vic_isr_usb1       );
    set_vic_handler(SPR_VICVA(VIC_IDX_USB0       ), vic_isr_usb0       );
    set_vic_handler(SPR_VICVA(VIC_IDX_QSPI       ), vic_isr_qspi       );
    set_vic_handler(SPR_VICVA(VIC_IDX_SBC        ), vic_isr_sbc        );
    set_vic_handler(SPR_VICVA(VIC_IDX_FFT        ), vic_isr_fft        );
    set_vic_handler(SPR_VICVA(VIC_IDX_GENER_DMA  ), vic_isr_gener_dma  );
    set_vic_handler(SPR_VICVA(VIC_IDX_MBX_DSP2CPU), vic_isr_mbx_dsp2cpu);
    set_vic_handler(SPR_VICVA(VIC_IDX_TOUCH      ), vic_isr_touch      );
    set_vic_handler(SPR_VICVA(VIC_IDX_VUSB_DLP   ), vic_isr_vusb_dlp   );

    // set priority    
    set_spr(SPR_VICPR(VIC_IDX_USBPLUG    ), 1);
    set_spr(SPR_VICPR(VIC_IDX_RTC        ), 1);
    set_spr(SPR_VICPR(VIC_IDX_ANC        ), 7);
    set_spr(SPR_VICPR(VIC_IDX_CEVA       ), 6);
    set_spr(SPR_VICPR(VIC_IDX_CEC        ), 1);
    set_spr(SPR_VICPR(VIC_IDX_I2S2       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_I2S1       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SPDIF      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SDIO       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SADC       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_IRDA       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_PWM        ), 5);
    set_spr(SPR_VICPR(VIC_IDX_I2S0       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_TIMER1     ), 1);
    set_spr(SPR_VICPR(VIC_IDX_TIMER0     ), 1);
    set_spr(SPR_VICPR(VIC_IDX_I2C1       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_I2C0       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SPI2       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SPI1       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SPI0       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_UART2      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_UART1      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_UART0      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_GPIO       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_RWBT0      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_RWBT1      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_RWBT2      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_BK24       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_USB1       ), 4);
    set_spr(SPR_VICPR(VIC_IDX_USB0       ), 4);
    set_spr(SPR_VICPR(VIC_IDX_QSPI       ), 1);
    set_spr(SPR_VICPR(VIC_IDX_SBC        ), 1);
    set_spr(SPR_VICPR(VIC_IDX_FFT        ), 1);
    set_spr(SPR_VICPR(VIC_IDX_GENER_DMA  ), 1);
    set_spr(SPR_VICPR(VIC_IDX_MBX_DSP2CPU), 6);
    set_spr(SPR_VICPR(VIC_IDX_TOUCH      ), 1);
    set_spr(SPR_VICPR(VIC_IDX_VUSB_DLP   ), 1);

    //enable pic envent if needed, such as HALT called
    //set_spr(SPR_PM_EVENT_CTRL, 0x01);

    //set global interrupt enable
    set_spr(SPR_SR, get_spr(SPR_SR) | SPR_SR_IEE);
}

static inline void int_handler_audio_sw(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & ~((1 << VIC_IDX_SWI_AUDIO) | (1 << VIC_IDX_SWI_SYS)));
    SYSirq_soft_clear_interrupt(VIC_IDX_SWI_AUDIO);
    cpu_set_interrupts_enabled(1);
    extern void audio_task_swi(void);
    audio_task_swi();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_usbplug(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    vusb_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}
void int_handler_rtc(void)        {}
void int_handler_anc(void)        {}
void int_handler_ceva(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS0);
    LSLCirq_IRQ_Handler();
    set_spr(SPR_VICMR(0), oldmask);
}
void int_handler_cec(void)        {}
void int_handler_i2s2(void)       {}
void int_handler_i2s1(void)       {}
void int_handler_i2s0(void)       {}
void int_handler_spdif(void)
{
#if VIC_IDX_SWI_AUDIO == VIC_IDX_SPDIF
    int_handler_audio_sw();
#elif defined(CONFIG_SPDIF_DRV)
    #if VIC_IDX_SWI_AUDIO < 32
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & ~((1 << VIC_IDX_SPDIF) | (1 << VIC_IDX_SWI_AUDIO) | (1 << VIC_IDX_SWI_SYS)));
    #else
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    unsigned int oldmask1 = get_spr(SPR_VICMR(1));
    set_spr(SPR_VICMR(0), oldmask & ~(1 << VIC_IDX_SPDIF));
    set_spr(SPR_VICMR(1), oldmask1 & ~(1 << (VIC_IDX_SWI_AUDIO - 32)));
    #endif
    cpu_set_interrupts_enabled(1);
    extern void spdif_isr(void);
    spdif_isr();
    cpu_set_interrupts_enabled(0);
    #if VIC_IDX_SWI_AUDIO < 32
    set_spr(SPR_VICMR(0), oldmask);
    #else
    set_spr(SPR_VICMR(0), oldmask);
    set_spr(SPR_VICMR(1), oldmask1);
    #endif
#endif
}

void int_handler_sdio(void)       {}
void int_handler_sadc(void)       
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
	
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    saradc_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}
void int_handler_irda(void)
{
#ifdef CONFIG_DRV_IR_RX
	unsigned int oldmask = get_spr(SPR_VICMR(0));
	set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
	extern void irda_rx_isr(void);
	irda_rx_isr();
	cpu_set_interrupts_enabled(0);
	set_spr(SPR_VICMR(0), oldmask);
#elif VIC_IDX_SWI_AUDIO == VIC_IDX_IRDA
    int_handler_audio_sw();
#endif
}
void int_handler_pwm(void)        
{
#if CONFIG_DRIVER_PWM
    unsigned int oldmask = get_spr(SPR_VICMR(0));
	
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    pwm_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
#endif
}

void int_handler_timer0(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    timer_timer0_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_timer1(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    timer_timer1_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_i2c1(void)       
{
#if (defined(CONFIG_DRIVER_I2C) && I2C1_ENABLE)
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);                 //mask all/low priority interrupt.
    cpu_set_interrupts_enabled(1);
    i2c_hw_interrupt(I2C_1);
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
#endif
}

void int_handler_i2c0(void)       
{
#if (defined(CONFIG_DRIVER_I2C) && I2C0_ENABLE)
    unsigned int oldmask = get_spr(SPR_VICMR(0));    //read old spr_vicmr
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);                 //mask all/low priority interrupt.
    cpu_set_interrupts_enabled(1);
    i2c_hw_interrupt(I2C_0);
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  //recover the old spr_vicmr.
#endif
}

void int_handler_spi2(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
#if (SPI_2_FUN4_ENABLE == 1)
    spi_hw_interrupt(SPI_2_FUN_4);
#elif (SPI_2_FUN1_ENABLE == 1)
    spi_hw_interrupt(SPI_2_FUN_1);
#endif
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_spi1(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
#if (SPI_1_FUN1_ENABLE == 1)
    spi_hw_interrupt(SPI_1_FUN_1);
#endif
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_spi0(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
#if (SPI_0_FUN1_ENABLE == 1)
    spi_hw_interrupt(SPI_0_FUN_1);
#endif
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_uart2(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));

    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    extern void uart2_handler(void);
    uart2_handler();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_uart1(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));

    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    uart1_handler();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_uart0(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
	
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    uart_handler();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}
void int_handler_gpio(void)       
{
    unsigned int oldmask = get_spr(SPR_VICMR(0)); 
	  
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    gpio_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);                  
}

#if (BT_DUALMODE_RW == 1)
void int_handler_rwbt0(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr( SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    rwip_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_rwbt1(void)
{
#if defined(CFG_BLE)
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr( SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    rwble_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
#endif
}

void int_handler_rwbt2(void)
{
#if defined(CFG_BT)
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr( SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
    rwbt_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
#endif
}
#else
void int_handler_rwbt0(void)      {}
void int_handler_rwbt1(void)      {}
void int_handler_rwbt2(void)      {}
#endif
void int_handler_bk24(void)       {}
void int_handler_usb1(void)
{
#if CONFIG_USE_USB
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS0);
    // cpu_set_interrupts_enabled(1);
    extern void usb1_handler(void);
    usb1_handler();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
#endif
}
void int_handler_usb0(void)       
{
#if CONFIG_USE_USB
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS0);
    // cpu_set_interrupts_enabled(1);
    extern void usb0_handler(void);
    usb0_handler();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask); 
#endif
}

void int_handler_qspi(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), oldmask & ~(1 << VIC_IDX_SWI_SYS));
    SYSirq_soft_clear_interrupt(VIC_IDX_SWI_SYS);
    cpu_set_interrupts_enabled(1);
    extern void sys_task_swi(void);
    sys_task_swi();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_sbc(void)
{
    __asm__("b.lwz  r3, 0x1030004(r0);");
    __asm__("b.ori  r3, r3, 0x1;");
    __asm__("b.sw   0x1030004(r0), r3;");
}
void int_handler_fft(void)        {}
void int_handler_gener_dma(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
    set_spr(SPR_VICMR(0), 0);
    dma_hw_interrupt();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

void int_handler_mbx_dsp2cpu(void){}
void int_handler_touch(void)
{
#if VIC_IDX_SWI_AUDIO == VIC_IDX_TOUCH
    int_handler_audio_sw();
#endif
}
void int_handler_vusb_dlp(void)
{
    unsigned int oldmask = get_spr(SPR_VICMR(0));
	
#if 0
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS_DEF);
    cpu_set_interrupts_enabled(1);
#endif
    set_spr(SPR_VICMR(0), oldmask & INTR_PRI_CLS0);
    vusb_dlp_isr();
    cpu_set_interrupts_enabled(0);
    set_spr(SPR_VICMR(0), oldmask);
}

