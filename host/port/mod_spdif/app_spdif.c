/**
 * **************************************************************************************
 * @file    app_spdif.c
 * 
 * @author  Borg Xiao
 * @date    modify @20230629, bug fixd: audio loss det invalid issue
 * @date    20230217
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * 
 * spdif(pcm24) --> spdif rb(pcm24) --> rb mov[src](pcm24) --> aso rb(pcm16/24)
 * spdif audio stream input fixed to pcm24, output config by SPDIF_ASO_BIT_WIDTH
 * 
 * to do:
 *  ASRC
 * **************************************************************************************
 * */
#include <string.h>
#include "driver_beken_includes.h"
#include "spdif_comm.h"
#include "drv_spdif.h"
#include "app_spdif.h"
#include "api_src.h"
#include "app_work_mode.h"

#ifdef CONFIG_APP_SPDIF

/* ---------------------------------------------------------------------- */

// #define spdif_aso_close                 aud_dac_close
// #define spdif_aso_open                  aud_dac_open
// #define spdif_aso_config(fs,chn,bit)  aud_dac_config(fs,chn,bit)
#define spdif_aso_close()               audio_aso_close(ASO_TYPE_SPDIF)
#define spdif_aso_open()                audio_aso_open(ASO_TYPE_SPDIF)
#define spdif_aso_config(fs,chn,bit)    audio_aso_config(fs,chn,bit,ASO_TYPE_SPDIF)
#define spdif_aso_rb_write              aud_dac_fill_buffer
#define spdif_aso_rb_free_size_get      aud_dac_get_free_buffer_size
#define spdif_aso_rb_fill_size_get      aud_dac_get_fill_buffer_size
#define spdif_aso_rb_clear              aud_dac_buffer_clear

/* ---------------------------------------------------------------------- */
enum{
    ST_SPDIF_DET_NORMAL,
    ST_SPDIF_DET_BUSY,
    ST_SPDIF_DET_LOST,
};

typedef struct _APP_SPDIF_CTX_t{
    uint8_t             init_cmp; //spdif init complete flag
    uint8_t             enable  ; //app mode enable
    uint8_t             ready   ; //spdif detect cmp and ready to start audio stream
    uint8_t             start   ; //audio start flag
    int                 fs;     //sample rate
    uint8_t             det_stat;
    // uint8_t             buf_rst_cnt;
    uint8_t             play_deb_cnt;
    // uint8_t             fra_smps;
    RingBufferContext*  p_rb_ctx;
    uint8_t*            p_rb_buf;
    SPDIF_CFG_CTX_t     spdif_cfg;
}APP_SPDIF_CTX_t;

/* ---------------------------------------------------------------------- */

#if CONFIG_SPDIF_SRC_AT_MCU == 1

static aud_src_ctx_t spdif_src_ctx = {
    .fs_in          = 0,
    .fs_out         = SPDIF_FS_OUT,
    .chs            = 2,
    .bits_in        = 0,   //init before audio_src_init
    .bits_out       = SPDIF_ASO_BIT_WIDTH,
    .max_smps_in    = 0,
    .max_smps_out   = 0,
    .max_fra_sz     = 192,
    .src_buf        = NULL,
};

#endif

APP_SPDIF_CTX_t app_spdif_st = {
    .init_cmp = 0,
    .enable = 0,
    .ready = 0,
    .start = 0,
    .fs = 0,
    .p_rb_ctx = NULL,
    .p_rb_buf = NULL,
    .spdif_cfg = {
        .io = SPDIF_GPIO,
        .cfg_idx = 0,
        .fifo_thrd = SPDIF_FIFO_THRD_DEF,
    },
};

RingBufferContext spdif_rb_ctx;
#if SPDIF_RINGBUF_MALLOC_EN == 0
uint8_t spdif_ringbuffer[AUDIO_SPDIF_RB_SIZE];
// def_aso_rb_addr_declare;
// #define spdif_ringbuffer    def_aso_rb_addr
#endif

/* ---------------------------------------------------------------------- */
#ifdef SPDIF_AS_DEBUG
static uint32_t g_spdif_log_flag = -1;
#else
static uint32_t g_spdif_log_flag = 0;
#endif
void spdif_log_flag_set(uint32_t flag, uint8_t en) { g_spdif_log_flag = en ? (g_spdif_log_flag | flag) : (g_spdif_log_flag & ~flag); }
uint32_t spdif_log_flag_get(uint32_t flag) { return (g_spdif_log_flag & flag); }
/* ---------------------------------------------------------------------- */

static uint32_t spdif_t_ref;
void spdif_timer_reset(void) { spdif_t_ref = sys_time_get(); }
uint32_t spdif_time_get(void) { return sys_time_get() - spdif_t_ref; }
int spdif_timeout(int over_tim_ms)
{
    if((sys_time_get() - spdif_t_ref) >= over_tim_ms){
        return 1;
    } else {
        // vTaskDelay(1);
        return 0;
    }
}

//io: GPIO11/12/14
int app_spdif_init(uint8_t enable)
{
    app_spdif_st.enable = 0;
    app_spdif_st.ready = 0;
    SPDIF_LOG_I("%s:GPIO%d\n", __FUNCTION__, app_spdif_st.spdif_cfg.io);
    if(enable)
    {
        spdif_init(&app_spdif_st.spdif_cfg);
    #if CONFIG_AUD_SYNC_SPDIF
        spdif_aud_sync_init();
    #endif
        spdif_enable(1);
        app_spdif_st.init_cmp = 1;
    }
    else
    {
        app_spdif_st.init_cmp = 0;
        spdif_enable(0);
        spdif_uninit(&app_spdif_st.spdif_cfg);
    }

    return SPDIF_RET_OK;
}

int app_spdif_enter(void)
{
    app_spdif_st.enable = 0;
    // app_spdif_st.ready = 0;
    // app_spdif_st.fs = 0;
    SPDIF_LOG_I("%s >> dmem left:%d\n", __FUNCTION__, dbg_show_dynamic_mem_info(1));
#if SPDIF_RINGBUF_MALLOC_EN
    if(app_spdif_st.p_rb_buf == NULL) app_spdif_st.p_rb_buf = jmalloc(AUDIO_SPDIF_RB_SIZE, M_ZERO);
    if(app_spdif_st.p_rb_buf == NULL) { SPDIF_LOG_E("malloc fail\n"); goto RET_ERR; }
#else
    app_spdif_st.p_rb_buf = (uint8_t*)spdif_ringbuffer;
#endif
#if (SPDIF_TRANS_MODE & SPDIF_TRANS_DMA)
    app_spdif_st.p_rb_ctx = &spdif_rb_ctx;
    spdif_dma_init(app_spdif_st.p_rb_ctx, app_spdif_st.p_rb_buf, AUDIO_SPDIF_RB_SIZE, 1);
    // spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, 1);
#endif
    app_spdif_st.enable = 1;
    SPDIF_LOG_I("%s << dmem left:%d\n", __FUNCTION__, dbg_show_dynamic_mem_info(1));

    if(app_spdif_st.ready) app_spdif_start(1);
    return SPDIF_RET_OK;

#if SPDIF_RINGBUF_MALLOC_EN
RET_ERR:
    if(app_spdif_st.p_rb_buf) jfree(app_spdif_st.p_rb_buf);
    SPDIF_LOG_E(" dmem left:%d\n", dbg_show_dynamic_mem_info(1));
    app_spdif_st.p_rb_ctx = NULL;
    app_spdif_st.p_rb_buf = NULL;
    return SPDIF_RET_FAIL;
#endif
}

void app_spdif_exit(void)
{
    app_spdif_st.enable = 0;
    // app_spdif_st.ready = 0;
    app_spdif_start(0);
#if (SPDIF_TRANS_MODE & SPDIF_TRANS_DMA)
    spdif_dma_uninit(&app_spdif_st.p_rb_ctx->dma);
#endif
    app_spdif_st.p_rb_ctx = NULL;
    #if SPDIF_RINGBUF_MALLOC_EN
    if(app_spdif_st.p_rb_buf) jfree(app_spdif_st.p_rb_buf);
    #endif
    app_spdif_st.p_rb_buf = NULL;
    SPDIF_LOG_I("%s << dmem left:%d\n", __FUNCTION__, dbg_show_dynamic_mem_info(1));
}

#if CONFIG_SPDIF_SRC_AT_MCU == 1
//frame_tim_ms : time msec per frame
//fs:sample rate
int aud_fra_smps_max_get(uint8_t frame_tim_ms, int fs)
{
    int temp = frame_tim_ms * fs;
    int fra_smps_max = temp / 1000;
    if((fra_smps_max * 1000) != temp) fra_smps_max += 1;
    return fra_smps_max;
}
#endif

void app_spdif_start(uint8_t en)
{
    SPDIF_LOG_I("%s(%d), dmem left:%d\n", __FUNCTION__, en, dbg_show_dynamic_mem_info(1));
    app_spdif_st.play_deb_cnt = 0;
    if(app_spdif_st.start == en) goto RET;
    if(en)
    {
        if(app_spdif_st.enable == 0) {
            SPDIF_LOG_E("spdif current is disabled!\n");
            goto RET;
        }
    #if CONFIG_SPDIF_SRC_AT_MCU == 1
        spdif_src_ctx.fs_in = app_spdif_st.fs;
        spdif_src_ctx.fs_out = SPDIF_FS_OUT;
        spdif_src_ctx.chs = 2;
        spdif_src_ctx.bits_in = SPDIF_ASO_BIT_WIDTH;//config at init
        spdif_src_ctx.bits_out = SPDIF_ASO_BIT_WIDTH;
        spdif_src_ctx.max_smps_in = aud_fra_smps_max_get(SPDIF_FRAME_TIMms, app_spdif_st.fs);
        spdif_src_ctx.max_smps_out = aud_fra_smps_max_get(SPDIF_FRAME_TIMms, SPDIF_FS_OUT);
        audio_src_init(&spdif_src_ctx);
        SPDIF_LOG_I("%d, dmem left:%d\n", __LINE__, dbg_show_dynamic_mem_info(1));
        spdif_aso_config(SPDIF_FS_OUT, 2, SPDIF_ASO_BIT_WIDTH);
    #else
        spdif_aso_config(app_spdif_st.fs, 2, SPDIF_ASO_BIT_WIDTH);
    #endif
    #if CONFIG_AUD_SYNC_SPDIF
        spdif_aud_sync_reset();
    #endif
        // spdif_fifo_clear();
    #if (SPDIF_TRANS_MODE & SPDIF_TRANS_DMA)//cconsumed fifo data
        spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, 1);
        sys_delay_us(1);
        spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, 0);
    #endif
        ring_buffer_reset(app_spdif_st.p_rb_ctx);
        spdif_aso_rb_clear();

        //fill mute data to half aso ringbuff
        uint8_t buff[spdif_aso_Smp2Byte(128)];
        int cnt = SPDIF_ASO_BUFF_SIZE / 2 / sizeof(buff);
        memset(buff, 0, sizeof(buff));
        for(int i = 0; i < cnt; i++){ spdif_aso_rb_write(buff, sizeof(buff)); }

        spdif_aso_open();
    #if (SPDIF_TRANS_MODE & SPDIF_TRANS_DMA)
        spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, en);
    #endif
    #if (SPDIF_TRANS_MODE & SPDIF_TRANS_INTR)
        spdif_int_enable(en);
    #endif
        app_spdif_st.start = 1;
    }
    else
    {
        app_spdif_st.start = 0;
        spdif_aso_close();
    #if (SPDIF_TRANS_MODE & SPDIF_TRANS_DMA)
        spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, en);
    #endif
    #if (SPDIF_TRANS_MODE & SPDIF_TRANS_INTR)
        spdif_int_enable(en);
    #endif
        // spdif_fifo_clear();
        ring_buffer_reset(app_spdif_st.p_rb_ctx);
        // spdif_aso_rb_clear();
    #if CONFIG_SPDIF_SRC_AT_MCU == 1
        audio_src_uninit(&spdif_src_ctx);
    #endif
    }

RET:
    return;
}

//spdif signal detected and init cmp
__attribute__((weak))
void app_spdif_ready_callback(void)
{
	SPDIF_LOG_I("%s\n", __FUNCTION__);
    if(spdif_mode_auto_sw_get()){
        system_work_mode_set_button(SYS_WM_SPDIF_MODE);
    }
}

//spdif signal lost
__attribute__((weak))
void app_spdif_lost_callback(void)
{
	SPDIF_LOG_I("%s\n", __FUNCTION__);
    if(spdif_mode_auto_exit_get()){
        SPDIF_LOG_I("lost action active\n", __FUNCTION__);
        system_work_mode_change_button();
    }
}

#ifdef APP_SPDIF_DEBUG
void app_spdif_debug_show()
{
    static uint32_t t_mark;
    if(!sys_timeout(t_mark, 2000)) return;
    t_mark = sys_time_get();

    spdif_log_flag_set(SPDIF_LOG_MSK_DBG, 1);
    SPDIF_LOG_D("dbg fs:%d, audio:%d\n", app_spdif_st.fs, spdif_is_audio());
    SPDIF_LOG_D("det fs, f:%d, c:%d\n", spdif_fs_det_fine(), spdif_fs_det_course());

//// driver debug
    // extern void spdif_debug_show(void);
    // spdif_debug_show();
}
#endif

int app_spdif_scan(void)
{
    int ret = 0;
    if(!app_spdif_st.init_cmp) goto RET;
    int spdif_fs_new = spdif_is_audio() ? spdif_sample_rate_get() : 0;
    #if 0//debug
    static uint32_t t_mark;
    if(sys_timeout(t_mark, 2000)){
        t_mark = sys_time_get();
        SPDIF_LOG_I("- fs new:%d, old:%d; audio:%d\n", app_spdif_st.fs, spdif_fs_new, spdif_is_audio());
    }
    #endif

    static int spdif_fs_prv = -1;
    switch (app_spdif_st.det_stat)
    {
    case ST_SPDIF_DET_NORMAL:
    {
        if(app_spdif_st.fs != spdif_fs_new)
        {
            if(app_spdif_st.ready) //close audio stream
            {
                app_spdif_st.ready = 0;
                app_spdif_start(0);
            }
            // app_spdif_lost_callback();

            SPDIF_LOG_I("#### det fs change:%d --> %d\t", app_spdif_st.fs, spdif_fs_new);
            spdif_fs_prv = spdif_fs_new;
            spdif_timer_reset();
            spdif_audio_rcv_check();
            app_spdif_st.det_stat = ST_SPDIF_DET_BUSY;
        }
    }
    break;
    case ST_SPDIF_DET_BUSY:
    {
        if(spdif_fs_new != spdif_fs_prv)
        {
            SPDIF_PRINTF("time:%d\n\n", spdif_time_get());
            SPDIF_LOG_I("audio:%d, fs:%d -> %d\t", spdif_is_audio(), spdif_fs_prv, spdif_fs_new);
            spdif_fs_prv = spdif_fs_new;
            spdif_timer_reset();
            spdif_audio_rcv_check();
        }
        else
        {
            SPDIF_PRINTF(".");
            if(spdif_timeout(FS_DET_DEBOUNCEms))
            {
                SPDIF_PRINTF("time_out:%d\n\n", spdif_time_get());
                app_spdif_st.fs = spdif_fs_new;
                spdif_timer_reset();
                if(spdif_fs_new < 16000 || spdif_fs_new > 48000)
                {
                    SPDIF_PRINTF("\n\n\n\n");
                    SPDIF_LOG_W("not supported fs:%d, audio:%d\n", spdif_fs_new, spdif_is_audio());
                    app_spdif_st.det_stat = (spdif_fs_new == 0) ? ST_SPDIF_DET_LOST : ST_SPDIF_DET_NORMAL;
                }
                else
                {
                    SPDIF_LOG_I("ready fs:%d\n", app_spdif_st.fs);
                    ret = spdif_fs_new;
                    app_spdif_st.ready = 1;
                    if(app_spdif_st.enable) app_spdif_start(1);//if is spdif mode, 
                    else app_spdif_ready_callback();//if not in spdif mode
                    app_spdif_st.det_stat = ST_SPDIF_DET_NORMAL;
                }
            }
        }
    }
    break;
    case ST_SPDIF_DET_LOST:
    {
        spdif_audio_rcv_check();
        if(app_spdif_st.enable) app_spdif_lost_callback();
        app_spdif_st.det_stat = ST_SPDIF_DET_NORMAL;
    }
    break;
    default: break;
    }
RET:
    return ret;
}

//return: 1:spdif is online and have audio signal, 0:off line
int spdif_is_online(void)
{
    return spdif_is_audio();
}

int app_spdif_ready(void)
{
    return (app_spdif_st.ready == 1);
}

int app_spdif_fs_get(void)
{
    return ((app_spdif_st.ready == 1) ? app_spdif_st.fs : 0);
}

/**
 * @brief move spdif ringbuff to aso, place in audio proc schedule
 * @note audio stream:[reg -> spdif ringbuf -> dsp ringbuf], 
 * */
void spdif_audio_proc(void)
{
#if (SPDIF_TRANS_MODE & (SPDIF_TRANS_DMA | SPDIF_TRANS_INTR))
    #define SAMPLES_THRD    (48 + (SPDIF_FIFO_THRD_DEF / 2))
    if(app_spdif_st.start == 0) { goto RET; }
#if 0
    int aso_free_smps = spdif_aso_Byte2Smp(spdif_aso_rb_free_size_get());
#if CONFIG_SPDIF_SRC_AT_MCU == 1
    // int aso_fra_smps = (SPDIF_FRAME_SMPS + SAMPLES_THRD) * SPDIF_FS_OUT / app_spdif_st.fs + 6;//+6 for src, 48/8=6
    #define aso_fra_smps    (SPDIF_FRAME_SMPS + SAMPLES_THRD) * SPDIF_FS_OUT / 8000
#else
    #define aso_fra_smps    (SPDIF_FRAME_SMPS + SAMPLES_THRD)
#endif
    if(aso_free_smps < aso_fra_smps) goto RET;
#endif
    int32_t pcm[(SPDIF_FRAME_SMPS + SAMPLES_THRD + 2) * 2];//+2 for spc
    // static uint8_t aud_fra_cnt = 0;
    // int samples_tgt = as_fra4ms_smps_get(&aud_fra_cnt, app_spdif_st.fs);
    int samples_tgt = SPDIF_FRAME_SMPS * app_spdif_st.fs / SPDIF_FS_OUT;
    int smps_thrd = SAMPLES_THRD * app_spdif_st.fs / SPDIF_FS_OUT;
    int samples = spdif_Byte2Smp(ring_buffer_get_fill_size(app_spdif_st.p_rb_ctx));
    SPDIF_AS_PROC_RUN(1);
    if(samples > (samples_tgt + smps_thrd) || samples < (samples_tgt - smps_thrd))
    {
        spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, 0);
        if(spdif_timeout(1000))
        {
            spdif_timer_reset();
            SPDIF_LOG_W("buf reset:%d/%d,%d\n", samples, samples_tgt,spdif_is_audio());
        }
        ring_buffer_reset(app_spdif_st.p_rb_ctx);
        spdif_dma_enable(app_spdif_st.p_rb_ctx->dma, 1);
        // if(samples == 0){ if(++cnt >= 250) { cnt = 0; } }
        samples = samples_tgt;
        memset(pcm, 0, sizeof(pcm));
        app_spdif_st.play_deb_cnt = 0;
        // goto RET;
    }
    else
    {
        ring_buffer_read(app_spdif_st.p_rb_ctx, (uint8_t *)pcm, spdif_Smp2Byte(samples));
        if(app_spdif_st.play_deb_cnt < (100 / SPDIF_FRAME_TIMms)) { app_spdif_st.play_deb_cnt++; memset(pcm, 0, sizeof(pcm)); }
    }
    spdif_audio_fmt_cvt(pcm, pcm, samples, SPDIF_ASO_BIT_WIDTH);
#if CONFIG_SPDIF_SRC_AT_MCU == 1
    SPDIF_SRC_CODE_RUN(1);
    samples = audio_src_apply(&spdif_src_ctx, (int32_t*)pcm, (int32_t*)pcm, samples);
    SPDIF_SRC_CODE_RUN(0);
#endif
#if CONFIG_AUD_SYNC_SPDIF
    int aso_fill_smps = spdif_aso_Byte2Smp(spdif_aso_rb_fill_size_get());
    int comp_num = spdif_aud_sync_proc(samples, aso_fill_smps);
    #if CONFIG_AUD_SYNC_SPDIF == 1
    samples = audio_spc_exec(pcm, (int32_t*)pcm, samples, SPDIF_ASO_BIT_WIDTH, 2, comp_num, samples);
    #else
    UNUSED(comp_num);
    #endif
#endif
    spdif_aso_rb_write((uint8_t *)pcm, spdif_aso_Smp2Byte(samples));
    SPDIF_AS_PROC_RUN(0);
RET:
    return;
#endif
}

void app_spdif_task(void)
{
#ifdef SPDIF_IRQ_NEST_DBG
    static int s_spdif_irq_nest_fg = 0;
    if(s_spdif_irq_nest_fg) SPDIF_LOG_I("irq\t");
    s_spdif_irq_nest_fg = 1;
#endif
    static uint32_t t_ref = 0;
    if(!sys_timeout(t_ref, SPDIF_FRAME_TIMms)) goto RET;
    t_ref = sys_time_get();
#ifdef CONFIG_APP_SPDIF
    app_spdif_scan();
#endif
#if SPDIF_TRANS_MODE == SPDIF_TRANS_DMA
    spdif_audio_proc();
#endif
RET:
#ifdef SPDIF_IRQ_NEST_DBG
    s_spdif_irq_nest_fg = 0;
#endif
    return;
}

void spdif_isr(void)
{
#if SPDIF_TRANS_MODE == SPDIF_TRANS_INTR
    #define FRAME_SAMPLES     (SPDIF_FIFO_THRD_DEF/1)
    int32_t audio_data[FRAME_SAMPLES*2];
    int samples = FRAME_SAMPLES; //2ch
    int32_t *pcm = (int32_t*)audio_data;

    uint32_t intr_status = REG_SPDIF_INTSTUS;
    REG_SPDIF_INTSTUS = intr_status;
    uint32_t audio_status = REG_SPDIF_AUDIO_STUS1;
    // SPDIF_LOG_I("aud:0x%d int:0x%X, msk:0x%X, cfg1:0x%X, aud_cfg1:0x%X\n", audio_status, intr_status, REG_SPDIF_INTMSK, REG_SPDIF_CONFIG1, REG_SPDIF_AUDIO_CONFIG1);
    
    SPDIF_AS_PROC_RUN(1);
    SPDIF_AS_PROC_RUN(0);
    // if (audio_status & MSK_SPDIF_AUDIO_STUS1_YNEED)
    if (intr_status & MSK_SPDIF_INTSTUS_XOVER)
    {
        // SPDIF_LOG_I(" $%d\n", audio_status);
        SPDIF_AS_PROC_RUN(1);
        int i;
        for (i = 0; i < (samples * 2); i++)//2ch
        {
            pcm[i] = REG_SPDIF_AUDIO_FIFO_XY;
            if(REG_SPDIF_AUDIO_STUS1 & MSK_SPDIF_AUDIO_STUS1_XUNDER) break;
            // if(REG_SPDIF_INTSTUS & MSK_SPDIF_INTSTUS_XUNDER) break;
        }
        // SPDIF_LOG_I("~%d/%d\n", i, REG_SPDIF_AUDIO_CONFIG1 & 0xFF /*fifo_thrd*/);
        if(app_spdif_st.ready == 1)
        {
            if(spdif_aso_rb_free_size_get() >= samples * 8)
            {
                spdif_audio_fmt_cvt(pcm, pcm, samples, SPDIF_ASO_BIT_WIDTH);
                //TODO ASRC
                spdif_aso_rb_write((uint8_t *)pcm, samples * 8);
            }
            else
            {
                SPDIF_LOG_E("data loss, rb_free:%d %X\n", spdif_aso_rb_free_size_get(), audio_status);
            }
        }
        SPDIF_AS_PROC_RUN(0);
    }

#elif SPDIF_TRANS_MODE == (SPDIF_TRANS_DMA | SPDIF_TRANS_INTR)

    SPDIF_AS_PROC_RUN(1);
    SPDIF_AS_PROC_RUN(0);
    // uint32_t intr_status = REG_SPDIF_INTSTUS;
    // REG_SPDIF_INTSTUS = intr_status;
    // if (intr_status & MSK_SPDIF_INTSTUS_DMAFINISH)
    {
        spdif_audio_proc();
    }
    REG_SPDIF_INTSTUS |= REG_SPDIF_INTSTUS;
#endif
}

#endif //CONFIG_APP_SPDIF

