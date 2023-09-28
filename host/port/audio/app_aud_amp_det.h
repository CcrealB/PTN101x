/**
 * **************************************************************************************
 * @file    app_aud_amp_det.h
 * 
 * @author  Borg Xiao
 * @date    20230612
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * **************************************************************************************
 * */
#ifndef _APP_AUD_AMP_DET_H_
#define _APP_AUD_AMP_DET_H_

#include <stdint.h>

// -------- user api
uint8_t aud_mag_det_enable_get(void);
void aud_mag_det_enable(uint8_t en);//audio magnitude detect enable
int16_t aud_mag_get_freq(uint8_t freq_idx);//freq_idx: 0 ~ (freq_num-1)(freq_num为设定的最大频点数)
int16_t aud_mag_get_freq_low(void) ;// freq 0
int16_t aud_mag_get_freq_mid(void) ;// freq 1
int16_t aud_mag_get_freq_high(void);// freq 2

// -------- system api
void aud_d2m_pcm16_LR_for_fft(int16_t *pcm, int samples); //audio data update, called in audio stream[note:must meet samples <= 256]
void audio_det_fft_task(unsigned int (*cbk_systime_get)(void));// fft proc task

// extern call
extern int32_t os_printf(const char *fmt, ...);
extern void SYSirq_Disable_Interrupts_Save_Flags(uint32_t *flags, uint32_t *mask);
extern void SYSirq_Interrupts_Restore_Flags(uint32_t flags, uint32_t mask);
extern uint32_t dbg_show_dynamic_mem_info(uint8_t en);
extern void *jmalloc_s(uint32_t size, uint16_t flags);
extern void jfree_s(void *item);

#endif
