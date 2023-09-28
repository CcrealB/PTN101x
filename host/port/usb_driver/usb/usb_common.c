/*************************************************************
 * @file		usb_common.c
 * @brief		descriptor analoger using USB spec1.1 chapter 9
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
//extern void HwUsb_SendZlp(void*bp,int endpn);
//extern void HwUsb_SendStall(void*bp,int endpn);
extern void HwUsb_Switch2Endp(void*bp,int endpn);
//extern void HwUsb_SetAddress(void*bp,int addr);
//extern void HwUsb_EndpWrite(void*bp,int endpn, void * buf, int len);
//extern void HwUsb_SendPkt(void*bp,uint8_t epn, CEndpoint * endp);
//extern void HalUsbClassRequest(void*bp,void * setupPkt, CEndpoint * endp);

/*variable implement section*/
/////////////////////////////////////////////////////////////////////////////////////////
//interface and endpoint should be arranged in order
static _t_usb_dev_drv*s_drv_dev=NULL;

//volatile uint8_t bUsbConfig=0; //usb configed value
//extern uint8_t bUsbState;
//#define DYN_USB_DRV
/*function implement section*/

/*
 *函数名:
 *	_find_tarDesc
 *功能:
 *	在指定描述符集合中查找指定类型的usb描述符
 *参数:
 *	1.desc,sz:描述符集合
 *	2.tarTyp,index:指定描述符类型及索引号
 *返回:
 *	指定描述符的指针,NULL:=未找到
 *特殊:
 *
*/

void* _find_tarDesc(void*desc,int sz,int tarTyp,int index){
	CUsbCommonDesc*pdesc=(CUsbCommonDesc*)desc;
	char*ptr=(char*)pdesc;
	int cnt=0;
	while(1){
		if((ptr-(char*)desc)>=sz)break;
		if(pdesc->descType==tarTyp){
			cnt++;
			if(index==cnt)return(pdesc);
		}
		ptr=&ptr[pdesc->len];
		pdesc=(CUsbCommonDesc*)ptr;
	}
	return(NULL);
}

void* _find_HidClassRpt(void*desc,int sz,int intfno){
	CUsbCommonDesc*pdesc=(CUsbCommonDesc*)desc;
	CUsbIntfDesc*pintf=NULL;
	char*ptr=(char*)pdesc;
//	int cnt=0;
	while(1){
		if((ptr-(char*)desc)>=sz)break;
		if(pdesc->descType==USB_DESCTYPE_INTERFACE){
			pintf=(CUsbIntfDesc*)pdesc;
			if(pintf->bIntfNo==intfno){
				return (void*)&ptr[pdesc->len];
			}
		}
		ptr=&ptr[pdesc->len];
		pdesc=(CUsbCommonDesc*)ptr;
	}
	return ((void*)NULL);
}

/*
 *函数名:
 *	_get_nextDesc
 *功能:
 *	获取指定描述符集合中当前描述符的下一个描述符
 *参数:
 *	1.desc,sz:描述符集合
 *	2.cur:当前描述符指针
 *返回:
 *	指定描述符的指针,NULL:=未找到
 *特殊:
 *
*/
void* _get_nextDesc(void*desc,int sz,void*cur){
	CUsbCommonDesc*pdesc=(CUsbCommonDesc*)cur;
	char*ptr=(char*)pdesc;
	ptr=&ptr[pdesc->len];
	pdesc=(CUsbCommonDesc*)ptr;
	if((ptr-(char*)desc)>=sz)return NULL;
	return(pdesc);
}

/*
 *函数名:
 *	_find_endpDesc_byEndpNo
 *功能:
 *	根据端点号获取指定描述符集合中的端点描述符
 *参数:
 *	1.desc,sz:描述符集合
 *	2.endp_no:端点号
 *返回:
 *	指定描述符的指针,NULL:=未找到
 *特殊:
 *
*/
void* _find_endpDesc_byEndpNo(void*desc,int sz,int endp_no){
	CUsbCommonDesc*pdesc=(CUsbCommonDesc*)desc;
	CUsbEndpDesc*pendp=NULL;
	char*ptr=(char*)pdesc;
	while(1){
		if((ptr-(char*)desc)>=sz)break;
		if(pdesc->descType==USB_DESCTYPE_ENDPOINT){
			pendp=(CUsbEndpDesc*)pdesc;
			if((pendp->bEndpAddr&0x0f)==endp_no)return((void*)pdesc);
		}
		ptr=&ptr[pdesc->len];
		pdesc=(CUsbCommonDesc*)ptr;
	}
	return((void*)NULL);
}

/*
 *函数名:
 *	UsbDesc_Find
 *功能:
 *	根据索引号获取描述符列表中指定描述符
 *参数:
 *	1.idx:索引号
 *	2.tb,cnt:描述符列表
 *返回:
 *	指定描述符的指针,NULL:=未找到
 *特殊:
 *
*/
void*UsbDesc_Find(int idx,void*tb,int cnt){
	_t_index_descriptor*ptbl=(_t_index_descriptor*)tb;
	int i;
	for(i=0;i<cnt;i++){
		if(ptbl[i].ind==idx){
			return (void*)&ptbl[i].desc;
		}
	}
	return(NULL);
}

/*
 *函数名:
 *	AplUsbDev_BuildDrv
 *功能:
 *	创建UsbDevice
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	
*/
void AplUsbDev_BuildDrv(){
static uint8_t bFlag=0;
	if(bFlag)return;
	s_drv_dev=(_t_usb_dev_drv*)malloc(sizeof(_t_usb_dev_drv));
	memset(s_drv_dev,0,sizeof(_t_usb_dev_drv));
	bFlag=1;
}

/*
 *函数名:
 *	AplUsbDev_GetDrv
 *功能:
 *	获取UsbDevice指针
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	
*/
void*AplUsbDev_GetDrv() { return ((void*)s_drv_dev); }

/*
 *函数名:
 *	AplUsb_SetSuspend
 *功能:
 *	设置suspend回调
 *参数:
 *	1.cbk:suspend回调函数
 *返回:
 *	无
 *特殊:
 *	
*/
CBK_USB_SUSPEND cbk_suspend;
void AplUsb_SetSuspend(void*cbk){
	cbk_suspend=(CBK_USB_SUSPEND)cbk;
}
/*
 *函数名:
 *	AplUsb_IsConfigured
 *功能:
 *	检测UsbDevice是否已经配置
 *参数:
 *	无
 *返回:
 *	0=未配置,!0=已配置
 *特殊:
 *	
*/
int AplUsb_IsConfigured(void){
	return(((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bIsConfig/*bUsbConfig*/);
}
#define DISTofPTR(p1,p0)		((((char*)p1)>((char*)p0))?((int)((char*)p1-(char*)p0)):((int)((char*)p0-(char*)p1)))

/*
 *函数名:
 *	__analog_config_desc_v2
 *功能:
 *	分析config描述符，并做指定处理
 *参数:
 *	1.cfg,sz:指定config描述符
 *	2.proc:分析后处理
 *返回:
 *	无
 *特殊:
 *	
*/
void __analog_config_desc_v2(void*cfg,int sz,void*proc){
	int maxEndp=1;
	int maxIntf=0;
	CUsbEndpDesc*pendpDesc=NULL;
	CUsbIntfDesc*pIntfDesc=NULL;
	CUsbCommonDesc*pDesc=(CUsbCommonDesc*)cfg;
	char*ptr=(char*)pDesc;
	int cnt=0;
	while(1){
		if(DISTofPTR(ptr, cfg)>=sz)break;
		USB_LOG_D("%dth desc:%.2x %.2x\r\n",cnt,pDesc->len,pDesc->descType);
		USB_LOG_D("DISTofPTR(ptr, cfg)=%d\r\n",DISTofPTR(ptr, cfg));
		if(pDesc->descType==USB_DESCTYPE_INTERFACE){
			pIntfDesc=(CUsbIntfDesc*)pDesc;
			if(pIntfDesc->bIntfNo>maxIntf)maxIntf=pIntfDesc->bIntfNo;
		}
		if(pDesc->descType==USB_DESCTYPE_ENDPOINT){
			pendpDesc=(CUsbEndpDesc*)pDesc;
			if((pendpDesc->bEndpAddr&0x0f)>maxEndp)maxEndp=pendpDesc->bEndpAddr&0x0f;
		}
		ptr=&ptr[pDesc->len];
		pDesc=(CUsbCommonDesc*)ptr;
		cnt++;
	}
	USB_LOG_D("analog config descriptor OK\r\n");
	void (*p_proc)(int ,int );
	p_proc=(void (*)(int,int))proc;
	maxIntf++;maxEndp++;
	if(p_proc)p_proc(maxIntf,maxEndp);
	USB_LOG_I("desc cfg ok, intf=%d,endp=%d\r\n", maxIntf, maxEndp);
}

/*
 *函数名:
 *	_config_endp_v2
 *功能:
 *	配置端点
 *参数:
 *	1.bp:usb寄存器基地址
 *	2.desc,sz:指定config描述符
 *	3.myDesc:1=本地描述符(作为usb device)，0=远程描述符(作为usb host)
 *	4.buildTxPip:发送管道的创建处理
 *	5.buildRxPip:接收管道的创建处理
 *返回:
 *	端点个数
 *特殊:
 *	
*/
int _config_endp_v2(void*bp,void*desc,int sz,int myDesc,void*buildTxPip,void*buildRxPip){
	CUsbEndpDesc*pendpDesc=NULL;
	int index=1;
	int cnt=0;
	int rx_tx;
	while(1){
		pendpDesc=(CUsbEndpDesc*)_find_tarDesc(desc, sz, USB_DESCTYPE_ENDPOINT, index);
		if(pendpDesc==NULL)break;
		cnt++;
		HwUsb_Switch2Endp(bp,pendpDesc->bEndpAddr&0x0f);
		rx_tx=1;
		if(pendpDesc->bEndpAddr&BIT(7)){//作为device，endp描述方向为IN时，endp向host输出数据(TX)
			rx_tx=0;
		}
		if(myDesc==0)rx_tx^=1;//作为host需反向
		//if(pendpDesc->bEndpAddr&BIT(7)){//tx mode
		if(rx_tx==0){//tx mode
			_REG8(bp,REG_USB_TXMAXP)=(pendpDesc->wMaxPktSz>>3);//
			_REG8(bp,REG_USB_CSR02)=BIT(5);//tx mode
			_REG8(bp,REG_USB_CSR0)=BIT(3);
			if((pendpDesc->bmAttribute&0x03)==USB_ENDP_ISO){
				_REG8(bp,REG_USB_CSR02)|=BIT(6);//iso
			}

			if((pendpDesc->bEndpAddr&0x0f)&BIT(3))_REG8(bp,REG_USB_INTRTX2E)|=BIT((pendpDesc->bEndpAddr&0x07));
			else
				_REG8(bp,REG_USB_INTRTX1E)|=BIT((pendpDesc->bEndpAddr&0x07));
			void (*p_buildTxPip)(int,int);
			p_buildTxPip=(void (*)(int,int))buildTxPip;
			if(p_buildTxPip)p_buildTxPip(pendpDesc->bEndpAddr&0x0f, pendpDesc->wMaxPktSz);
			//_build_pipTxBuf(pendpDesc->bEndpAddr&0x0f, pendpDesc->wMaxPktSz);
			if(myDesc==0){
				_REG8(bp,REG_USB_TXTYPE)=((pendpDesc->bmAttribute&0x03)<<4)|(pendpDesc->bEndpAddr&0x0f);
				if((pendpDesc->bmAttribute&0x03)==USB_ENDP_BULK)_REG8(bp,REG_USB_NAKLIMIT0)=0;
				else _REG8(bp,REG_USB_NAKLIMIT0)=pendpDesc->bInterval;
			}
		}else{
			_REG8(bp,REG_USB_RXMAXP)=(pendpDesc->wMaxPktSz>>3);
			_REG8(bp,REG_USB_CSR02)=0;
			_REG8(bp,REG_USB_RXCSR2)=0;//rx mode
			if((pendpDesc->bmAttribute&0x03)==USB_ENDP_ISO){
				_REG8(bp,REG_USB_RXCSR2)|=BIT(6);//iso
			}
			_REG8(bp,REG_USB_RXCSR1)=BIT(7);//clear data toggle
			if((pendpDesc->bEndpAddr&0x0f)&BIT(3))_REG8(bp,REG_USB_INTRRX2E)|=BIT((pendpDesc->bEndpAddr&0x07));
			else
				_REG8(bp,REG_USB_INTRRX1E)|=BIT((pendpDesc->bEndpAddr&0x07));
			void (*p_buildRxPip)(int,int);
			p_buildRxPip=(void (*)(int,int))buildRxPip;
			if(p_buildRxPip)p_buildRxPip(pendpDesc->bEndpAddr&0x0f, pendpDesc->wMaxPktSz);
			//_build_pipRxBuf(pendpDesc->bEndpAddr&0x0f, pendpDesc->wMaxPktSz);
			if(myDesc==0){
				_REG8(bp,REG_USB_RXTYPE)=((pendpDesc->bmAttribute&0x03)<<4)|(pendpDesc->bEndpAddr&0x0f);
				if((pendpDesc->bmAttribute&0x03)==USB_ENDP_BULK)_REG8(bp,REG_USB_RXINTERVAL)=0;
				else _REG8(bp,REG_USB_RXINTERVAL)=pendpDesc->bInterval;
			}
		}
		index++;
	}

	return(cnt);
}

/*
 *函数名:
 *	_usbdev_init
 *功能:
 *	根据接口数、端点数初始化usb device
 *参数:
 *	1.maxIntf:intf数量
 *	2.maxEndp:endp数量
 *返回:
 *	无
 *特殊:
 *	
*/
static void _usbdev_init(int maxIntf,int maxEndp){
	usb_hal_drv_init(AplUsbDev_GetDrv(), maxIntf, maxEndp);
}

/*
 *函数名:
 *	__analog_config_desc
 *功能:
 *	usb device分析config描述符
 *参数:
 *	1.cfg,sz:config描述符
 *返回:
 *	无
 *特殊:
 *	
*/
void __analog_config_desc(void*cfg,int sz){
	__analog_config_desc_v2(cfg, sz, _usbdev_init);
}

/*
 *函数名:
 *	AplUsb_GetEndpCount
 *功能:
 *	获取usb device端点数
 *参数:
 *	无
 *返回:
 *	usb device端点数
 *特殊:
 *	
*/
int AplUsb_GetEndpCount(){
	return(((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbCntOfEndp);
}

/*
 *函数名:
 *	AplUsb_GetIntfCount
 *功能:
 *	获取usb device intf数
 *参数:
 *	无
 *返回:
 *	usb device intf数
 *特殊:
 *	
*/
int AplUsb_GetIntfCount(){
	return(((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bUsbCntOfIntf);
}

/*
 *函数名:
 *	AplUsb_SetConfig
 *功能:
 *	设定usb device 的配置
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	
*/
void AplUsb_SetConfig(int cfg){
	((_t_usb_dev_drv*)AplUsbDev_GetDrv())->bIsConfig=cfg;
}

/*
 *函数名:
 *	_device_pre_analog_config_desc
 *	_pre_analog_config_desc
 *功能:
 *	usb device预分析config描述符，初始化usb device并生成usb device信息
 *参数:
 *	无
 *返回:
 *	无
 *特殊:
 *	
*/
void _pre_analog_config_desc(){
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	CBufferBaseDesc*pcfg=(CBufferBaseDesc*)pusb->f_get_config_desc();
	__analog_config_desc(pcfg->ptr,pcfg->sz);
}

void _device_pre_analog_config_desc(){
	_pre_analog_config_desc();
}

/*
 *函数名:
 *	_build_pipRxBuf
 *功能:
 *	usb device创建接收pip信息
 *参数:
 *	1.endp_no:端点号
 *	2.maxp:最大包长度
 *返回:
 *	无
 *特殊:
 *	
*/
void _build_pipRxBuf(int endp_no,int maxp){
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
//	#ifndef DYN_USB_DRV
//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
//	#else
	if(endp_no>=pdev->bUsbCntOfEndp)return;
//	#endif
	pdev->pipIntf[endp_no].rxBuf.sz=maxp;
	if(pdev->pipIntf[endp_no].rxBuf.ptr){
		free(pdev->pipIntf[endp_no].rxBuf.ptr);
		pdev->pipIntf[endp_no].rxBuf.ptr=NULL;
	}
	pdev->pipIntf[endp_no].rxBuf.ptr=malloc(maxp);
	USB_LOG_I("brb endp:%d, rxbuf=0x%.8x\r\n", endp_no, pdev->pipIntf[endp_no].rxBuf.ptr);
}

/*
 *函数名:
 *	_build_pipTxBuf
 *功能:
 *	usb device创建发送pip信息
 *参数:
 *	1.endp_no:端点号
 *	2.maxp:最大包长度
 *返回:
 *	无
 *特殊:
 *	
*/
void _build_pipTxBuf(int endp_no,int maxp){
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
//	#ifndef DYN_USB_DRV
//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
//	#else
	if(endp_no>=pdev->bUsbCntOfEndp)return;
//	#endif
	USB_LOG_D("btb endp=%d\r\n",endp_no);
	if(pdev->pipIntf[endp_no].txBuf.ptr){
		free(pdev->pipIntf[endp_no].txBuf.ptr);
		pdev->pipIntf[endp_no].txBuf.ptr=NULL;
	}
	pdev->pipIntf[endp_no].txBuf.sz=maxp;
	pdev->pipIntf[endp_no].txBuf.ptr=malloc(maxp);
	USB_LOG_D("btb end txbuf=0x%.8x\r\n",pdev->pipIntf[endp_no].txBuf.ptr);
}

/*
 *函数名:
 *	_config_endp
 *功能:
 *	usb device配置endp
 *参数:
 *	1.bp:usb寄存器基地址
 *	2.desc,sz:config描述符
 *	3.myDesc:1=本地描述符，0=远程描述符
 *返回:
 *	配置端点数量
 *特殊:
 *	
*/
//myDesc:1=本地描述符(作为device)，0=远端描述符(作为host)
//_config_endp之前必须要先预分析configDesc，获取usb intf、endp信息，分配相应的空间
int _config_endp(void*bp,void*desc,int sz,int myDesc){
	return _config_endp_v2(bp, desc, sz, myDesc, _build_pipTxBuf, _build_pipRxBuf);
}
#if 0
int config_device(){
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	CBufferBaseDesc*pcfg;
	pcfg=(CBufferBaseDesc*)pusb->f_get_config_desc();
	return _config_endp(pusb->bp,pcfg->ptr,pcfg->sz,1);
}
#endif
/*end file*/
