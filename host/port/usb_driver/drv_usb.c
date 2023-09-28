#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver_usb.h"
#include "usb\usbdef.h"
#include "drv_usb.h"

extern void*AplUsbHost_GetDrv();
void* _find_endpDesc_byEndpNo(void*desc,int sz,int endp_no);

_t_drv_usb_device usb_dev;

void*get_usb_dev_handle(){
	return ((void*)&usb_dev);
}
void load_usb_drv(void*drv){
	memcpy(&usb_dev,drv,sizeof(usb_dev));
}


void usb_hal_drv_init(void*drv,int maxIntf,int maxEndp){
	_t_usb_hal_drv *phost=(_t_usb_hal_drv *)drv;
	int i;
	DRV_USB_DBG(phost->bUsbAlterInterface);
	if(phost->bUsbAlterInterface)free(phost->bUsbAlterInterface);
	phost->bUsbAlterInterface=NULL;
	DRV_USB_DBG(phost->cbkSetInterface);
	if(phost->cbkSetInterface)free(phost->cbkSetInterface);
	phost->cbkSetInterface=NULL;
	DRV_USB_DBG(phost->pipIntf);
	if(phost->pipIntf){
		for(i=0;i<phost->bUsbCntOfEndp;i++){
			DRV_USB_DBG(phost->pipIntf[i].rxBuf.ptr);
			if(phost->pipIntf[i].rxBuf.ptr)free(phost->pipIntf[i].rxBuf.ptr);
			phost->pipIntf[i].rxBuf.ptr=NULL;
			DRV_USB_DBG(phost->pipIntf[i].txBuf.ptr);
			if(phost->pipIntf[i].txBuf.ptr)free(phost->pipIntf[i].txBuf.ptr);
			phost->pipIntf[i].txBuf.ptr=NULL;
		}
		free(phost->pipIntf);
		phost->pipIntf=NULL;
	}
	DRV_USB_DBG(phost->usb_endp_status);
	if(phost->usb_endp_status)free(phost->usb_endp_status);
	phost->usb_endp_status=NULL;
	
	phost->bUsbCntOfEndp=maxEndp;
	phost->bUsbCntOfIntf=maxIntf;
	phost->bUsbAlterInterface=(uint8_t*)malloc(phost->bUsbCntOfIntf);
	phost->cbkSetInterface=(CBK_SET_INTERFACE*)malloc(phost->bUsbCntOfIntf*sizeof(CBK_SET_INTERFACE));
	phost->pipIntf=(CUsbAppIntf*)malloc(phost->bUsbCntOfEndp*sizeof(CUsbAppIntf));
	phost->usb_endp_status=(uint16_t*)malloc(phost->bUsbCntOfEndp*sizeof(uint16_t));
	DRV_USB_DBG(phost->bUsbAlterInterface);
	DRV_USB_DBG(phost->cbkSetInterface);
	DRV_USB_DBG(phost->pipIntf);
	DRV_USB_DBG(phost->usb_endp_status);

	memset(phost->bUsbAlterInterface,0,phost->bUsbCntOfIntf);
	memset(phost->cbkSetInterface,0,phost->bUsbCntOfIntf*sizeof(CBK_SET_INTERFACE));
	memset(phost->pipIntf,0,phost->bUsbCntOfEndp*sizeof(CUsbAppIntf));
	memset(phost->usb_endp_status,0,phost->bUsbCntOfEndp*sizeof(uint16_t));
}

int AplUsbHost_IsConfigured(){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	return phost->bIsConfig;
}

void AplUsbHost_SetConfigured(int cfgId){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	phost->bIsConfig=cfgId;
}

int AplUsbHost_GetEndpCount(){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	return(phost->bUsbCntOfEndp);
}

int AplUsbHost_GetIntfCount(){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	return(phost->bUsbCntOfIntf);
}

void AplUsbHost_SetTxCbk(int endp_no,void*cbk){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	if(endp_no>phost->bUsbCntOfEndp)return;
	phost->pipIntf[endp_no].txcbk=(CBK_USBAPP)cbk;
}
void AplUsbHost_SetTxCompletedCbk(int endp_no,void*cbk){
	AplUsbHost_SetTxCbk(endp_no,cbk);
}
void AplUsbHost_SetRxPreCbk(int endp_no,void*cbk){
	AplUsbHost_SetTxCbk(endp_no,cbk);
}
void AplUsbHost_SetRxCbk(int endp_no,void*cbk){
//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	if(endp_no>=phost->bUsbCntOfEndp)return;
	phost->pipIntf[endp_no].rxcbk=(CBK_USBAPP)cbk;
}

void AplUsbHost_SetInterfaceCbk(int id_intf,void*cbk){
//	if(id_intf>=USB_MAX_INTERFACE_NUM)return;
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	if(id_intf>=phost->bUsbCntOfIntf)return;
	phost->cbkSetInterface[id_intf]=(CBK_SET_INTERFACE)cbk;
}
void AplUsbHost_SetRxCompletedCbk(int endp_no,void*cbk){
	AplUsbHost_SetRxCbk(endp_no, cbk);
}

void *AplUsbHost_GetRxBufDesc(int endp_no){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	if(endp_no>phost->bUsbCntOfEndp){
		return (void*)NULL;
	}else{
		return (void*)&phost->pipIntf[endp_no].rxBuf;
	}
}

void *AplUsbHost_GetRxBuf(int endp_no){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	if(endp_no>phost->bUsbCntOfEndp){
		return (void*)NULL;
	}else{
		return (void*)phost->pipIntf[endp_no].rxBuf.ptr;
	}
}

void *AplUsbHost_GetTxBufDesc(int endp_no){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	if(endp_no>phost->bUsbCntOfEndp){
		return (void*)NULL;
	}else{
		return (void*)&phost->pipIntf[endp_no].txBuf;
	}
}

void *AplUsbHost_GetTxBuf(int endp_no){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	if(endp_no>phost->bUsbCntOfEndp){
		return (void*)NULL;
	}else{
		return (void*)phost->pipIntf[endp_no].txBuf.ptr;
	}
}

void *AplUsbHost_GetTxCbk(int endp_no){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	if(endp_no>phost->bUsbCntOfEndp){
		return (void*)NULL;
	}else{
		//USB_LOG_I("endp%d txcbk=0x%.8x\r\n",endp_no,pipIntf[endp_no].txcbk);
		return (void*)phost->pipIntf[endp_no].txcbk;
	}
}

void *AplUsbHost_GetTxCompletedCbk(int endp_no){
	return ((void*)AplUsbHost_GetTxCbk(endp_no));
}

void *AplUsbHost_GetRxPreCbk(int endp_no){
	return ((void*)AplUsbHost_GetTxCbk(endp_no));
}

void *AplUsbHost_GetRxCbk(int endp_no){
	_t_usb_host_drv*phost=(_t_usb_host_drv*)AplUsbHost_GetDrv();
	if(endp_no>phost->bUsbCntOfEndp){
		return (void*)NULL;
	}else{
		//USB_LOG_I("endp%d rxcbk=0x%.8x\r\n",endp_no,pipIntf[endp_no].rxcbk);
		return (void*)phost->pipIntf[endp_no].rxcbk;
	}
}

void *AplUsbHost_GetRxCompletedCbk(int endp_no){
	return((void*)AplUsbHost_GetRxCbk(endp_no));
}
extern void*AplUsbDev_GetDrv();
void AplUsb_SetTxCbk(int endp_no,void*cbk){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp)return;
	pdev->pipIntf[endp_no].txcbk=(CBK_USBAPP)cbk;
}
void AplUsb_SetTxCompletedCbk(int endp_no,void*cbk){
	AplUsb_SetTxCbk(endp_no,cbk);
}
void AplUsb_SetRxPreCbk(int endp_no,void*cbk){
	AplUsb_SetTxCbk(endp_no,cbk);
}
void AplUsb_SetRxCbk(int endp_no,void*cbk){
//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>=pdev->bUsbCntOfEndp)return;
	pdev->pipIntf[endp_no].rxcbk=(CBK_USBAPP)cbk;
}

void AplUsb_SetInterfaceCbk(int id_intf,void*cbk){
//	if(id_intf>=USB_MAX_INTERFACE_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(id_intf>=pdev->bUsbCntOfIntf)return;
	pdev->cbkSetInterface[id_intf]=(CBK_SET_INTERFACE)cbk;
}
void AplUsb_SetRxCompletedCbk(int endp_no,void*cbk){
	AplUsb_SetRxCbk(endp_no, cbk);
}
void *AplUsb_GetRxBufDesc(int endp_no){
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp){
		return NULL;
	}else{
		return &pdev->pipIntf[endp_no].rxBuf;
	}
}

void *AplUsb_GetRxBuf(int endp_no){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp){
		return NULL;
	}else{
		return pdev->pipIntf[endp_no].rxBuf.ptr;
	}
}

void *AplUsb_GetTxBufDesc(int endp_no){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp){
		return NULL;
	}else{
		return &pdev->pipIntf[endp_no].txBuf;
	}
}

void *AplUsb_GetTxBuf(int endp_no){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp){
		return NULL;
	}else{
		return pdev->pipIntf[endp_no].txBuf.ptr;
	}
}
void *AplUsb_GetTxCbk(int endp_no){

	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp){
		return NULL;
	}else{
		//USB_LOG_I("endp%d txcbk=0x%.8x\r\n",endp_no,pipIntf[endp_no].txcbk);
		return pdev->pipIntf[endp_no].txcbk;
	}
}
void *AplUsb_GetTxCompletedCbk(int endp_no){
	return(AplUsb_GetTxCbk(endp_no));
}

void *AplUsb_GetRxPreCbk(int endp_no){
	return(AplUsb_GetTxCbk(endp_no));
}

void *AplUsb_GetRxCbk(int endp_no){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp){
		return NULL;
	}else{
		//USB_LOG_I("endp%d rxcbk=0x%.8x\r\n",endp_no,pipIntf[endp_no].rxcbk);
		return pdev->pipIntf[endp_no].rxcbk;
	}
}

void *AplUsb_GetRxCompletedCbk(int endp_no){
	return(AplUsb_GetRxCbk(endp_no));
}

int AplUsb_GetBufSz(int endp_no){
	//	if(endp_no>USB_MAX_ENDPOINT_NUM)return;
	_t_usb_dev_drv*pdev=(_t_usb_dev_drv*)AplUsbDev_GetDrv();
	if(endp_no>pdev->bUsbCntOfEndp)return 0;
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	CUsbEndpDesc*pendpDesc=NULL;
	CBufferBaseDesc*pcfg=(CBufferBaseDesc*)pusb->f_get_config_desc();
	pendpDesc=(CUsbEndpDesc*)_find_endpDesc_byEndpNo(pcfg->ptr, pcfg->sz, endp_no);
	if(pendpDesc)return(pendpDesc->wMaxPktSz);
	return(0);
}

