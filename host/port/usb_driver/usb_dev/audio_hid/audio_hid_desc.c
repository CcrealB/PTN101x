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
#include "audio_hid.h"

#if 1
static
const uint8_t tHidMSEReportDesc_aud_hid[]={
	HidUsagePage(HID_USAGEPAGE_Customer),
	HidUsage(0x01), 
	HidCollectionApp(),
		/*volume control*/
		HidUsage(HUC_VolumnMute),
		HidUsage(HUC_VolumnInc),
		HidUsage(HUC_VolumnDec),
		HidLogiMin8(0),
		HidLogiMax8(1),
		HidReportBits(1),
		HidReportCount(3),
		HidInput(0, 1, 0),
		
		HidReportBits(1),
		HidReportCount(5),
		HidInput(1, 1, 0),
		
		HidUsage(HUC_MediaPlay),
		HidUsage(HUC_MediaStop),
		HidUsage(HUC_MediaPause),
		HidUsage(HUC_MediaNext),
		HidUsage(HUC_MediaPrev),
		HidUsage(HUC_MediaRecord),
		HidUsage(HUC_MediaFastFwd),
		HidUsage(HUC_MediaRewind),
		HidReportBits(1),
		HidReportCount(8),
		HidInput(0, 1, 0),

		HidUsage(0x00),
		HidReportBits(8),
		HidReportCount(6),
		HidInput(0, 1, 0),

		HidUsage(0x00),
		HidReportBits(8),
		HidReportCount(8),
		HidOutput(0, 0, 0),
		
	HidEndCollection()
};
#endif
#if 0
const uint8_t tHidMSEReportDesc[]={
	HidUsagePage(HID_USAGEPAGE_Generic),
	HidUsage(HID_USAGE_Mouse),
	HidCollectionApp(),
	HidUsage(HID_USAGE_Pointer),
	HidCollectionPhy(),
	HidReportID(REPORTID_REL_MOUSE),
	HidUsagePage(HID_USAGEPAGE_Buttons),
	HidUsageMin8(1),
	HidUsageMax8(8),
	HidLogiMin8(0),
	HidLogiMax8(1),
	HidReportBits(1),
	HidReportCount(8),
	HidInput(0, 1, 0),

	HidUsagePage(HID_USAGEPAGE_Generic),
	HidUsage(HID_USAGE_AxisX),
	HidUsage(HID_USAGE_AxisY),
	HidUsage(HID_USAGE_AxisZ),
	HidLogiMin16(-32768),
	HidLogiMax16(32767),
	HidReportBits(16),
	HidReportCount(3),
	HidInput(0, 1, 1),
	HidEndCollection(),
	HidEndCollection(),


	HidUsage(HID_USAGE_Mouse),
	HidCollectionApp(),
	HidUsage(HID_USAGE_Pointer),
	HidCollectionPhy(),
	HidReportID(REPORTID_ABS_MOUSE),
	HidUsagePage(HID_USAGEPAGE_Buttons),
	HidUsageMin8(1),
	HidUsageMax8(8),
	HidLogiMin8(0),
	HidLogiMax8(1),
	HidReportBits(1),
	HidReportCount(8),
	HidInput(0, 1, 0),

	HidUsagePage(HID_USAGEPAGE_Generic),
	HidUsage(HID_USAGE_AxisX),
	HidUsage(HID_USAGE_AxisY),
	HidUsage(HID_USAGE_AxisZ),
	HidLogiMin16(0),
	HidLogiMax16(32767),
	HidReportBits(16),
	HidReportCount(3),
	HidInput(0, 1, 0),
	HidEndCollection(),
	HidEndCollection(),
};
#endif

static
const uint8_t tHidDbgReportDesc_aud_hid[]={
	HidUsagePage(HID_USAGEPAGE_Customer),
	HidUsage(0x01),
	HidCollectionApp(),
	HidUsage(0x00),
	HidReportBits(8),
	HidReportCount(32),	//yuan 64->32
	HidLogiMin8(0),
	HidLogiMax16(0xff),
	HidInput(0, 1, 0),
	HidUsage(0x00),
	HidReportBits(8),
	HidReportCount(32),	//yuan 64->32
	HidLogiMin8(0),
	HidLogiMax16(0xff),
	HidOutput(0, 1, 0),
	HidEndCollection()
};
static
const uint16_t tUsbLanguageID_aud_hid[]={
	0x04+(USB_DESCTYPE_STRING<<8),
	_LANGUAGE_ID_
};

#include "u_config.h"
static
const uint16_t tUsbProductStr_aud_hid[]={
//	(((14+1)*2))+(USB_DESCTYPE_STRING<<8),'H','I','D','&','A','u','d','i','o',' ','T','e','s','t'
#if defined(ZY_OA_001)
	((( 17+1)*2))+(USB_DESCTYPE_STRING<<8),'S','o','n','g','B','i','r','d','-','H','Q',' ','A','u','d','i','o'	//yuan
//#elif defined(YQ_DEMO)
#elif defined(XFN_S930)
	((( 4+1)*2))+(USB_DESCTYPE_STRING<<8),'S','9','3','0'
#elif defined(MZ_200K)
	((( 9+1)*2))+(USB_DESCTYPE_STRING<<8),'U','s','b',' ','A','u','d','i','o'
#elif defined(ZT_M184)
	((( 6+1)*2))+(USB_DESCTYPE_STRING<<8),'M','Z','_','1','8','4'
#else
	((( 9+1)*2))+(USB_DESCTYPE_STRING<<8),'P','T','N','.','A','u','d','i','o'	//yuan
#endif
};

static
const uint16_t tUsbVendorStr_aud_hid[]={
//	(((11+1)*2))+(USB_DESCTYPE_STRING<<8),'B','e','k','e','n',' ','C','o','r','p','.'
	(((13+1)*2))+(USB_DESCTYPE_STRING<<8),'P','a','r','t','n','e','r',' ','C','o','r','p','.'	//yuan
};
static
const uint16_t tUsbSnStr_aud_hid[]={
	(((10+1)*2))+(USB_DESCTYPE_STRING<<8),'1','2','3','4','3','3','3','3','3','3'
};

static
const CUsbDevDesc tUsbDeviceDescriptor_aud_hid={
	sizeof(CUsbDevDesc),
	USB_DESCTYPE_DEVICE,
	USB_VERSION,
	USB_DEVICE_CLASS(0),
	USB_DEVICE_SUBCLASS(0),
	USB_DEVICE_PROTOCOL(0),
	USB_MAX_PACKET_LEN(64),
	_AUD_HID_VID_,
	_AUD_HID_PID_,
	0x0101,
	_Vendor_String_ID_,
	_Product_String_ID_,
	_Sn_String_ID_,
	USB_DEVICE_CONFIG_NUM(USB_MAX_CONFIG_NUM)
};

#if 1
#if AUDIO_MIC_CHNs == 1
    #define USB_ACS_ACI_FU_MIC(uid,sid)  USB_ACS_ACI_FU_Mono(uid,sid)
#elif AUDIO_MIC_CHNs == 2
    #define USB_ACS_ACI_FU_MIC(uid,sid)  USB_ACS_ACI_FU_Stereo(uid,sid)
#endif

/*
*MIC+SPK+Mouse+HID vendor IO
*/
#define SZ_Audio_ACI_FU(chns)  (6+((chns+1)*2)+1)
#define SZ_Audio_ACI(nIntfAs)				9/*Interface AC*/+\
	8+((nIntfAs))+/*Header*/+\
	12	/*IT(1)*/+\
	12	/*IT(2)*/+\
	9	/*OT(1)*/+\
	9	/*OT(2)*/+\
	0	/*FU(1)*/+\
	SZ_Audio_ACI_FU(AUDIO_SPK_CHNs)	/*FU(2)*/+\
	0

#define SZ_Audio_ASI(nSR)				9+/*Interface AS Bw0*/+\
	9	/*Interface AS Normal*/+\
	7	/*AS General*/+\
	8+((nSR)*3)	/*AS FormatI*/+\
	9	/*Endpoint for Audio class*/+\
	7	/*Endpoint AS General*/+\
	0

#define SZ_HID_Intf(nEndp)				9+/*HID Interface descriptor*/+\
	9	/*HID class descriptor*/+\
	7*(nEndp)	/*Endpoint descriptor*/+\
	0

#define SZ_CONFIG(nMicSR,nSpkSR,nIntfAs)		9/*config*/+\
	SZ_Audio_ACI(nIntfAs)+\
	SZ_Audio_ASI(nMicSR)/*MIC descs size*/+\
	SZ_Audio_ASI(nSpkSR)/*SPK descs size*/+\
	SZ_HID_Intf(1) +\
	SZ_HID_Intf(2) +\
	0

static
const uint8_t tUsbConfig_aud_hid[]={
	USB_Gen_Config(SZ_CONFIG(1,1,2), USB_MAX_INTERFACE_NUM, 100),
	USB_AUDIO_InterfaceAC(USB_INTFID_AudioAC),
	USB_ACS_ACI_Header(SZ_Audio_ACI(2),2),USB_INTFID_AudioAS_MIC,USB_INTFID_AudioAS_SPK,
	USB_ACS_ACI_IT(USBAUDIO_IT_MIC,0,AUDIO_TERMTYPE_IT_MIC,2,AUDIO_CHANNEL_CONFIG(1,1,0,0,0,0,0,0,0,0,0,0)),
    USB_ACS_ACI_FU_MIC(USBAUDIO_FU_MIC,USBAUDIO_IT_MIC),
	USB_ACS_ACI_OT(USBAUDIO_OT_MIC,0,AUDIO_TERMTYPE_USB_STREAM,USBAUDIO_FU_MIC),
	USB_ACS_ACI_IT(USBAUDIO_IT_SPK,0,AUDIO_TERMTYPE_USB_STREAM,2,AUDIO_CHANNEL_CONFIG(1,1,0,0,0,0,0,0,0,0,0,0)),
    USB_ACS_ACI_FU_Stereo(USBAUDIO_FU_SPK,USBAUDIO_IT_SPK),
	USB_ACS_ACI_OT(USBAUDIO_OT_SPK,0,AUDIO_TERMTYPE_OT_SPK,USBAUDIO_FU_SPK),

	//MIC desc
	USB_AUDIO_IntfAltAS_BW0(USB_INTFID_AudioAS_MIC,0),
	USB_AUDIO_InterfaceAS(USB_INTFID_AudioAS_MIC,1),
	USB_ACS_ASI_General(USBAUDIO_OT_MIC,1,0x0001),
	USB_ACS_ASI_FmtTypeI(AUDIO_MIC_CHNs,AUDIO_MIC_BR,1),Byte3Data(AUDIO_MIC_SAMPRATE),
	USB_AUDIO_EndpAS_IN(USB_ENDPID_Audio_MIC,0/*AUDIO_SYNCMODE_SYNC*/,AUDIO_MAXP(AUDIO_MIC_CHNs,AUDIO_MIC_BR,AUDIO_MIC_SAMPRATE)),
	USB_ACS_EP_General(0),
	//SPK desc
	USB_AUDIO_IntfAltAS_BW0(USB_INTFID_AudioAS_SPK,0),
	USB_AUDIO_InterfaceAS(USB_INTFID_AudioAS_SPK,1),
	USB_ACS_ASI_General(USBAUDIO_IT_SPK,1,0x0001),
	USB_ACS_ASI_FmtTypeI(AUDIO_SPK_CHNs,AUDIO_SPK_BR,1),Byte3Data(AUDIO_SPK_SAMPRATE),
	USB_AUDIO_EndpAS_OUT(USB_ENDPID_Audio_SPK,0/*AUDIO_SYNCMODE_SYNC*/,AUDIO_MAXP(AUDIO_SPK_CHNs,AUDIO_SPK_BR,AUDIO_SPK_SAMPRATE)),
	USB_ACS_EP_General(0),

	USB_INTERFACE_DESC(USB_INTFID_Hid_Mse, 0, 1, USB_DEVICE_CLASS_HID, 0,0),
	USB_HID_CLS_DESC(tHidMSEReportDesc_aud_hid),
	USB_HID_EndpIN(USB_ENDPID_Hid_MSE, 8, 1),

	USB_INTERFACE_DESC(USB_INTFID_Hid_Dbg, 0, 2, USB_DEVICE_CLASS_HID, 0,0),
	USB_HID_CLS_DESC(tHidDbgReportDesc_aud_hid),
	USB_HID_EndpIN(USB_ENDPID_Hid_DBG_IN, 32, 1),	//yuan 64->32
	USB_HID_EndpOUT(USB_ENDPID_Hid_DBG_OUT, 32, 1)	//yuan 64->32

};
#endif

static
const CBufferBaseDesc _c_device_desc_aud_hid={(void*)&tUsbDeviceDescriptor_aud_hid,sizeof(tUsbDeviceDescriptor_aud_hid)};
static
const CBufferBaseDesc _c_config_desc_aud_hid={(void*)tUsbConfig_aud_hid,sizeof(tUsbConfig_aud_hid)};
static
const _t_index_descriptor tblUsbHidRptDescs_aud_hid[]={
	{USB_INTFID_Hid_Mse,{(void*)tHidMSEReportDesc_aud_hid,sizeof(tHidMSEReportDesc_aud_hid)}},
	{USB_INTFID_Hid_Dbg,{(void*)tHidDbgReportDesc_aud_hid,sizeof(tHidDbgReportDesc_aud_hid)}},
};
static
const _t_index_descriptor tblUsbStringDescs_aud_hid[]={
	{_Language_String_ID_,{(void*)tUsbLanguageID_aud_hid,sizeof(tUsbLanguageID_aud_hid)}},
	{_Vendor_String_ID_,{(void*)tUsbVendorStr_aud_hid,sizeof(tUsbVendorStr_aud_hid)}},
	{_Product_String_ID_,{(void*)tUsbProductStr_aud_hid,sizeof(tUsbProductStr_aud_hid)}},
	{_Sn_String_ID_,{(void*)tUsbSnStr_aud_hid,sizeof(tUsbSnStr_aud_hid)}},
};

void*aud_hid_GetDeviceDesc(void){
	return (void*)&_c_device_desc_aud_hid;
}

void*aud_hid_GetConfigDesc(void){
	return (void*)&_c_config_desc_aud_hid;
}
extern void*UsbDesc_Find(int idx,void*tb,int cnt);

void*aud_hid_GetHidRptDesc(int idx){
	return UsbDesc_Find(idx,(void*)tblUsbHidRptDescs_aud_hid,GET_ELEMENT_TBL(tblUsbHidRptDescs_aud_hid));
}

void*aud_hid_GetStringDesc(int idx){
	return UsbDesc_Find(idx,(void*)tblUsbStringDescs_aud_hid,GET_ELEMENT_TBL(tblUsbStringDescs_aud_hid));
}

