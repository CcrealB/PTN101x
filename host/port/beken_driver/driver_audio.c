#include "hw_leds.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "coder.h"
#include "bkmp3_resample.h"
#include "bkreg.h"
#include "drv_audio.h"
#include "drv_system.h"
#include "math.h"
#include <string.h>

#ifdef USER_KARAOK_MODE
#include "u_com.h"
#include "utils_audio.h"
#endif
#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_DSP || CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_DSP
#include "drv_mailbox.h"
#endif

extern uint32_t _aud_rb_begin;
static uint8_t s_audio_open_stat = 0;
volatile static uint16_t mic_is_mute=0;

#if (BT_AUD_SYNC_ADJ_BY_SW == 0)
#define DAC_CLK_ADJUST_STEP     8192

uint32_t s_aud_dac_fraccoef[2][3] =
{
    {AUDIO_DIV_441K_SLOW, AUDIO_DIV_441K, AUDIO_DIV_441K_FAST},
    {AUDIO_DIV_48K_SLOW,  AUDIO_DIV_48K,  AUDIO_DIV_48K_FAST}
};


static t_dac_clk_state s_aud_dac_state = AUDIO_DAC_NORM;
static t_dac_clk_state s_current_aud_dac_state = AUDIO_DAC_NORM;
#endif

static uint32_t s_current_aud_dac_sample_rate = 0;

#if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
AUDIO_CTRL_BLK audio_adc_ctrl_blk;
#endif
#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
AUDIO_CTRL_BLK audio_dac_ctrl_blk;
#endif

int16 g_soft_dig_gain = 256;
int16 g_soft_dig_goal = 256;
#ifndef AUD_CFG_ADC_SEL
#define AUD_CFG_ADC_SEL     0
#endif

#ifdef USER_KARAOK_MODE


static AUD_AS_TYPE_e s_audio_aso_play_sel = ASO_TYPE_NONE;//audio stream play
static AUD_AS_TYPE_e s_audio_asi_record_sel = ASI_TYPE_NONE;//audio stream record

#ifdef AUD_D2M_MCU_RINGBUF_EN
    #define ASI_DEF_CRITICAL_RUN(en)    as_critical_code_protect(en)
    RingBufferContext def_asi_ringbuf;//default audio stream input
    volatile uint8_t rec_rb_overflow_cnt = 0;
#endif

#ifdef AUD_M2D_MCU_RINGBUF_EN
    #define ASO_DEF_CRITICAL_RUN(en)    as_critical_code_protect(en)
    RingBufferContext def_aso_ringbuf;
#endif
#ifdef AUD_M2D_MCU_RINGBUF_EN
uint8_t usb_aso_rb_buff[AUDIO_USBO_BUF_SIZE];
RingBufferContext usb_aso_rb;
#endif
#ifdef AUD_D2M_MCU_RINGBUF_EN
uint8_t usb_asi_rb_buff[AUDIO_USBI_BUF_SIZE];
RingBufferContext usb_asi_rb;
#endif

#endif //USER_KARAOK_MODE

static void as_critical_code_protect(uint8_t en)
{
    static uint32_t interrupts_info, mask;
    (en) ? SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask) : SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    // REG_GPIO_0x0C = en ? 2 : 0; //debug
}

void aud_init(void)
{
	app_env_handle_t  env_h = app_env_get_handle();

    //system_apll_config(SYS_APLL_90p3168_MHZ);

    audio_init();
    if(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_DAC_DIFFER)
    	audio_dac_init(AUDIO_DAC_MODE_DIFFERENCE);
    else
    	audio_dac_init(AUDIO_DAC_MODE_VCOM);
    audio_dac_sample_rate_set(48000); 
    
    #if AUD_CFG_ADC_SEL
    audio_anc_init(AUDIO_ADC_WIDTH_16, AUDIO_ADC_MODE_SINGLE_END);
    if(env_h->env_cfg.used == 1)
    {
        audio_anc_dig_gain_set(env_h->env_cfg.feature.vol_mic_dig&0x3f);
        audio_anc_ana_gain_set(env_h->env_cfg.feature.vol_mic_ana&0xf);
    }
    audio_anc_sample_rate_set(16000);
    #else
    audio_adc_init(AUDIO_ADC_WIDTH_16, AUDIO_ADC_MODE_SINGLE_END);
    if(env_h->env_cfg.used == 1)
    {
        audio_adc_dig_gain_set(env_h->env_cfg.feature.vol_mic_dig&0x3f);
        audio_adc_ana_gain_set(env_h->env_cfg.feature.vol_mic_ana&0xf);
    }
    audio_adc_sample_rate_set(16000);
    #endif
    audio_dac_ana_gain_set(0);

    #if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
    extern uint32_t _aud_rb_begin;
    void* adc_dma = dma_channel_malloc();
    audio_adc_ctrl_blk.data_buff = (uint8_t*)((uint32)&_aud_rb_begin);

    dma_channel_config(adc_dma,
                    #if AUD_CFG_ADC_SEL //0x41~0x44:MIC2~MIC5, 0x48:MIC2~5 Array
                       DMA_REQ_AUDIO_ANCIN,
                       DMA_MODE_REPEAT,
                       (uint32_t)&REG_ANC_0x42,
                       (uint32_t)&REG_ANC_0x42,
                    #else
                       DMA_REQ_AUDIO_ADC,
                       DMA_MODE_REPEAT,
                       (uint32_t)&REG_ANC_0x47,
                       (uint32_t)&REG_ANC_0x47,
                    #endif
                       DMA_ADDR_NO_CHANGE,
                       DMA_DATA_TYPE_SHORT,
                       (uint32_t)audio_adc_ctrl_blk.data_buff,
                       (uint32_t)audio_adc_ctrl_blk.data_buff + AUDIO_ADC_BUFF_LEN,
                       DMA_ADDR_AUTO_INCREASE,
                       DMA_DATA_TYPE_LONG,
                       AUDIO_ADC_BUFF_LEN
                      );

    audio_adc_ctrl_blk.dma_handle = adc_dma;

    dma_channel_enable(adc_dma, 1);
    dma_channel_enable(adc_dma, 0);
    #endif

    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    extern uint32_t _aud_rb_begin;
    void* dac_dma = dma_channel_malloc();
    audio_dac_ctrl_blk.data_buff = (uint8_t*)((uint32)&_aud_rb_begin) + AUDIO_ADC_BUFF_LEN;

    dma_channel_config(dac_dma,
                       DMA_REQ_AUDIO_DAC,
                       DMA_MODE_REPEAT,
                       (uint32_t)audio_dac_ctrl_blk.data_buff,
                       (uint32_t)audio_dac_ctrl_blk.data_buff + AUDIO_DAC_BUFF_LEN,
                       DMA_ADDR_AUTO_INCREASE,
                       DMA_DATA_TYPE_LONG,
                       (uint32_t)&REG_ANC_0x4E,
                       (uint32_t)&REG_ANC_0x4E,
                       DMA_ADDR_NO_CHANGE,
                       DMA_DATA_TYPE_LONG,
                       AUDIO_DAC_BUFF_LEN
                      );

    audio_dac_ctrl_blk.dma_handle = dac_dma;

    dma_channel_enable(dac_dma, 1);
    dma_channel_enable(dac_dma, 0);
    #endif
#ifdef AUD_D2M_MCU_RINGBUF_EN
    ring_buffer_init(&def_asi_ringbuf, (uint8_t *)def_asi_rb_addr, def_asi_rb_size, NULL, RB_DMA_TYPE_NULL);
#endif
#ifdef AUD_M2D_MCU_RINGBUF_EN
    ring_buffer_init(&def_aso_ringbuf, (uint8_t *)def_aso_rb_addr, def_aso_rb_size, NULL, RB_DMA_TYPE_NULL);
#endif
#ifdef AUD_M2D_MCU_RINGBUF_EN
    ring_buffer_init(&usb_aso_rb, (uint8_t *)usb_aso_rb_buff, sizeof(usb_aso_rb_buff), NULL, RB_DMA_TYPE_NULL);
#endif
#ifdef AUD_D2M_MCU_RINGBUF_EN
    ring_buffer_init(&usb_asi_rb, (uint8_t *)usb_asi_rb_buff, sizeof(usb_asi_rb_buff), NULL, RB_DMA_TYPE_NULL);
#endif
}

#ifdef USER_KARAOK_MODE
void audio_aso_type_set(AUD_AS_TYPE_e as_type, uint8_t en) { s_audio_aso_play_sel = en ? (s_audio_aso_play_sel | as_type) : (s_audio_aso_play_sel & ~as_type); }
uint32_t audio_aso_type_get(AUD_AS_TYPE_e as_type) { return (s_audio_aso_play_sel & as_type); }
void audio_asi_type_set(AUD_AS_TYPE_e as_type, uint8_t en) { s_audio_asi_record_sel = en ? (s_audio_asi_record_sel | as_type) : (s_audio_asi_record_sel & ~as_type); }
uint32_t audio_asi_type_get(AUD_AS_TYPE_e as_type) { return (s_audio_asi_record_sel & as_type); }

#if BT_A2DP_SRC248_EN
#include "api_src.h"
aud_src_ctx_t bt_aso_src_s = {
    .fs_in = 44100,
    .fs_out = SRC_OUT_SAMPLERATE,
    .chs = 2,
    .bits_in = 24,
    .bits_out = 24,
    .src_buf = NULL,
    .max_fra_sz = 192,
};
#endif
int audio_aso_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample, AUD_AS_TYPE_e as_type)
{
    if(as_type == ASO_TYPE_USB)
    {
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_AS_USBO_CONFIG | MAILBOX_CMD_FAST_RSP_FLAG, freq, channels, bits_per_sample, NULL);
        audio_aso_type_set(ASO_TYPE_USB, 0);
        LOG_I(DRV, "audio_aso_config(%d, %d, %d, 0x%x)\n",freq, channels, bits_per_sample, as_type);
        return 0;
    }

    if(as_type & ASO_TYPE_BT)
    {
        #if BT_A2DP_SRC248_EN
        bt_aso_src_s.fs_in = freq;
        bt_aso_src_s.chs = channels;
        bt_aso_src_s.max_smps_in = 128;
        audio_src_init(&bt_aso_src_s);
        freq = 48000;
        #endif
        #if (BT_AUD_SYNC_ADJ_BY_SW > 0)
        audio_sync_reset();
        #elif (BT_AUD_SYNC_ADJ_BY_SW == 0)
        s_aud_dac_state = AUDIO_DAC_NORM;
        s_current_aud_dac_state = AUDIO_DAC_NORM;
        #endif
    }

    audio_aso_type_set(ASO_TYPE_DEF, 0);
	s_audio_open_stat = 0;
    s_current_aud_dac_sample_rate = freq;

    mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_DAC_CONFIG | MAILBOX_CMD_FAST_RSP_FLAG, freq, channels, def_aso_bit_width/*bits_per_sample*/, NULL);

    LOG_I(DRV, "audio_aso_config(%d, %d, %d, 0x%x)\n",freq, channels, bits_per_sample, as_type);

    return 0;//aud_dac_config(freq, channels, bits_per_sample);
}

uint32_t aud_dac_sample_rate_cur_get(void)
{
    return s_current_aud_dac_sample_rate;
}
#endif

int aud_dac_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample)
{
#ifdef USER_KARAOK_MODE
    audio_aso_config(freq, channels, bits_per_sample, ASO_TYPE_DEF);
#else
#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    uint32_t apll_freq = (((freq / 11025) * 11025) == freq) ? SYS_APLL_90p3168_MHZ : SYS_APLL_98p3040_MHZ;
    uint32_t freq_changed = freq != s_current_aud_dac_sample_rate;

    (void)freq_changed;
#endif
	s_audio_open_stat = 0;
    s_aud_dac_state = AUDIO_DAC_NORM;
    s_current_aud_dac_state = AUDIO_DAC_NORM;
    s_current_aud_dac_sample_rate = freq;

    #if CONFIG_ANC_ENABLE == 1
    if(freq_changed && app_anc_status_get())
    {
        app_anc_enable(0);
        #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
        audio_dac_enable(0);
        #endif

        if(apll_freq != system_apll_freq()) app_anc_freq_changed(apll_freq);
    }
    #endif

    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    if(freq_changed)
    {
        system_apll_config(apll_freq);
        audio_dac_sample_rate_set(freq);
    }
    audio_dac_ctrl_blk.channels = channels;
    rb_init(&audio_dac_ctrl_blk.aud_rb, (uint8_t *)audio_dac_ctrl_blk.data_buff, AUDIO_DAC_BUFF_LEN, 1, audio_dac_ctrl_blk.dma_handle);
    dma_channel_enable(audio_dac_ctrl_blk.dma_handle,1);
    dma_channel_enable(audio_dac_ctrl_blk.dma_handle,0);
    memset(audio_dac_ctrl_blk.data_buff, 0, AUDIO_DAC_BUFF_LEN);
    aud_dac_fill_buffer((uint8_t *)audio_dac_ctrl_blk.data_buff, ((freq>16000)?(AUDIO_DAC_BUFF_LEN>>1):1024));  // will increase delay in sco
    aud_mute_func_init(6, 9);
    audio_dac_enable(1);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_DAC_CONFIG | MAILBOX_CMD_FAST_RSP_FLAG, freq, channels, bits_per_sample);
    #endif

    #if CONFIG_ANC_ENABLE == 1
    if(freq_changed && app_anc_status_get()) app_anc_enable(1);
    #endif

#endif
    LOG_I(DRV, "audio_dac_config(%d, %d, %d)\r\n",freq, channels, bits_per_sample);
    return(0);
}

#ifdef USER_KARAOK_MODE
void audio_aso_open(AUD_AS_TYPE_e as_type)
{
    if(audio_aso_type_get(as_type)) return;

    if(as_type == ASO_TYPE_USB)
    {
    #if USB_AUDIO_FUNC_VALID
        usb_out_ringbuff_clear();//fill ringbuff avoid noise
        uint8_t pcm[AUDIO_USBO_BUF_SIZE / 2];
        memset(pcm, 0, sizeof(pcm));
        usb_out_ringbuff_write((uint8_t*)pcm, AUDIO_USBO_BUF_SIZE / 2);
        uint32_t asrc_en = (CONFIG_AUD_SYNC_USBO == 2) ? 1 : 0;
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_AS_USBO_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, asrc_en, 0, NULL);
    #endif
    }
    else
    {
        // if(s_audio_open_stat) return;
    #if 1//must enable for asrc inlcude src, whitch used by sdcard&udisk also
        uint32_t asrc_en = 1;
    #else
        uint32_t asrc_en = 0;
        #if CONFIG_AUD_SYNC_SPDIF == 2
        if(as_type == ASO_TYPE_SPDIF) { asrc_en = 1;}
        #endif
        #if BT_AUD_SYNC_ADJ_BY_SW == 2
        if(as_type == ASO_TYPE_BT) { asrc_en = 1;}
        #endif
    #endif

        mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, asrc_en, 0, NULL);//en, asrc_en

        aud_volume_mute(0);

        s_audio_open_stat = 1;

        aud_PAmute_delay_operation(0);

        #ifdef A2DP_MPEG_AAC_DECODE
        if(A2DP_CODEC_MPEG_AAC == a2dp_get_codec_type())//a2dp_has_connection()
        {
            LOG_I(DRV, "bt_a2dp_aac_stream_sync init\n");
            extern result_t bt_a2dp_aac_stream_sync(uint32_t type);
            bt_a2dp_aac_stream_sync(0);
        }
        #endif
    }
    audio_aso_type_set(as_type, 1);
    LOG_I(DRV, "audio_aso_open:0x%x\n", as_type);
}
#endif
void aud_dac_open(void)
{
#ifdef USER_KARAOK_MODE
    if(!s_audio_open_stat)
        LOG_I(DRV, "audio dac open\r\n");
    audio_aso_open(ASO_TYPE_DEF);
#else
    if(s_audio_open_stat) return;

    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    //audio_dac_enable(1);
    dma_channel_enable(audio_dac_ctrl_blk.dma_handle, 1);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0);
    #endif

    aud_volume_mute(0);

    s_audio_open_stat = 1;

    LOG_I(DRV, "audio dac open\r\n");
    aud_PAmute_delay_operation(0);

    #ifdef A2DP_MPEG_AAC_DECODE
    if(A2DP_CODEC_MPEG_AAC == a2dp_get_codec_type())
    {
        extern result_t bt_a2dp_aac_stream_sync(uint32_t type);
        bt_a2dp_aac_stream_sync(0);
    }
    #endif
#endif
}


#ifdef USER_KARAOK_MODE

void audio_aso_close(AUD_AS_TYPE_e as_type)
{
    if(!audio_aso_type_get(as_type)) return;

    audio_aso_type_set(as_type, 0);

    if(as_type == ASO_TYPE_USB)
    {
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_AS_USBO_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, NULL);
    }
    else
    {
        audio_aso_type_set(ASO_TYPE_DEF, 0);
        // if(!s_audio_open_stat) return;
        s_audio_open_stat = 0;
        aud_PAmute_delay_operation(1);
        set_flag_sbc_buffer_play(0);

        #if(CONFIG_AUD_FADE_IN_OUT == 1)
        set_aud_fade_in_out_state(AUD_FADE_NONE);
        #endif    

        mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, NULL);
    }

    LOG_I(DRV, "audio_aso_close:0x%x\n", as_type);
}
#endif
void aud_dac_close(void)
{
#ifdef USER_KARAOK_MODE
    audio_aso_close(ASO_TYPE_DEF);
    LOG_I(DRV, "audio dac close\r\n");
#else
    if(!s_audio_open_stat) return;
    s_audio_open_stat = 0;
    aud_PAmute_delay_operation(1);
    set_flag_sbc_buffer_play(0);
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    dma_channel_enable(audio_dac_ctrl_blk.dma_handle, 0);
    #endif

    #if(CONFIG_AUD_FADE_IN_OUT == 1)
    set_aud_fade_in_out_state(AUD_FADE_NONE);
    #endif    

    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    #if CONFIG_ANC_ENABLE == 1
    if(app_anc_status_get() == ANC_STATUS_IDLE)
    #endif
    audio_dac_enable(0);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0);
    #endif

    LOG_I(DRV, "audio dac close\r\n");
#endif
}


void aud_dac_dig_volume_fade_in(void)
{
    g_soft_dig_gain = 0;
}


#if(CONFIG_DAC_CLOSE_IN_IDLE == 1)
void aud_dac_close_in_idle(void)
{
    #if CONFIG_ANC_ENABLE == 1
    if(app_anc_status_get()) return;
    #endif

    if(!app_bt_flag1_get(APP_AUDIO_WORKING_FLAG_SET) && !(get_system_idle_cnt()))
    {
        aud_dac_close();
    }
}
#endif

#if (BT_AUD_SYNC_ADJ_BY_SW == 0)
uint8_t get_audio_dac_clk_state(void)
{
    return s_aud_dac_state;
}

void aud_dac_clk_set_default(void)
{
    uint8_t sbc_node_stat = sbc_node_buff_monitor();
    uint8_t sbc_freq  = (sbc_node_stat >> 2)&0x01;
    uint32_t *p_dac_clk_coef = s_aud_dac_fraccoef[sbc_freq];
    BK3000_AUD_DAC_FRACCOEF = 0x80000000 | p_dac_clk_coef[1];
}

void aud_dac_clk_set_coef(uint32_t clk_val)
{
    BK3000_AUD_DAC_FRACCOEF = 0x80000000 | clk_val;
}
#endif

void aud_dac_clk_process(void)
{
#if (BT_AUD_SYNC_ADJ_BY_SW > 0)//sbc
    aud_bt_sbc_clk_proc();
#elif (BT_AUD_SYNC_ADJ_BY_SW == 0)
    uint8_t sbc_node_stat = sbc_node_buff_monitor();
    uint8_t sbc_freq  = (sbc_node_stat >> 2)&0x01;
    uint8_t sbc_clk   = (sbc_node_stat & 0x03);
    uint32_t *p_dac_clk_coef = s_aud_dac_fraccoef[sbc_freq];
    //extern app_sbc_t app_sbc; os_printf("%d, %d\n", sbc_clk, app_sbc.sbc_ecout); return;
    switch(s_aud_dac_state)
    {
    case AUDIO_DAC_NORM:
        if(s_current_aud_dac_state != AUDIO_DAC_NORM)
        {
            BK3000_AUD_DAC_FRACCOEF = 0x80000000 | p_dac_clk_coef[1];
        }
        s_current_aud_dac_state = AUDIO_DAC_NORM;
        if(sbc_clk == 0)
        {
            s_aud_dac_state =  AUDIO_DAC_SLOW;         // Local CLK is slow
        }
        else if(sbc_clk == 2)
        {
            s_aud_dac_state =  AUDIO_DAC_FAST;         // Local CLK is fast
        }
        break;
    case AUDIO_DAC_SLOW:
        if(s_current_aud_dac_state != AUDIO_DAC_SLOW)
        {
        BK3000_AUD_DAC_FRACCOEF = 0x80000000 | p_dac_clk_coef[2];  //AUDIO_DIV_441K_SLOW;
        }
        s_current_aud_dac_state = AUDIO_DAC_SLOW;
        if(sbc_clk == 0)
        {
            s_aud_dac_state =  AUDIO_DAC_SLOWER;    // Local CLK is very slow
        }
        else if(sbc_clk == 2)
        {
            s_aud_dac_state =  AUDIO_DAC_NORM;        // Local CLk between norm and slow
        }

        break;
    case AUDIO_DAC_SLOWER:
        if(s_current_aud_dac_state != AUDIO_DAC_SLOWER)
        {
            BK3000_AUD_DAC_FRACCOEF = 0x80000000 | (p_dac_clk_coef[2] + DAC_CLK_ADJUST_STEP);
        }
        s_current_aud_dac_state = AUDIO_DAC_SLOWER;
        if(sbc_clk == 0)
        {
            s_aud_dac_state =  AUDIO_DAC_SLOWER;    // Local clk is very slow,but not follow the mobile yet;
        }
        else if(sbc_clk == 2)
        {
            s_aud_dac_state =  AUDIO_DAC_SLOW;        // Local clk between  veryslow and slow
        }
        break;
    case AUDIO_DAC_FAST:
        if(s_current_aud_dac_state != AUDIO_DAC_FAST)
        {
            BK3000_AUD_DAC_FRACCOEF = 0x80000000 | p_dac_clk_coef[0];
        }
        s_current_aud_dac_state = AUDIO_DAC_FAST;
        if(sbc_clk == 0)
        {
            s_aud_dac_state =  AUDIO_DAC_NORM;        // Local clk between fast and norm
        }
        else if(sbc_clk == 2)
        {
            s_aud_dac_state =  AUDIO_DAC_FASTER;    // Local clk is very fast
        }
        break;
    case AUDIO_DAC_FASTER:
        if(s_current_aud_dac_state != AUDIO_DAC_FASTER)
        {
            BK3000_AUD_DAC_FRACCOEF = 0x80000000 | (p_dac_clk_coef[0] - DAC_CLK_ADJUST_STEP);
        }
        s_current_aud_dac_state = AUDIO_DAC_FASTER;
        if(sbc_clk == 0)
        {
            s_aud_dac_state =  AUDIO_DAC_FAST;        // Local clk between veryfast and fast
        }
        else if(sbc_clk == 2)
        {
            s_aud_dac_state =  AUDIO_DAC_FASTER;    // Local clk is very fast,but not follow the mobile yet;
        }
        break;
    default:
        break;
    }
#endif
}

void aud_dac_set_volume(int8_t volume)
{
//	extern void Music32_VolSet(uint8_t val);
//	Music32_VolSet(volume);	//yuan++
    LOG_I(VOL,"volumeSync:%d\n", volume);

#if 0	//yuan41
    aud_volume_t *vol_tbl_ptr;
    uint32_t ana_gain = 0;
    int16 dig_gain = 0;

    app_env_handle_t env_h = app_env_get_handle();

#ifdef CONFIG_APP_EQUANLIZER
    if (env_h->env_cfg.aud_eq.eq_enable)
    {
        dig_gain = (int16)env_h->env_cfg.aud_eq.eq_gain;
    }
#endif

    if (hfp_has_sco() || app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.hfp_vol;
#ifdef CONFIG_APP_EQUANLIZER
        dig_gain = 0;
#endif
    }
#ifdef CONFIG_LINEIN_FUNCTION
    else if (app_bt_flag1_get(APP_FLAG_LINEIN) && (!app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE)))
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.linein_vol;
    }
#endif
    else
    {
        vol_tbl_ptr = (aud_volume_t *)&env_h->env_cfg.feature.a2dp_vol;
    }

    ana_gain = (vol_tbl_ptr[volume & 0x1F].ana_dig_gain) >>13;
    dig_gain = (vol_tbl_ptr[volume & 0x1F].ana_dig_gain) & 0x1fff;
#ifdef CONFIG_APP_EQUANLIZER
    if (dig_gain < 0)
        dig_gain = 0;
#endif

//yuan++    app_bt_flag2_set(APP_FLAG2_SW_MUTE, !volume);	// 關閉 SBC MUTE

    if(volume > 0){
        aud_PAmute_operation(0);
        if (volume > AUDIO_VOLUME_MAX) 
            volume = AUDIO_VOLUME_MAX;
    }else{
        aud_PAmute_operation(1);
        if (volume <= AUDIO_VOLUME_MIN) 
            volume = AUDIO_VOLUME_MIN;
    }

    g_soft_dig_goal = dig_gain;
    (void) ana_gain;
#endif
}

static __inline int32_t SMULLRAS(int32_t a, int32_t b, uint32_t bits)
{
    int32_t result;
    __asm
    (
        "b.mulas %0,%1,%2,%3;"
        : "=r" (result)
        : "r" (a), "r" (b), "i" (bits)
        :
    );
    return result;
}

void aud_dac_volume_control( uint8_t *buff, uint32_t size)
{
    int32 *smpl = (int32 *)buff;
    int32_t smpl32_L,smpl32_R;
    int16_t k;
#ifndef USER_KARAOK_MODE
    {
        app_env_handle_t  env_h = app_env_get_handle();
        uint8_t flag_L_is_LplusR = !!(env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_L_is_LplusR);
        if (flag_L_is_LplusR)  // L = R = L + R;
        {
            for (k = 0; k < (size >> 3); k++)
            {
                if (g_soft_dig_gain < g_soft_dig_goal)
                    g_soft_dig_gain++;
                else if  (g_soft_dig_gain > g_soft_dig_goal)
                    g_soft_dig_gain--;

                smpl32_L = SMULLRAS(*(smpl + 2*k    ), g_soft_dig_gain, 8);
                smpl32_R = SMULLRAS(*(smpl + 2*k + 1), g_soft_dig_gain, 8);
                smpl32_L = ((smpl32_L + smpl32_R + 1) >> 1);

                *(smpl + 2*k    ) = smpl32_L;
                *(smpl + 2*k + 1) = smpl32_L;
            }
        }
        else
        {
            for (k = 0; k < (size >> 3); k++)
            {
                if (g_soft_dig_gain < g_soft_dig_goal)
                    g_soft_dig_gain++;
                else if  (g_soft_dig_gain > g_soft_dig_goal)
                    g_soft_dig_gain--;

                smpl32_L = SMULLRAS(*(smpl + 2*k    ), g_soft_dig_gain, 8);
                smpl32_R = SMULLRAS(*(smpl + 2*k + 1), g_soft_dig_gain, 8);

                *(smpl + 2*k    ) = smpl32_R;
                *(smpl + 2*k + 1) = smpl32_L;
            }
        }
    }
#endif
#if(CONFIG_AUD_FADE_IN_OUT == 1)
    {   extern int16_t s_fade_scale;
        if(get_aud_fade_in_out_state() & (AUD_FADE_IN | AUD_FADE_OUT ))
        {
            for (k = 0; k < (size >> 3); k++)
            {
                smpl32_L = (*(smpl + 2*k) >> 8) * g_soft_dig_gain * s_fade_scale;
                smpl32_R = (*(smpl + 2*k + 1) >> 8) * g_soft_dig_gain * s_fade_scale;
                *(smpl + 2*k) = smpl32_L >> AUD_FADE_SCALE_LEFT_SHIFT_BITS;
                *(smpl + 2*k + 1) = smpl32_R >> AUD_FADE_SCALE_LEFT_SHIFT_BITS;
            }
        }
    }
#endif
}

#ifdef USER_KARAOK_MODE

int share_mem_var_val_get(char *str, volatile int **p, uint32_t cmd)
{
    int ret = 0;
    if(*p == NULL) {
        uint32_t dsp2mcu_ack_data[4] = {0};
        mbx_mcu2dsp_transfer(cmd | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
        uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
        if(ack_cmd == cmd) {
            ret = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
            *p = (int*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
            LOG_I(DRV, "[%s:%s], ret:%d, p_free:0x%p\n", __FUNCTION__, str, ret, *p);
        }else{
            ret = 0;
            LOG_I(DRV, "[%s:%s] !!!!!!!! ack_cmd:0x%08X\n\n\n", __FUNCTION__, str, ack_cmd);
        }
    } else {
        ret = **p;
    }
    return ret;
}

#ifdef AUD_WAV_TONE_SEPARATE
void aud_wav_fill_buffer(uint8_t *buff, uint16_t size)
{
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_WAV_BUF_WRITE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, (uint32_t)size, 0, NULL);
}

int aud_wav_ringbuf_free_get(void)
{
    #if 1
    static volatile int *p_wav_rb_free_sz = NULL;
    return share_mem_var_val_get("wav_rb_free", &p_wav_rb_free_sz, USR_MBX_CMD_AUDIO_WAV_FREE_BUF_SIZE_CHECK);
    #else
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_WAV_FREE_BUF_SIZE_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    return dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    #endif
}
#endif //AUD_WAV_TONE_SEPARATE

#if !(defined(ASO_DEF_TRANS_BY_SHARE_RB) || defined(ASO_USB_TRANS_BY_SHARE_RB)) 
//audio stream output
int aud_aso_ringbuf_write(unsigned char *buff, int size, AUD_AS_TYPE_e as_type)
{
    // if(s_audio_aso_play_sel != as_type) return;
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    aud_dac_volume_control( buff, size);
    rb_fill_buffer(&audio_dac_ctrl_blk.aud_rb, buff, size, DAC_FILL_WITH_COMP);
    #else
    uint32_t cmd = (as_type == ASO_TYPE_USB) ? USR_MBX_CMD_AUDIO_AS_USBO_BUF_WRTIE : MAILBOX_CMD_AUDIO_DAC_BUF_WRITE;
    if(as_type != ASO_TYPE_USB) aud_dac_volume_control(buff, size);
    mbx_mcu2dsp_transfer(cmd | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, (uint32_t)size, as_type, NULL);
    #endif
    return size;
}

int aud_aso_ringbuf_fill_get(AUD_AS_TYPE_e as_type)
{
    #if CONFIG_DAC_CTRL_MODE ==DAC_CTRL_BY_MCU
    return rb_get_buffer_fill_size(&audio_dac_ctrl_blk.aud_rb);
    #else
    uint32_t dsp2mcu_ack_data[4] = {0};
    uint32_t cmd = (as_type == ASO_TYPE_USB) ? USR_MBX_CMD_AUDIO_AS_USBO_BUF_FILL_CHK : MAILBOX_CMD_AUDIO_DAC_FILL_BUF_SIZE_CHECK;
    mbx_mcu2dsp_transfer(cmd | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    return dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    #endif
}

int aud_aso_ringbuf_free_get(AUD_AS_TYPE_e as_type)
{
#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
return rb_get_buffer_free_size(&audio_dac_ctrl_blk.aud_rb);
#else

    int free_size = 0;

    #if 1//use share mem var to get ringbuf free size
    if(as_type == ASO_TYPE_USB) {
        static volatile int *p_usbrx_rb_free_sz = NULL;
        free_size = share_mem_var_val_get("usbrx_rb_free", &p_usbrx_rb_free_sz, USR_MBX_CMD_AUDIO_AS_USBO_BUF_FREE_CHK);
    } else {
        static volatile int *p_m2d_rb_free_sz = NULL;
        free_size = share_mem_var_val_get("m2d_rb_free", &p_m2d_rb_free_sz, MAILBOX_CMD_AUDIO_DAC_FREE_BUF_SIZE_CHECK);
    }
    #else
    uint32_t dsp2mcu_ack_data[4] = {0};
    uint32_t cmd = (as_type == ASO_TYPE_USB) ? USR_MBX_CMD_AUDIO_AS_USBO_BUF_FREE_CHK : MAILBOX_CMD_AUDIO_DAC_FREE_BUF_SIZE_CHECK;
    mbx_mcu2dsp_transfer(cmd | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    free_size = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    #endif

    return free_size;

#endif
}
#endif

void aud_dac_fill_buffer(uint8_t *buff, uint16_t size)
{
#ifdef AUD_M2D_MCU_RINGBUF_EN
    // aud_bt_a2dp_ringbuff_write(buff, size);
    if(app_is_bt_mode())
    {
#ifdef BT_AUDIO_SYNC_DEBUG
        #define TEST_MARK_BEGIN     REG_GPIO_0x0E = 2;
        #define TEST_MARK_END       REG_GPIO_0x0E = 0;
#else
        #define TEST_MARK_BEGIN   //  REG_GPIO_0x0E = 2;
        #define TEST_MARK_END     //  REG_GPIO_0x0E = 0;
#endif
        int32_t pcm[141*2];//(128*480/441+1 + 1)*2ch
        int samples_in = size >> 3;
        int smps_out;
        #if BT_A2DP_SRC248_EN
        int smps_subsect = bt_aso_src_s.max_smps_in;
        #else
        int smps_subsect = 128;
        #endif
        int i;
        TEST_MARK_BEGIN
        if(samples_in % smps_subsect) {
            samples_in *= samples_in / smps_subsect;
            LOG_E(ASO, "[AS ERROR] %d %s()\n", __LINE__, __FUNCTION__);//for 128pts for sbc. 1024pts for aac
        }
        for(i = 0; i < samples_in; i += smps_subsect)//for bt_src_buff size limit, spilt process
        {
        #if BT_A2DP_SRC248_EN
            smps_out = audio_src_apply(&bt_aso_src_s, (void*)((int32_t*)buff + 2*i), (void*)pcm, smps_subsect);
        #else
            memcpy(pcm, (int32_t*)buff + 2*i, smps_subsect << 3);
            smps_out = smps_subsect;
        #endif
        #if (BT_AUD_SYNC_ADJ_BY_SW == 1)
            smps_out = aud_async_src_proc(aud_sync_bt_hdl_get(), pcm, smps_out);
            #ifdef BT_AUDIO_SYNC_DEBUG
            // if(smps_out != smps_subsect) LOG_I(ASO, " +-+- audio sync comp:%d\n", smps_out);
            #endif
        #endif
            ASO_DEF_CRITICAL_RUN(1);
            ring_buffer_write(&def_aso_ringbuf, (uint8_t*)pcm, smps_out << 3);
            ASO_DEF_CRITICAL_RUN(0);
        }
        TEST_MARK_END
    }
    else
    {
        ASO_DEF_CRITICAL_RUN(1);
        ring_buffer_write(&def_aso_ringbuf, buff, size);
        ASO_DEF_CRITICAL_RUN(0);
    }

#else
#ifdef AUD_WAV_TONE_SEPARATE
    aud_aso_ringbuf_write(buff, size, ASO_TYPE_BT);
#else
    aud_aso_ringbuf_write(buff, size, (app_wave_playing ()) ? ASO_TYPE_WAV : ASO_TYPE_BT);
#endif
#endif
#ifndef BT_AUDIO_SYNC_DEBUG
    if(aud_sync_log_is_on())
#endif
    {
        static int cnt = 0;
        cnt += size;
        if(cnt >= 240000*4*2)//5sec
        {
            os_printf("dac smps acc:%d, cur:%d\n", cnt/8, size/8);
            aud_sync_bt_debug();
            cnt = 0;
        }
    }
}


/******************************usb audio out********************************/
/******************************usb audio out********************************/
/******************************usb audio out********************************/
#ifdef AUD_M2D_MCU_RINGBUF_EN

int usb_out_ringbuff_read(uint8_t *buff, int size)
{
    return ring_buffer_read(&usb_aso_rb, (uint8_t*)buff, (uint32_t)size);
}

int usb_out_ringbuff_write(uint8_t *buff, int size)
{
    return ring_buffer_write(&usb_aso_rb, buff, size);
}

int usb_out_ringbuff_free_get(void)
{
    return ring_buffer_get_free_size(&usb_aso_rb);
}

int usb_out_ringbuff_fill_get(void)
{
    return ring_buffer_get_fill_size(&usb_aso_rb);
}

void usb_out_ringbuff_clear(void)
{
    ring_buffer_reset(&usb_aso_rb);
}

//calc the next sample num src
int src_4ms_smps_get(int sample_rate)
{
    int samples;
    static uint8_t src_frame_cnt = 0;
    switch (sample_rate)
    {
    case 44100://4ms, 176.4->192, 176.4=(176*3+177*2)/5
        if(++src_frame_cnt > 5) src_frame_cnt = 1;
        samples = (src_frame_cnt <= 3) ? 176 : 177;
        break;
    case 22050://4ms, 88.2->192, 88.2=(88*4+89)/5
        if(++src_frame_cnt > 5) src_frame_cnt = 1;
        samples = (src_frame_cnt <= 4) ? 88 : 89;
        break;
    // case 11025://4ms, 44.1->192, 44.1=(44*9+45)/10
    //     if(++src_frame_cnt > 10) src_frame_cnt = 1;
    //     samples = (src_frame_cnt <= 9) ? 44 : 45;
        break;
    case 48000: samples = 192;  break;
    case 32000: samples = 128;  break;
    case 16000: samples = 64;   break;
    case  8000: samples = 32;   break;
    default: samples = 0;       break;
    }
    return samples;
}

#ifndef AUDIO_MCU2DSP_INVALID
#if !(defined(ASO_DEF_TRANS_BY_SHARE_RB) && defined(ASO_USB_TRANS_BY_SHARE_RB)) 
//move audio data from ringbuff to dsp
void audio_mcu2dsp_play(void)
{
    int32_t pcm[SAMPLES_PER_FRAME * 2];

    if(!audio_aso_type_get(-1)) goto RET;
    if(mailbox_mcu2dsp_is_busy()) goto RET;

////////////////////////////////////////////// bt/sd/udisk
#ifndef ASO_DEF_TRANS_BY_SHARE_RB
    // if(a2dp_has_music())
    if(audio_aso_type_get(ASO_TYPE_DEF))
    {
        #define DSP_MUSIC_RB_SZ     16384
        static int bytes_per_frame = SAMPLES_PER_FRAME * 8;//32bit2ch
        if((ring_buffer_get_fill_size(&def_aso_ringbuf) >= bytes_per_frame)
        && (aud_aso_ringbuf_free_get(ASO_TYPE_DEF) >= bytes_per_frame + (DSP_MUSIC_RB_SZ - (SAMPLES_PER_FRAME*4*2*2))))//4ms*4byte*2ch*2frame
        {
            ring_buffer_read(&def_aso_ringbuf, (uint8_t*)pcm, bytes_per_frame);
            aud_aso_ringbuf_write((uint8_t*)pcm, bytes_per_frame, ASO_TYPE_DEF);
            bytes_per_frame = src_4ms_smps_get(aud_dac_sample_rate_cur_get()) * 8;//32bit2ch
            #ifdef BT_AUDIO_SYNC_DEBUG
            // static int loop = 0; if(++loop >= 1000){ loop = 0; LOG_I(ASO, "def rb fill smps:%d, all:%d\n", ring_buffer_get_fill_size(&def_aso_ringbuf) >> 3, def_aso_ringbuf.capacity >> 3); }//debug
            // static int smps_rec_buf[5]; if(loop < 5) smps_rec_buf[loop] = bytes_per_frame >> 3;
            // if(loop == 0) LOG_I(ASO, "def rb smps out:%d, %d, %d, %d, %d\n", smps_rec_buf[0], smps_rec_buf[1], smps_rec_buf[2], smps_rec_buf[3], smps_rec_buf[4]);
            #endif
        }
    }
#endif //ASO_DEF_TRANS_BY_SHARE_RB

////////////////////////////////////////////// usb out
#ifndef ASO_USB_TRANS_BY_SHARE_RB
    if(audio_aso_type_get(ASO_TYPE_USB))
    {
        #define bytes_per_frame_usb     (SAMPLES_PER_FRAME*4)//16bit2ch, usb 48K fixed
        if((usb_out_ringbuff_fill_get() >= bytes_per_frame_usb)
        && (aud_aso_ringbuf_free_get(ASO_TYPE_USB) >= bytes_per_frame_usb))
        {
            usb_out_ringbuff_read((uint8_t*)pcm, bytes_per_frame_usb);
            aud_aso_ringbuf_write((uint8_t*)pcm, bytes_per_frame_usb, ASO_TYPE_USB);
            // static int loop = 0; if(++loop >= 1000){ loop = 0; LOG_I(ASO, "aso usb loop\n"); }//debug
        }
    }
#endif //ASO_USB_TRANS_BY_SHARE_RB

RET:
    return;
}
#endif
#endif// AUDIO_MCU2DSP_INVALID


#endif // AUD_M2D_MCU_RINGBUF_EN



/******************************usb audio in********************************/
/******************************usb audio in********************************/
/******************************usb audio in********************************/
#ifdef AUD_D2M_MCU_RINGBUF_EN

int usb_in_ringbuff_read(uint8_t *buff, int size)
{
    return ring_buffer_read(&usb_asi_rb, (uint8_t*)buff, (uint32_t)size);
}

int usb_in_ringbuff_write(uint8_t *buff, int size)
{
    return ring_buffer_write(&usb_asi_rb, buff, size);
}

int usb_in_ringbuff_free_get(void)
{
    return ring_buffer_get_free_size(&usb_asi_rb);
}

int usb_in_ringbuff_fill_get(void)
{
    return ring_buffer_get_fill_size(&usb_asi_rb);
}

void usb_in_ringbuff_clear(void)
{
    ring_buffer_reset(&usb_asi_rb);
}
#endif // AUD_D2M_MCU_RINGBUF_EN

#ifdef CONFIG_AUD_AMP_DET_FFT
__attribute__((weak)) void aud_d2m_pcm16_LR_for_fft(int16_t *pcm, int samples) {}
#endif
__attribute__((weak)) void audio_rec_proc_stereo(int16_t *pcm, int samples) {}
__attribute__((weak)) void user_audio_rec_proc_stereo(int16_t *pcm, int samples) {}

#ifndef AUDIO_MCU2DSP_INVALID
#if 1//(defined(ASI_DEF_TRANS_BY_SHARE_RB) || defined(ASI_USB_TRANS_BY_SHARE_RB))
void audio_dsp2mcu_record(void)
{
    int16_t buf[SAMPLES_PER_FRAME*2];//2ch
    int aud_fra_smps = SAMPLES_PER_FRAME;
    int aud_fra_size = def_asi_Smp2Byte(SAMPLES_PER_FRAME);

    if(aud_asi_ringbuf_fill_get(ASI_TYPE_DEF) < aud_fra_size) goto RET;

    // static int loop = 0; if(++loop >= 1000){ loop = 0; LOG_I(AS, "rec loop\n"); }//debug

//// get audio data from share rb
    aud_asi_ringbuf_read((uint8_t*)buf, aud_fra_size, ASI_TYPE_DEF);

//// audio record process
    audio_rec_proc_stereo(buf, aud_fra_smps);

#ifdef CONFIG_AUD_AMP_DET_FFT
//// audio amp detect by fft
    aud_d2m_pcm16_LR_for_fft(buf, aud_fra_smps);
#endif

//// provide audio data information for user, used to ui,etc...
#ifdef CONFIG_AUD_REC_AMP_DET
    user_audio_rec_proc_stereo(buf, aud_fra_smps);
#endif
RET:
    return;
}
#else
#if !defined(ASI_DEF_TRANS_BY_SHARE_RB)
static int asi_def_rb_fill_size(void)
{
#if 1
    static volatile int *p_d2m_rb_fill_sz = NULL;
    return share_mem_var_val_get("d2m_rb_fill", &p_d2m_rb_fill_sz, MAILBOX_CMD_AUDIO_ADC_FILL_BUF_SIZE_CHECK);
#else
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_ADC_FILL_BUF_SIZE_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, as_type, &dsp2mcu_ack_data[0]);
    return dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
#endif
}

static int asi_def_rb_read(unsigned char *buff, int size)
{
    uint32_t ret=0;
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_ADC_BUF_READ | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, (uint32_t)size, 0, &dsp2mcu_ack_data[0]);
    ret = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
    if(mic_is_mute) memset(buff, 0, size);
    return ret;
}
#endif

//get audio stream data from dsp and process, don't call frequently, recommended call interval: 2ms
void audio_dsp2mcu_record(void)
{
    int16_t buf[SAMPLES_PER_FRAME*2];//2ch
    // int frame_samples = SAMPLES_PER_FRAME;
    int frame_bytes = SAMPLES_PER_FRAME << 2;//16bit2ch

    if(mailbox_mcu2dsp_is_busy()) goto RET;
    if(asi_def_rb_fill_size() < frame_bytes) goto RET;

    // static int loop = 0; if(++loop >= 1000){ loop = 0; LOG_I("bt loop\n"); }//debug

////////////////////////////////////////////// get audio data from dsp
    asi_def_rb_read((uint8_t*)buf, frame_bytes);

////////////////////////////////////////////// write data to default channel for audio record to sdcard/udisk
#if defined(AUD_D2M_MCU_RINGBUF_EN) && (!defined(ASI_DEF_TRANS_BY_SHARE_RB))
    // memset(buf, 0, frame_bytes);
    if(ring_buffer_get_free_size(&def_asi_ringbuf) >= frame_bytes){
        ring_buffer_write(&def_asi_ringbuf, (uint8_t*)buf, (uint32_t)frame_bytes);
    }else{
        rec_rb_overflow_cnt++;
        ring_buffer_reset(&def_asi_ringbuf);
    }
#endif

////////////////////////////////////////////// write data to usb in channel
#if defined(AUD_D2M_MCU_RINGBUF_EN) && (!defined(ASI_USB_TRANS_BY_SHARE_RB))
    if(usb_in_ringbuff_free_get() >= frame_bytes){
        usb_in_ringbuff_write((uint8_t*)buf, frame_bytes);
        // static int loop = 0; if(++loop >= 1000){ loop = 0; LOG_I(ASI, "usbi loop\n"); }//debug
    }
#endif

////////////////////////////////////////////// provide audio data information for user, used to ui,etc...
#ifdef CONFIG_AUD_REC_AMP_DET
    user_audio_rec_proc_stereo(buf, SAMPLES_PER_FRAME);
#endif
RET:
    return;
}
#endif
#endif //AUDIO_MCU2DSP_INVALID


#else

void aud_dac_fill_buffer( uint8_t *buff, uint16_t size )
{
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    aud_dac_volume_control( buff, size);
    rb_fill_buffer(&audio_dac_ctrl_blk.aud_rb, buff, size, DAC_FILL_WITH_COMP);
    #else
    aud_dac_volume_control( buff, size);
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_DAC_BUF_WRITE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buff, (uint32_t)size, 0);
    #endif
    #ifdef BT_AUDIO_SYNC_DEBUG
    static int cnt = 0;
    cnt += size;
    if(cnt >= 48000*4*2)//1sec
    {
        os_printf("dac point all:%d, cur:%d\t", cnt/8, size/8);
        cnt = 0;
    }
    #endif
}
#endif //USER_KARAOK_MODE


void DRAM_CODE adio_sco_fill_buffer(uint8_t *buff, uint8_t fid,uint16_t size )
{
#if (CONFIG_HFP_SPK_EQ == 1)
	hfp_spk_eq_proc((uint8 *)buff, size);
#endif

    uint16_t i = 0;
    int32_t  buff_st[240];
    int32_t* dst = (int32_t*)buff_st;
    uint8_t* src = (uint8_t*)buff;

    while (i < size)
    {
        int32 t = (*(src + i) << 16) | (*(src + i + 1) << 24);

        t >>= 8;
        *dst++ = t;
        *dst++ = t;
        i += 2;
    }

    aud_dac_fill_buffer((uint8_t*)buff_st, size * 4);
}

uint16_t aud_dac_get_fill_buffer_size(void)
{
#ifdef USER_KARAOK_MODE
    #ifdef AUD_M2D_MCU_RINGBUF_EN
    uint16_t size;
    // ASO_DEF_CRITICAL_RUN(1);
    size = ring_buffer_get_fill_size(&def_aso_ringbuf);
    // ASO_DEF_CRITICAL_RUN(0);
    #else
    size = aud_aso_ringbuf_fill_get(ASO_TYPE_BT);
    #endif
    return size;
#else
    #if CONFIG_DAC_CTRL_MODE ==DAC_CTRL_BY_MCU
    return rb_get_buffer_fill_size(&audio_dac_ctrl_blk.aud_rb);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_DAC_FILL_BUF_SIZE_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0);
    return mailbox_dsp2mcu_ack_get(MAILBOX_CMD_REG_IDX_PARAM0);
    #endif
#endif
}

uint16_t aud_dac_get_free_buffer_size(void)
{
#ifdef USER_KARAOK_MODE

#ifdef AUD_M2D_MCU_RINGBUF_EN
    // ASO_DEF_CRITICAL_RUN(1);
    int size = ring_buffer_get_free_size(&def_aso_ringbuf);
    // ASO_DEF_CRITICAL_RUN(0);
    #if 0//BT_A2DP_SRC248_EN || (BT_AUD_SYNC_ADJ_BY_SW == 1)
    if(app_is_bt_mode() && (A2DP_CODEC_MPEG_AAC == a2dp_get_codec_type()))
    {
        #if BT_A2DP_SRC248_EN
        size = size * 441 / 480;
        #endif
        #if (BT_AUD_SYNC_ADJ_BY_SW == 1)
        uint16_t sync_size = (1024 / 128 * 8);//for aac sync compesate one point
        size = (size > sync_size) ? (size - sync_size) : 0;
        #endif
    }
    #endif
    return (uint16_t)size;
#else
    return aud_aso_ringbuf_free_get(ASO_TYPE_BT);
#endif

#else
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    return rb_get_buffer_free_size(&audio_dac_ctrl_blk.aud_rb);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_DAC_FREE_BUF_SIZE_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0);
    return mailbox_dsp2mcu_ack_get(MAILBOX_CMD_REG_IDX_PARAM0);
    #endif
#endif
}

void aud_dac_buffer_clear(void)
{
#ifdef AUD_M2D_MCU_RINGBUF_EN
    ring_buffer_reset(&def_aso_ringbuf);
#else
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    ring_buffer_clear(&audio_dac_ctrl_blk.aud_rb);
    #else
    //TODO
    #endif
#endif
}



int audio_asi_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample, AUD_AS_TYPE_e as_type)
{
    audio_asi_type_set(as_type, 0);

    if(as_type == ASI_TYPE_USB)
    {
        #if defined(ASI_USB_TRANS_BY_SHARE_RB)
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_AS_USBI_CONFIG | MAILBOX_CMD_FAST_RSP_FLAG, freq, channels, bits_per_sample, NULL);
        #endif
    }
    else
    {
        #if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
        #if AUD_CFG_ADC_SEL
        audio_anc_sample_rate_set(freq);
        #else
        audio_adc_sample_rate_set(freq);
        #endif
        audio_adc_ctrl_blk.channels = channels;
        rb_init(&audio_adc_ctrl_blk.aud_rb,(uint8_t *)audio_adc_ctrl_blk.data_buff, AUDIO_ADC_BUFF_LEN, 2, audio_adc_ctrl_blk.dma_handle);
        #else
        mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_ADC_CONFIG | MAILBOX_CMD_FAST_RSP_FLAG, freq, channels, bits_per_sample, NULL);
        #endif
    }
    LOG_I(DRV, "audio_asi_config(%d, %d, %d, 0x%x)\n",freq, channels, bits_per_sample, as_type);
    return 0;
}

void audio_asi_open(uint8_t enable, AUD_AS_TYPE_e as_type)
{
    if(audio_asi_type_get(as_type)) return;

    if(enable == 0) audio_asi_type_set(as_type, enable);
    if(as_type == ASI_TYPE_USB)
    {
    #if USB_AUDIO_FUNC_VALID
        #if defined(ASI_USB_TRANS_BY_SHARE_RB)
        uint32_t asrc_en = (CONFIG_AUD_SYNC_USBI == 2) ? enable : 0;
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_AS_USBI_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, enable, asrc_en, 0, NULL);//intf_en, asrc_en
        #endif
    #endif
    }
    else
    {
        #if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
        #if AUD_CFG_ADC_SEL
        audio_anc_enable(enable);
        #else
        audio_adc_enable(enable);
        #endif
        dma_channel_enable(audio_adc_ctrl_blk.dma_handle, enable);
        #else
        mbx_mcu2dsp_transfer(MAILBOX_CMD_AUDIO_ADC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, enable, 0, 0, NULL);
        #endif
        mic_is_mute=(!enable);
    }
    if(enable == 1) audio_asi_type_set(as_type, enable);
    LOG_I(DRV, "audio_asi_open(%d, 0x%x)\n", enable, as_type);
}

int aud_mic_config(uint32_t freq, uint32_t channels, uint32_t bits_per_sample)
{
    return audio_asi_config(freq, channels, bits_per_sample, ASI_TYPE_DEF);
}

void aud_mic_open(int enable)
{
    audio_asi_open(enable, ASI_TYPE_DEF);
}

void aud_mic_set_volume(uint8_t volume)
{

}

void aud_mic_mute(uint8_t enable)
{
    mic_is_mute=enable;
}

#ifdef USER_KARAOK_MODE

int aud_asi_ringbuf_fill_get(AUD_AS_TYPE_e as_type)
{
    if(as_type == ASI_TYPE_USB)
    {
        return ring_buffer_get_fill_size(&usb_asi_rb);
    }
    else
    {
        return ring_buffer_get_fill_size(&def_asi_ringbuf);
    }
}

//audio stream input
int aud_asi_ringbuf_read(unsigned char *buff, int size, AUD_AS_TYPE_e as_type)
{
    if(as_type == ASI_TYPE_USB)
    {
        return ring_buffer_read(&usb_asi_rb, (uint8_t*)buff, (uint32_t)size);
    }
    else
    {
        return ring_buffer_read(&def_asi_ringbuf, (uint8_t*)buff, (uint32_t)size);
    }
}
#endif

#ifdef AUD_D2M_MCU_RINGBUF_EN

uint16_t aud_mic_get_fill_buffer_size(void)
{
    return (uint16_t)aud_asi_ringbuf_fill_get(ASI_TYPE_DEF);
}

uint32_t aud_mic_read_buffer(uint8_t* buf, uint16_t len)
{
    return aud_asi_ringbuf_read(buf, len, ASI_TYPE_DEF);
}

#else

uint16_t aud_mic_get_fill_buffer_size(void)
{
    #if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
    return rb_get_buffer_fill_size(&audio_adc_ctrl_blk.aud_rb);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_ADC_FILL_BUF_SIZE_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0);
    return mailbox_dsp2mcu_ack_get(MAILBOX_CMD_REG_IDX_PARAM0);
    #endif
}

uint32_t aud_mic_read_buffer(uint8_t* buf, uint16_t len)
{
    uint32_t rt=0;
    #if CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_MCU
    rt= rb_read_buffer(&audio_adc_ctrl_blk.aud_rb, buf, len);
    #else
    mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_AUDIO_ADC_BUF_READ | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)buf, (uint32_t)len, 0);
    rt= mailbox_dsp2mcu_ack_get(MAILBOX_CMD_REG_IDX_PARAM2);
    #endif
    if(mic_is_mute)
    {
      memset(buf,0,len);
    }
    return rt;
}
#endif

uint8_t DRAM_CODE aud_discard_sco_data(void)
{
    if(app_bt_flag1_get( APP_FLAG_WAVE_PLAYING))
    {
        return 1;
    }
	else if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DISABLE_IOS_INCOMING_RING)
		&& app_bt_flag2_get(APP_FLAG2_HFP_INCOMING)
		&& !app_bt_flag1_get( APP_FLAG_CALL_ESTABLISHED))
	{
		return 1;
	}
    else
    {
        return 0;
    }
}

void aud_volume_mute( uint8_t enable )
{
    //TODO
}

/******************************************************/

void extPA_set_req(uint8_t open_req)    { /* TODO */ }
void extPA_open(uint8_t delay_enable)   { /* TODO */ }
void extPA_close(uint8_t delay_enable)  { /* TODO */ }

/******************************************************/

#define MUTE_LOW_THD        2
#define MUTE_HIGH_THD       7

static aud_mute_cfg_t aud_mute_cfg;
static int8_t   mute_fast_shift = 6; // range: 3 - 10
static int8_t   mute_slow_shift = 9; // range: 5 - 12
static uint8_t  mute_state      = 0;
static uint32_t mute_amp        = 0;

void aud_mute_init( void )
{
    app_env_handle_t env_h = app_env_get_handle();

    if( env_h->env_cfg.used == 0x01 )
    {
        aud_mute_cfg.mute_pin = app_env_get_pin_num(PIN_paMute);
        aud_mute_cfg.mute_high_flag = app_env_get_pin_valid_level(PIN_paMute);
        aud_mute_cfg.mute_status = 2;
        aud_mute_cfg.mute_outside = app_env_get_pin_enable(PIN_paMute);
        aud_mute_cfg.shade_flag = 0;//((env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_SHADE_OUT) >> 10 );
		aud_mute_cfg.auto_mute_flag = (!!(env_h->env_cfg.feature.feature_flag & APP_ENV_FEATURE_FLAG_FAST_MUTE));
    }
    else
    {
        aud_mute_cfg.mute_pin       = PAMUTE_GPIO_PIN;
        aud_mute_cfg.mute_high_flag = 0;
        aud_mute_cfg.mute_status    = 2;
        aud_mute_cfg.mute_outside   = 0;
        aud_mute_cfg.shade_flag     = 0;
        aud_mute_cfg.auto_mute_flag = 0;
    }

    if( aud_mute_cfg.mute_outside )
    {
        gpio_config(aud_mute_cfg.mute_pin, 1);
        gpio_output(aud_mute_cfg.mute_pin, !!aud_mute_cfg.mute_high_flag);
    }
}

void aud_mute_func_init(int8_t fast_shift, int8_t slow_shift)
{
    mute_fast_shift = fast_shift;
    mute_slow_shift = slow_shift;
}

void aud_mute_update( int16_t samplel, int16_t sampler )
{
    if(aud_mute_cfg.auto_mute_flag)
    {
        int8_t   mute_shift;
        uint32_t abs_samplel = (ABS(samplel) << 16);
        uint32_t abs_sampler = (ABS(sampler) << 16);
        uint32_t mute_tmp    = mute_amp;
        uint32_t abs_sample  = (abs_samplel >> 1) + (abs_sampler >> 1);

        mute_shift = abs_sample > mute_amp ? mute_fast_shift : mute_slow_shift;

        mute_amp -= (mute_amp >> mute_shift);
        mute_amp += (abs_sample >> mute_shift);

        mute_tmp = mute_amp >> 16;

        if(mute_tmp > MUTE_HIGH_THD && mute_state == 0)
        {
            mute_state = 1;
            aud_PAmute_operation(0);
            app_bt_flag2_set(APP_FLAG2_MUTE_FUNC_MUTE, 0);
        }
        else if(mute_tmp < MUTE_LOW_THD && mute_state == 1)
        {
            mute_state = 0;
            aud_PAmute_operation(1);
            app_bt_flag2_set(APP_FLAG2_MUTE_FUNC_MUTE, 1);
        }
    }
}

/*  1 -- direct PA mute  0 -- delay PA unmute */
void aud_PAmute_operation(int enable)
{
    if(!aud_mute_cfg.mute_outside) return;

    if(enable && (aud_mute_cfg.mute_status != 1))
    {
        //os_delay_ms(0);
       if(s_audio_open_stat) return;//Can't mute if DAC is opening
        if(app_get_audio_task_handle()!= NULL) jtask_stop(app_get_audio_task_handle());

        if(aud_mute_cfg.mute_high_flag)  // bit7 = 1: high effect
            gpio_output(aud_mute_cfg.mute_pin, 1);
        else
            gpio_output(aud_mute_cfg.mute_pin, 0);
	os_printf("PA mute\r\n");
        mute_state = 0;
    }
    else if((0 == enable) && (aud_mute_cfg.mute_status != 0))
    {
        if(app_bt_flag1_get(APP_FLAG_POWERDOWN) ||
         !s_audio_open_stat)//Can't unmute if DAC is closing
        		return;
        if(aud_mute_cfg.mute_high_flag)  // bit7 = 1: high effect
            gpio_output(aud_mute_cfg.mute_pin, 0);
        else
            gpio_output(aud_mute_cfg.mute_pin, 1);
	 os_printf("PA unmute\r\n");
        mute_state = 1;
    }

    aud_mute_cfg.mute_status = enable;

    return;
}

void aud_PAmute_delay_operation(int enable)
{
    app_env_handle_t env_h = app_env_get_handle();
    if (enable)
    {
        aud_PAmute_operation(1);
        if(env_h->env_cfg.feature.pa_mute_delay_time) os_delay_ms((10*env_h->env_cfg.feature.pa_mute_delay_time));
    }
    else
    {
        if(env_h->env_cfg.feature.pa_unmute_delay_time) os_delay_ms((10*env_h->env_cfg.feature.pa_unmute_delay_time));
        aud_PAmute_operation(0);
    }
}

/******************************************************/

#if(CONFIG_AUD_FADE_IN_OUT == 1)
int16_t s_fade_scale = 0;
static t_aud_fade_state s_aud_fade_status = AUD_FADE_NONE;

__INLINE__ void  set_aud_fade_in_out_state(t_aud_fade_state state)
{
    if(state == AUD_FADE_IN)
        s_fade_scale = AUD_FADE_SCALE_MIN;
    else if(state == AUD_FADE_OUT)
        s_fade_scale = AUD_FADE_SCALE_MAX;
    s_aud_fade_status = state;
}

__INLINE__ t_aud_fade_state get_aud_fade_in_out_state(void)
{
    return s_aud_fade_status;
}

void aud_fade_status_debug(void)
{
    os_printf("---------Fade in/out status------------\r\n");
    os_printf("| Fade in/out state:%d\r\n",s_aud_fade_status);
    os_printf("| Fade in/out step :%d\r\n",s_fade_scale);
    os_printf("---------Fade in/out end---------------\r\n");
}

void aud_fade_in_out_process(void)
{
    t_aud_fade_state status = get_aud_fade_in_out_state();

    if(AUD_FADE_IN == status)
    {
        s_fade_scale += AUD_FADE_STEP;
        if(s_fade_scale >= AUD_FADE_SCALE_MAX)
        {
            set_aud_fade_in_out_state(AUD_FADE_FINISHED);
        }
    }
    else if(AUD_FADE_OUT == status)
    {
        s_fade_scale -= AUD_FADE_STEP;
        if(s_fade_scale <= AUD_FADE_SCALE_MIN)
        {
            set_aud_fade_in_out_state(AUD_FADE_FINISHED);
        }
    }
    else
    {
        set_aud_fade_in_out_state(AUD_FADE_NONE);
    }

}
#endif

/******************************************************/

#if A2DP_ROLE_SOURCE_CODE

typedef struct _driver_lineindata_s
{
    driver_ringbuff_t   data_rb;
}DRIVER_LINEINDATA_T;

driver_ringbuff_t *getLineRingbuf(void);

DRIVER_LINEINDATA_T lineinDataBlk;

driver_ringbuff_t *getLineRingbuf(void)
{
	return &lineinDataBlk.data_rb;
}

void RAM_CODE audLineinFillBuffer( uint8_t *buff, uint16_t size )
{
    //os_printf("f.b. %d \r\n", size);
    aud_dac_volume_control( buff, size);
    rb_fill_buffer(getLineRingbuf(), buff, size, LINEIN_FILL);
}

void RAM_CODE dacClkAdjust( int mode )
{
	switch( mode )
	{
	case 1: //fast
		BK3000_AUD_DAC_FRACCOEF -= BK3000_AUD_DAC_FRACCOEF>>16;//15.258ppm
		break;
	case 3: //fast fast
		BK3000_AUD_DAC_FRACCOEF -= BK3000_AUD_DAC_FRACCOEF>>14;// 4*15.258ppm
		break;
	case 2: //slow
		BK3000_AUD_DAC_FRACCOEF += BK3000_AUD_DAC_FRACCOEF>>16;//15.258ppm
		break;
	case 4: //slow slow
		BK3000_AUD_DAC_FRACCOEF += BK3000_AUD_DAC_FRACCOEF>>14;// 4*15.258ppm
		break;
	case 0:
	default:
		if( (BK3000_AUD_AUDIO_CONF & 0x3 ) == 2 ) // 44.1k
			BK3000_AUD_DAC_FRACCOEF = AUDIO_DIV_441K;
		else
			BK3000_AUD_DAC_FRACCOEF = AUDIO_DIV_48K;
		break;
	}
}

#endif

/******************************************************/

#if (CONFIG_APP_MSBC_RESAMPLE == 1)

int16_t fir_coef[NFIR] = {
12,-2,-15,-3,20,12,-26,-27,27,50,-21,-79,0,110,40,-135,-103,144,190,-123,
-296,56,415,75,-536,-303,648,697,-739,-1518,798,5137,7369,5137,798,-1518,
-739,697,648,-303,-536,75,415,56,-296,-123,190,144,-103,-135,40,110,0,
-79,-21,50,27,-27,-26,12,20,-3,-15,-2,12,
};
int16_t b_coef[N_IIR] = {
	125,822,2878,6868,12268,17137,19117,17137,12268,6868,2878,822,125
};
int16_t a_coef[N_IIR] = {
	4096,7223,14909,17593,19093,15553,10841,6022,2727,958,251,44,4
};

int16_t fir_hardcore_init(void)
{
    int16_t i;
    int16_t *coef = (int16_t *)&fir_coef[0];
    BK3000_PMU_PERI_PWDS &= ~bit_PMU_FFT_PWD;
    BK3000_FFT_FFT_CONF = 0x00;
    BK3000_FFT_FIR_CONF = (NFIR+1) | (1L<<sft_FIR_CONF_FIR_ENABLE);
    //BK3000_FFT_FIR_CONF = (CORE_NFIR) | (1L<<sft_FIR_CONF_FIR_MODE) | (1L<<sft_FIR_CONF_FIR_ENABLE);
    for(i=0;i<NFIR;i++)
    {
        BK3000_FFT_COEF_PORT =coef[i];
    }
    return 0;

}
int16_t fir_hardcore_close(void)
{
    BK3000_FFT_FIR_CONF = 0x00;
    BK3000_PMU_PERI_PWDS |= bit_PMU_FFT_PWD;
    return 0;
}
int16_t fir_hardcore_filter(int16_t *smpl,uint16_t nSmpls)
{
	uint16_t i;
	int32_t tmp;
	for(i=0;i<nSmpls;i++)
	{
        BK3000_FFT_DATA_PORT = smpl[i];
        BK3000_FFT_FFT_START = 0x01;
        while(!(BK3000_FFT_FFT_STATUS & bit_FFT_STATUS_FIR_DONE));
        tmp = BK3000_FFT_DATA_PORT;
        smpl[i] = (tmp & 0xffff);
	}
	return 0;
}
int16_t FIR_sw_filter_init(pFIR_STATE p_fir_st)
{
    unsigned int i;
	p_fir_st->coef = &fir_coef[0];
	p_fir_st->taps = NFIR;
	p_fir_st->curr_pos = NFIR - 1;
	p_fir_st->ratio = 0;
	for(i=0;i<NFIR;i++)
	{
		p_fir_st->laststate[i] = 0;
	}
	return 0;
}
static int16_t fir_sw_filter_sample(pFIR_STATE pfir_stat,int16_t smpl)
{
	int16_t i;
    int32_t y;
    int16_t offset1;
    int16_t offset2;
    pfir_stat->laststate[pfir_stat->curr_pos] = smpl;
    offset2 = pfir_stat->curr_pos;
    offset1 = pfir_stat->taps - offset2;
    y = 0;
    for (i = pfir_stat->taps - 1;  i >= offset1;  i--)
        y += pfir_stat->coef[i]*pfir_stat->laststate[i - offset1];

    for (  ;  i >= 0;  i--)
        y += pfir_stat->coef[i]*pfir_stat->laststate[i + offset2];

    if (pfir_stat->curr_pos <= 0)
    	pfir_stat->curr_pos = pfir_stat->taps;
    pfir_stat->curr_pos--;
    y >>= 14;
	if(y > 32767)
	    y = 32767;
	else if(y < -32768)
	    y = -32768;
    return  y;
}
int16_t FIR_sw_filter_exe(pFIR_STATE p_fir_st,int16_t *in_smpl,int16_t *out_smpl,int16_t len)
{
    int16_t i;
    for(i=0;i<len;i++)
    {
        out_smpl[i] = fir_sw_filter_sample(p_fir_st,in_smpl[i]);
    }
    return 0;
}
void IIR_filter_init(t_IIR_State *st)
{
	int16_t i;
	st->a = (int16_t *)&a_coef[0];
	st->b = (int16_t *)&b_coef[0];
	for(i=0;i<N_IIR;i++)
	{
		st->y[i] = 0;
		st->x[i] = 0;
	}
}
void IIR_filter_exe(t_IIR_State *st,int16_t *x,int16_t *y,int16_t len)
{
	int16_t i,k;
	int32_t smpl_tmp;
	for(i=0; i<len; i++)
  {
   // Shift the register values.
	for(k=N_IIR-1; k>0; k--)st->x[k] = st->x[k-1];
    for(k=N_IIR-1; k>0; k--)st->y[k] = st->y[k-1];

   st->x[0] = x[i];
   smpl_tmp = st->b[0] * st->x[0];
   for(k=1; k<N_IIR; k++)
   {
     smpl_tmp += (st->b[k] * st->x[k] - st->a[k] * st->y[k]);
   }
   smpl_tmp >>= 12;
	if(smpl_tmp > 32767)
		smpl_tmp = 32767;
	else if(smpl_tmp < -32768)
		smpl_tmp = -32768;

   st->y[0] = smpl_tmp;
	// a[0] should = 1
   y[i] = st->y[0];
  }
}

#endif
