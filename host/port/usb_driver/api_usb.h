#ifndef _API_USB_H_
#define _API_USB_H_

#include "usb_dev\audio_hid\audio_hid.h"
#include "usb_host\usbh_udisk.h"

enum{
	E_USBMOD_HOST=0,
	E_USBMOD_FS_DEV=1,
	E_USBMOD_LS_DEV=3
};

enum {
	USBPORT_0=0,//PTN101x USB0
	/////////只有 PTN1012 有2个USB接口
	USBPORT_1,//PTN1012 USB1
	USBPORT_2,//PTN1012 USB0
	USBPORT_UnDef,//undefined host
};

enum {
	USBDEV_UDISK=0,//SD card reader
	USBDEV_AUDIO_HID,//audio&HID device
	USBDEV_UnDef,//undefined device
};
	
//将usb device初始化为指定设备
void usbdev_init(int sel);

//切换usb device
void usbdev_switch(int sel);

//关闭当前设备
void usbdev_close();

uint8_t usbd_active_get(void);
uint8_t usbd1_active_get(void);
void usb_conn_det_proc(void);

//EOF
#endif

