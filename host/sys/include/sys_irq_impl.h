/***********************************************************************
 *
 * MODULE NAME:    sys_irq_chimera_impl.h
 * PROJECT CODE:   BlueStream
 * MAINTAINER:     John Sheehy
 * DATE:           Header placed here on 24th April 2001
 *
 * SOURCE CONTROL: $Id: sys_irq_chimera_impl.h,v 1.5 2004/07/07 14:30:50 namarad Exp $
 *
 * LICENCE:
 *    This source code is copyright (c) 2001-2004 Ceva Inc.
 *    All rights reserved.
 *
 * REVISION HISTORY:
 *                 Initial date - see sys_irq.h
 *                 Fixed for new transport plane (clean UART choice 
 *                 mechanism) and cleaned up by JS 
 *
 ***********************************************************************/
#ifndef _PARTHUS_SYS_IRQ_CHIMERA_IMPL
#define _PARTHUS_SYS_IRQ_CHIMERA_IMPL

#include "sys_types.h"
#include "sys_config.h"
#include "sys_irq.h"
#include "bautil.h"
//#include "bk3000_reg.h"

void _sdio_isr (void);
void _sadc_isr (void);
void _aud_isr  (void);
void _pwm0_isr (void);
void _pwm1_isr (void);
void _pwm2_isr (void);
void _pwm3_isr (void);
void _pwm4_isr (void);
void _pwm5_isr (void);
void _gpio_isr (void);
void _spi_isr  (void);
void _ceva_isr (void);
void _i2c1_isr (void);
void _uart_isr (void);
void _uart1_isr (void);
void _irda_isr (void);
void _i2c0_isr (void);
void _usb_isr  (void);
void _fft_isr  (void);
void _dma_isr  (void);
void _timer1_isr(void);
void _usb_plug_in_isr(void);
void _mailbox_isr(void);

#endif	/* _PARTHUS_SYS_IRQ_CHIMERA_IMPL */
