/*************************************************************
 * @file		driver_usb_v2.c
 * @brief		HW driver of PTN101x Usb(Mentor Graphics)
 * @author		Jiang Kaigan
 * @version		V2.0
 * @date		2022-12-15
 * @par
 * @attention
 *
 * @history		2022-12-15 jkg	create this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver_usb.h"
#include "usb\usbdef.h"
#include "api_usb.h"

//#define _REG8(pbase,reg)		(((volatile unsigned char *)pbase)[((unsigned char*)&reg-(unsigned char*)REG_USB_BASE_ADDR)])

#include "bkreg.h"

#if 1
#include "sys_irq.h"
#define USB_GLOBAL_INT_DISABLE(); \
  do{ \
          uint32_t cpu_flags; \
          SYSirq_Disable_Interrupts_Except(&cpu_flags, (1 << VIC_IDX_CEVA));
#define USB_GLOBAL_INT_RESTORE(); \
          SYSirq_Enable_All_Interrupts(cpu_flags); \
    }while(0);
#else
#define USB_GLOBAL_INT_DISABLE();
#define USB_GLOBAL_INT_RESTORE();
#endif

extern void HalUsbSetupHandle(void*bp,void*setupPkt,CEndpoint *endp);

extern uint8_t bUsbState;
CEndpoint ep0;
CUsbSetupPkt setupPkt;
void usb_device_isr(void*);
void usb_host_isr(void*);
extern void AplUsb_SetConfig(int cfg);
extern void*get_usb_dev_handle();

void HwUsb_SetAddress(void*bp,int addr);
typedef void (*_t_usb_isr)(void*);
static _t_usb_isr s_isr[2]={NULL,NULL};

void DelayNops_usb(volatile unsigned long nops)
{
    while (nops --)
    {
    }
}

void HwUsb_SetRxIE(void*bp, uint16_t ie){
	_REG8(bp, REG_USB_INTRRX1E)=(ie&0xff);
	_REG8(bp, REG_USB_INTRRX2E)=((ie>>8)&0xff);

}

uint32_t HwUsb_GetRxIE(void*bp){
	uint32_t r;
	r=_REG8(bp, REG_USB_INTRRX1E);
	r|=((_REG8(bp, REG_USB_INTRRX2E))<<8);
	return(r);
}
uint32_t HwUsb_GetRxIF(void*bp){
	uint32_t r;
	r=(_REG8(bp, REG_USB_INTRRX1));
	r|=((_REG8(bp, REG_USB_INTRRX2))<<8);
	return(r);
}

void HwUsb_SetTxIE(void*bp, uint16_t ie){
	_REG8(bp, REG_USB_INTRTX1E)=(ie&0xff);
	_REG8(bp, REG_USB_INTRTX2E)=((ie>>8)&0xff);

}
uint32_t HwUsb_GetTxIF(void*bp){
	uint32_t r;
	r=_REG8(bp, REG_USB_INTRTX1);
	r|=(_REG8(bp, REG_USB_INTRTX2))<<8;
	return(r);
}

uint32_t HwUsb_GetTxIE(void*bp){
	uint32_t r;
	r=_REG8(bp, REG_USB_INTRTX1E);
	r|=(_REG8(bp, REG_USB_INTRTX2E))<<8;
	return(r);
}

void HwUsb_Switch2Endp(void*bp,int endpn){
	_REG8(bp, REG_USB_INDEX)=endpn;
}

void HwUsb_EnableTxIE(void*bp,int endpn,int en_dis){
	if(endpn>7){
		if(en_dis)_REG8(bp, REG_USB_INTRTX2E)|=(en_dis<<(endpn-8));
		else _REG8(bp, REG_USB_INTRTX2E)&=~(1<<(endpn-8));
	}else{
		if(en_dis)_REG8(bp, REG_USB_INTRTX1E)|=(en_dis<<endpn);
		else _REG8(bp, REG_USB_INTRTX1E)&=~(1<<endpn);
	}
}
void HwUsb_EnableRxIE(void*bp,int endpn,int en_dis){
	if(endpn>7){
		if(en_dis)_REG8(bp, REG_USB_INTRRX2E)|=(en_dis<<(endpn-8));
		else _REG8(bp, REG_USB_INTRRX2E)&=~(1<<(endpn-8));
	}else{
		if(en_dis)_REG8(bp, REG_USB_INTRRX1E)|=(en_dis<<endpn);
		else _REG8(bp, REG_USB_INTRRX1E)&=~(1<<endpn);
	}
}
uint32_t HwUsb_GetCSR(void*bp){
	uint32_t r;
	int endpn=_REG8(bp, REG_USB_INDEX)&0x0f;
	if(endpn){
		r=_REG8(bp, REG_USB_RXCSR1);
		r|=(_REG8(bp, REG_USB_RXCSR2)<<8);
	}else{
		r=_REG8(bp, REG_USB_CSR0);
		r|=(_REG8(bp, REG_USB_CSR02)<<8);
	}
	return r;
}
void HwUsb_ClrRxStall_CurSel(void*bp){
//	uint32_t r;
	int endpn=_REG8(bp, REG_USB_INDEX)&0x0f;
	if(endpn){
		_REG8(bp, REG_USB_RXCSR1)=0;//_BIT(6);//rxcsr1:[6]=rx stall
	}else{
		_REG8(bp, REG_USB_CSR0)=0;//_BIT(2);//csr0:[2]=rx stall
	}	
}
uint32_t HwUsb_GetRxCount(void*bp){
	uint32_t r;
	r=_REG8(bp, REG_USB_COUNT0);
	r|=(_REG8(bp, REG_USB_RXCOUNT2)<<8);
	return(r);
}
int HwUsbHost_GetTxNakLimit(void*bp){
	uint32_t r;
	r=_REG8(bp, REG_USB_NAKLIMIT0);
	return(r);
}
int HwUsbHost_GetRxNakLimit(void*bp){
	uint32_t r;
	r=_REG8(bp, REG_USB_RXINTERVAL);
	return(r);
}

void HwUsbHost_SetTxNakLimit(void*bp,int lmt){
	_REG8(bp, REG_USB_NAKLIMIT0)=lmt;
}
void HwUsbHost_SetRxNakLimit(void*bp,int lmt){
	_REG8(bp, REG_USB_RXINTERVAL)=lmt;
}

//#define _Read8(_offset) *((volatile uint8_t *)(REG_USB_BASE_ADDR + _offset))
//#define _Read32(_offset) *((volatile uint32_t *)(REG_USB_BASE_ADDR + _offset))
//#define _Write8(_offset, _data) (*((volatile uint8_t *)(REG_USB_BASE_ADDR + _offset)) = _data)
//#define _Write32(_offset, _data) (*((volatile uint32_t *)(REG_USB_BASE_ADDR + _offset)) = _data)

#define _Read8(bp,_offset)			*((volatile uint8_t *)((unsigned char*)bp + _offset))
//_REG8(bp, _offset)
#define _Read32(bp,_offset) 		*((volatile uint32_t *)((unsigned char*)bp + _offset))
#define _Write8(bp,_offset, _data) (*((volatile uint8_t *)((unsigned char*)bp + _offset)) = _data)
#define _Write32(bp,_offset, _data) (*((volatile uint32_t *)((unsigned char*)bp + _offset)) = _data)

uint32_t HwUsb_EndpRead(void*bp,int endpn,void*buf){
	uint32_t cnt;
	uint32_t val;
	uint8_t*p8=(uint8_t*)buf;
	HwUsb_Switch2Endp(bp,endpn);
	cnt=HwUsb_GetRxCount(bp);
    uint32_t dwIndex, dwIndex32;
    uint32_t dwCount32 = ((uint32_t)buf & 3) ? 0 : (cnt >> 2);
    uint8_t bFifoOffset = 0x20+endpn*4;
//	dwIndex=0;
	for (dwIndex = dwIndex32 = 0; dwIndex32 < dwCount32; dwIndex32++, dwIndex += 4){
		val=_Read32(bp,bFifoOffset);
		memcpy(&p8[dwIndex],&val,4);
	}
//	if(cnt&0x03){
//		val=_Read32(bp,bFifoOffset);
//		memcpy(&p8[dwIndex],&val,cnt&0x03);
//	}
	while (dwIndex < cnt){
		p8[dwIndex++] = _Read8(bp,bFifoOffset);
	}
	if(endpn){
 		_REG8(bp, REG_USB_RXCSR1)=0;
//		REG_USB_RXCSR1|=BIT(4);//PTN101x的usb fifo应该是有2级，如果进行flush fifo，那么所有fifo已接收到的信息都被清除
	}else{
		_REG8(bp, REG_USB_CSR0)=0;
	}
	return(cnt);
}

void HwUsb_EndpWrite(void*bp,int endpn, const void*buf,int len){
    uint32_t dwIndex, dwIndex32;
    uint32_t dwCount32 = ((uint32_t)buf& 3) ? 0 : (len >> 2);
    uint8_t bFifoOffset = 0x20+endpn*4;//MGC_FIFO_OFFSET(bEnd);
    uint8_t *p8=(uint8_t*)buf;
    uint32_t val;
    p8=(uint8_t*)buf;//samp;
	//USB_LOG_I("ep%d wr\r\n",endpn);
	/* doublewords when possible */
	for (dwIndex = dwIndex32 = 0; dwIndex32 < dwCount32; dwIndex32++, dwIndex += 4){
		memcpy(&val,&p8[dwIndex],4);
		_Write32(bp,bFifoOffset, val);
	}
	while (dwIndex < len){
		_Write8(bp,bFifoOffset, p8[dwIndex++]);
	}
}

void HwUsb_SendStall(void*bp,int endpn){
	HwUsb_Switch2Endp(bp,endpn);
	_REG8(bp, REG_USB_CSR0)=BIT(5);
}

void HwUsb_SendZlp(void*bp,int endpn){
	HwUsb_Switch2Endp(bp,endpn);
	_REG8(bp, REG_USB_CSR02)=BIT(0);
	_REG8(bp, REG_USB_CSR0)=BIT(1)|BIT(3);
}

void HwUsb_ServicedSetupEnd(void*bp,int endpn){
	HwUsb_Switch2Endp(bp,endpn);
	_REG8(bp, REG_USB_CSR02)=BIT(0);
	_REG8(bp, REG_USB_CSR0)=BIT(7);
}

void HwUsb_ServicedRxPktRdy(void*bp,int endpn){
	if(endpn){
		HwUsb_Switch2Endp(bp,endpn);
		_REG8(bp, REG_USB_RXCSR1)=BIT(0);
	}else{
		_REG8(bp, REG_USB_CSR02)=BIT(0);
		_REG8(bp, REG_USB_CSR0)=BIT(6);
	}
}

void HwUsb_FlushFifo(void*bp,int endpn){
	HwUsb_Switch2Endp(bp,endpn);
	if(endpn==0)_REG8(bp, REG_USB_CSR02)=BIT(0);
	else{
		_REG8(bp, REG_USB_CSR0)=BIT(3);//TXCSR1
	}
}
void HwUsbHost_SendStatusOut(void*bp){
	HwUsb_Switch2Endp(bp,0);
//	REG_USB_CSR02=BIT(0);
	_REG8(bp, REG_USB_CSR0)=BIT(1)|BIT(6);//csr0:[1]=txpktrdy,[6]=status pkt
}
/*
* 函数名:
*	HwUdpSendPkt
* 功能:
*	udp端点发送包
* 参数:
*	1.udp:usb设备指针
*	2.epn:端点号
*	3.endp:端点管理器
* 输出:
*	无
* 返回:
*	无
*/
void HwUsb_SendPkt(void*bp, uint8_t epn, CEndpoint*endp)  {
	uint8_t* ptr;
	uint16_t len;
	uint16_t sta0;
	uint8_t* buf=NULL;
	if(endp->status&USB_EndpStt_Tx){
		if(endp->txLen==0){
			endp->status^=USB_EndpStt_Tx;
			HwUsb_SendZlp(bp,epn);
			UNS_SET(endp,UNS_StatusOut);
		}else{
			ptr=(uint8_t*)endp->txPtr;
			sta0=endp->status;
			endp->status &=~USB_EndpStt_TxExCfg;
			if(endp->txLen<endp->lmtLen){
				len=endp->txLen;
				endp->txLen=0;
				UNS_SET(endp,UNS_StatusOut);
			}else{
				len=endp->lmtLen;
				endp->txLen-= len;
				UNS_SET(endp,UNS_DataIn);
			}
			endp->txPtr+=len;
			///////////////////////////////////////////////////////////////
			//解决在手机上列举失败及断线问题
			HwUsb_FlushFifo(bp,epn);
			//if(epn==0)REG_USB_CSR02=BIT(0);//endp 0
			//else REG_USB_CSR0=BIT(3);//non-0 endp
			//
			///////////////////////////////////////////////////////////////
			buf=(uint8_t*)malloc(len);
			memcpy(buf,ptr,len);
			if(sta0&USB_EndpStt_TxExCfg){
				//修改自动计算配置描述符长度，只从status标志判断，使用txCursor作为临时变量保存其长度
				//另外使用动态缓存作为usb直接使用的缓冲区，而非之前直接发送表格数据
				memcpy(&buf[2],&endp->txCursor,2);
				endp->txCursor=len;
			}else{
				endp->txCursor+=len;
			}
			HwUsb_EndpWrite(bp,epn, buf, len);
			free(buf);
			//HwUsb_EndpWrite(epn, ptr, len);
			if(epn==0){
				if(endp->txLen)_REG8(bp, REG_USB_CSR0)=BIT(1);
				else
					_REG8(bp, REG_USB_CSR0)=BIT(1)|BIT(3);//csr0：[1]=txPktRdy,[3]=dataEnd				
			}
		}
	}
}

void AplUsb_StartTx(void*bp,int epn,void*buf,int len){//epn!=0
	HwUsb_Switch2Endp(bp,epn);
	_REG8(bp, REG_USB_CSR0)=BIT(3);//flush fifo
	if(len)HwUsb_EndpWrite(bp,epn, buf, len);
	_REG8(bp, REG_USB_CSR0)=BIT(0);//csr1:TxPktRdy
 	_REG8(bp, REG_USB_CSR02)|=BIT(5);//set endp to tx mode
}

/*
* 函数名:
*	AplUsbHost_StartTx
* 功能:
*	host端点发送包
* 参数:
*	1.epn:端点号
*	2.setupPkt:=NULL,发送buf数据；!=NULL(buf=NULL),发送(setupPkt,len)
*	3.buf,len:数据参数，buf==NULL =>statusPkt;buf!=NULL => 发送(buf,len)
* 输出:
*	无
* 返回:
*	无
*/
/*AplUsbHost_StartTx */

void HwUsbHost_EndpWrite(void*bp,int epn, void*setupPkt,void*buf,int len){
	HwUsb_Switch2Endp(bp,epn);
	if(epn){
		_REG8(bp, REG_USB_CSR02)|=BIT(5);//csr1:[0]=txPktRdy;
		HwUsb_EndpWrite(bp,epn, buf, len);
		_REG8(bp, REG_USB_CSR0)=BIT(0);//csr1:[0]=txPktRdy;
	}else{
		if(setupPkt){
			HwUsb_EndpWrite(bp,epn, setupPkt, 8);
			_REG8(bp, REG_USB_CSR0)=BIT(3)|BIT(1);//csr0:[3]=setupPkt,[1]=txPktRdy,[6]=statusPkt
		}else{
			if(buf){
				HwUsb_EndpWrite(bp,epn, buf, len);
				_REG8(bp, REG_USB_CSR0)=BIT(1);//csr0:[3]=setupPkt,[1]=txPktRdy,[6]=statusPkt
			}else{//statusPkt
				_REG8(bp, REG_USB_CSR0)=BIT(6)|BIT(1);//csr0:[3]=setupPkt,[1]=txPktRdy,[6]=statusPkt
			}
		}
	}
}

void AplUsbHost_StartRx(void*bp,int epn){
	HwUsb_Switch2Endp(bp,epn);
	if(epn){
		//_REG8(bp, REG_USB_RXCSR1)|=BIT(4);
		_REG8(bp, REG_USB_RXCSR1)=BIT(5);
	}else {
		_REG8(bp, REG_USB_CSR0)=BIT(5);
	}
}
void AplUsbHost_StopRx(void*bp,int epn){
	HwUsb_Switch2Endp(bp,epn);
	if(epn){
		_REG8(bp, REG_USB_RXCSR1)=0;
	}else {
		_REG8(bp, REG_USB_CSR0)=0;
	}
}

void HwUsb_SetAddress(void*bp,int addr){
	_REG8(bp, REG_USB_FADDR)=addr&0x7f;
}
/////////////////////////////////////////////////////////////
//REG_AHB2_USB_OTG_CFG:
//	[3]:d+ PU;[4]:d+ PD;[5]:d- PU;[6]:d- PD
//	[0]:1=device,0=host;[1]:test mode;[2]:otg mode has ID detect-pin;
void HwUsb_SetDeviceMode(void*bp,int fs_ls/*1=FS,0=LS*/){//in device mode:Full Speed=d+ pullup,Low Speed=D- pullup
	_REG8(bp, REG_AHB2_USB_OTG_CFG )&= _BFD(0xf, 3, 4);		// 
	if(fs_ls==0)_REG8(bp, REG_AHB2_USB_OTG_CFG )|= (BIT(3));		// d+ pullup enable	
	else _REG8(bp, REG_AHB2_USB_OTG_CFG )|= (BIT(5));		// d- pullup enable
	_REG8(bp, REG_AHB2_USB_OTG_CFG )|= BIT(0);		// device mode
	//REG_USB_DEVCTL=0;
}

void HwUsb_SetMode(void*bp,int mod/*0=host,1=FS-device,3=LS-device*/){//in host mode:d+/d- should be work in pulldown mode(USB SPEC:15Kohm)
	if((mod&1)==0){
		_REG8(bp, REG_AHB2_USB_OTG_CFG) &= _BFD(0xf, 3, 4);		// 
		_REG8(bp, REG_AHB2_USB_OTG_CFG) |= (BIT(4)|BIT(6));		// d+\d- pulldown enable	
		_REG8(bp, REG_AHB2_USB_OTG_CFG) &= 0xfe;		// host mode
	}else{
		HwUsb_SetDeviceMode(bp,mod&2/* 0=FS,!0=LS*/);
	}
}
void HwUsb_SetHostMode(void*bp,int mod/*0=host,1=FS-device,3=LS-device*/){//in host mode:d+/d- should be work in pulldown mode(USB SPEC:15Kohm)
	_REG8(bp, REG_AHB2_USB_OTG_CFG) &= _BFD(0xf, 3, 4);		// 
	_REG8(bp, REG_AHB2_USB_OTG_CFG) |= (BIT(4)|BIT(6));		// d+\d- pulldown enable	
	_REG8(bp, REG_AHB2_USB_OTG_CFG) &= 0xfe;		// host mode
}


void HwUsb_Host_BusReset(void*bp,int en_dis){//moduler should work as HOST
	if(en_dis){
		_REG8(bp, REG_USB_POWER)|=BIT(3);
	}else{
		_REG8(bp, REG_USB_POWER)&=_BIT(3);
	}
}

void HwUsb_Host_Suspend(void*bp,int en_dis){//moduler should work as HOST
	if(en_dis){
		_REG8(bp, REG_USB_POWER)|=BIT(1);
	}else{
		_REG8(bp, REG_USB_POWER)&=_BIT(1);
	}
}

void HwUsb_Host_Resume(void*bp,int en_dis){//moduler should work as HOST
	if(en_dis){
		_REG8(bp, REG_USB_POWER)|=BIT(2);
	}else{
		_REG8(bp, REG_USB_POWER)&=_BIT(2);
	}
}

extern void HwUsbOpen(void);
void usb_init_sw(){
	memset(&ep0,0,sizeof(ep0));
	memset(&setupPkt,0,sizeof(setupPkt));
	SET_USBSTATE(USB_STATE_INIT);
	HwUsbOpen();
}
void usb_init_ex_v2(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk,void*oth_cbk,int mod)
{
	volatile unsigned char *pbase=(volatile unsigned char *)bp;
	unsigned char ucUSBIntStatus;
	void (*ctrl_cbk)(int);
	void (*ie_cbk)(int);
	void (*oth_fun)();
	
	ctrl_cbk=(void (*)(int))usb_mod_ctrl_cbk;
	ie_cbk=(void (*)(int))usb_mod_int_ie_cbk;
	oth_fun=(void (*)())oth_cbk;
//	EnterFunc();
	if(ctrl_cbk)ctrl_cbk(1);//usb modal enable，使能USB模块

	_REG8(pbase, REG_USB_INTRUSBE) = 0x0;
	HwUsb_SetRxIE(bp,0x0000);//default 0x001e
	HwUsb_SetTxIE(bp,0x0000);//default 0x001f
	_REG8(pbase, REG_AHB2_USB_VTH) &= _BIT(7);//disable interrupt of VBUS changed

	HwUsb_SetRxIE(bp,0x01);
	HwUsb_SetTxIE(bp,0x01);

	_REG8(pbase, REG_USB_INTRUSBE) |= 0x3F;//set USB bus interrupt
	_REG8(pbase, REG_AHB2_USB_DMA_ENDP)=0;
//	REG_AHB2_USB_OTG_CFG = 0x08;        // dp pull up
	HwUsb_SetMode(bp,mod);
//	HwUsb_SetHostMode(1);
	_REG8(pbase, REG_AHB2_USB_DEV_CFG) = 0xF4;//select controller
//	REG_AHB2_USB_OTG_CFG |= 0x01;       // device

	DelayNops_usb(500);

	//clear VBUS-interrupt
	ucUSBIntStatus = _REG8(pbase, REG_AHB2_USB_INT);
	DelayNops_usb(500);
	_REG8(pbase, REG_AHB2_USB_INT) = ucUSBIntStatus;
	DelayNops_usb(500);

	_REG8(pbase, REG_AHB2_USB_GEN)=BFD(7,4,4)|BFD(7,0,4);	//set D+\D-drive current


	_REG8(pbase, REG_USB_FADDR)=0;
	_REG8(pbase, REG_USB_DEVCTL)=BIT(0);//start device 'B'
	if(mod==E_USBMOD_HOST){
		if(bp==(void*)REG_USB_BASE_ADDR){//是usb0
				s_isr[0]=usb_host_isr;
			}
		if(bp==(void*)(REG_USB_BASE_ADDR+0x10000)){//是usb1
				s_isr[1]=usb_host_isr;
			}
	}else {
		if(bp==(void*)REG_USB_BASE_ADDR){//是usb0
				s_isr[0]=usb_device_isr;
			}
		if(bp==(void*)(REG_USB_BASE_ADDR+0x10000)){//是usb1
				s_isr[1]=usb_device_isr;
			}
	}
	if(oth_fun)oth_fun();
	if(ie_cbk)ie_cbk(1);//使能usb中断
//	ExitFunc();
}

#if 0
#else
void usb_init(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk)
{
	usb_init_ex_v2(bp,usb_mod_ctrl_cbk, usb_mod_int_ie_cbk, usb_init_sw, E_USBMOD_FS_DEV);
}
#endif


void usb_host_init(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk){
	usb_init_ex_v2(bp,usb_mod_ctrl_cbk, usb_mod_int_ie_cbk, NULL, E_USBMOD_HOST);
}

void usb_deinit(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk){
	void (*ctrl_cbk)(int);
	void (*ie_cbk)(int);
	ctrl_cbk=(void (*)(int))usb_mod_ctrl_cbk;
	ie_cbk=(void (*)(int))usb_mod_int_ie_cbk;
	_REG8(bp,REG_USB_INTRRX1E) = 0x0;
	_REG8(bp,REG_USB_INTRTX1E) = 0x0;
	_REG8(bp,REG_USB_INTRUSBE) = 0x0;
	ie_cbk(0);
	_REG8(bp,REG_AHB2_USB_GEN) = 0;
	_REG8(bp,REG_AHB2_USB_OTG_CFG) = 0;
	ctrl_cbk(0);
}

int AplUsb_GetSOF(void*bp){
	return(_REG8(bp,REG_USB_FRAME1)+(_REG8(bp,REG_USB_FRAME2)<<8));
}

uint16_t usbd_frame_num_get(uint8_t USBx)
{
    void* bp = (void*)(REG_USB_BASE_ADDR + (USBx ? 0x10000 : 0));
	return(_REG8(bp, REG_USB_FRAME1) + (_REG8(bp, REG_USB_FRAME2) << 8));
}

uint8_t bUsbAddress;
#include "usb\usb_cls_audio.h"
int uac_set_feature_unit(USBD_SetupReqTypedef *setup, uint8_t *pbuf, int size);

//out data after setup cmd
void ep0_rx_data_proc(void *bp, USBD_SetupReqTypedef *setup, uint8_t *pbuf, int size)
{
    // usb_dbg_buf_show("setup rx", setup, cnt);
    // USB_LOG_I("@\\t%X,%d\n", pbuf ,size);
    switch(setup->bRequest)
    {
        case AUDIO_RC_SET_CUR:
        case AUDIO_RC_SET_MIN:		
        case AUDIO_RC_SET_MAX:		
        case AUDIO_RC_SET_RES: uac_set_feature_unit(setup, pbuf, size); break;
        // case HID_RQT_Set_Report: hid_set_report(); break;
        // case HID_RQT_Set_IDLE: hid_set_idle(); break;
        // case HID_RQT_Set_Protocol: hid_set_protocol(); break;
        default: break;
    }

    memset(setup, 0, sizeof(USBD_SetupReqTypedef));
}

void _ep0_isr(void*bp){
	int csr,cnt;
	char buf[0x40];
	// uint16_t setup[4];
    static USBD_SetupReqTypedef s_setup_pkg;
    USBD_SetupReqTypedef* setup = &s_setup_pkg;
	HwUsb_Switch2Endp(bp,0);
	csr=HwUsb_GetCSR(bp);

	if(csr&BIT(0)){//RxPacket Ready
		if(UNS_GET(&ep0)==UNS_DataOut){
			cnt=HwUsb_EndpRead(bp,0, buf);
			if(ep0.rxLen<=cnt){
				memcpy(ep0.rxPtr,buf,ep0.rxLen);
                ep0_rx_data_proc(bp, setup, ep0.rxPtr, ep0.rxLen);
				ep0.rxLen=0;
				HwUsb_SendZlp(bp,0);
                UNS_SET(&ep0, UNS_Complete);
			}else{
				memcpy(ep0.rxPtr,buf,cnt);
				ep0.rxLen-=cnt;
			}
		}else{
			cnt=HwUsb_EndpRead(bp,0, buf);
			if(cnt>0){
				memcpy(setup,buf,cnt);
				usb_dbg_buf_show("setupPkt", setup, cnt);
				HalUsbSetupHandle(bp,setup,&ep0);
			}else{
				HwUsb_SendPkt(bp,0, &ep0);
			}
		}
	}else if(csr&BIT(4)){//Setup End
        // USB_LOG_I("%d SetupEnd\n", __LINE__);
		_REG8(bp,REG_USB_CSR0)=BIT(7);
		UNS_SET(&ep0, UNS_Complete);
	}else{
		if(UNS_GET(&ep0)==UNS_DataIn){
			HwUsb_SendPkt(bp,0, &ep0);
		}else{
			if(bUsbState==USB_STATE_ADDRESS){
				HwUsb_SetAddress(bp,bUsbAddress);
				SET_USBSTATE(USB_STATE_ADDRESSED);
			}
			UNS_SET(&ep0, UNS_Complete);
		}
	}
}

void _endp_tx(void*bp,int endpn){
	char*txbuf;
	HwUsb_Switch2Endp(bp,endpn);
	HwUsb_GetCSR(bp);
	txbuf=(char*)AplUsb_GetTxBuf(endpn);
	CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetTxCbk(endpn);
	if(cbk){
		cbk(txbuf,0);//AplUsb_GetBufSz(endpn));
	}
}

void _endp_rx(void*bp,int endpn){
	char*rxBuf;
	int sz;
	rxBuf=(char*)AplUsb_GetRxBuf(endpn);
	sz=HwUsb_EndpRead(bp,endpn, rxBuf);

	CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetRxCbk(endpn);
	if(cbk){
		cbk(rxBuf,sz);
	}
}
void _endp_rx_v1(void*bp,int endpn){
	char*rxBuf;
	int sz;
	rxBuf=(char*)AplUsb_GetRxBuf(endpn);
	CBK_USBAPP_R cbk_r=(CBK_USBAPP_R)AplUsb_GetRxPreCbk(endpn);
	if(cbk_r){
		if(cbk_r(rxBuf,0))return;
	}
	HwUsb_Switch2Endp(bp,endpn);
	volatile uint8_t bcsr1=_REG8(bp,REG_USB_RXCSR1);
	(void)bcsr1;
	sz=HwUsb_EndpRead(bp,endpn, rxBuf);

	CBK_USBAPP cbk=(CBK_USBAPP)AplUsb_GetRxCbk(endpn);
	if(cbk){
		cbk(rxBuf,sz);
	}
}
static CBK_USBAPP sofCbk=NULL;

void AplUsb_SetSOFCbk(void*cbk){
	sofCbk=(CBK_USBAPP)cbk;
}
void*AplUsb_GetSOFCbk(){
	return (void*)sofCbk;
}

void USB_IsrDispatch(void*bp,int rxIF,int txIF,int usbIF){
	int i;
	if((txIF&BIT(0))||(rxIF&BIT(0))){
		_ep0_isr(bp);
	}
    USB_GLOBAL_INT_DISABLE();
	for(i=1;i</*16*/MAX_ENDPOINT;i++){

		if(txIF&BIT(i)){
			//USB_LOG_I("55\r\n");
			_endp_tx(bp,i);
			//USB_LOG_I("66\r\n");
		}
		if(rxIF&BIT(i)){
			//USB_LOG_I("77\r\n");
			_endp_rx_v1(bp,i);
			//USB_LOG_I("88\r\n");
			////rx packet ready
			//HwUsb_Switch2Endp(i);
			//uint8_t bcsr1=REG_USB_RXCSR1;
			//if(bcsr1&BIT(0)){
			//	_endp_rx(i);
			//}
		}
	}
    USB_GLOBAL_INT_RESTORE();
	if(usbIF){
		if(usbIF&BIT(0)){//suspend
		    USB_LOG_I("suspend\r\n");
			_REG8(bp,REG_USB_POWER)=1;//enable suspend
			_REG8(bp,REG_USB_DEVCTL)=0;//stop PHY CLKOUT,save power,soft disconnect from host
			extern CBK_USB_SUSPEND cbk_suspend;
			if(cbk_suspend)cbk_suspend();
		}
		if(usbIF&BIT(1)){//resume
			USB_LOG_I("resume\r\n");
			_REG8(bp,REG_USB_POWER)=0;//end suspend
			_REG8(bp,REG_USB_DEVCTL)=1;//start device
		}

		if(usbIF&BIT(2)){//bus reset
			_REG8(bp,REG_USB_FADDR)=0;
			//usb_init_sw();
//			extern volatile uint8_t bUsbConfig;
//			bUsbConfig=0;
			AplUsb_SetConfig(0);
			USB_LOG_I("bus reset\r\n");
			usbdev_print(get_usb_dev_handle());
		}
		if(usbIF&BIT(3)){//sof
			//USB_LOG_I("sof\r\n");
			if(sofCbk)sofCbk(NULL,0);
		}
//		if(usbIF&BIT(4)){//usb peripheral connected,act as HOST
//			USB_LOG_I("usb connected\r\n");
//		}
//		if(usbIF&BIT(5)){//usb peripheral disconnected,act as HOST
//			USB_LOG_I("usb disconnected\r\n");
//		}
	}
}
void usb_device_isr(void*bp){
	uint8_t bIntrUsbValue = _REG8(bp,REG_USB_INTRUSB);
	uint8_t old_index=_REG8(bp,REG_USB_INDEX);
	uint16_t wIntrTxValue ;
	uint16_t wIntrRxValue ;
	wIntrTxValue=HwUsb_GetTxIF(bp);
	wIntrRxValue=HwUsb_GetRxIF(bp);
	HwUsb_Switch2Endp(bp,0);
	if(_REG8(bp,REG_USB_CSR0)&(BIT(4)|BIT(0)))wIntrRxValue|=BIT(0);
//	USB_LOG_I("usbIf=%.2x,txif=%.4x,rxif=%.4x\r\n", bIntrUsbValue,wIntrTxValue,wIntrRxValue);
	USB_IsrDispatch(bp,wIntrRxValue, wIntrTxValue, bIntrUsbValue);
	_REG8(bp,REG_USB_INDEX)=old_index;
}
extern void usb_host_proc(void*bp,int rxIF, int txIF, int usbIF);
void usb_host_isr(void*bp){
	uint8_t bIntrUsbValue = _REG8(bp,REG_USB_INTRUSB);
	uint8_t old_index=_REG8(bp,REG_USB_INDEX);
	uint16_t wIntrTxValue ;
	uint16_t wIntrRxValue ;
	wIntrTxValue=HwUsb_GetTxIF(bp);
	wIntrRxValue=HwUsb_GetRxIF(bp);
	HwUsb_Switch2Endp(bp,0);
	if(_REG8(bp,REG_USB_CSR0)&BIT(0))wIntrRxValue|=BIT(0);
	//USB_LOG_I("h:usbIf=%.2x,txif=%.4x,rxif=%.4x\r\n", bIntrUsbValue,wIntrTxValue,wIntrRxValue);
	usb_host_proc(bp,wIntrRxValue, wIntrTxValue, bIntrUsbValue);
	_REG8(bp,REG_USB_INDEX)=old_index;
}

void usb0_handler(void)
{
    USB0_ISR_ENTER
    if(s_isr[0])s_isr[0]((void*)REG_USB_BASE_ADDR);//usb_device_isr/usb_host_isr
    USB0_ISR_EXIT
}

void usb1_handler(void)
{
    USB1_ISR_ENTER
    if(s_isr[1])s_isr[1]((void*)(REG_USB_BASE_ADDR+0x10000));//usb_device_isr/usb_host_isr
    USB1_ISR_EXIT
}


