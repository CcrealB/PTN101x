/*************************************************************
 * @file		usb.h
 * @brief		APIs for usb App
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2020-12-03
 * @par
 * @attention
 *
 * @history		2020-12-03 jkg	create this file
 */
#ifndef _USB_H_ //this file's unique symbol checking section
#define _USB_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef void (*USB_MOD_CTRL_CBK)(int en_dis);
/*
 * func prototype:
 void USB_MOD_CTRL_CBK_PROC(int en_dis){
	if(en_dis){
        //open usb power
        //open usb clock
        //open usb other control
	}else{
        //close usb other control
        //close usb clock
        //close usb power
	}
 }
*/

typedef void (*USB_MOD_IE_CBK)(int en_dis);
/*
 * func prototype:
 void USB_MOD_IE_CBK_PROC(int en_dis){
	if(en_dis){
        //usb interrupt enable
	}else{
        //usb interrupt disable
	}
 }
*/
typedef void (*USB_OTH_CBK)();

/*define current USB App feature */

//extern volatile unsigned char bUsbConfig; //usb configed value

/*
 *函数名:
 *	AplUsb_StartTx
 *功能:
 *	启动usb在指定endp发送数据
 *参数:
 *	1.epn:endp号
 *	2.buf,len:数据参数
 *返回:
 *	无
 *特殊:
 *
*/
void AplUsb_StartTx(void*bp,int epn,void*buf,int len);

/*
 *函数名:
 *	AplUsb_SetTxCbk
 *功能:
 *	指定endp发送数据cbk
 *参数:
 *	1.epn:endp号
 *	2.cbk:CALLBACK函数
 *返回:
 *	无
 *特殊:
 *
*/
void AplUsb_SetTxCbk(int endp_no,void*cbk);
void AplUsb_SetTxCompletedCbk(int endp_no,void*cbk);

/*
 *函数名:
 *	AplUsb_SetRxPreCbk
 *功能:
 *	指定endp接收数据前cbk
 *参数:
 *	1.epn:endp号
 *	2.cbk:CALLBACK函数
 *返回:
 *	无
 *特殊:
 *
*/
void AplUsb_SetRxPreCbk(int endp_no,void*cbk);

void AplUsb_SetInterfaceCbk(int id_intf,void*cbk);

/*
 *函数名:
 *	AplUsb_SetRxCbk
 *功能:
 *	指定endp收到数据cbk
 *参数:
 *	1.epn:endp号
 *	2.cbk:CALLBACK函数
 *返回:
 *	无
 *特殊:
 *
*/
void AplUsb_SetRxCbk(int endp_no,void*cbk);
void AplUsb_SetRxCompletedCbk(int endp_no,void*cbk);

/*
 *函数名:
 *	AplUsb_GetRxBufDesc
 *功能:
 *	获取endp接收数据缓冲区描述
 *参数:
 *	1.epn:endp号
 *返回:
 *	接收数据缓冲区描述
 *特殊:
 *
*/
void *AplUsb_GetRxBufDesc(int endp_no);

/*
 *函数名:
 *	AplUsb_GetRxBuf
 *功能:
 *	获取endp接收数据指针
 *参数:
 *	1.epn:endp号
 *返回:
 *	接收数据缓冲区指针
 *特殊:
 *
*/
void *AplUsb_GetRxBuf(int endp_no);

/*
 *函数名:
 *	AplUsb_GetTxBufDesc
 *功能:
 *	获取endp发送数据缓冲区描述
 *参数:
 *	1.epn:endp号
 *返回:
 *	发送数据缓冲区描述
 *特殊:
 *
*/
void *AplUsb_GetTxBufDesc(int endp_no);

/*
 *函数名:
 *	AplUsb_GetTxBuf
 *功能:
 *	获取endp发送数据指针
 *参数:
 *	1.epn:endp号
 *返回:
 *	发送数据缓冲区指针
 *特殊:
 *
*/
void *AplUsb_GetTxBuf(int endp_no);

/*
 *函数名:
 *	AplUsb_GetBufSz
 *功能:
 *	获取endp缓冲区大小
 *参数:
 *	1.epn:endp号
 *返回:
 *	数据缓冲区大小
 *特殊:
 *
*/
int AplUsb_GetBufSz(int endp_no);

/*
 *函数名:
 *	AplUsb_GetTxCbk
 *功能:
 *	获取指定endp发送数据cbk
 *参数:
 *	1.epn:endp号
 *返回:
 *	cbk函数指针
 *特殊:
 *
*/
void *AplUsb_GetTxCbk(int endp_no);
void *AplUsb_GetTxCompletedCbk(int endp_no);
void *AplUsb_GetRxPreCbk(int endp_no);

/*
 *函数名:
 *	AplUsb_GetRxCbk
 *功能:
 *	获取指定endp收到数据cbk
 *参数:
 *	1.epn:endp号
 *返回:
 *	cbk函数指针
 *特殊:
 *
*/
void *AplUsb_GetRxCbk(int endp_no);
void *AplUsb_GetRxCompletedCbk(int endp_no);


/*
 *函数名:
 *	AplUsb_GetSOF
 *功能:
 *	获取USB SOF计数器
 *参数:
 *	1.无
 *返回:
 *	SOF计数值
 *特殊:
 *
*/
int AplUsb_GetSOF(void*bp);

/*
 *函数名:
 *	AplUsb_GetSOFCbk
 *功能:
 *	获取sof回调函数
 *参数:
 *	1.无
 *返回:
 *	sof回调函数
 *特殊:
 *
*/
void*AplUsb_GetSOFCbk();

/*
 *函数名:
 *	AplUsb_SetSOFCbk
 *功能:
 *	设定sof回调函数
 *参数:
 *	1.cbk:sof回调函数
 *返回:
 *	无
 *特殊:
 *
*/
void AplUsb_SetSOFCbk(void*cbk);

/*
 *函数名:
 *	AplUsb_IsConfigured
 *功能:
 *	usb设备是否已配置
 *参数:
 *	1.无
 *返回:
 *	0=未配置,1=已配置
 *特殊:
 *
*/
int AplUsb_IsConfigured(void);

/*
 *函数名:
 *	usb_init
 *功能:
 *	usb device 初始化
 *参数:
 *	1.usb_mod_ctrl_cbk:usb modal控制回调，usb模块依赖于芯片其他部分的初始化：时钟、电源开关等
 *	2.usb_mod_int_ie_cbk:usb中断使能/禁能回调
 *返回:
 *	无
 *特殊:
 *
*/
void usb_init(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk);

/*
 *函数名:
 *	usb_deinit
 *功能:
 *	usb模块关闭
 *参数:
 *	1.usb_mod_ctrl_cbk:usb modal控制回调，usb模块依赖于芯片其他部分的初始化：时钟、电源开关等
 *	2.usb_mod_int_ie_cbk:usb中断使能/禁能回调
 *返回:
 *	无
 *特殊:
 *
*/
void usb_deinit(void*bp,void*usb_mod_ctrl_cbk,void*usb_mod_int_ie_cbk);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif
/*end file*/
