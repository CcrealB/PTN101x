/*************************************************************
 * @file		usbclass.c
 * @brief		HW driver of PTN101x Usb(Mentor Graphics)
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2020-12-03
 * @par
 * @attention
 *
 * @history		2020-12-03 jkg	create this file
 */


/*internal symbols import section*/
#include "usbclass.h"
#include "usbdef.h"
#include "usb.h"
#include "..\drv_usb.h"
/*external symbols import section*/

/*variable implement section*/

uint8_t bUsbState ;

//extern uint8_t ep0PktMaxSz;
extern uint8_t GetRqtRecipient(void * setupPkt);
extern void HwUsb_SendStall(void*bp,int endpn);
extern void HwUsb_SendZlp(void*bp,int endpn);
extern void HwUsb_SendPkt(void*bp,uint8_t epn, CEndpoint * endp);
extern void HwUsb_ServicedRxPktRdy(void*bp,int endpn);
extern void SetTxParameter(CEndpoint*endp,void*setupPkt,void*dat,uint32_t sz);
void SetRxParameter(CEndpoint*endp,void*setupPkt,void*dat,uint32_t sz);
/*
 *函数名:
 *	SetRxParameter
 *功能:
 *	设定端点接收参数
 *参数:
 *	1.endp:端点管理器
 *	2.setupPkt:setup包
 *	3.dat:接收缓冲区指针
 *	4.sz:接收缓冲区长度
 *返回:
 *	无
 *特殊:
 *	
*/
#if 0
//static
void SetRxParameter(CEndpoint*endp,void*setupPkt,void*dat,uint32_t sz) {
	UI16*ptr=(UI16*)setupPkt;
	UI16 rlen=ptr[3];
	endp->lmtLen=ep0PktMaxSz;
	endp->rxPtr=dat;
	endp->rxLen=rlen;//sz;
	endp->status|=USB_EndpStt_Rx;
}
#endif
/*
 *函数名:
 *	HalUsbClassRequest
 *功能:
 *	USB标准请求处理函数
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	1.改变:
 *	2.stack:
*/
int __GetMaxLUN(){//make a default maxLUN
	return(0);
}
int GetMaxLUN() __attribute__ ((weak,alias("__GetMaxLUN")));

uint8_t bIntOutBuf[64];
void HalUsbClassRequest(void*bp,void*setupPkt,CEndpoint*endp) {
//	int maxLun;
	uint8_t *ptr=(uint8_t*)setupPkt;
	uint8_t rcp=GetRqtRecipient(setupPkt);
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	CBufferBaseDesc*pdb;
	if((rcp>=USB_RT_RECIPIENT_OTHER)||
		(rcp==USB_RT_RECIPIENT_DEVICE)){
		HwUsb_SendStall(bp,0);
		return;
	}
	if(rcp==USB_RT_RECIPIENT_INTF){
		//检查驱动是否支持class命令
		if(pusb->is_class_cmd){
			//检查是否是驱动支持的class命令
			if(pusb->is_class_cmd(setupPkt,8)){//如果是，判断是ctrl out还是ctrl in
				if(ptr[0]&0x80){//Dev->Host, Get
					if(pusb->get_ctrl_in_db){
						pdb=(CBufferBaseDesc*)pusb->get_ctrl_in_db(setupPkt,8);
						if(pdb){
							SetTxParameter(endp,setupPkt,pdb->ptr,pdb->sz);
							UNS_SET(endp,UNS_DataIn);
							HwUsb_SendPkt(bp,0,endp);
							return;
						}
					}
				}else{//Host->Dev, Set
					if(pusb->get_ctrl_out_db){
						pdb=(CBufferBaseDesc*)pusb->get_ctrl_out_db(setupPkt,8);
						if(pdb){
							SetRxParameter(endp,setupPkt,pdb->ptr,pdb->sz);
							UNS_SET(endp,UNS_DataOut);
                            // HwUsb_SendPkt(bp,0,endp);
                            HwUsb_ServicedRxPktRdy(bp, 0);
							return;
						}
					}
				}
			}
		}
		UNS_SET(endp,UNS_Complete);
		HwUsb_SendStall(bp,0);
		return;
	}
	if(rcp==USB_RT_RECIPIENT_ENDP){
		//检查驱动是否支持class命令
		if(pusb->is_pipe_cmd){
			//检查是否是驱动支持的class命令
			if(pusb->is_pipe_cmd(setupPkt,8)){//如果是，判断是ctrl out还是ctrl in
				if(ptr[0]&0x80){
					if(pusb->get_pipe_in_db){
						pdb=(CBufferBaseDesc*)pusb->get_pipe_in_db(setupPkt,8);
						if(pdb){
							SetTxParameter(endp,setupPkt,pdb->ptr,pdb->sz);
							UNS_SET(endp,UNS_DataIn);
							HwUsb_SendPkt(bp,0,endp);
							return;
						}
					}
				}else{
					if(pusb->get_pipe_out_db){
						pdb=(CBufferBaseDesc*)pusb->get_pipe_out_db(setupPkt,8);
						if(pdb){
							SetRxParameter(endp,setupPkt,pdb->ptr,pdb->sz);
							UNS_SET(endp,UNS_DataOut);
							// HwUsb_SendPkt(bp,0,endp);
                            HwUsb_ServicedRxPktRdy(bp, 0);
							return;
						}
					}
				}
			}
		}
		UNS_SET(endp,UNS_Complete);
		HwUsb_SendStall(bp,0);
		return;
	}

}
/*end file*/

