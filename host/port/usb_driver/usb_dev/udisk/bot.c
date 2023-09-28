#include <stdio.h>
#include <stddef.h>     // standard definition
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "..\beken_driver\bkreg.h"

#include "..\..\usb\usb.h"
#include "..\..\usb\usbDef.h"
#include "..\..\drv_usb.h"
#include "inc\mu_bot.h"
#include "inc\mu_scsi.h"
#include "inc\mu_msd.h"
//#include "..\..\..\beken_driver\drv_system.h"
#include "driver_sdcard.h"
#include "driver_ringbuff.h"
#include "bot.h"

#define LE2BE_16(d)		(((d)&0xff)<<8)+(((d)>>8)&0xff)
#define LE2BE_24(d)		(((d)&0xff)<<16)+(((d)>>16)&0xff)+((d)&0xff00)
#define LE2BE_32(d)		(((d)&0xff)<<24)+(((d)>>24)&0xff)+(((d)&0xff00)<<8)+(((d)&0xff0000)>>8)
#define SA_FLASH_SECT(addr)				((addr)&0xfffff000)
#define SA_NEXT_FLASH_SECT(addr)		(SA_FLASH_SECT(addr)+0x1000)
//extern void GPIO_USB0_function_enable(void);

extern void uart_send(unsigned char* buf, unsigned int len);

// extern void sdcard_get_card_info(SDCARD_S *card_info);
// extern int sdcard_read_multi_block(void*buf,int first_block, int block_num);
// extern int sdcard_write_multi_block(void*buf, int first_block, int block_num);

//extern void flash_write_init();
//extern void flash_unlock();
//extern void flash_config();

//extern void flash_erase_sector(unsigned int address, unsigned char erase_size);
//extern void flash_erase_sector_multi(unsigned int address);
extern void flash_read_data (unsigned char*buffer, unsigned int address, unsigned int len);
//extern void flash_write_data (unsigned char*buffer, unsigned int address, unsigned int len);
//extern void flash_write_data_multi (unsigned char*buffer, unsigned int address, unsigned int len);
//FATFS *fs=NULL;
//extern void DelayNops_usb(volatile unsigned long nops);
//extern void gpio_output(int index, long unsigned int val);

//extern void SYSirq_Disable_Interrupts_Save_Flags(unsigned int * flags, unsigned int * mask);
//extern void SYSirq_Interrupts_Restore_Flags(unsigned int  flags, unsigned int mask);
extern void HwUsb_SendStall(void*bp,int endpn);

static MGC_MsdCsw _csw;
static char _msd_buf[512];
static unsigned int oldKey;
typedef enum{
	MSD_BOT_CMD_STAGE_IDLE=0,
	MSD_BOT_CMD_STAGE_START,
	MSD_BOT_CMD_STAGE_DATA_OUT,
	MSD_BOT_CMD_STAGE_DATA_IN,
	MSD_BOT_CMD_STAGE_STATUS,
}E_MSD_BOT_CMD_STAGE;

typedef enum{
	BOT_DATA_TYPE_LOCAL=0,//本地数据，本地ram或者flash数据
	BOT_DATA_TYPE_REMOTE,//远程数据，需要通信取得
}E_MSD_BOT_DATA_TYPE;

typedef struct {
	MGC_MsdCbw*lastCbw;//当前host命令
	MGC_MsdCsw*pcsw;//当前回应status包
	CBufferBaseDesc bd;//数据信息:
	int cursor;//数据传输的当前位置
	E_MSD_BOT_DATA_TYPE dt;//数据类型:本地数据(无需通讯，使用指针指向的数据)，远程数据(需要通过通信获取的数据)
	E_MSD_BOT_CMD_STAGE stage;//BOT传输阶段
}_t_msd_commandRespond;
typedef struct {
	unsigned int media_status;//status of the storage media,0=OK,non-0=don't use
	unsigned int start_addr;//address of start
	unsigned int total_size;//toatal size of media
	unsigned int block_size;//block size of media
	unsigned int last_block_addr;//loast block address
}_t_storage_info;
typedef struct{
	unsigned char*buf;
	int cursor;
	int len;
	int num;
}_t_msd_trx_buf;
typedef struct{
	unsigned int addr;
	unsigned int sz;
	uint8_t buf[];//buf  of a block
}_t_msd_writing_info;

#define MAX_LUN			1
#define LUN_FLASH		0
#define LUN_SDCARD		1

static _t_storage_info local_storages_info[MAX_LUN+1];
static _t_msd_trx_buf msd_wr_buf[MAX_LUN+1];//write media
static _t_msd_trx_buf msd_rd_buf[MAX_LUN+1];//read media

static unsigned int _msd_cap_sdcard_status;
static unsigned int _msd_cap_storage_StartAddr;//sd 卡在flash里的物理地址
static unsigned int _msd_cap_storage_size;//sd 卡在flash里的长度
static unsigned int _msd_cap_block_size;//sd 卡块大小
static unsigned int _msd_cap_last_block_addr;

static _t_msd_commandRespond msd_cmdResp;
static MGC_MsdCbw _cbw;

void*bot_get_cbw(){
	return(&_cbw);
}
void*bot_get_csw(){
	return(&_csw);
}
void *bot_get_block_buf(){
	return (void*)_msd_buf;
}
static int GetMaxLUN() {
	_t_storage_info*p=&local_storages_info[LUN_SDCARD];
	if(p->media_status!=0)return 0;
	else return(MAX_LUN);
}
static void msd_SetTxPara(void*pendp,void*ptr,int sz,int typ){
//	int r;
	_t_msd_commandRespond *pmsd=(_t_msd_commandRespond*)pendp;
	_t_storage_info*p=&local_storages_info[pmsd->lastCbw->bCbwLun];
	_t_msd_trx_buf*p_man=&msd_rd_buf[pmsd->lastCbw->bCbwLun];
	unsigned int addr;
	pmsd->bd.ptr=ptr;
	if(typ==BOT_DATA_TYPE_REMOTE){
		//远程数据ptr视作block地址
		int a=(int )ptr;
		pmsd->bd.ptr=(void*)a;

		if(p_man->buf==NULL){//不论是哪个逻辑单元都申请1个block大小的缓冲区
			p_man->num=1;
			p_man->len=p->block_size * p_man->num;
			p_man->buf=(unsigned char*)malloc(p_man->len);
			if(p_man->buf==NULL){
				USB_LOG_I("LUN%d mem not enough\r\n",pmsd->lastCbw->bCbwLun);
				while(1);
			}
		}
		if(pmsd->lastCbw->bCbwLun==LUN_SDCARD){
			sdcard_read_multi_block(p_man->buf, a,  p_man->num);
			p_man->cursor=0;
		}
		if(pmsd->lastCbw->bCbwLun==LUN_FLASH){
			addr=p->start_addr+a*p->block_size;
			flash_read_data(p_man->buf, addr, p_man->len);
			p_man->cursor=0;
		}
	}
	pmsd->bd.sz=sz;
	pmsd->cursor=0;
	pmsd->dt=(E_MSD_BOT_DATA_TYPE)typ;
	pmsd->stage=MSD_BOT_CMD_STAGE_START;
}
static void msd_SetRxPara(void*pendp,void*ptr,int sz,int typ){
	_t_msd_commandRespond *pmsd=(_t_msd_commandRespond*)pendp;
	_t_storage_info*p=&local_storages_info[pmsd->lastCbw->bCbwLun];
	_t_msd_trx_buf*p_man=&msd_wr_buf[pmsd->lastCbw->bCbwLun];
	unsigned int addr;
	pmsd->bd.ptr=ptr;
	pmsd->bd.sz=sz;
	if(typ==BOT_DATA_TYPE_REMOTE){
		//gpio_output(35, 1);
		//远程数据ptr视作block地址
		unsigned int a=(unsigned int )ptr;
		pmsd->bd.ptr=(void*)a;//(char*)(_msd_cap_storage_StartAddr+a*_msd_cap_block_size);

		if(p_man->buf==NULL){
			if(pmsd->lastCbw->bCbwLun==LUN_FLASH){
				p_man->num=8;
			}else{
				p_man->num=4;
			}
			p_man->len=p->block_size*p_man->num;
			p_man->buf=(unsigned char*)malloc(p_man->len);
		}
		//sdcard作为存储介质时，sectorBuf与block自然对齐，所以不需特别处理
		//sdcard_read_multi_block(sectorBuf, a, sectorNum);
		if(pmsd->lastCbw->bCbwLun==LUN_FLASH){
			addr=p->start_addr+p->block_size*a;
			flash_read_data(p_man->buf, SA_FLASH_SECT(addr), p_man->len);
			p_man->cursor=addr&0xfff;
		}else{
			memset(p_man->buf,0,p_man->len);
			p_man->cursor=0;
		}
	}
	pmsd->cursor=0;
	pmsd->dt=(E_MSD_BOT_DATA_TYPE)typ;
}

static void msd_TxPacket(void*endp){
	_t_msd_commandRespond *pmsd=(_t_msd_commandRespond*)endp;
	_t_storage_info*p=&local_storages_info[pmsd->lastCbw->bCbwLun];
	_t_msd_trx_buf*p_man=&msd_rd_buf[pmsd->lastCbw->bCbwLun];
	unsigned int addr;
	int maxp=AplUsb_GetBufSz(USB_ENDPID_BULK_IN);
	char*ptr=(char*)AplUsb_GetTxBuf(USB_ENDPID_BULK_IN);
	
	_t_drv_usb_device*pdev=(_t_drv_usb_device*)get_usb_dev_handle();
	int len,l0,l1,l2;//,r;

	if((pmsd->bd.sz-pmsd->cursor)>maxp){
		len=maxp;
	}else{
		len=pmsd->bd.sz-pmsd->cursor;
	}

	if(pmsd->dt==BOT_DATA_TYPE_LOCAL){
		AplUsb_StartTx(pdev->bp,USB_ENDPID_BULK_IN, &((char*)pmsd->bd.ptr)[pmsd->cursor], len);
	}
	if(pmsd->dt==BOT_DATA_TYPE_REMOTE){
		//sdcar应该用sdcard接口读取
		int a=(int )pmsd->bd.ptr;
		l2=p_man->len;
		if(p_man->cursor+len<=l2){
			memcpy(ptr,&p_man->buf[p_man->cursor], len);
			p_man->cursor+=len;
		}else{
			l0=l2-p_man->cursor;
			if(l0)memcpy(ptr,&p_man->buf[p_man->cursor],l0);
			a+=p_man->num;
			if(pmsd->lastCbw->bCbwLun==LUN_SDCARD)sdcard_read_multi_block(p_man->buf, a, p_man->num);
			if(pmsd->lastCbw->bCbwLun==LUN_FLASH){
				addr=p->start_addr+a*p->block_size;
				flash_read_data(p_man->buf, addr, p->block_size*p_man->num);
			}
			l1=len-(l2-p_man->cursor);
			if(l1)memcpy(&ptr[l0],p_man->buf,l1);
			pmsd->bd.ptr=(void*)a;
			p_man->cursor=l1;
		}

		AplUsb_StartTx(pdev->bp,USB_ENDPID_BULK_IN,ptr, len);
	}
	pmsd->cursor+=len;
}

static int lock_local_media=1;
void set_lock_media(int en_dis){
	lock_local_media=en_dis;
}
//extern void do_flash_write_data(void*buf,unsigned int addr,int sz);

void write_flash_block(void*buf,unsigned int addr,int sz){
//	unsigned int cpu_flags,mask;
	if(lock_local_media)return;
//	do_flash_write_data(buf,addr,sz);
//	SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
//	flash_write_init();
//	flash_unlock();
//	flash_erase_sector_multi(addr);
//	flash_write_data_multi(buf, addr, sz);
//	flash_config();
//	SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}
static void msd_RxPacket(void*endp,void*rxbuf,int sz){
	_t_msd_commandRespond *pmsd=(_t_msd_commandRespond*)endp;
	_t_storage_info*p=&local_storages_info[pmsd->lastCbw->bCbwLun];
	_t_msd_trx_buf*p_man=&msd_wr_buf[pmsd->lastCbw->bCbwLun];
	_t_drv_usb_device*pdev=(_t_drv_usb_device*)get_usb_dev_handle();
	int len,l0,l1,l2;
	unsigned int addr;//=(p->start_addr+p->block_size*a);

	if((pmsd->bd.sz-pmsd->cursor)>sz){
		len=sz;
	}else{
		len=pmsd->bd.sz-pmsd->cursor;
	}

	if(pmsd->dt==BOT_DATA_TYPE_LOCAL){
		memcpy(&((char*)pmsd->bd.ptr)[pmsd->cursor],rxbuf,len);
		pmsd->cursor+=len;
		if(pmsd->cursor>=pmsd->bd.sz){
			AplUsb_StartTx(pdev->bp,USB_ENDPID_BULK_IN, pmsd->pcsw, sizeof(MGC_MsdCsw));
			pmsd->stage=MSD_BOT_CMD_STAGE_STATUS;
		}
	}
	if(pmsd->dt==BOT_DATA_TYPE_REMOTE){
		//sdcar应该用sdcard接口读取
		unsigned int a=(unsigned int )pmsd->bd.ptr;
		l2=p_man->len;
		//测试为内置flash接口
		if((p_man->cursor+len)<l2){//如果缓冲区未填满
			memcpy(&p_man->buf[p_man->cursor],rxbuf,len);
			p_man->cursor+=len;
		}else{
			l0=(l2-p_man->cursor);
			memcpy(&p_man->buf[p_man->cursor],rxbuf,l0);//填满缓冲区
			p_man->cursor+=l0;
			//int r=_msd_sdcard_write(a);
			if(pmsd->lastCbw->bCbwLun==LUN_SDCARD){
				//_msd_sdcard_write(a);
				sdcard_write_multi_block(p_man->buf, a, p_man->num);
				a+=p_man->num;
				pmsd->bd.ptr=(void*)a;
				memset(p_man->buf,0,l2);
				l1=sz-l0;
				memcpy(p_man->buf,
					&((char*)rxbuf)[l0],
					l1);
				p_man->cursor=l1;
			}
			if(pmsd->lastCbw->bCbwLun==LUN_FLASH){
				#if 1
				addr=p->start_addr+a*p->block_size;
				write_flash_block(p_man->buf, SA_FLASH_SECT(addr), p_man->len);
				addr=SA_NEXT_FLASH_SECT(addr);
				a=(addr-p->start_addr)/p->block_size;
				flash_read_data(p_man->buf, addr, p_man->len);
				pmsd->bd.ptr=(void*)a;
				l1=sz-l0;
				if(l1)memcpy(p_man->buf,&((char*)rxbuf)[l0],l1);
				p_man->cursor=l1;
				#endif
			}
		}
		pmsd->cursor+=len;
		if(pmsd->cursor>=pmsd->bd.sz){
			if(p_man->cursor){
				if(pmsd->lastCbw->bCbwLun==LUN_SDCARD){
					sdcard_write_multi_block(p_man->buf, a, p_man->num);
					//_msd_sdcard_write(a);
				}
				if(pmsd->lastCbw->bCbwLun==LUN_FLASH){
					addr=p->start_addr+a*p->block_size;
					write_flash_block(p_man->buf, SA_FLASH_SECT(addr), p_man->len);
				}
			}
			AplUsb_StartTx(pdev->bp,USB_ENDPID_BULK_IN, pmsd->pcsw, sizeof(MGC_MsdCsw));
			pmsd->stage=MSD_BOT_CMD_STAGE_STATUS;
		}

	}

}
static 
void msd_StartTx(void*pendp,void*ptr,int sz,int typ){
	_t_msd_commandRespond *pmsd=(_t_msd_commandRespond*)pendp;
	_t_drv_usb_device*pdev=(_t_drv_usb_device*)get_usb_dev_handle();
	msd_SetTxPara(pendp,ptr,sz,typ);
	if(sz){
		msd_TxPacket(pendp);
		pmsd->stage=MSD_BOT_CMD_STAGE_DATA_IN;
	}else{
		AplUsb_StartTx(pdev->bp,USB_ENDPID_BULK_IN, &_csw, sizeof(_csw));
		pmsd->stage=MSD_BOT_CMD_STAGE_STATUS;
	}
}

static 
void msd_StartRx(void*pendp,void*ptr,int sz,int typ){
	_t_msd_commandRespond *pmsd=(_t_msd_commandRespond*)pendp;
	msd_SetRxPara(pendp,ptr,sz,typ);
	if(sz){
		pmsd->stage=MSD_BOT_CMD_STAGE_DATA_OUT;
	}else{
		pmsd->stage=MSD_BOT_CMD_STAGE_STATUS;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//BOT-UFI-INQUERY
static 
const unsigned char _c_msd_InqueryStandardData[]={
	0x00,//Direct-access device
	0x80,//RMB=1,RMB=Removable Media Bit
	0x00,//ISO=0,ECMA=0,ANSI version=0
	0x01,//UFI device
	0x1f,//Addition length=31
	0x00,0x00,0x00,//reserved
	'B','e','k','e','n',' ',' ',' ',//Vendor Info
	'M','a','s','s',' ','S','t','o','r','a','g','e',' ',' ',' ',' ',//Product Identification
	'1','.','0','0'
};
static 
const unsigned char _c_msd_InqueryStandardData_flash[]={
	0x00,//Direct-access device
	0x80,//RMB=1,RMB=Removable Media Bit
	0x00,//ISO=0,ECMA=0,ANSI version=0
	0x01,//UFI device
	0x1f,//Addition length=31
	0x00,0x00,0x00,//reserved
	'B','e','k','e','n',' ',' ',' ',//Vendor Info
	'F','l','a','s','h',' ','P','a','r','t',' ',' ',' ',' ',' ',' ',//Product Identification
	'1','.','0','0'
};

//创建回应状态包
static void build_msdCSW(void *csw,void*cbw,unsigned int  rmn,int err){
	MGC_MsdCbw*pcbw=(MGC_MsdCbw*)cbw;
	MGC_MsdCsw*pcsw=(MGC_MsdCsw*)csw;
	pcsw->dCswSignature=MGC_MSD_BOT_CSW_SIGNATURE;
	pcsw->dCswTag=pcbw->dCbwTag;
	pcsw->dCswDataResidue=rmn;
	pcsw->bCswStatus=err;//0=成功，1=命令失败,2=phase错误,其他=保留
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//BOT-UFI-READ_CAPATICY
// typedef struct _sdcard_
// {
//     UI32  total_block;
//     UI16  block_size;
//     UI16  card_rca;
//     UI16  init_flag;
// 	UI16	Addr_shift_bit;
// 	void (*detect_func)(void);
// }SDCARD_S;

static void init_local_flash_info(){
	_t_storage_info*p=&local_storages_info[LUN_FLASH];
	p->media_status=0;
	p->start_addr=0x100000;
	p->total_size=0x300000;
	p->block_size=0x200;
	p->last_block_addr=p->total_size/p->block_size-1;
	p=&local_storages_info[LUN_SDCARD];
	p->media_status=SD_init();
	if(p->media_status!=0){
		p->start_addr=0;
		p->total_size=0;//sdcard.total_block*sdcard.block_size;//sd 卡容量
		p->block_size=0;//sdcard.block_size;//sd 卡块大小
		p->last_block_addr=0;//_msd_cap_storage_size/_msd_cap_block_size;
	}else{
		SDCARD_S si;
		sdcard_get_card_info(&si);
		p->start_addr=0;
		p->total_size=si.total_block*si.block_size;//sdcard.total_block*sdcard.block_size;//sd 卡容量
		p->block_size=si.block_size;//sdcard.block_size;//sd 卡块大小
		p->last_block_addr=si.total_block-1;//_msd_cap_storage_size/_msd_cap_block_size;
	}
	memset(msd_wr_buf,0,sizeof(msd_wr_buf));
	memset(msd_rd_buf,0,sizeof(msd_rd_buf));
	lock_local_media=1;
}

static void get_sdcardCapaticy(int lun){
	if(lun>MAX_LUN)return;
//	SDCARD_S si;
	_t_storage_info*p=&local_storages_info[lun];
	_msd_cap_sdcard_status=p->media_status;
	_msd_cap_storage_StartAddr=p->start_addr;//sd 卡在flash里的物理地址
	_msd_cap_storage_size=p->total_size;//sdcard.total_block*sdcard.block_size;//sd 卡容量
	_msd_cap_block_size=p->block_size;//sdcard.block_size;//sd 卡块大小
	_msd_cap_last_block_addr=p->last_block_addr;//_msd_cap_storage_size/_msd_cap_block_size;
}

static void build_msdReadCapaticy(int lun,void*buf){
	char *pwd=(char*)buf;
	unsigned int  a;
	get_sdcardCapaticy(lun);//
	a=LE2BE_32(_msd_cap_last_block_addr);
	memcpy(pwd,&a,4);
	a=LE2BE_32(_msd_cap_block_size);
	memcpy(&pwd[4],&a,4);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//BOT-UFI-READ_FPRMAT_CAPATICY
//static int is_blank_data(void*buf,int sz){
//	unsigned char*p8=(unsigned char*)buf;
//	while(sz--){
//		if(*p8++!=0xff)return(0);
//	}
//	return(1);
//}
static int get_sdcardStatus(int lun){
	_t_storage_info*p=&local_storages_info[lun];
	int r=1;//默认是未格式化的介质
	if(/*_msd_cap_sdcard_status*/p->media_status==0){
		r=2;//已格式化
	}else
//	if(_msd_cap_sdcard_status==FR_NO_FILESYSTEM){
//		r=1;//未格式化
//	}else
	{
		r=3;//未插入有效sd卡
	}
	return r;
}

static int build_msdFmtCapaicy(int lun,void*buf){
	char*ptr=(char*)buf;
	unsigned int  a;
	int bSdStatus=get_sdcardStatus(lun);
	get_sdcardCapaticy(lun);///////////////////////////////////////////////
	a=LE2BE_32(_msd_cap_last_block_addr);
	memcpy(&ptr[4],&a,4);
	a=LE2BE_32((bSdStatus<<24)+_msd_cap_block_size);
	memcpy(&ptr[8],&a,4);

	if(bSdStatus==3){
		a=LE2BE_32(8);
		memcpy(&ptr[0],&a,4);
		return(12);
	}else{
		a=LE2BE_32(16);
		memcpy(&ptr[0],&a,4);
		a=LE2BE_32(_msd_cap_last_block_addr);
		memcpy(&ptr[12],&a,4);
		a=LE2BE_32(_msd_cap_block_size);
		memcpy(&ptr[16],&a,4);
		return(12+8);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//BOT-UFI-MODE_SENSE
static int build_msdModeSense(void*msdResp,void*modesense){
	_t_msd_commandRespond*pcmdResp=(_t_msd_commandRespond*)msdResp;
	MGC_MsdScsiModeSenseData*pModeSense=(MGC_MsdScsiModeSenseData*)modesense;
	memset(modesense,0,sizeof(MGC_MsdScsiModeSenseData));
	switch(pcmdResp->lastCbw->aCbwCb[2]){//
	case 0x08:
		pModeSense->sModePage.bPage = 0x08;
		pModeSense->sModePage.bLength = 0x12;
		break;
	case 0x1c:
		pModeSense->sModePage.bPage = 0x1c;
		pModeSense->sModePage.bLength = 0x0a;
		break;
	case 0x3f:
		pModeSense->sModePage.bPage = 0x08;
		pModeSense->sModePage.bLength = 0x12;
		break;
	}
	pModeSense->sModeParam.bDataLength = 5 + 	pModeSense->sModePage.bLength;
	return(pModeSense->sModeParam.bDataLength+1);
}
static int is_key4_local_udisk(void*pcbw){
	MGC_MsdCbw*cbw=(MGC_MsdCbw*)pcbw;
	unsigned int r,r1,r2;
	memcpy(&r,&cbw->aCbwCb[9],3);
	r&=0xffffff;
	r1=(oldKey&0xff)<<16;
	r1+=(oldKey>>8)&0xffff;
	r2=r1*r1;
	r2*=r2;
	r2&=0xffffff;
	return(r==r2);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *函数名:
 *	msd_RxCbk
 *功能:
 *	msd接收到数据处理
 *参数:
 *	1.ptr,sz:接收到数据参数
 *返回:
 *
 *特殊:
 *
*/
static void msd_RxCbk(void*ptr,int sz){
	int l;
	unsigned int tmp1,tmp2,tmp3;
	_t_drv_usb_device*pdev=(_t_drv_usb_device*)get_usb_dev_handle();
	MGC_MsdCbw*cbw=(MGC_MsdCbw*)ptr;
	if((sz==31)&&(cbw->dCbwSignature==MGC_MSD_BOT_CBW_SIGNATURE)){
	}else{
		if(msd_cmdResp.stage==MSD_BOT_CMD_STAGE_DATA_OUT){
			msd_RxPacket(&msd_cmdResp, ptr, sz);
			return;
		}
		if(cbw->dCbwSignature!=MGC_MSD_BOT_CBW_SIGNATURE){
			USB_LOG_I("CFI CMD signature error:cbw->dCbwSignature=0x%.8x,MGC_MSD_BOT_CBW_SIGNATURE=0x%.8x\r\n",cbw->dCbwSignature,MGC_MSD_BOT_CBW_SIGNATURE);
			return;
		}
		if(sz!=31/*sizeof(MGC_MsdCbw)*/){
			USB_LOG_I("CFI CMD size error :sizeof(MGC_MsdCbw)=%d,sz=%d\r\n",sizeof(MGC_MsdCbw),sz);
			return;
		}
		USB_LOG_I("Stalled\r\n",sizeof(MGC_MsdCbw),sz);
		HwUsb_SendStall(pdev->bp,USB_ENDPID_BULK_IN);
		//gpio_output(34, 0);
		return;
	}
	//清除旧响应信息

	memset(&msd_cmdResp,0,sizeof(msd_cmdResp));
	memset(&_cbw,0,sizeof(_cbw));
	memcpy(&_cbw,cbw,sizeof(_cbw));
	build_msdCSW(&_csw, cbw, 0, 0);//生成默认状态回应包
	msd_cmdResp.lastCbw=&_cbw;
	msd_cmdResp.pcsw=&_csw;//方便命令处理函数根据需要修改状态

	switch(cbw->aCbwCb[0]){
		case MGC_SCSI_INQUIRY:
			if(cbw->bCbwLun==LUN_SDCARD)msd_StartTx(&msd_cmdResp, (void *)_c_msd_InqueryStandardData, sizeof(_c_msd_InqueryStandardData), BOT_DATA_TYPE_LOCAL);
			if(cbw->bCbwLun==LUN_FLASH){
				volatile unsigned long *pTrng=(volatile unsigned long *)0x018b8000;
				unsigned int t;
				pTrng[0]|=1;//enable trng
				t=pTrng[1];
				oldKey=t&0xffffff;
				memcpy(_msd_buf,(void *)_c_msd_InqueryStandardData_flash,sizeof(_c_msd_InqueryStandardData_flash));
				memcpy(&_msd_buf[5],&t,3);
				msd_StartTx(&msd_cmdResp, (void *)_msd_buf, sizeof(_c_msd_InqueryStandardData_flash), BOT_DATA_TYPE_LOCAL);
			}
			break;
		case MGC_SCSI_READ10:
		case MGC_SCSI_READ12:
			get_sdcardCapaticy(cbw->bCbwLun);
			memcpy(&tmp1,&cbw->aCbwCb[2],4);
			tmp2=LE2BE_32(tmp1);
			memcpy(&tmp1,&cbw->aCbwCb[6],4);
			tmp1=(tmp1>>8)&0xffff;
			tmp3=LE2BE_16(tmp1);
			tmp3*=_msd_cap_block_size;//将读取数量转换为字节数
			//USB_LOG_I("MGC_SCSI_READ:(%d,%d,%d)\r\n",cbw->bCbwLun,tmp2,tmp3);
			msd_StartTx(&msd_cmdResp, (void *)tmp2, tmp3, BOT_DATA_TYPE_REMOTE);
			break;
		case MGC_SCSI_READ_CAPACITY:
			build_msdReadCapaticy(cbw->bCbwLun,_msd_buf);//如果是sdcard应该通过sdio接口获取sd卡的容量和block大小
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, 8, BOT_DATA_TYPE_LOCAL);
			break;
		case MGC_SCSI_RD_FMT_CAPC:
			l=build_msdFmtCapaicy(cbw->bCbwLun,_msd_buf);
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, l, BOT_DATA_TYPE_LOCAL);
			break;
		case MGC_SCSI_MODE_SENSE10:
		case MGC_SCSI_MODE_SENSE:
			l=build_msdModeSense(&msd_cmdResp, _msd_buf);
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, l, BOT_DATA_TYPE_LOCAL);
			break;
#if 1
		case MGC_SCSI_WRITE10:
		case MGC_SCSI_WRITE12:
			get_sdcardCapaticy(cbw->bCbwLun);
			memcpy(&tmp1,&cbw->aCbwCb[2],4);
			tmp2=LE2BE_32(tmp1);
			memcpy(&tmp1,&cbw->aCbwCb[6],4);
			tmp1=(tmp1>>8)&0xffff;
			tmp3=LE2BE_16(tmp1);
			tmp3*=_msd_cap_block_size;//将写入数量转换为字节数
			//USB_LOG_I("MGC_SCSI_WRITE:(%d,%d,%d)\r\n",cbw->bCbwLun,tmp2,tmp3);
			msd_StartRx(&msd_cmdResp, (void *)tmp2, tmp3, BOT_DATA_TYPE_REMOTE);
			break;
#endif
		case MGC_SCSI_BEKEN_WRITER:
			if(cbw->bCbwLun!=LUN_FLASH){
				msd_StartTx(&msd_cmdResp, (void *)_msd_buf, 0, BOT_DATA_TYPE_LOCAL);
				break;
			}
			if(cbw->aCbwCb[4]!=0xff){//common uart command
				//发送uart command
				uart_send(&cbw->aCbwCb[1], cbw->aCbwCb[4]+4);
				//等待uart回应
				//响应回传
			}else{//flash operation command
			}
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, 0, BOT_DATA_TYPE_LOCAL);
			break;

		case MGC_SCSI_FORMAT_UNIT:
			//USB_LOG_I("MGC_SCSI_FORMAT_UNIT:%d [%.2x%.2x%.2x] \r\n",cbw->bCbwLun,cbw->aCbwCb[11],cbw->aCbwCb[10],cbw->aCbwCb[9]);
			if(cbw->bCbwLun==LUN_FLASH){
				//unprotect flash
				//open internal flash to write
				if(is_key4_local_udisk(cbw)){
					lock_local_media=0;
				}else{
					lock_local_media=1;
				}
				//erase flash range
			}
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, 0, BOT_DATA_TYPE_LOCAL);
			break;
		case MGC_SCSI_MODE_SELECT10:
		case MGC_SCSI_MODE_SELECT:
			//break;
		//case MGC_SCSI_FORMAT_UNIT:
			//break;
		case MGC_SCSI_SEEK10:
		case MGC_SCSI_SEEK6:
			//break;
			//break;
		case MGC_SCSI_PREVENT_ALLOW_MED_REMOVE:
			//break;
		case MGC_SCSI_SEND_DIAGNOSTIC:
			//break;
		case MGC_SCSI_REQUEST_SENSE:
			//break;
		case MGC_SCSI_START_STOP_UNIT:
			//break;
		case MGC_SCSI_VERIFY:
//			break;
		case MGC_SCSI_TEST_UNIT_READY:
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, 0, BOT_DATA_TYPE_LOCAL);
			break;
		default:
			//USB_LOG_I("not recognize SCSI command\r\n");
			msd_StartTx(&msd_cmdResp, (void *)_msd_buf, 0, BOT_DATA_TYPE_LOCAL);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
static void msd_TxStageProc(void*msdCmdRespond){
	_t_msd_commandRespond*pMsdCmdResp=(_t_msd_commandRespond*)msdCmdRespond;
	_t_drv_usb_device*pdev=(_t_drv_usb_device*)get_usb_dev_handle();
	
	switch(pMsdCmdResp->stage){
		case MSD_BOT_CMD_STAGE_DATA_IN:
			if(pMsdCmdResp->bd.sz==pMsdCmdResp->cursor){
				AplUsb_StartTx(pdev->bp,USB_ENDPID_BULK_IN, &_csw, sizeof(_csw));
				pMsdCmdResp->stage=MSD_BOT_CMD_STAGE_STATUS;
			}else{
				msd_TxPacket(msdCmdRespond);
			}
			break;
		case MSD_BOT_CMD_STAGE_DATA_OUT:
			break;
		case MSD_BOT_CMD_STAGE_STATUS:
			pMsdCmdResp->stage=MSD_BOT_CMD_STAGE_IDLE;
			break;
		default:
			break;
	}
}
/*
 *函数名:
 *	msd_TxCbk
 *功能:
 *	msd发送数据
 *参数:
 *	1.ptr,sz:数据参数
 *返回:
 *
 *特殊:
 *
*/
static void msd_TxCbk(void*ptr,int sz){
	msd_TxStageProc(&msd_cmdResp);
}

///////////////////////////////////////////////////////////////////////////////////////////
///udisk class public interface
typedef void (*CBK_USBAPP)(void*buf,int sz);
extern void usb0dev_mod_enable(int en_dis);
extern void usb0_mod_ie_enable(int en_dis);
static
void usb_join_udisk(){
	init_local_flash_info();
	usb_init(USBDEV_BASE_PTR,usb0dev_mod_enable,usb0_mod_ie_enable);
	AplUsb_SetRxCbk(USB_ENDPID_BULK_OUT, (void*)msd_RxCbk);
	AplUsb_SetTxCbk(USB_ENDPID_BULK_IN,(void*)msd_TxCbk);
	memset(&msd_cmdResp,0,sizeof(msd_cmdResp));
}

static
void usb_disconn_udisk(){
	usb_deinit(USBDEV_BASE_PTR,usb0dev_mod_enable,usb0_mod_ie_enable);
	int i;
	for(i=0;i<MAX_LUN+1;i++){
		if(msd_wr_buf[i].buf)free(msd_wr_buf[i].buf);
		msd_wr_buf[i].buf=NULL;
		if(msd_rd_buf[i].buf)free(msd_rd_buf[i].buf);
		msd_rd_buf[i].buf=NULL;
	}
}

extern void*udisk_GetDeviceDesc(void);
extern void*udisk_GetConfigDesc(void);
extern void*udisk_GetStringDesc(int idx);
static int maxLun;
static CBufferBaseDesc s_msd_db;
static int is_msd_class_cmd(void*setup_pkt,int sz){
	unsigned char*ptr=setup_pkt;
	if(ptr[4]==USB_INTFID_BOT){
		if(ptr[1]==0xfe){
			return(1);
		}
	}
	return(0);
}
static void*get_msd_class_in_db(void*setup_pkt,int sz){
	unsigned char*ptr=setup_pkt;
	if(ptr[4]==USB_INTFID_BOT){
		if(ptr[1]==0xfe){
			maxLun=GetMaxLUN();
			s_msd_db.ptr=(void*)&maxLun;
			s_msd_db.sz=1;
			return(&s_msd_db);
		}
	}
	return(NULL);
}
const _t_drv_usb_device cDrvUdisk={
	USBDEV_BASE_PTR,
	usb_join_udisk,
	usb_disconn_udisk,

	udisk_GetDeviceDesc,
	udisk_GetConfigDesc,
	udisk_GetStringDesc,
	NULL,

	is_msd_class_cmd,
	get_msd_class_in_db,
	NULL,

	NULL,
	NULL,
	NULL
};

////
///////////////////////////////////////////////////////////////////////////////////////////

//EOF

