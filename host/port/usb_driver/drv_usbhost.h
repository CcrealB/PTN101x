#ifndef _DRV_USBHOST_H_
#define _DRV_USBHOST_H_
void*AplUsbHost_GetAppInitCbk();
void AplUsbHost_SetAppInitCbk(void*cbk);
void*AplUsbHost_GetLostDeviceCbk();
void AplUsbHost_SetLostDeviceCbk(void*cbk);
void AplUsbHost_SetSOFCbk(void*cbk);
void *AplUsbHost_GetSOFCbk();
void*AplUsbHost_GetDrv();
void usbhost_open(int usb_sel);
void*AplUsbHost_GetBP();
void usbhost_bus_reset();
void usbhost_power_on();
void usbhost_suspend();
void usbhost_resume();


void usbhost_start_ctrl_transfer(void*bp,void*setup,void*dat,int sz,void*sucCbk,void*failCbk);
int usbhost_is_ctrl_transfer_busy();
void usbhost_busreset_proc();//在10ms定时器里运行

#define USB_BUILD_SETUP_PKT(psetup,req,dir,typ,recip,index)		\
	{\
		memset(psetup,0,sizeof(CUsbSetupPkt));\
		((CUsbSetupPkt *)psetup)->dat[0]=(dir&USB_RT_DIR_MASK)|(typ&USB_RT_TYP_MASK)|(recip&USB_RT_RECIPT_MASK);\
		((CUsbSetupPkt *)psetup)->dat[1]=req;\
		((CUsbSetupPkt *)psetup)->idx=index;\
	}

#endif

