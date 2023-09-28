
#include "usb_include.h"
#include "drv_usb.h"
#include "usb\usbDef.h"

void usbdev_print(void*drv)
{
    #ifdef USB_DEBUG
	_t_drv_usb_device*pusb=(_t_drv_usb_device*)drv;
	USB_PRINTF("drv@=%.8x\r\n", (unsigned int)drv);
	USB_PRINTF("pusb->open@=%p\r\n", pusb->open);
	USB_PRINTF("pusb->close@=%p\r\n", pusb->close);
	USB_PRINTF("pusb->f_get_device_desc@=%p\r\n", pusb->f_get_device_desc);	
	USB_PRINTF("pusb->f_get_config_desc@=%p\r\n", pusb->f_get_config_desc);	
	USB_PRINTF("pusb->f_get_string_desc@=%p\r\n", pusb->f_get_string_desc);
	USB_PRINTF("pusb->f_get_hidreport_desc@=%p\r\n", pusb->f_get_hidreport_desc);

	USB_PRINTF("pusb->is_class_cmd@=%p\r\n", pusb->is_class_cmd);
	USB_PRINTF("pusb->get_ctrl_in_db@=%p\r\n", pusb->get_ctrl_in_db);
	USB_PRINTF("pusb->get_ctrl_out_db@=%p\r\n", pusb->get_ctrl_out_db);	
	USB_PRINTF("pusb->is_pipe_cmd@=%p\r\n", pusb->is_pipe_cmd);	
	USB_PRINTF("pusb->get_pipe_in_db@=%p\r\n", pusb->get_pipe_in_db);
	USB_PRINTF("pusb->get_pipe_out_db@=%p\r\n", pusb->get_pipe_out_db);
    #endif
}

void usb_dbg_setup_show(const char* msg, void* setup_pkt, int sz)
{
    #ifdef USB_DEBUG
    USBD_SetupReqTypedef* setup = setup_pkt;
    USB_PRINTF("%s(%d): bmReq:%02X bReq:%02X\t", msg, sz, setup->bmRequest, setup->bRequest);
    USB_PRINTF("val:%04X idx:%04X len:%04X\n", setup->wValue, setup->wIndex, setup->wLength);
    #endif
}

void usb_dbg_buf_show(void*msg,void*dat,int len)
{
    #ifdef USB_DEBUG
	unsigned char*ptr=(unsigned char*)dat;
	int i;
	USB_PRINTF("%s[%d]={", msg, len);
	for(i=0;i<len;i++){
		if((i&0x07)==0x0)USB_PRINTF("\r\n\t");
		USB_PRINTF("%.2x ",ptr[i]);
	}
	USB_PRINTF("\r\n}\r\n");
    #endif
}
