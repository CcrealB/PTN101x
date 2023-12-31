/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem module  R0.13                               /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2017, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:

/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/


#ifndef FF_DEFINED
#define FF_DEFINED	87030	/* Revision ID */

#ifdef __cplusplus
extern "C" {
#endif
#include "integer.h"
#include "ffconf.h"
#include "sys_config.h"

#if FF_DEFINED != FFCONF_DEF
#error Wrong configuration file (ffconf.h).
#endif


#if MCU_DBG_FILE_SYSTEM_ENABLE
#define FF_DEBUG(fmt,...)           do{if(mcu_dbg_ctrl & MCU_DBG_FILE_SYSTEM_ENABLE_MASK)MCU_DBG("FF","",fmt,##__VA_ARGS__);}while(0)
#else
#define FF_DEBUG(fmt,...)
#endif
#define FF_FATAL(fmt,...)           os_printf("[FF_FATAL] "fmt, ##__VA_ARGS__)


#define F_MUL_ASYNC_MAX 6
#define F_READ_TMP_BUF 2048 //2k

/* Definitions of volume management */

#if FF_MULTI_PARTITION		/* Multiple partition configuration */
typedef struct {
	BYTE pd;	/* Physical drive number */
	BYTE pt;	/* Partition: 0:Auto detect, 1-4:Forced partition) */
} PARTITION;
extern PARTITION VolToPart[];	/* Volume - Partition resolution table */
#endif



/* Type of path name strings on FatFs API */

#if FF_LFN_UNICODE && FF_USE_LFN	/* Unicode (UTF-16) string */

    #if FF_STRF_ENCODE != 3
        #ifndef _INC_TCHAR
        typedef WCHAR TCHAR;
        #define _T(x) L ## x
        #define _TEXT(x) L ## x
        #define _INC_TCHAR
        #endif
    #else //utf-8
    
        #ifndef _INC_TCHAR
        typedef char TCHAR;
        #define _T(x) x
        #define _TEXT(x) x
        #define _INC_TCHAR
        #endif
   #endif
#else						/* ANSI/OEM string */
    #ifndef _INC_TCHAR
    typedef char TCHAR;
    #define _T(x) x
    #define _TEXT(x) x
    #define _INC_TCHAR
    #endif

#endif



/* Type of file size variables */

#if FF_FS_EXFAT
#if !FF_USE_LFN
#error LFN must be enabled when enable exFAT
#endif
typedef QWORD FSIZE_t;
#else
typedef DWORD FSIZE_t;
#endif

typedef struct _fat_file_info_s
{
    BYTE filename[64];
    BYTE  ext_name[3];
    DWORD   file_start_cluster;
    DWORD  file_blks;
}fat_file_info_t;

/* Filesystem object structure (FATFS) */

typedef struct {
	BYTE	fs_type;		/* Filesystem type (0:N/A) */
	BYTE	pdrv;			/* Physical drive number */
	BYTE	n_fats;			/* Number of FATs (1 or 2) */
	BYTE	wflag;			/* win[] flag (b0:dirty) */
	BYTE	fsi_flag;		/* FSINFO flags (b7:disabled, b0:dirty) */
	WORD	id;				/* Volume mount ID */
	WORD	n_rootdir;		/* Number of root directory entries (FAT12/16) */
	WORD	csize;			/* Cluster size [sectors] */
#if FF_MAX_SS != FF_MIN_SS
	WORD	ssize;			/* Sector size (512, 1024, 2048 or 4096) */
#endif
#if FF_USE_LFN
	WCHAR*	lfnbuf;			/* LFN working buffer */
#endif
#if FF_FS_EXFAT
	BYTE*	dirbuf;			/* Directory entry block scratchpad buffer for exFAT */
#endif
#if FF_FS_REENTRANT
	FF_SYNC_t	sobj;		/* Identifier of sync object */
#endif
#if !FF_FS_READONLY
	DWORD	last_clst;		/* Last allocated cluster */
	DWORD	free_clst;		/* Number of free clusters */
#endif
#if FF_FS_RPATH
	DWORD	cdir;			/* Current directory start cluster (0:root) */
#if FF_FS_EXFAT
	DWORD	cdc_scl;		/* Containing directory start cluster (invalid when cdir is 0) */
	DWORD	cdc_size;		/* b31-b8:Size of containing directory, b7-b0: Chain status */
	DWORD	cdc_ofs;		/* Offset in the containing directory (invalid when cdir is 0) */
#endif
#endif
	DWORD	n_fatent;		/* Number of FAT entries (number of clusters + 2) */
	DWORD	fsize;			/* Size of an FAT [sectors] */
	DWORD	volbase;		/* Volume base sector */
	DWORD	fatbase;		/* FAT base sector */
	DWORD	dirbase;		/* Root directory base sector/cluster */
	DWORD	database;		/* Data base sector */
	DWORD	winsect;		/* Current sector appearing in the win[] */
	BYTE	win[FF_MAX_SS];	/* Disk access window for Directory, FAT (and file data at tiny cfg) */
} FATFS;



/* Object ID and allocation information (FFOBJID) */

typedef struct {
	FATFS*	fs;				/* Pointer to the hosting volume of this object */
	WORD	id;				/* Hosting volume mount ID */
	BYTE	attr;			/* Object attribute */
	BYTE	stat;			/* Object chain status (b1-0: =0:not contiguous, =2:contiguous, =3:flagmented in this session, b2:sub-directory stretched) */
	DWORD	sclust;			/* Object data start cluster (0:no cluster or root directory) */
	FSIZE_t	objsize;		/* Object size (valid when sclust != 0) */
#if FF_FS_EXFAT
	DWORD	n_cont;			/* Size of first fragment - 1 (valid when stat == 3) */
	DWORD	n_frag;			/* Size of last fragment needs to be written to FAT (valid when not zero) */
	DWORD	c_scl;			/* Containing directory start cluster (valid when sclust != 0) */
	DWORD	c_size;			/* b31-b8:Size of containing directory, b7-b0: Chain status (valid when c_scl != 0) */
	DWORD	c_ofs;			/* Offset in the containing directory (valid when file object and sclust != 0) */
#endif
#if FF_FS_LOCK
	UINT	lockid;			/* File lock ID origin from 1 (index of file semaphore table Files[]) */
#endif
} FFOBJID;



/* File object structure (FIL) */

typedef struct {
	FFOBJID	obj;			/* Object identifier (must be the 1st member to detect invalid object pointer) */
	BYTE	flag;			/* File status flags */
	BYTE	err;			/* Abort flag (error code) */
	FSIZE_t	fptr;			/* File read/write pointer (Zeroed on file open) */
	DWORD	clust;			/* Current cluster of fpter (invalid when fptr is 0) */
	DWORD	sect;			/* Sector number appearing in buf[] (0:invalid) */
#if !FF_FS_READONLY
	DWORD	dir_sect;		/* Sector number containing the directory entry (not used at exFAT) */
	BYTE*	dir_ptr;		/* Pointer to the directory entry in the win[] (not used at exFAT) */
#endif
#if FF_USE_FASTSEEK
	DWORD*	cltbl;			/* Pointer to the cluster link map table (nulled on open, set by application) */
#endif
#if !FF_FS_TINY
	BYTE	buf[FF_MAX_SS];	/* File private data read/write window */
#endif
} FIL;



/* Directory object structure (DIR) */

typedef struct {
	FFOBJID	obj;			/* Object identifier */
	DWORD	dptr;			/* Current read/write offset */
	DWORD	clust;			/* Current cluster */
	DWORD	sect;			/* Current sector (0:Read operation has terminated) */
	BYTE*	dir;			/* Pointer to the directory item in the win[] */
	BYTE	fn[12];			/* SFN (in/out) {body[8],ext[3],status[1]} */
#if FF_USE_LFN
	DWORD	blk_ofs;		/* Offset of current entry block being processed (0xFFFFFFFF:Invalid) */
#endif
#if FF_USE_FIND
	const TCHAR* pat;		/* Pointer to the name matching pattern */
#endif
} DIR;



/* File information structure (FILINFO) */

typedef struct {
	FSIZE_t	fsize;			/* File size */
	WORD	fdate;			/* Modified date */
	WORD	ftime;			/* Modified time */
	BYTE	fattrib;		/* File attribute */
#if FF_USE_LFN
	TCHAR	altname[13];			/* Altenative file name */
	TCHAR	fname[FF_MAX_LFN + 1];	/* Primary file name */
#else
	TCHAR	fname[13];		/* File name */
#endif

#if FF_BEKEN_FILESCAN
	BYTE	ExNoChainFlag;
	UINT	fcluster;
#endif
} FILINFO,FILINFOADD,*PFILINFOADD;


#if FF_BEKEN_FILESCAN

typedef struct
{
	UINT	fcluster;	//文件的簇号
	char	fname[64];	/* Short file name (28.3 format) */
	char extname[3];
	char fat_ok_flag;
} FILE_INFO;

/*app struct*/
typedef struct
{
	UINT		first_cluster;//当前目录第一个簇的簇号
	WORD 		music_total;//当前目录下音乐文件的数量
	BYTE		broot_dir;
	BYTE		ExNoChainFlag;
}FAT_DIR_INFO;

typedef struct _DIR_QUEUE
{
	UINT		cluster_number;
	WORD 		dirlevel;
	BYTE 		broot_dir;
	BYTE		ExNoChainFlag;
	struct 		_DIR_QUEUE* next;
}DIR_QUEUE;

#endif

/* File function return code (FRESULT) */

typedef enum {
	FR_BUSY=-1,
	FR_OK = 0,				/* (0) Succeeded */
	FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,				/* (2) Assertion failed */
	FR_NOT_READY,			/* (3) The physical drive cannot work */
	FR_NO_FILE,				/* (4) Could not find the file */
	FR_NO_PATH,				/* (5) Could not find the path */
	FR_INVALID_NAME,		/* (6) The path name format is invalid */
	FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	FR_EXIST,				/* (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/* (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any problem */
	FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > FF_FS_LOCK */
	FR_INVALID_PARAMETER,	/* (19) Given parameter is invalid */
	FR_FILE_END,			/* (20) File end */
	FR_FILE_BEGIN			/* (21) File Begin*/
} FRESULT;
typedef enum 
{
	ASYNC_BUSY = -1,
	ASYNC_OK = 0,
	ASYNC_PART_OK,	//部分ok
	ASYNC_FAIL,
}ASYNC_STATE;

typedef struct
{
	BYTE		get_fat;
	DWORD 		get_fat_clst;
	BYTE		disk_read1;
	FRESULT     disk_read1_res;
	BYTE		disk_read2;
	FRESULT		disk_read2_res;
	BYTE		disk_write;
}FREAD_ASYNC_FLG_INT_S;
typedef struct
{
	BYTE flg;
    BYTE try_cnt;  
}FREAD_ASYNC_FLG_ABNORMAL_S;

typedef struct
{
    FREAD_ASYNC_FLG_INT_S interrupt;
    FREAD_ASYNC_FLG_ABNORMAL_S abnormal;
    BYTE init;
	BYTE running;    
}FREAD_ASYNC_FLG_S;
typedef struct
{
    FFOBJID obj;
    BYTE    flag;           /* File status flags */
    BYTE    err;            /* Abort flag (error code) */
    FSIZE_t fptr;           /* File read/write pointer (Zeroed on file open) */
    DWORD   clust;          /* Current cluster of fpter (invalid when fptr is 0) */
    DWORD   sect;           /* Sector number appearing in buf[] (0:invalid) */
    DWORD   dir_sect;       /* Sector number containing the directory entry (not used at exFAT) */
    BYTE*   dir_ptr;        /* Pointer to the directory entry in the win[] (not used at exFAT) */
}FREAD_ASYNC_PARA_FP_S;
typedef struct 
{
	FIL *fp; 	/* Pointer to the file object */
	void *buff;	/* Pointer to data buffer */
	UINT btr;	/* Number of bytes to read */
	UINT btr_back;	/* Number of bytes to read */
	UINT *br;	/* Pointer to number of bytes read */
	void *cbk;
	void *cbk_arg;
	FREAD_ASYNC_PARA_FP_S fp_back;
}FREAD_ASYNC_PARA_S;

typedef struct
{
	BYTE		flg_clst1;
	BYTE 		flg_clst2;
	DWORD		clst;
	BYTE     	flg_disk_write1;
	BYTE		flg_disk_write2;
	BYTE		flg_disk_read;
}FWRITE_ASYNC_FLG_INT_S;

typedef struct
{
    FWRITE_ASYNC_FLG_INT_S interrupt;
    BYTE init;
	BYTE running;    
}FWRITE_ASYNC_FLG_S;

typedef struct 
{
	FIL *fp; 	/* Pointer to the file object */
	void *buff;	/* Pointer to data buffer */
	UINT btw;	/* Number of bytes to read */
	UINT *bw;	/* Pointer to number of bytes read */
	void *cbk;
	void *cbk_arg;
	BYTE run_flg;
}FWRITE_ASYNC_PARA_S;

typedef struct
{
	BYTE		flg_open_init;
	BYTE     	flg_open_find_volume;
	FRESULT		res_open_find_volume;
	BYTE		flg_open_follow_path;
	FRESULT		res_open_follow_path;
	
	BYTE		flg_follow_path_init;
	BYTE		flg_follow_path1;
	BYTE		flg_follow_path_dir_find;
	FRESULT		res_follow_path_dir_find;
	
	BYTE		flg_dir_find_init;
	BYTE		flg_dir_find_1;
	BYTE		flg_dir_find_read_dir;
	FRESULT		res_dir_find_read_dir;
	
	BYTE		flg_dir_read_init;	
	BYTE		flg_read_dir_movement;
	FRESULT		res_read_dir_movement;
	
	BYTE		flg_movement_sync_window;
	BYTE		flg_movement_disk_read;
	FRESULT		res_movement_disk_read;

	BYTE        init;
	BYTE        running;
}FOPEN_ASYNC_FLG_S;

typedef struct 
{
	FIL *fp; 	/* Pointer to the file object */
	const TCHAR *path;	/* Pointer to data buffer */
	BYTE mode;
	void *cbk;
	void *cbk_arg;
}FOPEN_ASYNC_PARA_S;

typedef struct
{
    FREAD_ASYNC_PARA_S para[F_MUL_ASYNC_MAX];
    FREAD_ASYNC_FLG_S flg;
    BYTE index_in;
    BYTE index_out;
    UINT br_cnt; 
}FREAD_ASYNC_S;

typedef struct
{
    FWRITE_ASYNC_PARA_S para[F_MUL_ASYNC_MAX];
    FWRITE_ASYNC_FLG_S flg;
    BYTE index_in;
    BYTE index_out;
    UINT bw_cnt;
}FWRITE_ASYNC_S;
typedef struct
{
    FOPEN_ASYNC_PARA_S para[F_MUL_ASYNC_MAX];
    FOPEN_ASYNC_FLG_S flg;
    BYTE index_in;
    BYTE index_out;
    DIR *dj;
    const TCHAR* path;
    UINT vol;
}FOPEN_ASYNC_S;

typedef void (*CBK_FUN1)(void);
typedef void (*CBK_FUN2)(void *para);
typedef FRESULT (*CBK_FUN3)(void);
typedef void (*CBK_FUN4)(void *buf, UINT sz);
typedef void (*CBK_FUN5)(UINT arg1, UINT arg2, void *obj);


typedef enum 
{
    STATE_OP_IDLE = 0,
    STATE_OP_OK,
    STATE_OP_BUSY,
}STATE_OP;

/*--------------------------------------------------------------*/
/* FatFs module application interface                           */

FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode);				/* Open or create a file */
FRESULT f_close (FIL* fp);											/* Close an open file object */
FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br);			/* Read data from the file */
FRESULT f_wave_read (FIL* fp, void* buff, UINT btr, UINT* br);			/* Read data from the file */
FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw);	/* Write data to the file */
FRESULT f_lseek (FIL* fp, FSIZE_t ofs);								/* Move file pointer of the file object */
FRESULT f_truncate (FIL* fp);										/* Truncate the file */
FRESULT f_sync (FIL* fp);											/* Flush cached data of the writing file */
FRESULT f_opendir (DIR* dp, const TCHAR* path);						/* Open a directory */
FRESULT f_closedir (DIR* dp);										/* Close an open directory */
FRESULT f_readdir (DIR* dp, FILINFO* fno);							/* Read a directory item */
FRESULT f_findfirst (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);	/* Find first file */
FRESULT f_findnext (DIR* dp, FILINFO* fno);							/* Find next file */
FRESULT f_mkdir (const TCHAR* path);								/* Create a sub directory */
FRESULT f_unlink (const TCHAR* path);								/* Delete an existing file or directory */
FRESULT f_rename (const TCHAR* path_old, const TCHAR* path_new);	/* Rename/Move a file or directory */
FRESULT f_stat (const TCHAR* path, FILINFO* fno);					/* Get file status */
FRESULT f_chmod (const TCHAR* path, BYTE attr, BYTE mask);			/* Change attribute of a file/dir */
FRESULT f_utime (const TCHAR* path, const FILINFO* fno);			/* Change timestamp of a file/dir */
FRESULT f_chdir (const TCHAR* path);								/* Change current directory */
FRESULT f_chdrive (const TCHAR* path);								/* Change current drive */
FRESULT f_getcwd (TCHAR* buff, UINT len);							/* Get current directory */
FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs);	/* Get number of free clusters on the drive */
FRESULT f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn);	/* Get volume label */
FRESULT f_setlabel (const TCHAR* label);							/* Set volume label */
FRESULT f_forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf);	/* Forward data to the stream */
FRESULT f_expand (FIL* fp, FSIZE_t szf, BYTE opt);					/* Allocate a contiguous block to the file */
FRESULT f_mount (FATFS* fs, const TCHAR* path, BYTE opt);			/* Mount/Unmount a logical drive */
FRESULT f_unmount (FATFS* fs,	const TCHAR* path,BYTE opt);
FRESULT f_mkfs (const TCHAR* path, BYTE opt, DWORD au, void* work, UINT len);	/* Create a FAT volume */
FRESULT f_fdisk (BYTE pdrv, const DWORD* szt, void* work);			/* Divide a physical drive into some partitions */
FRESULT f_setcp (WORD cp);											/* Set current code page */
int f_putc (TCHAR c, FIL* fp);										/* Put a character to the file */
int f_puts (const TCHAR* str, FIL* cp);								/* Put a string to the file */
int f_printf (FIL* fp, const TCHAR* str, ...);						/* Put a formatted string to the file */
TCHAR* f_gets (TCHAR* buff, int len, FIL* fp);						/* Get a string from the file */

#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj.objsize))
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj.objsize)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)
//#define f_unmount(path) f_mount(0, path, 0)
FRESULT f_SeekfromCurPos( FIL *fp ,BYTE ff_fb,DWORD bytes);
FRESULT f_EOF(FIL *fp );
#ifndef EOF
#define EOF (-1)
#endif




/*--------------------------------------------------------------*/
/* Additional user defined functions                            */

/* RTC function */
#if !FF_FS_READONLY && !FF_FS_NORTC
DWORD get_fattime (void);
#endif

/* LFN support functions */
#if FF_USE_LFN						/* Code conversion (defined in unicode.c) */
WCHAR ff_oem2uni (WCHAR oem, WORD cp);	/* OEM code to Unicode conversion */
WCHAR ff_uni2oem (WCHAR uni, WORD cp);	/* Unicode to OEM code conversion */
WCHAR ff_wtoupper (WCHAR uni);			/* Unicode upper-case conversion */
#endif
#if FF_USE_LFN == 3						/* Dynamic memory allocation */
void* ff_memalloc (UINT msize);			/* Allocate memory block */
void ff_memfree (void* mblock);			/* Free memory block */
#endif
unsigned long long ff_unicode2utf8(unsigned long long unicode);
unsigned long long ff_utf82unicode(unsigned long long utf8);

/* Sync functions */
#if FF_FS_REENTRANT
int ff_cre_syncobj (BYTE vol, FF_SYNC_t* sobj);	/* Create a sync object */
int ff_req_grant (FF_SYNC_t sobj);		/* Lock sync object */
void ff_rel_grant (FF_SYNC_t sobj);		/* Unlock sync object */
int ff_del_syncobj (FF_SYNC_t sobj);	/* Delete a sync object */
#endif


FRESULT Beken_dir_sdi(DIR* dp,DWORD ofs);
FRESULT Beken_dir_read(DIR* dp,int vol);
FRESULT Beken_dir_next(DIR* dp,int stretch);
FRESULT init_buf(FATFS *fs);
void uninit_buf(FATFS *fs);
void Beken_get_fileinfo(DIR* dp,FILINFO* fno);
void beken_mem_set(void* dst, int val, UINT cnt);
void beken_mem_cpy (void* dst, const void* src, UINT cnt);

FRESULT f_read_async_config_cbk(
	FIL* fp, 	/* Pointer to the file object */
	void* buff,	/* Pointer to data buffer */
	UINT *btr,	/* Number of bytes to read */
	UINT* br,	/* Pointer to number of bytes read */
	void*cbk
);

void f_async_timming_call(void);
FRESULT f_open_async_init (
	FIL* fp,			/* Pointer to the blank file object */
	const TCHAR* path,	/* Pointer to the file name */
	BYTE mode,			/* Access mode and file open mode flags */
	void *cbk,
	void *cbk_arg
);

FRESULT f_read_async_init (
	FIL * fp, 	/* Pointer to the file object */
	void * buff,	/* Pointer to data buffer */
	UINT btr,	/* Number of bytes to read */
	UINT * br,	/* Pointer to number of bytes read */
	void *cbk,
	void *cbk_arg
);
FRESULT f_write_async_init (
	FIL * fp, 	/* Pointer to the file object */
	void * buff,	/* Pointer to data buffer */
	UINT btw,	/* Number of bytes to read */
	UINT * bw,	/* Pointer to number of bytes read */
	void *cbk,
	void *cbk_arg
);
FRESULT f_wait_async_idle(const char *func_name);
ASYNC_STATE f_get_async_state(void);
void f_set_async_state(ASYNC_STATE state);
void f_async_para_init(void);
int get_fread_pending(void);
int get_fwrite_pending(void);
int ff_debug(const char *fmt, ...);

/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File access mode and open method flags (3rd argument of f_open) */
#define	FA_READ				0x01
#define	FA_WRITE			0x02
#define	FA_OPEN_EXISTING	0x00
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define	FA_OPEN_APPEND		0x30

/* Fast seek controls (2nd argument of f_lseek) */
#define CREATE_LINKMAP	((FSIZE_t)0 - 1)

/* Format options (2nd argument of f_mkfs) */
#define FM_FAT		0x01
#define FM_FAT32	0x02
#define FM_EXFAT	0x04
#define FM_ANY		0x07
#define FM_SFD		0x08

/* Filesystem type (FATFS.fs_type) */
#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3
#define FS_EXFAT	4

/* File attribute bits for directory entry (FILINFO.fattrib) */
#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */


#define FF_CHECK_DONE(str) {\
			if(gs_async_state == ASYNC_OK)\
			{\
				FF_FATAL(str);\
				os_printf(" done\r\n\r\n");\
			}\
			}
#ifdef __cplusplus
}
#endif

#endif /* FF_DEFINED */
