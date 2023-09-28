/*************************************************************
 * @file		drv_usbhost.c
 * @brief		core driver of usb host
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2020-12-03
 * @par
 * @attention
 *
 * @history		2022-09-20 jkg	create this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver_usb.h"
#include "usb\usbdef.h"
#include "drv_usb.h"

extern unsigned long long os_get_tick_counter(void);
extern void HwUsb_Switch2Endp(void*bp,int endpn);
extern void HwUsb_Host_BusReset(void*bp,int en_dis);
extern void HwUsbHost_EndpWrite(void*bp,int epn,void*setupPkt,void*buf,int len);
extern void AplUsbHost_StartRx(void*bp,int epn);
int usbhost_do_ctrl_transfer_v2(void*bp,void*setup,void*dat,int sz,int rxIf,int txIf,int usbIf);
void HwUsb_Host_BusReset(void*bp,int en_dis);

void *AplUsbHost_GetRxCbk(int endp_no);
void *AplUsbHost_GetTxCbk(int endp_no);
void *AplUsbHost_GetRxBufDesc(int endp_no);
//void *AplUsbHost_GetTxBufDesc(int endp_no);
void HwUsb_Host_BusReset(void*bp,int en_dis);
void*AplUsbHost_GetBP();
int AplUsbHost_GetEndpCount();
void AplUsbHost_SetConfigured(int cfgId);
void AplUsbHost_StopRx(void*bp,int epn);
void HwUsbHost_SendStatusOut(void*bp);
void __analog_config_desc_v2(void*cfg,int sz,void*proc);
int _config_endp_v2(void*bp,void*desc,int sz,int myDesc,void*buildTxPip,void*buildRxPip);

uint32_t HwUsb_GetCSR(void*bp);
void HwUsb_SetAddress(void*bp,int addr);

//extern CUsbSetupPkt setupPkt;
//int usbhost_do_ctrl_transfer(void*bp,void*setup,void*dat,int sz,void*evt);
void*AplUsbHost_GetDrv();

///////////////////////////////////////////////////////////////////////////////////////////
//
//usbhost架构+使用说明：usbhost驱动(core driver)+应用层驱动(app-driver)
//	1、核心层：usbhost驱动实现device的枚举过程+配置，运行在系统后台
//	2、用户层：实现各种具体应用层驱动(比如U盘初始化、scsi命令传输驱动、U盘协议实现、文件系统接口等)，运行在主循环
//TO DO:
//	1、需要有一个单独的app init初始化函数
//	2、调用usb_host_open():完成usb通用协议的挂载
//初始化函数需要做的是：将host app的init挂接到host core driver
//host App init要做：
//	1、挂接各endpoint的发送、接收回调函数
//	2、挂接sof回调函数
//依赖项：
//	1、core driver需要有一个10ms的时钟源
//	2、驱动需要_os_get_tick_counter()
//
///////////////////////////////////////////////////////////////////////////////////////////

enum{
	E_USBHOST_IDLE=0,//idle
	E_USBHOST_CONN,//connected
	E_USBHOST_BUSRST,//bus reset
	E_USBHOST_Get_Short_DevDesc,//get short device descriptor
	E_USBHOST_SetAddr,//set address
	E_USBHOST_WaitAddressOK,
	E_USBHOST_Get_DevDesc,//get full device descriptor
	E_USBHOST_Get_DevDesc_Wait,//get full device descriptor
	
	E_USBHOST_Get_Short_CfgDesc,//get short config descriptor
	E_USBHOST_Get_Short_CfgDesc_Wait,//get short config descriptor wait
	E_USBHOST_Get_CfgDesc,//get full config descriptor
	E_USBHOST_Get_CfgDesc_Wait,//get full config descriptor
	E_USBHOST_Set_Cfg,//set config
	E_USBHOST_Set_Cfg_Wait,//set config
	E_USBHOST_App_Init,//host App init
	E_USBHOST_App_Loop,//host App loop	
	E_USBHOST_Disconnect,//device disconnected
	E_USBHOST_Suspend,//device suspend
	E_USBHOST_Resume,//device Resume
};
	
//#pragma pack(1)	
typedef int (*thread_fun)(void*para);

static _t_usb_host_drv *s_drv_host=NULL;
//#pragma pack()
static int s_host_sm=E_USBHOST_IDLE;
CCallback_P0 cbk_hostAppInit=NULL;
CCallback_P0 cbk_hostLostDevice=NULL;
CCallback_P0 cbk_hostSof=NULL;


/////////////////////////////////////////////////////////////////////////////////////////
//usb event pool operations(fifo)

void*AplUsbHost_GetAppInitCbk(){
	return cbk_hostAppInit;
}

void AplUsbHost_SetAppInitCbk(void*cbk){
	cbk_hostAppInit=(CCallback_P0)cbk;
}
void*AplUsbHost_GetLostDeviceCbk(){
	return cbk_hostLostDevice;
}

void AplUsbHost_SetLostDeviceCbk(void*cbk){
	cbk_hostLostDevice=(CCallback_P0)cbk;
}

void AplUsbHost_SetSOFCbk(void*cbk){
	cbk_hostSof=(CCallback_P0)cbk;
}

void *AplUsbHost_GetSOFCbk(){
	return cbk_hostSof;
}

#if 0
static _t_usb_event_pool s_usb_evnet_pool;
void init_event(void*p){
	memset(p,0,sizeof(_t_usb_event_pool));
}
int push_event(void*p,int e,void*para){
	_t_usb_event_pool*epool=(_t_usb_event_pool*)p;
	_t_usb_event*pevt=NULL;
	pevt=&epool->events[epool->tail];
	pevt->msg=e;
	pevt->para=para;
	epool->tail++;
	if(epool->tail>=MAX_EVENT_USBHOST)epool->tail=0;
	epool->cnt++;
	if(epool->tail==epool->top)return(-1);
//	epool->tail&=0x0f;
	return(1);
}
void*pop_event(void*p){
	_t_usb_event_pool*epool=(_t_usb_event_pool*)p;
	_t_usb_event*pevt=NULL;
	if(epool->cnt==0)return(NULL);
	epool->cnt--;
//	if(epool->top==epool->tail)return(pevt);//fifo empty
	pevt=&epool->events[epool->top];
	epool->top++;
	if(epool->top>=MAX_EVENT_USBHOST)epool->top=0;
	return(pevt);
}
void*get_event(void*p){
	_t_usb_event_pool*epool=(_t_usb_event_pool*)p;
	_t_usb_event*pevt=NULL;
	if(epool->top==epool->tail)return(pevt);//fifo empty
	pevt=&epool->events[epool->top];
	return(pevt);
}
void reset_event_pool(void*p){
//	_t_usb_event_pool*epool=(_t_usb_event_pool*)p;
	_t_usb_event*pevt=NULL;
	while(1){
		pevt=(_t_usb_event*)pop_event(p);
		if(pevt==NULL)break;
		if(pevt->para)free(pevt->para);
		pevt->para=NULL;
	}
}
/////
//////////////////////////////////////////////////////////////////////////////////////////
int usb_push_event(int e,void*para){
	return(push_event(&s_usb_evnet_pool, e, para));
}
void*usb_pop_event(void){
	return pop_event(&s_usb_evnet_pool);
}
void*usb_get_event(void){
	return get_event(&s_usb_evnet_pool);
}
int usb_get_event_cnt(void){
	return s_usb_evnet_pool.cnt;
}

int usb_check_msg(void*e){
	_t_usb_event*pe=(_t_usb_event*)e;
	if(pe==NULL)return(E_USB_EVENT_None);
	return(pe->msg);
}
#endif

_t_usbhost_transfer hostEp0={.endp.stage=UNS_Undef};

void usbhost_start_setup(void*bp, void*endp, void*setup, void*dat, int len, int maxP){
	_t_usbhost_transfer *p_tr=(_t_usbhost_transfer *)endp;
	p_tr->setupPkt=(CUsbSetupPkt*)setup;
	memset(&p_tr->endp,0,sizeof(p_tr->endp));
	if(p_tr->setupPkt->dat[0]&BIT(7)){//根据setup命令确定是发送还是接受
		p_tr->endp.rxPtr=dat;
		p_tr->endp.rxLen=len;
		if(len>p_tr->setupPkt->len)p_tr->endp.rxLen=p_tr->setupPkt->len;
		p_tr->endp.lmtLen=maxP;
	}else{
		p_tr->endp.txPtr=dat;
		p_tr->endp.txLen=len;
		p_tr->endp.lmtLen=maxP;
	}
	HwUsbHost_EndpWrite(bp,0, setup, NULL,0);//发送setup token	
}

void*usbhost_get_ep0trans(){
	return &hostEp0;
}
void usb_host_SM_v2(void*bp,int rxIF,int txIF,int usbIF);

void usb_host_proc_v2(void*bp,int rxIF,int txIF,int usbIF){
	usb_host_SM_v2(bp,rxIF,txIF,usbIF);
}

void usb_host_proc(void*bp,int rxIF,int txIF,int usbIF){
	usb_host_proc_v2(bp, rxIF, txIF, usbIF);
}

void _usbhost_init(int maxIntf,int maxEndp){
	usb_hal_drv_init(AplUsbHost_GetDrv(), maxIntf, maxEndp);
}

void usbhost_build_pipRxBuf(int endp_no,int maxp){
	_t_usb_host_drv *phost=(_t_usb_host_drv *)AplUsbHost_GetDrv();
	if(endp_no>=phost->bUsbCntOfEndp)return;
	USB_LOG_D("brb endp=%d\r\nFree",endp_no);
	phost->pipIntf[endp_no].rxBuf.sz=maxp;
	USBH_DBG_PRT(phost->pipIntf[endp_no].rxBuf.ptr);
	if(phost->pipIntf[endp_no].rxBuf.ptr){
		free(phost->pipIntf[endp_no].rxBuf.ptr);
		phost->pipIntf[endp_no].rxBuf.ptr=NULL;
	}
	phost->pipIntf[endp_no].rxBuf.ptr=malloc(maxp);
	USB_LOG_D("brb end rxbuf=0x%.8x\r\n",phost->pipIntf[endp_no].rxBuf.ptr);
}

void usbhost_build_pipTxBuf(int endp_no,int maxp){
	_t_usb_host_drv *phost=(_t_usb_host_drv *)AplUsbHost_GetDrv();
	if(endp_no>=phost->bUsbCntOfEndp)return;
	USB_LOG_D("btb endp=%d\r\nFree",endp_no);
	USBH_DBG_PRT(phost->pipIntf[endp_no].txBuf.ptr);
	
	if(phost->pipIntf[endp_no].txBuf.ptr){
		free(phost->pipIntf[endp_no].txBuf.ptr);
		phost->pipIntf[endp_no].txBuf.ptr=NULL;
	}
	phost->pipIntf[endp_no].txBuf.sz=maxp;
	phost->pipIntf[endp_no].txBuf.ptr=malloc(maxp);
	USB_LOG_D("btb end txbuf=0x%.8x\r\n",phost->pipIntf[endp_no].txBuf.ptr);
}

void _host_analog_config_desc(void*bp,void*cfg,int sz){
	__analog_config_desc_v2(cfg, sz,_usbhost_init);
	_config_endp_v2(bp,cfg, sz, 0,usbhost_build_pipTxBuf,usbhost_build_pipRxBuf);
}

const char _cmd_get_short_device_desc[]={0x80,USB_GET_DESCRIPTOR,0x00,USB_DESCTYPE_DEVICE,0x00,0x00,0x08,0x00};
const char _cmd_set_address[]={0x00,USB_SET_ADDRESS,0x01,0x00,0x00,0x00,0x00,0x00};
const char _cmd_get_short_cfg_desc[]={0x80,USB_GET_DESCRIPTOR,0x00,USB_DESCTYPE_CONFIG,0x00,0x00,0x09,0x00};
const char _cmd_set_cfg_[]={0x00,USB_SET_CONFIGURATION,0x00,0x00,0x00,0x00,0x00,0x00};


//static int maxPktEp0=0;

//extern CUsbSetupPkt setupPkt;
//extern CEndpoint ep0;
//extern uint8_t ep0PktMaxSz;

//static uint8_t devMaxEp0Pkt;
static CUsbDevDesc s_devDesc;
static char*s_pcfgDesc=NULL;
static char s_tmpbuf[8];
#define PRINT_SM(sm)		
//USB_LOG_I(#sm"\r\n")

//在app中启动，使用结束回调和消息通知来
static volatile uint8_t s_usb_ctrl_transfer_flag=0;
static CBK_USBAPP s_ctrl_suc_cbk=NULL;
static CBK_USBAPP s_ctrl_fail_cbk=NULL;
void*usbhost_get_devDesc(){
	return &s_devDesc;
}
void*usbhost_get_cfgDesc(){
	return s_pcfgDesc;
}
static uint32_t t0=0;
void usbhost_start_ctrl_transfer(void*bp, void*setup, void*dat, int sz, void*sucCbk, void*failCbk){
	_t_usbhost_transfer*hep0=(_t_usbhost_transfer*)usbhost_get_ep0trans();
//	uint8_t s;
	usbhost_start_setup(bp,hep0, setup, dat, sz, ((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz);
	if(hep0->setupPkt->cmd&BIT(7)){
		if(dat)hep0->endp.stage=UNS_DataIn;
		else hep0->endp.stage=UNS_StatusIn;
	}else{
		if(dat)hep0->endp.stage=UNS_DataOut;
		else hep0->endp.stage=UNS_StatusIn;
	}
	t0=os_get_tick_counter();
	s_ctrl_suc_cbk=(CBK_USBAPP)sucCbk;
	s_ctrl_fail_cbk=(CBK_USBAPP)failCbk;
	s_usb_ctrl_transfer_flag=1;
}

void usbhost_ctrl_transfer_proc_v2(void*bp,int rxIf,int txIf,int usbIf){
	if(s_usb_ctrl_transfer_flag){
//		_t_usbhost_transfer*pep0=(_t_usbhost_transfer*)usbhost_get_ep0trans();
		int r=usbhost_do_ctrl_transfer_v2(bp,NULL,NULL,0,rxIf,txIf,usbIf);
		if(r<0){
			s_usb_ctrl_transfer_flag=0;
		}
		if(r>0){
			s_usb_ctrl_transfer_flag=0;
		}
	}
}
#if 0
void usbhost_ctrl_transfer_proc(void*bp,void *evt){
	if(s_usb_ctrl_transfer_flag){
		_t_usbhost_transfer*pep0=(_t_usbhost_transfer*)usbhost_get_ep0trans();
		int r=usbhost_do_ctrl_transfer(bp,NULL,NULL,0,evt);
		if(r<0){
			//发送一个usb ctrl transfer失败消息
			//调用传输失败回调函数
//			if(s_ctrl_fail_cbk){
//				if(pep0->setupPkt->cmd&BIT(7))s_ctrl_fail_cbk(pep0->endp.rxPtr,pep0->endp.rxLen);
//				else s_ctrl_fail_cbk(pep0->endp.txPtr,pep0->endp.txLen);
//				s_ctrl_fail_cbk=NULL;
//				s_ctrl_suc_cbk=NULL;
//			}
			s_usb_ctrl_transfer_flag=0;
		}
		if(r>0){
			//发送一个usb ctrl transfer成功消息
			//调用传输成功回调函数
//			if(s_ctrl_suc_cbk){
//				if(pep0->setupPkt->cmd&BIT(7))s_ctrl_suc_cbk(pep0->endp.rxPtr,pep0->endp.rxLen);
//				else s_ctrl_suc_cbk(pep0->endp.txPtr,pep0->endp.txLen);
//				s_ctrl_suc_cbk=NULL;
//				s_ctrl_fail_cbk=NULL;
//			}
			s_usb_ctrl_transfer_flag=0;
		}
	}
}
#endif
int usbhost_is_ctrl_transfer_busy(){
	return (int)s_usb_ctrl_transfer_flag;
}
int usbhost_do_ctrl_transfer_v2(void*bp, void*setup, void*dat,int sz,int rxIf,int txIf,int usbIf){
	int r=0;
	uint8_t ep0buf[0x40];
	_t_usbhost_transfer*hep0=(_t_usbhost_transfer*)usbhost_get_ep0trans();
	if(hep0->endp.stage>UNS_Undef){
		if(os_get_tick_counter()-t0>100){
			hep0->endp.stage=UNS_Undef;
			return(-1);
		}
	}
	switch(hep0->endp.stage){
		case UNS_Undef:
		{
			if(usbIf&BIT(3)){
				usbhost_start_setup(bp,hep0, setup, dat, sz, ((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz);
				if(hep0->setupPkt->cmd&BIT(7)){
					if(dat)hep0->endp.stage=UNS_DataIn;
					else hep0->endp.stage=UNS_StatusIn;
				}else{
					if(dat)hep0->endp.stage=UNS_DataOut;
					else hep0->endp.stage=UNS_StatusIn;
				}
				t0=os_get_tick_counter();
				//USB_LOG_I("st=%d",hep0->endp.stage);
			}
		}
		break;
		case UNS_DataOut:
		{
			//if(os_get_tick_counter()-t0>100){
			//	r=-1;
			//	break;
			//}
			//if((ev==E_USB_EVENT_DEV_EPn_TxCompletion)){
			if(txIf&BIT(0)){
				int len=hep0->endp.lmtLen;
				if(hep0->endp.txCursor+len < hep0->endp.txLen){
					HwUsbHost_EndpWrite(bp,0,NULL,&hep0->endp.txPtr[hep0->endp.txCursor], len);
				}else{
					len=hep0->endp.txLen-hep0->endp.txCursor;
					HwUsbHost_EndpWrite(bp,0,NULL,&hep0->endp.txPtr[hep0->endp.txCursor], len);
					hep0->endp.stage=UNS_StatusIn;
				}
			}			
		}
		break;
		case UNS_DataIn:
		{
			//if(os_get_tick_counter()-t0>100){
			//	r=-1;
			//	break;
			//}
			if(/*(txIf&BIT(0))||*/(usbIf&BIT(3))){
				AplUsbHost_StartRx(bp,0);
			}
			//if(ev==E_USB_EVENT_DEV_EPn_RxCompletion){
			if(rxIf&BIT(0)){
				int len=HwUsb_EndpRead(bp,0, ep0buf);
				//USB_LOG_I("rxl=%d\r\n", len);
				if((hep0->endp.rxCursor+len) <hep0->endp.rxLen){
					memcpy(&hep0->endp.rxPtr[hep0->endp.rxCursor],ep0buf,len);
					AplUsbHost_StartRx(bp,0);
				}else{
					len=hep0->endp.rxLen-hep0->endp.rxCursor;
					memcpy(&hep0->endp.rxPtr[hep0->endp.rxCursor],ep0buf,len);
					hep0->endp.stage=UNS_StatusOut;
					HwUsbHost_SendStatusOut(bp);
				}
				hep0->endp.rxCursor+=len;
			}
		}
		break;
		case UNS_StatusIn:
		{
			//if(os_get_tick_counter()-t0>100){
			//	r=-1;
			//	break;
			//}
			if(usbIf&BIT(3)){
				AplUsbHost_StartRx(bp,0);
			}
			if(rxIf&BIT(0)){
				r=1;
				hep0->endp.stage=UNS_Undef;
			}
		}
		break;
		case UNS_StatusOut:
		{
			//if(os_get_tick_counter()-t0>100){
			//	r=-1;
			//	break;
			//}
			PRINT_SM(UNS_StatusOut);
			if(txIf&BIT(0)){
				hep0->endp.stage=UNS_Undef;
				r=1;
			}
		}
		break;
		default:
			break;
	}
	return r;

}
#if 0
int usbhost_do_ctrl_transfer(void*bp,void*setup,void*dat,int sz,void*evt){
	int r=0;
	_t_usbhost_transfer*hep0=(_t_usbhost_transfer*)usbhost_get_ep0trans();
	_t_usb_event*pevt=(_t_usb_event*)evt;
	int ev=usb_check_msg(pevt);
	//if(ev==E_USB_EVENT_None)return(0);
	switch(hep0->endp.stage){
		case UNS_Undef:
		{
			if(ev==E_USB_EVENT_SOF){
				usbhost_start_setup(bp,hep0, setup, dat, sz, ((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz/*devMaxEp0Pkt*/);
				if(hep0->setupPkt->cmd&BIT(7)){
					if(dat)hep0->endp.stage=UNS_DataIn;
					else hep0->endp.stage=UNS_StatusIn;
				}else{
					if(dat)hep0->endp.stage=UNS_DataOut;
					else hep0->endp.stage=UNS_StatusIn;
				}
				t0=os_get_tick_counter();
				PRINT_SM(UNS_Undef);
			}
		}
		break;
		case UNS_DataOut:
		{
			if(os_get_tick_counter()-t0>100){
				r=-1;
				break;
			}
			if((ev==E_USB_EVENT_DEV_EPn_TxCompletion)){
				PRINT_SM(UNS_DataOut-E_USB_EVENT_DEV_EPn_TxCompletion);
				int len=hep0->endp.lmtLen;
				if(hep0->endp.txCursor+len < hep0->endp.txLen){
					//HwUsb_EndpWrite(0, &hep0->endp.txPtr[hep0->endp.txCursor], len);
					HwUsbHost_EndpWrite(bp,0,NULL,&hep0->endp.txPtr[hep0->endp.txCursor], len);
				}else{
					len=hep0->endp.txLen-hep0->endp.txCursor;
					//HwUsb_EndpWrite(0, &hep0->endp.txPtr[hep0->endp.txCursor], len);
					HwUsbHost_EndpWrite(bp,0,NULL,&hep0->endp.txPtr[hep0->endp.txCursor], len);
					hep0->endp.stage=UNS_StatusIn;
				}
			}
		
		}
		break;
		case UNS_DataIn:
		{
			if(os_get_tick_counter()-t0>100){
				r=-1;
				break;
			}
			if((ev==E_USB_EVENT_SOF)){
				PRINT_SM(UNS_DataIn-E_USB_EVENT_SOF);
				AplUsbHost_StartRx(bp,0);
			}
//			if((ev==E_USB_EVENT_DEV_EPn_TxCompletion)){
//				PRINT_SM(UNS_DataIn-E_USB_EVENT_DEV_EPn_TxCompletion);
//				AplUsbHost_StartRx(0);
//			}
			if(ev==E_USB_EVENT_DEV_EPn_RxCompletion){
				CBufferBaseDesc*pdb=(CBufferBaseDesc*)pevt->para;
				PRINT_SM(UNS_DataIn-E_USB_EVENT_DEV_EPn_RxCompletion);
				int len=pdb->sz;
				USB_LOG_I("rxl=%d\r\n", len);
				if((hep0->endp.rxCursor+len) <hep0->endp.rxLen){
					memcpy(&hep0->endp.rxPtr[hep0->endp.rxCursor],pdb->ptr,len);
					AplUsbHost_StartRx(bp,0);
				}else{
					len=hep0->endp.rxLen-hep0->endp.rxCursor;
					memcpy(&hep0->endp.rxPtr[hep0->endp.rxCursor],pdb->ptr,pdb->sz);
					hep0->endp.stage=UNS_StatusOut;
					HwUsbHost_SendStatusOut(bp);
				}
				hep0->endp.rxCursor+=len;
				
			}
		}
		break;
		case UNS_StatusIn:
		{
			if(os_get_tick_counter()-t0>100){
				r=-1;
				break;
			}
			if((ev==E_USB_EVENT_SOF)){
				PRINT_SM(UNS_StatusIn-E_USB_EVENT_SOF);
				AplUsbHost_StartRx(bp,0);
			}

//			if((ev==E_USB_EVENT_DEV_EPn_TxCompletion)){
//				PRINT_SM(UNS_StatusIn-E_USB_EVENT_DEV_EPn_TxCompletion);
//				AplUsbHost_StartRx(0);
//			}
			if(ev==E_USB_EVENT_DEV_EPn_RxCompletion){
				PRINT_SM(UNS_StatusIn-E_USB_EVENT_DEV_EPn_RxCompletion);
				r=1;
				hep0->endp.stage=UNS_Undef;
			}
		}
		break;
		case UNS_StatusOut:
		{
			if(os_get_tick_counter()-t0>100){
				r=-1;
				break;
			}
			PRINT_SM(UNS_StatusOut);
			//HwUsbHost_SendStatusOut();
			//HwUsb_SendZlp(0);
			if(ev==E_USB_EVENT_DEV_EPn_TxCompletion){
				hep0->endp.stage=UNS_Undef;
				r=1;
			}
		}
		break;
		default:
			break;
	}
	return r;
}
#endif
#define HOST_SM(sm)		s_host_sm=(sm)
static uint8_t s_usb_busreset_flag=0;
static int s_usb_busreset_tmr=0;
void usbhost_start_busreset(){
	s_usb_busreset_tmr=0;	
	s_usb_busreset_flag=1;
}
void usbhost_busreset_proc(/*void*bp*/){//在10ms定时器里运行
	if(s_usb_busreset_flag){
		s_usb_busreset_tmr++;
		if(s_usb_busreset_tmr>100){			
			HOST_SM(E_USBHOST_BUSRST);
			HwUsb_Host_BusReset(AplUsbHost_GetBP(),0);
			s_usb_busreset_flag=0;
		}
	}
}

void AplUsbHost_BuildDrv(){
	s_drv_host=(_t_usb_host_drv*)malloc(sizeof(_t_usb_host_drv));
	memset(s_drv_host,0,sizeof(_t_usb_host_drv));
}

void*AplUsbHost_GetDrv(){
	return(s_drv_host);
}


/*
* 函数名:
*	usb_host_SM
* 功能:
*	host状态机
* 参数:
*	1.无
* 输出:
*	无
* 返回:
*	无
*/
void usb_host_SM_v2(void*bp,int rxIF,int txIF,int usbIF){
//static unsigned int t0=0;
static int rpt=0;
static CUsbSetupPkt setupPkt;
	int r;
	CBufferBaseDesc*pdb=NULL;
	if(usbIF&BIT(5)){
		if(s_pcfgDesc)free(s_pcfgDesc);
		s_pcfgDesc=NULL;
		s_host_sm=E_USBHOST_IDLE;
		//需要添加断开处理+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		if(cbk_hostLostDevice)cbk_hostLostDevice();
		memset(&hostEp0,0,sizeof(hostEp0));
		AplUsbHost_SetConfigured(0);
		rpt=0;
		USB_LOG_I("device disconnected\r\n");
		return ;
	}
	switch(s_host_sm){
	case E_USBHOST_IDLE:
		if(usbIF&BIT(4)){
		//if(ev==E_USB_EVENT_DEV_CONNECTED){
			USB_LOG_I("device connected\r\n");
			//如果收到connected，发出busreset信号，启动定时1s
			HwUsb_Host_BusReset(bp,1);
			usbhost_start_busreset(bp);
			//t0=os_get_tick_counter();
			HOST_SM(E_USBHOST_CONN);
			//HOST_SM(E_USBHOST_BUSRST);
			((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz/*devMaxEp0Pkt*/=0x40;
			
		}
		break;
	case E_USBHOST_CONN:
		break;
	case E_USBHOST_BUSRST:
		//发送Get_DEVICEDESC命令
		r=usbhost_do_ctrl_transfer_v2(bp, (uint8_t*)_cmd_get_short_device_desc, &s_devDesc, 8,rxIF,txIF,usbIF);
		if(r<0){
			//USB_LOG_I("setup TO\r\n");
			rpt++;
			if(rpt>3){
				HOST_SM(E_USBHOST_IDLE);
			}else{
				memset(&hostEp0,0,sizeof(hostEp0));
				HwUsb_Host_BusReset(bp,1);
				usbhost_start_busreset(bp);
				HOST_SM(E_USBHOST_CONN);				
			}
		}
		if(r>0){
			rpt=0;
			usb_dbg_buf_show("device desc",&s_devDesc,8);
			((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz=s_devDesc.maxPkt;
			HOST_SM(E_USBHOST_SetAddr);
		}
		break;
	case E_USBHOST_SetAddr:
		r=usbhost_do_ctrl_transfer_v2(bp, (uint8_t*)_cmd_set_address, NULL, 0,rxIF,txIF,usbIF);
		if(r<0){
			HOST_SM(E_USBHOST_IDLE);
		}
		if(r>0){
			((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz=s_devDesc.maxPkt;
			memcpy(&setupPkt,_cmd_get_short_device_desc,sizeof(setupPkt));
			setupPkt.len=sizeof(CUsbDevDesc);
			HOST_SM(E_USBHOST_Get_DevDesc);
			HwUsb_SetAddress(bp,1);
		}
		break;
	case E_USBHOST_Get_DevDesc:
		r=usbhost_do_ctrl_transfer_v2(bp,&setupPkt, &s_devDesc, sizeof(s_devDesc),rxIF,txIF,usbIF);
		if(r<0){
			HOST_SM(E_USBHOST_IDLE);
		}
		if(r>0){
			usb_dbg_buf_show("device desc1",&s_devDesc,sizeof(s_devDesc));
			if((s_devDesc.devType!=USB_DESCTYPE_DEVICE)||(s_devDesc.len!=sizeof(s_devDesc))){
				HOST_SM(E_USBHOST_Get_DevDesc);
				t0=os_get_tick_counter();
				break;
			}
			HOST_SM(E_USBHOST_Get_Short_CfgDesc);			
		}
		break;
	case E_USBHOST_Get_Short_CfgDesc:
		r=usbhost_do_ctrl_transfer_v2(bp, (uint8_t*)_cmd_get_short_cfg_desc, s_tmpbuf, 8,rxIF,txIF,usbIF);
		if(r<0){
			HOST_SM(E_USBHOST_IDLE);
		}
		if(r>0){
			usb_dbg_buf_show("cfg desc",s_tmpbuf,sizeof(s_tmpbuf));
			CUsbConfigDesc*pcfg=(CUsbConfigDesc*)s_tmpbuf;
			s_pcfgDesc=(char*)malloc(pcfg->wTotalLength);
			memset(s_pcfgDesc,0,pcfg->wTotalLength);
			memcpy(s_pcfgDesc,s_tmpbuf,sizeof(s_tmpbuf));
			HOST_SM(E_USBHOST_Get_CfgDesc);
			memcpy(&setupPkt,_cmd_get_short_cfg_desc,sizeof(setupPkt));
			setupPkt.len=pcfg->wTotalLength;
		}
		break;

	case E_USBHOST_Get_CfgDesc:
		r=usbhost_do_ctrl_transfer_v2(bp,&setupPkt, s_pcfgDesc, ((CUsbConfigDesc*)s_tmpbuf)->wTotalLength, rxIF,txIF,usbIF);
		if(r<0){
			HOST_SM(E_USBHOST_IDLE);
		}
		if(r>0){
			CUsbConfigDesc*pcfg=(CUsbConfigDesc*)s_pcfgDesc;
			usb_dbg_buf_show("full cfg desc",s_pcfgDesc,pcfg->wTotalLength);
			HOST_SM(E_USBHOST_Set_Cfg);
			t0=os_get_tick_counter();
			_host_analog_config_desc(bp,pcfg, pcfg->wTotalLength);
			memcpy(&setupPkt,_cmd_set_cfg_,sizeof(setupPkt));
			setupPkt.tar=s_devDesc.nConfig;
		}
		break;
	case E_USBHOST_Set_Cfg:
		r=usbhost_do_ctrl_transfer_v2(bp,&setupPkt, NULL, 0,rxIF,txIF,usbIF);
		if(r<0){
			HOST_SM(E_USBHOST_IDLE);
		}
		if(r>0){
			if(cbk_hostAppInit)cbk_hostAppInit();//定义应用
			//在app init处理中按需要添加枚举成功处理
			HOST_SM(E_USBHOST_App_Loop);			
			AplUsbHost_SetConfigured(s_devDesc.nConfig);
		}
		break;
	case E_USBHOST_App_Loop:
		{
			if(usbIF){
				//host start of frame
				void (*sofCbk)();
				sofCbk=(void (*)())AplUsbHost_GetSOFCbk();
				if(sofCbk)sofCbk();
			}
			usbhost_ctrl_transfer_proc_v2(bp,rxIF,txIF,usbIF);
			int i;
			int csr;
			unsigned int pos=BIT(1);
			for(i=1;i<AplUsbHost_GetEndpCount();i++){
				if(rxIF&pos){
					HwUsb_Switch2Endp(bp, i);
					csr=HwUsb_GetCSR(bp);
					CBK_USBAPP rxCbk=(CBK_USBAPP)AplUsbHost_GetRxCbk(i);
					pdb=(CBufferBaseDesc*)AplUsbHost_GetRxBufDesc(i);
					int len;
					if(csr&BIT(6)){
						if(rxCbk)rxCbk(NULL,-1);
						AplUsbHost_StopRx(bp, i);
					}else{
						len=HwUsb_EndpRead(bp, i, pdb->ptr);
						if(rxCbk)rxCbk(pdb->ptr,len);
					}
				}
				if(txIF&pos){
					CBK_USBAPP txCbk=(CBK_USBAPP)AplUsbHost_GetTxCbk(i);
					if(txCbk)txCbk(NULL,0);//发送结束回调
				}
				pos<<=1;
			}	
		}
		break;
		
	default:
		break;
	}
}

