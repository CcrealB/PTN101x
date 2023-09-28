/**
 **************************************************************************************
 * @file    app_anc.c
 * @brief   Application for Active Noise Cancellation
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2020 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include <math.h>
#include "config.h"
#if USER_KARAOK_MODE == 0
#include <string.h>
#include "app_anc.h"
#include "bkreg.h"
#include "config_debug.h"
#include "port_mem.h"
#include "os_common.h"
#include "drv_mailbox.h"
#include "sbc_decoder.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"

#define ANC_CHANNEL_MODE        (1)//0:MONO, 1:STEREO
#define ANC_SAMPLE_RATE         (384000)

#define ANC_EXEC_MODE_FLOAT     (0)
#define ANC_EXEC_MODE_FIXED     (1)
#define ANC_EXEC_MODE           ANC_EXEC_MODE_FLOAT
#if     ANC_EXEC_MODE_FIXED ==  ANC_EXEC_MODE
#define ANC_FILTER_COEF_TYPE    int32_t
#define F2F(v)                  (int32_t)(v > 0 ? (v * (1 << 24) + 0.5) : (v * (1 << 24) - 0.5))
#else
#define ANC_FILTER_COEF_TYPE    float
#define F2F(v)                  v
#endif

#if ANC_CHANNEL_MODE
#define ANC_CHANNLES            (AUDIO_ANC_CHANNEL_0 | AUDIO_ANC_CHANNEL_1 | AUDIO_ANC_CHANNEL_2 | AUDIO_ANC_CHANNEL_3)
#else
#define ANC_CHANNLES            (AUDIO_ANC_CHANNEL_0 | AUDIO_ANC_CHANNEL_1)
#endif

#define ANC_ANA_GAIN            0x08
#define ANC_DIG_GAIN            0x2D

CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_NOISE_MIC2[] =
{
    -0.274454, 0.466924,  -0.260671,  1.618021, -0.823608,
    0.707234, -1.388121,  0.691171,  1.940819, -0.948969,
    0.692090, -1.368623,  0.676552,  1.977148, -0.977161,
    0.631786, -0.697406,  0.102036,  0.413859,  0.405315,
    0.223653, -0.438560,  0.223392,  1.955033, -0.970261,
    1.000812, -1.998811,  0.998004,  1.998811, -0.998816,
};

CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_NOISE_MIC3[] =
{
	0xFFFFFFFF, 0, 100.000000, 0.700000, 1.000000,
	0xFFFFFFFF, 0, 3000.000000, 1.000000, 0.649382,
	0xFFFFFFFF, 0, 45.000000, 1.000000, 1.333521,
};

CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_NOISE_MIC4[] =
{
	0xFFFFFFFF, 1, 60.000000, 1.200000, 1.372461,
	0xFFFFFFFF, 0, 123.000000, 5.000000, 1.053174,
	0xFFFFFFFF, 0, 260.000000, 2.500000, 0.968835,
	0xFFFFFFFF, 0, 425.000000, 1.000000, 1.333521,
	0xFFFFFFFF, 0, 650.000000, 2.000000, 0.794328,
	0xFFFFFFFF, 2, 1500.000000, 0.500000, 0.771792,
	0xFFFFFFFF, 2, 4500.000000, 1.500000, 0.562341,
};
CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_NOISE_MIC5[] =
{
	0xFFFFFFFF, 0, 100.000000, 0.700000, 1.000000,
	0xFFFFFFFF, 0, 3000.000000, 1.000000, 0.649382,
	0xFFFFFFFF, 0, 45.000000, 1.000000, 1.333521,
};
CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_TRANS_MIC2[] =
{
    0.501184, -0.995456,  0.494242,  0.990882,  0.000000,
    0.355584, -0.681290,  0.329898,  1.962351, -0.971163,
    0.011828, -0.019160,  0.009250,  1.912783, -0.914701,
    0.129343,  2.346437,  0.000000,  1.298604, -0.837254,
    0.127200, -0.206605,  0.798841,  1.772361, -0.911745,
};
CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_NOISE_EC0[] =
{
	0xFFFFFFFF, 0, 100.000000, 0.500000, 1.333521,
	0xFFFFFFFF, 0, 50.000000, 1.000000, 1.154782,
};


CONST ANC_FILTER_COEF_TYPE ANC_FILTER_COEFS_NOISE_EC1[] =
{
	0xFFFFFFFF, 0, 100.000000, 0.500000, 1.333521,
	0xFFFFFFFF, 0, 50.000000, 1.000000, 1.154782,
};

uint8_t  anc_test_flag  = 0;
uint8_t  anc_bak_status = 0;
uint8_t  anc_cur_status = ANC_STATUS_IDLE;
int16_t* anc_volume;
int16_t* anc_volume1;

AncParam anc_params     = {0, 2400, 0, 500, 0, 0xC0, 0, 0 };
AncParam anc_params1    = {0, 2400, 0, 500, 0, 0xC1, 0, 0 };

float* anc_scale0;
float* anc_scale1;

AncVolScaler anc_vol_scaler0 = { 1.0, 1.0, 1.0, 1.0, 1.0, 0xC0, 1.0 };
AncVolScaler anc_vol_scaler1 = { 1.0, 1.0, 1.0, 1.0, 1.0, 0xC0, 1.0 };

uint32_t anc_filter_address = 0;

uint8_t  anc_filter_count_noise_mic3 = 0;
uint8_t  anc_filter_count_noise_mic2 = 0;
uint8_t  anc_filter_count_noise_ec0  = 0;
uint8_t  anc_filter_count_trans_mic3 = 0;
uint8_t  anc_filter_count_trans_mic2 = 0;
uint8_t  anc_filter_count_trans_ec0  = 0;

ANC_FILTER_COEF_TYPE anc_filter_coefs_noise_mic3[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_noise_mic2[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_noise_ec0[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_trans_mic3[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_trans_mic2[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_trans_ec0[10][5];

#if ANC_CHANNEL_MODE == 1
uint8_t  anc_filter_count_noise_mic5 = 0;
uint8_t  anc_filter_count_noise_mic4 = 0;
uint8_t  anc_filter_count_noise_ec1  = 0;
uint8_t  anc_filter_count_trans_mic5 = 0;
uint8_t  anc_filter_count_trans_mic4 = 0;
uint8_t  anc_filter_count_trans_ec1  = 0;

ANC_FILTER_COEF_TYPE anc_filter_coefs_noise_mic5[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_noise_mic4[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_noise_ec1[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_trans_mic5[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_trans_mic4[10][5];
ANC_FILTER_COEF_TYPE anc_filter_coefs_trans_ec1[10][5];
#endif

static ANC_SPP_FILETER_CMD anc_spp_filter_var;

static __inline int32_t app_anc_ssat16(int32_t a)
{
    int32_t bits = (1 << (16 - 1)) - 1;
    int32_t result; __asm("b.lim %0,%1,%2;" : "=r" (result) : "r" (a), "r" (bits) : ); return result;
}

static void app_anc_filter_coefs_update(void)
{
    uint32_t cmd, vol;

    os_printf("app_anc_filter_coefs_update(%d)\n", anc_spp_filter_var.trans_noise_mode);

    switch(anc_spp_filter_var.trans_noise_mode)
    {
    case ANC_NOISE_MIC3:
        cmd = MAILBOX_CMD_ANC_MIC3_SET;
        vol = anc_volume[0];
        anc_filter_count_noise_mic3 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_noise_mic3, anc_spp_filter_var.filter1, anc_filter_count_noise_mic3 * 20);
        break;
    case ANC_TRANS_MIC3:
        cmd = MAILBOX_CMD_ANC_MIC3_SET;
        vol = anc_volume[0];
        anc_filter_count_trans_mic3 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_trans_mic3, anc_spp_filter_var.filter1, anc_filter_count_trans_mic3 * 20);
        break;
    case ANC_NOISE_MIC2:
        cmd = MAILBOX_CMD_ANC_MIC2_SET;
        vol = anc_volume[1];
        anc_filter_count_noise_mic2 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_noise_mic2, anc_spp_filter_var.filter1, anc_filter_count_noise_mic2 * 20);
        break;
    case ANC_TRANS_MIC2:
        cmd = MAILBOX_CMD_ANC_MIC2_SET;
        vol = anc_volume[1];
        anc_filter_count_trans_mic2 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_trans_mic2, anc_spp_filter_var.filter1, anc_filter_count_trans_mic2 * 20);
        break;
    case ANC_NOISE_EC0:
        cmd = MAILBOX_CMD_ANC_EC0_SET;
        vol = anc_volume[4];
        anc_filter_count_noise_ec0 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_noise_ec0, anc_spp_filter_var.filter1, anc_filter_count_noise_ec0 * 20);
        break;
    case ANC_TRANS_EC0:
        cmd = MAILBOX_CMD_ANC_EC0_SET;
        vol = anc_volume[4];
        anc_filter_count_trans_ec0 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_trans_ec0, anc_spp_filter_var.filter1, anc_filter_count_trans_ec0 * 20);
        break;
    #if ANC_CHANNEL_MODE == 1
    case ANC_NOISE_MIC5:
        cmd = MAILBOX_CMD_ANC_MIC5_SET;
        vol = anc_volume1[0];
        anc_filter_count_noise_mic5 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_noise_mic5, anc_spp_filter_var.filter1, anc_filter_count_noise_mic5 * 20);
        break;
    case ANC_TRANS_MIC5:
        cmd = MAILBOX_CMD_ANC_MIC5_SET;
        vol = anc_volume1[0];
        anc_filter_count_trans_mic5 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_trans_mic5, anc_spp_filter_var.filter1, anc_filter_count_trans_mic5 * 20);
        break;
    case ANC_NOISE_MIC4:
        cmd = MAILBOX_CMD_ANC_MIC4_SET;
        vol = anc_volume1[1];
        anc_filter_count_noise_mic4 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_noise_mic4, anc_spp_filter_var.filter1, anc_filter_count_noise_mic4 * 20);
        break;
    case ANC_TRANS_MIC4:
        cmd = MAILBOX_CMD_ANC_MIC4_SET;
        vol = anc_volume1[1];
        anc_filter_count_trans_mic4 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_trans_mic4, anc_spp_filter_var.filter1, anc_filter_count_trans_mic4 * 20);
        break;
    case ANC_NOISE_EC1:
        cmd = MAILBOX_CMD_ANC_EC1_SET;
        vol = anc_volume1[4];
        anc_filter_count_noise_ec1 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_noise_ec1, anc_spp_filter_var.filter1, anc_filter_count_noise_ec1 * 20);
        break;
    case ANC_TRANS_EC1:
        cmd = MAILBOX_CMD_ANC_EC1_SET;
        vol = anc_volume1[4];
        anc_filter_count_trans_ec1 = anc_spp_filter_var.filter_num;
        memcpy(anc_filter_coefs_trans_ec1, anc_spp_filter_var.filter1, anc_filter_count_trans_ec1 * 20);
        break;
    #endif
    default:
        return;
    }

    if(REG_SYSTEM_0x15 == 0) mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | cmd, (uint32_t)anc_spp_filter_var.filter1, (uint32_t)anc_spp_filter_var.filter_num, vol);
}

void app_anc_init(void)
{
	INFO_PRT("anc_coefs  = { %d, %d }\r\n", anc_filter_count_noise_mic2, anc_filter_count_trans_mic2);
	INFO_PRT("anc_params = { %d, %d, %d, %d }\r\n", anc_params.noise_cancel_ff_volume, anc_params.noise_cancel_fb_volume, anc_params.transparency_ff_volume, anc_params.transparency_fb_volume);

	if(anc_filter_count_noise_mic2== 0)
	{
	    anc_filter_count_noise_mic2 = sizeof(ANC_FILTER_COEFS_NOISE_MIC2) / 20;
	    memcpy(anc_filter_coefs_noise_mic2, ANC_FILTER_COEFS_NOISE_MIC2, sizeof(ANC_FILTER_COEFS_NOISE_MIC2));
	}

	if(anc_filter_count_noise_mic3 == 0)
	{
	    anc_filter_count_noise_mic3 = sizeof(ANC_FILTER_COEFS_NOISE_MIC3) / 20;
	    memcpy(anc_filter_coefs_noise_mic3, ANC_FILTER_COEFS_NOISE_MIC3, sizeof(ANC_FILTER_COEFS_NOISE_MIC3));
	}

	if(anc_filter_count_noise_mic4 == 0)
	{
	    anc_filter_count_noise_mic4 = sizeof(ANC_FILTER_COEFS_NOISE_MIC4) / 20;
	    memcpy(anc_filter_coefs_noise_mic4, ANC_FILTER_COEFS_NOISE_MIC4, sizeof(ANC_FILTER_COEFS_NOISE_MIC4));
	}

	if(anc_filter_count_noise_mic5 == 0)
	{
	    anc_filter_count_noise_mic5 = sizeof(ANC_FILTER_COEFS_NOISE_MIC5) / 20;
	    memcpy(anc_filter_coefs_noise_mic5, ANC_FILTER_COEFS_NOISE_MIC5, sizeof(ANC_FILTER_COEFS_NOISE_MIC5));
	}

	if(anc_filter_count_noise_ec0 == 0)
	{
	    anc_filter_count_noise_ec0 = sizeof(ANC_FILTER_COEFS_NOISE_EC0) / 20;
	    memcpy(anc_filter_coefs_noise_ec0, ANC_FILTER_COEFS_NOISE_EC0, sizeof(ANC_FILTER_COEFS_NOISE_EC0));
	}
	
	if(anc_filter_count_noise_ec1 == 0)
	{
	    anc_filter_count_noise_ec1 = sizeof(ANC_FILTER_COEFS_NOISE_EC1) / 20;
	    memcpy(anc_filter_coefs_noise_ec1, ANC_FILTER_COEFS_NOISE_EC1, sizeof(ANC_FILTER_COEFS_NOISE_EC1));
	}

    audio_anc_init(AUDIO_ADC_WIDTH_24, AUDIO_ADC_MODE_SINGLE_END);
    audio_anc_sample_rate_set(ANC_SAMPLE_RATE);
    audio_anc_ana_gain_set(ANC_ANA_GAIN);
    audio_anc_dig_gain_set(ANC_DIG_GAIN);
    //Enable TX1 LPF FIFO
    REG_ANC_0x05 &= ~(1 << 9);
    //Modify threshold
    audio_ctrl(AUDIO_CTRL_CMD_ANC_FIFO_WRTH_SET, 0);
    //audio_ctrl(AUDIO_CTRL_CMD_DAC_TX1_FIFO_RDTH_SET, 0);
    audio_ctrl(AUDIO_CTRL_CMD_DAC_TX2_FIFO_RDTH_SET, 0);
    audio_ctrl(AUDIO_CTRL_CMD_ANC_INT_ENABLE, 1);
    audio_ctrl(AUDIO_CTRL_CMD_ANC_FIFO_DATA_MODE_SET, AUDIO_FIFO_DATA_MODE_INTERLEAVE);
    audio_ctrl(AUDIO_CTRL_CMD_DAC_FIFO_DATA_MODE_SET, AUDIO_FIFO_DATA_MODE_INTERLEAVE);
    app_anc_status_switch_to(ANC_STATUS_IDLE);
    //REG_SYSTEM_0x55 &= ~(0x1F << 6);
}

void app_anc_enable(uint32_t enable)
{
    if(enable)
    {
        if(REG_SYSTEM_0x15 == 0)
        {
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_ENABLE, AUDIO_ANC_CHANNEL_0 | ((!!enable) << 8), ANC_SAMPLE_RATE / 10, ANC_SAMPLE_RATE == 384000);
        }

        audio_ctrl(AUDIO_CTRL_CMD_ANC_INT_ENABLE, 1);
        audio_anc_enable_ext(ANC_CHANNLES, enable);
    }
    else
    {
        audio_ctrl(AUDIO_CTRL_CMD_ANC_INT_ENABLE, 0);
        audio_anc_enable(enable);

        if(REG_SYSTEM_0x15 == 0)
        {
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_ENABLE, AUDIO_ANC_CHANNEL_0 | ((!!enable) << 8), ANC_SAMPLE_RATE / 10, ANC_SAMPLE_RATE == 384000);
        }
    }
}

int32_t app_anc_status_get(void)
{
    return anc_cur_status;
}

void app_anc_dsp_status_switch(int32_t status, int32_t freq)
{
    if(REG_SYSTEM_0x15 == 0)
    {
        int32_t factor = 1;

        uint32_t* scale0 = (uint32_t*)anc_scale0;
        #if ANC_CHANNEL_MODE == 1
        uint32_t* scale1 = (uint32_t*)anc_scale1;
        #endif

        #if (CONFIG_EAR_IN == 1)
        if(!spp_is_connected())
        {
            if((app_env_get_pin_enable(PIN_earInDet) && !get_earin_status()))  factor = 0;
        }
        #endif

        mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_STATUS_SWITCH, status, ANC_EXEC_MODE, freq);

        switch(status)
        {
        case ANC_STATUS_NOISE_CANCEL:
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_SCALE0_SET, scale0[0], scale0[1], scale0[4]);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC3_SET, (uint32_t)anc_filter_coefs_noise_mic3, anc_filter_count_noise_mic3, anc_volume[0]  * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC2_SET, (uint32_t)anc_filter_coefs_noise_mic2, anc_filter_count_noise_mic2, anc_volume[1]  * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_EC0_SET,  (uint32_t)anc_filter_coefs_noise_ec0,  anc_filter_count_noise_ec0,  anc_volume[4]  * factor);
            #if ANC_CHANNEL_MODE == 1
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_SCALE1_SET, scale1[0], scale1[1], scale1[4]);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC5_SET, (uint32_t)anc_filter_coefs_noise_mic5, anc_filter_count_noise_mic5, anc_volume1[0] * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC4_SET, (uint32_t)anc_filter_coefs_noise_mic4, anc_filter_count_noise_mic4, anc_volume1[1] * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_EC1_SET,  (uint32_t)anc_filter_coefs_noise_ec1,  anc_filter_count_noise_ec1,  anc_volume1[4] * factor);
            #endif
            break;
        case ANC_STATUS_TRANSPARENCY:
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_SCALE0_SET, scale0[0], scale0[1], scale0[4]);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC3_SET, (uint32_t)anc_filter_coefs_trans_mic3, anc_filter_count_trans_mic3, anc_volume[0]  * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC2_SET, (uint32_t)anc_filter_coefs_trans_mic2, anc_filter_count_trans_mic2, anc_volume[1]  * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_EC0_SET,  (uint32_t)anc_filter_coefs_trans_ec0,  anc_filter_count_trans_ec0,  anc_volume[4]  * factor);
            #if ANC_CHANNEL_MODE == 1
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_SCALE1_SET, scale1[0], scale1[1], scale1[4]);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC5_SET, (uint32_t)anc_filter_coefs_trans_mic5, anc_filter_count_trans_mic5, anc_volume1[0] * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_MIC4_SET, (uint32_t)anc_filter_coefs_trans_mic4, anc_filter_count_trans_mic4, anc_volume1[1] * factor);
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_EC1_SET,  (uint32_t)anc_filter_coefs_trans_ec1,  anc_filter_count_trans_ec1,  anc_volume1[4] * factor);
            #endif
            break;
        default:
            break;
        }
    }
}

void app_anc_status_switch_to(int32_t status)
{
    if(anc_cur_status == status) return;

    app_anc_enable(0);

    anc_volume  = status == ANC_STATUS_IDLE ? NULL : ((int16_t*)&anc_params)  + 2 * (status - 1);
    #if ANC_CHANNEL_MODE == 1
    anc_volume1 = status == ANC_STATUS_IDLE ? NULL : ((int16_t*)&anc_params1) + 2 * (status - 1);
    #endif

    anc_scale0 = status == ANC_STATUS_IDLE ? NULL : ((float*)&anc_vol_scaler0) + 2 * (status - 1);
    #if ANC_CHANNEL_MODE == 1
    anc_scale1 = status == ANC_STATUS_IDLE ? NULL : ((float*)&anc_vol_scaler1) + 2 * (status - 1);
    #endif

    app_anc_dsp_status_switch(status, system_apll_freq());

    switch(status)
    {
    case ANC_STATUS_IDLE:
        if(!app_bt_flag1_get(APP_FLAG_MUSIC_PLAY | APP_FLAG_SCO_CONNECTION | APP_FLAG_WAVE_PLAYING))
        {
            audio_dac_enable(0);
        }
        else
        {
            audio_dac_fadeout(2);
        }
        break;
    case ANC_STATUS_NOISE_CANCEL:
    case ANC_STATUS_TRANSPARENCY:
        if(anc_cur_status == ANC_STATUS_IDLE)
        {
#if CONFIG_ANC_ENABLE
            BK3000_set_clock(CPU_ANC_CLK_SEL, CPU_ANC_CLK_DIV);
#endif
            if(!app_bt_flag1_get(APP_FLAG_MUSIC_PLAY | APP_FLAG_SCO_CONNECTION | APP_FLAG_WAVE_PLAYING))
            {
                audio_dac_enable(1);
            }
        }
        app_anc_enable(1);
        break;
    default:
        return;
    }

    anc_cur_status = status;
}

void app_anc_freq_changed(uint32_t freq)
{
    app_anc_dsp_status_switch(anc_cur_status, freq);
}

void app_anc_volume_restore(int32_t flag)
{
    if(REG_SYSTEM_0x15 == 0 && anc_cur_status)
    {
        mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_VOLUME_CHANGE, anc_volume[0] * (!!flag), anc_volume[1] * (!!flag), 0);
    }
}

void app_anc_mode_restore(int32_t flag)
{
    os_printf("%s(%d)\r\n", __func__, flag);

    if(flag)
    {
        uint8_t anc_status = anc_bak_status;

        anc_bak_status = 0;
        audio_ctrl(AUDIO_CTRL_CMD_ANC_INT_ENABLE, 1);
        audio_anc_init(AUDIO_ADC_WIDTH_24, AUDIO_ADC_MODE_SINGLE_END);
        audio_anc_sample_rate_set(ANC_SAMPLE_RATE);
        audio_anc_ana_gain_set(ANC_ANA_GAIN);
        audio_anc_dig_gain_set(ANC_DIG_GAIN);
        app_anc_status_switch_to(anc_status);
    }
    else
    {
        uint8_t anc_status = anc_cur_status;

        audio_ctrl(AUDIO_CTRL_CMD_ANC_INT_ENABLE, 0);
        app_anc_status_switch_to(ANC_STATUS_IDLE);
        audio_anc_init(AUDIO_ADC_WIDTH_16, AUDIO_ADC_MODE_SINGLE_END);
        anc_bak_status = anc_status;
    }
}

void app_anc_white_noise_out(int32_t enable)
{
    if(REG_SYSTEM_0x15 == 0 && anc_cur_status)
    {
        mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_WHITE_NOISE_OUT, enable, 0, 0);
    }
}

void app_anc_pulse_out(int32_t enable)
{
    if(REG_SYSTEM_0x15 == 0 && anc_cur_status)
    {
        mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_PULSE_OUT, enable, 0, 0);
    }
}

void app_anc_dbg(uint8_t key)
{
    switch(key)
    {
    case '-':
        if(REG_SYSTEM_0x15 == 0 && anc_cur_status)
        {
            anc_volume[0] -= 8;
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_VOLUME_CHANGE, anc_volume[0], anc_volume[1], 0);
        }
        break;
    case '+':
        if(REG_SYSTEM_0x15 == 0 && anc_cur_status)
        {
            anc_volume[0] += 8;
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_VOLUME_CHANGE, anc_volume[0], anc_volume[1], 0);
        }
        break;
    case 's':
        {
            uint8_t new_status = anc_cur_status;
            if(++new_status > 2) new_status = 0;
            os_printf("[KEY]: ANC STATUS %d\r\n", new_status);
            app_anc_status_switch_to(new_status);
        }
        break;
    case 'r':
        {
            char rsp[64];
            int  len;

            len = sprintf(rsp, "<%d, %d, %d, %d><%08x>\r\n", anc_params.noise_cancel_ff_volume, anc_params.noise_cancel_fb_volume, anc_params.transparency_ff_volume, anc_params.transparency_fb_volume, (unsigned int)anc_filter_address);

            if(len > 0) os_printf("%s", rsp);
        }
        break;
    case 'w':
        os_printf("[KEY]: ANC PARAMS <%d, %d, %d, %d> write into flash\r\n", anc_params.noise_cancel_ff_volume, anc_params.noise_cancel_fb_volume, anc_params.transparency_ff_volume, anc_params.transparency_fb_volume);
        if(FALSE == app_append_tlv_data(TLV_SECTOR_ENVCALI,TLV_TYPE_CALI_ANC_PARAM,(uint8*)&anc_params,sizeof(AncParam)))
        {
            uint8_t buffer[15];

            flash_read_data(buffer, FLASH_ENVCALI_DEF_ADDR_ABS, sizeof(buffer));
            flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);
            flash_write_data(buffer, FLASH_ENVCALI_DEF_ADDR_ABS, sizeof(buffer));

            app_append_tlv_data(TLV_SECTOR_ENVCALI,TLV_TYPE_CALI_ANC_PARAM,(uint8*)&anc_params,sizeof(AncParam));
        }
        break;
    case 'W':
        anc_test_flag = !anc_test_flag;
        os_printf("[KEY]: ANC WHITE NOISE OUT (%d)\r\n", anc_test_flag);
        app_anc_white_noise_out(anc_test_flag);
        break;
    case 'P':
        anc_test_flag = !anc_test_flag;
        os_printf("[KEY]: ANC PULSE OUT (%d)\r\n", anc_test_flag);
        app_anc_pulse_out(anc_test_flag);
        break;
	case 'C':
	    os_printf("[KEY]: ANC PARAMS CLEAR\r\n");
        {
	        //uint8_t buffer[15];
            //flash_read_data(buffer, FLASH_ENVCALI_DEF_ADDR_ABS, sizeof(buffer));
            flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);
            //flash_write_data(buffer, FLASH_ENVCALI_DEF_ADDR_ABS, sizeof(buffer));
        }
	    break;
    default:
        os_printf("[KEY]: <%c | %02X>\r\n", key, key);
        break;
    }
}

static uint8_t app_anc_channel_check(void)
{
    return 'S';
}

static uint32_t app_anc_debug_cmd_valid_check(uint8_t* mode)
{
    uint32_t valid;

    #if 0 //FOR TWS
    if(app_env_is_tws_L_R_dif())
    {
        if(!app_env_is_tws_L() && (mode[0] & 0x2))
        {
            *mode &= ~0x2;
            valid  = 1;
        }
        else if(app_env_is_tws_L() && !(mode[0] & 0x2))
        {
            valid = 1;
        }
        else
        {
            valid = 0;
        }
    }
    else
    #endif
    {
        valid = 1;
    }

    return valid;
}

int app_anc_debug(HCI_COMMAND_PACKET* rp, HCI_EVENT_PACKET* tp)
{
    int res = 1;

    switch(rp->param[0])
    {
    case ANC_CMD_STATUS_SWITCH:
        app_anc_status_switch_to(rp->param[1]);
        tp->total = HCI_EVENT_HEAD_LENGTH+4;
        tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
        tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
        tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
        tp->param[HCI_EVENT_HEAD_LENGTH+3] = app_anc_channel_check();
        break;
    case ANC_CMD_GAN_FILTER_READ:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            uint32_t anaGain  = 0;
            int16_t  digGain  = 0;
            uint32_t  pfcount = 0;
            uint32_t* pfcoefs = 0;

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
                digGain = anc_volume[0];
                anaGain = AUDIO_ANC_CHANNEL_1;
                pfcount = anc_filter_count_noise_mic3;
                pfcoefs = (uint32_t*)anc_filter_coefs_noise_mic3;
                break;
            case ANC_TRANS_MIC3:
                digGain = anc_volume[0];
                anaGain = AUDIO_ANC_CHANNEL_1;
                pfcount = anc_filter_count_trans_mic3;
                pfcoefs = (uint32_t*)anc_filter_coefs_trans_mic3;
                break;
            case ANC_NOISE_MIC2:
                digGain = anc_volume[1];
                anaGain = AUDIO_ANC_CHANNEL_0;
                pfcount = anc_filter_count_noise_mic2;
                pfcoefs = (uint32_t*)anc_filter_coefs_noise_mic2;
                break;
            case ANC_TRANS_MIC2:
                digGain = anc_volume[1];
                anaGain = AUDIO_ANC_CHANNEL_0;
                pfcount = anc_filter_count_trans_mic2;
                pfcoefs = (uint32_t*)anc_filter_coefs_trans_mic2;
                break;
            case ANC_NOISE_EC0:
                digGain = anc_volume[4];
                pfcount = anc_filter_count_noise_ec0;
                pfcoefs = (uint32_t*)anc_filter_coefs_noise_ec0;
                break;
            case ANC_TRANS_EC0:
                digGain = anc_volume[4];
                pfcount = anc_filter_count_trans_ec0;
                pfcoefs = (uint32_t*)anc_filter_coefs_trans_ec0;
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
                digGain = anc_volume1[0];
                anaGain = AUDIO_ANC_CHANNEL_3;
                pfcount = anc_filter_count_noise_mic5;
                pfcoefs = (uint32_t*)anc_filter_coefs_noise_mic5;
                break;
            case ANC_TRANS_MIC5:
                digGain = anc_volume1[0];
                anaGain = AUDIO_ANC_CHANNEL_3;
                pfcount = anc_filter_count_trans_mic5;
                pfcoefs = (uint32_t*)anc_filter_coefs_trans_mic5;
                break;
            case ANC_NOISE_MIC4:
                digGain = anc_volume1[1];
                anaGain = AUDIO_ANC_CHANNEL_2;
                pfcount = anc_filter_count_noise_mic4;
                pfcoefs = (uint32_t*)anc_filter_coefs_noise_mic4;
                break;
            case ANC_TRANS_MIC4:
                digGain = anc_volume1[1];
                anaGain = AUDIO_ANC_CHANNEL_2;
                pfcount = anc_filter_count_trans_mic4;
                pfcoefs = (uint32_t*)anc_filter_coefs_trans_mic4;
                break;
            case ANC_NOISE_EC1:
                digGain = anc_volume1[4];
                pfcount = anc_filter_count_noise_ec1;
                pfcoefs = (uint32_t*)anc_filter_coefs_noise_ec1;
                break;
            case ANC_TRANS_EC1:
                digGain = anc_volume1[4];
                pfcount = anc_filter_count_trans_ec1;
                pfcoefs = (uint32_t*)anc_filter_coefs_trans_ec1;
                break;
            #endif
            default:
                break;
            }

            if(anaGain) audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_GET, (uint32_t)&anaGain);

            tp->total = HCI_EVENT_HEAD_LENGTH+6+pfcount*20;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = anaGain;
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = digGain & 0xFF;
            tp->param[HCI_EVENT_HEAD_LENGTH+5] = (digGain >> 8) & 0xFF;
            memcpy(&tp->param[HCI_EVENT_HEAD_LENGTH+6], pfcoefs, pfcount*20);
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_GAN_FILTER_UPDATE:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            uint8_t anaGain = rp->param[2];
            int16_t digGain = (uint16_t)rp->param[4] << 8 | rp->param[3];

            anc_spp_filter_var.trans_noise_mode = rp->param[1];
            memcpy((uint8*)&anc_spp_filter_var + 3, &rp->param[5], rp->total-5);

            app_anc_filter_coefs_update();

            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, ANC_CMD_SET_GAIN, rp->param[1], digGain);

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                anc_volume[0] = digGain;
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_1 | (anaGain << 8));
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                anc_volume[1] = digGain;
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_0 | (anaGain << 8));
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                anc_volume[4] = digGain;
                //do nothing
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                anc_volume1[0] = digGain;
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_3 | (anaGain << 8));
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                anc_volume1[1] = digGain;
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_2 | (anaGain << 8));
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                anc_volume1[4] = digGain;
                //do nothing
                break;
            #endif
            default:
                break;
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+8;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = rp->param[3];
            tp->param[HCI_EVENT_HEAD_LENGTH+5] = rp->param[4];
            tp->param[HCI_EVENT_HEAD_LENGTH+6] = (rp->total-5) / 20;
            tp->param[HCI_EVENT_HEAD_LENGTH+7] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_FILTER_UPDATE:
    case ANC_CMD_FILTER_WRITE:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            memcpy((uint8*)&anc_spp_filter_var + 2, &rp->param[1], rp->total-1);
            app_anc_filter_coefs_update();
            if(ANC_CMD_FILTER_WRITE == rp->param[0] && FALSE == app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_COEFS, (uint8*)&anc_spp_filter_var + 2, rp->total-1))
            {
                flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);
                app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_COEFS, (uint8*)&anc_spp_filter_var + 2, rp->total-1);
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+5;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;

    case ANC_CMD_READ_GAIN:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            int16_t vol = 0;

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                vol = anc_volume[0];
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                vol = anc_volume[1];
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                vol = anc_volume[4];
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                vol = anc_volume1[0];
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                vol = anc_volume1[1];
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                vol = anc_volume1[4];
                break;
            #endif
            default:
                break;
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+5;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = vol & 0xFF;
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = (vol >> 8) & 0xFF;
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_SET_GAIN:
    case ANC_CMD_WRITE_GAIN:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            int16_t vol = (uint16_t)rp->param[3] << 8 | rp->param[2];

            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, ANC_CMD_SET_GAIN, rp->param[1], vol);

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                anc_volume[0] = vol;
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                anc_volume[1] = vol;
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                anc_volume[4] = vol;
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                anc_volume1[0] = vol;
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                anc_volume1[1] = vol;
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                anc_volume1[4] = vol;
                break;
            #endif
            default:
                break;
            }

            if(rp->param[0] == ANC_CMD_WRITE_GAIN)
            {
                uint8* buf = (uint8*)(rp->param[1] & 0x2 ? &anc_params1 : &anc_params);

                if(FALSE == app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_PARAM, buf, sizeof(AncParam)))
                {
                    flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);
                    app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_PARAM, buf, sizeof(AncParam));
                }
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+6;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = rp->param[3];
            tp->param[HCI_EVENT_HEAD_LENGTH+5] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;

    case ANC_CMD_DBGAIN_READ:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            float vol = 1.0;

            Variant32 v;

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                vol = anc_scale0[0];
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                vol = anc_scale0[1];
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                vol = anc_scale0[4];
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                vol = anc_scale1[0];
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                vol = anc_scale1[1];
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                vol = anc_scale1[4];
                break;
            #endif
            default:
                break;
            }

            v.f32 = 20 * log10f(vol < 0 ? -vol : vol);

            tp->total = HCI_EVENT_HEAD_LENGTH+7;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = v.u8[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = v.u8[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+5] = v.u8[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+6] = v.u8[3];
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_DBGAIN_UPDATE:
    case ANC_CMD_DBGAIN_WRITE:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            Variant32 v;

            v.u8[0] = rp->param[2];
            v.u8[1] = rp->param[3];
            v.u8[2] = rp->param[4];
            v.u8[3] = rp->param[5];

            v.f32 = powf(10, v.f32 / 20);

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                anc_scale0[0] = v.f32;
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                anc_scale0[1] = v.f32;
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                anc_scale0[4] = v.f32;
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                anc_scale1[0] = v.f32;
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                anc_scale1[1] = v.f32;
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                anc_scale1[4] = v.f32;
                break;
            #endif
            default:
                break;
            }

            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, ANC_CMD_DBGAIN_UPDATE, rp->param[1], v.u32);

            if(rp->param[0] == ANC_CMD_DBGAIN_WRITE)
            {
                uint8* buf = (uint8*)(rp->param[1] & 0x2 ? &anc_vol_scaler1 : &anc_vol_scaler0);

                if(FALSE == app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_SCALER, buf, sizeof(AncVolScaler)))
                {
                    flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);
                    app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_SCALER, buf, sizeof(AncVolScaler));
                }
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+8;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = rp->param[3];
            tp->param[HCI_EVENT_HEAD_LENGTH+5] = rp->param[4];
            tp->param[HCI_EVENT_HEAD_LENGTH+6] = rp->param[5];
            tp->param[HCI_EVENT_HEAD_LENGTH+7] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_DBGAIN_WRITE2:
        {
            uint8* buf = (uint8*)(rp->param[1] & 0x2 ? &anc_vol_scaler1 : &anc_vol_scaler0);

            if(FALSE == app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_SCALER, buf, sizeof(AncVolScaler)))
            {
                flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);
                app_append_tlv_data(TLV_SECTOR_ENVCALI, TLV_TYPE_CALI_ANC_SCALER, buf, sizeof(AncVolScaler));
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+4;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = 0x00;
        }
        break;
    case ANC_CMD_DBGAIN_INVERSE:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            Variant32 v;

            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                v.f32 = anc_scale0[0] = -anc_scale0[0];
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                v.f32 = anc_scale0[1] = -anc_scale0[1];
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                v.f32 = anc_scale0[4] = -anc_scale0[4];
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                v.f32 = anc_scale1[0] = -anc_scale1[0];
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                v.f32 = anc_scale1[1] = -anc_scale1[1];
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                v.f32 = anc_scale1[4] = -anc_scale1[4];
                break;
            #endif
            default:
                break;
            }

            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, ANC_CMD_DBGAIN_UPDATE, rp->param[1], v.u32);

            tp->total = HCI_EVENT_HEAD_LENGTH+4;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;

    case ANC_CMD_CTRL_BYPASS:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, rp->param[0], rp->param[1], rp->param[2]);

            tp->total = HCI_EVENT_HEAD_LENGTH+5;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_CTRL_CLEAR:
        {
            flash_erase_sector(FLASH_ENVCALI_DEF_ADDR_ABS, 1);

            tp->total = HCI_EVENT_HEAD_LENGTH+3;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = app_anc_channel_check();
        }
        break;
    case ANC_CMD_ANC_PULSE_OUT:
        {
            app_anc_pulse_out(rp->param[1]);

            tp->total = HCI_EVENT_HEAD_LENGTH+4;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = app_anc_channel_check();
        }
        break;
    case ANC_CMD_CTRL_ANA_GAIN_SET:
        if(app_anc_debug_cmd_valid_check(&rp->param[1]))
        {
            switch(rp->param[1])
            {
            case ANC_NOISE_MIC3:
            case ANC_TRANS_MIC3:
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_1 | (rp->param[2] << 8));
                break;
            case ANC_NOISE_MIC2:
            case ANC_TRANS_MIC2:
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_0 | (rp->param[2] << 8));
                break;
            case ANC_NOISE_EC0:
            case ANC_TRANS_EC0:
                //do nothing
                break;
            #if ANC_CHANNEL_MODE == 1
            case ANC_NOISE_MIC5:
            case ANC_TRANS_MIC5:
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_3 | (rp->param[2] << 8));
                break;
            case ANC_NOISE_MIC4:
            case ANC_TRANS_MIC4:
                audio_ctrl(AUDIO_CTRL_CMD_ADC_ANC_ANA_GAIN_SET, AUDIO_ANC_CHANNEL_2 | (rp->param[2] << 8));
                break;
            case ANC_NOISE_EC1:
            case ANC_TRANS_EC1:
                //do nothing
                break;
            #endif
            default:
                break;
            }

            tp->total = HCI_EVENT_HEAD_LENGTH+6;
            tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
            tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
            tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
            tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
            tp->param[HCI_EVENT_HEAD_LENGTH+4] = 0x00;
        }
        else
        {
            res = 0;
        }
        break;
    case ANC_CMD_CTRL_I2S:
        mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, rp->param[0], rp->param[1], rp->param[2]);
        tp->total = HCI_EVENT_HEAD_LENGTH+5;
        tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
        tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
        tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
        tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
        tp->param[HCI_EVENT_HEAD_LENGTH+4] = app_anc_channel_check();
        break;
    case ANC_CMD_CTRL_LIMITER:
        mailbox_mcu2dsp_send_until_recv(MAILBOX_CMD_FAST_RSP_FLAG | MAILBOX_CMD_ANC_CTRL, rp->param[0], rp->param[1], rp->param[2] | (rp->param[3] << 8) | (rp->param[4] << 16) | (rp->param[5] << 24));
        tp->total = HCI_EVENT_HEAD_LENGTH+8;
        tp->param[HCI_EVENT_HEAD_LENGTH+0] = rp->cmd;
        tp->param[HCI_EVENT_HEAD_LENGTH+1] = rp->param[0];
        tp->param[HCI_EVENT_HEAD_LENGTH+2] = rp->param[1];
        tp->param[HCI_EVENT_HEAD_LENGTH+3] = rp->param[2];
        tp->param[HCI_EVENT_HEAD_LENGTH+4] = rp->param[3];
        tp->param[HCI_EVENT_HEAD_LENGTH+5] = rp->param[4];
        tp->param[HCI_EVENT_HEAD_LENGTH+6] = rp->param[5];
        tp->param[HCI_EVENT_HEAD_LENGTH+7] = app_anc_channel_check();
        break;
    default:
        res = 0;
        break;
    }

    if(res) memcpy(tp->param, rp, HCI_EVENT_HEAD_LENGTH);

    return res;
}

void app_anc_gain_params_read_from_flash(uint32_t addr, uint32_t size)
{
    AncParam param;

    flash_read_data((uint8*)&param, (uint32)addr, size);

    switch(param.channel_id)
    {
    case 0xC0:
        anc_params = param;
        break;
    case 0xC1:
        anc_params1 = param;
        break;
    default:
        anc_params = param;
        anc_params.channel_id = 0xC0;
        anc_params.reserved   = 0x00;
        anc_params.noise_cancel_ec_volume = 0;
        anc_params.transparency_ec_volume = 0;
        break;
    }
}

void app_anc_filter_coefs_read_from_flash(uint32_t addr, uint32_t size)
{
    anc_filter_address = addr;

    flash_read_data((uint8*)&anc_spp_filter_var + 2, (uint32)addr, size);

    app_anc_filter_coefs_update();
}

void app_anc_vol_scaler_read_from_flash(uint32_t addr, uint32_t size)
{
    AncVolScaler scaler;

    flash_read_data((uint8*)&scaler, (uint32)addr, size);

    switch(scaler.channel_id)
    {
    case 0xC0:
        anc_vol_scaler0 = scaler;
        break;
    case 0xC1:
        anc_vol_scaler1 = scaler;
        break;
    default:
        break;
    }
}

#endif //USER_KARAOK_MODE
