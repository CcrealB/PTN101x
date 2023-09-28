/**
 * **************************************************************************************
 * @file    audio_sync_usb.c
 * 
 * @author  Borg Xiao
 * @date    20230315
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include <stdint.h>
#include <string.h>
#include "driver_audio.h"
#include "audio_hid.h"
#include "audio_spc.h"
#include "app_dsp.h"


#ifdef USB_DEV_DEBUG //define @ audio_hid.h
    #define AUDIO_STREAM_DBG        1
    #define BUFF_LVL_WAVE_DBG       0   //show err change wave by sscom
#endif

#if CONFIG_AUD_SYNC_USBO || CONFIG_AUD_SYNC_USBI
typedef struct _CLK_TRACKER_CTX_t{
    int32_t     Kp;
    int32_t     Ki;
    int32_t     Kd;

    int32_t     Ek;
    int32_t     Ek1;
    int32_t     Ek2;

    uint32_t    target;
    uint32_t    thrd;
    uint32_t    lvl_cur;
    int32_t     ppm_max; //the max ppm can be adjusted(ref:300~2000)
    int32_t     ppm;     //adj -ppm
}CLK_TRACKER_CTX_t;

typedef struct _AUD_SYNC_USB_t{
    AUD_SPC_CTX_t* spc_h;
    CLK_TRACKER_CTX_t* tracker_h;
    int32_t     ppm;
    uint16_t    smps_in;
    uint16_t    smps_out;
    uint32_t    dbg_cnt;
    uint32_t    lvl_cur_acc;
    int16_t     lvl_cur_cnt;
    uint8_t     bits;
    uint8_t     is_src;     //object audio stream is src
}AUD_SYNC_USB_t;



/** @brief estimate obj ppm
 * @return object ppm base local
 *  per obj is src, (level > level_H) --> (err > 0) --> (obj clk is faster) --> (adj_ppm > 0)
 *  per obj is sink, (level > level_H) --> (err > 0) --> (obj clk is slower) --> (adj_ppm < 0)
 * */
static int clock_tracking_exec(CLK_TRACKER_CTX_t* tracker_h)
{
    if(tracker_h == NULL) return 0;

    int ppm = tracker_h->ppm;
    int ppm_max = tracker_h->ppm_max;

    tracker_h->Ek2 = tracker_h->Ek1;
    tracker_h->Ek1 = tracker_h->Ek;
    tracker_h->Ek = tracker_h->lvl_cur - tracker_h->target;

    #if 1 //calc ppm value
    ppm = tracker_h->Kp * tracker_h->Ek
        + tracker_h->Kd * (tracker_h->Ek - tracker_h->Ek1);
    #else //calc ppm increment
    ppm += tracker_h->Kp * (tracker_h->Ek - tracker_h->Ek1)
        + tracker_h->Ki * tracker_h->Ek
        + tracker_h->Kd * (tracker_h->Ek - (tracker_h->Ek1 << 1) + Ek2);
    #endif

    //limit ppm adj max
    if(ppm > ppm_max) ppm = ppm_max;
    else if(ppm < -ppm_max) ppm = -ppm_max;

    tracker_h->ppm = ppm;

    #if BUFF_LVL_WAVE_DBG
    if(usbd_log_flag_get(USBD_LOG_MSK_ASO_STD))
    {
        // ---- debug by sscom wave show
        uint8_t val = (char)tracker_h->Ek;
        extern void uart_send(unsigned char* buf, unsigned int len);
        uart_send((uint8_t*)&val, 1);
    }
    #endif

    return ppm;
}
#endif

#if CONFIG_AUD_SYNC_USBO

#define USBO_LVL_ACC_NUM        100

static AUD_SYNC_USB_t audio_sync_usbo;
static AUD_SYNC_USB_t* p_as_sync_usbo;
static CLK_TRACKER_CTX_t st_clk_tracker_usbo;
#if CONFIG_AUD_SYNC_USBO == 1
static AUD_SPC_CTX_t st_aud_spc_usbo;
#endif

void usbo_aud_sync_reset(void)
{
    AUD_SYNC_USB_t* as_sync_h = p_as_sync_usbo;
    as_sync_h->tracker_h->lvl_cur = 0;
    as_sync_h->tracker_h->ppm = 0;
    as_sync_h->lvl_cur_acc = 0;
    as_sync_h->lvl_cur_cnt = 0;

    audio_spc_init(as_sync_h->spc_h);
}

void usbo_aud_sync_init(void)
{
    USBD_LOG_I("%s, spc_mod:%d\n", __FUNCTION__, CONFIG_AUD_SYNC_USBO);
    p_as_sync_usbo = &audio_sync_usbo;
    AUD_SYNC_USB_t* as_sync_h = p_as_sync_usbo;
    as_sync_h->tracker_h = &st_clk_tracker_usbo;
    #if CONFIG_AUD_SYNC_USBO == 1
    as_sync_h->spc_h = &st_aud_spc_usbo;
    #elif CONFIG_AUD_SYNC_USBO == 2
    as_sync_h->spc_h = (AUD_SPC_CTX_t*)app_dsp_spc_h_get("usbo_aud_sync", 2);
    #endif
    as_sync_h->smps_in = 48;//fixed
    as_sync_h->smps_out = 48;
    as_sync_h->bits = 16;
    as_sync_h->is_src = 1;
    
////clock_tracking_init
    CLK_TRACKER_CTX_t* tracker_h = as_sync_h->tracker_h;
    tracker_h->Kp = 4;
    tracker_h->Ki = 2;
    tracker_h->Kd = 0;
    tracker_h->ppm_max = 300;
    tracker_h->target = USBO_Byte2Smp(AUDIO_USBO_BUF_SIZE) / 2;
    tracker_h->thrd = USBO_Byte2Smp(AUDIO_USBO_BUF_SIZE) / 6;

    usbo_aud_sync_reset();
#if (AUDIO_STREAM_DBG == 1)
    usbd_log_flag_set(USBD_LOG_MSK_ASO, 1);
#endif

#if (BUFF_LVL_WAVE_DBG == 1)
    extern void dsp_log_open(void);
    dsp_log_open(0);
    usbd_log_flag_set(-1, 0);
    usbd_log_flag_set(USBD_LOG_MSK_ASO_STD, 1);
#endif
}


/** @brief usb audio out async & spc process
 * @param lvl_cur ringbuff fill samples
 * @return: comp sample num out
 * */
int usbo_aud_sync_proc(int frame_smps, int lvl_cur)
{
    int comp_num = 0;
    AUD_SYNC_USB_t *as_sync_h = p_as_sync_usbo;
    CLK_TRACKER_CTX_t* tracker_h = p_as_sync_usbo->tracker_h;
#if CONFIG_AUD_SYNC_USBO
    AUD_SPC_CTX_t* spc_h = p_as_sync_usbo->spc_h;
#endif

    as_sync_h->lvl_cur_acc += lvl_cur;
    if(++as_sync_h->lvl_cur_cnt >= USBO_LVL_ACC_NUM)
    {
        tracker_h->lvl_cur = as_sync_h->lvl_cur_acc / USBO_LVL_ACC_NUM;
        as_sync_h->lvl_cur_acc = 0;
        as_sync_h->lvl_cur_cnt = 0;
        clock_tracking_exec(tracker_h);
        as_sync_h->ppm = tracker_h->ppm;

    #if CONFIG_AUD_SYNC_USBO // calc sample point compesate dir & interval
        audio_spc_param_set(spc_h, -tracker_h->ppm);
    #endif
    }

#if CONFIG_AUD_SYNC_USBO == 1
    comp_num = audio_spc_calc(spc_h, frame_smps);
    as_sync_h->smps_out = frame_smps + comp_num;
#else
    UNUSED(comp_num);
    as_sync_h->smps_out = frame_smps;
#endif

#if 1//AUDIO_STREAM_DBG
    if(++as_sync_h->dbg_cnt >= 5000)
    {
        as_sync_h->dbg_cnt = 0;
        USBO_LOG_AS("dir:%d, int:%d, p:%d, n:%d\n", spc_h->adj_dir, spc_h->adj_int_smps, spc_h->adj_p_cnt, spc_h->adj_n_cnt);
        USBO_LOG_AS("obj:%d, spc_mod:%d, ppm:%d, lvl:%d/%d\n", as_sync_h->is_src, CONFIG_AUD_SYNC_USBO, as_sync_h->ppm, tracker_h->lvl_cur, USBO_Byte2Smp(AUDIO_USBO_BUF_SIZE));
        spc_h->adj_p_cnt = spc_h->adj_n_cnt = 0;
    }
#endif
    return comp_num;
}

#endif /* CONFIG_AUD_SYNC_USBO */



#if CONFIG_AUD_SYNC_USBI

#define USBI_LVL_ACC_NUM        100

static AUD_SYNC_USB_t audio_sync_usbi;
static AUD_SYNC_USB_t* p_as_sync_usbi;
static CLK_TRACKER_CTX_t st_clk_tracker_usbi;
#if CONFIG_AUD_SYNC_USBI == 1
static AUD_SPC_CTX_t st_aud_spc_usbi;
#endif


void usbi_aud_sync_reset(void)
{
    AUD_SYNC_USB_t* as_sync_h = p_as_sync_usbi;
    as_sync_h->tracker_h->lvl_cur = 0;
    as_sync_h->tracker_h->ppm = 0;
    as_sync_h->lvl_cur_acc = 0;
    as_sync_h->lvl_cur_cnt = 0;
    audio_spc_init(as_sync_h->spc_h);
}

void usbi_aud_sync_init(void)
{
    USBD_LOG_I("%s, spc_mod:%d\n", __FUNCTION__, CONFIG_AUD_SYNC_USBI);
    p_as_sync_usbi = &audio_sync_usbi;
    AUD_SYNC_USB_t* as_sync_h = p_as_sync_usbi;
    as_sync_h->tracker_h = &st_clk_tracker_usbi;
    #if CONFIG_AUD_SYNC_USBI == 1
    as_sync_h->spc_h = &st_aud_spc_usbi;
    #elif CONFIG_AUD_SYNC_USBI == 2
    as_sync_h->spc_h = (AUD_SPC_CTX_t*)app_dsp_spc_h_get("usbi_aud_sync", 3);
    #endif
    as_sync_h->smps_in = 48;
    as_sync_h->smps_out = 48;//fixed
    as_sync_h->bits = 16;
    as_sync_h->is_src = 0;

////clock_tracking_init
    CLK_TRACKER_CTX_t* tracker_h = as_sync_h->tracker_h;
    tracker_h->Kp = 4;
    tracker_h->Ki = 2;
    tracker_h->Kd = 0;
    tracker_h->ppm_max = 300;
    tracker_h->target = USBI_Byte2Smp(AUDIO_USBI_BUF_SIZE) / 2;
    tracker_h->thrd = USBI_Byte2Smp(AUDIO_USBI_BUF_SIZE) / 6;

    usbi_aud_sync_reset();

#if (AUDIO_STREAM_DBG == 1)
    usbd_log_flag_set(USBD_LOG_MSK_ASI, 1);
#endif
#if (BUFF_LVL_WAVE_DBG == 1)
    extern void dsp_log_open(void);
    dsp_log_open(0);
    usbd_log_flag_set(-1, 0);
    usbd_log_flag_set(USBD_LOG_MSK_ASO_STD, 1);
#endif
}


/** @brief usb audio in async & spc process
 * @param lvl_cur ringbuff fill samples
 * @return: comp sample num out
 * */
// int usbi_aud_sync_proc(int16_t *pcmi, int16_t *pcmo, int smps_in, int lvl_cur)
int usbi_aud_sync_proc(int frame_smps, int lvl_cur)
{
    int comp_num = 0;
    AUD_SYNC_USB_t *as_sync_h = p_as_sync_usbi;
    CLK_TRACKER_CTX_t* tracker_h = as_sync_h->tracker_h;
#if CONFIG_AUD_SYNC_USBI
    AUD_SPC_CTX_t* spc_h = as_sync_h->spc_h;
#endif

    as_sync_h->lvl_cur_acc += lvl_cur;
    if(++as_sync_h->lvl_cur_cnt >= USBI_LVL_ACC_NUM)
    {
        tracker_h->lvl_cur = as_sync_h->lvl_cur_acc / USBI_LVL_ACC_NUM;
        as_sync_h->lvl_cur_acc = 0;
        as_sync_h->lvl_cur_cnt = 0;
        clock_tracking_exec(tracker_h);
        as_sync_h->ppm = -tracker_h->ppm;

    #if CONFIG_AUD_SYNC_USBI // calc sample point compesate dir & interval
        audio_spc_param_set(spc_h, -tracker_h->ppm);
    #endif
    }

#if CONFIG_AUD_SYNC_USBI == 1
    comp_num = audio_spc_calc(spc_h, frame_smps);
    as_sync_h->smps_in = frame_smps - comp_num;//if add pt, read less from rb, then comp 1 pt
#else
    UNUSED(comp_num);
    as_sync_h->smps_in = frame_smps;
#endif

#if 1//(AUDIO_STREAM_DBG == 1)
    if(++as_sync_h->dbg_cnt >= 5000)
    {
        as_sync_h->dbg_cnt = 0;
        USBI_LOG_AS("dir:%d, int:%d, p:%d, n:%d\n", spc_h->adj_dir, spc_h->adj_int_smps, spc_h->adj_p_cnt, spc_h->adj_n_cnt);
        USBI_LOG_AS("obj:%d, spc_mod:%d, ppm:%d, lvl:%d/%d\n", as_sync_h->is_src, CONFIG_AUD_SYNC_USBI, as_sync_h->ppm, tracker_h->lvl_cur, USBI_Byte2Smp(AUDIO_USBI_BUF_SIZE));
        spc_h->adj_p_cnt = spc_h->adj_n_cnt = 0;
    }
#endif
    return comp_num;
}

#endif /* CONFIG_AUD_SYNC_USBI */
