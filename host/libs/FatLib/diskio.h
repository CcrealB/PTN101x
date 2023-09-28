/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file 
/-----------------------------------------------------------------------*/
#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED
#ifdef __cplusplus
extern "C" {
#endif
#include "ff.h"
#include "driver_beken_includes.h"
#include "app_debug.h"

#if DRV_DEBUG_DISKIO_ENABLE
#define DISK_DEBUG(fmt,...)    do{if(mcu_dbg_drv_ctrl & MCU_DBG_DRV_DISKIO_MASK)DEBUG_DRV_PRINTF("_DISK",fmt,##__VA_ARGS__);}while(0)
#else
#define DISK_DEBUG(fmt,...)
#endif

#define DISK_FATAL(fmt,...)    os_printf("[DISK_FATAL] "fmt, ##__VA_ARGS__)



#define DISK_MUL_ASYNC_MAX F_MUL_ASYNC_MAX

typedef enum _DISK_TYPE_e{
	DISK_TYPE_SD = 0,
	DISK_TYPE_UDISK = 1,
}DISK_TYPE_e;

typedef struct
{
	BYTE	read1;
}DREAD_ASYNC_FLG_INT_S;
typedef struct
{
	uint8 flg;
    uint8 try_cnt;  
}DREAD_ASYNC_FLG_ABNORMAL_S;

typedef struct
{
	DREAD_ASYNC_FLG_INT_S intterupt;
	DREAD_ASYNC_FLG_ABNORMAL_S abnormal;
	BYTE init;
	volatile BYTE running;
}DREAD_ASYNC_FLG_S;

typedef struct 
{
    BYTE pdrv;  /* Physical drive nmuber to identify the drive */
	void *buff;	/* Pointer to data buffer */
	UINT sect;	/* Number of bytes to read */
	UINT cc;	/* Number of bytes to read */
	void *cbk;
	void *cbk_arg;
}DREAD_ASYNC_PARA_S;

typedef struct
{
	BYTE write1;
}DWRITE_ASYNC_FLG_INT_S;
typedef struct
{
	uint8 flg;
    uint8 try_cnt;  
}DWRITE_ASYNC_FLG_ABNORMAL_S;

typedef struct
{
	DWRITE_ASYNC_FLG_INT_S intterupt;
	DWRITE_ASYNC_FLG_ABNORMAL_S abnormal;
	BYTE init;
	BYTE running;
}DWRITE_ASYNC_FLG_S;
typedef struct 
{
    BYTE pdrv;  /* Physical drive nmuber to identify the drive */
	void *buff;	/* Pointer to data buffer */
	UINT sect;	/* Number of bytes to read */
	UINT cc;	/* Pointer to number of bytes read */
	void *cbk;
	void *cbk_arg;
}DWRITE_ASYNC_PARA_S;


typedef struct
{
    DREAD_ASYNC_PARA_S para[F_MUL_ASYNC_MAX];
    DREAD_ASYNC_FLG_S flg;
    uint8 index_in;
    uint8 index_out;
}DREAD_ASYNC_S;

typedef struct
{
    DWRITE_ASYNC_PARA_S para[F_MUL_ASYNC_MAX];
    DWRITE_ASYNC_FLG_S flg;
    uint8 index_in;
    uint8 index_out;
}DWRITE_ASYNC_S;



typedef uint8_t	 DSTATUS;

/* Results of Disk Functions */
typedef enum {
	RES_BUSY=-1,
	RES_OK = 0,		/* 0: Successful */
	RES_ERROR,		/* 1: R/W Error */
	RES_WRPRT,		/* 2: Write Protected */
	RES_NOTRDY,		/* 3: Not Ready */
	RES_PARERR,		/* 4: Invalid Parameter */
	RES_NOMEDIA		/* 5: Media is not present*/
} DRESULT;
/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

#if 1//(CONFIG_APP_MP3PLAYER == 1)

/*---------------------------------------*/
/* Prototypes for disk control functions */


DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_write (BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT disk_unmount(uint8 pdrv);
//DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);
// DSTATUS disk_uninitialize ( BYTE pdrv);
uint8 Media_is_online(void);
uint8 get_cur_media_type(void);
DRESULT disk_read_async (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	uint8* buff,
	DWORD sector,
	UINT count,
	void*cbk
);
DRESULT disk_write_async (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	uint8* buff,
	DWORD sector,
	UINT count,
	void*cbk
);
FRESULT disk_async_read_init (
    BYTE pdrv,
    void * buff,    /* Pointer to data buffer */
    UINT sect,  /* Number of bytes to read */
    UINT cc,    /* Pointer to number of bytes read */
    void *cbk,
    void *cbk_arg
);
FRESULT disk_async_write_init (
    BYTE pdrv,
    void * buff,    /* Pointer to data buffer */
    UINT sect,  /* Number of bytes to read */
    UINT cc,    /* Pointer to number of bytes read */
    void *cbk,
    void *cbk_arg
);
void disk_async_timming_call(void);
void disk_async_para_init(void);
int get_read_pending(void);
int get_write_pending(void);


/* Command code for disk_ioctrl fucntion */

/* Generic command (Used by FatFs) */
#define CTRL_SYNC			0	/* Complete pending write process (needed at _FS_READONLY == 0) */
#define GET_SECTOR_COUNT	1	/* Get media size (needed at _USE_MKFS == 1) */
#define GET_SECTOR_SIZE		2	/* Get sector size (needed at _MAX_SS != _MIN_SS) */
#define GET_BLOCK_SIZE		3	/* Get erase block size (needed at _USE_MKFS == 1) */
#define CTRL_TRIM			4	/* Inform device that the data on the block of sectors is no longer used (needed at _USE_TRIM == 1) */

/* Generic command (Not used by FatFs) */
#define CTRL_POWER			5	/* Get/Set power status */
#define CTRL_LOCK			6	/* Lock/Unlock media removal */
#define CTRL_EJECT			7	/* Eject media */
#define CTRL_FORMAT			8	/* Create physical format on the media */

/* MMC/SDC specific ioctl command */
#define MMC_GET_TYPE		10	/* Get card type */
#define MMC_GET_CSD			11	/* Get CSD */
#define MMC_GET_CID			12	/* Get CID */
#define MMC_GET_OCR			13	/* Get OCR */
#define MMC_GET_SDSTAT		14	/* Get SD status */
#define ISDIO_READ			55	/* Read data form SD iSDIO register */
#define ISDIO_WRITE			56	/* Write data to SD iSDIO register */
#define ISDIO_MRITE			57	/* Masked write data to SD iSDIO register */

/* ATA/CF specific ioctl command */
#define ATA_GET_REV			20	/* Get F/W revision */
#define ATA_GET_MODEL		21	/* Get model name */
#define ATA_GET_SN			22	/* Get serial number */

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_APP_MP3PLAYER */
#endif
