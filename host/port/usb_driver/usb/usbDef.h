/*************************************************************
 * @file		usbDef.h
 * @brief		Usb protocol defintions by USB spec1.1
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2020-12-03
 * @par
 * @attention
 *
 * @history		2020-12-03 jkg	create this file
 */

#ifndef _USBDEF_H_ //this file's unique symbol checking section
#define _USBDEF_H_
#include <stdint.h>
// #include "..\types.h"
#include "usb.h"
#include "usb_include.h"

/*constant define section*/
/*data format define*/
#define BYTE2(a)				((a)&0xff),((a>>8)&0xff)
#define WordData(dat)			((dat)&0xff),(((dat)>>8)&0xff)
#define Byte3Data(dat)			((dat)&0xff),(((dat)>>8)&0xff),(((dat)>>16)&0xff)
#define Byte4Data(dat)			((dat)&0xff),(((dat)>>8)&0xff),(((dat)>>16)&0xff),(((dat)>>24)&0xff)
#define Comb2Byte(b0,b1)		((b0)|((b1)<<8))
#define Comb3Byte(b0,b1,b2)		((b0)|((b1)<<8)|((b2)<<16))
#define Comb4Byte(b0,b1,b2,b3)	\
	((b0)|((b1)<<8)|((b2)<<16)|((b3)<<24))

#define MUSB_SWAP16(_data) (((_data) << 8) | ((_data) >> 8))
#define MUSB_MSB(_data)    (uint8_t)(((_data) >> 8) & 0xFF)
#define MUSB_LSB(_data)    (uint8_t)((_data) & 0xFF)

/*manufacturer info*/
#define _SELF_POWER_			0
#define _REMOTE_WAKEUP_			1

	//UNICODE language ID
#define _LANGUAGE_ID_			0x0409
/*usb version*/
#define USB_VERSION				0x0110

/*standard usb desriptor type define*/
#define USB_DESCTYPE_DEVICE		0x01
#define USB_DESCTYPE_CONFIG		0x02
#define USB_DESCTYPE_STRING		0x03
#define USB_DESCTYPE_INTERFACE	0x04
#define USB_DESCTYPE_ENDPOINT	0x05

/*usb-if device class define*/
#define USB_DEVICE_CLASS(cls)		(cls)
#define USB_DEVICE_CLASS_AUDIO		0x01
#define USB_DEVICE_CLASS_MASS		0x08
#define USB_DEVICE_CLASS_HUB
#define USB_DEVICE_CLASS_HID		0x03

#define USB_DEVICE_SUBCLASS(subcls)		(subcls)

#define USB_DEVICE_PROTOCOL(prt)		(prt)

#define USB_PID_VID(vid,pid)			(((pid)<<16)+(vid))
#define USB_DEVICE_VERSION(ver)			((ver)&0xff),(((ver)>>8)&0xff)
#define USB_DEVICE_CONFIG_NUM(num)		(num)

/**/
//定义配置的信息长度
#define USB_CONFIG_TOTAL_LEN(len)		(len)
#define USB_CONFIG_INTF_NUM(num)		(num)
//定义usb端点支持的最大封包的大小
#define USB_MAX_PACKET_LEN(len)			(len)

//定义usb对象的序号
#define USB_OBJECT_NO(no)				(no)
#define USB_STRING_INDEX(idx)			(idx)

#define USB_CONFIG_ATTRIBUTE(selfpwr,remote_wakeup)		\
	(0x80|((selfpwr)<<6)|((remote_wakeup)<<5))
#define USB_CONFIG_MAX_POWER(pwr)		((pwr)>>1)

#define USB_INTF_ALTERSETTING(no)		(no)
#define USB_INTF_ENDP_NUM(num)			(num)
#define USB_INTF_CLASS(cls)				(cls)
#define USB_INTF_SUBCLASS(scls)			(scls)
#define USB_INTF_PROTOCOL(prt)			(prt)

#define USB_ENDP_ADDRESS(inp,addr)		\
	(((inp)<<7)|(addr))
#define USB_ENDP_ATTRIBUTE(attr)		(attr)
#define USB_ENDP_CTRL					0x00
#define USB_ENDP_ISO					0x01
#define USB_ENDP_BULK					0x02
#define USB_ENDP_INT					0x03
#define USB_ENDP_INTERVAL(n_ms)			(n_ms)

//USB Request define
#define USB_RT_DIR_MASK				0x80
#define USB_RT_DIR_OUT				0x00
#define USB_RT_DIR_IN				0x80
#define USB_RT_DIR_Host2Dev			0x00
#define USB_RT_DIR_Dev2Host			0x80

#define USB_RT_TYP_MASK				0x60
#define USB_RT_TYP_STD				0x00
#define USB_RT_TYP_CLASS			0x20
#define USB_RT_TYP_VENDOR			0x40


#define USB_RT_RECIPT_MASK			0x1f
#define USB_RT_RECIPIENT_DEVICE		0x00
#define USB_RT_RECIPIENT_INTF		0x01
#define USB_RT_RECIPIENT_ENDP		0x02
#define USB_RT_RECIPIENT_OTHER		0x03
#define FIELD_USB_RT_RECIPIENT		0,5

//USB std request cmd
#define USB_GET_STATUS					0x00
#define USB_GET_DESCRIPTOR				0x06
#define USB_CLEAR_FEATURE				0x01
#define USB_SET_FEATURE					0x03
#define USB_SET_ADDRESS					0x05
#define USB_GET_CONFIGURATION			0x08
#define USB_SET_CONFIGURATION			0x09
#define USB_GET_INTERFACE				0x0a
#define USB_SET_INTERFACE				0x0b

//define a device descriptor
/*
#define USB_DEVICE_DESC(ver,clss,vid,pid,maxp,iMan,iPro,iSer)	\
	0x12,\
	USB_DESCTYPE_DEVICE,\
	WordData(USB_VERSION),\
	0,0,clss,\
	USB_MAX_PACKET_LEN(maxp),\
	WordData(vid),\
	WordData(pid),\
	WordData(ver),\
	iMan,iPro,iSer,\
	USB_DEVICE_CONFIG_NUM(USB_MAX_CONFIG_NUM)
*/
#define USB_DEVICE_MSGS(iMan,iPro,iSer)		\
	iMan,iPro,iSer
#define USB_DEVICE_CLASSDESC(cls,scls,prt)	\
	cls,scls,prt

//define a config descriptor
#define USB_CONFIG_DESC(szCfg,n_intf,pwr)	\
	0x09,\
	USB_DESCTYPE_CONFIG,\
	BYTE2(/*sizeof*/(szCfg)),\
	USB_CONFIG_INTF_NUM(n_intf),\
	USB_OBJECT_NO(1),\
	USB_STRING_INDEX(0),\
	USB_CONFIG_ATTRIBUTE(0,0),\
	USB_CONFIG_MAX_POWER(pwr)

//define a interface descriptor
#define USB_INTERFACE_DESC(oid,aid,n_endp,cls,sub_cls,prt)	\
	9,\
	USB_DESCTYPE_INTERFACE,\
	USB_OBJECT_NO(oid),\
	USB_INTF_ALTERSETTING(aid),\
	USB_INTF_ENDP_NUM(n_endp),\
	USB_INTF_CLASS(cls),\
	USB_INTF_SUBCLASS(sub_cls),\
	USB_INTF_PROTOCOL(prt),/*1=keyboard,2=mouse*/\
	USB_STRING_INDEX(0)

//define a endpoint descriptor
#define USB_ENDPOINT_DESC(i_o,epn,tra_typ,maxp,inv)	\
	7,\
	USB_DESCTYPE_ENDPOINT,\
	USB_ENDP_ADDRESS(i_o,epn),\
	USB_ENDP_ATTRIBUTE(tra_typ),\
	BYTE2(maxp),\
	USB_ENDP_INTERVAL(inv)

/*hardware define section*/

/*function macro define section*/

/*struct and union define(export) section*/
//定义usb 的状态
#define USB_STATE_INIT			0x00	/*初始化*/
#define USB_STATE_ATTACHED		0x01	/*连接到host*/
#define USB_STATE_POWERED		0x02	/*上电*/
#define USB_STATE_DEFAULT		0x03	/*使用缺省地址*/
#define USB_STATE_ADDRESS		0x04	/*配址中*/
#define USB_STATE_ADDRESSED		0x08	/*可寻址*/

#define USB_STATE_CONFIG		0x05	/*已配置*/
#define USB_STATE_USING			0x06	/*使用中,config 100ms之后进入该状态*/
#define USB_STATE_SLEEP			0x07	/*睡眠*/

#define SET_USBSTATE(ust)	\
	(bUsbState=(ust))

//UNS:USB Next Stage
#define UNS_Undef		0
#define UNS_DataIn		1
#define UNS_DataOut		2
#define UNS_StatusOut	3
#define UNS_StatusIn	4
#define UNS_Complete	5

#define UNS_SET(endp,stg)		\
	(((CEndpoint*)endp)->stage)=stg

#define UNS_GET(endp)		\
	(((CEndpoint*)endp)->stage)

#define USB_EndpStt_Tx		BIT(0)
#define USB_EndpStt_Rx		BIT(1)
#define USB_EndpStt_TxExCfg		BIT(2)
//endpoint start TX extend bit,用在发送config 描述符时，自动获取config描述符的总长度
//在发送config描述符时，会先写4个字节的数据，包含config的前4个字节


/*struct define section*/

#define MAX_EVENT_USBHOST		16
enum{
	E_USB_EVENT_EP_NUM=16,
	E_USB_EVENT_None=0,
	E_USB_EVENT_DEV_CONNECTED=1,
	E_USB_EVENT_DEV_DISCONNECTED,
	E_USB_EVENT_SOF,
	E_USB_EVENT_DEV_SUSPEND,
	E_USB_EVENT_DEV_RESUME,
	E_USB_EVENT_DEV_EPn_TxCompletion,//每个ep对应一个事件
	E_USB_EVENT_DEV_EPn_RxCompletion=E_USB_EVENT_DEV_EPn_TxCompletion+E_USB_EVENT_EP_NUM,
};

//USB数据的传输为小端序方式
typedef  struct  usb_setup_req
{
  uint8_t   bmRequest;
  uint8_t   bRequest;
  uint16_t  wValue;
  uint16_t  wIndex;
  uint16_t  wLength;
} USBD_SetupReqTypedef;

typedef unsigned int _t_usb_msg;
typedef struct _t_usb_event{
	_t_usb_msg msg;
	void*para;
}_t_usb_event;
typedef struct {
	int cnt;
	int top;//core:RO,app:WO
	int tail;//core:WO,app:RO
	_t_usb_event events[MAX_EVENT_USBHOST];//core:WO,app:RO
}_t_usb_event_pool;

typedef struct CBufferBaseDesc{
	void*ptr;
	int sz;
}CBufferBaseDesc;


typedef struct CEndpoint{
	uint8_t* txPtr;
	uint8_t* rxPtr;
	#ifdef _USB_DEVICE_
	union{//如果是USB_DEVICE，采用单向收发
	#endif
		uint16_t rxCursor;
		uint16_t txLen;
	#ifdef _USB_DEVICE_
		};
	#endif
	
	#ifdef _USB_DEVICE_
	union{
	#endif
		uint16_t txCursor;
		uint16_t rxLen;
	#ifdef _USB_DEVICE_	
		};
	#endif
	uint16_t lmtLen;
	uint8_t stage;
	uint8_t status;
}CEndpoint;

#pragma pack(1)

typedef union CUsbSetupPkt{
	struct{
		uint16_t cmd;
		uint16_t tar;
		uint16_t idx;
		uint16_t len;
	}/*pkt*/;
	uint8_t dat[8];
}CUsbSetupPkt;
typedef struct CUsbDevDesc{
	uint8_t len;
	uint8_t devType;
	uint16_t usbVer;
	uint8_t clsCode;
	uint8_t subClsCode;
	uint8_t prtCode;
	uint8_t maxPkt;
	uint16_t vid;
	uint16_t pid;
	uint16_t devVer;
	uint8_t iVendor;
	uint8_t iProd;
	uint8_t iSerial;
	uint8_t nConfig;
}CUsbDevDesc;

typedef struct CUsbConfigDesc{
	uint8_t len;
	uint8_t devType;
	uint16_t wTotalLength;
	uint8_t bNumberOfIntf;
	uint8_t bConfigValue;
	uint8_t iConfig;
	uint8_t bmAttribute;
	uint8_t MaxPower;
}CUsbConfigDesc;

typedef struct CUsbIntfDesc{
	uint8_t len;
	uint8_t devType;
	uint8_t bIntfNo;
	uint8_t bAlertIntf;
	uint8_t bNumberOfEndp;
	uint8_t bIntfClass;
	uint8_t bIntfSubClass;
	uint8_t bIntfProtool;
	uint8_t iIntf;
}CUsbIntfDesc;

typedef struct CHidClassDesc{
	uint8_t len;
	uint8_t devType;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDesc;
	uint8_t bDescType;
	uint16_t wDescLength;
}CHidClassDesc;

typedef struct CUsbEndpDesc{
	uint8_t len;
	uint8_t devType;
	uint8_t bEndpAddr;
	uint8_t bmAttribute;
	uint16_t wMaxPktSz;
	uint8_t bInterval;
}CUsbEndpDesc;
/*
typedef struct CUsbConfig{
	CUsbConfigDesc cfg;
	CUsbIntfDesc dbgIntf;
	CHidClassDesc dbgHidCls;
	CUsbEndpDesc dbgEndp;
	CUsbIntfDesc mseIntf;
	CHidClassDesc mseHidCls;
	CUsbEndpDesc mseEndp;
}CUsbConfig;
*/
typedef struct CUsbCommonDesc{
	uint8_t len;
	uint8_t descType;
	uint8_t dat[0];
}CUsbCommonDesc;

#pragma pack()

typedef struct {
	int ind;
	CBufferBaseDesc desc;
} _t_index_descriptor;

typedef struct{
	CUsbSetupPkt*setupPkt;
	CEndpoint endp;
}_t_usbhost_transfer;

typedef void (*CBK_USBAPP)(void*buf,int  sz);
typedef void (*CBK_USB_SUSPEND)(void);
typedef int (*CBK_USBAPP_R)(void*buf,int  sz);
typedef void (*CCallback_P0)();

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
/*end file*/
