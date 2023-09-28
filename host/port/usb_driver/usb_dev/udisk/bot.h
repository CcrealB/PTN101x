#ifndef _BOT_H_
#define _BOT_H_

#define USB_MAX_CONFIG_NUM				1

#define _UDISK_VID_					0x0738
#define _UDISK_PID_					0x0202

//#define _Vendor_String_ID_		0x01
//#define _Product_String_ID_		0x02
//#define _Sn_String_ID_			0x03
enum{
	_Language_String_ID_=0,
	_Vendor_String_ID_,
	_Product_String_ID_,
	_Sn_String_ID_
};

enum {
	USB_INTFID_BOT=0,
	USB_MAX_INTERFACE_NUM,
};

enum{
	USB_ENDPID_Default=0,
	USB_ENDPID_BULK_OUT,
	USB_ENDPID_BULK_IN,
	USB_MAX_ENDPOINT_NUM,
};
//注意：
//3435\3431Q usb endpoint个数为5（0~4）， fifo大小为64Bytes，TXMAXP，RXMAXP最大值为8
#endif
