
#ifndef _USB_INCLUDE_H_
#define _USB_INCLUDE_H_

#include <stdint.h>
#include "usbDef.h"
// #define CONFIG_USB_DMA
// -------------------------------------------------------- debug

// #define USB_DEBUG
// #define USBH_DISK_DEBUG
// #define USBH_INTR_DEBUG

#define USB_PRINTF                 os_printf
#define USB_LOG_E(fmt,...)         USB_PRINTF("[U|E:%s():%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define USB_LOG_W(fmt,...)         USB_PRINTF("[U|W:%d]"fmt, __LINE__, ##__VA_ARGS__)
#define USB_LOG_I(fmt,...)         USB_PRINTF("[U|I]"fmt, ##__VA_ARGS__)
#define USB_LOG_D(fmt,...)         //USB_PRINTF("[U|D]"fmt, ##__VA_ARGS__)

#define USBH_DBG_PRT(ptr)		   //USB_PRINTF(#ptr"=%.8x\r\n",ptr)
#define DRV_USB_DBG(ptr)		   //USB_PRINTF(#ptr"=%.8x\r\n",ptr)

extern int32_t os_printf(const char * fmt,...);
void usb_dbg_setup_show(const char* msg, void* setup_pkt, int sz);
void usb_dbg_buf_show(void*msg,void*dat,int len);
void usbdev_print(void*drv);



#ifdef USBH_DISK_DEBUG
    #define USBH_DISK_CODE_ENTER        REG_GPIO_0x20 = 2;
    #define USBH_DISK_CODE_EXIT         REG_GPIO_0x20 = 0;
#else
    #define USBH_DISK_CODE_ENTER
    #define USBH_DISK_CODE_EXIT
#endif

#ifdef USBH_INTR_DEBUG
    #define USB0_ISR_ENTER      REG_GPIO_0x13 = 2;
    #define USB0_ISR_EXIT       REG_GPIO_0x13 = 0;
    #define USB1_ISR_ENTER      REG_GPIO_0x14 = 2;
    #define USB1_ISR_EXIT       REG_GPIO_0x14 = 0;
#else
    #define USB0_ISR_ENTER
    #define USB0_ISR_EXIT 
    #define USB1_ISR_ENTER
    #define USB1_ISR_EXIT
#endif
// --------------------------------------------------------


#define BIT(n)      (1<<(n))
#define _BIT(n)     (~ BIT(n))

#define BFD(val,bs,bl)          (((val)&(BIT(bl)-1))<<(bs))

#define GET_BFD(val,bs,bl)      (((val)&((BIT(bl)-1)<<(bs)))>>(bs))

#define _BFD(val,bs,bl)         (~ BFD(val,bs,bl))

#define GET_ELEMENT_TBL(tbl)	(sizeof(tbl)/(sizeof(tbl[0])))

//BIG ENDIAN to LITTLE ENDIAN
#define _BYTE(n,v)          (((v)>>(8*n))&0xff)
#define B2L_16(bv)          ((_BYTE(0, bv)<<8)|(_BYTE(1, bv)))
#define B2L_32(bv)          ((_BYTE(0, bv)<<24)|(_BYTE(1, bv)<<16)|(_BYTE(2, bv)<<8)|(_BYTE(3, bv)))

#define RW      //可读写
#define RO 	    //只读
#define WO      //只写
#define W1C     //写1清0
#define RC      //读出清0

#define GET_REG_ADDR(x)		(((uint32_t)(&(x))))

// --------------------------------------------------------


extern uint64_t os_get_tick_counter(void);


#endif /* _USB_INCLUDE_H_ */
