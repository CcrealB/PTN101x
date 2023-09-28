#ifndef _APP_AEC_H
#define _APP_AEC_H

#include <stdint.h>
#include "aec.h"


#include "compiler_arm.h"
#if 1//def CONFIG_APP_EQUANLIZER
#include "app_equ.h"
#endif

typedef struct __app_hfp_cfg_s
{
   int8    aec_fft_shift;
   uint8_t aec_decay_time;
   int8    aec_ear_gain;
   int8    aec_mic_gain;
   int16_t   hfp_spk_eq_gain;
   uint16_t   hfp_spk_eq_enable;
   app_eq_para_t hfp_spk_eq_para[4];
   int16_t   hfp_mic_eq_gain;
   uint16_t   hfp_mic_eq_enable;
   app_eq_para_t hfp_mic_eq_para[4];
} __PACKED_POST__ app_hfp_cfg_t;

void app_aec_init(int32_t sample_rate);
void app_aec_uninit(void);
void app_aec_fill_rin_buf(uint8_t* buf, uint8_t fid, uint32_t len);
int32_t app_aec_read_out_buf(uint8_t* buf, uint32_t len);
void app_aec_swi(void);
void app_aec_set_params(uint8_t* params);

#endif
