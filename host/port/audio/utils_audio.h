/**
 * **************************************************************************************
 * @file    utils_audio.h
 * 
 * @author  Borg Xiao
 * @date    20230524
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#ifndef _UTILS_AUDIO_H_
#define _UTILS_AUDIO_H_

#include <stdint.h>

#define UTIL_AS_CVT_EN          1
#define UTIL_AS_DBG_EN          1

#define AS_CVT_ATTR     inline

#define SATto16bit(val)             (((val) > 32767) ? 32767 : (((val) < -32768) ? (-32768) : (val)))
#define SATto24bit(val)             (((val) > 0x7FFFFF) ? 0x7FFFFF : (((val) < ((int32_t)0xFF800000)) ? ((int32_t)0xFF800000) : (val)))
#define _shift_L(val, bits)         ((val) << (bits))
#define _shift_R(val, bits)         ((val) >> (bits))

//convert samples to bytes.(S32b->Stereo 24/32bit...)
#define SMP2BYTE_Q32b(smps)      _shift_L((int)smps, 4)//4ch*4byte
#define SMP2BYTE_Q16b(smps)      _shift_L((int)smps, 3)//4ch*2byte
#define SMP2BYTE_S32b(smps)      _shift_L((int)smps, 3)//2ch*4byte
#define SMP2BYTE_S16b(smps)      _shift_L((int)smps, 2)//2ch*2byte
#define SMP2BYTE_M32b(smps)      _shift_L((int)smps, 2)//1ch*4byte
#define SMP2BYTE_M16b(smps)      _shift_L((int)smps, 1)//1ch*2byte

//convert bytes to samples.(S32b->Stereo 24/32bit...)
#define BYTE2SMP_Q32b(size)      _shift_R((int)size, 4)//4ch*4byte
#define BYTE2SMP_Q16b(size)      _shift_R((int)size, 3)//4ch*2byte
#define BYTE2SMP_S32b(size)      _shift_R((int)size, 3)//2ch*4byte
#define BYTE2SMP_S16b(size)      _shift_R((int)size, 2)//2ch*2byte
#define BYTE2SMP_M32b(size)      _shift_R((int)size, 2)//1ch*4byte
#define BYTE2SMP_M16b(size)      _shift_R((int)size, 1)//1ch*2byte

#pragma pack(1)
typedef struct _AUD_WAV_FORMAT_t{
	uint16_t wTag;  //Compression format
	uint16_t wChn;  //channels
	uint32_t dwSampleRate;
	uint16_t wBitsPerSample;  //sample width
	uint16_t wBlockAlign;     //bytes per sample for all channels
	uint32_t dwAvgBytesPerSec;//average bytes per second
}AUD_WAV_FORMAT_t;
#pragma pack()


void aud_wav_fmt_init(void *pfmt, int freq, int chns, int width);

#if UTIL_AS_CVT_EN

#define aud_memmove_mono_32b(pcmi,pcmo,samples)     memmove((uint8_t*)(pcmo), (uint8_t*)(pcmi), SMP2BYTE_M32b(samples))
#define aud_memmove_stereo_32b(pcmi,pcmo,samples)   memmove((uint8_t*)(pcmo), (uint8_t*)(pcmi), SMP2BYTE_S32b(samples))

AS_CVT_ATTR void aud_cvt_pcm16_to_pcm24(int16_t *pcm_in, int32_t *pcm_out, int samples);
AS_CVT_ATTR void aud_cvt_pcm24_to_pcm16(int32_t *pcm_in, int16_t *pcm_out, int samples);

AS_CVT_ATTR void aud_cvt_mono_to_stereo_pcm16(int16_t *pcm_in, int16_t *pcm_out, int samples);
AS_CVT_ATTR void aud_cvt_mono_to_stereo_pcm32(int32_t *pcm_in, int32_t *pcm_out, int samples);

AS_CVT_ATTR void aud_mix_proc_pcm16(int16_t *pcm0, const int16_t *pcm1, int samples);
AS_CVT_ATTR void aud_mix_proc_pcm24(int32_t *pcm0, const int32_t *pcm1, int samples);

AS_CVT_ATTR void aud_cvt_pcm24us_to_pcm24s(int32_t *pcm_in, int32_t *pcm_out, int samples);

AS_CVT_ATTR void aud_cvt_wav24_to_pcm16(uint8_t *dat_in, int16_t *pcm_out, int samples);
AS_CVT_ATTR void aud_cvt_wav24_to_pcm24(uint8_t *dat_in, int32_t *pcm_out, int samples);

static inline int ringbuf_level_check(int fill, int target_val, int tolerance)
{
    if((fill > (target_val + tolerance)) || (fill < (target_val - tolerance)))
    {
        return (fill - target_val);
    }
    return 0;
}

#endif /* UTILS_AS_CVT_EN */


typedef struct _VOLUME_IDX_CTX_t{
    uint8_t mute;
    uint8_t vol_L;
    uint8_t vol_R;
}VOLUME_IDX_CTX_t;

void audio_volume_mute_16bit2ch(int16_t *pcmi, int16_t *pcmo, int samples);
void audio_volume_adj_16bit2ch(int16_t* pcmi, int16_t *pcmo, uint8_t vol_idx_L, uint8_t vol_idx_R, int samples);
void audio_volume_dB_16bit2ch(int16_t* pcmi, int16_t *pcmo, int8_t vol_dB_L, int8_t vol_dB_R, int samples);
void audio_volume_hdB_16bit2ch(int16_t* pcmi, int16_t *pcmo, int8_t vol_hdB_L, int8_t vol_hdB_R, int samples);
#endif /* _UTILS_AUDIO_H_ */

//EOF
