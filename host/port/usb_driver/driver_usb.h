/*************************************************************
 * @file        driver_usb.h
 * @brief       Header file of driver_usb.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_USB_H__
#define __DRIVER_USB_H__

// #include "types.h"
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
//3435:5
//3633,101x,7231x:16
#define MAX_ENDPOINT					16

#define REG_USB_BASE_ADDR_PTN3633		(0x00804000UL)
#define REG_USB_BASE_ADDR_PTN7231U		(0x00804000UL)
#define REG_USB_BASE_ADDR_PTN3435		(0x00804000UL)
#define REG_USB_BASE_ADDR_PTN101x		(0x01052000UL)
#define REG_USB_BASE_ADDR               REG_USB_BASE_ADDR_PTN101x

#if REG_USB_BASE_ADDR==REG_USB_BASE_ADDR_PTN101x
#define REG_USB_CPU_OFFSET               0x200
#else
#define REG_USB_CPU_OFFSET               0
#endif

#define USB_3435_v2_STYLE

#ifdef USB_3435_v2_STYLE
#define REG_USB_FADDR                     (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x00)))
#define REG_USB_POWER                     (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x01)))
#define REG_USB_INTRTX1                   (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x02)))
#define REG_USB_INTRTX2                   (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x03)))
#define REG_USB_INTRRX1                   (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x04)))
#define REG_USB_INTRRX2                   (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x05)))
#define REG_USB_INTRUSB                   (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x06)))
#define REG_USB_INTRTX1E                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x07)))
#define REG_USB_INTRTX2E                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x08)))
#define REG_USB_INTRRX1E                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x09)))
#define REG_USB_INTRRX2E                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x0A)))
#define REG_USB_INTRUSBE                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x0B)))
#define REG_USB_FRAME1                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x0C)))
#define REG_USB_FRAME2                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x0D)))
#define REG_USB_INDEX                     (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x0E)))
#define REG_USB_DEVCTL                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x0F)))
#define REG_USB_TXMAXP                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x10)))
#define REG_USB_CSR0                      (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x11)))
#define REG_USB_CSR02                     (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x12)))
#define REG_USB_RXMAXP                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x13)))
#define REG_USB_RXCSR1                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x14)))
#define REG_USB_RXCSR2                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x15)))
#define REG_USB_COUNT0                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x16)))
#define REG_USB_RXCOUNT2                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x17)))
#define REG_USB_TXTYPE                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x18)))
#define REG_USB_NAKLIMIT0                 (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x19)))
#define REG_USB_RXTYPE                    (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x1A)))
#define REG_USB_RXINTERVAL                (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x1B)))

#define REG_USB_FIFOSIZE                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x1F)))

#define REG_USB_FIFO0                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x20)))
#define REG_USB_FIFO1                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x24)))
#define REG_USB_FIFO2                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x28)))
#define REG_USB_FIFO3                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x2C)))
#define REG_USB_FIFO4                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x30)))
#define REG_USB_FIFO5                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x34)))
#define REG_USB_FIFO6                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x38)))
#define REG_USB_FIFO7                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x3C)))
#define REG_USB_FIFO8                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x40)))
#define REG_USB_FIFO9                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x44)))
#define REG_USB_FIFOA                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x48)))
#define REG_USB_FIFOB                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x4C)))
#define REG_USB_FIFOC                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x50)))
#define REG_USB_FIFOD                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x54)))
#define REG_USB_FIFOE                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x58)))
#define REG_USB_FIFOF                     (*((volatile unsigned long *) (REG_USB_BASE_ADDR + 0x5C)))

#define REG_AHB2_USB_DMA_INTR_STUS        (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x00 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_OTG_CFG              (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x80 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_DMA_ENDP             (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x84 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_VTH                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x88 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_GEN                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x8C + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_STAT                 (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x90 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_INT                  (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x94 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_RESET                (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x98 + REG_USB_CPU_OFFSET)))
#define REG_AHB2_USB_DEV_CFG              (*((volatile unsigned char *) (REG_USB_BASE_ADDR + 0x9C + REG_USB_CPU_OFFSET)))
#endif      /* #ifdef USB_3435_v2_STYLE */
#define _REG8(pbase,reg)		(((volatile unsigned char *)pbase)[((unsigned char*)&reg-(unsigned char*)REG_USB_BASE_ADDR)])
#define MGC_M_INTR_SUSPEND    0x01
#define MGC_M_INTR_RESUME     0x02
#define MGC_M_INTR_RESET      0x04
#define MGC_M_INTR_BABBLE     0x04
#define MGC_M_INTR_SOF        0x08
#define MGC_M_INTR_CONNECT    0x10
#define MGC_M_INTR_DISCONNECT 0x20
#define MGC_M_INTR_SESSREQ    0x40
#define MGC_M_INTR_VBUSERROR  0x80   /* FOR SESSION END */


#if 1//def CONFIG_USB_DMA //add by Borg @230326

typedef struct _USB_DMAx_CFG_t{
    union
    {
        struct{
        volatile uint32_t en         :1;//dma enable
        volatile uint32_t dir_is_rx  :1;//0:tx, mem->usb; 1:rx, mem<-usb;
        volatile uint32_t cont_mod_en:1;//0:single/1:continue mode
        volatile uint32_t intr_en    :1;//dma intr enable
        volatile uint32_t endp       :4;//end point num
        volatile uint32_t max_pkt_sz :7;//(unit:8 Byte) max trans size, only for continue mode
        volatile uint32_t bus_err    :1;
        volatile uint32_t rsvd_cfg   :16;
        };
        volatile uint32_t cfg;
    };
    volatile uint32_t addr;//mem addr
    volatile uint32_t size;//byte num
}USB_DMAx_CFG_t;

typedef struct _USB_DMA_CFG_t{
    volatile uint32_t dma1_intr_stus :1;
    volatile uint32_t dma2_intr_stus :1;
    volatile uint32_t rsvd           :30;
    volatile USB_DMAx_CFG_t dma1;
    volatile uint32_t rsvd_0x210;
    volatile USB_DMAx_CFG_t dma2;
}USB_DMA_CFG_t;

void usb_dma_init(void* bp, uint8_t DMAx, uint8_t* buff, uint32_t size,
    uint8_t dir_is_rx, uint8_t cont_mod_en, uint8_t intr_en, uint8_t endp);
USB_DMA_CFG_t* get_usb_dma_ptr(void* bp);

#endif

uint32_t HwUsb_EndpRead(void*bp,int endpn,void*buf);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_USB_H__ */
