/**
 * **************************************************************************************
 * @file    utils_audio.c
 * 
 * @author  Borg Xiao
 * @date    20230524
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include "utils_audio.h"

#if UTIL_AS_DBG_EN

#define DBG_AUD_SIN_LEN     48

// Audio sin data 1000Hz@(48000Hz,16bit, -3.0dBFS), sample_num: 48 point@(1 period)
const int16_t g_sin_data_16bit[DBG_AUD_SIN_LEN] = {
0x0000, 0x0BD3, 0x1774, 0x22AD, 0x2D4E, 0x372A, 0x4013, 0x47E4, 0x4E7A, 0x53B8, 0x5787, 0x59D7,
0x5A9D, 0x59D7, 0x5787, 0x53B8, 0x4E7A, 0x47E4, 0x4013, 0x372A, 0x2D4E, 0x22AD, 0x1774, 0x0BD3,
0x0000, 0xF42C, 0xE88B, 0xDD52, 0xD2B1, 0xC8D5, 0xBFEC, 0xB81B, 0xB185, 0xAC47, 0xA878, 0xA628,
0xA562, 0xA628, 0xA878, 0xAC47, 0xB185, 0xB81B, 0xBFEC, 0xC8D5, 0xD2B1, 0xDD52, 0xE88B, 0xF42C};

// Audio sin data 1000Hz@(48000Hz,24bit, -3.0dBFS), sample_num: 48 point@(1 period), peak_val = 5938679.
const int32_t g_sin_data_24bit[DBG_AUD_SIN_LEN] = {
0x00000000, 0x000BD3F1, 0x00177413, 0x0022AD7A, 0x002D4EFB, 0x00372A06, 0x00401370, 0x0047E42F, 0x004E7A06, 0x0053B81F, 0x00578783, 0x0059D780,
0x005A9DF6, 0x0059D780, 0x00578783, 0x0053B820, 0x004E7A06, 0x0047E42E, 0x00401370, 0x00372A06, 0x002D4EFA, 0x0022AD79, 0x00177412, 0x000BD3F1,
0x00000000, 0xFFF42C0F, 0xFFE88BED, 0xFFDD5285, 0xFFD2B104, 0xFFC8D5FA, 0xFFBFEC91, 0xFFB81BD2, 0xFFB185F9, 0xFFAC47E0, 0xFFA8787D, 0xFFA62880,
0xFFA5620A, 0xFFA62880, 0xFFA8787D, 0xFFAC47E1, 0xFFB185FB, 0xFFB81BD2, 0xFFBFEC92, 0xFFC8D5FA, 0xFFD2B106, 0xFFDD5287, 0xFFE88BEC, 0xFFF42C12};
#endif

//wav format
void aud_wav_fmt_init(void *pfmt, int freq, int chns, int width)
{
    AUD_WAV_FORMAT_t *fmt = (AUD_WAV_FORMAT_t *)pfmt;
    fmt->dwSampleRate = freq;
    fmt->wChn = chns;
    fmt->wBitsPerSample = width;
    fmt->wTag = 1;
    fmt->wBlockAlign = chns * width / 8;
    fmt->dwAvgBytesPerSec = fmt->dwSampleRate * fmt->wBlockAlign;
}

#if UTIL_AS_CVT_EN

/**
 * @brief width convert, from 16bit to 32bit, mono or stereo.
 * @param pcm_in : input pointer
 * @param pcm_out : output pointer, can be the same as input
 * @param samples : num of sample point, multiply by 2 for stereo
 * */
AS_CVT_ATTR void aud_cvt_pcm16_to_pcm24(int16_t *pcm_in, int32_t *pcm_out, int samples)
{
    int i = samples;
    while(i--) pcm_out[i] = (pcm_in[i] << 16) >> 8;
    // while(i--) pcm_out[i] = pcm_in[i] << 8;
}

AS_CVT_ATTR void aud_cvt_pcm24_to_pcm16(int32_t *pcm_in, int16_t *pcm_out, int samples)
{
    int i = 0;
    for (i = 0; i < samples; i++) pcm_out[i] = (int16_t)((pcm_in[i] >> 8) & 0xFFFF);
}


/**
 * @brief channel convert, from mono to stereo, 16bit, 24bit or 32bit.
 * @param pcm_in : input pointer
 * @param pcm_out : output pointer, can be the same as input
 * @param samples : num of sample point input
 * @note pcm_in and pcm_out data type need to be consistent(int16 or int32)
 * */
AS_CVT_ATTR void aud_cvt_mono_to_stereo_pcm16(int16_t *pcm_in, int16_t *pcm_out, int samples)
{
    int i = samples;
    if(i == 0) return;
    while (i--){
        pcm_out[2*i] = pcm_in[i];
        pcm_out[2*i + 1] = pcm_in[i];
    }
}

AS_CVT_ATTR void aud_cvt_mono_to_stereo_pcm32(int32_t *pcm_in, int32_t *pcm_out, int samples)
{
    int i = samples;
    if(i == 0) return;
    while (i--){
        pcm_out[2*i] = pcm_in[i];
        pcm_out[2*i + 1] = pcm_in[i];
    }
}


/**
 * @brief mix pcm0 and pcm1 then output at pcm0.
 * @param samples : num of sample point input
 * */
AS_CVT_ATTR void aud_mix_proc_pcm16(int16_t *pcm0, const int16_t *pcm1, int samples)
{
    int i = 0;
    int32_t pcm = 0;
    for (i = 0; i < samples; i++){
        pcm = pcm0[i] + pcm1[i];
        if(pcm > 32767) pcm = 32767;
        else if(pcm < -32768) pcm = -32768;
        pcm0[i] = (int16_t)pcm;
    }
}

AS_CVT_ATTR void aud_mix_proc_pcm24(int32_t *pcm0, const int32_t *pcm1, int samples)
{
    int i = 0;
    for (i = 0; i < samples; i++){
        pcm0[i] += pcm1[i];
        if(pcm0[i] > (int32_t)0x007FFFFF) pcm0[i] = (int32_t)0x007FFFFF;
        else if(pcm0[i] < (int32_t)0xFF800000) pcm0[i] = (int32_t)0xFF800000;
    }
}


/**
 * @brief deal the audio data with format 0x00FFFFFF, convert to correct signed 32bit data.
 * @param pcm_in : input pointer
 * @param pcm_out : output pointer, can be the same as input
 * @param samples : num of sample point, multiply by 2 for stereo
 * */
AS_CVT_ATTR void aud_cvt_pcm24us_to_pcm24s(int32_t *pcm_in, int32_t *pcm_out, int samples)
{
    int i = 0;
    for (i = 0; i < samples; i++) pcm_out[i] = (pcm_in[i] << 8) >> 8;
}


/**
 * @brief 24bit wav file to pcm 16/24bit, mono/stereo(block_align == 3/6) compatible.
 * @param dat_in : input pointer
 * @param pcm_out : output pointer, can be the same as input for 16bit out
 * @param samples : num of sample point, multiply by 2 for stereo
 * */
AS_CVT_ATTR void aud_cvt_wav24_to_pcm16(uint8_t *dat_in, int16_t *pcm_out, int samples)
{
    int i, k;
    for(i = 0; i < samples; i++, k += 3){
        pcm_out[i] = (int16_t)(((int16_t)dat_in[k + 2] << 8) | dat_in[k + 1]);
    }
}

AS_CVT_ATTR void aud_cvt_wav24_to_pcm24(uint8_t *dat_in, int32_t *pcm_out, int samples)
{
    int i = samples - 1;
    int k = i * 3;
    if(samples <= 0) return;
    for(; i >= 0; i--, k -= 3){
        pcm_out[i] = (int32_t)((((int32_t)dat_in[k + 2] << 24) >> 8) | ((int32_t)dat_in[k + 1] << 8) | dat_in[k]);
    }
}

#endif /* UTILS_AS_CVT_EN */

//EOF
