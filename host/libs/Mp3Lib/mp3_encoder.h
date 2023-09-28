/**
 **************************************************************************************
 * @file    mp3_encoder.h
 * @brief   An MPEG layer III encoder inplementation
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2023 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __MP3_ENCODER_H__
#define __MP3_ENCODER_H__

#include <stdint.h>


#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


#define B_STAT_PREFIX                         bk
#ifdef  B_STAT_PREFIX
#define B_STATCC1(x, y, z)                    B_STATCC2(x, y, z)
#define B_STATCC2(x, y, z)                    x##y##z
#define B_STATNAME(func)                      B_STATCC1(B_STAT_PREFIX, _, func)
#else
#define B_STATNAME(func)                      func
#endif


#define mp3_encoder_size                    B_STATNAME(mp3_encoder_size)
#define mp3_encoder_init                    B_STATNAME(mp3_encoder_init)
#define mp3_encoder_ctrl                    B_STATNAME(mp3_encoder_ctrl)
#define mp3_encoder_encode                  B_STATNAME(mp3_encoder_encode)
#define mp3_encoder_deinit                  B_STATNAME(mp3_encoder_deinit)
#define mp3_encoder_get_samples_per_frame   B_STATNAME(mp3_encoder_get_samples_per_frame)

typedef enum
{
    MP3_ENCODER_ERROR_NONE = 0,
    MP3_ENCODER_ERROR_UNSUPPORTED_SAMPLERATE,
    MP3_ENCODER_ERROR_UNSUPPORTED_BITRATE,
    MP3_ENCODER_ERROR_UNSUPPORTED_CTRL_CMD,
    MP3_ENCODER_ERROR_INVALID_OUT_BUFFER,
    MP3_ENCODER_ERROR_INVALID_CTRL_PARAMETER,
}MP3_ENCODER_ERROR;

typedef enum
{
    MP3_ENCODER_CTRL_CMD_NULL = 0,
    MP3_ENCODER_CTRL_CMD_GET_CODE_SBLIMIT,
    MP3_ENCODER_CTRL_CMD_SET_CODE_SBLIMIT,
}MP3_ENCODER_CTRL_CMD;

typedef struct _MP3Encoder MP3Encoder;

/**
 * @brief Get MP3 encoder context size
 * @return size of MP3 encoder context
 */
int32_t mp3_encoder_size(void);

/**
 * @brief Initialize MP3 encoder
 * @param[in] mp3           MP3 encoder struct pointer
 * @param[in] sampling_rate encoder parameter: sampling rate, only below vales are supported.
 *            - 44100, 48000, 32000 for MPEG-I
 *            - 22050, 24000, 16000 for MPEG-II
 *            - 11025, 12000,  8000 for MPEG-2.5
 * @param[in] num_channels  encoder parameter: number of channels, only 1(MONO) and 2(STEREO) supported.
 * @param[in] bitrate       encoder parameter: bit rate, unit in kbps, only below vales are supported.
 *            - 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 for MPEG-I
 *            - 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 for MPEG-II and MPEG-2.5
 * @return error code, @ref MP3_ENCODER_ERROR
 */
int32_t mp3_encoder_init(MP3Encoder* mp3, int32_t sampling_rate, int32_t num_channels, int32_t bitrate);

/**
 * @brief Control MP3 encoder
 * @param[in] mp3 MP3 encoder struct pointer
 * @param[in] cmd control command, @ref MP3_ENCODER_CTRL_CMD
 * @param[in] arg control parameter
 * @return error code, @ref MP3_ENCODER_ERROR
 */
int32_t mp3_encoder_ctrl(MP3Encoder* mp3, uint32_t cmd, uint32_t arg);

/**
 * @brief Encode a MP3 frame
 * @param[in]  mp3    MP3 encoder struct pointer
 * @param[out] data   address of encoded data.
 * @param[out] length length of encoded data, unit in byte
 * @return error code, @ref MP3_ENCODER_ERROR
 */
int32_t mp3_encoder_encode(MP3Encoder* mp3, int16_t* pcm/*[mp3->samples_per_frame * mp3->num_channels]*/, uint8_t* data, uint32_t* length);

/**
 * @brief De-Initialize MP3 encoder
 * @param[in] mp3 MP3 encoder struct pointer
 * @return error code, @ref MP3_ENCODER_ERROR
 */
int32_t mp3_encoder_deinit(MP3Encoder* mp3);

/**
 * @brief  Get MP3 encoder context size
 * @param[in] mp3 MP3 encoder struct pointer
 * @return samples per frame per channel
 */
int32_t mp3_encoder_get_samples_per_frame(MP3Encoder* mp3);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__MP3_ENCODER_H__
