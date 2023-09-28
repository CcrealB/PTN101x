
/**
 * **************************************************************************************
 * @file    audio_spc.h
 * 
 * @author  Borg Xiao
 * @date    20230301
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * to do:
 * **************************************************************************************
 * */

#ifndef _AUDIO_SPC_H_
#define _AUDIO_SPC_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#define API_AUDIO_SPC

/* ------------------------------------------------------------ */

typedef struct _AUD_SPC_CTX_t{//Note:must consistent with AUD_SPC_CTX_t in dsp side
    int         adj_dir;        //pts adj dir, if >0 then +pt, else -pt
    uint32_t    adj_int_smps;   //adj interval smps (unit:smps)
    uint32_t    smps_cnt;       //samples counter in audio stream
    uint32_t    adj_p_cnt;      //+pt counter in a period of time
    uint32_t    adj_n_cnt;      //-pt counter in a period of time
    int32_t     prv_val[2];
}AUD_SPC_CTX_t __attribute__((aligned(4)));

void audio_spc_init(AUD_SPC_CTX_t* spc_h);
void audio_spc_param_set(AUD_SPC_CTX_t* spc_h, int ppm);
int audio_spc_calc(AUD_SPC_CTX_t* spc_h, int fra_smps);
int audio_spc_exec(void *pcmi, void *pcmo, int smps_in, uint8_t bits, uint8_t chn, int comp_num, int apply_smps);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//_AUDIO_ASPC_H_
