/*************************************************************
 * @file		usb.c
 * @brief		main protocol of USB spec1.1 chapter 9
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2020-12-03
 * @par
 * @attention
 *
 * @history		2020-12-03 jkg	create this file
 */

/*file type declaration section*/
#define _C_SRC_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*internal symbols import section*/
#include "usbclass.h"
#include "usb.h"
#include "usbdef.h"
#include "..\driver_usb.h"
#include "..\drv_usb.h"
/*external symbols import section*/
extern void HwUsb_SendZlp(void*bp,int endpn);
extern void HwUsb_SendStall(void*bp,int endpn);
extern void HwUsb_Switch2Endp(void*bp,int endpn);
//extern void HwUsb_SetAddress(void*bp,int addr);
//extern void HwUsb_EndpWrite(void*bp,int endpn, void * buf, int len);
extern void HwUsb_SendPkt(void*bp,uint8_t epn, CEndpoint * endp);
extern void HalUsbClassRequest(void*bp,void * setupPkt, CEndpoint * endp);

extern void*AplUsbDev_GetDrv();
void* _find_HidClassRpt(void*desc,int sz,int intfno);
void AplUsb_SetConfig(int cfg);
int _config_endp(void*bp,void*desc,int sz,int myDesc);
int32_t os_printf(const char *fmt, ...);

int AplUsb_GetIntfCount();
/*variable implement section*/
/////////////////////////////////////////////////////////////////////////////////////////
//interface and endpoint should be arranged in order
extern uint8_t bUsbState;
/*
 *函数名:
 *	HalUsbAttibuteInit
 *功能:
 *	初始化USB设备的特性
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	1.改变:
 *	2.stack:
*/
void HalUsbAttributeInit(){
//	memset(bUsbAlterInterface,0,sizeof(bUsbAlterInterface));
}

/*
 *函数名:
 *	HwUsbOpen
 *功能:
 *	打开USB
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	1.改变:
 *	2.stack:
*/
void HwUsbOpen(){
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	CBufferBaseDesc*pdesc;
	os_printf("usb dr=%.8x f_get_device_desc=%.8x\r\n",pusb,pusb->f_get_device_desc);
	pdesc=(CBufferBaseDesc*)pusb->f_get_device_desc();
	HalUsbAttributeInit();
	((_t_usb_dev_drv*)AplUsbDev_GetDrv())->ep0PktMaxSz=((CUsbDevDesc*)pdesc->ptr)->maxPkt;
}

typedef void (*CFunPtr)(void*bp,void*setupPkt,CEndpoint*endp);

/*
 *函数名:
 *	URB_NOT_SUPPORT
 *功能:
 *	不支持的URB
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.改变:
 *	2.stack:
*/
void URB_NOT_SUPPORT(void*bp,void*setupPkt,CEndpoint*endp)
{//send stall
	HwUsb_SendStall(bp,0);
	UNS_SET(endp,UNS_Complete);
}


/*
 *函数名:
 *	SetRxParameter
 *功能:
 *	设定端点发送参数
 *参数:
 *	1.endp:端点管理器
 *	2.setupPkt:setup包
 *	3.dat:发送缓冲区指针
 *	4.sz:发送数据长度
 *返回:
 *	无
 *特殊:
 *	无
*/
void SetTxParameter(CEndpoint*endp,void*setupPkt,void*dat, uint32_t sz)  {
	uint16_t*ptr=(uint16_t*)setupPkt;
	uint16_t rlen=ptr[3];
	uint16_t len0=sz;//&0x7fffffff;
	endp->lmtLen=((_t_usb_dev_drv*)AplUsbDev_GetDrv())->ep0PktMaxSz;
	endp->txPtr=dat;
	if(len0>rlen){
		endp->txLen=rlen;
	}else{
		endp->txLen=sz;
	}
	endp->txCursor=0;
	if(endp->status&USB_EndpStt_TxExCfg)endp->txCursor=sz;
	endp->status|=USB_EndpStt_Tx;
//	if(sz&0x80000000)endp->status|=USB_EndpStt_TxExCfg;
}

void SetRxParameter(CEndpoint*endp,void*setupPkt,void*dat,uint32_t sz) {
	uint16_t*ptr=(uint16_t*)setupPkt;
	uint16_t rlen=ptr[3];
	endp->lmtLen=((_t_usb_dev_drv*)AplUsbDev_GetDrv())->ep0PktMaxSz;
	endp->rxPtr=dat;
	endp->rxLen=rlen;//sz;
	endp->status|=USB_EndpStt_Rx;
}

//取请求的接收端,usb标准定义3种标准接收端:
//	0.设备,device;1.接口,interface;2.端点,endpoint
uint8_t GetRqtRecipient(void*setupPkt)  {
	uint8_t*ptr=(uint8_t*)setupPkt;
	uint8_t rcp=ptr[0];
	return(GET_BFD(rcp,0,5));
}

//取请求的类型,usb标准定义3种类型:
//	0.标准,standard;1.类,class;2.厂商自订,vendor
uint8_t GetRqtType(void*setupPkt)  {
	uint8_t*ptr=(uint8_t*)setupPkt;
	uint8_t rcp=ptr[0];
	return(GET_BFD(rcp,5,2));
}

/*
 *函数名:
 *	URB_GET_STATUS
 *功能:
 *	GET_STATUS
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
uint16_t usb_status[3]={
	(0<<_SELF_POWER_)|(1<<_REMOTE_WAKEUP_),
	0x0000,
	0x0000,
};

void URB_GET_STATUS(void*bp,void*setupPkt,CEndpoint*endp){
	int rcp=GetRqtRecipient(setupPkt);
	CUsbSetupPkt*psetup=(CUsbSetupPkt*)setupPkt;
	char*ptr=NULL;
	switch(rcp){
		case USB_RT_RECIPIENT_DEVICE:
			ptr=(char*)&usb_status[0];
			break;
		case USB_RT_RECIPIENT_INTF:
			ptr=(char*)&usb_status[1];
			break;
		case USB_RT_RECIPIENT_ENDP:
			ptr=(char*)&((_t_usb_dev_drv*)AplUsbDev_GetDrv())->usb_endp_status[psetup->idx&0x0f];
			break;
	}
	if(ptr){
		SetTxParameter(endp,setupPkt,ptr,2);
		UNS_SET(endp,UNS_DataIn);
		HwUsb_SendPkt(bp,0,endp);
	}else{
		UNS_SET(endp,UNS_Complete);
		HwUsb_SendStall(bp,0);
	}
}

/*
 *函数名:
 *	URB_CLEAR_FEATURE
 *功能:
 *	CLEAR_FEATURE
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void URB_CLEAR_FEATURE(void*bp,void*setupPkt,CEndpoint*endp){
// 	int rcp=GetRqtRecipient(setupPkt);
// 	CUsbSetupPkt*psetup=(CUsbSetupPkt*)setupPkt;
// 	if(rcp==USB_RT_RECIPIENT_ENDP){
// 		rcp=psetup->pkt.idx;
// 		HwUsb_Switch2Endp(rcp&0x0f);
// 		REG_USB_CSR0=0;
// 	}
// 	HwUsb_SendZlp(0);
// 	UNS_SET(endp,UNS_Complete);

	int rcp=GetRqtRecipient(setupPkt);
	CUsbSetupPkt*psetup=(CUsbSetupPkt*)setupPkt;
	char*ptr=NULL;
	switch(rcp){
		case USB_RT_RECIPIENT_DEVICE:
			ptr=(char*)&usb_status[0];
			if(psetup->tar==1){//DEVICE_REMOTE_WAKEUP
				ptr[0]&=_BIT(_REMOTE_WAKEUP_);
			}
			break;
		case USB_RT_RECIPIENT_INTF:
			ptr=(char*)&usb_status[1];
			break;
		case USB_RT_RECIPIENT_ENDP:
			ptr=(char*)&((_t_usb_dev_drv*)AplUsbDev_GetDrv())->usb_endp_status[psetup->idx&0x0f];
			if(psetup->tar==0){
				ptr[0]&=_BIT(0);
			}
			//os_printf("CLEAR endp%d FEATURE\r\n",psetup->pkt.idx&0x0f);
			HwUsb_Switch2Endp(bp,psetup->idx&0x0f);
			_REG8(bp, REG_USB_RXCSR1)&=_BFD(3, 5, 2);
			_REG8(bp, REG_USB_RXCSR1)|=BIT(7);
			break;
	}
	HwUsb_SendZlp(bp,0);
	UNS_SET(endp,UNS_Complete);
}
/*
 *函数名:
 *	URB_SET_FEATURE
 *功能:
 *	SET_FEATURE
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
// void HwUsb_HaltEndp(int endpn){
// 	HwUsb_Switch2Endp(endpn);
// }
void URB_SET_FEATURE(void*bp,void*setupPkt,CEndpoint*endp){
	int rcp=GetRqtRecipient(setupPkt);
	CUsbSetupPkt*psetup=(CUsbSetupPkt*)setupPkt;
	char*ptr=NULL;
	switch(rcp){
		case USB_RT_RECIPIENT_DEVICE:
			ptr=(char*)&usb_status[0];
			if(psetup->tar==1){//DEVICE_REMOTE_WAKEUP
				ptr[0]|=BIT(_REMOTE_WAKEUP_);
			}
			break;
		case USB_RT_RECIPIENT_INTF:
			ptr=(char*)&usb_status[1];
			break;
		case USB_RT_RECIPIENT_ENDP:
			ptr=(char*)&((_t_usb_dev_drv*)AplUsbDev_GetDrv())->usb_endp_status[psetup->idx&0x0f];
			if(psetup->tar==0){
				ptr[0]|=BIT(0);
			}
			break;
	}
	HwUsb_SendZlp(bp,0);
	UNS_SET(endp,UNS_Complete);
}

/*
 *函数名:
 *	URB_SET_ADDRESS
 *功能:
 *	SET_ADDRESS
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
//extern void DelayNops_usb(volatile unsigned long nops);
extern uint8_t bUsbAddress;

void URB_SET_ADDRESS(void*bp,void*setupPkt,CEndpoint*endp){
	uint16_t *ptr=(uint16_t*)setupPkt;
	HwUsb_SendZlp(bp,0);

//	DelayNops_usb(10000);
	//os_printf("My Address=0x%.2x\r\n",ptr[1]);
//	HwUsb_SetAddress(bp,ptr[1]);
//	UNS_SET(endp,UNS_Complete);
	bUsbAddress=ptr[1];
	UNS_SET(endp,UNS_StatusIn);
	SET_USBSTATE(USB_STATE_ADDRESS);
	//HwUdpStatusOut(udp);
}
/*
 *函数名:
 *	URB_GET_DESCRIPTOR
 *功能:
 *	GET_DESCRIPTOR
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void URB_GET_DESCRIPTOR(void*bp,void*setupPkt,CEndpoint*endp){
	uint8_t *ptr=(uint8_t*)setupPkt;
	uint8_t rcp=GetRqtRecipient(setupPkt);
	uint8_t* tar=NULL;
	uint32_t len=0;
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	CBufferBaseDesc*pcfg;
//	#ifndef DYN_USB_DRV
//	CBufferBaseDesc*pcfg=(CBufferBaseDesc*)USBDesc_GetConfigDesc();
//	#else
	pcfg=(CBufferBaseDesc*)pusb->f_get_config_desc();
//	#endif
	CBufferBaseDesc*preq_desc;
	if(rcp>=USB_RT_RECIPIENT_ENDP){
		URB_NOT_SUPPORT(bp,setupPkt,endp);
		return;
	}
	if(rcp==USB_RT_RECIPIENT_INTF){
		rcp=ptr[4];
		//if(rcp>=USB_MAX_INTERFACE_NUM){
		if(rcp>=((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbCntOfIntf){
			URB_NOT_SUPPORT(bp,setupPkt,endp);
			return;
		}
		rcp=ptr[3];
		switch(rcp){
		case USB_DESCTYPE_HID:
			tar=(uint8_t*)_find_HidClassRpt(
				pcfg->ptr,
				pcfg->sz,
				ptr[4]);
			len=sizeof(CHidClassDesc);
			if(tar==NULL){URB_NOT_SUPPORT(bp,setupPkt, endp);return;}
			break;
		case USB_DESCTYPE_HID_Report:
//			#ifndef DYN_USB_DRV
//			preq_desc=(CBufferBaseDesc*)USBDesc_GetHidRptDesc(ptr[4]);
//			tar=(uint8_t*)(preq_desc->ptr);
//			len=preq_desc->sz;
//			#else
			if(pusb->f_get_hidreport_desc){
				preq_desc=(CBufferBaseDesc*)pusb->f_get_hidreport_desc(ptr[4]);
				tar=(uint8_t*)(preq_desc->ptr);
				len=preq_desc->sz;
			}
//			#endif
			break;
		default:
			URB_NOT_SUPPORT(bp,setupPkt,endp);
			return;
		}
		SetTxParameter(endp,setupPkt,tar,len);
		UNS_SET(endp,UNS_DataIn);
		HwUsb_SendPkt(bp,0,endp);
		return;
	}
	rcp=ptr[3];
	tar=NULL;
	len=0;
	switch(rcp){
	case USB_DESCTYPE_DEVICE:
//		#ifndef DYN_USB_DRV
//		preq_desc=(CBufferBaseDesc*)USBDesc_GetDeviceDesc();
//		#else
		if(pusb->f_get_device_desc)preq_desc=(CBufferBaseDesc*)pusb->f_get_device_desc();
		else break;
//		#endif
		tar=(uint8_t*)(preq_desc->ptr);
		len=preq_desc->sz;
		break;
	case USB_DESCTYPE_CONFIG:
//		#ifndef DYN_USB_DRV
//		preq_desc=(CBufferBaseDesc*)USBDesc_GetConfigDesc();
//		#else
		if(pusb->f_get_config_desc)preq_desc=(CBufferBaseDesc*)pusb->f_get_config_desc();
		else break;
//		#endif
		tar=(uint8_t*)(preq_desc->ptr);
		len=preq_desc->sz;
//		char b[4];
//		b[0]=0x09;
//		b[1]=USB_DESCTYPE_CONFIG;
//		b[2]=len&0xff;
//		b[3]=(len>>8)&0xff;
//		HwUsb_EndpWrite(0, b, 4);
		endp->status=USB_EndpStt_TxExCfg;
//		//len|=0x80000000;
//		len-=4;
//		tar+=4;
		break;
	case USB_DESCTYPE_STRING:
//		#ifndef DYN_USB_DRV
//		preq_desc=(CBufferBaseDesc*)USBDesc_GetStringDesc(ptr[2]);
//		#else
		if(pusb->f_get_string_desc)preq_desc=(CBufferBaseDesc*)pusb->f_get_string_desc(ptr[2]);
		else break;
//		#endif
		tar=(uint8_t*)(preq_desc->ptr);
		len=preq_desc->sz;
		break;
// 	default:
	}
	if(tar==NULL){
		URB_NOT_SUPPORT(bp,setupPkt,endp);
		return;
	}

//	os_printf("ptr=%.8x,sz=%d\r\n",preq_desc->ptr,preq_desc->sz);
	SetTxParameter(endp,setupPkt,tar,len);
	UNS_SET(endp,UNS_DataIn);
	HwUsb_SendPkt(bp,0,endp);
	//HwUdpStatusOut(udp);
}
/*
 *函数名:
 *	URB_GET_CONFIGURATION
 *功能:
 *	GET_CONFIGURATION
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void URB_GET_CONFIGURATION(void*bp,void*setupPkt,CEndpoint*endp){
	SetTxParameter(endp,setupPkt,(void*)&((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bIsConfig/*bUsbConfig*/,1);
	UNS_SET(endp,UNS_DataIn);
	HwUsb_SendPkt(bp,0,endp);
}
/*
 *函数名:
 *	URB_SET_CONFIGURATION
 *功能:
 *	SET_CONFIGURATION
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void URB_SET_CONFIGURATION(void*bp,void*setupPkt,CEndpoint*endp){
	uint8_t*ptr=(uint8_t*)setupPkt;
	uint32_t tmp=ptr[2];
	CBufferBaseDesc*pcfg=NULL;
	if(tmp!=1){
		//bUsbConfig=0;
		AplUsb_SetConfig(0);
		UNS_SET(endp,UNS_Complete);
		HwUsb_SendZlp(bp,0);
		SET_USBSTATE(USB_STATE_ADDRESS);
//
// 		URB_NOT_SUPPORT(setupPkt,endp);
		return;
	}else{
		AplUsb_SetConfig(ptr[2]);
		//bUsbConfig=ptr[2];
		UNS_SET(endp,UNS_Complete);
		HwUsb_SendZlp(bp,0);
//		#ifndef DYN_USB_DRV
//		pcfg=(CBufferBaseDesc*)USBDesc_GetConfigDesc();
//		#else
		_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
		pcfg=(CBufferBaseDesc*)pusb->f_get_config_desc();
//		#endif
		//因为device的configDesc在本地，在初始化的时候就预分析过，所以此处没有进行预分析
		_config_endp(bp,pcfg->ptr, pcfg->sz,1);
// 		_config_endp((void*)&tUsbConfig, sizeof(tUsbConfig));

		SET_USBSTATE(USB_STATE_CONFIG);
		//HwUdpConfig(udp);//config my device
	}

}
/*
 *函数名:
 *	URB_GET_INTERFACE
 *功能:
 *	GET_INTERFACE
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void URB_GET_INTERFACE(void*bp,void*setupPkt,CEndpoint*endp){
	uint16_t*ptr=(uint16_t*)setupPkt;
	uint32_t tmp=ptr[2];
//	if(tmp>=USB_MAX_INTERFACE_NUM){
	if(tmp>=AplUsb_GetIntfCount()/*((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbCntOfIntf*/){
		URB_NOT_SUPPORT(bp,setupPkt,endp);
		return;
	}else{
		SetTxParameter(endp,setupPkt,&((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbAlterInterface[tmp],1);
		UNS_SET(endp,UNS_DataIn);
		HwUsb_SendPkt(bp,0,endp);
	}
}
/*
 *函数名:
 *	URB_SET_INTERFACE
 *功能:
 *	SET_INTERFACE
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
//extern void _endp_tx(int endpn);
void URB_SET_INTERFACE(void*bp,void*setupPkt,CEndpoint*endp){
	uint8_t *ptr=(uint8_t*)setupPkt;
	uint32_t altIntf=ptr[2];
	uint32_t idx=ptr[4];
//	if(idx>=USB_MAX_INTERFACE_NUM){
	if(idx>=AplUsb_GetIntfCount()/*((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbCntOfIntf*/){
		URB_NOT_SUPPORT(bp,setupPkt,endp);
		return;
	}else{
		((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbAlterInterface[idx]=altIntf;
		//if audio interface is called,it should ctrl endpoint
		UNS_SET(endp,UNS_StatusIn);
		HwUsb_SendZlp(bp,0);
		//需要根据interface类型有不同处理
		//比如mic interface有可能需要打开mic模拟电路、adc采样、启动USB发送等处理步骤
		//speaker interface有可能需要打开dac、功放电路等处理步骤
		if(((_t_usb_dev_drv*)AplUsbDev_GetDrv())->cbkSetInterface[idx]){
			((_t_usb_dev_drv*)AplUsbDev_GetDrv())->cbkSetInterface[idx](idx,altIntf);
		}
//		if((idx==USB_INTFID_AudioAS_MIC)&&(altIntf==1)){
//			_endp_tx(idx);
//		}
	}
}

const CFunPtr tUsbStdRequest[]={
	URB_GET_STATUS,		//URB_NOT_SUPPORT,//	;; Get_Status		,0
	URB_CLEAR_FEATURE,	//URB_NOT_SUPPORT,//	;; Clear_Feature	,1
	URB_NOT_SUPPORT,//URB_RESERVED			;; Reserved		,2
	URB_SET_FEATURE,		//URB_NOT_SUPPORT,//	;; Set_Feature		,3
	URB_NOT_SUPPORT,//						;; Reserved		,4
	URB_SET_ADDRESS,//URB_SET_ADDRESS		;; Set_Address		,5
	URB_GET_DESCRIPTOR,//					;; Get_Descriptor	,6
	URB_NOT_SUPPORT,//URB_SET_DESCRIPTOR	;; Set_Descriptor	,7
	URB_GET_CONFIGURATION,//				;; Get_Configuration	,8
	URB_SET_CONFIGURATION,//				;; Set_Configuration	,9
	URB_GET_INTERFACE,//					;; Get_Interface		,10
	URB_SET_INTERFACE,//					;; Set_Interface		,11
	URB_NOT_SUPPORT,//	URB_SYNCH_FRAME		;; Synch_Frame			,12

};

/*
 *函数名:
 *	HalUsbStandardRequest
 *功能:
 *	usb标准请求处理
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void HalUsbStandardRequest(void*bp, void* setupPkt, CEndpoint* endp){
	uint8_t* pSetup=(uint8_t*)setupPkt;
	CFunPtr fp;
	fp=(CFunPtr)tUsbStdRequest[pSetup[1]];
	fp(bp, setupPkt, endp);
}

/*
 *函数名:
 *	HalUsbSetupHandle
 *功能:
 *	usb setup封包处理
 *参数:
 *	1.setupPkt:setup包指针
 *	2.endp:端点管理器
 *	3.udp:udp指针
 *返回:
 *	无
 *特殊:
 *	1.
*/
void HalUsbSetupHandle(void*bp, void*setupPkt, CEndpoint *endp)
{
	uint8_t type=GetRqtType(setupPkt);
	//usb_dbg_buf_show("setup",setupPkt,8);
	if(type==0){//std request
		//USB_LOG_D("std request cmd\r\n");
		HalUsbStandardRequest(bp,setupPkt,endp);
	}
	if(type==1){//class request
		// USB_LOG_D("class cmd\r\n");
		HalUsbClassRequest(bp,setupPkt,endp);
		// USB_LOG_D("class cmd end\r\n");
	}
	if(type>=2){//vendor req2uest
		HwUsb_SendStall(bp,0);
	}
}
/*end file*/

/*end file*/
