/**
 **************************************************************************************
 * @file    audio_sync.c
 * @brief   Audio synchronization
 *
 * @author  Aixing.Li
 * @version V2.2.0
 *
 * &copy; 2017-2020 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include "app_beken_includes.h"
#include "driver_beken_includes.h"

#include "bt_a2dp_mpeg_aac_decode.h"

#if CONFIG_AUDIO_SYNC_ENABLE

#define AUDIO_SYNC_DBG_ENABLE           (0)
#if     AUDIO_SYNC_DBG_ENABLE
extern  int32 os_printf(const char *fmt, ...) ;
#define AUDIO_SYNC_DBG(fmt, ...)        os_printf(fmt, ##__VA_ARGS__)
#else
#define AUDIO_SYNC_DBG(fmt, ...)
#endif

#define AUDIO_SYNC_FLAG_P4_DAC_OPEN     (0x00000001)
#define AUDIO_SYNC_FLAG_W4_DAC_OPEN     (0x00000002)

#define AUDIO_SYNC_DAC_ADJUST_BASE_PPM  (200)
#define AUDIO_SYMC_DAC_COEF_OFFSET_STEP (16)
#define AUDIO_SYMC_DAC_COEF_OFFSET_TH   (3)
#define AUDIO_SYMC_DAC_COEF_OFFSET_MAX  (320*3)
#define AUDIO_SYNC_BY_BUF_LEVEL_NMODE   (60)
#define AUDIO_SYNC_BY_BUF_LEVEL_GMODE   (30)
#define AUDIO_SYNC_BY_BUF_TH            (30)
#define AUDIO_SYNC_RATIO_FRA_BITS       (4)

#define SAMPLE_WIDTH                    (4)
#define SAMPLES_PER_AAC_FRAME           (1024)
#define SAMPLES_PER_SBC_FRAME           (128)

extern uint32_t a2dp_get_freq(void);
extern uint32_t a2dp_get_codec_type(void);

extern t_clock HW_Get_Bt_Clk_Avoid_Race(void);

int32_t audio_sync_process_by_buffer(uint32_t tp);

static uint8_t  AUDIO_SYNC_RATIO_Q4[] = { 0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 19, 20, 22, 24, 25, 27, 29, 31, 33, 36, 38, 40, 43, 45, 48, 51, 54, 57, 60, 63, 67, 71, 75, 79, 83, 87, 92, 96, 101, 107, 112, 118, 124, 130, 136, 143, 150, 157, 165, 173, 181, 190, 199, 209, 219, 229, 240 };
static uint32_t AUDIO_SYNC_BY_BUF_LEVEL;
static uint32_t audio_sync_flag;

static uint8_t aud_sync_log_on_fg = 0;
void aud_sync_log_on_set(uint8_t log_flag) { aud_sync_log_on_fg = log_flag; }
uint8_t aud_sync_log_is_on(void) { return aud_sync_log_on_fg == 1; }

uint32_t audio_sync_get_bt_clk(void)
{
    return HW_Get_Bt_Clk_Avoid_Race();
}

static inline uint32_t audio_sync_get_dac_sample_rate(void)
{
    return a2dp_get_freq();
}

static inline uint32_t audio_sync_get_samples_per_frame(void)
{
    return a2dp_get_codec_type() == 0/*SBC*/ ? SAMPLES_PER_SBC_FRAME : SAMPLES_PER_AAC_FRAME;
}

static inline uint32_t audio_sync_get_max_nodes(void)
{
    if(a2dp_get_codec_type() == 0/*SBC*/)
    {
        return SBC_MEM_POOL_MAX_NODE;
    }
    else
    {
        #ifdef A2DP_MPEG_AAC_DECODE
        return AAC_FRAME_BUFFER_MAX_NODE;
        #else
        return 0;
        #endif
    }
}

static inline uint32_t audio_sync_get_residue_nodes(void)
{
    if(a2dp_get_codec_type() == 0/*SBC*/)
    {
        extern app_sbc_t app_sbc;
        return app_sbc.sbc_ecout + ((aud_dac_get_fill_buffer_size() >> 3) / audio_sync_get_samples_per_frame());//>>3 for 32bit2ch
    }
    else
    {
        #ifdef A2DP_MPEG_AAC_DECODE
        return ring_buffer_node_get_fill_nodes(&aac_frame_nodes);
        #else
        return 0;
        #endif
    }
}

#if (BT_AUD_SYNC_ADJ_BY_SW > 0)//sbc

#define ADJ_INTVL_GET_BY_PPM(ppm)           (1000000 / (ppm))
#define ADJ_MAX_PPM                         300U//clk offset:-ADJ_MAX_PPM ~ +ADJ_MAX_PPM
#define ADJ_STEP_PPM                        10U//ppm adj step

AUD_SPC_CTX_t aud_sync_bt = {
    .adj_dir = 0,
    .adj_int_smps = 0,
    .smps_cnt = 0,
    .adj_p_cnt = 0,
    .adj_n_cnt = 0,
};
AUD_SPC_CTX_t *p_aud_sync_bt = NULL;

void audio_sync_sys_init(void)
{
    #if (BT_AUD_SYNC_ADJ_BY_SW == 1)
    if(p_aud_sync_bt == NULL) p_aud_sync_bt = &aud_sync_bt;
    #elif (BT_AUD_SYNC_ADJ_BY_SW == 2)
    if(p_aud_sync_bt== NULL) p_aud_sync_bt = (AUD_SPC_CTX_t*)app_dsp_spc_h_get("aud_sync_bt", 1);
    #endif
    audio_spc_init(p_aud_sync_bt);
}

void audio_sync_reset(void)
{
    if(p_aud_sync_bt == NULL) return;
    audio_spc_init(p_aud_sync_bt);
}
AUD_SPC_CTX_t *aud_sync_bt_hdl_get(void) { return p_aud_sync_bt; }
// int aud_rb_adj_dir_get(void) { return p_aud_sync_bt->adj_dir; }
// uint32_t aud_rb_adj_int_get(void) { return p_aud_sync_bt->adj_int_smps; }

void aud_bt_sbc_clk_proc(void)
{
    uint8_t sbc_node_stat = sbc_node_buff_monitor();
    int sbc_clk = (int)(sbc_node_stat & 0x03) - 1;//sbc_clk: current local clk is -1:slow/0:normal/1:fast

    if(p_aud_sync_bt == NULL) return;
    // static int remain_node_prv;
    // int remain_node = sbc_buf_get_node_count();//audio_sync_get_residue_nodes();
    if(sbc_clk == -1)        { p_aud_sync_bt->adj_dir--; } // remain > level high thrd, del point
    else if(sbc_clk == 1)   { p_aud_sync_bt->adj_dir++; } // remain < level low thrd, add point
    else if(sbc_clk == 0)   { // level normal
        // if(remain_node > (remain_node_prv + 5)) p_aud_sync_bt->adj_dir--;
        // if(remain_node < (remain_node_prv - 5)) p_aud_sync_bt->adj_dir++;
        if(p_aud_sync_bt->adj_dir < 0) p_aud_sync_bt->adj_dir++;
        else if(p_aud_sync_bt->adj_dir > 0) p_aud_sync_bt->adj_dir--;
    }
    // remain_node_prv = remain_node;

    //limit adj class num
    int adj_cls_max = ADJ_MAX_PPM / ADJ_STEP_PPM;
    if(p_aud_sync_bt->adj_dir > adj_cls_max) p_aud_sync_bt->adj_dir = adj_cls_max;
    else if(p_aud_sync_bt->adj_dir < -adj_cls_max) p_aud_sync_bt->adj_dir = -adj_cls_max;

    //calc ppm and compesate interval
    int ppm = p_aud_sync_bt->adj_dir * ADJ_STEP_PPM;//ppm base host clock
    if(ppm) p_aud_sync_bt->adj_int_smps = ADJ_INTVL_GET_BY_PPM(ppm < 0 ? -ppm : ppm);
}

int aud_async_src_proc(AUD_SPC_CTX_t *aud_sync_h, int32_t *pcm, int smps_in)
{
    return audio_spc_exec(pcm, pcm, smps_in, 24, 2, audio_spc_calc(aud_sync_h, smps_in), 128);
}
#elif (BT_AUD_SYNC_ADJ_BY_SW == 0)
void audio_sync_reset(void) { }
#endif


void aud_sync_bt_debug(void)
{
#ifndef BT_AUDIO_SYNC_DEBUG
    if(!aud_sync_log_is_on()) return;
#endif
#if (BT_AUD_SYNC_ADJ_BY_SW > 0)
    os_printf("[%d]adj_dir:%d, int:%d, P:%d N:%d, node:%d/%d\n", BT_AUD_SYNC_ADJ_BY_SW, 
        p_aud_sync_bt->adj_dir, p_aud_sync_bt->adj_int_smps, 
        p_aud_sync_bt->adj_p_cnt, p_aud_sync_bt->adj_n_cnt,
        audio_sync_get_residue_nodes(), audio_sync_get_max_nodes());
    p_aud_sync_bt->adj_p_cnt = 0;
    p_aud_sync_bt->adj_n_cnt = 0;
#else
    int reg_adj = (int)((REG_SYSTEM_0x4B & 0x3FFFFFFF) - 0x0F1FAA4C);
    os_printf("is sbc:%d, adj:%d(~ppm:%d), node:%d/%d\n", (a2dp_get_codec_type() == 0), reg_adj, (reg_adj / 254), 
        audio_sync_get_residue_nodes(), audio_sync_get_max_nodes());
#endif
}

static inline void audio_sync_adjust_dac_sample_rate(int32_t ppm, int32_t offset)
{
    #if 0//def BT_AUDIO_SYNC_DEBUG//aac
    static int cnt = 0;
    static int cnt_p = 0;
    static int adj_p_cnt = 0;
    static int cnt_n = 0;
    static int adj_n_cnt = 0;
    if(ppm > 0)     { cnt_p++; adj_p_cnt += ppm; }
    else if(ppm < 0){ cnt_n++; adj_n_cnt += ppm; }
    if(++cnt >= 200){
        os_printf("AAC ppm:%d, P:%d,%d N:%d,%d, node:%d\n", ppm, adj_p_cnt, cnt_p, adj_n_cnt, cnt_n, audio_sync_get_residue_nodes());
        cnt = adj_p_cnt = cnt_p = adj_n_cnt = cnt_n = 0;
    }
    #endif
#if (BT_AUD_SYNC_ADJ_BY_SW > 0)//aac
    if(p_aud_sync_bt == NULL) return;
    // ppm = (ppm * ADJ_MAX_PPM) / 100;//the ppm will not exceed 100
    p_aud_sync_bt->adj_dir = -ppm;
    if(ppm) p_aud_sync_bt->adj_int_smps = ADJ_INTVL_GET_BY_PPM(ppm < 0 ? -ppm : ppm);
#elif (BT_AUD_SYNC_ADJ_BY_SW == 0)
    #ifdef APLL_KEEP_98p304MHZ
    int32_t dac_frac_coef = 0x0F1FAA4C;//apll keep about 98.304M
    #else
    int32_t dac_frac_coef = audio_sync_get_dac_sample_rate() == 44100 ? 0x0DE517A9 : 0x0F1FAA4C;
    #endif

    dac_frac_coef += (int32_t)((int64_t)dac_frac_coef * ppm    / 1000000);
    dac_frac_coef += (int32_t)((int64_t)dac_frac_coef * offset / 1000000);

    REG_SYSTEM_0x4B = 0x80000000 | dac_frac_coef;
#endif
}

void audio_sync_init(void)
{
    if(a2dp_get_codec_type() == 0/*SBC*/) return;//FIXME

    audio_sync_flag = AUDIO_SYNC_FLAG_P4_DAC_OPEN;

    //AUDIO_SYNC_BY_BUF_LEVEL = AUDIO_SYNC_BY_BUF_LEVEL_NMODE;
    AUDIO_SYNC_BY_BUF_LEVEL = get_aud_mode_flag(AUD_FLAG_GAME_MODE) ? AUDIO_SYNC_BY_BUF_LEVEL_GMODE : AUDIO_SYNC_BY_BUF_LEVEL_NMODE;
}

void audio_sync_prepare_for_dac_open(uint32_t t)
{
    if((audio_sync_flag & AUDIO_SYNC_FLAG_P4_DAC_OPEN) && !(audio_sync_flag & AUDIO_SYNC_FLAG_W4_DAC_OPEN))
    {
        //Do nothing here for NON-TWS solution
        audio_sync_flag |= AUDIO_SYNC_FLAG_W4_DAC_OPEN;
    }
}

void audio_sync_process_for_dac_open(void)
{
    if(audio_sync_flag & AUDIO_SYNC_FLAG_W4_DAC_OPEN)
    {
        if(audio_sync_get_residue_nodes() >= audio_sync_get_max_nodes() * AUDIO_SYNC_BY_BUF_LEVEL / 100)
        {
            audio_sync_flag &= ~(AUDIO_SYNC_FLAG_W4_DAC_OPEN | AUDIO_SYNC_FLAG_P4_DAC_OPEN);
            os_printf("AUDIO SYNC START <%d>\r\n", audio_sync_get_residue_nodes());
        }
    }
}

int32_t audio_sync_process_by_buffer(uint32_t tp)
{
    int32_t  dif, dir;
    uint32_t nodes = audio_sync_get_residue_nodes();
    uint32_t level = audio_sync_get_max_nodes() * AUDIO_SYNC_BY_BUF_LEVEL / 100 - (AUDIO_DAC_BUFF_LEN / (audio_sync_get_samples_per_frame() * SAMPLE_WIDTH * 2) - 1);
    uint32_t th    = audio_sync_get_max_nodes() * AUDIO_SYNC_BY_BUF_TH / 100;

    (void)tp;

    AUDIO_SYNC_DBG("%d\n", nodes - level);

    if(nodes > level)
    {
        dir = 1;
        dif = nodes - level;
    }
    else
    {
        dir = -1;
        dif = level - nodes;
    }

    dif = ((dif << AUDIO_SYNC_RATIO_FRA_BITS) + th / 2) / th;

    if(dif >= sizeof(AUDIO_SYNC_RATIO_Q4)) dif = sizeof(AUDIO_SYNC_RATIO_Q4) - 1;

    dif = AUDIO_SYNC_RATIO_Q4[dif] / 1;
    dif = AUDIO_SYNC_DAC_ADJUST_BASE_PPM * dif >> AUDIO_SYNC_RATIO_FRA_BITS;

    audio_sync_adjust_dac_sample_rate(dir * dif, 0);

    return 0;
}

int32_t audio_sync_process(uint32_t tp)
{
    return audio_sync_process_by_buffer(tp);
}

uint32_t audio_sync_get_flag(void)
{
    return audio_sync_flag;
}

void audio_sync_set_flag(uint32_t flag)
{
    audio_sync_flag = flag;
}

#endif//CONFIG_AUDIO_SYNC_ENABLE
