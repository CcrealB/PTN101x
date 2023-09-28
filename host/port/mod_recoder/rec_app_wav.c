/**
 * **************************************************************************************
 * @file    rec_app_wav.c
 * 
 * @author  Borg Xiao
 * @date    20230515
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */

#include "config.h"

#if CONFIG_APP_RECORDER == 1

#include <string.h>
#include "app_beken_includes.h"
#include "driver_beken_includes.h"
#include "app_fat.h"
#include "rec_api.h"
#include "rec_include.h"
#include "diskio.h"


#define RECORD_FRAME_SAMPLES        48//512
#define RECORD_FRAME_SIZE           (RECORD_FRAME_SAMPLES * 4)

#define PER_DISK_WRITE_SIZE         2048//write 2KByte at one time(multi of 512 is suggexted)


typedef struct
{
    char *buf;
    int szBuf;
    int cursor;
} linear_buffer_t;

typedef struct
{
    char *sector1; // wav文件文件头,临时数据缓冲区
    int sampCnt;
    FIL *fp;
    linear_buffer_t lbuf;
    _t_wave_hdr_format fmt;
    int bRec;
// ---- debug
    uint32_t f_wr_tms;  //file write cur time(unit:ms)
} wav_rec_ctx_t;
wav_rec_ctx_t s_wav_rec_ctx = {0};

static void malloc_rec_source(void *wav_rec_h)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)wav_rec_h;
    // app_rec_rb_init();
    // dbg_show_dynamic_mem_info(1);
    p_wav_rec_h->fp = (FIL *)rec_malloc(sizeof(FIL));
    p_wav_rec_h->sector1 = (char *)rec_malloc(0x200);
    p_wav_rec_h->lbuf.szBuf = PER_DISK_WRITE_SIZE;
    p_wav_rec_h->lbuf.buf = (char *)rec_malloc(p_wav_rec_h->lbuf.szBuf);
    p_wav_rec_h->lbuf.cursor = 0;
    p_wav_rec_h->sampCnt = 0;
    p_wav_rec_h->bRec = 0;
    memset(p_wav_rec_h->lbuf.buf, 0, p_wav_rec_h->lbuf.szBuf);
    memset(p_wav_rec_h->sector1, 0, 0x200);
}

static void free_rec_source(void *wav_rec_h)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)wav_rec_h;
    if (p_wav_rec_h->fp)
        rec_free(p_wav_rec_h->fp);
    p_wav_rec_h->fp = NULL;
    if (p_wav_rec_h->sector1)
        rec_free(p_wav_rec_h->sector1);
    p_wav_rec_h->sector1 = NULL;
    if (p_wav_rec_h->lbuf.buf)
        rec_free(p_wav_rec_h->lbuf.buf);
    p_wav_rec_h->lbuf.buf = NULL;
    // app_rec_rb_uninit();
    // dbg_show_dynamic_mem_info(1);
}

static void print_rec_proc(void *wav_rec_h)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)wav_rec_h;

    REC_LOG_I("sector1=%p\r\n", p_wav_rec_h);
    REC_LOG_I("sector1=%p\r\n", p_wav_rec_h->sector1);
    REC_LOG_I("sampCnt=%d\r\n", p_wav_rec_h->sampCnt);

    REC_LOG_I("fp=%p\r\n", p_wav_rec_h->fp);
    REC_LOG_I("lbuf.buf=%p\r\n", p_wav_rec_h->lbuf.buf);
    REC_LOG_I("lbuf.szBuf=%d\r\n", p_wav_rec_h->lbuf.szBuf);
    REC_LOG_I("lbuf.cursor=%d\r\n", p_wav_rec_h->lbuf.cursor);
    REC_LOG_I("fmt.wTag=%d\r\n", p_wav_rec_h->fmt.wTag);
    REC_LOG_I("fmt.wChn=%d\r\n", p_wav_rec_h->fmt.wChn);
    REC_LOG_I("fmt.dwSampleRate=%d\r\n", p_wav_rec_h->fmt.dwSampleRate);
    REC_LOG_I("fmt.dwAvgBytesPerSec=%d\r\n", p_wav_rec_h->fmt.dwAvgBytesPerSec);
    REC_LOG_I("fmt.wBlockAlign=%d\r\n", p_wav_rec_h->fmt.wBlockAlign);
    REC_LOG_I("fmt.wBitsPerSample=%d\r\n", p_wav_rec_h->fmt.wBitsPerSample);
    REC_LOG_I("bRec=%d\r\n", p_wav_rec_h->bRec);
}

static void build_wav_hdr_fmt(void *fmt, int sr, int chns, int br)
{
    _t_wave_hdr_format *pfmt = (_t_wave_hdr_format *)fmt;
    pfmt->wTag = 1;
    pfmt->wChn = chns;
    pfmt->dwSampleRate = sr;
    pfmt->wBitsPerSample = br;
    pfmt->wBlockAlign = pfmt->wChn * pfmt->wBitsPerSample / 8;
    pfmt->dwAvgBytesPerSec = pfmt->dwSampleRate * pfmt->wBlockAlign;
}

// static 
int wav_rec_lbuf_free_get(void)
{
    linear_buffer_t *plbuf = &s_wav_rec_ctx.lbuf;
    return plbuf->szBuf - plbuf->cursor;
}

static void wav_rec_lbuf_reset(void)
{
    linear_buffer_t *plbuf = &s_wav_rec_ctx.lbuf;
    plbuf->cursor = 0;
}

#if APP_ASYNC_SDCARD_ENABLE
static void wav_rec_wr_async_cbk(int res, int arg1, void *obj)
{
    wav_rec_lbuf_reset();
    // REC_LOG_I("%s\r\n",__func__);
}
#endif

static int write_to_file(void *wav_rec_h, void *buff, int size)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)wav_rec_h;
    linear_buffer_t *pbuf = &p_wav_rec_h->lbuf; //(linear_buffer_t*)cbuf;
    int ret = size;

    //////// fill linear buff
    if (pbuf->cursor + size <= pbuf->szBuf)
    {
        memcpy(&pbuf->buf[pbuf->cursor], buff, size);
        pbuf->cursor += size;
    }
    else
    {
        REC_LOG_E("rec buff overflow\n");
        ret = 0;
        goto RET;
    }

    //////// write file
    if(pbuf->cursor == pbuf->szBuf)
    {
        int write_size_get;
        file_wr_time_monitor(&s_wav_rec_ctx.f_wr_tms, 0, 0);
        REC_FILE_WR_OP(1);
    #if APP_ASYNC_SDCARD_ENABLE
        f_write_async_init(p_wav_rec_h->fp, pbuf->buf, pbuf->szBuf, (UINT *)&write_size_get, wav_rec_wr_async_cbk, NULL);
    #else
        f_write(p_wav_rec_h->fp, pbuf->buf, pbuf->szBuf, (UINT *)&write_size_get);
        wav_rec_lbuf_reset();
    #endif
        REC_FILE_WR_OP(0);
        file_wr_time_monitor(&s_wav_rec_ctx.f_wr_tms, write_size_get, 5);
    }
    // REC_LOG_I("cursor=%d\r\n",pbuf->cursor);
RET:
    return ret;
}

void *wav_rec_h_get(void)
{
    return (void *)&s_wav_rec_ctx;
}

int wav_rec_file_create(char* fname, int sr, int chn, int br)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)&s_wav_rec_ctx;
    FIL *fp;
    FRESULT fr;
    int ret;

    REC_LOG_I("wav_rec_file_create\n");
    malloc_rec_source((void*)p_wav_rec_h);
    fp = p_wav_rec_h->fp;

    fr = f_open(fp, fname, FA_CREATE_NEW | FA_WRITE | FA_READ);
    if (fr != FR_OK)
    {
        REC_LOG_E("file open err:%d\n", fr);
        ret = -1;
        goto RET;
    }
    build_wav_hdr_fmt(&p_wav_rec_h->fmt, sr, chn, br);
    int size = sizeof(p_wav_rec_h->fmt);
    // 写文件头
    write_to_file(p_wav_rec_h, "RIFF", 4); // 4 "RIFF"
    size = 0x2c;
    write_to_file(p_wav_rec_h, &size, 4);    // 4 RIFF-chunk size,should be modify when record ends
    write_to_file(p_wav_rec_h, "WAVE", 4); // 4 "WAVE"
    write_to_file(p_wav_rec_h, "fmt ", 4); // 4 "fmt "
    size = sizeof(p_wav_rec_h->fmt);
    write_to_file(p_wav_rec_h, &size, 4); // 4 fmt-chunk size
    write_to_file(p_wav_rec_h, &p_wav_rec_h->fmt,
                  sizeof(p_wav_rec_h->fmt) // 16 fmt-chunk data
    );
    write_to_file(p_wav_rec_h, "data", 4);               // 4 "data"
    write_to_file(p_wav_rec_h, &p_wav_rec_h->sampCnt, 4); // 4 data-chunk size,should be modify when record ends

    p_wav_rec_h->bRec = 1;
    print_rec_proc((void*)p_wav_rec_h);
RET:
    return ret;
}

//return: write file size
int wav_rec_file_write(void *wav_rec_h, void *buff, int size)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)wav_rec_h;
    int ret = write_to_file(wav_rec_h, buff, size);
    if(ret > 0){
        p_wav_rec_h->sampCnt += size / p_wav_rec_h->fmt.wBlockAlign;
    }
    return ret;
}

void wav_rec_enc_file_task(void)
{
    int ret;
    int rec_pcm_size = RECORD_FRAME_SIZE;//sizeof(rec_pcm_buff);
    int fill_size = rec_ringbuf_fill_get();

    if(fill_size < rec_pcm_size) goto RET;

    uint8_t rec_pcm_buff[RECORD_FRAME_SIZE];
    int lbuf_free_sz = wav_rec_lbuf_free_get();
    if(lbuf_free_sz == 0) goto RET;
    if(lbuf_free_sz < rec_pcm_size)  rec_pcm_size = lbuf_free_sz;

    rec_ringbuf_read((uint8_t*)rec_pcm_buff, rec_pcm_size);
    audio_stream_debug((int16_t*)rec_pcm_buff, rec_pcm_size << 2);
    ret = wav_rec_file_write(wav_rec_h_get(), rec_pcm_buff, rec_pcm_size);
    if(ret != rec_pcm_size) { REC_LOG_W("rec data loss:%d / %d\n", ret, rec_pcm_size); }
RET:
    return;
}

void wav_rec_file_write_end(void)
{
    wav_rec_ctx_t *p_wav_rec_h = (wav_rec_ctx_t *)&s_wav_rec_ctx;
    uint32 bw;
    FRESULT fr = 0;
    fr = fr;

    REC_LOG_I("wav_rec_file_write_end\n");

    // 将缓冲区内的剩余数据写入文件
    if (p_wav_rec_h->lbuf.cursor)
    {
        fr = f_write(p_wav_rec_h->fp, p_wav_rec_h->lbuf.buf, p_wav_rec_h->lbuf.cursor, &bw);
    }
    // 更新文件头:data size,file size
    f_lseek(p_wav_rec_h->fp, 0);
    f_read(p_wav_rec_h->fp, p_wav_rec_h->sector1, 0x200, &bw);
    bw = p_wav_rec_h->sampCnt * p_wav_rec_h->fmt.wBlockAlign;
    memcpy(&p_wav_rec_h->sector1[0x28], &bw, 4); // data size
    REC_LOG_I("data size:%x %d\n\n", bw, bw);
    bw += 0x24;
    memcpy(&p_wav_rec_h->sector1[0x04], &bw, 4); // RIFF chunk size
    f_lseek(p_wav_rec_h->fp, 0);
    fr = f_write(p_wav_rec_h->fp, p_wav_rec_h->sector1, 0x200, &bw);
    f_close(p_wav_rec_h->fp);
    print_rec_proc((void*)p_wav_rec_h);

    free_rec_source((void*)p_wav_rec_h);// 释放存储空间
    p_wav_rec_h->bRec = 0;
}

#endif