

/**
 * **************************************************************************************
 * @file    utils_volume.c
 * 
 * @author  Borg Xiao
 * @date    20230524
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include <stdint.h>
#include <string.h>

#define SAT16_LIM(val)      (int16_t)(((val) > 32767) ? 32767 : (((val) < -32767) ? -32767 : (val)))


#define xVOLUME_SFT     	(11)
#define X_VOLUME_BASE    	(1 << xVOLUME_SFT)
#define VOL_IDX_0dB         100
#define VOL_dB2IDX(dB)		((int8_t)(dB * 2) + VOL_IDX_0dB)//dB[-50(mute),-49,-48,0,1,2...,23,24]
#define VOL_IDX2dB(idx)     ((int8_t)((idx - VOL_dB2IDX(0)) / 2))


//0.5dB step, [mute, -49.5dB ~ 24dB], mute~0~x_volume_tab[0], 0dB~2048~x_volume_tab[100], 24dB~32459~x_volume_tab[148].
const uint16_t x_volume_tab[149]={0/*mute*/,  //6/*-50dB, not used*/,
    7,     7,     8,     8,     9,     9,    10,    10,    11,    12,
   12,    13,    14,    14,    15,    16,    17,    18,    19,    20,
   22,    23,    24,    26,    27,    29,    31,    32,    34,    36,
   39,    41,    43,    46,    49,    51,    54,    58,    61,    65,
   69,    73,    77,    82,    86,    91,    97,   103,   109,   115,
  122,   129,   137,   145,   154,   163,   172,   183,   193,   205,
  217,   230,   243,   258,   273,   289,   306,   325,   344,   364,
  386,   409,   433,   458,   486,   514,   545,   577,   611,   648,
  686,   727,   770,   815,   864,   915,   969,  1026,  1087,  1152,
 1220,  1292,  1369,  1450,  1536,  1627,  1723,  1825,  1933,  2048,//0dB~2048
 2169,  2298,  2434,  2578,  2731,  2893,  3064,  3246,  3438,  3642,
 3858,  4086,  4328,  4585,  4857,  5144,  5449,  5772,  6114,  6476,
 6860,  7267,  7697,  8153,  8636,  9148,  9690, 10264, 10873, 11517,
12199, 12922, 13688, 14499, 15358, 16268, 17232, 18253, 19334, 20480,
21694, 22979, 24341, 25783, 27311, 28929, 30643, 32459};


void audio_volume_mute_16bit2ch(int16_t *pcmi, int16_t *pcmo, int samples)
{
    int size  = samples << 2;
    memmove(pcmo, pcmi, size);
    memset(pcmo, 0, size);
}

/// @brief 
/// @param pcmi 
/// @param pcmo 
/// @param vol_idx_L [in] index of volume table in left channel, range:0 ~ 148
/// @param vol_idx_R refer param vol_idx_L
/// @param samples 
inline void audio_volume_adj_16bit2ch(int16_t* pcmi, int16_t *pcmo, uint8_t vol_idx_L, uint8_t vol_idx_R, int samples)
{
    int32_t i, pcm32;
    uint8_t idx_max = sizeof(x_volume_tab) / sizeof(x_volume_tab[0]);
    if(vol_idx_L > idx_max) vol_idx_L = idx_max;
    if(vol_idx_R > idx_max) vol_idx_R = idx_max;

    int vol_L = x_volume_tab[vol_idx_L];
    int vol_R = x_volume_tab[vol_idx_R];
    for (i = 0; i < samples; i++)
    {
        pcm32 = (pcmi[2*i] * vol_L) >> xVOLUME_SFT;
        pcmo[2*i] = SAT16_LIM(pcm32);

        pcm32 = (pcmi[2*i + 1] * vol_R) >> xVOLUME_SFT;
        pcmo[2*i + 1] = SAT16_LIM(pcm32);
    }
}

/// @brief 
/// @param pcmi 
/// @param pcmo 
/// @param vol_hdB_L[in] dB value in left channel, range:-50 ~ 24(-50dB ~ 24dB, step:1dB)
/// @param vol_hdB_R refer param vol_dB_L
/// @param samples 
void audio_volume_dB_16bit2ch(int16_t* pcmi, int16_t *pcmo, int8_t vol_dB_L, int8_t vol_dB_R, int samples)
{
    audio_volume_adj_16bit2ch(pcmi, pcmo, VOL_dB2IDX(vol_dB_L), VOL_dB2IDX(vol_dB_R), samples);
}


/// @brief 
/// @param pcmi 
/// @param pcmo 
/// @param vol_hdB_L[in] half of dB value in left channel, range:-100 ~ 48(-50dB ~ 24dB, step:0.5dB)
/// @param vol_hdB_R refer param vol_hdB_L
/// @param samples 
void audio_volume_hdB_16bit2ch(int16_t* pcmi, int16_t *pcmo, int8_t vol_hdB_L, int8_t vol_hdB_R, int samples)
{
    audio_volume_adj_16bit2ch(pcmi, pcmo, VOL_IDX_0dB + vol_hdB_L, VOL_IDX_0dB + vol_hdB_R, samples);
}
