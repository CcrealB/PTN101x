/**
 **************************************************************************************
 * @file    aac_decoder_api.h
 * @brief   Advanced Audio Coding, AAC DECODER API.
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2018 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AAC_DECODER_API_H__
#define __AAC_DECODER_API_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "types.h"

/**
 * @brief  Get AAC decoder context size
 * @return AAC decoder context size
 */
uint32_t aac_decoder_get_ram_size(void);
uint32_t aac_decoder_get_ram_size_without_in_buffer(void);
uint32_t aac_decoder_get_sample_rate(void* aac_decoder);
uint32_t aac_decoder_get_num_channels(void* aac_decoder);
uint32_t aac_decoder_get_pcm_samples(void* aac_decoder);
uint8_t* aac_decoder_get_pcm_buffer(void* aac_decoder);
uint32_t aac_decoder_get_filled_buffer(void* aac_decoder);

/**
 * @brief  Initialize AAC decoder
 * @return initialize result, 0: success, others: failed with error code.
 */
int32_t aac_decoder_initialize(void* aac_decoder, void* handle, void* read, void* seek, void* tell);

/**
 * @brief  Decode an AAC frame
 * @return decode result, 0: no error, <0: fatal error, Cannot not be ignored, >0: Slight error, can be ignored.
 */
int32_t aac_decoder_decode(void* aac_decoder);

/**
 * @brief  Initialize AAC decoder for BT A2DP
 * @return initialize result, 0: success, others: failed with error code.
 */
int32_t bt_a2dp_aac_decoder_init(void* aac_decoder, uint32_t sample_rate, uint32_t channels);

/**
 * @brief  Decode an BT-A2DP-AAC frame
 * @return decode result, 0: no error, <0: fatal error, Cannot not be ignored, >0: Slight error, can be ignored.
 */
int32_t bt_a2dp_aac_decoder_decode(void* aac_decoder, uint8_t* inbuf, uint32_t inlen, uint8_t** outbuf, uint32_t* outlen);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AAC_DECODER_API_H__
