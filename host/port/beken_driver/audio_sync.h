/**
 **************************************************************************************
 * @file    audio_sync.h
 * @brief   Audio synchronization
 * 
 * @author  Aixing.Li
 * @version V2.2.0
 *
 * &copy; 2017-2020 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_SYNC_H__
#define __AUDIO_SYNC_H__

#include "types.h"
#include "audio_spc.h"

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief  Audio synchronization initialization
 * @return NULL
 */
void audio_sync_init(void);

/**
 * @brief  Prepare for DAC open
 * @param  t the playing time for the received stream packet
 */
void audio_sync_prepare_for_dac_open(uint32_t);

/**
 * @brief  Process for DAC open
 * @return NULL
 */
void audio_sync_process_for_dac_open(void);

/**
 * @brief  Audio synchronization process
 * @param  tpm the received node frame expected playing time
 * @return 0: should not discard or add a node frame,
 *         -1: should discard a node frame
 *          1: should add a node frame
 */
int32_t audio_sync_process(uint32_t tp);

/**
 * @brief  Get current BT clock
 * @param  NULL
 * @return current BT clock
 */
uint32_t audio_sync_get_bt_clk(void);

/**
 * @brief  Get audio synchronization process flag
 * @param  NULL
 * @return audio synchronization process flag
 */
uint32_t audio_sync_get_flag(void);

/**
 * @brief  Set audio synchronization process flag
 * @param  audio synchronization process flag
 * @return NULL
 */
void audio_sync_set_flag(uint32_t flag);

/**
 * @brief  Calculate audio DAC coefficient offset
 * @param  NULL
 * @return audio DAC coefficient offset
 */
int32_t audio_sync_calc_dac_coef_offset(void);

/**
 * @brief  Get audio DAC coefficient offset
 * @param  NULL
 * @return audio DAC coefficient offset
 */
int32_t audio_sync_get_dac_coef_offset(void);

/**
 * @brief  Set audio DAC coefficient offset
 * @param  audio DAC coefficient offset
 * @return NULL
 */
void audio_sync_set_dac_coef_offset(int32_t offset);

#if 1//BT_AUD_SYNC_ADJ_BY_SW

void audio_sync_sys_init(void);
void aud_sync_log_on_set(uint8_t log_flag);
uint8_t aud_sync_log_is_on(void);

void aud_bt_sbc_clk_proc(void);
void audio_sync_reset(void);
AUD_SPC_CTX_t *aud_sync_bt_hdl_get(void);
void aud_sync_bt_debug(void);
int aud_async_src_proc(AUD_SPC_CTX_t *aud_sync_h, int32_t *pcm, int smps_in);
#endif
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_SYNC_H__
