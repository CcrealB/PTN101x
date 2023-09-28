/* for rec app internal include */
#ifndef _REC_INCLUDE_H_
#define _REC_INCLUDE_H_

#include "config.h"
// ---------------------------------------- config

#define REC_RB_BUF_SIZE     8192 //rec ringbuff size


#ifndef REC_DIR_NAME
    #define REC_DIR_NAME            "_RECORD"//""
#endif
#ifndef FNAME_PREFIX
    #define FNAME_PREFIX            "rec"   //file name prefix, rec000000.wav
#endif
#define REC_FN_IDX_LEN              6 //3

#if CONFIG_APP_RECORDER == 1
    #define EXT_NAME    "wav"
    #define rec_file_create(fname, fs, chs, bits)   wav_rec_file_create(fname, fs, chs, bits)
    #define rec_enc_file_task()                     wav_rec_enc_file_task()
    #define rec_file_write_end()                    wav_rec_file_write_end()
#elif CONFIG_APP_RECORDER == 2
    #define EXT_NAME    "mp3"
    #define rec_file_create(fname, fs, chs, bits)   mp3_rec_file_create(fname, fs, chs, bits)
    #define rec_enc_file_task()                     mp3_rec_enc_file_task()
    #define rec_file_write_end()                    mp3_rec_file_write_end()
#endif

#define REC_READ_START()    //system_peri_mcu_irq_disable(SYS_PERI_IRQ_TIMER0)
#define REC_READ_STOP()     //system_peri_mcu_irq_enable(SYS_PERI_IRQ_TIMER0)

// ---------------------------------------- debug

#define REC_PRINTF                  os_printf
#define REC_LOG_I(fmt,...)          REC_PRINTF("[REC|I]"fmt, ##__VA_ARGS__)
#define REC_LOG_W(fmt,...)          REC_PRINTF("[REC|W:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define REC_LOG_E(fmt,...)          REC_PRINTF("[REC|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define REC_LOG_D(fmt,...)          REC_PRINTF("[REC|D]"fmt, ##__VA_ARGS__)

// #define DBG_REC_AS
// #define DBG_REC_TIME
// #define DBG_REC_MP3_LOG


#ifdef DBG_REC_TIME
    #define REC_DATA_ENC_OP(en)     REG_GPIO_0x1F = ((en) ? 2 : 0)//encode disk write
    #define REC_FILE_WR_OP(en)      REG_GPIO_0x20 = ((en) ? 2 : 0)//disk write
#else
    #define REC_DATA_ENC_OP(en);
    #define REC_FILE_WR_OP(en);
#endif

#ifdef DBG_REC_AS
void audio_stream_debug(int16_t *pcm, int samples)
#else
#define audio_stream_debug(...)
#endif

// ---------------------------------------- macro

#ifndef NULL
#define NULL ((void *)0)
#endif

// ---------------------------------------- typedef

typedef enum _REC_ST_EVT_e{
    REC_STAT_IDLE,
    REC_STAT_BUSY,

    REC_EVT_NONE = 0,
    REC_EVT_START,
    REC_EVT_STOP,
}REC_ST_EVT_e;

// ---------------------------------------- func

//common
void* rec_malloc(int size);
void rec_free(void* p);
inline void file_wr_time_monitor(uint32_t *wr_tms, int t_min, int wr_sz);

void audio_rec_proc_stereo(int16_t *pcm, int samples);


//app
void rec_idx_to_fname(char *fn_buf, int idx);
int rec_fname_idx_pick(char *rec_fn);

//wav
int wav_rec_file_create(char* fname, int sr, int chn, int br);
void wav_rec_enc_file_task(void);
void wav_rec_file_write_end(void);

//mp3
int mp3_rec_file_create(char* fname, int fs, uint8_t chs, uint8_t bits);
void mp3_rec_enc_file_task(void);
void mp3_rec_file_write_end(void);

// as
void app_rec_rb_init(void);
void app_rec_rb_uninit(void);
void* get_rec_rb(void);

int rec_ringbuf_fill_get(void);
int rec_ringbuf_read(uint8_t* buff, int size);


// rec fat
uint32_t rec_dir_sclaster_get(void);
void rec_dir_sclaster_set(uint32_t sclaster);

uint16_t rec_dir_idx_get(void);
void rec_dir_idx_set(uint16_t dir_idx);

uint16_t rec_first_file_gidx_get(void);
void rec_first_file_gidx_set(uint16_t sfile_gidx);

uint16_t rec_file_total_num_get(void);
void rec_file_total_num_set(uint16_t file_total);

uint8_t is_rec_dir(char *dir_name);
uint8_t is_rec_file(char* fn);

int rec_dir_scan_idx_max(char* path);

#endif /* _REC_INCLUDE_H_ */
