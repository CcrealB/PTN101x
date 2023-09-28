/*************************************************************
 * @file		usb_desc.c
 * @brief		USB descriptors ,discribed USB spec1.1 chapter 9
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2020-12-03
 * @par
 * @attention
 *
 * @history		2020-12-03 jkg	create this file
 */

/*file type declaration section*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*internal symbols import section*/
#include "..\..\usb\usbclass.h"
#include "..\..\usb\usb.h"
#include "..\..\usb\usbdef.h"
#include "..\..\driver_usb.h"
#include "bot.h"

extern void*UsbDesc_Find(int idx,void*tb,int cnt);
static
const uint16_t tUsbLanguageID_udisk[]={
	0x04+(USB_DESCTYPE_STRING<<8),
	_LANGUAGE_ID_
};

static
const uint16_t tUsbProductStr_udisk[]={
	(((13+1)*2))+(USB_DESCTYPE_STRING<<8),'S','D','C','a','r','d',' ','R','e','a','d','e','r',
};
static
const uint16_t tUsbVendorStr_udisk[]={
	// (((11+1)*2))+(USB_DESCTYPE_STRING<<8),'B','e','k','e','n',' ','C','o','r','p','.'
	(((13+1)*2))+(USB_DESCTYPE_STRING<<8),'P','a','r','t','n','e','r',' ','C','o','r','p','.'
};
static
const uint16_t tUsbSnStr_udisk[]={
	(((10+1)*2))+(USB_DESCTYPE_STRING<<8),'1','2','3','4','3','3','3','3','3','3'
};
static
const CUsbDevDesc tUsbDeviceDescriptor_udisk={
	sizeof(CUsbDevDesc),
	USB_DESCTYPE_DEVICE,
	USB_VERSION,
	USB_DEVICE_CLASS(0),
	USB_DEVICE_SUBCLASS(0),
	USB_DEVICE_PROTOCOL(0),
	USB_MAX_PACKET_LEN(64),
	_UDISK_VID_,
	_UDISK_PID_,
	0x0101,
	_Vendor_String_ID_,
	_Product_String_ID_,
	_Sn_String_ID_,
	USB_DEVICE_CONFIG_NUM(USB_MAX_CONFIG_NUM)
};

static
const uint8_t tUsbConfig_udisk[]={
	USB_Gen_Config(9*2+7*2, USB_MAX_INTERFACE_NUM, 100),
	USB_INTERFACE_DESC(0, 0, 2, USB_DEVICE_CLASS_MASS, 0x06, 0x50),
	USB_ENDPOINT_DESC(1, USB_ENDPID_BULK_IN, USB_ENDP_BULK, 64, 0),
	USB_ENDPOINT_DESC(0, USB_ENDPID_BULK_OUT, USB_ENDP_BULK, 64, 0),
};

static const CBufferBaseDesc _c_device_desc_udisk={(void*)&tUsbDeviceDescriptor_udisk,sizeof(tUsbDeviceDescriptor_udisk)};
static const CBufferBaseDesc _c_config_desc_udisk={(void*)tUsbConfig_udisk,sizeof(tUsbConfig_udisk)};
static const _t_index_descriptor tblUsbStringDescs_udisk[]={
	{_Language_String_ID_,{(void*)tUsbLanguageID_udisk,sizeof(tUsbLanguageID_udisk)}},
	{_Vendor_String_ID_,{(void*)tUsbVendorStr_udisk,sizeof(tUsbVendorStr_udisk)}},
	{_Product_String_ID_,{(void*)tUsbProductStr_udisk,sizeof(tUsbProductStr_udisk)}},
	{_Sn_String_ID_,{(void*)tUsbSnStr_udisk,sizeof(tUsbSnStr_udisk)}},
};

void*udisk_GetDeviceDesc(void){
	return (void*)&_c_device_desc_udisk;
}

void*udisk_GetConfigDesc(void){
	return (void*)&_c_config_desc_udisk;
}

void*udisk_GetStringDesc(int idx){
	return UsbDesc_Find(idx,(void*)tblUsbStringDescs_udisk,GET_ELEMENT_TBL(tblUsbStringDescs_udisk));
}

