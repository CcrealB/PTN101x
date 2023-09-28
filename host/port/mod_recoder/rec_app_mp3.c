/**
 * **************************************************************************************
 * @file    rec_app_mp3.c
 * 
 * @author  Borg Xiao
 * @date    20230515
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */

#include "config.h"

#if CONFIG_APP_RECORDER == 2

#include <string.h>
#include "app_beken_includes.h"
#include "driver_beken_includes.h"
#include "app_fat.h"
#include "rec_api.h"
#include "rec_include.h"
#include "diskio.h"
#include "mp3_encoder.h"

#define SAMPLE_RATE     48000
#define CHANNELS        2
#define BIT_RATE        128
#define MP3_FRA_SMPS    1152 //samples per channel, define by MPEG1 Layer3
#define MP3_FRA_SIZE    (MP3_FRA_SMPS * CHANNELS * sizeof(int16_t))



typedef struct _MP3_REC_CTX_t{
// ---- mp3
    MP3Encoder* mp3;
    uint8_t bit_rate;
    uint8_t chs;
    // uint16_t fra_smps;
    uint16_t fra_size;
// ---- file op
    FIL* fp;
    FSIZE_t f_wp;       //audio encoded size counter
    uint32_t pcm_sz_cnt;//audio pcm size counter
// ---- debug
    uint32_t f_wr_tms;  //file write cur time(unit:ms)
}MP3_REC_CTX_t;

static MP3_REC_CTX_t s_mp3_rec_ctx;
static MP3_REC_CTX_t* mp3_rec_h = &s_mp3_rec_ctx;

void *mp3_rec_h_get(void)
{
    return (void *)mp3_rec_h;
}

#if 1
void* mp3_enc_mem_malloc(int size)
{
    extern uint32_t _recmem_begin;
    return (void*)((uint32_t)&_recmem_begin & ~0x3);
}
#define mp3_enc_mem_free(...)
#else
#define mp3_enc_mem_malloc  rec_malloc
#define mp3_enc_mem_free    rec_free
#endif

int mp3_rec_file_create(char* fname, int fs, uint8_t chs, uint8_t bits)
{
    FRESULT res;
    int32_t ret;
    MP3_REC_CTX_t* mp3_h = mp3_rec_h;
    REC_LOG_I("%s(%d, %d, %d), br:%dbps\n", __FUNCTION__, fs, chs, bits, BIT_RATE);
//// malloc resource
    int32_t enc_ctx_size = mp3_encoder_size();
    mp3_h->mp3 = (MP3Encoder*)mp3_enc_mem_malloc(enc_ctx_size);
    if (mp3_h->mp3 == NULL)
    {
        REC_LOG_E("malloc mp3 enc err\n");
        goto RET;
    }

    mp3_h->fp = (FIL*)rec_malloc(sizeof(FIL));
    if (mp3_h->fp == NULL)
    {
        REC_LOG_E("malloc mp3 fp err\n");
        goto RET;
    }

//// mp3 enc init
    ret = mp3_encoder_init(mp3_h->mp3, SAMPLE_RATE, CHANNELS, BIT_RATE);
    if (ret != MP3_ENCODER_ERROR_NONE)
    {
        REC_LOG_E("mp3_encoder_init error\n");
        goto RET;
    }
    int32_t samples = mp3_encoder_get_samples_per_frame(mp3_h->mp3);//1152
    mp3_h->fra_size = samples * sizeof(int16_t) * CHANNELS;
    REC_LOG_I("mp3 frame smps:%d(%d bytes), enc_ctx_size:%d(%p)\n", samples, mp3_h->fra_size, enc_ctx_size, mp3_h->mp3);

//// mp3 file open/create
    res = f_open(mp3_h->fp, fname, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    if (res != FR_OK)
    {
        REC_LOG_E("f_open err:%d\n", res);
        goto RET;
    }

//// misc
    mp3_h->f_wp = 0;
    mp3_h->pcm_sz_cnt = 0;

    return 0;
RET:
    return -1;
}

void mp3_rec_enc_file_task(void)
{
    MP3_REC_CTX_t* mp3_h = mp3_rec_h;

    int rec_frame_size = MP3_FRA_SIZE;//mp3_rec_h->fra_size
    int fill_size = rec_ringbuf_fill_get();

    if (fill_size < rec_frame_size) goto RET;

//// get audio data
    uint8_t rec_frame_buff[MP3_FRA_SIZE];
    rec_ringbuf_read((uint8_t*)&rec_frame_buff[0], rec_frame_size);

//// mp3 encode
    // uint32_t mp3_i_sz = mp3_rec_h->fra_size; //pcm data size to encode
    uint32_t mp3_o_sz; //encoded data size
    uint8_t *mp3_i_pcm = (uint8_t*)rec_frame_buff;
    uint8_t *mp3_o_enc = (uint8_t*)rec_frame_buff;//384byte
    REC_DATA_ENC_OP(1);
    mp3_encoder_encode(mp3_h->mp3, (int16_t *)mp3_i_pcm, mp3_o_enc, &mp3_o_sz);
    REC_DATA_ENC_OP(0);

//// write to file
    uint8_t *f_wr_buf = (uint8_t *)mp3_o_enc;
    UINT f_wr_sz = mp3_o_sz;
    UINT f_wr_sz_get;
    file_wr_time_monitor(&mp3_rec_h->f_wr_tms, 0, 0);
    REC_FILE_WR_OP(1);
    f_lseek(mp3_h->fp, mp3_h->f_wp);
    f_write(mp3_h->fp, f_wr_buf, f_wr_sz, (UINT *)&f_wr_sz_get);
    REC_FILE_WR_OP(0);
    file_wr_time_monitor(&mp3_rec_h->f_wr_tms, f_wr_sz_get, 5);
    mp3_h->f_wp += f_wr_sz_get;

#ifdef DBG_REC_MP3_LOG
    static uint32_t t_ref = 0;
    if(sys_timeout(t_ref, 5000))
    {
        t_ref = sys_time_get();
        REC_PRINTF(" fill:%d, mp3_i_sz:%d, mp3_o_sz:%d\n", fill_size, mp3_rec_h->fra_size, mp3_o_sz);
        REC_PRINTF(" pcm_sz:%d, f_wp:%d\n", mp3_h->pcm_sz_cnt, mp3_h->f_wp);
    }
#endif

RET:
    return;
}

void mp3_rec_file_write_end(void)
{
    MP3_REC_CTX_t* mp3_h = mp3_rec_h;
    f_close(mp3_h->fp);

//// free resource
    if(mp3_h->fp) rec_free(mp3_h->fp);
    mp3_h->fp = NULL;
    if(mp3_h->mp3) mp3_enc_mem_free(mp3_h->mp3);
    mp3_h->mp3 = NULL;
}


#endif // CONFIG_APP_RECORDER
