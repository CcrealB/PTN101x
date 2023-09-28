
#include "config.h"

#ifdef CONFIG_APP_RECORDER

#include <string.h>
#include <stdlib.h>
#include "ff.h"
#include "playmode.h"
#include "app_fat.h"
#include "app_player.h"

#include "rec_include.h"


typedef struct _REC_FAT_CTX_t{
    uint32_t sclaster;
    uint16_t dir_idx;       // 0 ~ get_musicdir_count()
    uint16_t sfile_gidx;    // first file global index, 0 ~ get_musicfile_count()
    uint16_t file_total;    // (file_queue + dir_idx)->music_total
}REC_FAT_CTX_t;

static REC_FAT_CTX_t s_rec_fat_ctx;
static REC_FAT_CTX_t* s_rec_fat_h = &s_rec_fat_ctx;

uint32_t rec_dir_sclaster_get(void) { return s_rec_fat_h->sclaster; }
void rec_dir_sclaster_set(uint32_t sclaster) { s_rec_fat_h->sclaster = sclaster;  REC_LOG_I("fcluster:0x%X\n", sclaster); }

uint16_t rec_dir_idx_get(void) { return s_rec_fat_h->dir_idx; }
void rec_dir_idx_set(uint16_t dir_idx) { s_rec_fat_h->dir_idx = dir_idx; REC_LOG_I("dir_idx:%d\n", dir_idx); }

uint16_t rec_first_file_gidx_get(void) { return s_rec_fat_h->sfile_gidx; }
void rec_first_file_gidx_set(uint16_t sfile_gidx) { s_rec_fat_h->sfile_gidx = sfile_gidx; REC_LOG_I("sfile_gidx:%d\n", sfile_gidx); }

uint16_t rec_file_total_num_get(void) { return s_rec_fat_h->file_total; }
void rec_file_total_num_set(uint16_t file_total) { s_rec_fat_h->file_total = file_total; REC_LOG_I("file_total:%d\n", file_total); }


uint8_t is_rec_dir(char *dir_name)
{
    char *rec_dir_name = REC_DIR_NAME;
    if(strncmp(dir_name, rec_dir_name, strlen(rec_dir_name)) == 0)
    {
        return 1;
    }
    return 0;
}

uint8_t is_rec_file(char* fn)
{
    char rec_fn[32];
    rec_idx_to_fname(rec_fn, 0);
    int fn_len = strlen(fn);
    int rec_fn_len = strlen(rec_fn);
    // REC_LOG_I("is_rec_file:%s, %d, %s, %d\n", rec_fn, rec_fn_len, fn, fn_len);
    // REC_LOG_I("%d, %d\n", strncmp(fn, rec_fn, strlen(FNAME_PREFIX)),  ptn_strnicmp(&fn[fn_len - 3], EXT_NAME, 3));
//// check file name length & prefix & ext
    if(fn_len == rec_fn_len) 
    {
        int rec_fn_ext_len = strlen(EXT_NAME);
        if((strncmp(fn, rec_fn, strlen(FNAME_PREFIX)) == 0)
        && (ptn_strnicmp(&fn[fn_len - rec_fn_ext_len], EXT_NAME, rec_fn_ext_len) == 0))
        {
            return 1;
        }
    }
    return 0;
}

#if 1
static void rec_dir_scan_idx_max_cbk(char *fn, int *rec_file_cnt, int *rec_fn_idx_max)
{
    fn = ptn_strrchr(fn, '/') + 1;
    // REC_LOG_I("%s, %d\n", fn, strlen(fn));//print current dir
    if(is_rec_file(fn))
    {
        int rec_fn_idx = rec_fname_idx_pick(fn);
        if(*rec_fn_idx_max < rec_fn_idx) { *rec_fn_idx_max = rec_fn_idx; }
        // REC_LOG_I("rec_fn_idx:%d max:%d\n", rec_fn_idx, rec_fn_idx_max);
        (*rec_file_cnt)++;
    }
    // if(is_audio_file(fn) >= F_TYP_MIN_SUPPORTED){ total_music_count++; }
}

/** @brief get max file name index of rec dir
 * @param path the target dir to be traversal.
*/
int rec_dir_scan_idx_max(char* path)
{
    int rec_file_cnt = 0;
    int rec_fn_idx_max = 0;
    Traversal_Dir(path, 0, rec_dir_scan_idx_max_cbk, &rec_file_cnt, &rec_fn_idx_max);
    // s_rec_fat_h->file_total = rec_file_cnt;
    REC_LOG_I("rec_file_cnt:%d, rec_fn_idx_max:%d\n", rec_file_cnt, rec_fn_idx_max);
    return rec_fn_idx_max;
}
#else
int rec_dir_scan_idx_max(char* path)
{
    FRESULT res;
    DIR dir;
    // UINT i;
    // static FILINFO fno;
    FILINFO fno;

    int total_file_cnt = 0;
    int rec_file_cnt = 0;
    int rec_fn_idx_max = 0; //file name suffix num max value

    REC_LOG_I("%s:path:%s\n", __FUNCTION__, path);
    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res != FR_OK) goto RET;

    while(1)
    {
        res = f_readdir(&dir, &fno);                   /* Read a directory item */
        if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
        if((fno.fattrib & AM_HID) == 0)
        {
            if (fno.fattrib & AM_ARC)
            {
                total_file_cnt++;
                // REC_LOG_I("%s, %d\n", fno.fname, strlen(fno.fname));//print current dir
                if(is_rec_file(fno.fname))
                {
                    int rec_fn_idx = rec_fname_idx_pick(fno.fname);
                    if(rec_fn_idx_max < rec_fn_idx) { rec_fn_idx_max = rec_fn_idx; }
                    // REC_LOG_I("rec_fn_idx:%d max:%d\n", rec_fn_idx, rec_fn_idx_max);
                    rec_file_cnt++;
                }
            }
        }
    }
    f_closedir(&dir);
    s_rec_fat_h->file_num = rec_file_cnt;
    REC_LOG_I("total_file_cnt:%d, rec_file_cnt:%d, rec_fn_idx_max:%d\n", total_file_cnt, rec_file_cnt, rec_fn_idx_max);
RET:
    return rec_fn_idx_max;
    // return res;
}

/** @brief find the end file index of rec dir
 * @return end file index
*/
int rec_new_file_idx_get(char* path)
{
    FIL *fp = (FIL *)rec_malloc(sizeof(FIL));
    FRESULT fr;
    char fn[32];
    int k = 0;
    while (1)
    {
        sprintf(fn, "%s/%s%.6d.%s", path, FNAME_PREFIX, k, EXT_NAME);
        fr = f_open(fp, fn, FA_READ);
        if (fr != FR_OK)
            break;
        f_close(fp);
        k++;
    }
    rec_free(fp);
    return (k);
}
#endif


/** @brief get file name by rec file index
 * @param rec_file_idx file index in rec dir(0 ~ s_rec_fat_h->file_total) */
uint8_t rec_file_name_get_by_idx(uint16_t rec_file_idx, char* fn)
{
    uint8_t find = 0;
    uint16_t file_total = rec_file_total_num_get();

    if(rec_file_idx >= file_total) goto RET;

    find = Get_File_Name_From_Number(s_rec_fat_h->sfile_gidx + rec_file_idx, fn);
RET:
    return find;
}
#if defined(ZT_M184)
	uint16_t my_tf_index=0;
#endif
/** @brief play rec file by file index
 * @param rec_file_idx file index in rec dir(0 ~ s_rec_fat_h->file_total) */
void rec_file_play_by_idx(uint16_t rec_file_idx)
{
    uint16_t file_total = rec_file_total_num_get();

    if(rec_file_idx >= file_total) return;

    uint16_t global_file_index = rec_first_file_gidx_get() + rec_file_idx; //limit to get_musicdir_num();

    REC_LOG_I("rec_file_idx:%d, global_file_index:%d\n", rec_file_idx, global_file_index);
    app_player_set_fixed_dir(rec_dir_idx_get(), 1);
#if defined(ZT_M184)    
	my_tf_index=global_file_index;
#endif
    app_player_select_music(global_file_index);
}

#endif
