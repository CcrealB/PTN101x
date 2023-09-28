/**
 * **************************************************************************************
 * @file    drv_spdif.c
 * 
 * @author  Borg Xiao
 * @date    modify @20230629, bug fixd: audio loss det invalid issue
 * @date    20230217
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include <string.h>
#include "config.h"

#ifdef CONFIG_SPDIF_DRV

#include "bkreg.h"
#include "driver_beken_includes.h"
#include "drv_spdif.h"


#define FS_ERR_LIMIT_Hz         1000 //spdif sample rate detect err max(unit:Hz)

#define SPDIF_DRV_LOG_D(fmt,...)        //SPDIF_PRINTF("[SPD|I]"fmt, ##__VA_ARGS__)

//---------------------------------------- private func prototype


void spdif_init(SPDIF_CFG_CTX_t *p_spdif_st)
{
    SPDIF_DRV_LOG_D("%s(GPIO%d)\n", __FUNCTION__, p_spdif_st->io);

    system_mem_clk_enable(SYS_MEM_CLK_SPDIF);
    system_peri_clk_enable(SYS_PERI_CLK_SPDIF);
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_SPDIF);

    switch(p_spdif_st->cfg_idx)
    {
    case 0: //auto detect
        REG_SPDIF_CONFIG1 = 0x00000000;
        break;
    case 1: //manual detect
        REG_SPDIF_CONFIG1 = /* REG0x0 */
            ((11 & 0xFF) << 0)      /* spdif_pulse_width(bi phase bit) -> 3 bi phase bit width(count value is base on system period) */
            | ((1 & 0x1) << 8)      /* spdif sample rate detect -> 0:auto/1:manuel detect */
            | ((5 & 0x7FF) << 10)   /* min spdif pulse width set -> set 1 bi phase bit width threshold */
            | ((9 & 0x7FF) << 21);  /* max spdif pulse width set -> set 3 bi phase bit width threshold */
        break;
    case 2: //manual detect
        REG_SPDIF_CONFIG1 = /* REG0x0 */
            ((51 & 0xFF) << 0)      /* spdif_pulse_width(bi phase bit) -> 3 bi phase bit width(count value is base on system period) */
            | ((1 & 0x1) << 8)      /* spdif sample rate detect -> 0:auto/1:manuel detect */
            | ((18 & 0x7FF) << 10)  /* min spdif pulse width set -> set 1 bi phase bit width threshold */
            | ((49 & 0x7FF) << 21); /* max spdif pulse width set -> set 3 bi phase bit width threshold */
        break;
    default:
        break;
    }

    REG_SYSTEM_0x1D &= ~MSK_SYSTEM_0x1D_SPDIF_POS;
    REG_SYSTEM_0x40 &= ~(7 << 22);

    switch(p_spdif_st->io)
    {
    case GPIO11:
        REG_SYSTEM_0x1D |= 0 << SFT_SYSTEM_0x1D_SPDIF_POS;
        REG_SYSTEM_0x40 |= 1 << 22;
        break;
    case GPIO12:
        REG_SYSTEM_0x1D |= 1 << SFT_SYSTEM_0x1D_SPDIF_POS;
        REG_SYSTEM_0x40 |= 1 << 23;
        break;
    case GPIO14:
        REG_SYSTEM_0x1D |= 2 << SFT_SYSTEM_0x1D_SPDIF_POS;
        REG_SYSTEM_0x40 |= 1 << 24;
        break;
    default:
        break;
    }

    gpio_config_new(p_spdif_st->io, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
    #ifdef CONFIG_HDMI_CEC
    gpio_config_new(GPIO13, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
    #endif

    REG_SPDIF_CONFIG2 = /* REG0x1 */
        (1 << 1)        /* data (0:not store)/(1:store) in sample buffer*/
        | (0 << 2)      /* interrupt 1:enable/0:disable */
        | (1 << 3)      /* store channel 0:a/1:b status in rx status */
    #if 0
        | (1 << 4)      /* store sample data 0:regradless/1:according valid bit */
        | (1 << 16)     /* 1:Store/0:noStore validity bit in bit 24 in sample buffer, valid len */
        | (1 << 17)     /* 1:Store/0:noStore validity bit in bit 25 in sample buffer, channel */
        | (1 << 18)     /* 1:Store/0:noStore validity bit in bit 26 in sample buffer, user status */
        | (1 << 19)     /* 1:Store/0:noStore validity bit in bit 27 in sample buffer, parity */
        | (1 << 24)     /* 1:Store/0:noStore validity bit in bit 28 in sample buffer, block */
    #endif
        | ((SPDIF_ASI_BIT_WIDTH - 16) << 20); /* data reg store samples bit width */
    REG_SPDIF_CONFIG3 = /* REG0x2 */
        ((4 & 0xFFFF) << 0)     /* range:0~65535 -> 1~65536, detect number of lock then lock */
        | ((3 & 0xFF) << 16)    /* set min spdif pulse width ratio */
        | ((4 & 0xFF) << 24);   /* set max spdif pulse width ratio */
    REG_SPDIF_INTMSK =  /* REG0x7 */
        (0 << 0)        /* spdif signal lock(0:mask,1:no mask) */
        | (0 << 1)      /* y buffer full(0:mask,1:no mask) */
        | (0 << 2)      /* x buffer full(0:mask,1:no mask) */
        | (0 << 3)      /* channel parity error(0:mask,1:no mask) */
        | (0 << 4)      /* channel parity error(0:mask,1:no mask) */
        | (0 << 5)      /* spdif signal unlock(0:mask,1:no mask) */
        | (0 << 6)      /* y buffer overrun(0:mask,1:no mask) */
        | (0 << 7)      /* y buffer underrun(0:mask,1:no mask) */
    #if SPDIF_TRANS_MODE == SPDIF_TRANS_INTR
        | (1 << 8)      /* x buffer overrun(0:mask,1:no mask) */
    #else
        | (0 << 8)      /* x buffer overrun(0:mask,1:no mask) */
    #endif
        | (0 << 9)      /* x buffer underrun(0:mask,1:no mask) */
    #if SPDIF_TRANS_MODE == (SPDIF_TRANS_DMA | SPDIF_TRANS_INTR)
        | (1 << 10);    /* dma finish(0:mask,1:no mask) */
    #else
        | (0 << 10);    /* dma finish(0:mask,1:no mask) */
    #endif

    REG_SPDIF_AUDIO_CONFIG1 |= /* REG0x3 */
        (((p_spdif_st->fifo_thrd - 1) & 0xFF) << 0) /* 0~255 -> 1~256 buffer data */ 
        | (1 << 10);    /* enable only write xy buffer */
}

void spdif_uninit(SPDIF_CFG_CTX_t *p_spdif_st)
{
    gpio_config_new(p_spdif_st->io, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
#ifdef CONFIG_HDMI_CEC
    gpio_config_new(GPIO13, GPIO_HRES, GPIO_PULL_NONE, GPIO_PERI_NONE);
#endif

    REG_SYSTEM_0x1D &= ~MSK_SYSTEM_0x1D_SPDIF_POS;
    REG_SYSTEM_0x40 &= ~(7 << 22);

    system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_SPDIF);
    system_peri_clk_disable(SYS_PERI_CLK_SPDIF);
    system_mem_clk_disable(SYS_MEM_CLK_SPDIF);
}

void spdif_enable(uint8_t enable)
{
    uint32_t reg = REG_SPDIF_CONFIG1;
    reg &= ~MSK_SPDIF_CONFIG1_RECEIVESTART;
    reg |= (!!enable) << SFT_SPDIF_CONFIG1_RECEIVESTART;
    REG_SPDIF_CONFIG1 = reg;
    SPDIF_DRV_LOG_D("spdif_enable:%d\n", enable);
}

void spdif_int_enable(uint8_t enable)
{
    uint32_t reg = REG_SPDIF_CONFIG2;
    reg &= ~MSK_SPDIF_CONFIG2_INTERRUPTENABLE;
    reg |= (!!enable) << SFT_SPDIF_CONFIG2_INTERRUPTENABLE;
    REG_SPDIF_CONFIG2 = reg;
    if (enable == 1)
    	system_peri_mcu_irq_enable(SYS_PERI_IRQ_SPDIF);
    else
    	system_peri_mcu_irq_disable(SYS_PERI_IRQ_SPDIF);
}

void spdif_dma_enable(void* dma, uint8_t enable)
{
    if(dma) dma_channel_enable(dma, !!enable);
    uint32_t reg = REG_SPDIF_AUDIO_CONFIG1;
    reg &= ~MSK_SPDIF_AUDIO_CONFIG1_DMAENABLE;
    reg |= (!!enable) << SFT_SPDIF_AUDIO_CONFIG1_DMAENABLE;
    REG_SPDIF_AUDIO_CONFIG1 = reg;
}

void spdif_dma_init(RingBufferContext *rb_st, uint8_t *rb_buff, int rb_size, uint8_t loop_en)
{
    if(rb_st == NULL || rb_buff == NULL || rb_size == 0){
        SPDIF_LOG_E("param err, rb_st:%p, rb_buff:%p, rb_size:%d\n", rb_st, rb_buff, rb_size);
        return;
    }
	void *dma = (rb_st->dma) ? rb_st->dma : dma_channel_malloc();
    SPDIF_LOG_I("%s use DMA%d:0x%08X, loop_en:%d\n", __FUNCTION__, ((uint32_t)dma - MDU_GENER_DMA_BASE_ADDR) / 0x20, (uint32_t)dma, loop_en);

    dma_channel_config(dma,
                       DMA_REQ_SPDIF_RX,
                       (!!loop_en) ? DMA_MODE_REPEAT : DMA_MODE_SINGLE,
                       (uint32_t)&REG_SPDIF_AUDIO_FIFO_XY,
                       (uint32_t)&REG_SPDIF_AUDIO_FIFO_XY,
                       DMA_ADDR_NO_CHANGE,
                       (SPDIF_ASI_BIT_WIDTH == 16) ? DMA_DATA_TYPE_SHORT : DMA_DATA_TYPE_LONG,
                       (uint32_t)rb_buff,
                       (uint32_t)rb_buff + rb_size,
                       DMA_ADDR_AUTO_INCREASE,
                       DMA_DATA_TYPE_LONG,
					   rb_size
                       );
    ring_buffer_init(rb_st, (uint8_t*)rb_buff, rb_size, dma, RB_DMA_TYPE_WRITE);
    dma_channel_enable(dma, 1);
    dma_channel_enable(dma, 0);
}

void spdif_dma_uninit(void **p_dma)
{
    if(*p_dma) dma_channel_free(*p_dma);
    *p_dma = NULL;
}

int spdif_is_audio(void)
{
    return (!!(REG_SPDIF_STUS2 & MSK_SPDIF_STUS2_AUDIO));
}

//audio signal det invalid process. [for sometimes signal loss but audio det invalid] Borg @230629
void spdif_audio_rcv_check(void)
{
    if(!(REG_SPDIF_STUS2 & MSK_SPDIF_STUS2_AUDIO))
    {
        REG_SPDIF_CONFIG1 &= ~MSK_SPDIF_CONFIG1_RECEIVESTART;
        REG_SPDIF_CONFIG1 |= MSK_SPDIF_CONFIG1_RECEIVESTART;
    }
}

int spdif_fs_det_fine(void)
{
    /*detail sample period detected, spdif one symbol period(may fixed when no spdif signal)
    ('<< 1' for 1 sample period(Frame) = 2 SubFrames(SamplePerChannel/symbol)*/
    int sample_period_fine = ((REG_SPDIF_AUDIO_CONFIG1 >> 16) & 0xFFFF) << 1;
    int sample_rate_Hz = system_apll_freq() / sample_period_fine;
    return sample_rate_Hz;
}

int spdif_fs_det_course(void)
{
    /* rough sample period detected, spdif one data bit width
    ('<< 6' for 1 sample period = 2 SubFrames(SamplePerChannel/symbol) = 64 Data Bits */
    int sample_period_coarse = (REG_SPDIF_STUS1 & 0xFF) << 6;
    int sample_rate_Hz = system_apll_freq() / sample_period_coarse;
    return sample_rate_Hz;
}

/** 
 * @brief get spdif sample rate
 * @return sample rate(unit:Hz) / 0:not surpport
 * */
static int spdif_sample_rate_check(int spdif_fs_det)
{
    int i;
	int spdif_fs = 0;//sample rate in
    int fs_tbl[] = {192000, 176400, 96000, 88200, /*64000, */48000, 44100, 32000, 24000, 22050, 16000, /*11025, 8000,*/ FS_ERR_LIMIT_Hz};

    for(i = 0; i < (sizeof(fs_tbl) / sizeof(int)); i++)
    {
        if((spdif_fs_det > (fs_tbl[i] - FS_ERR_LIMIT_Hz)) && (spdif_fs_det < (fs_tbl[i] + FS_ERR_LIMIT_Hz)))
        {
            spdif_fs = fs_tbl[i];
            break;
        }
    }

    //fs not/err detect
    if(spdif_fs == FS_ERR_LIMIT_Hz) { spdif_fs = 0; }

    if(spdif_fs == 0) SPDIF_DRV_LOG_D("[%d] spdif_fs_det:%d, 0x3:0x%08X, 0x4:0x%08X\n", __LINE__, spdif_fs_det, REG_SPDIF_AUDIO_CONFIG1, REG_SPDIF_STUS1);;
	return spdif_fs;
}

/** 
 * @brief get spdif sample rate
 * @return sample rate(unit:Hz) / 0:not surpport
 * */
int spdif_sample_rate_get(void)
{
    return spdif_sample_rate_check(spdif_fs_det_fine());
}

void spdif_debug_show(void)
{
    SPDIF_LOG_I("cfg: 0x0:0x%08X, 0x1:0x%08X, 0x2:0x%08X, 0x3:0x%08X\n", 
        REG_SPDIF_CONFIG1, REG_SPDIF_CONFIG2, REG_SPDIF_CONFIG3, REG_SPDIF_AUDIO_CONFIG1);
    SPDIF_LOG_I("st:  0x4:0x%08X, 0x5:0x%08X, 0x6:0x%08X, 0x7:0x%08X, 0x8:0x%08X\n", 
        REG_SPDIF_STUS1, REG_SPDIF_STUS2, REG_SPDIF_AUDIO_STUS1, REG_SPDIF_INTMSK, REG_SPDIF_INTSTUS);
}

//convert spdif reg audio data to pcm 32bit2ch
static inline void spdif_aud_cvt_2_pcm24s(void *pcm_in, int32_t *pcm_out, int samples)
{
#if (SPDIF_ASI_BIT_WIDTH == 16)
    int16_t* pcmi = (int16_t*)pcm_in;
    if(samples <= 0) return;
    int i = samples * 2; while(i--) { pcm_out[i] = (int32_t)pcmi[i] << 8; }//pcm16 -> pcm24s
#elif (SPDIF_ASI_BIT_WIDTH == 20)
    int32_t *pcmi = (int32_t*)pcm_in;
    int i; for (i = 0; i < samples * 2; i++) { pcm_out[i] = ((int32_t)pcmi[i] << 12) >> 8; }//pcm24u -> pcm24s
#elif (SPDIF_ASI_BIT_WIDTH == 24)
    int32_t *pcmi = (int32_t*)pcm_in;
    int i; for (i = 0; i < samples * 2; i++) { pcm_out[i] = (((int32_t)pcmi[i] << 8) >> 8) /*&0xF*/; }//pcm24u -> pcm24s, "&0xF if aux data is not audio data"
#endif
}

//convert spdif reg audio data to pcm 16bit2ch
static inline void spdif_aud_cvt_2_pcm16s(void *pcm_in, int16_t *pcm_out, int samples)
{
#if (SPDIF_ASI_BIT_WIDTH == 16)
    memmove(pcm_out, pcm_in, samples * 4);//16bit2ch
#elif (SPDIF_ASI_BIT_WIDTH == 20)
    int32_t *pcmi = (int32_t*)pcm_in;
    int i; for (i = 0; i < samples * 2; i++) { pcm_out[i] = ((int32_t)pcmi[i] << 12) >> 16; }//pcm24u -> pcm24s
#elif (SPDIF_ASI_BIT_WIDTH == 24)
    int32_t *pcmi = (int32_t*)pcm_in;
    int i; for (i = 0; i < samples * 2; i++) { pcm_out[i] = (((int32_t)pcmi[i] << 8) >> 16) /*&0xF*/; }//pcm24u -> pcm24s, "&0xF if aux data is not audio data"
#endif
}

void spdif_audio_fmt_cvt(void *pcm_in, void *pcm_out, int samples, int bits_out)
{
    if(bits_out == 24)
    {
        spdif_aud_cvt_2_pcm24s(pcm_in, (int32_t*)pcm_out, samples);
    }
    else//16bit
    {
        spdif_aud_cvt_2_pcm16s(pcm_in, (int16_t*)pcm_out, samples);
    }
}

int spdif_fifo_clear(void)
{
    int i = 0;
    uint32_t data;
    while (!(REG_SPDIF_AUDIO_STUS1 & (MSK_SPDIF_AUDIO_STUS1_XUNDER | MSK_SPDIF_AUDIO_STUS1_YUNDER)))
    {
        data = REG_SPDIF_AUDIO_FIFO_XY;
        i++;
    }
    (void)data;
    return i;
}

__attribute__((weak)) void spdif_isr(void)
{
#if 0
    uint32_t new_spdif_sample = 0;

#if SPDIF_DMA
#define FRAME_SAMPLES 128
    int32_t i;
    int16_t adc_buffer[FRAME_SAMPLES * 2];
    int32_t dac_buffer[FRAME_SAMPLES * 2];
    if (ring_buffer_get_fill_size(&spdif_ring_buffer) > sizeof(adc_buffer) && aud_dac_get_free_buffer_size() > sizeof(dac_buffer))
    {
        ring_buffer_read(&spdif_ring_buffer, (uint8_t *)adc_buffer, sizeof(adc_buffer));
        for (i = 0; i < FRAME_SAMPLES * 2; i++) dac_buffer[i] = (int32_t)adc_buffer[i] << 8;
        aud_dac_fill_buffer((uint8_t *)dac_buffer, sizeof(dac_buffer));
    }
#else
    if (REG_SPDIF_AUDIO_STUS1 & MSK_SPDIF_AUDIO_STUS1_YNEED)
    {
        for (int i = 0; i < 64; i++)
        {
            REG_ANC_0x4E = REG_SPDIF_AUDIO_FIFO_XY << 8;
            REG_ANC_0x4E = REG_SPDIF_AUDIO_FIFO_XY << 8;
        }
    }
#endif

    new_spdif_sample = get_spdif_sample();
    if (new_spdif_sample != old_spdif_sample)
    {
        old_spdif_sample = new_spdif_sample;
        aud_dac_close();
        aud_dac_config(old_spdif_sample, 2, 16);
        aud_dac_open();
    }
#endif
}

#endif //CONFIG_SPDIF_DRV
