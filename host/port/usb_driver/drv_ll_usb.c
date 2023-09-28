#include <stdio.h>
#include <stddef.h>     // standard definition
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "..\beken_driver\bkreg.h"

#include "..\beken_driver\drv_system.h"
#include "usb\usbdef.h"
// #include "driver_gpio.h"
#include "driver_usb.h"
#include "api_usb.h"

extern void GPIO_USB0_function_enable(void);
void AplUsbHost_BuildDrv();

extern void DelayNops_usb(volatile unsigned long nops);

void print(void * msg, void * dat, int len);


//PTN101x 硅片上gpio与usb（d+、d-）是不同pad，封装时，根据需要从同一pin封装出来
//PTN1012之外芯片d+、d-与gpio0-gpio1复用，gpio0-gpio1的置为高阻态，就是usb d+、d-起作用
void GPIO_USB0_function_enable(void)
{
    REG_GPIO_0x00 = 0x08;//PTN101x DP1
    REG_GPIO_0x01 = 0x08;//PTN101x DN1
}

void ptn1012_enable_usb0()
{
    REG_GPIO_0x06 = 0x08;//PTN1012 DP1
    //PTN1012 DN1 -> dedicate pin#18
}

void ptn1012_enable_usb1()
{
    REG_GPIO_0x12 = 0x08;//PTN1012 DP2
    REG_GPIO_0x07 = 0x08;//PTN1012 DN2
}

static unsigned char bUsbPllInited=0;
void system_usb_pll_init(void)
{
	if(bUsbPllInited)return;
	//power on USBPLL
    REG_SYSTEM_0x40 &= _BIT(0);//0:0=power on USBPLL LDO
    

    REG_SYSTEM_0x48 |= BIT(31);//enable USBPLL clock output to digital

    DelayNops_usb(1);
    REG_SYSTEM_0x48 |= BIT(1);//enable clock to band calibration

    DelayNops_usb(1);
    REG_SYSTEM_0x48 &= _BIT(18);
    DelayNops_usb(1);
    REG_SYSTEM_0x48 |= BIT(18);
    DelayNops_usb(1);
    REG_SYSTEM_0x48 &= _BIT(18);
	bUsbPllInited=1;

}

/*
 *函数名:
 *	usb_mod_enable
 *功能:
 *	usb模块电源、时钟等系统外部输入
 *参数:
 *	1.en_dis:1=使能，0=禁能
 *返回:
 *
 *特殊:
 *
*/
#define IC_TYP_PTN1011				0
#define IC_TYP_PTN1012				1
#define PTN101x_IC_TYP				IC_TYP_PTN1012
#if PTN101x_IC_TYP == IC_TYP_PTN1011
static uint8_t s_bSelUsb0=USBPORT_0;//选择 1011 usb0硬件接口位置gpio0-gpio1
#else
static uint8_t s_bSelUsb0=USBPORT_2;//选择 1012 usb0硬件接口位置
#endif

void AplUsb_SelPort0(int usb_sel){
	s_bSelUsb0=usb_sel;
}

void usb0_mod_enable(int en_dis,int mod){
	if(en_dis){//usb modal enable
		system_usb_pll_init();
		USB_LOG_I("usb_init ---device\r\n");
		REG_SYSTEM_0x40 &= _BIT(4);
		REG_SYSTEM_0x40 |= BIT(2);
		REG_SYSTEM_0x5B &= _BFD(7,11,3);
		if((mod&E_USBMOD_FS_DEV)==E_USBMOD_FS_DEV)REG_SYSTEM_0x5B |= BFD(4,11,3);

		system_peri_mcu_irq_disable(SYS_PERI_IRQ_USB0);
		system_peri_clk_enable(SYS_PERI_CLK_USB0);
		system_mem_clk_enable(SYS_MEM_CLK_USB0);
		system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_USB0);
		REG_SYSTEM_0x01 &= ~(1<<23);//peri cfgpwd,22:APB,23:AHB,24:CEVA,25:RWBT

		if(s_bSelUsb0==USBPORT_0){GPIO_USB0_function_enable();}//非PTN1012芯片，只有USB0，其与uart0[gpio0-gpio1]复用
		else if(s_bSelUsb0==USBPORT_2){ptn1012_enable_usb0();}//PTN1012芯片
		else{//PTN101x必须选择USBPORT_0或者USBPORT_2作为USB0的接口，否则报错
			USB_LOG_I("usb select error\r\n");
			while(1);
		}
	}	else{
		REG_SYSTEM_0x40 |= BIT(4);
		REG_SYSTEM_0x40 &= _BIT(2);
	}
}

void usb0host_mod_enable(int en_dis){	
	usb0_mod_enable(en_dis, E_USBMOD_HOST);
}

void usb0dev_mod_enable(int en_dis){
	usb0_mod_enable(en_dis, E_USBMOD_FS_DEV);
}

void usb1_mod_enable(int en_dis,int mod){
	if(en_dis){//usb modal enable
		system_usb_pll_init();
		USB_LOG_I("usb_init ---host\r\n");
		REG_SYSTEM_0x40 &= _BIT(3);
		REG_SYSTEM_0x40 |= BIT(1);
		REG_SYSTEM_0x5B &= _BFD(7,4,3);
		if((mod&E_USBMOD_FS_DEV)==E_USBMOD_FS_DEV)REG_SYSTEM_0x5B |= BFD(4,4,3);
		
		system_peri_mcu_irq_disable(SYS_PERI_IRQ_USB1);
		system_peri_clk_enable(SYS_PERI_CLK_USB1);
		system_mem_clk_enable(SYS_MEM_CLK_USB1);
		system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_USB1);
		REG_SYSTEM_0x01 &= ~(1<<23);//peri cfgpwd,22:APB,23:AHB,24:CEVA,25:RWBT
		ptn1012_enable_usb1();
	}else{
		//close usb1 otg power
		REG_SYSTEM_0x40 |= BIT(3);
		REG_SYSTEM_0x40 &= _BIT(1);
		//bUsbPllInited=0;
	}
}

void usb1host_mod_enable(int en_dis){
	usb1_mod_enable(en_dis, E_USBMOD_HOST);
}

void usb1dev_mod_enable(int en_dis){
	usb1_mod_enable(en_dis, E_USBMOD_FS_DEV);
}

/*
 *函数名:
 *	usb_mod_ie_enable
 *功能:
 *	usb中断控制
 *参数:
 *	1.en_dis:1=使能，0=禁能
 *返回:
 *
 *特殊:
 *
*/
void usb0_mod_ie_enable(int en_dis){
    if(en_dis){
        system_peri_mcu_irq_enable(SYS_PERI_IRQ_USB0);
    }else{
        system_peri_mcu_irq_disable(SYS_PERI_IRQ_USB0);
    }
}

void usb1_mod_ie_enable(int en_dis){
    if(en_dis){
        system_peri_mcu_irq_enable(SYS_PERI_IRQ_USB1);
    }else{
        system_peri_mcu_irq_disable(SYS_PERI_IRQ_USB1);
    }
}

#if 1
extern void usb_host_init(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk);
static void*s_usbHostBP=NULL;
void*AplUsbHost_GetBP(){
	return((void*)s_usbHostBP);
}
void AplUsbHost_SetBP(void*bp){
	s_usbHostBP=bp;
}

void usbhost_open(int usb_sel){
	AplUsbHost_BuildDrv();
	void*bp;
	void (*mod_en)(int);
	void (*mod_ie)(int);
	if(usb_sel==USBPORT_1){
		bp=(void*)(REG_USB_BASE_ADDR+0x10000);
		mod_en=(void (*)(int))usb1host_mod_enable;
		mod_ie=(void (*)(int))usb1_mod_ie_enable;
	}else{
		s_bSelUsb0=usb_sel;
		bp=(void*)REG_USB_BASE_ADDR;
		mod_en=(void (*)(int))usb0host_mod_enable;
		mod_ie=(void (*)(int))usb0_mod_ie_enable;
	}
	AplUsbHost_SetBP(bp);
	usb_host_init(AplUsbHost_GetBP(),mod_en, mod_ie);
}

#if 1
//extern void HwUsb_SetHostMode(void*bp,int mod);
extern void HwUsb_Host_BusReset(void*bp,int en_dis);
extern void HwUsb_Host_Suspend(void*bp,int en_dis);
extern void HwUsb_Host_Resume(void*bp,int en_dis);
void usbhost_bus_reset(){
	HwUsb_Host_BusReset(AplUsbHost_GetBP(),1);
}

void usbhost_power_on(){
	HwUsb_Host_BusReset(AplUsbHost_GetBP(),0);
}

void usbhost_suspend(){
	HwUsb_Host_Suspend(AplUsbHost_GetBP(),1);
}

void usbhost_resume(){
	HwUsb_Host_Resume(AplUsbHost_GetBP(),1);
}
#endif

#ifdef CONFIG_USB_DMA //add by Borg @230326

/**
 * @param bp base addr of USBx
 * @param DMAx 0:DMA1 / 1:DMA2
 * @param dir 0:tx, mem->usb; 1:rx, mem<-usb;
 * */
void usb_dma_init(void* bp, uint8_t DMAx, uint8_t* buff, uint32_t size,
    uint8_t dir_is_rx, uint8_t cont_mod_en, uint8_t intr_en, uint8_t endp)
{
    USB_DMA_CFG_t* usb_dma = (USB_DMA_CFG_t*)&_REG8(bp, REG_AHB2_USB_DMA_INTR_STUS);
    USB_DMAx_CFG_t* dma = (USB_DMAx_CFG_t*)((DMAx == 0) ? &usb_dma->dma1 : &usb_dma->dma2);

    if(endp < 0 || endp > 4){
        USB_LOG_E("endp:%d\n", endp);
        return;
    }
#if 1
    volatile uint8_t *use_dma_endp = &_REG8(bp, REG_AHB2_USB_DMA_ENDP);
    *use_dma_endp &= ~0xF;
    *use_dma_endp |= endp;
#endif
    USB_DMAx_CFG_t dma_ctx;
    memset(&dma_ctx, 0, sizeof(dma_ctx.cfg));
    dma_ctx.en = 0;
    dma_ctx.dir_is_rx = dir_is_rx;
    dma_ctx.cont_mod_en = cont_mod_en;
    dma_ctx.intr_en = intr_en;
    dma_ctx.endp = endp;
    dma_ctx.max_pkt_sz = _REG8(bp,REG_USB_RXMAXP);//64 / 8;
    dma_ctx.bus_err = 0;
    dma->cfg &= ~0xFFFF;
    dma->cfg |= dma_ctx.cfg;
    dma->addr = (uint32_t)buff;
    dma->size = size;
    // USB_LOG_I(" ----------- cfg %p:%X, %X, endp:%d\n", &dma->cfg, dma->cfg, dma_ctx.cfg, endp);
    // USB_LOG_I(" ----------- addr %p:%d\n", dma->addr, dma->size);
    // USB_LOG_I(" ----------- buff %p:%d\n", buff, size);
    // USB_LOG_I(" ----------- use_dma_endp %p:%d\n", use_dma_endp, *use_dma_endp);
}

USB_DMA_CFG_t* get_usb_dma_ptr(void* bp)
{
    USB_DMA_CFG_t* usb_dma = (USB_DMA_CFG_t*)&_REG8(bp, REG_AHB2_USB_DMA_INTR_STUS);
    return (USB_DMA_CFG_t*)usb_dma;
}
#if 0
/** @param p_usb_dma type USB_DMA_CFG_t
 * @param DMAx 0:DMA1 / 1:DMA2*/
void usb_dma_enable(USB_DMA_CFG_t* usb_dma, uint8_t DMAx, uint8_t en)
{
    (DMAx == 0) ? (usb_dma->dma1.en = en) : (usb_dma->dma2.en = en);
}

/** @param p_usb_dma type USB_DMA_CFG_t
 * @param DMAx 0:DMA1 / 1:DMA2*/
uint8_t usb_dma_intr_stus(USB_DMA_CFG_t* usb_dma, uint8_t DMAx)
{
    return ((DMAx == 0) ? usb_dma->dma1_intr_stus : usb_dma->dma2_intr_stus);
}

/** @param p_usb_dma type USB_DMA_CFG_t
 * @param DMAx 0:DMA1 / 1:DMA2*/
uint8_t usb_dma_bus_err_stus(USB_DMA_CFG_t* usb_dma, uint8_t DMAx)
{
    return ((DMAx == 0) ? usb_dma->dma1.bus_err : usb_dma->dma2.bus_err);
}
#endif

#endif

#endif


