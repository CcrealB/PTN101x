/**
 * **************************************************************************************
 * @file    audio_aspc.c
 * 
 * @author  Borg Xiao
 * @date    20230301
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * audio sample point compensate
 * to do:
 * **************************************************************************************
 * */
#include <stdint.h>
#include <string.h>
#include "audio_spc.h"



#ifdef API_AUDIO_SPC

#define AUDIO_COMP_SMOOTH_EN        1
#define ADJ_INTVL_GET_BY_PPM(ppm)   (1000000 / (ppm))

static int audio_spc_exec_16b2ch(int16_t* pcmi, int16_t* pcmo, int samples, int comp_num, int apply_smps);
static int audio_spc_exec_24b2ch(int32_t* pcmi, int32_t* pcmo, int samples, int comp_num, int apply_smps);

void audio_spc_init(AUD_SPC_CTX_t* spc_h)
{
    memset(spc_h, 0, sizeof(AUD_SPC_CTX_t));
    spc_h->prv_val[0] = 0x7FFFFFFF;
    spc_h->prv_val[1] = 0x7FFFFFFF;
}

/** @brief calc compesate dir and interval by adj ppm
 * @param ppm adj ppm, could be signed type, >0 add pts/<0 del pts
 * */
void audio_spc_param_set(AUD_SPC_CTX_t* spc_h, int ppm)
{
    spc_h->adj_dir = ppm;
    spc_h->adj_int_smps = ppm ? ADJ_INTVL_GET_BY_PPM((ppm < 0) ? -ppm : ppm) : (uint32_t)-1;
}

/** @brief compesate samples calc
 * @return comp num out
*/
int audio_spc_calc(AUD_SPC_CTX_t* spc_h, int fra_smps)
{
    int comp_num = 0;
    spc_h->smps_cnt += fra_smps;
    if(spc_h->smps_cnt >= spc_h->adj_int_smps)
    {
        spc_h->smps_cnt = 0;
        if(spc_h->adj_dir > 0){//add point
            comp_num = 1;
            spc_h->adj_p_cnt++;
        }else if(spc_h->adj_dir < 0){//del point
            comp_num = -1;
            spc_h->adj_n_cnt++;
        }
    }
    return comp_num;
}

/** @brief samples compesate
 * @param chn not used @230315
 * @return samples out
*/
int audio_spc_exec(void *pcmi, void *pcmo, int smps_in, uint8_t bits, uint8_t chn, int comp_num, int apply_smps)
{
    int smps_out = smps_in;

    if(comp_num == 0)
    {
        memmove(pcmo, pcmi, smps_in << (1 + (chn == 2) + (bits != 16)));
        return smps_in;
    }

    if(chn == 2)
    {
        if(bits == 16){
            smps_out = audio_spc_exec_16b2ch((int16_t*)pcmi, (int16_t*)pcmo, smps_in, comp_num, apply_smps);
        }else{
            smps_out = audio_spc_exec_24b2ch((int32_t*)pcmi, (int32_t*)pcmo, smps_in, comp_num, apply_smps);
        }
    }
    return smps_out;
}


//comp_num: -1, 0. 1, apply_smps: process sample point num at each compesate opration
static int audio_spc_exec_16b2ch(int16_t* pcmi, int16_t* pcmo, int samples, int comp_num, int apply_smps)
{
    if(samples <= 0) return 0;

    if(comp_num == 0)
    {
        memmove(pcmo, pcmi, samples << 2);
        return samples;
    }

    #if 0//AUDIO_COMP_SMOOTH_EN
    int adj_smps = samples < apply_smps ? samples : apply_smps;//set the point num used to adjust
    int delta;
    int n;
    int i = samples - adj_smps;
    int ii = i << 1;
    int16_t cur_val;
    int16_t prv_val = pcmi[ii];//left
    int16_t prv_val1 = pcmi[ii + 1];//right
    if(comp_num > 0)
    {
        pcmo[2*samples] = pcmi[2*(samples - 1)];
        pcmo[2*samples + 1] = pcmi[2*(samples - 1) + 1];
        //loop times:(adj_smps - 1), [samples - adj_smps + 1] ~ [samples - 1]
        for (i += 1,n = 1; n < adj_smps; i++, n++) //for (i += 1, n = 1; i < samples; i++, n++)
        {
            ii = i << 1;
            cur_val = pcmi[ii];
            delta = cur_val - prv_val;
            pcmo[ii] = cur_val - (delta * n / adj_smps);
            prv_val = cur_val;
            
            cur_val = pcmi[ii + 1];
            delta = cur_val - prv_val1;
            pcmo[ii + 1] = cur_val - (delta * n / adj_smps);
            prv_val1 = cur_val;
        }
        return samples + 1;
    }
    else //if(comp_num < 0)
    {
        //loop times:(adj_smps - 3), [samples - adj_smps + 1] ~ [samples - 3]
        adj_smps -= 2;
        for (i += 1, n = 1; n < adj_smps; i++, n++) //for (i += 1, n = 1; i < samples - 2; i++, n++)
        {
            ii = i << 1;
            cur_val = pcmi[ii];
            delta = cur_val - prv_val;
            pcmo[ii] = cur_val + (delta * n / adj_smps);
            prv_val = cur_val;
            
            cur_val = pcmi[ii + 1];
            delta = cur_val - prv_val1;
            pcmo[ii + 1] = cur_val + (delta * n / adj_smps);
            prv_val1 = cur_val;
        }
        pcmo[2*(samples - 2)] = pcmi[2*(samples - 1)];
        pcmo[2*(samples - 2) + 1] = pcmi[2*(samples - 1) + 1];
        return samples - 1;
    }
    // LOG_PRT("L:%d, %d, %d, smps:%d, adj:%d\n", pcmo[2*(samples - 2)], pcmo[2*(samples - 1)], pcmo[2 * samples], samples, adj_smps);
    // LOG_PRT("R:%d, %d, %d, smps:%d, adj:%d\n", pcmo[2*(samples - 2) + 1], pcmo[2*(samples - 1) + 1], pcmo[2 * samples + 1], samples, adj_smps);
    #else
    memmove(pcmo, pcmi, samples << 2);
    if(comp_num > 0){
        pcmo[2*samples] = pcmi[2*(samples - 1)];
        pcmo[2*samples + 1] = pcmi[2*(samples - 1) + 1];
        return samples + 1;
    }else{
        return samples - 1;
    }
    #endif   
}

//comp_num: -1, 0. 1, apply_smps: process sample point num at each compesate opration, exp:128
static int audio_spc_exec_24b2ch(int32_t* pcmi, int32_t* pcmo, int samples, int comp_num, int apply_smps)
{
    if(samples <= 0) return 0;

    if(comp_num == 0)
    {
        memmove(pcmo, pcmi, samples << 3);
        return samples;
    }

    #if AUDIO_COMP_SMOOTH_EN
    int adj_smps = samples < apply_smps ? samples : apply_smps;//set the point num used to adjust
    int delta;
    int n;
    int i = samples - adj_smps;
    int ii = i << 1;
    int32_t cur_val;
    int32_t prv_val = pcmi[ii];//left
    int32_t prv_val1 = pcmi[ii + 1];//right
    if(comp_num > 0)
    {
        pcmo[2*samples] = pcmi[2*(samples - 1)];
        pcmo[2*samples + 1] = pcmi[2*(samples - 1) + 1];
        //loop times:(adj_smps - 1), [samples - adj_smps + 1] ~ [samples - 1]
        for (i += 1,n = 1; n < adj_smps; i++, n++) //for (i += 1, n = 1; i < samples; i++, n++)
        {
            ii = i << 1;
            cur_val = pcmi[ii];
            delta = cur_val - prv_val;
            pcmo[ii] = cur_val - (delta * n / adj_smps);
            prv_val = cur_val;
            
            cur_val = pcmi[ii + 1];
            delta = cur_val - prv_val1;
            pcmo[ii + 1] = cur_val - (delta * n / adj_smps);
            prv_val1 = cur_val;
        }
        return samples + 1;
    }
    else //if(comp_num < 0)
    {
        //loop times:(adj_smps - 3), [samples - adj_smps + 1] ~ [samples - 3]
        adj_smps -= 2;
        for (i += 1, n = 1; n < adj_smps; i++, n++) //for (i += 1, n = 1; i < samples - 2; i++, n++)
        {
            ii = i << 1;
            cur_val = pcmi[ii];
            delta = cur_val - prv_val;
            pcmo[ii] = cur_val + (delta * n / adj_smps);
            prv_val = cur_val;
            
            cur_val = pcmi[ii + 1];
            delta = cur_val - prv_val1;
            pcmo[ii + 1] = cur_val + (delta * n / adj_smps);
            prv_val1 = cur_val;
        }
        pcmo[2*(samples - 2)] = pcmi[2*(samples - 1)];
        pcmo[2*(samples - 2) + 1] = pcmi[2*(samples - 1) + 1];
        return samples - 1;
    }
    // LOG_PRT("L:%d, %d, %d, smps:%d, adj:%d\n", pcmo[2*(samples - 2)], pcmo[2*(samples - 1)], pcmo[2 * samples], samples, adj_smps);
    // LOG_PRT("R:%d, %d, %d, smps:%d, adj:%d\n", pcmo[2*(samples - 2) + 1], pcmo[2*(samples - 1) + 1], pcmo[2 * samples + 1], samples, adj_smps);
    #else
    memmove(pcmo, pcmi, samples << 3);
    if(comp_num > 0){
        pcmo[2*samples] = pcmi[2*(samples - 1)];
        pcmo[2*samples + 1] = pcmi[2*(samples - 1) + 1];
        return samples + 1;
    }else{
        return samples - 1;
    }
    #endif   
}

#endif /* API_AUDIO_SPC */

