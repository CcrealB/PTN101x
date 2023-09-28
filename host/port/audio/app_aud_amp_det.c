/**
 * **************************************************************************************
 * @file    app_aud_amp_det.c
 * 
 * @author  Borg Xiao
 * @date    20230617 for lib
 * @date    20230612
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * 
 * samples get in rec isr, fft in main loop.
 * as : dsp ---- mailbox ---- mcu ---- LP ---- samples get ---- FFT ---- result
 * 
 * how to use:(for AUD_DET_FS == 16000)
    //0: freq_step * 1 = 62.5 Hz 
    //1: freq_step * 7 = 437.5 Hz
    //2: freq_step * 16 = 1000 Hz
    //3: freq_step * 40 = 2500 Hz 
    //4: freq_step * 64 = 4000 Hz
    //5: freq_step * 96 = 6000 Hz
    //6: freq_step * 128 = 8000 Hz
    #define AUD_DET_FREQ_IDX_TAB    {1, 7, 16, 40, 64, 100, 128 }   //demo 7 freq
    #define AUD_DET_FREQ_IDX_TAB    {1, 7, 40 }                     //demo 3 freq
 * **************************************************************************************
 * */
// #include "config.h"
#if 1//def CONFIG_AUD_AMP_DET_FFT

#include <math.h>
#include <stdint.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>

// #include "port_mem.h"
// #include "os_common.h"
// #include "sys_irq.h"

// #include "utils_audio.h"
#include "fft_ba22.h"
#include "app_aud_amp_det.h"
/* ------------------------------------------------------------ */ //config
#define AUD_DET_FS              16000
#define AUD_DET_FFT_FRA_SMPS    256// 128/256, don't over 256
#define AUD_DET_FREQ_STEP       ((float)AUD_DET_FS / AUD_DET_FFT_FRA_SMPS)
#define AUD_DET_BUF_MALLOC      1
#define AUD_DET_PCM_L_plus_R

#if AUD_DET_FS == 48000
    #define AUD_DET_INTERVALms      5  //update interval(unit:ms)

    // config freq index, freq = freq_step * index, freq_step = (1 * 48K / 256) = 187.5Hz.
    /* freq_step * 1 = 187.5 Hz */
    /* freq_step * 2 = 375 Hz */
    /* freq_step * 13 = 2437.5 Hz */
    #define AUD_DET_FREQ_IDX_TAB    {1, 2, 13}
#elif AUD_DET_FS == 16000
    #define AUD_DET_INTERVALms      16  //update interval(unit:ms)
    // config freq index, freq = freq_step * index, freq_step = (1 * 16K / 256) = 62.5Hz.
//	63Hz	160Hz 	400Hz 	1000Hz	2500Hz	6250Hz	16000Hz
//	62.5	375		437.5	2500	6000
    #define AUD_DET_FREQ_IDX_TAB    {1, 6, 7, 40, 96}
    // src audio from 48K to 16k(need change UTILS_AUD_SRC_SEL to UTILS_AUD_SRC in src.h if enable)
    #define AUD_DET_PCM_SRC_EN      0 //don't use, resample down to 16KHz for FFT freq resolution
    #define AUD_DET_LP_ENABLE       1 //1:mono, 2:stereo, low pass, function mutex with AUD_DET_PCM_SRC_EN
#endif

/* ------------------------------------------------------------ */ //debug
#define AUD_DET_PRINTF              os_printf
#define AUD_DET_LOG_E(fmt,...)      AUD_DET_PRINTF("[DET|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define AUD_DET_LOG_W(fmt,...)      AUD_DET_PRINTF("[DET|W:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define AUD_DET_LOG_I(fmt,...)      AUD_DET_PRINTF("[DET|I]"fmt, ##__VA_ARGS__)
#define AUD_DET_LOG_F(fmt,...)      AUD_DET_PRINTF("[DET|F:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

//debug
//#define AUDIO_DET_FFT_TEST
// #define AUD_DET_AS_PROC_TIME_DBG

#ifdef AUD_DET_AS_PROC_TIME_DBG
    #define AUD_DET_AS_PROC_RUN(en)     REG_GPIO_0x0D = en ? 2 : 0
#else
    #define AUD_DET_AS_PROC_RUN(en)
#endif
/* ------------------------------------------------------------ */ //macro
#define STATIC_CONST                static const __attribute__((section(".rodata")))
#define AUD_DET_MAX_FRA_SMPS        256 //the max samples could be processd by func
#define EVT_AUD_DET_START           (1 << 0)
#define EVT_AUD_DET_STOP            (1 << 1)

/* ------------------------------------------------------------ */ //var
#if AUD_DET_BUF_MALLOC == 0
static int16_t fft_pcm_buff[AUD_DET_FFT_FRA_SMPS];
#else
#define as_det_malloc(size)     jmalloc_s(size, 1/*M_ZERO*/)
#define as_det_free(p)          jfree_s(p)
#endif
static int16_t *p_fft_pcm = NULL;
static uint8_t pcm_wp = 0; //AUD_DET_FFT_FRA_SMPS must <= 256 for uint8_t type
static uint8_t aud_mag_det_event = 0;

static const uint8_t sc_det_freq_tab[] = AUD_DET_FREQ_IDX_TAB;
#define AUD_DET_FREQ_NUM    sizeof(sc_det_freq_tab) / sizeof(sc_det_freq_tab[0])
static int16_t s_det_mag_tab[AUD_DET_FREQ_NUM];

#if AUD_DET_PCM_SRC_EN
#include "api_src.h"
aud_src_ctx_t aud_det_src_ctx = {
    .fs_in = 48000,
    .fs_out = 16000,
    .bits_in = 16,
    .bits_out = 16,
    .chs = 1,
    .max_smps_in = 192,
    .max_fra_sz = 192,
};
aud_src_ctx_t *p_aud_det_src = &aud_det_src_ctx;
#endif

/* ------------------------------------------------------------ */ //func

#if AUD_DET_LP_ENABLE
static inline int32_t SSAT24(int32_t value)
{
    #if   defined(__BA2__)
    int32_t result;
    __asm("b.lim %0,%1,%2;" : "=r" (result) : "r" (value), "r" (8388607) : );
    return result;
    #elif defined(CEVAX2)
    return _lim(sat24, (int)value);
    #else
    return (value > 8388607) ? 8388607 : ((value < -8388607) ? -8388607 : value);
    #endif
}

#define TotalEQnum  1        //EQ_COEF { -a0, -a1, b0, b1, b2 }
#define EQ0_COEF { -(-650293), -(252011), 162573, 325146, 162573 } //8KHz lowpass
// #define EQ0_COEF { -(-1341738), -(500730), 51891, 103783, 51891 } //4KHz lowpass
// #define EQ0_COEF { -(-1903498), -(871347), 4106, 8212, 4106 } //1KHz low pass, for test

// extern SAMPLE_ALIGN uint32_t s_eq_last_XY[]; int64_t* s_eq_lastXY = (int64_t*)s_eq_last_XY;
static uint64_t s_eq_lastXY[TotalEQnum][4]__attribute__((aligned(4)));

#if AUD_DET_LP_ENABLE == 1
static void aud_det_eq_low_pass_1ch(int32_t *pcm, int samples)
{
    int32_t eq_coef[][5] = { EQ0_COEF, EQ0_COEF, EQ0_COEF, EQ0_COEF, EQ0_COEF };//refer to aud_pre_equ_para_t
    // REG_GPIO_0x0E = 2;
    extern void func_sw_eq_mono(int32_t* eq, int64_t* delays, int32_t* pcm, uint32_t samples, uint32_t filters);
    func_sw_eq_mono((int32_t*)eq_coef, (int64_t*)s_eq_lastXY, (int32_t*)pcm, samples, TotalEQnum);
    int i; for(i = 0; i < samples; i++) pcm[i] = SSAT24(pcm[i]);
    // REG_GPIO_0x0E = 0;
}
#elif AUD_DET_LP_ENABLE == 2
static void aud_det_eq_low_pass_2ch(int32_t *pcm, int samples)
{
    int32_t eq_coef[][5] = { EQ0_COEF, EQ0_COEF, EQ0_COEF, EQ0_COEF, EQ0_COEF };//refer to aud_pre_equ_para_t
    // REG_GPIO_0x0E = 2;
    #if 0//5阶1KHz低通滤波，48pt*2ch处理耗时 86us
    // extern SAMPLE_ALIGN aud_pre_equ_para_t tbl_eq_coef[];
    extern SAMPLE_ALIGN app_eq_para_t tbl_eq_coef[];
    extern void func_sw_eq(int *input,int *output,int len,int eq_cnt,uint32_t index);//len: 1=L+R=4bytes
    memcpy(tbl_eq_coef, eq_coef, sizeof(eq_coef));
    func_sw_eq((int*)pcm, (int*)pcm, samples, TotalEQnum, 0);
    #else//5阶1KHz低通滤波，48pt*2ch处理耗时 185.3us
    extern void func_sw_eq2(int32_t* eq, int64_t* delays, int32_t* pcm, uint32_t samples, uint32_t filters);
    func_sw_eq2((int32_t*)eq_coef, (int64_t*)s_eq_lastXY, (int32_t*)pcm, samples, TotalEQnum);
    #endif
    int i; for(i = 0; i < samples * 2; i++) pcm[i] = SSAT24(pcm[i]);
    // REG_GPIO_0x0E = 0;
}
#endif 
/*//test, note:don't call aud_det_eq_low_pass_1ch/2ch in other function when in test mode
//#include "utils_audio.h"
void aud_det_eq_low_pass_16b2ch(int16_t *pcm, int samples)
{
#if AUD_DET_LP_ENABLE == 1
    int i;
    int32_t pData[192];
    for (i = samples - 1; i >= 0; i--) pData[i] = (pcm[2*i]) << 8;//aud_cvt_pcm16Left_to_1chpcm24
    // for (i = samples - 1; i >= 0; i--) pData[i] = (pcm[2*i] + pcm[2*i + 1]) << 8;//aud_cvt_pcm16LaddR_to_1chpcm24
    aud_det_eq_low_pass_1ch(pData, samples);
    aud_cvt_pcm24_to_pcm16((int32_t*)pData, (int16_t*)pData, samples);
    aud_cvt_mono_to_stereo_pcm16((int16_t*)pData, (int16_t*)pcm, samples);
#elif AUD_DET_LP_ENABLE == 2
    int32_t pData[192 * 2];
    aud_cvt_pcm16_to_pcm24((int16_t*)pcm, (int32_t*)pData, samples * 2);
    aud_det_eq_low_pass_2ch(pData, samples);
    aud_cvt_pcm24_to_pcm16((int32_t*)pData, (int16_t*)pcm, samples * 2);
#endif
}//test*/
#endif //AUD_DET_LP_ENABLE

//audio data update, called in audio stream
//[note:must meet samples <= 256], refer AUD_DET_MAX_FRA_SMPS
void aud_d2m_pcm16_LR_for_fft(int16_t *pcm, int samples)
{
    int i;
    if(p_fft_pcm == NULL) return;
    AUD_DET_AS_PROC_RUN(1);
#if AUD_DET_FS == 48000
    for(i = 0; i < (samples * 2); i += 2){// 48KHz
        p_fft_pcm[pcm_wp++ & (AUD_DET_FFT_FRA_SMPS - 1)] = pcm[i] >> 8;//left channel
    }
#elif AUD_DET_FS == 16000
    #if AUD_DET_PCM_SRC_EN
    int16_t aud_fra[192];
    for(i = 0; i < samples; i++){ aud_fra[i] = pcm[2*i]; }//2ch -> 1ch left channel
    audio_src_apply(p_aud_det_src, aud_fra, aud_fra, samples);// down to 16KHz//about 327.0us
    for(i = 0; i < samples / 3; i++){
        p_fft_pcm[pcm_wp++ & (AUD_DET_FFT_FRA_SMPS - 1)] = aud_fra[i] >> 8;
    }
    #elif AUD_DET_LP_ENABLE == 1//mono, run~110us@ba22@150MHz
    int32_t pData[AUD_DET_MAX_FRA_SMPS];
    #ifdef AUD_DET_PCM_L_plus_R
    for (i = samples - 1; i >= 0; i--) pData[i] = (pcm[2*i] + pcm[2*i + 1]) << 8;//pcm16(L+R)_to_pcm24mono
    #else
    for (i = samples - 1; i >= 0; i--) pData[i] = (pcm[2*i]) << 8;//pcm16(L)_to_pcm24mono
    #endif
    aud_det_eq_low_pass_1ch(pData, samples);
    // aud_cvt_pcm24_to_pcm16((int32_t*)pData, (int16_t*)pData, samples);
    for(i = 0; i < samples; i += 3){// down to 16KHz
        p_fft_pcm[pcm_wp++ & (AUD_DET_FFT_FRA_SMPS - 1)] = pData[i] >> (8 + 8);//8 for FFT, 8 for pcm24_2_pcm16
    }
    #elif AUD_DET_LP_ENABLE == 2
    int32_t pData[AUD_DET_MAX_FRA_SMPS * 2];
    aud_cvt_pcm16_to_pcm24((int16_t*)pcm, (int32_t*)pData, samples * 2);
    aud_det_eq_low_pass_2ch(pData, samples);
    aud_cvt_pcm24_to_pcm16((int32_t*)pData, (int16_t*)pData, samples * 2);
    pcm = (int16_t*)pData;
    for(i = 0; i < (samples * 2); i += 6){// down to 16KHz
        p_fft_pcm[pcm_wp++ & (AUD_DET_FFT_FRA_SMPS - 1)] = pcm[i] >> 8;//left channel
    }
    #else
    for(i = 0; i < (samples * 2); i += 6){// down to 16KHz
        p_fft_pcm[pcm_wp++ & (AUD_DET_FFT_FRA_SMPS - 1)] = pcm[i] >> 8;//left channel
    }
    #endif
#endif
    AUD_DET_AS_PROC_RUN(0);
}

static void audio_det_fft_init(uint8_t en)
{
    if(en)
    {
        if(p_fft_pcm == NULL)
        {
        #if AUD_DET_PCM_SRC_EN
            p_aud_det_src = &aud_det_src_ctx;
            audio_src_init(p_aud_det_src);
        #endif
        #if AUD_DET_BUF_MALLOC
            p_fft_pcm = (int16_t*)as_det_malloc(AUD_DET_FFT_FRA_SMPS * sizeof(int16_t));
        #else
            p_fft_pcm = (int16_t*)&fft_pcm_buff;
        #endif
        }
    }
    else
    {
    #if AUD_DET_BUF_MALLOC
        if(p_fft_pcm) as_det_free(p_fft_pcm);
    #endif
        p_fft_pcm = NULL;
    #if AUD_DET_PCM_SRC_EN
        audio_src_uninit(p_aud_det_src);
    #endif
    }
    AUD_DET_LOG_I("%s(%d)\n", __FUNCTION__, en);
    dbg_show_dynamic_mem_info(1);
}

static void audio_det_fft_proc(void)
{
    int fft_len = AUD_DET_FFT_FRA_SMPS;
    int16_t real[AUD_DET_FFT_FRA_SMPS];
    int16_t imag[AUD_DET_FFT_FRA_SMPS] = {0};
    uint32_t interrupts_info, mask;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
//// get audio pcm data
    //[start--------wp----end], copy old data in fft_pcm_buff[wp ~ end]
    memcpy(&real[0], p_fft_pcm + pcm_wp, (fft_len - pcm_wp) << 1); 
    //[start--------wp----end], copy new data in fft_pcm_buff[start ~ wp]
    memcpy(&real[fft_len - pcm_wp], p_fft_pcm, pcm_wp << 1);
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    ba22_fft(real, imag, fft_len, 0);
//// calc magnitude
    for(int i = 0; i < AUD_DET_FREQ_NUM; i++){
        s_det_mag_tab[i] = ba22_cmplx_mag_q15_single(real, imag, sc_det_freq_tab[i]);
    }
}

void audio_det_fft_task(unsigned int (*cbk_systime_get)(void))
{
#ifdef AUDIO_DET_FFT_TEST
    extern void aud_det_fft_dbg_show(unsigned int (*cbk_systime_get)(void));
    aud_det_fft_dbg_show(cbk_systime_get);
#endif

    static uint32_t t_ref = 0;
    if((cbk_systime_get() - t_ref) < AUD_DET_INTERVALms) return;
    t_ref = cbk_systime_get();

//// event proc
    if(aud_mag_det_event)
    {
        if(aud_mag_det_event & EVT_AUD_DET_STOP) {
            aud_mag_det_event &= ~EVT_AUD_DET_STOP;
            audio_det_fft_init(0);
        }else if(aud_mag_det_event & EVT_AUD_DET_START) {
            aud_mag_det_event &= ~EVT_AUD_DET_START;
            audio_det_fft_init(1);
        } 
    }

//// fft proc
    if(p_fft_pcm == NULL) return;

    audio_det_fft_proc();
}

uint8_t aud_mag_det_enable_get(void) { return (p_fft_pcm != NULL); }
void aud_mag_det_enable(uint8_t en)
{
    // if(aud_mag_det_event){
    //     AUD_DET_LOG_W("evt:0x%X", aud_mag_det_event);
    //     return;
    // }
    if(en){
        aud_mag_det_event = EVT_AUD_DET_START;
    }else{
        aud_mag_det_event = EVT_AUD_DET_STOP;
    }
}

int16_t aud_mag_get_freq(uint8_t freq_idx) { return freq_idx < AUD_DET_FREQ_NUM ? s_det_mag_tab[freq_idx] : 0; }
int16_t aud_mag_get_freq_low(void){ return s_det_mag_tab[0]; }
int16_t aud_mag_get_freq_mid(void)
{
    return ((s_det_mag_tab[1] > s_det_mag_tab[2]) ? s_det_mag_tab[1] : s_det_mag_tab[2]);
}
int16_t aud_mag_get_freq_high(void)
{
    return ((s_det_mag_tab[3] > s_det_mag_tab[4]) ? s_det_mag_tab[3] : s_det_mag_tab[4]);
}
#ifdef AUDIO_DET_FFT_TEST
//max value, 0dB~9500, -10dB~7600, -20dB~4900(for fft_ba22 when audio 16bit pcm data >> 8)
#define AMP_THRD_0dB        9500 //windows volume 100%
#define AMP_THRD_n10dB      7600 //windows volume 50%
#define AMP_THRD_n20dB      4900 //windows volume 25%
#define AMP_THRD            AMP_THRD_n20dB
#define PK_MIN_Tms          200
void aud_det_fft_dbg_show(unsigned int (*cbk_systime_get)(void))
{
    static uint32_t t_ref = 0;
    if((cbk_systime_get() - t_ref) < 500) return;
    t_ref = cbk_systime_get();

#if 1
    AUD_DET_LOG_I("Step:%.1fHz, f_num:%d\n", AUD_DET_FREQ_STEP, AUD_DET_FREQ_NUM);
    for(int i = 0; i < AUD_DET_FREQ_NUM; i++){
        AUD_DET_PRINTF("\t%.1fHz:\t%d\n", sc_det_freq_tab[i] * AUD_DET_FREQ_STEP , s_det_mag_tab[i]);
    }
#else
    int mag_freq_low = aud_mag_get_freq_low();
    int mag_freq_mid = aud_mag_get_freq_mid();
    int mag_freq_high = aud_mag_get_freq_high();
    static uint32_t t_ref_L = 0;
    static uint32_t t_ref_M = 0;
    static uint32_t t_ref_H = 0;
    //如果本次击打幅度超过门限，且距离上次击打时间超过100ms，则确认击打事件。
    if((mag_freq_low > AMP_THRD) && ((t_ref - t_ref_L) >= PK_MIN_Tms))
    { AUD_DET_LOG_I("\tL:%5d\n", mag_freq_low); t_ref_L = t_ref; }
    if((mag_freq_mid > AMP_THRD) && ((t_ref - t_ref_M) >= PK_MIN_Tms))
    { AUD_DET_LOG_I("\t\tM:%5d\n", mag_freq_mid); t_ref_M = t_ref; }
    if((mag_freq_high > AMP_THRD && ((t_ref - t_ref_H) >= PK_MIN_Tms)))
    { AUD_DET_LOG_I("\t\t\tH:%5d\n", mag_freq_high); t_ref_H = t_ref; }
#endif
}
#endif


#endif //CONFIG_AUD_AMP_DET_FFT
