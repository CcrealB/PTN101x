#include <stdio.h>
#include <stddef.h>     // standard definition
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "drv_usb.h"
#include "usb\usbdef.h"
// #include "types.h"
#include "api_usb.h"
#include "u_config.h"

extern const _t_drv_usb_device cDrvUdisk;
extern const _t_drv_usb_device cDrvAudio_hid;
extern void _pre_analog_config_desc();
extern uint16_t usbd_frame_num_get(uint8_t USBx);
static int s_usb_sel;

typedef _t_drv_usb_device *_p_t_drv_usb_device;
const _p_t_drv_usb_device tblUsbDrvs[]={
	(_t_drv_usb_device *)&cDrvUdisk,
	(_t_drv_usb_device *)&cDrvAudio_hid,
};
extern void AplUsbDev_BuildDrv();
void usbdev_select(int sel){
	if(sel>=(GET_ELEMENT_TBL(tblUsbDrvs)))return;
	EnterFunc();
	s_usb_sel=sel;
	
	//usbdev_print(get_usb_dev_handle());
	AplUsbDev_BuildDrv();
	load_usb_drv(tblUsbDrvs[s_usb_sel]);
	usbdev_print(get_usb_dev_handle());
	_pre_analog_config_desc();
	
	ExitFunc();
//	if(sel==0){
//		load_usb_drv((void *)&cDrvUdisk);
//		_pre_analog_config_desc();
//	}else if(sel==1){
//		load_usb_drv((void *)&cDrvAudio_hid);
//		_pre_analog_config_desc();
//	}
}

void usbdev_switch(int sel){
	if(sel>1)return;
	if(sel==s_usb_sel)return;
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	pusb->close();
	usbdev_select(sel);
	pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	pusb->open();
}

void usbdev_init(int sel){
	usbdev_select(sel);	
	((_t_drv_usb_device*)get_usb_dev_handle())->open();
}

void usbdev_close(){
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)get_usb_dev_handle();
	pusb->close();
}

static uint8_t usb_conn_state = 0;
uint8_t usbd_active_get(void) { return usb_conn_state & 1; }
uint8_t usbd1_active_get(void) { return usb_conn_state & 2; }

//pls place in >2ms loop
void usb_conn_det_proc(void)
{
#if USB0_FUNC_SEL
    static uint16_t last_frame = 0;
    uint16_t frame = usbd_frame_num_get(0);
    if(frame != last_frame) {
        last_frame = frame;
        usb_conn_state |= 1;
    } else {
        usb_conn_state &= ~1;
    }
#endif
#if USB1_FUNC_SEL
    static uint16_t last_frame1 = 0;
    uint16_t frame1 = usbd_frame_num_get(1);
    if(frame1 != last_frame1) {
        last_frame1 = frame1;
        usb_conn_state |= 2;
    } else {
        usb_conn_state &= ~2;
    }
#endif
}



void app_usb_init(void)
{
#if (USB0_FUNC_SEL & USBD_CLS_HID)
    usbdev_init(USBDEV_AUDIO_HID);
#endif

#if 0//(USB1_FUNC_SEL == USBD_CLS_AUDIO_HID)//not surpport yet
    usbdev_init(USBDEV_AUDIO_HID);
#endif

#if (USB0_FUNC_SEL == USBH_CLS_DISK || USB1_FUNC_SEL == USBH_CLS_DISK)
    extern void usbhost_open(int usb_sel);
    extern void load_bot_drv();
    // extern void bot_set_lost_deviceCbk(void*cbk);
    #if IC_MODEL == IC_PTN1012
    int USB_Px = (USB0_FUNC_SEL == USBH_CLS_DISK) ? USBPORT_2 : USBPORT_1;
   	usbhost_open(USB_Px);
    #else
   	usbhost_open(USBPORT_0);
    #endif
	load_bot_drv();
	// bot_set_lost_deviceCbk(/*Unload_Clear_Player*/NULL);
#endif
}
//EOF

