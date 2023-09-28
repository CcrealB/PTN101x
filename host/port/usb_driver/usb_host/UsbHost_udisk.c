/*************************************************************
 * @file		AplUsbHost.c
 * @brief		udisk app-driver of usb host
 * @author		Jiang Kaigan
 * @version		V1.0
 * @date		2022-09-20
 * @par
 * @attention
 *
 * @history		2022-09-20 jkg	create this file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bkreg.h"
#include "../driver_usb.h"
#include "../usb/usb.h"
#include "../usb/usbdef.h"

#include "../usb_dev/udisk/inc/mu_bot.h"
#include "../usb_dev/udisk/inc/mu_scsi.h"
#include "../usb_dev/udisk/inc/mu_msd.h"
#include "../drv_usb.h"
#include "../drv_usbhost.h"
#include "usbh_udisk.h"
// #include "app_player.h"
#include "app_work_mode.h"

//-------------------------------------------------- debug


//-------------------------------------------------- define
#define LE2BE_16(d)		(((d)&0xff)<<8)+(((d)>>8)&0xff)
#define LE2BE_24(d)		(((d)&0xff)<<16)+(((d)>>16)&0xff)+((d)&0xff00)
#define LE2BE_32(d)		(((d)&0xff)<<24)+(((d)>>24)&0xff)+(((d)&0xff00)<<8)+(((d)&0xff0000)>>8)

enum{
	E_APP_UH_IDLE=0,
	E_APP_UH_Delay,	
	E_APP_UH_SetIntface,
	E_APP_UH_WaitSetupOK,
	E_APP_UH_WaitPipeCmdEnd,
	E_APP_UH_GetLUN,
	E_APP_UH_GetBulkInStatus1,
	E_APP_UH_GetBulkInStatus1OK,
	E_APP_UH_GetBulkInStatus2,
	E_APP_UH_GetBulkInStatus2OK,
	E_APP_UH_GetBulkInStatus3,
	E_APP_UH_GetBulkInStatus3OK,
	E_APP_UH_ResetIntf,
	E_APP_UH_WaitResetOK,
	E_APP_UH_ClearBulkInHalt,
	E_APP_UH_ResetEndp=E_APP_UH_ClearBulkInHalt,
	E_APP_UH_ClearBulkInHaltOK,
	E_APP_UH_ClearBulkOutHalt,
	E_APP_UH_ClearBulkOutHaltOK,
	E_APP_UH_StartRx,
	E_APP_UH_StopRx,
	
	E_APP_UH_GetCap,
	E_APP_UH_InquiryFull,
	
	E_APP_UH_GetCap1,
	E_APP_UH_GetCap2,
	E_APP_UH_RequestSense=E_APP_UH_GetCap2,
	E_APP_UH_GetCap3,
	E_APP_UH_GetCap4,
	E_APP_UH_TestReady,
	E_APP_UH_StartUnit,
	E_APP_UH_InquiryInfo,
	E_APP_UH_ReadBlock,
	E_APP_UH_UDiskInit,
	E_APP_UH_PlayMedia,
	E_APP_UH_Stop,
	
	E_APP_UH_Unknown,

};

const CUsbSetupPkt cSetInterface={
	.cmd = 0x0b01,
    .tar = 0x0000,
    .idx = 0x0000,
    .len = 0x0000,
};

const CUsbSetupPkt cGetLUN={
	.cmd = 0xfea1,
    .tar = 0x0000,
    .idx = 0x0000,
    .len = 0x0001,
};
//const 
CUsbSetupPkt cResetIntf;
	
static int botSm=0;
static int botInited=0;
static uint32_t s_udisk_configed_time = 0;
static void*s_bot_blk_buf=NULL;
static uint8_t numLUN=0;
//extern UI8 ep0PktMaxSz;
extern int usbhost_is_ctrl_transfer_busy();
//extern void*usbhost_get_devDesc();
extern void*usbhost_get_cfgDesc();
extern void* _find_tarDesc(void*desc,int sz,int tarTyp,int index);
extern void AplUsb_StartTx(void*bp,int epn, void * buf, int len);
extern int AplUsbHost_GetEndpCount();
extern void *AplUsbHost_GetRxBuf(int endp_no);
extern void *AplUsbHost_GetTxBuf(int endp_no);
extern void *AplUsbHost_GetRxBufDesc(int endp_no);
extern void *AplUsbHost_GetTxBufDesc(int endp_no);
extern int AplUsbHost_IsConfigured();
extern void AplUsbHost_SetRxCbk(int endp_no,void*cbk);
extern void AplUsbHost_SetTxCbk(int endp_no,void*cbk);
extern void AplUsbHost_StartRx(void*bp,int epn);
extern void AplUsbHost_StopRx(void*bp,int epn);
extern void HwUsb_Switch2Endp(void*bp,int endpn);
extern void HwUsbHost_EndpWrite(void*bp,int epn,void*setupPkt,void*buf,int len);
extern int HwUsbHost_GetTxNakLimit(void*bp);
extern int HwUsbHost_GetRxNakLimit(void*bp);
extern uint64_t os_get_tick_counter(void);
extern inline unsigned int sys_time_get(void);

static uint8_t s_bot_bulkIn_endp=0;
static uint8_t s_bot_bulkOut_endp=0;
static int sofDly=0;
static unsigned char sofDlyEn=0;
static unsigned char sofDlyInit=0;
static int sof0=0;
static volatile unsigned char sofDlyEnd=0;

static uint32_t udisk_t_ref;
__attribute__((weak)) void udsik_time_slot_callback(void) {}//run critical code
void udsik_timer_init(void) { udisk_t_ref = sys_time_get(); }           
int udisk_timeout(int over_tim_ms)
{
    if((sys_time_get() - udisk_t_ref) >= over_tim_ms){
        return 1;
    } else {
        // vTaskDelay(1);
        udsik_time_slot_callback();
        return 0;
    }
}

#if 0
static void enable_sof_dly(int cnt){
	sofDly=cnt;
	sofDlyEnd=0;
	sofDlyEn=1;
}
static int is_sof_dly_end(){
	return sofDlyEnd;
}
#endif
int is_bot_protocol(){
	CUsbConfigDesc*pcfg=(CUsbConfigDesc*)usbhost_get_cfgDesc();
	CUsbIntfDesc*pintf=NULL;
	if(pcfg->bNumberOfIntf>1)return 0;
	pintf=(CUsbIntfDesc*)_find_tarDesc(pcfg, pcfg->wTotalLength, USB_DESCTYPE_INTERFACE,1);
	if(pintf==NULL)return 0;
	if((pintf->bIntfClass==USB_DEVICE_CLASS_MASS)&&
		(pintf->bIntfSubClass==0x06)&&//scsi command set
		(pintf->bIntfProtool==0x50))//bulk only transport
		return 1;
	return(0);
}

typedef struct {
	MGC_MsdCbw*lastCbw;//当前host命令
	MGC_MsdCsw*pcsw;//当前回应status包
	CBufferBaseDesc bd;//数据信息:
	int cursor;//数据传输的当前位置
	void (*cmpl)(void*ptr,int sz);//结束回调,ptr=NULL&sz<0:=错误返回sz，其他:=成功返回
	volatile int stage;//BOT传输阶段
}_t_msd_scsi_cmd_info;

//extern void AplUsb_SetRxCbk(int endp_no, void * cbk);
//extern void*bot_get_cbw();
//extern void*bot_get_csw();
//extern void*bot_get_block_buf();
typedef struct {
	unsigned int lastBlock;
	unsigned int blockSize;
}_t_msd_cap_info;

_t_msd_cap_info s_cap_info;
MGC_MsdCbw *s_host_cbw=NULL;
MGC_MsdCsw *s_host_csw=NULL;
static _t_msd_scsi_cmd_info s_bot_scsi_transfer;

const uint8_t cScsiWriteCmd[]={
	MGC_SCSI_WRITE10,
	MGC_SCSI_WRITE12,
};
void*bothost_get_cbw(){
	return(s_host_cbw);
}
void*bothost_get_csw(){
	return(s_host_csw);
}
void*bothost_get_block_buf(){
	return(s_bot_blk_buf);
}
int _is_in_scsi_tbl(void*tbl,int sz,int scsi_cmd){
	int i;
	uint8_t* ptr=(uint8_t*)tbl;
	for(i=0;i<sz;i++){
		if(ptr[i]==scsi_cmd)return 1;
	}
	return(0);
}
static unsigned int s_bot_scsi_t0;
void _build_scsi_cmd(void*cbw,int lun,int reqLen,void*cb,int cbLen){
	static unsigned int cnt=1;
	MGC_MsdCbw*pcbw=(MGC_MsdCbw*)cbw;
	uint8_t* p_cb=(uint8_t*)cb;
	memset(cbw,0,sizeof(MGC_MsdCbw));
	pcbw->dCbwSignature='U'|('S'<<8)|('B'<<16)|('C'<<24);
	cnt+=0x017385a9;
	pcbw->dCbwTag=cnt;
	pcbw->bmCbwFlags=BIT(7);
	if(_is_in_scsi_tbl((void*)cScsiWriteCmd, GET_ELEMENT_TBL(cScsiWriteCmd),p_cb[0])){
		pcbw->bmCbwFlags=0;
	}
	if(reqLen==0)pcbw->bmCbwFlags=0;
	pcbw->dCbwDataTransferLength=reqLen;
	pcbw->bCbwCbLength=cbLen;
	memset(pcbw->aCbwCb,0,sizeof(pcbw->aCbwCb));
	memcpy(pcbw->aCbwCb,cb,cbLen);
}

void bot_start_scsi_command(void*cmd,void*dat,int sz){
	//EnterFunc();
	_t_msd_scsi_cmd_info*ptrans=&s_bot_scsi_transfer;
	ptrans->lastCbw=(MGC_MsdCbw*)cmd;
	ptrans->bd.ptr=dat;
	ptrans->bd.sz=sz;
	ptrans->cursor=0;
	if(ptrans->lastCbw->bmCbwFlags&BIT(7)){//data in
		ptrans->stage=3;//data in
	}else{
		ptrans->stage=1;//data out
	}
	if(ptrans->lastCbw->dCbwDataTransferLength==0){//如果传输长度为0，则直接到status in
		ptrans->stage=4;//status in
	}
	AplUsb_StartTx(AplUsbHost_GetBP(),s_bot_bulkOut_endp, s_bot_scsi_transfer.lastCbw, sizeof(MGC_MsdCbw));
	//HwUsbHost_EndpWrite(AplUsbHost_GetBP(),s_bot_bulkOut_endp, NULL, ptrans->lastCbw, sizeof(MGC_MsdCbw));
	//AplUsb_StartTx(s_bot_bulkOut_endp, s_bot_scsi_transfer.lastCbw, sizeof(MGC_MsdCbw));
	s_bot_scsi_t0 = sys_time_get();
//	enable_sof_dly(500);
//	USB_LOG_I("S");
}
static int is_bot_csw(void*dat,int sz){
	MGC_MsdCsw*pcsw=(MGC_MsdCsw*)dat;
	if(memcmp(dat,"USBS",4)!=0)	return(0);
	if(sz!=sizeof(MGC_MsdCsw))	return(4);
	if(s_bot_scsi_transfer.lastCbw->dCbwTag!=pcsw->dCswTag)return(5);
	if(pcsw->bCswStatus==0)return(1);
	else return(2);
//	if(memcmp(&s_bot_scsi_transfer.lastCbw->dCbwTag,&pcsw->dCswTag,sizeof(s_bot_scsi_transfer.lastCbw->dCbwTag))!=0){
//		USB_LOG_I("sT=%.8X,rT=%.8X\r\n", s_bot_scsi_transfer.lastCbw->dCbwTag,pcsw->dCswTag);
//		return 0;
//	}
	return(3);
}

int is_bot_transfer_busy(){
//	if(os_get_tick_counter()-s_bot_scsi_t0>100){
//		s_bot_scsi_transfer.stage=0;
//		return 0;//300ms timeout
//	}
	return s_bot_scsi_transfer.stage;
}

int wait_bot_trans_cmp(const char *str_func, int tim_out_ms)
{
    udsik_timer_init();
    while(is_bot_transfer_busy() > 0)
    {
        if(udisk_timeout(tim_out_ms)){
            USB_LOG_I("bot tim out @ %s(), %d ms\n", str_func, tim_out_ms);
            return -1;
        }
    }
    return 0;
}

static void cbk_bot_sof(){
	if(sofDlyEn){
		if(sofDlyInit==0){
			sof0=AplUsb_GetSOF(AplUsbHost_GetBP());
			sofDlyInit=1;
		}else{
			int c;
			c=AplUsb_GetSOF(AplUsbHost_GetBP());
			if(c<sof0){
				c=0x800-sof0+c;
			}else{
				c-=sof0;
			}
			if(c>=sofDly){
				sofDlyEn=0;
				sofDlyInit=0;
				sofDlyEnd=1;
				USB_LOG_I("TO\r\n");
				//s_bot_scsi_transfer.stage=2;
				//AplUsbHost_StartRx(AplUsbHost_GetBP(),s_bot_bulkIn_endp);				
			}
		}
	}
}

static void cbk_bot_bulk_in(void*dat,int sz){
	//USB_LOG_I("%s\r\n",__FUNCTION__);
	uint8_t* ptr=NULL;
	int len;
	void*bp=AplUsbHost_GetBP();
	_t_msd_scsi_cmd_info*ptrans=&s_bot_scsi_transfer;
//	if((s_bot_scsi_transfer.lastCbw->aCbwCb[0]!=MGC_SCSI_READ10)&&
//		(s_bot_scsi_transfer.lastCbw->aCbwCb[0]!=MGC_SCSI_WRITE10)){
//			usb_dbg_buf_show("", dat, sz);
//			uart_wait_end();
//		}
	if(ptrans->stage<0){
		return;
	}	
	if((dat==NULL)&&(sz<0)){
		ptrans->stage=-1;
		if(ptrans->cmpl){
			ptrans->cmpl(NULL,ptrans->stage);
			ptrans->cmpl=NULL;
		}
		return;
	}
//	int r=is_bot_csw(dat, sz);
//	USB_LOG_I("R%d=%d",s_bot_scsi_transfer.stage,sz);
//	USB_LOG_I("e%d\r\n",r);
//	if(r>0){
//		s_bot_scsi_transfer.stage=0;
//		return;
//	}
	int ser;
	switch(ptrans->stage){
		case 1://data out
			//USB_LOG_I("BDO Rx\r\n");
			if(is_bot_csw(dat, sz)>0){
				//USB_LOG_I("e1\r\n");
				ptrans->stage=0;
				if(ptrans->cmpl){
					ptrans->cmpl(ptrans->bd.ptr,ptrans->bd.sz);
					ptrans->cmpl=NULL;
				}
			}
			break;
		case 2://data in
			{
			//USB_LOG_I("R\r\n");
			//USB_LOG_I("ts=%d c=%d isz=%d\r\n",s_bot_scsi_transfer.bd.sz,s_bot_scsi_transfer.cursor,sz);
			ser=is_bot_csw(dat, sz);
			if(ser>0){
				//USB_LOG_I("CSW");
				//USB_LOG_I("e2\r\n");
				if(ser==1)ptrans->stage=0;
				else ptrans->stage=-2;
				if(ptrans->cmpl){
					if(ser==1)ptrans->cmpl(ptrans->bd.ptr,ptrans->bd.sz);
					else ptrans->cmpl(NULL,ptrans->stage);
					ptrans->cmpl=NULL;
				}
				//if(ser==2)s_bot_scsi_transfer.stage=-2;
				//s_bot_scsi_transfer.stage=-2;
				return;
			}
			if(sz<(ptrans->bd.sz-ptrans->cursor)){
				//USB_LOG_I("1\r\n");
				//uart_wait_end();
				len=sz;
				ptr=(uint8_t*)ptrans->bd.ptr;
				memcpy(&ptr[ptrans->cursor],dat,len);
				AplUsbHost_StartRx(bp,s_bot_bulkIn_endp);
				ptrans->cursor+=len;
			}else{
				//USB_LOG_I("2\r\n");
				//uart_wait_end();
				len=ptrans->bd.sz-ptrans->cursor;
				ptr=(uint8_t*)ptrans->bd.ptr;
				memcpy(&ptr[ptrans->cursor],dat,len);
				ptrans->cursor+=len;
				if(ptrans->cursor>=ptrans->bd.sz){
					ptrans->stage=6;
					AplUsbHost_StartRx(bp,s_bot_bulkIn_endp);
				}
			}		
			}
			break;
		case 6://status in
			//USB_LOG_I("e");
			//s_bot_scsi_transfer.stage=0;
			ser=is_bot_csw(dat, sz);
			if(ser==1)ptrans->stage=0;
			else ptrans->stage=-3;
			if(ptrans->cmpl){				
				if(ser==1)ptrans->cmpl(ptrans->bd.ptr,ptrans->bd.sz);
				else ptrans->cmpl(NULL,ptrans->stage);
				ptrans->cmpl=NULL;
			}
			//USB_LOG_I("%d\r\n",s_bot_scsi_transfer.stage);
			break;
		default:
			break;
	}
}
static void cbk_bot_bulk_out(void*dat,int sz){
	//USB_LOG_I("%s\r\n",__FUNCTION__);
	CBufferBaseDesc*pdb=NULL;
	int len;
	uint8_t* ptr=NULL;
	void*bp=AplUsbHost_GetBP();
	_t_msd_scsi_cmd_info*ptrans=&s_bot_scsi_transfer;
//	USB_LOG_I("T");
	if(ptrans->stage<0){
		return;
	}	
	if((dat==NULL)&&(sz<0)){
		ptrans->stage=-1;
		if(ptrans->cmpl){
			ptrans->cmpl(NULL,-1);
			ptrans->cmpl=NULL;
		}
		return;
	}
	switch(ptrans->stage){
		case 1://data out
			//USB_LOG_I("BDO Tx\r\n");
			pdb=(CBufferBaseDesc*)AplUsbHost_GetTxBufDesc(s_bot_bulkOut_endp);
			// USB_LOG_I("endp buf=%.8x,sz=%.4x\r\n",pdb->ptr,pdb->sz);
			if(pdb->sz<(ptrans->bd.sz-ptrans->cursor)){
				len=pdb->sz;
				ptr=(uint8_t*)ptrans->bd.ptr;
				AplUsb_StartTx(bp, s_bot_bulkOut_endp, &ptr[ptrans->cursor], len);
				//AplUsb_StartTx(s_bot_bulkOut_endp, &ptr[s_bot_scsi_transfer.cursor], len);
				//HwUsbHost_EndpWrite(bp,s_bot_bulkOut_endp, &ptr[ptrans->cursor], len);
				ptrans->cursor+=len;
			}else{
				len=ptrans->bd.sz-ptrans->cursor;
				ptr=(uint8_t*)ptrans->bd.ptr;
				AplUsb_StartTx(bp, s_bot_bulkOut_endp, &ptr[ptrans->cursor], len);
				//AplUsb_StartTx(s_bot_bulkOut_endp, &ptr[s_bot_scsi_transfer.cursor], len);
				//HwUsbHost_EndpWrite(bp,s_bot_bulkOut_endp, &ptr[ptrans->cursor], len);
				ptrans->cursor+=len;
				if(ptrans->cursor>=ptrans->bd.sz){
					ptrans->stage=5;
				}
			}
			break;
		case 2://data in
			//USB_LOG_I("BDI Tx\r\n");
			//uart_wait_end();
			//AplUsbHost_StartRx(bp,s_bot_bulkIn_endp);			
			//s_bot_scsi_transfer.stage=3;
			break;
		case 3:
			ptrans->stage=2;
			AplUsbHost_StartRx(bp,s_bot_bulkIn_endp);			
//			sofDly=2;
//			sofDlyEn=1;
			break;
		case 4://status in
			//USB_LOG_I("BSI Tx\r\n");			
			ptrans->stage=6;
			AplUsbHost_StartRx(bp,s_bot_bulkIn_endp);			
			break;
		case 5:
			ptrans->stage=6;
			AplUsbHost_StartRx(bp,s_bot_bulkIn_endp);
			break;
	}
}
const uint8_t cScsiCmdSendDiagnostic[]={MGC_SCSI_SEND_DIAGNOSTIC,0x04,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdInquery[]={MGC_SCSI_INQUIRY,0x00,0x00,0x00,0x40,0x00};
const uint8_t cScsiCmdReadCap[]={MGC_SCSI_READ_CAPACITY,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,0x00};
const uint8_t cScsiCmdRead10[]={MGC_SCSI_READ10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdWrite10[]={MGC_SCSI_WRITE10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdRequestSense[]={MGC_SCSI_REQUEST_SENSE,0x00,0x00,0x00,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdRdFmtCapc[]={MGC_SCSI_RD_FMT_CAPC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdTestUnitRdy[]={MGC_SCSI_TEST_UNIT_READY,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdStopUnit[]={MGC_SCSI_START_STOP_UNIT,0x00,0x00,0x00,0x00/*b0:start,b1:eject*/,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdStartUnit[]={MGC_SCSI_START_STOP_UNIT,0x00,0x00,0x00,0x01/*b0:start,b1:eject*/,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const uint8_t cScsiCmdEjectUnit[]={MGC_SCSI_START_STOP_UNIT,0x00,0x00,0x00,0x02/*b0:start,b1:eject*/,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
#pragma pack(1)
typedef struct {
    unsigned char status;
    unsigned char first_sector[3];//[2]=cylinder,[1]=sector,[0]=header,起始柱面/扇区/磁头
    unsigned char partition_type;
    unsigned char last_sector[3];//[2]=cylinder,[1]=sector,[0]=header,结束柱面/扇区/磁头
    unsigned int lba_first_sector;
    unsigned int sector_count;
}_t_MBR_partition_table;
typedef struct {
    unsigned char boot_code[440];
    unsigned int disk_signature;
    unsigned short reserved;
    _t_MBR_partition_table partition_table[4];
    unsigned short signature;
} _t_MBR;
#pragma pack()

typedef struct{
	unsigned char err_code:7;	
	unsigned char valid:1;
	unsigned char :8;
	unsigned char sense_key:4;
	unsigned char :4;
	unsigned char info[4];
	unsigned char add_sense_len;
	unsigned char :8;
	unsigned char :8;
	unsigned char :8;
	unsigned char :8;
	unsigned char asc;
	unsigned char ascq;
	unsigned char :8;
	unsigned char :8;
	unsigned char :8;
	unsigned char :8;
}_t_std_sense_report;
typedef struct{
	struct{
		unsigned char :8;
		unsigned char :8;
		unsigned char :8;
		unsigned char cap_list_len:8;
		}cap_list_hdr;
	struct{
		unsigned char num_blocks[4];//block数量，BE		
		unsigned char desc_code:2;//00=可格式化的容量描述符,01=unformatted,最大容量描述符,10=formatted,当前容量描述符
									//11=没有存储介质
		unsigned char :6;
		unsigned char len_block[3];//block大小，BE
		}cur_max_cap_desc[1];
}_t_formatted_cap;
typedef union {
	unsigned char b[0x24];
	struct{
		unsigned char peri_dev_typ:5;//外设类型
		unsigned char :3;
		unsigned char :7;
		unsigned char rmb:1;//removable media bit,可移除介质
		unsigned char ansi_ver:3;
		unsigned char ecma_ver:3;
		unsigned char iso_ver:2;
		unsigned char resp_data_fmt:4;
		unsigned char :4;
		unsigned char add_length:8;
		unsigned char :8;
		unsigned char vendor_info[8];
		unsigned char prod_id[16];
		unsigned char prod_ver[4];
		};
}_t_inquiry_info;

void bot_begin_inquiry(int lun,void*info,int sz){
	_build_scsi_cmd(s_host_cbw, lun, sz, (void *)cScsiCmdInquery, 6);
	bot_start_scsi_command(s_host_cbw,info, sz);
}
void bot_begin_readCap(int lun,void*info,int sz){
	_build_scsi_cmd(s_host_cbw, lun, sz, (void *)cScsiCmdReadCap, 10);
	bot_start_scsi_command(s_host_cbw,info, sz);
}
void bot_begin_readFmtCap(int lun,void*info,int sz){
	_build_scsi_cmd(s_host_cbw, lun, sz, (void *)cScsiCmdRdFmtCapc, 10);
	bot_start_scsi_command(s_host_cbw,info, sz);
}
void bot_begin_requstSense(int lun,void*info,int sz){
	_build_scsi_cmd(s_host_cbw, lun, sz, (void *)cScsiCmdRequestSense, 6);
	bot_start_scsi_command(s_host_cbw,info, sz);
}

void bot_begin_testUnitRdy(int lun){
	_build_scsi_cmd(s_host_cbw, lun, 0, (void *)cScsiCmdTestUnitRdy, 12);
	bot_start_scsi_command(s_host_cbw,NULL, 0);
}
void bot_begin_startUnit(int lun){
	_build_scsi_cmd(s_host_cbw, lun, 0, (void *)cScsiCmdStartUnit, 12);
	bot_start_scsi_command(s_host_cbw,NULL, 0);
}
void bot_begin_ejectUnit(int lun){
	_build_scsi_cmd(s_host_cbw, lun, 0, (void *)cScsiCmdEjectUnit, 12);
	bot_start_scsi_command(s_host_cbw,NULL, 0);
}
void bot_begin_stopUnit(int lun){
	_build_scsi_cmd(s_host_cbw, lun, 0, (void *)cScsiCmdStopUnit, 12);
	bot_start_scsi_command(s_host_cbw,NULL, 0);
}

int bot_get_info_udisk(int lun,void*info,int sz){
	bot_begin_inquiry(lun, info, sz);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}

int bot_test_unit_ready(int lun){
	bot_begin_testUnitRdy(lun);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}
int bot_start_unit(int lun){
	bot_begin_startUnit(lun);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}
int bot_stop_unit(int lun){
	bot_begin_stopUnit(lun);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}
int bot_eject_unit(int lun){
	bot_begin_ejectUnit(lun);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}
int bot_get_udisk_cap(int lun,void*info,int sz){
	bot_begin_readCap(lun, info, sz);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}
int bot_requst_sense(int lun,void*info,int sz){
	bot_begin_requstSense(lun, info, sz);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}
int bot_read_fmtCapc(int lun,void*info,int sz){
	bot_begin_readFmtCap(lun, info, sz);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}

int bot_begin_readBlock(int lun,unsigned int blockAddr,void*rxbuf,int blkN){
	uint32_t len=blkN*0x200;
	if(rxbuf==NULL)return 0;
	_build_scsi_cmd(s_host_cbw, lun, len, (void *)cScsiCmdRead10, 10);
	s_host_cbw->aCbwCb[2]=(blockAddr>>24)&0xff;
	s_host_cbw->aCbwCb[3]=(blockAddr>>16)&0xff;
	s_host_cbw->aCbwCb[4]=(blockAddr>>8)&0xff;
	s_host_cbw->aCbwCb[5]=(blockAddr>>0)&0xff;
	s_host_cbw->aCbwCb[7]=(blkN>>8)&0xff;
	s_host_cbw->aCbwCb[8]=(blkN>>0)&0xff;
	
	bot_start_scsi_command(s_host_cbw,rxbuf, len);
	return 0;
}
int bot_begin_writeBlock(int lun,unsigned int blockAddr,void*rxbuf,int blkN){
	uint32_t len=blkN*0x200;
	if(rxbuf==NULL)return 0;
	_build_scsi_cmd(s_host_cbw, lun, len, (void *)cScsiCmdWrite10, 10);
	s_host_cbw->aCbwCb[2]=(blockAddr>>24)&0xff;
	s_host_cbw->aCbwCb[3]=(blockAddr>>16)&0xff;
	s_host_cbw->aCbwCb[4]=(blockAddr>>8)&0xff;
	s_host_cbw->aCbwCb[5]=(blockAddr>>0)&0xff;
	s_host_cbw->aCbwCb[7]=(blkN>>8)&0xff;
	s_host_cbw->aCbwCb[8]=(blkN>>0)&0xff;
	
	bot_start_scsi_command(s_host_cbw,rxbuf, len);
	return 0;
}
int bot_read_block(int lun,unsigned int blockAddr,void*rxbuf,int blkN){
	bot_begin_readBlock(lun, blockAddr, rxbuf, blkN);
    USBH_DISK_CODE_ENTER
	wait_bot_trans_cmp(__func__, 1000);
    USBH_DISK_CODE_EXIT
	return(is_bot_transfer_busy());
}
int bot_write_block(int lun,unsigned int blockAddr,void*rxbuf,int blkN){
	bot_begin_writeBlock(lun, blockAddr, rxbuf, blkN);
	wait_bot_trans_cmp(__func__, 1000);
	return(is_bot_transfer_busy());
}

void bot_init_cbk(){
	int i;
	for(i=1;i<AplUsbHost_GetEndpCount();i++){
		if(AplUsbHost_GetRxBuf(i)){
			USB_LOG_I("blkIn ep:%d\n",i);
			AplUsbHost_SetRxCbk(i, (void*)cbk_bot_bulk_in);
			s_bot_bulkIn_endp=i;
		}
		if(AplUsbHost_GetTxBuf(i)){
			USB_LOG_I("blkOut ep:%d\n",i);
			AplUsbHost_SetTxCbk(i, (void*)cbk_bot_bulk_out);
			s_bot_bulkOut_endp=i;
		}
	}
	AplUsbHost_SetSOFCbk(cbk_bot_sof);
}

void bot_init(){
	botSm=0;
	//检查是否是bot协议设备，是则初始化bot回调系统
	if(is_bot_protocol()){//
		memset(&s_bot_scsi_transfer,0,sizeof(s_bot_scsi_transfer));
		USB_LOG_I("bot device found\n");
		s_host_cbw=(MGC_MsdCbw*)malloc(sizeof(MGC_MsdCbw));//bot_get_cbw();
		s_host_csw=(MGC_MsdCsw*)malloc(sizeof(MGC_MsdCsw));//bot_get_csw();
		s_bot_blk_buf=malloc(0x200);
		bot_init_cbk();
	}
    void *bp = AplUsbHost_GetBP();
	HwUsb_Switch2Endp(bp, s_bot_bulkIn_endp);
    int rx_nak_lim = HwUsbHost_GetRxNakLimit(bp);
	//HwUsbHost_SetRxNakLimit(bp,50);
	HwUsb_Switch2Endp(bp, s_bot_bulkOut_endp);
    int tx_nak_lim = HwUsbHost_GetTxNakLimit(bp);
	USB_LOG_I("nak limit rx:%d, tx:%d\n", rx_nak_lim, tx_nak_lim);
	//HwUsbHost_SetTxNakLimit(bp,50);
}
static int afterDly=0;
static int afterCtrl=0;
static uint32_t dly=0;
static uint32_t t0;
static int afterTestSm1=0;
static int afterTestSm0=0;
static int afterResetEndp=0;

static int afterCmdOk=0;
static int afterCmdNg=0;

static void bot_set_dly(int sm_goto,int d){
	afterDly=sm_goto;
	botSm=E_APP_UH_Delay;
	dly=d;
	t0=os_get_tick_counter();
}
static void bot_reset_intf(int sm_goto){
	afterResetEndp=sm_goto;
	botSm=E_APP_UH_ResetEndp;
}
#if 0
static void bot_testUnitRdy_intf(int sm_goto,int sm_goto0){
	afterTestSm1=sm_goto;
	afterTestSm0=sm_goto0;
	botSm=E_APP_UH_TestReady;
}
#endif
static void bot_wait_PipeCmd(int sm_goto,int sm_goto0){
	afterCmdOk=sm_goto;
	afterCmdNg=sm_goto0;
	botSm=E_APP_UH_WaitPipeCmdEnd;
}

static void bot_wait_ctrl_end(int sm_goto){
	afterCtrl=sm_goto;
	botSm=E_APP_UH_WaitSetupOK;
}

void bot_do_reset_bulkin_endpoint(){
	USB_BUILD_SETUP_PKT(&cResetIntf, USB_CLEAR_FEATURE, USB_RT_DIR_Host2Dev, USB_RT_TYP_STD, USB_RT_RECIPIENT_ENDP, s_bot_bulkIn_endp|BIT(7));
	//cResetIntf.idx=s_bot_bulkIn_endp|BIT(7);
	//cResetIntf.len=sizeof(uint16_t);
	usbhost_start_ctrl_transfer(AplUsbHost_GetBP(),&cResetIntf, NULL, 0, /*((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz,*/NULL,NULL);
	while(usbhost_is_ctrl_transfer_busy()!=0);
}

int bot_do_get_bulkin_endpoint_status(){
	int res;
	USB_BUILD_SETUP_PKT(&cResetIntf, USB_GET_STATUS, USB_RT_DIR_Dev2Host, USB_RT_TYP_STD, USB_RT_RECIPIENT_ENDP, s_bot_bulkIn_endp|BIT(7));
	//cResetIntf.idx=s_bot_bulkIn_endp|BIT(7);
	cResetIntf.len=sizeof(uint16_t);
	usbhost_start_ctrl_transfer(AplUsbHost_GetBP(),&cResetIntf, &res, 2, /*((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz,*/NULL,NULL);
	while(usbhost_is_ctrl_transfer_busy()!=0);
	return(res&0xffff);
}

void bot_do_req(){
	AplUsbHost_StartRx(AplUsbHost_GetBP(), s_bot_bulkIn_endp);
}

void bot_do_stop_req(){
	AplUsbHost_StopRx(AplUsbHost_GetBP(), s_bot_bulkIn_endp);
}

//extern int fat_malloc_files_buffer(void);
// extern int fat_free_files_buffer(void);
//extern unsigned int Media_Fs_Init(uint8_t type);

//extern int player_init(int typ);
//extern int is_player_inited();
//extern void player_media(int idx);
//extern int get_musicfile_count(void);
// uint16_t s_bulkIn_status=0;

void bot_proc()
{
	void*bp=AplUsbHost_GetBP();
	int r;
	switch(botSm){
		
		case E_APP_UH_IDLE:
			if(AplUsbHost_IsConfigured()){
                s_udisk_configed_time = sys_time_get();
				USB_LOG_I("usb is configed@ %u ms\r\n", s_udisk_configed_time);
				//bot_init();
				bot_set_dly(E_APP_UH_SetIntface, 100);
			}
			break;
		case E_APP_UH_SetIntface:
			usbhost_start_ctrl_transfer(bp, (uint8_t*)&cSetInterface, NULL, 0, /*((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz,*/NULL,NULL);
			bot_wait_ctrl_end(E_APP_UH_GetLUN);
			USB_LOG_I("usb interface enable\r\n");
			break;
		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		//sm sub functions
		case E_APP_UH_Delay:
			if(os_get_tick_counter()-t0>dly){
				botSm=afterDly;
			}
			break;
			
		case E_APP_UH_WaitSetupOK:
			if(usbhost_is_ctrl_transfer_busy()==0){
				botSm=afterCtrl;
			}
			break;
		case E_APP_UH_WaitPipeCmdEnd:
			r=is_bot_transfer_busy();
			if(r==0){
				bot_set_dly(afterCmdOk, 1);
			}else if(r<0){
				bot_reset_intf(afterCmdNg);
			}
			break;
		////sm sub functions
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case E_APP_UH_GetLUN:
			USB_LOG_I("usb get MaxLUN\r\n");
			usbhost_start_ctrl_transfer(bp, (uint8_t*)&cGetLUN, &numLUN, 1, /*((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz,*/NULL,NULL);
			bot_wait_ctrl_end(E_APP_UH_GetCap);
			break;
		case E_APP_UH_ResetEndp:
			USB_BUILD_SETUP_PKT(&cResetIntf, USB_CLEAR_FEATURE, USB_RT_DIR_OUT, USB_RT_TYP_STD, USB_RT_RECIPIENT_ENDP, s_bot_bulkIn_endp|BIT(7));
			//cResetIntf.idx=s_bot_bulkIn_endp|BIT(7);
			usbhost_start_ctrl_transfer(bp,&cResetIntf, NULL, 0, /*((_t_usb_host_drv*)AplUsbHost_GetDrv())->ep0PktMaxSz,*/NULL,NULL);
			bot_wait_ctrl_end(E_APP_UH_ClearBulkOutHaltOK);
			break;
		case E_APP_UH_ClearBulkOutHaltOK:
			bot_do_req();
			bot_set_dly(E_APP_UH_StopRx, 2);
			break;
		case E_APP_UH_StopRx:
			bot_do_stop_req();
			bot_set_dly(afterResetEndp, 4);
			break;
		case E_APP_UH_TestReady:
			if(bot_test_unit_ready(0)==0){
				botSm=afterTestSm1;//跳至成功后状态
			}else{
				botSm=afterTestSm0;
			}
			break;
		case E_APP_UH_GetCap:
			{
				USB_LOG_I("max LUN=%d\r\n",numLUN);
				bot_begin_inquiry(0, s_bot_blk_buf, 0x24);
				bot_wait_PipeCmd(E_APP_UH_InquiryFull, E_APP_UH_GetCap1);
			}
			break;
		case E_APP_UH_InquiryFull:
			{
				_t_inquiry_info*pinq=(_t_inquiry_info*)s_bot_blk_buf;
				USB_LOG_I("Device Type:%.2x\r\n",pinq->peri_dev_typ);
				if(pinq->rmb) USB_LOG_I("Device is removable\r\n");
				// USB_LOG_I("vendorID=%s\r\n", pinq->vendor_info);
				USB_LOG_I("vendor_info:0x%08X, 0x%08X\n", (uint32_t*)pinq->vendor_info, (uint32_t*)&pinq->vendor_info[4]);
				USB_LOG_I("prod_id:%s\n", pinq->prod_id);
				USB_LOG_I("prod_ver:%s\n", pinq->prod_ver);
				if(pinq->add_length-0x1f>0x24){
					bot_begin_inquiry(0, s_bot_blk_buf, 0x24+(pinq->add_length-0x1f));
					bot_wait_PipeCmd(E_APP_UH_GetCap1, E_APP_UH_GetCap1);
				}else{
					bot_set_dly(E_APP_UH_GetCap1,1);
				}
			}
			break;
		case E_APP_UH_GetCap1:
			{
				bot_begin_readFmtCap(0, s_bot_blk_buf, 12);
				bot_wait_PipeCmd(E_APP_UH_RequestSense, E_APP_UH_RequestSense);
			}
			break;
		case E_APP_UH_RequestSense:
			bot_begin_requstSense(0, s_bot_blk_buf, 0x12);
			bot_wait_PipeCmd(E_APP_UH_InquiryInfo, E_APP_UH_InquiryInfo);
			break;
		case E_APP_UH_InquiryInfo:
			bot_begin_readCap(0, &s_cap_info, sizeof(s_cap_info));
			bot_wait_PipeCmd(E_APP_UH_GetCap3, E_APP_UH_GetCap3);
			break;
		case E_APP_UH_GetCap3:
			bot_begin_readFmtCap(0, s_bot_blk_buf, 0x14);
			bot_wait_PipeCmd(E_APP_UH_GetCap4, E_APP_UH_GetCap4);
			break;
		case E_APP_UH_GetCap4:
			bot_begin_readCap(0, &s_cap_info, sizeof(s_cap_info));
			bot_wait_PipeCmd(E_APP_UH_ReadBlock, E_APP_UH_ReadBlock);
			break;
		case E_APP_UH_ReadBlock:
			bot_begin_readBlock(0, 0,s_bot_blk_buf, 1);
			bot_wait_PipeCmd(E_APP_UH_UDiskInit, E_APP_UH_GetCap);
			break;
		case E_APP_UH_UDiskInit:
			USB_LOG_I("read block-0 success\r\n");
			bot_set_dly(E_APP_UH_PlayMedia,1);
			usb_dbg_buf_show("block0",s_bot_blk_buf,0x200);
			botInited=1;
            usbh_udisk_init_cmp_callback();
            botSm = E_APP_UH_PlayMedia;// : E_APP_UH_Stop;
			break;
		case E_APP_UH_PlayMedia:
			break;
		case E_APP_UH_Stop:
			break;
		default:
			break;
	}
}
// static CCallback_P0 cbk_bot_lost=NULL;
// void bot_set_lost_deviceCbk(void*cbk){
// 	cbk_bot_lost=(CCallback_P0)cbk;
// }
void bot_release(){
	if(s_bot_blk_buf)free(s_bot_blk_buf);
	s_bot_blk_buf=NULL;
	if(s_host_cbw)free(s_host_cbw);
	s_host_cbw=NULL;
	if(s_host_csw)free(s_host_csw);
	s_host_csw=NULL;
    s_udisk_configed_time = 0;
	botInited=0;
}

__attribute__((weak))
void usbh_udisk_init_cmp_callback(void)
{
	USB_LOG_I("%s\n", __FUNCTION__);
    // if(udisk_mode_auto_sw_get())
    // {
    //     extern void system_work_mode_set_button(uint8_t mode);
    //     system_work_mode_set_button(SYS_WM_UDISK_MODE);
    // }
}

//udisk is lost/pulled out (in isr)
__attribute__((weak))
void usbh_udisk_lost_callback(void)
{
	USB_LOG_I("%s\n", __FUNCTION__);
//     if(!app_is_udisk_mode()) return;
// #if (CONFIG_APP_MP3PLAYER == 1)
//     set_app_player_flag_hw_detach();
// #endif
//     system_work_mode_change_button();
}

void unload_bot_drv(){	
	USB_LOG_I("%s\r\n", __FUNCTION__);
	// fat_free_files_buffer();
    usbh_udisk_lost_callback();
	bot_release();
}

void load_bot_drv(){
    s_udisk_configed_time = 0;
	botInited=0;
	AplUsbHost_SetAppInitCbk(bot_init);
	AplUsbHost_SetLostDeviceCbk(unload_bot_drv);
}


ERR_UDISK udisk_init(void){
	return UDISK_RET_OK;
}
ERR_UDISK udisk_rd_blk_sync(int sector,int count,void*buff){
//	if(AplUsb_IsConfigured())return bot_read_block(0, sector,buff, count);
//	else
//		return UDISK_RET_ERR;
	int r;//=bot_read_block(0, sector,buff, count);
	r=bot_read_block(0, sector,buff, count);
	return r;
}

ERR_UDISK udisk_write_blk_sync(int sector,int count,void*buff){
	return bot_write_block(0, sector,buff, count);
}
ERR_UDISK udisk_rd_blk_async(int sector,int count,void*buff,void*cbk){
//	if(AplUsb_IsConfigured())return bot_read_block(0, sector,buff, count);
//	else
//		return UDISK_RET_ERR;
	int r=UDISK_RET_OK;//=bot_read_block(0, sector,buff, count);
	bot_begin_readBlock(0, sector,buff, count);
	_t_msd_scsi_cmd_info*ptrans=&s_bot_scsi_transfer;
	ptrans->cmpl=(void (*)(void*,int))cbk;
	return r;
}

ERR_UDISK udisk_write_blk_async(int sector,int count,void*buff,void*cbk){
	bot_begin_writeBlock(0, sector,buff, count);
	_t_msd_scsi_cmd_info*ptrans=&s_bot_scsi_transfer;
	ptrans->cmpl=(void (*)(void*,int))cbk;
	return UDISK_RET_OK;
}
int udisk_is_attached(void){
	return botInited;
}

void udisk_uninit(void){

}
uint32_t udisk_get_size(void){
	uint64_t res=(s_cap_info.lastBlock+1);
	res*=s_cap_info.blockSize;
	return res;
}

int udisk_is_enumerated()
{
	return AplUsbHost_IsConfigured();
}

//get system time when udisk enumurated
uint32_t udisk_enumed_time_get(void)
{
	return s_udisk_configed_time;
}


