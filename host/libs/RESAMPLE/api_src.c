
#include "src.h"

#if UTILS_AUD_SRC_SEL

#include "os_common.h"
#include "port_generic.h"
#include "app_beken_includes.h"
#include "api_src.h"
#include "utils_audio.h"

typedef int32_t (*pFunc_src_t)(void*, void*, uint32_t, void*, uint32_t, int32_t);

int src_id_get(int fs_in, int fs_out);
int src_248_id_get(int fs_in);
int src_244_id_get(int fs_in);

/// @brief sample rate convert
/// @param src handler
/// @return err code
int audio_src_init(aud_src_ctx_t *src)
{
    int ret = 0;
    int pFunc_ok = 0;
    int src_id = 0;//SRC248_ID_NULL;
    int src_sz = 0;

    uint8_t* (*pFunc_src_verstr)(void);
    int32_t (*pFunc_src_init)(void*, int32_t, int32_t, int32_t);
    int32_t (*pFunc_src_size)(int32_t, int32_t, int32_t);

    audio_src_uninit(src);
    if((src->fs_out == 0) || (src->fs_in == src->fs_out)) { goto RET;}

    //pFunc init and src_id get
    switch (src->fs_out)
    {
#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC248)
    case 48000: 
        pFunc_src_verstr = src_248_verstr;
        pFunc_src_size = src_248_size;
        pFunc_src_init = src_248_init;
        src->pFunc_src = src_248_exec;
        src_id = src_248_id_get(src->fs_in);
        pFunc_ok = 1;
        break;
#endif
#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC244)
    case 44100: 
        pFunc_src_verstr = src_244_verstr;
        pFunc_src_size = src_244_size;
        pFunc_src_init = src_244_init;
        src->pFunc_src = src_244_exec;
        src_id = src_244_id_get(src->fs_in);
        pFunc_ok = 1;
        break;
#endif
#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC)
    default:
        pFunc_src_verstr = src_verstr;
        pFunc_src_size = src_size;
        pFunc_src_init = src_init;
        src->pFunc_src = src_exec_ex;
        src_id = src_id_get(src->fs_in, src->fs_out);
        pFunc_ok = 1;
        break;
#endif
    }

    if(!pFunc_ok)
    {
        LOG_E(SRC, "##ERROR: src pFunc init fail!\n");
        ret = -1;
        goto RET;
    }

    LOG_I(SRC,"src ver:%s\n", pFunc_src_verstr());
    if(src_id == 0) { goto RET;}
    else if(src_id == -1) {
        LOG_E(SRC, "src fs in not supported!\n");
        ret = -1;
        goto RET;
    }
    //get buff size that src needed
    src_sz = pFunc_src_size(src_id, src->max_fra_sz, src->chs);//size=9908byte@(24bit2ch,max_fra_sz=384), size=5012 byte@(24bit2ch,max_fra_sz=192)
    src->src_buf = jmalloc(src_sz, M_ZERO);
    if(src->src_buf)
    {
        int ini_ret = pFunc_src_init(src->src_buf, src_id, src->max_fra_sz, src->chs);
        if(ini_ret != 0){
            LOG_E(SRC, "src_init fail:%d\n", ini_ret);
            audio_src_uninit(src);
            ret = -2;
        }
    }
    else
    {
        LOG_E(SRC, "src buff malloc fail!\n");
        ret = -3;
    }
RET:
    LOG_I(SRC,"src:%d->%d, id:%d, sz:%d, max_fra_sz:%d\n", src->fs_in, src->fs_out, src_id, src_sz, src->max_fra_sz);
    LOG_I(SRC,"src ch:%d, bits:%d->%d\n", src->chs, src->bits_in, src->bits_out);
    return ret;
}

//samples: samples in
//return out sample num
int audio_src_apply(aud_src_ctx_t *src, void* pcmi, void* pcmo, int samples)
{
    int smps_out = samples;
    if((src->fs_in != src->fs_out))
    {
        if(src->src_buf != NULL) {
            smps_out = ((pFunc_src_t)src->pFunc_src)(src->src_buf, pcmi, src->bits_in, pcmo, src->bits_out, samples);
        }else{
            LOG_E(SRC, "%d:%s() ERROR:src_buf is NULL !!!\n", __LINE__, __FUNCTION__);
            smps_out = 0;
        }
    }
    else
    {
        if(src->bits_in == src->bits_out) { memmove(pcmo, pcmi, samples * ((src->bits_in == 16) ? 2 : 4) * src->chs); }
        else if(src->bits_in == 16) { aud_cvt_pcm16_to_pcm24((int16_t*)pcmi, (int32_t*)pcmo, samples * src->chs); }
        else if(src->bits_in == 24) { aud_cvt_pcm24_to_pcm16((int32_t*)pcmi, (int16_t*)pcmo, samples * src->chs); }
        else smps_out = 0;
    }
    return smps_out;
}

void audio_src_uninit(aud_src_ctx_t *src)
{
    if (src->src_buf != NULL)
    {
        jfree(src->src_buf);
        src->src_buf = NULL;
    }
}

/**
 * @brief calc the next sample num src
 * @param p_aud_fra_cnt : should be global var, used to calc sample num in audio process
 * */
int as_fra4ms_smps_get(uint8_t *p_aud_fra_cnt, int sample_rate)
{
    int samples;
    switch (sample_rate)
    {
    case 44100://4ms, 176.4->192, 176.4=(176*3+177*2)/5
    {
        uint8_t src_frame_cnt = *p_aud_fra_cnt;
        if(++src_frame_cnt > 5) src_frame_cnt = 1;
        samples = (src_frame_cnt <= 3) ? 176 : 177;
        *p_aud_fra_cnt = src_frame_cnt;
    }
    break;
    case 22050://4ms, 88.2->192, 88.2=(88*4+89)/5
    {
        uint8_t src_frame_cnt = *p_aud_fra_cnt;
        if(++src_frame_cnt > 5) src_frame_cnt = 1;
        samples = (src_frame_cnt <= 4) ? 88 : 89;
        *p_aud_fra_cnt = src_frame_cnt;
    }
    break;
    case 11025://4ms, 44.1->192, 44.1=(44*9+45)/10
    {
        uint8_t src_frame_cnt = *p_aud_fra_cnt;
        if(++src_frame_cnt > 10) src_frame_cnt = 1;
        samples = (src_frame_cnt <= 9) ? 44 : 45;
        *p_aud_fra_cnt = src_frame_cnt;
    }
    break;
    case 48000: samples = 192;  break;
    case 32000: samples = 128;  break;
    case 16000: samples = 64;   break;
    case  8000: samples = 32;   break;
    default: samples = 0;       break;
    }
    return samples;
}



#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC244)
int src_244_id_get(int fs_in)
{
    SRC244_ID_e src_id = SRC244_ID_NULL;
    switch (fs_in)
    {
    case 8000 : src_id = SRC244_ID_U441D80; break;
    case 11025: src_id = SRC244_ID_U4; break;
    case 16000: src_id = SRC244_ID_U441D160; break;
    case 22050: src_id = SRC244_ID_U2; break;
    case 24000: src_id = SRC244_ID_U147D80; break;
    case 32000: src_id = SRC244_ID_U441D320; break;
    case 44100: src_id = SRC244_ID_NULL; break;
    case 48000: src_id = SRC244_ID_U147D160; break;
    default: src_id = -1; break;
    }
    return src_id;
}
#endif

#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC248)
int src_248_id_get(int fs_in)
{
    SRC248_ID_e src_id = SRC248_ID_NULL;
    switch (fs_in)
    {
    case 8000 : src_id = SRC248_ID_U6; break;
    case 11025: src_id = SRC248_ID_U640D147; break;
    case 16000: src_id = SRC248_ID_U3; break;
    case 22050: src_id = SRC248_ID_U320D147; break;
    case 24000: src_id = SRC248_ID_U2; break;
    case 32000: src_id = SRC248_ID_U3D2; break;
    case 44100: src_id = SRC248_ID_U160D147; break;
    case 48000: src_id = SRC248_ID_NULL; break;
    default: src_id = -1; break;
    }
    return src_id;
}
#endif


#if (UTILS_AUD_SRC_SEL & UTILS_AUD_SRC)
int src_id_get(int fs_in, int fs_out)
{
    int src_id = SRC_ID_NULL;
    if(fs_out == 44100)
    {
        switch (fs_in)
        {
        case 16000: src_id = SRC_ID_U441D160; break;
        case 22050: src_id = SRC_ID_U2; break;
        case 32000: src_id = SRC_ID_U441D320; break;
        case 44100: src_id = SRC_ID_NULL; break;
        case 48000: src_id = SRC_ID_U147D160; break;
        default: src_id = -1; break;
        }
    }
    else if(fs_out == 48000)
    {
        switch (fs_in)
        {
        case 8000 : src_id = SRC_ID_U6; break;
        case 16000: src_id = SRC_ID_U3; break;
        case 24000: src_id = SRC_ID_U2; break;
        case 32000: src_id = SRC_ID_U3D2; break;
        case 44100: src_id = SRC_ID_U160D147; break;
        case 48000: src_id = SRC_ID_NULL; break;
        default: src_id = -1; break;
        }
    }
    else if(fs_out == 16000)
    {
        switch (fs_in)
        {
        case 8000 : src_id = SRC_ID_U2; break;
        case 16000: src_id = SRC_ID_NULL; break;
        case 24000: src_id = SRC_ID_U2D3; break;
        case 32000: src_id = SRC_ID_D2; break;
        case 44100: src_id = SRC_ID_U160D441; break;
        case 48000: src_id = SRC_ID_D3; break;
        default: src_id = -1; break;
        }
    }
    else
    {
        src_id = -1;
    }
    // if(src_id == SRC_ID_NULL) os_printf("src mode not supported, in:%d, out:%d\n", fs_in, fs_out);
    // else if(src_id == -1) os_printf("src none, in:%d, out:%d\n", fs_in, fs_out);
    return src_id;
}
#endif //UTILS_AUD_SRC


#endif