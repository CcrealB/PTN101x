/**
 * **************************************************************************************
 * @file    hal_dsp_audio.h
 * 
 * @author  Borg Xiao
 * @date    20230508
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#ifndef _HAL_AUDIO_DSP_H_
#define _HAL_AUDIO_DSP_H_

#include "config.h"


int mbx_cmd_mcu2dsp_set_single(uint32_t *p_cmd, uint8_t cmd_sz, uint8_t fast_en);
int mbx_cmd_mcu2dsp_get_single(uint32_t *p_cmd, uint8_t cmd_sz);
int mbx_cmd_mcu2dsp_set(uint8_t *cmd_buf, int cmd_size, int cmd_num);
int mbx_cmd_mcu2dsp_get(uint8_t *cmd_buf, int cmd_size, int cmd_num);

#endif
