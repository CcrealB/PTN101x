
#include "u_include.h"

#include "u_config.h"
#include "bkreg.h"
#include "driver_beken_includes.h"
#include "app_dsp.h"
//#include "drv_audio.h"
//
//#include "driver_i2s.h"


//must consistent with DSP, i2s0 is not used@220803
typedef enum AUD_IF_CH_e{
    AUD_IF_MIC1 = 0,
    AUD_IF_MIC2,
    AUD_IF_MIC3,
    AUD_IF_MIC4,
    AUD_IF_MIC5,
    AUD_IF_I2S1_I_L,
    AUD_IF_I2S1_I_R,
    AUD_IF_I2S2_I_L,
    AUD_IF_I2S2_I_R,
    AUD_IF_USB_I_L,
    AUD_IF_USB_I_R,
    AUD_IF_MBX_I_L,
    AUD_IF_MBX_I_R,
    AUD_IF_WAV,//prompt tone
    AUIDO_RX_NUM,
    AUD_IF_DAC_O_L = 0,
    AUD_IF_DAC_O_R,
    AUD_IF_I2S1_O_L,
    AUD_IF_I2S1_O_R,
    AUD_IF_I2S2_O_L,
    AUD_IF_I2S2_O_R,
    AUD_IF_REC_O_L,//rec
    AUD_IF_REC_O_R,
    AUIDO_TX_NUM,
}AUD_IF_CH_et;

typedef struct _AUD_AS_CH_EN_t{
    uint32_t adc        :1;//bit0
    uint32_t anc        :4;
    uint32_t dac        :2;//bit5-6
    uint32_t i2s0_in    :2;//bit7-8
    uint32_t i2s0_out   :2;
    uint32_t i2s0_out2  :2;
    uint32_t i2s0_out3  :2;
    uint32_t i2s1_in    :2;//bit15-16
    uint32_t i2s1_out   :2;
    uint32_t i2s2_in    :2;//bit19-20
    uint32_t i2s2_out   :2;
    uint32_t rsvd       :9;
}AUD_AS_CH_EN_t;

//audio stream channel enable
AUD_AS_CH_EN_t g_as_ch_en = {
    .adc       = ADC0_IN_En,
    .anc       = ANC_IN_En,
    .dac       = 3,
    .i2s0_in   = 0,
    .i2s0_out  = 0,
    .i2s0_out2 = 0,
    .i2s0_out3 = 0,
    .i2s1_in   = I2S2_IN_En,
    .i2s1_out  = I2S2_OUT_En,
    .i2s2_in   = I2S3_IN_En,
    .i2s2_out  = I2S3_OUT_En,
};

//gain_dB: 0/2/4/6
void line_in_ana_gain_set(uint8_t gain_dB)
{
    if(gain_dB > 6) return;
    REG_SYSTEM_0x4F &= ~0xC;
    gain_dB >>= 1;
    REG_SYSTEM_0x4F |= gain_dB;
}
//return: 0/2/4/6dB
uint8_t line_in_ana_gain_get(void)
{
    return (uint8_t)(((REG_SYSTEM_0x4F & 0xC) >> 2) << 1);
}


//**************************************************
void AngInOutGainSet(int32_t Gain)
{
//	line_in_ana_gain_set((Gain&0xFF));
	audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, ((Gain>>16)&0xFF) << 8 | AUDIO_ADC_CHANNEL_0);	//gain
	audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, ((Gain>>16)&0xFF) << 8 | AUDIO_ANC_CHANNEL_2);	//gain
	audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, ((Gain>>24)&0xFF) << 8 | AUDIO_ANC_CHANNEL_3);	//gain
}

//map audio interface to audio effect module channel
void audio_interface_map_config(void)
{
    int i;
    uint8_t aud_i_map_buf[16];
    uint8_t aud_o_map_buf[16];
    //audio interface map init
    for(i = 0; i < AUIDO_RX_NUM; i++){ aud_i_map_buf[i] = i; }
    for(i = 0; i < AUIDO_TX_NUM; i++){ aud_o_map_buf[i] = i; }
    //audio in interface map
    // aud_i_map_buf[AUD_IF_MIC2] = AUD_IF_MIC3;
    // aud_i_map_buf[AUD_IF_MIC3] = AUD_IF_MIC2;
    //audio out interface map
    // aud_o_map_buf[AUD_IF_DAC_O_L] = AUD_IF_DAC_O_R;
    // aud_o_map_buf[AUD_IF_DAC_O_R] = AUD_IF_DAC_O_L;
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_IF_PATH_CFG | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&aud_i_map_buf, (uint32_t)&aud_o_map_buf, 0, NULL);
}

//***************************************
void dsp_audio_init(void)
{
#if (USER_KARAOK_MODE && (CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_DSP) && (CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_DSP))

    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_AUD_IF_PROTECT | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);//enable, invalid, invalid

    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_AS_EN_INIT | MAILBOX_CMD_FAST_RSP_FLAG, *((uint32_t*)&g_as_ch_en), 0, 0, NULL);
    audio_interface_map_config();
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_INIT | MAILBOX_CMD_FAST_RSP_FLAG, 48000, 1, 0, NULL);
    //adc
    // if(g_as_ch_en.adc)
    {//adc must enable, even not used, if g_as_ch_en.adc == 0, tehn adc mute
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_ADC_INIT | MAILBOX_CMD_FAST_RSP_FLAG, 48000, 1, 0, NULL);//freq, enable, invalid
        audio_adc_init(AUDIO_ADC_WIDTH_24, AUDIO_ADC_MODE_DIFFERENCE);	// adc0 diff mode enable
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_ADC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);//enable, invalid, invalid
        //analog gain to be set, value range from 0 ~ 12 which means 0dB ~ 24dB, step is 2dB	yuan++
    	#define val	0
        audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, val << 8 | AUDIO_ADC_CHANNEL_0);	//gain
    }

    //anc
    if(g_as_ch_en.anc) {
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_ANC_INIT | MAILBOX_CMD_FAST_RSP_FLAG, 48000, 1, 0, NULL);
        audio_anc_init(AUDIO_ADC_WIDTH_24, AUDIO_ADC_MODE_DIFFERENCE); // adc1~4 diff mode enable
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_ANC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);
    }

    //i2s
    if(g_as_ch_en.i2s0_in || g_as_ch_en.i2s0_out || g_as_ch_en.i2s1_in || g_as_ch_en.i2s1_out || g_as_ch_en.i2s2_in || g_as_ch_en.i2s2_out)
    {
        I2S_CFG_t i2s_cfg = {
            .I2Sx = I2S2,
            .role = I2S_ROLE_MASTER,
            .sample_rate = 48000,
            .datawith = 24,
            .clk_gate_dis = 1,
            .lrck_inv_en = 1,
            .bck_valid = 1,
            .lrck_valid = 1,
            .din_valid = 1,
            .dout_valid = 1,
            .dout2_valid = 0,//only for i2s0
            .dout3_valid = 0,//only for i2s0
        };

        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_PRE_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);
	#ifdef	I2sMclk_En
		mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_MCLK_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);
	#else
		gpio_config_new(GPIO24, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_NONE);	// 未使用不能配置高阻, 否則干擾LineIn會有底噪
	#endif
        if(g_as_ch_en.i2s0_in || g_as_ch_en.i2s0_out)
        {
            i2s_cfg.I2Sx = I2S0;
            i2s_cfg.din_valid = g_as_ch_en.i2s0_in;
            i2s_cfg.dout_valid = g_as_ch_en.i2s0_out;
            i2s_cfg.dout2_valid = g_as_ch_en.i2s0_out2;
            i2s_cfg.dout3_valid = g_as_ch_en.i2s0_out3;
            mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_INIT | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&i2s_cfg, 1, 0, NULL);
            mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&i2s_cfg, 1, 0, NULL);
        }
        if(g_as_ch_en.i2s1_in || g_as_ch_en.i2s1_out)
        {
            i2s_cfg.I2Sx = I2S1;
            i2s_cfg.din_valid = g_as_ch_en.i2s1_in;
            i2s_cfg.dout_valid = g_as_ch_en.i2s1_out;
            mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_INIT | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&i2s_cfg, 1, 0, NULL);
            mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&i2s_cfg, 1, 0, NULL);
        }
        if(g_as_ch_en.i2s2_in || g_as_ch_en.i2s2_out)
        {
            i2s_cfg.I2Sx = I2S2;
            i2s_cfg.din_valid = g_as_ch_en.i2s2_in;
            i2s_cfg.dout_valid = g_as_ch_en.i2s2_out;
            mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_INIT | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&i2s_cfg, 1, 0, NULL);
            mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_I2S_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)&i2s_cfg, 1, 0, NULL);
        }
    }

    //dac
    if(g_as_ch_en.dac) {
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_DAC_INIT | MAILBOX_CMD_FAST_RSP_FLAG, 48000, 1, 0, NULL);
        mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_DAC_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);
    }

    #ifdef AUD_WAV_TONE_SEPARATE
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_WAV_ENABLE | MAILBOX_CMD_FAST_RSP_FLAG, 1, 0, 0, NULL);
    #endif

    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_INIT_POST | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, NULL);

    //----------------------------------------------------------------------------------------------------
#ifdef CONFIG_USER_SPI_FUNC
//    if(CONFIG_USER_SPI_FUNC != 0)	audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, LineInExchange); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
#elif (defined(LineInExchange))
    audio_ctrl(AUDIO_CTRL_CMD_SWITCH_TO_LINEIN, LineInExchange); // Ch:0->Gpio2/3   Ch:1->Gpio4/5
#endif

    //enable audio record funtion
    aud_mic_open(1);

#endif
}

#ifdef CONFIG_AUD_REC_AMP_DET
static int16_t s_rec_aud_amp_max = 0;
//**** called every 4ms by audio driver ******************************
void user_audio_rec_proc_stereo(int16_t *pcm, int samples)
{
    int i = 0;
    for(i = 0; i < samples; i++){
        if(s_rec_aud_amp_max < pcm[2*i]) s_rec_aud_amp_max = pcm[2*i];
//        if(s_rec_aud_amp_max < pcm[2*i + 1]) s_rec_aud_amp_max = pcm[2*i + 1];
    }
}

#include <stdlib.h>
//*********************************************************************
uint32_t user_aud_amp_task(void)
{
 //	DBG_LOG_INFO("==== s_rec_aud_amp_max:%d\n", s_rec_aud_amp_max);
 //	if(s_rec_aud_amp_max)	s_rec_aud_amp_max/=1.05f;
    if(s_rec_aud_amp_max)	s_rec_aud_amp_max/=2;
    return s_rec_aud_amp_max;
}
#endif
