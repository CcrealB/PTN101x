/**
 * **************************************************************************************
 * @file    uac_ctrl.c
 * 
 * @author  Borg Xiao
 * @date    20230524
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include "config.h"
#include "timer.h"
#include "sys_irq.h"
#include "utils_audio.h"
#include "audio_hid.h"
#include "usb\usb_include.h"
#include "usb\usb_cls_audio.h"

// #define UAC_CTRL_DEBUG

#ifdef UAC_CTRL_DEBUG
#define FU_GET_LOG_D(fmt,...)       std_printf("[UAC]"fmt, ##__VA_ARGS__)
#define FU_SET_LOG_D(fmt,...)       std_printf("[UAC]"fmt, ##__VA_ARGS__)
#else
#define FU_GET_LOG_D(fmt,...)
#define FU_SET_LOG_D(fmt,...)
#endif


/* --------------------------------------------------- */
/*usb audio class feature unit volume config
  min,max,ini range:-127 ~ +127, and have relation with min <= ini <= max
*/
//usb in 
#define UAC_I_FU_VOL_dBres      (0.5f)
#define UAC_I_FU_VOL_dBmin      (-64.0f)
#define UAC_I_FU_VOL_dBmax      (0.0f)
#define UAC_I_FU_VOL_dBini      (0.0f)

//usb out
#define UAC_O_FU_VOL_dBres      (0.5f)
#define UAC_O_FU_VOL_dBmin      (-64.0f)
#define UAC_O_FU_VOL_dBmax      (0.0f)
#define UAC_O_FU_VOL_dBini      (0.0f)

#define UAC_I_VOL_dB2Code(dB)       ((short)(dB * 256))
#define UAC_O_VOL_dB2Code(dB)       ((short)(dB * 256))
#define UAC_I_VOL_Code2dB(Code)     ((float)(Code / ((float)256)))
#define UAC_O_VOL_Code2dB(Code)     ((float)(Code / ((float)256)))

typedef volatile struct _UAC_VOLUME_CTX_t{
    uint8_t in_mute;
    float in_vol_dB_L;
    float in_vol_dB_R;
    uint8_t out_mute;
    float out_vol_dB_L;
    float out_vol_dB_R;
}UAC_VOLUME_CTX_t;

static UAC_VOLUME_CTX_t s_uac_vol_fu = {
    .in_mute = 0,
    .in_vol_dB_L = UAC_I_FU_VOL_dBini,
    .in_vol_dB_R = UAC_I_FU_VOL_dBini,
    .out_mute = 0,
    .out_vol_dB_L = UAC_O_FU_VOL_dBini,
    .out_vol_dB_R = UAC_O_FU_VOL_dBini,
};

static UAC_VOLUME_CTX_t* p_uac_vol_fu = &s_uac_vol_fu;

/// @brief 
/// @param setup[in] 
/// @param pbuf[in] 
/// @param size[out] 
/// @return -1:Not Supported
int uac_get_featrue_unit(USBD_SetupReqTypedef *setup, uint8_t *pbuf, int *size)
{
    int ret = 0;
    UAC_VOLUME_CTX_t* unit = p_uac_vol_fu;

    uint8_t unit_id = MUSB_MSB(setup->wIndex);
    uint8_t ctrl_sel = MUSB_MSB(setup->wValue);
    uint8_t chnl_num = MUSB_LSB(setup->wValue);

    usb_dbg_setup_show(__func__, setup, 8);

    switch (unit_id)
    {
    case USBAUDIO_FU_MIC:
    {
        if(ctrl_sel == AUDIO_ACC_FU_CS_MUTE)
        {
            uint8_t bMute = 0;
            if(setup->bRequest == AUDIO_RC_GET_CUR) {
                bMute = unit->in_mute;
                FU_GET_LOG_D("\tmic GET_CUR -- ch:%d, mute:%X\n", chnl_num, bMute);
            } else {
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
            }
            *((uint8_t*)pbuf) = bMute;
            *size = 1;
        }
        else if(ctrl_sel == AUDIO_ACC_FU_CS_VOLUMN)
        {
            int16_t wVolume = 0;
            float vol_dB = 0;
            switch (setup->bRequest)
            {
            case AUDIO_RC_GET_CUR:
                if(chnl_num == 1) { vol_dB = unit->in_vol_dB_L; }
                else if(chnl_num == 2) { vol_dB = unit->in_vol_dB_R; }
                break;
            case AUDIO_RC_GET_MIN:
                vol_dB = UAC_I_FU_VOL_dBmin;
                break;
            case AUDIO_RC_GET_MAX:
                vol_dB = UAC_I_FU_VOL_dBmax;
                break;
            case AUDIO_RC_GET_RES:
                vol_dB = UAC_I_FU_VOL_dBres;
                break;
            default:
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
                break;
            }
            wVolume = UAC_I_VOL_dB2Code(vol_dB);
            *((int16_t*)pbuf) = wVolume;
            *size = 2;
            FU_GET_LOG_D("micGET 0x%02X, ch:%d, vol_dB:%.1f(0x%X)\n", setup->bRequest, chnl_num, vol_dB, wVolume);
        }
        else
        {
            ret = -1;
        }
    }break;
    case USBAUDIO_FU_SPK:
    {
        if(ctrl_sel == AUDIO_ACC_FU_CS_MUTE)
        {
            uint8_t bMute = 0;
            if(setup->bRequest == AUDIO_RC_GET_CUR) {
                bMute = unit->out_mute;
                FU_GET_LOG_D("\tspk GET_CUR -- ch:%d, mute:%X\n", chnl_num, bMute);
            } else {
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
            }
            *((uint8_t*)pbuf) = bMute;
            *size = 1;
        }
        else if(ctrl_sel == AUDIO_ACC_FU_CS_VOLUMN)
        {
            int16_t wVolume = 0;
            float vol_dB = 0;
            switch (setup->bRequest)
            {
            case AUDIO_RC_GET_CUR:
                if(chnl_num == 1) { vol_dB = unit->out_vol_dB_L; }
                else if(chnl_num == 2) { vol_dB = unit->out_vol_dB_R; }
                break;
            case AUDIO_RC_GET_MIN:
                vol_dB = UAC_O_FU_VOL_dBmin;
                break;
            case AUDIO_RC_GET_MAX:
                vol_dB = UAC_O_FU_VOL_dBmax;
                break;
            case AUDIO_RC_GET_RES:
                vol_dB = UAC_O_FU_VOL_dBres;
                break;
            default:
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
                break;
            }
            wVolume = UAC_O_VOL_dB2Code(vol_dB);
            *((int16_t*)pbuf) = wVolume;
            *size = 2;
            FU_GET_LOG_D("spkGET 0x%02X, ch:%d, vol_dB:%.1f(0x%X)\n", setup->bRequest, chnl_num, vol_dB, wVolume);
        }
        else
        {
            ret = -1;
        }
    }
    break;
    default: ret = -1; break;
    }
    return ret;
}

/// @brief 
/// @param setup 
/// @param pbuf 
/// @param size 
/// @return -1:Not Supported
int uac_set_feature_unit(USBD_SetupReqTypedef *setup, uint8_t *pbuf, int size)
{
    int ret = 0;
    UAC_VOLUME_CTX_t* unit = p_uac_vol_fu;

    uint8_t unit_id = MUSB_MSB(setup->wIndex);
    uint8_t ctrl_sel = MUSB_MSB(setup->wValue);
    uint8_t chnl_num = MUSB_LSB(setup->wValue);//1:left, 2:right

    switch (unit_id)
    {
    case USBAUDIO_FU_MIC:
    {
        if(ctrl_sel == AUDIO_ACC_FU_CS_MUTE)
        {
            uint8_t bMute = *((uint8_t*)pbuf);
            if(setup->bRequest == AUDIO_RC_SET_CUR) {
                unit->in_mute = bMute;
                FU_SET_LOG_D("\tmic SET_CUR -- ch:%d, mute:%X\n", chnl_num, bMute);
            } else {
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
            }
        }
        else if(ctrl_sel == AUDIO_ACC_FU_CS_VOLUMN)
        {
            int16_t wVolume = *((int16_t*)pbuf);
            if (setup->bRequest == AUDIO_RC_SET_CUR){
                float vol_dB = UAC_I_VOL_Code2dB(wVolume);
                if(chnl_num == 1) { unit->in_vol_dB_L = vol_dB; }
                else if(chnl_num == 2) { unit->in_vol_dB_R = vol_dB; }
                FU_SET_LOG_D("\tmicSET_CUR -- ch:%d, vol_dB:%.1f(0x%X)\n", chnl_num, vol_dB, wVolume);
            } else {
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }
    }
    break;
    case USBAUDIO_FU_SPK:
    {
        if(ctrl_sel == AUDIO_ACC_FU_CS_MUTE)
        {
            uint8_t bMute = *((uint8_t*)pbuf);
            if(setup->bRequest == AUDIO_RC_SET_CUR) {
                FU_SET_LOG_D("\tspkSET_CUR -- ch:%d, mute:%X\n", chnl_num, bMute);
                unit->out_mute = bMute;
            } else {
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
            }
        }
        else if(ctrl_sel == AUDIO_ACC_FU_CS_VOLUMN)
        {
            int16_t wVolume = *((int16_t*)pbuf);
            if (setup->bRequest == AUDIO_RC_SET_CUR){
                float vol_dB = UAC_O_VOL_Code2dB(wVolume);
                if(chnl_num == 1) { unit->out_vol_dB_L = vol_dB; }
                else if(chnl_num == 2) { unit->out_vol_dB_R = vol_dB; }
                FU_SET_LOG_D("\tspkSET_CUR -- ch:%d, vol_dB:%.1f(0x%X)\n", chnl_num, vol_dB, wVolume);
            } else {
                USBD_LOG_W("0x%02X\n", setup->bRequest);
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }
    }
    break;
    default: ret = -1; break;
    }
    return ret;
}

//protect global var
static inline float global_dB_read(volatile float *vol_dB)
{
    uint32_t interrupts_info, mask;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    float vol = *vol_dB;
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    return vol;
}

/* --------------------------------------- *///usb volume api
//dB
float usb_in_vol_dB_max_get(void) { return UAC_I_FU_VOL_dBmax; }
float usb_in_vol_dB_min_get(void) { return UAC_I_FU_VOL_dBmin; }
float usb_in_vol_dB_res_get(void) { return UAC_I_FU_VOL_dBres; }
float usb_out_vol_dB_max_get(void) { return UAC_O_FU_VOL_dBmax; }
float usb_out_vol_dB_min_get(void) { return UAC_O_FU_VOL_dBmin; }
float usb_out_vol_dB_res_get(void) { return UAC_O_FU_VOL_dBres; }

float usb_in_vol_dB_L_get(void) {return global_dB_read(&p_uac_vol_fu->in_vol_dB_L); }
float usb_in_vol_dB_R_get(void) { return global_dB_read(&p_uac_vol_fu->in_vol_dB_R); }
float usb_out_vol_dB_L_get(void) { return global_dB_read(&p_uac_vol_fu->out_vol_dB_L); }
float usb_out_vol_dB_R_get(void) { return global_dB_read(&p_uac_vol_fu->out_vol_dB_R); }

//idx
#define UAC_I_FU_IDX_CLS        32
#define UAC_O_FU_IDX_CLS        32
#define UAC_I_FU_dB_PRE_CLS     ((uint8_t)((UAC_I_FU_VOL_dBmax - UAC_I_FU_VOL_dBmin) / UAC_I_FU_IDX_CLS))
#define UAC_O_FU_dB_PRE_CLS     ((uint8_t)((UAC_O_FU_VOL_dBmax - UAC_O_FU_VOL_dBmin) / UAC_O_FU_IDX_CLS))

#define UAC_I_VOL_dB2idx(dB)    ((uint8_t)((uint8_t)(dB - UAC_I_FU_VOL_dBmin) / UAC_I_FU_dB_PRE_CLS))
#define UAC_O_VOL_dB2idx(dB)    ((uint8_t)((uint8_t)(dB - UAC_O_FU_VOL_dBmin) / UAC_O_FU_dB_PRE_CLS))
// #define UAC_I_VOL_idx2dB(idx)   ((float)(UAC_I_FU_VOL_dBmin + (idx * UAC_I_FU_dB_PRE_CLS)))
// #define UAC_O_VOL_idx2dB(idx)   ((float)(UAC_O_FU_VOL_dBmin + (idx * UAC_O_FU_dB_PRE_CLS)))

uint8_t usb_in_vol_idx_max_get(void) { return UAC_I_VOL_dB2idx(UAC_I_FU_VOL_dBmax); }
uint8_t usb_in_vol_idx_L_get(void) { return UAC_I_VOL_dB2idx(global_dB_read(&p_uac_vol_fu->in_vol_dB_L)); }
uint8_t usb_in_vol_idx_R_get(void) { return UAC_I_VOL_dB2idx(global_dB_read(&p_uac_vol_fu->in_vol_dB_R)); }

uint8_t usb_out_vol_idx_max_get(void) { return UAC_O_VOL_dB2idx(UAC_O_FU_VOL_dBmax); }
uint8_t usb_out_vol_idx_L_get(void) { return UAC_O_VOL_dB2idx(global_dB_read(&p_uac_vol_fu->out_vol_dB_L)); }
uint8_t usb_out_vol_idx_R_get(void) { return UAC_O_VOL_dB2idx(global_dB_read(&p_uac_vol_fu->out_vol_dB_R)); }



/* --------------------------------------- *///usb volume proc
#if 0
static inline void uac_fu_volume_proc(int16_t *pcmi, int16_t *pcmo, int samples, 
    uint8_t mute, int8_t vol_dB_L, int8_t vol_dB_R)
{
    if(mute){
        audio_volume_mute_16bit2ch(pcmi, pcmo, samples);
    }else{
        audio_volume_dB_16bit2ch(pcmi, pcmo, vol_dB_L, vol_dB_R, samples);
    }
}
#endif
void usb_in_volume_proc(int16_t *pcmi, int16_t *pcmo, int samples)
{
    // memmove(pcmo, pcmi, USBO_Smp2Byte(samples));
    UAC_VOLUME_CTX_t* unit = p_uac_vol_fu;
    if(unit->in_mute){
        audio_volume_mute_16bit2ch(pcmi, pcmo, samples);
    }else{
    #ifndef USB_IN_VOL_AT_DSP
        int8_t hdB_L = (int8_t)unit->in_vol_dB_L << 1;
        int8_t hdB_R = (int8_t)unit->in_vol_dB_R << 1;
        audio_volume_hdB_16bit2ch(pcmi, pcmo, hdB_L, hdB_R, samples);
    #endif
    }
}

void usb_out_volume_proc(int16_t *pcmi, int16_t *pcmo, int samples)
{
    // memmove(pcmo, pcmi, USBO_Smp2Byte(samples));
    UAC_VOLUME_CTX_t* unit = p_uac_vol_fu;
    if(unit->out_mute){
        audio_volume_mute_16bit2ch(pcmi, pcmo, samples);
    }else{
    #ifndef USB_OUT_VOL_AT_DSP
        int8_t hdB_L = (int8_t)unit->out_vol_dB_L << 1;
        int8_t hdB_R = (int8_t)unit->out_vol_dB_R << 1;
        audio_volume_hdB_16bit2ch(pcmi, pcmo, hdB_L, hdB_R, samples);
    #endif
    }
}


/* --------------------------------------- *///usb audio ctrl
#if 1
enum{
    ST_UAC_O_VOL_IDLE = 0,
    ST_UAC_O_VOL_MUTE,
    ST_UAC_O_VOL_P,
    ST_UAC_O_VOL_N,
    ST_UAC_O_VOL_END,
};
static uint8_t s_vol_adj_state = 0;
// static uint32_t s_volume_timer_mark = 0;
void usb_volume_schedule(void)
{
    // if(!sys_timeout(s_volume_timer_mark, 10)) return;
    // s_volume_timer_mark = sys_time_get();

    if(s_vol_adj_state == ST_UAC_O_VOL_IDLE) return;

    // USBD_LOG_I("s_vol_adj_state:%d\n", s_vol_adj_state);
    switch (s_vol_adj_state)
    {
    case ST_UAC_O_VOL_MUTE  : csm_hid_key_send(0, CSM_MUTE);   s_vol_adj_state = ST_UAC_O_VOL_END; break;
    case ST_UAC_O_VOL_P     : csm_hid_key_send(0, CSM_VOL_P);  s_vol_adj_state = ST_UAC_O_VOL_END; break;
    case ST_UAC_O_VOL_N     : csm_hid_key_send(0, CSM_VOL_N);  s_vol_adj_state = ST_UAC_O_VOL_END; break;
    case ST_UAC_O_VOL_END   : csm_hid_key_send(0, CSM_HID_END);s_vol_adj_state = ST_UAC_O_VOL_IDLE;break;
    default: break;
    }
}

// uint8_t usb_out_vol_adj_get(void) { return s_vol_adj_state; }
static void usb_out_vol_adj_set(uint8_t st) { s_vol_adj_state = st; }
void usb_out_vol_mute(void) { usb_out_vol_adj_set(ST_UAC_O_VOL_MUTE); }
void usb_out_vol_inc(void) {  usb_out_vol_adj_set(ST_UAC_O_VOL_P); }
void usb_out_vol_dec(void) {  usb_out_vol_adj_set(ST_UAC_O_VOL_N); }
void usb_out_vol_end(void) {  usb_out_vol_adj_set(ST_UAC_O_VOL_END); }
#else
void usb_volume_schedule(void) {}
void usb_out_vol_mute(void) { csm_hid_key_send(0, CSM_MUTE);  }
void usb_out_vol_inc(void) { csm_hid_key_send(0, CSM_VOL_P);   }
void usb_out_vol_dec(void) { csm_hid_key_send(0, CSM_VOL_N);   }
void usb_out_vol_end(void) { csm_hid_key_send(0, CSM_HID_END); }
#endif
void usb_out_audio_play_pause(void) { csm_hid_key_send(0, CSM_PLAY); }
void usb_out_audio_stop(void) { csm_hid_key_send(0, CSM_STOP); }
void usb_out_audio_next(void) { csm_hid_key_send(0, CSM_NEXT); }
void usb_out_audio_prev(void) { csm_hid_key_send(0, CSM_PREV); }
