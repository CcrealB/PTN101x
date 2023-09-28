#ifndef _APP_AUDIO_H_
#define _APP_AUDIO_H_
#include <stdint.h>

#define SRC_OUT_SAMPLERATE      48000

typedef struct aud_src_ctx{
//// config by user, used by src
    int fs_in;          //sample rate in
    int fs_out;         //sample rate out
    uint8_t chs;        //channel num
    uint8_t bits_in;    //bit width, 16 or 24
    uint8_t bits_out;   //bit width, 16 or 24
    int max_fra_sz;     //used by src lib func, config by app, related to audio frame size
//// config & used by user
    int max_smps_in;    //max samples input, the var larger the larger size of src buff needed
    int max_smps_out;   //not used@221122
//// config & used by src
    void *pFunc_src;
    void *src_buf;      //src buffer addr
}aud_src_ctx_t;

int audio_src_init(aud_src_ctx_t *src);
//return out sample num
int audio_src_apply(aud_src_ctx_t *src, void* pcmi, void* pcmo, int samples);

void audio_src_uninit(aud_src_ctx_t *src);


int as_fra4ms_smps_get(uint8_t *p_aud_fra_cnt, int sample_rate);

#endif //_APP_AUDIO_H_
