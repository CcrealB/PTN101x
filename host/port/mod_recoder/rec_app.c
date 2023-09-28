

#include "config.h"

#ifdef CONFIG_APP_RECORDER

#include <string.h>
#include <stdlib.h>
#include "app_beken_includes.h"
#include "driver_beken_includes.h"
#include "app_fat.h"
#include "rec_api.h"
#include "rec_include.h"
#include "diskio.h"



extern volatile uint8_t rec_rb_overflow_cnt;

static int s_rec_file_idx = 0;  //used by file name index
static uint8_t s_recorder_state = REC_STAT_IDLE;
static uint8_t s_recorder_event = REC_EVT_NONE;
static uint8_t s_rec_disk_type = DISK_TYPE_SD;

typedef struct _REC_CONTEXT_t{
    uint16_t file_idx;
    uint16_t file_total;
}REC_CONTEXT_t;

// static REC_CONTEXT_t s_recoder_ctx;
// static REC_CONTEXT_t *recoder_h;

void app_rec_res_malloc(void)
{
    // recoder_h = rec_malloc(sizeof(REC_CONTEXT_t));
}

void app_rec_res_free(void)
{
    // if(recoder_h) rec_free(recoder_h);
}

/** gen rec file name by index number
 * @param idx[in] idx
 * @param fn_buf[out] rec file name
 * @exp: rec_idx_to_fname(buf, 5) -> buf: "rec000005.mp3"
*/
void rec_idx_to_fname(char *fn_buf, int idx)
{
#if REC_FN_IDX_LEN == 6
    sprintf(fn_buf, "%s%.6d.%s", FNAME_PREFIX, idx, EXT_NAME);
#elif REC_FN_IDX_LEN == 3
    sprintf(fn_buf, "%s%.4d.%s", FNAME_PREFIX, idx, EXT_NAME);
#endif
}

//get rec fname index from rec file name
int rec_fname_idx_pick(char *rec_fn)
{
    char rec_fn_idx_str[REC_FN_IDX_LEN + 1];
    int rec_fn_idx;

    memcpy(rec_fn_idx_str, &rec_fn[strlen(FNAME_PREFIX)], REC_FN_IDX_LEN);
    rec_fn_idx_str[REC_FN_IDX_LEN] = 0;
    rec_fn_idx = atoi(rec_fn_idx_str);
    return rec_fn_idx;
}

//disk_type: DISK_TYPE_SD / DISK_TYPE_UDISK (refer @diskio.h)
void rec_disk_type_set(uint8_t disk_type) { s_rec_disk_type = disk_type; }
uint8_t rec_disk_type_get(void) { return s_rec_disk_type; }

int rec_init_fs(uint8_t disk_type)
{
    rec_disk_type_set(disk_type);
    if((get_fat_ok_flag() == 1) && (get_cur_media_type() != disk_type))
    {
        Media_Fs_Uninit(get_cur_media_type());
    }
    if((get_fat_ok_flag() == 0) && 1)//if fat_ok and sdcard is mount ok
    {
        if(Media_Fs_Init(disk_type) != FR_OK)
        {
            REC_LOG_E("disk Init Err:%d !!!\r\n", disk_type);
            if(disk_type == DISK_TYPE_SD) msg_put(MSG_SD_READ_ERR);
            goto RET_ERR;
        }
    }
    return 0;
RET_ERR:
    return -1;
}

//judge rec dir, create rec dir if not exist.
void rec_init_first(void)
{
    FRESULT res;
    char dir_name_buf[64];
    REC_LOG_I("%s\n", __FUNCTION__);
    sprintf(dir_name_buf, "%d:/%s", rec_disk_type_get(), REC_DIR_NAME);
    res = f_mkdir(dir_name_buf);
    if(res == FR_OK){
        FAT_LOG_I("f_mkdir:%s\n", dir_name_buf);
    }else if(res == FR_EXIST){
        FAT_LOG_I("record dir '%s' exist.\n", dir_name_buf);
    }else if(res != FR_OK){
        FAT_LOG_E("f_mkdir:%d\n", res);
        goto RET;
    }
    // s_rec_file_idx = rec_new_file_idx_get(dir_name_buf);
RET:
    return;
}

void rec_init(void)
{
    uint8_t disk_type = rec_disk_type_get();
    FAT_LOG_I("%s, disk_type:%d\n", __FUNCTION__, disk_type);
//// fs init
    if(rec_init_fs(disk_type) != 0) goto RET;
//// mk dir init
    rec_init_first();
//// resource init
    app_rec_res_malloc();
    app_rec_rb_init();

//// gen file name index
    char rec_path[64];
    sprintf(rec_path, "%d:/%s", rec_disk_type_get(), REC_DIR_NAME);
    s_rec_file_idx = rec_dir_scan_idx_max(rec_path) + 1;

//// gen file name
    char fn_all[64] = {0};
    char fn[32] = {0};
    rec_idx_to_fname(fn, s_rec_file_idx);
    sprintf(fn_all, "%d:/%s/%s", rec_disk_type_get(), REC_DIR_NAME, fn);

//// create file
    rec_file_create(fn_all, 48000, 2, 16);
    FAT_LOG_I("create file:%s/%s\n", rec_path, fn);
RET:
    return;
}

void rec_uninit(void)
{
    rec_file_write_end();
    app_rec_rb_uninit();
    app_rec_res_free();
    // s_rec_file_idx++;
}

uint8_t recorder_is_working(void) { return (s_recorder_state == REC_STAT_BUSY); }

//disk_type: DISK_TYPE_SD / DISK_TYPE_UDISK (refer @diskio.h)
void recorder_start(uint8_t disk_type, uint8_t en)
{
    if(en) {
        if(recorder_is_working()) return;
        rec_disk_type_set(disk_type);
        s_recorder_event = REC_EVT_START;
    }else{
        s_recorder_event = REC_EVT_STOP;
    }
}

static void rec_event_proc(void)
{
    if(s_recorder_event == REC_EVT_NONE)
    {
        goto RET;
    }
    else if(s_recorder_event == REC_EVT_START)
    {
        if(s_recorder_state == REC_STAT_BUSY)
        {
            REC_LOG_I("can't start, record is busy.\n");
            goto RET;
        }
        s_recorder_state = REC_STAT_BUSY;
        rec_init();
        REC_LOG_I("record start @ %ums\n", sys_time_get());
#if WAV_SRC_MIC == 1 //open mic
        aud_mic_config(sr, chn, br);
        aud_mic_open(1);
#endif
    }
	else if(s_recorder_event == REC_EVT_STOP)
    {
        if(s_recorder_state == REC_STAT_IDLE)
        {
            REC_LOG_I("can't stop, record is idle.\n");
            goto RET;
        }

#if WAV_SRC_MIC == 1 //close mic
        aud_mic_open(0); 
#endif
        rec_uninit();
        int err = Media_Fs_Init(rec_disk_type_get());
        if (err == 0)
        {
            REC_LOG_I("file system remount ok!\n");
        }
        else
        {
            REC_LOG_I("file system remount err:%d\n", err);
        }
        s_recorder_state = REC_STAT_IDLE;
        REC_LOG_I("record stop @ %ums, %d\n", sys_time_get(), s_rec_file_idx);
	}
    dbg_show_dynamic_mem_info(1);
RET:
    s_recorder_event = REC_EVT_NONE;
    return;
}

void app_sound_record(void)
{
#if APP_ASYNC_SDCARD_ENABLE
    if(f_get_async_state() == ASYNC_BUSY) { goto RET;; }
#endif
    rec_event_proc();

    if (s_recorder_state == REC_STAT_IDLE)
    {
        goto RET;
    }

    if(rec_rb_overflow_cnt){
        REC_LOG_W("rec ringbuff overflow, times:%d\n", rec_rb_overflow_cnt);
        rec_rb_overflow_cnt = 0;
    }

    rec_enc_file_task();

RET:
    return;
}


#endif // CONFIG_APP_RECORDER
