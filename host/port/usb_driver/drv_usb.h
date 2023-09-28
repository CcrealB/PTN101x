#ifndef _DRV_USB_H_
#define _DRV_USB_H_
#define USBDEV_BASE_PTR		(void*)0x01052000
#define USBHOST_BASE_PTR		(void*)0x01062000
//#define USBHOST_BASE_PTR		USBDEV_BASE_PTR

typedef void* (*CBK_USB_GET_DEVICE_DESC)();
typedef void* (*CBK_USB_GET_CONFIG_DESC)();
typedef void* (*CBK_USB_GET_STRING_DESC)(int idx);
typedef void* (*CBK_USB_GET_HIDREPORT_DESC)(int idx);
typedef void (*CBK_USB_OPEN)(void);
typedef void (*CBK_USB_CLOSE)(void);
typedef int (*CBK_USB_CLASS_CHECK)(void*setup_pkt,int sz);
typedef int (*CBK_USB_PIPE_CHECK)(void*setup_pkt,int sz);
typedef void* (*CBK_USB_CLASS_CTRL_IN)(void*setup_pkt,int sz);
typedef void* (*CBK_USB_CLASS_CTRL_OUT)(void*setup_pkt,int sz);
typedef void* (*CBK_USB_CLASS_PIPE_IN)(void*setup_pkt,int sz);
typedef void* (*CBK_USB_CLASS_PIPE_OUT)(void*setup_pkt,int sz);

#define DbgPrintf		os_printf
#define EnterFunc()		DbgPrintf("%s\r\n",__FUNCTION__);
#define ExitFunc()		DbgPrintf("End %s\r\n",__FUNCTION__);
#define DbgPoint()		DbgPrintf("%s:%d: \r\n",__FILE__,__LINE__);
#define RegDbg(reg)		DbgPrintf(#reg##"=%.8x\r\n",reg);
//extern int os_printf(const char * fmt,...);

#if 0
#include"usb\usbdef.h"
typedef struct{
	CBufferBaseDesc rxBuf;//接收buf
	CBufferBaseDesc txBuf;//发送buf
	union{
		CBK_USBAPP txcbk;//发送结束回调函数
		CBK_USBAPP txCompleted;//发送结束回调函数
		CBK_USBAPP_R rxPrecbk;//收包前回调函数,函数须有整形值返回，0=正常返回，非0=异常返回
	};
	union{
		CBK_USBAPP rxcbk;//收包后回调函数
		CBK_USBAPP rxCompleted;//收包结束回调函数
	};
}CUsbAppIntf;
//callback for USB Standard Request SET_INTERFACE
typedef void (*CBK_SET_INTERFACE)(int intf_no,int alt_intf);

typedef struct{//usb hardware abstract layer driver
	CUsbAppIntf *pipIntf;
	CBK_SET_INTERFACE *cbkSetInterface;
	unsigned char*bUsbAlterInterface;
	unsigned short int*usb_endp_status;
	unsigned char bUsbCntOfEndp;//counter of endpoint
	unsigned char bUsbCntOfIntf;//counter of interface
	unsigned char ep0PktMaxSz;
	volatile unsigned char bIsConfig;
}_t_usb_host_drv,_t_usb_dev_drv,_t_usb_hal_drv;
#endif
typedef struct {
	void*bp;//usb base pointer
	CBK_USB_OPEN open;//open usb device
	CBK_USB_CLOSE close;//close usb device
	CBK_USB_GET_DEVICE_DESC f_get_device_desc;//get buffer desc of the device descriptor
	CBK_USB_GET_CONFIG_DESC f_get_config_desc;//get buffer desc of the config descriptor
	CBK_USB_GET_STRING_DESC f_get_string_desc;//get buffer desc of the string descriptor
	CBK_USB_GET_HIDREPORT_DESC f_get_hidreport_desc;//get buffer desc of the hid report descriptor

	CBK_USB_CLASS_CHECK is_class_cmd;//检查指定命令是否为class驱动支持的命令
	CBK_USB_CLASS_CTRL_IN get_ctrl_in_db;//获取指定命令回应数据的缓冲区描述
	CBK_USB_CLASS_CTRL_OUT get_ctrl_out_db;//获取指定命令接收数据的缓冲区描述
	CBK_USB_PIPE_CHECK is_pipe_cmd;//检查指定命令是否为pipe驱动支持的命令
	CBK_USB_CLASS_PIPE_IN get_pipe_in_db;//获取指定命令pipe回应数据的缓冲区描述
	CBK_USB_CLASS_PIPE_OUT get_pipe_out_db;//获取指定命令pipe接收数据的缓冲区描述

}_t_drv_usb_device;

void usb_hal_drv_init(void*drv,int maxIntf,int maxEndp);
extern void*get_usb_dev_handle();
extern void load_usb_drv(void*drv);

#endif
