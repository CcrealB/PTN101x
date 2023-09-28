#include "includes.h"
#include <config/config.h>
#include "ff.h"
#include "diskio.h"     /* Declarations of low level disk I/O functions */
#include "playmode.h"
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_APP_RECORDER
#include "rec_include.h"
#endif
#if 1//(CONFIG_APP_MP3PLAYER == 1)

/* ----------------------------------------------------------- */
#define PLYMOD_LOG_I(fmt,...)   os_printf(fmt,##__VA_ARGS__)
#define PLYMOD_LOG_D(fmt,...)   //os_printf(fmt,##__VA_ARGS__)

/* ----------------------------------------------------------- */
#define MAX_DIR_SUPPORT 1024
#define MAX_SONG_DIR    64       // 128
#define MAX_DIR_LEVEL   6

extern FATFS *Fatfs_buf ;


static FILE_INFO FileInfo;

static FAT_DIR_INFO* file_queue = NULL;
static FIL *fsFD = NULL;
static uint16_t mp3queuecount = 0; //num of dir that have music file
static uint16_t mp3filecount = 0;//total num of music file
static DIR_QUEUE *point_front;//queue header pointer
static DIR_QUEUE *point_rear;//queue tail pointer
static uint16_t rear_cnt = 0;

const MYSTRING tblAudio_file_exts[] = {".wav", ".mp3"};


//extern void mem_set (void* dst, int val, uint32_t cnt);
//extern FRESULT chk_mounted_con (FATFS *rfs, uint8_t chk_wp);


/** @brief refer to strnicmp */
int ptn_strnicmp(const void* dst, const void* src, int len)
{
    if(len == 0) return 0;

    const char *d = (const char *)dst, *s = (const char *)src;
    while(len--)
    {
        int diff = *s++ - *d++;
        if((diff == 0) || (diff == 'a' - 'A') || (diff == 'A' - 'a')) {
            continue;
        }else{
            return diff;
        }
    }
    return 0;
}

/** @brief refer to strrchr */
char* ptn_strrchr(char * str, char c)
{
    char *p = str;
    while (*p) { p++; }
    while ((p-- != str) && (*p != c));
    return ((*p == c) ? p : NULL);
}

void strn2upper(char *str, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if((str[i] >= 'a') && (str[i] <= 'z')) { str[i] -= ('a' - 'A'); }
    }
}

/** fn_ext: output ext name without case conversion.
 * @return file ext name in "fn_ext_out" without case conversion, '.' is excluded.
* @note sizeof(fn_ext_out) >= 8 is suggested.
*/
int get_file_ext_name(const char *fn, char* fn_ext_out)
{
    int ret = 0;
    int fn_len = strlen(fn);
    if (fn_len < 3) {ret = -1; goto RET; }
//// find ext name address
    char *fn_ext = strrchr(fn, '.'); //find ext name address
    fn_ext += 1;//exclude '.'
    if (fn_ext == NULL) {ret = -2; goto RET; }
//// check ext name address
    // char fn_ext_out[256] = {0};//'\0' is inlcuded
    int fn_ext_len = strlen(fn_ext);
    if(fn_ext_len >= 255) { ret = -3; goto RET; }
//// convert ext name to up case
    memcpy(fn_ext_out, fn_ext, fn_ext_len);
    // strn2lower(fn_ext_out, fn_ext_len);
RET:
    return ret;
}

//check if the file is audio type
int is_audio_file(char *fn)
{
    int i;
    char *p;
    int fn_len = strlen(fn);
    if (fn_len < 5) { return (F_TYP_NOT_SUPPORTED); }
//// find ext name address
    char *fn_ext = ptn_strrchr(fn, '.');
    if (fn_ext == NULL) { return (F_TYP_NOT_SUPPORTED); }

    int fn_ext_len = strlen(fn_ext); //'.' and '\0' is inlcuded
    if(fn_ext_len <= 2) { return (F_TYP_NOT_SUPPORTED); }
//// compare
    for (i = 0; i < sizeof(tblAudio_file_exts) / sizeof(tblAudio_file_exts[0]); i++)
    {
        p = (char *)tblAudio_file_exts[i];
        if((fn_ext_len == strlen(p)) && (ptn_strnicmp(fn_ext, p, fn_ext_len) == 0))
        {
            return (i + F_TYP_MIN_SUPPORTED);
        }
    }
    return (F_TYP_NOT_SUPPORTED);
}

FRESULT f_open_con (
    FIL *fp,            /* Pointer to the blank file object */
    PFILINFOADD finfo,
    BYTE mode           /* Access mode and file open mode flags */
)
{
    fp->obj.sclust = finfo->fcluster;/* File start cluster */
    fp->obj.objsize = finfo->fsize; /* File size */
    fp->obj.fs = Fatfs_buf;
    fp->obj.id = Fatfs_buf->id; /* Owner file system object of the file */
    fp->flag = mode;                /* File access mode */
    fp->err = 0;            /* Clear error flag */
    fp->sect = 0;           /* Invalidate current data sector */
    fp->fptr = 0;           /* Set file pointer top of the file */
    if(fp->obj.fs->fs_type == FS_EXFAT)
    {    
#if FF_FS_EXFAT
        fp->obj.n_frag = 0;
#endif
        fp->obj.stat = finfo->ExNoChainFlag;
    }
    return FR_OK;
}

/*input:
i -- directory index
number -- music file idx in  directory i */
static FIL  *GetFile_From_NumInDir(WORD  i, WORD number)
{
    WORD  n = 0;
    WORD  brootdir = 0;
    int music_count = 0;
    FRESULT res;
    FILINFOADD fno;
    DIR dj_con;
    FIL *fio = fsFD;
    FAT_DIR_INFO* file = file_queue + i;
    //int ft;
    dj_con.obj.fs = Fatfs_buf;
    brootdir = file->broot_dir;
    res=init_buf(Fatfs_buf);
    if(res != FR_OK)
    {
        return NULL;
    }
    if(brootdir)
    {
        dj_con.obj.sclust = 0;
    }
    else
    {
        dj_con.obj.sclust = file->first_cluster;
    }
    music_count = 0;
    number += 1;
    os_printf("dir_sdi\r\n");
    res = Beken_dir_sdi(&dj_con, 0);
    if (res != FR_OK)
    {
        fio =NULL;
        goto ret_free;
    }
    while(res == FR_OK)
    {
        res = Beken_dir_read(&dj_con, 0);
        if (res != FR_OK)
        {
            break;
        }
        /* Get the directory name and push it to the buffer */
        Beken_get_fileinfo(&dj_con, &fno);
        if ((fno.fattrib & AM_ARC) != 0) /* It is a Archive */
        {
            // os_printf("file name=%s\r\n",fno.fname);
            for (n = 0; fno.fname[n]; n++);
            if((n > 4)&&(n <= FF_MAX_LFN))
            {	
                if(is_audio_file(fno.fname) >= F_TYP_MIN_SUPPORTED){
                    music_count++;
                }
            }
        }
        if (music_count == number)
        {
            // os_printf("set file name info...\r\n");
            beken_mem_cpy(FileInfo.fname,fno.fname,63);
            // os_printf("set file ext info...\r\n");
            beken_mem_cpy(FileInfo.extname,fno.fname+n-3,3);
            os_printf("file found:%s\r\n",fno.fname);
            break;
        }
        else
        {
            // os_printf("dir_next\r\n");
            res = Beken_dir_next(&dj_con, 0);
        }
        // os_printf("Next Loop\r\n");
    }
    // os_printf("target file is found \r\n");
    FileInfo.fcluster = fno.fcluster;
    // os_printf("f_open_con\r\n");
    os_printf("fio:%p \r\n",fio);
    f_open_con(fio, &fno, FA_READ);
    // os_printf("End f_open_con\r\n");
ret_free:
    uninit_buf(Fatfs_buf);
    return fio;
}


static bool GetFileName_From_NumInDir(WORD  i, WORD number,char *name)
{
    WORD  n = 0,brootdir = 0,music_count = 0;
    FRESULT res;
    FILINFOADD fno;
    DIR dj_con;
    //FIL *fio = fsFD;
    FAT_DIR_INFO* file = file_queue + i;
    bool find = false;
    //int ft;
    dj_con.obj.fs = Fatfs_buf;
    brootdir = file->broot_dir;
    res=init_buf(Fatfs_buf);
    if(res != FR_OK)
    {
        return NULL;
    }
    if(brootdir)
    {
        dj_con.obj.sclust = 0;
    }
    else
    {
        dj_con.obj.sclust = file->first_cluster;
    }
    music_count = 0;
    number += 1;
    PLYMOD_LOG_D("dir_sdi\r\n");
    res = Beken_dir_sdi(&dj_con, 0);
    if (res != FR_OK)
    {
        //fio =NULL;
        goto ret_free;
    }
    while(res == FR_OK)
    {
        res = Beken_dir_read(&dj_con, 0);
        if (res != FR_OK)
        {
            break;
        }
        /* Get the directory name and push it to the buffer */
        Beken_get_fileinfo(&dj_con, &fno);
        if ((fno.fattrib & AM_ARC) != 0) /* It is a Archive */
        {
            // os_printf("file name=%s\r\n",fno.fname);
            for (n = 0; fno.fname[n]; n++);
            if((n > 4)&&(n <= FF_MAX_LFN))
            {	
                if(is_audio_file(fno.fname) >= F_TYP_MIN_SUPPORTED){
                    music_count++;
                }
            }
        }
        if (music_count == number)
        {
            // os_printf("set file name info...\r\n");
            beken_mem_cpy(name,fno.fname,63);
            PLYMOD_LOG_D("file found:%s\r\n",name);
            find = true;
            break;
        }
        else
        {
            // os_printf("dir_next\r\n");
            res = Beken_dir_next(&dj_con, 0);
        }
    }
ret_free:
    uninit_buf(Fatfs_buf);
    return find;
}


/*
dir_num:audio dir index, 0 ~ (mp3queuecount - 1)
song_idx: audio file index, 0 ~ (file->music_total -)
*/
void *Get_Cur_Dir_file(uint16_t dir_num, uint16_t song_idx)
{
    FIL *fhFile;
    FAT_DIR_INFO* file = NULL;
    if(dir_num >= mp3queuecount)
    {
        dir_num = 0;
    }
    file = file_queue + dir_num;
    if(song_idx > file->music_total)
    {
        song_idx = 0;
    }
    PLYMOD_LOG_I("total_dir:%d, cur_dir:%d, cur_dir_song_num:%d, cur_dir_song_index:%d\r\n",
        mp3queuecount, dir_num, file->music_total, song_idx);
    fhFile = GetFile_From_NumInDir(dir_num, song_idx);
    return fhFile;
}

//get file pointer according file index
/*input:number -- 0~mp3filecount-1 */
//Playlist_GetSongFileInfo
void*Get_File_From_Number(int number)
{
    //fast find file way without disk read operation
    WORD i;
    FIL *fhFile;
    FAT_DIR_INFO* file = NULL;
    for(i=0; i<MAX_SONG_DIR; i++)
    {
        file = file_queue + i;
        if(number >= file->music_total) //if file index > music file num in current dir
        {
            number = number - file->music_total;
            continue;
        }
        else
        {
            break;    //target file find
        }
    }
    if(i == MAX_SONG_DIR)
    {
        return NULL;
    }
    fhFile = GetFile_From_NumInDir(i, number);
    return fhFile;
}

/**
 * @brief get golbal file index by dir index and file index
 * @param FileIdx index of cur dir
 * @return global_file_idx : 0 ~ (get_musicfile_count() - 1), -1:Fail
 **/
int Get_gFileIndex_By_DirFileIdx(uint16_t DirIdx, uint16_t FileIdx)
{
    int i;
    int global_file_idx = 0;
    int total_aud_dir_num = get_musicdir_num();
    int total_aud_file_num = get_musicfile_count();
    FAT_DIR_INFO *dir_info = NULL;

////assert
    if(((DirIdx + 1) > total_aud_dir_num) || (global_file_idx > total_aud_file_num))
    {
        os_printf("#ERROR: DirIdx:%d[total:%d], FileIdx:%d[total:%d]\n",
                    DirIdx, total_aud_dir_num, FileIdx, total_aud_file_num);
        return -1;
    }
////acc dir from 0~(DirIdx-1)
    for (i = 0; i < DirIdx; i++)
    {
        dir_info = file_queue + i;
        global_file_idx += dir_info->music_total;
    }

////add dir DirIdx
    dir_info = file_queue + i;

////assert
    if ((FileIdx + 1) > dir_info->music_total)
    {
        os_printf("#ERROR: FileIdx not exit!\n");
        return -1;
    }

    global_file_idx += FileIdx;

    return global_file_idx;
}


/**
 * @brief get dir index and file index by golbal file number
 * @param gFileIdx : 0 ~ (get_musicfile_count() - 1)
 * @return 0:OK, -1:Fail
 **/
int Get_DirFileIdx_By_gFileIndex(int gFileIdx, uint16_t *pDirIdx, uint16_t *pFileIdx)
{
    int DirIdx;
    int FileIdx = gFileIdx;
    int dir_num = mp3queuecount;//music_dir_num_get();
    FAT_DIR_INFO *dir_info = NULL;

    if(FileIdx < 0) return -1;// PRJ_IS_GCX_RENEGADE and index error

    for (DirIdx = 0; DirIdx < dir_num; DirIdx++)
    {
        dir_info = file_queue + DirIdx;
        if (FileIdx >= dir_info->music_total) // if greater than music num of cur dir, subtract num of cur dir, and check next dir
        {
            FileIdx = FileIdx - dir_info->music_total;
            continue;
        }
        else
        {
            break; //the dir of the file found
        }
    }
    if (DirIdx == dir_num)
    {
        return -1;
    }
    *pDirIdx = DirIdx;
    *pFileIdx = FileIdx;

    return 0;
}

#if 0
bool Get_File_Dir_Index_From_Number(int number, int *dir, int *index)
{    
    char name[68];
    bool ret = true;
    WORD i;
    //FIL *fhFile;
    FAT_DIR_INFO* file = NULL;
    for(i=0; i<MAX_SONG_DIR; i++)
    {
        file = file_queue + i;
        if(number >= file->music_total) 
        {
            number = number - file->music_total;
            continue;
        }
        else
        {
            break;    //target file find
        }
    }
    if(i == MAX_SONG_DIR)
    {
        return false;
    }
    *dir = i;
    *index = number;
    ret=GetFileName_From_NumInDir(i,number,name);
    return ret;
}
#endif

bool Get_File_Name_From_Number(int number, char *name)
{

    WORD i;
    //FIL *fhFile;
    FAT_DIR_INFO* file = NULL;
    for(i=0; i<MAX_SONG_DIR; i++)
    {
        file = file_queue + i;
        if(number >= file->music_total) 
        {
            number = number - file->music_total;
            continue;
        }
        else
        {
            break;    
        }
    }
    if(i == MAX_SONG_DIR)
    {
        return false;
    }
    return GetFileName_From_NumInDir(i,number,name);
}


static FRESULT  get_curdir_info(DIR_QUEUE *p_front)
{
    //WORD n;
    DIR dj;
    FILINFOADD fno;
    unsigned short  cur_musicfile = 0;
    //char *Ext = NULL;
    FAT_DIR_INFO* file = NULL;
    FRESULT res = FR_OK;
    DIR_QUEUE* front;
    DIR_QUEUE* next;
    dj.obj.fs = Fatfs_buf;
    dj.obj.stat = p_front->ExNoChainFlag;
    res=init_buf(Fatfs_buf);
    if(res != FR_OK)
    {
        return res;
    }
    if(p_front->broot_dir)
    {
        dj.obj.sclust = 0;    //for Fat16
    }
    else
    {
        dj.obj.sclust = p_front ->cluster_number;
        dj.obj.stat = p_front ->ExNoChainFlag;
    }
    front = p_front;
    next = p_front->next;
    res = Beken_dir_sdi(&dj, 0);
    while(res == FR_OK)
    {
        res = Beken_dir_read(&dj, 0);
        if (res != FR_OK)
        {
            break;
        }
        /* Get the directory name and push it to the buffer */
        Beken_get_fileinfo(&dj, &fno);
        if(((fno.fattrib & AM_DIR) != 0) && (front->dirlevel + 1 < MAX_DIR_LEVEL))
        {
#ifdef CONFIG_APP_RECORDER
            // os_printf("dir: %s | %d | %d\r\n",fno.fname, front->dirlevel, strncmp(fno.fname, "_RECORD", 7));
            if(front->dirlevel == 0){ if(is_rec_dir(fno.fname)) { rec_dir_sclaster_set(fno.fcluster); } }
#endif
            p_front->next = point_rear;
            p_front = point_rear;
            point_rear->cluster_number = fno.fcluster;
            point_rear->dirlevel = front->dirlevel + 1;
            point_rear->broot_dir = 0;
            point_rear->ExNoChainFlag = fno.ExNoChainFlag;
            point_rear->next = next;
            if(++rear_cnt > (MAX_DIR_SUPPORT - 1))
            {
                break;
            }
            point_rear++;
        }
        if((fno.fattrib & AM_ARC)!=0)
        {
            if(is_audio_file(fno.fname) >= F_TYP_MIN_SUPPORTED)
            {
                cur_musicfile++;
            }
        }
        res = Beken_dir_next(&dj, 0);
    }
    if (cur_musicfile)
    {
        file = file_queue + mp3queuecount;
        file->first_cluster = front->cluster_number;
        file->music_total = cur_musicfile;
        file->broot_dir     = front->broot_dir;
        file->ExNoChainFlag = front->ExNoChainFlag;
#ifdef CONFIG_APP_RECORDER
        if(rec_dir_sclaster_get() == file->first_cluster)
        {
            rec_dir_idx_set(mp3queuecount);
            rec_first_file_gidx_set(mp3filecount);
            rec_file_total_num_set(cur_musicfile);
            // os_printf("rec dir_cluster:0x%X, dir_idx:%d, first_file_gidx:%d, file_total:%d\n", file->first_cluster, mp3queuecount, sfile_gidx, );
        }
#endif
//      dirSongMax[mp3queuecount]=cur_musicfile;
        // os_printf("+++++++++++dir[%d]=%d,file=0x%x,file->broot_dir=%d\r\n",mp3queuecount,file->music_total,file,file->broot_dir);
        mp3queuecount++;
        mp3filecount += cur_musicfile;
    }
    uninit_buf(Fatfs_buf);
    return FR_OK;
}
/*scan whole disk, get total music file number and store the DIRs info
which have music files
*/
static uint32_t initfatsystem(void)
{
    UINT count = 0;
    DIR_QUEUE *dir_buf;
    rear_cnt = 0;
    CLEAR_WDT;
    if((Fatfs_buf == NULL) || (file_queue == NULL))
    {
        os_printf("Fatfs_buf or file_queue is NULL\r\n");
        return 1;
    }
    dir_buf=(DIR_QUEUE*)ff_memalloc(MAX_DIR_SUPPORT*sizeof(DIR_QUEUE));
    if(!dir_buf)
    {
        return 1;
    }
    beken_mem_set(file_queue, 0, MAX_SONG_DIR* sizeof(FAT_DIR_INFO));
    os_printf("dir queue malloc size:%d\n", MAX_DIR_SUPPORT * sizeof(DIR_QUEUE));
    mp3filecount = 0;
    mp3queuecount = 0;
    point_front = point_rear = dir_buf;
    point_front ->cluster_number = Fatfs_buf->dirbase;
    point_front ->dirlevel = 0;
    point_front ->ExNoChainFlag = 0;
    point_front->next           = NULL;
    if ((Fatfs_buf->fs_type == FS_FAT12)||(Fatfs_buf->fs_type == FS_FAT16))
    {
        point_front ->broot_dir = 1;
    }
    else
    {
        point_front ->broot_dir = 0;
    }
    point_rear++;
    rear_cnt++;
    while(1)
    {
        if ((mp3queuecount<MAX_SONG_DIR) && point_front && (count<MAX_DIR_SUPPORT))
        {
            if(rear_cnt > MAX_DIR_SUPPORT-1)
            {
                break;
            }
            get_curdir_info(point_front);
            count++;
            point_front = point_front->next;
        }
        else
        {
            break;
        }
    }
    os_printf("dir cnt=%d,  mp3 dir cnt=%d  mp3filecount=%d,rear_cnt =%d\r\n",count,mp3queuecount,mp3filecount,rear_cnt);
    ff_memfree(dir_buf);
    return 0;
}
/*Sd card & filesystem initialization*/
int Media_Fs_Init(uint8_t type)
{
    os_printf("%s(%d)\n", __func__, type);
    FRESULT res ;
    int ret = -1;

    FileInfo.fat_ok_flag = 0;

    if (fat_malloc_files_buffer() != FR_OK) {
        os_printf("#error [%d] fat_malloc_files_buffer failed !!!\n", __LINE__);
        while(1);
    }

    if(type == 0)
    {
        res = f_mount(Fatfs_buf,"0:",1);
    }
    else
    {
        res = f_mount(Fatfs_buf,"1:",1);
    }
    if( res == FR_OK)
    {
        os_printf(" mount ok\r\n");
        if(initfatsystem() == FR_OK)
        {
            FileInfo.fat_ok_flag = 1;
            ret = 0;
        }
    }
    else
    {
        os_printf(" mount fail:%d\r\n", res);
    }
    CLEAR_SLEEP_TICK;
    return ret;
}

int Media_Fs_Uninit(uint8_t type)
{
    FRESULT res;
    os_printf("%s(%d)\n", __func__, type);
    FileInfo.fat_ok_flag = 0;
    if(type == 0)
    {
        res = f_unmount(Fatfs_buf,"0:", 0);
    }
    else
    {
        res = f_unmount(Fatfs_buf,"1:", 0);
    }
    if (res == FR_OK)
    {
        os_printf(" unmount ok\r\n");
    }
    else
    {
        os_printf(" unmount fail:%d\r\n", res);
        return -1;
    }
    return 0;
}

uint8_t get_fat_ok_flag(void)
{
    return FileInfo.fat_ok_flag;
}

int get_music_gFileNumTotal(void) { return mp3filecount; }
int get_music_DirNumTotal(void) { return mp3queuecount; }
int get_music_dFileNumTotal(int DirIdx)
{
    return ((DirIdx >= mp3queuecount) ? -1 : ((file_queue + DirIdx)->music_total));
}

uint16_t get_musicdir_num(void)
{
    return mp3queuecount;
}

uint16_t get_musicfile_count(void)
{
    return mp3filecount;
}

uint16_t get_curDirMusicfile_count(uint16_t curDir)
{
    FAT_DIR_INFO* file = NULL;
    //os_printf("get_curDirMusicfile_count curDir=%d,\r\n",curDir);
    if(curDir >= mp3queuecount)
    {
        curDir = 0;
    }
    file = file_queue + curDir;
    os_printf("file_queue=%x,get_curDirMusicfile_file=0x%x,file->music_total=%d\r\n",file_queue,file,file->music_total);
    return file->music_total;
}
void *get_file_info(void)
{
    return (&FileInfo);
}
BYTE get_disk_type(void)
{
    return Fatfs_buf->pdrv;
}
//  modify thoes structs such as fsFD Fatfs_buf file_queue to malloc mode
//  can save ram space: 548+544+512=1604-3*4=1592 byte
int fat_malloc_files_buffer(void)
{
    fat_free_files_buffer();
    fsFD = (FIL*)jmalloc(sizeof(FIL), 1);
    Fatfs_buf = (FATFS*)jmalloc(sizeof(FATFS), 0);
    file_queue = (FAT_DIR_INFO*)jmalloc(MAX_SONG_DIR*sizeof(FAT_DIR_INFO),0);

    if(!fsFD || !Fatfs_buf || !file_queue) goto RET_ERR;

    return FR_OK;

RET_ERR:
    fat_free_files_buffer();
    return FR_INT_ERR;
}

int fat_free_files_buffer(void)
{
    if(file_queue) jfree(file_queue);
    if(Fatfs_buf) jfree(Fatfs_buf);
    if(fsFD) jfree(fsFD);
    file_queue = NULL;
    Fatfs_buf = NULL;
    fsFD = NULL;
    return FR_OK;
}
#endif

#ifdef SDCARD_FATFS_TEST
//refer to <http://elm-chan.org/fsw/ff/doc/readdir.html>
FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                os_printf("%s/%s/\n", path, fno.fname);//print current dir
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                os_printf("%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return res;
}

void dump_all_file_name(void)
{
    char buff[256];
    strcpy(buff, "/");//test ok
    // strcpy(buff, "");//test ok
    // strcpy(buff, "0:");//test ok
    // strcpy(buff, "1:");//test fail
    FRESULT ret = scan_files(buff);
    if(ret != FR_OK){
        os_printf("scan_files fail !!!\n");
    }
}

void sdcard_fatfs_test(void)
{
    int err;
    int f_cnt = 0;
    app_sd_set_online(1);
    err = fat_malloc_files_buffer();
    if(err != FR_OK)
    {
        os_printf("fat_malloc_files_buffer failed, please check\r\n");
        while(1);
    }
    Media_Fs_Init(0);
    f_cnt = get_musicfile_count();
    os_printf("there are %d files on disk\r\n",f_cnt);
    extern void dump_all_file_name(void);
    dump_all_file_name();
}
#endif
