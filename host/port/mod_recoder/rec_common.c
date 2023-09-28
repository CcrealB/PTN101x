

#include "port_mem.h"
#include "os_common.h"
#include "timer.h"

void* rec_malloc(int size)
{
    return jmalloc(size, M_ZERO);
}

void rec_free(void* p)
{
    jfree(p);
}

/** @brief file write time monitor, log out if wr time over(time unit:ms)
 * @param wr_tms current write time
 * @param t_min log out if time our t_min ms
 * @param wr_sz 0:write start/!0:write end, the value is write size
*/
inline void file_wr_time_monitor(uint32_t *wr_tms, int t_min, int wr_sz)
{
    if(wr_sz == 0){
        *wr_tms = sys_time_get();
    }else{
        *wr_tms = sys_time_get() - *wr_tms;
        if(*wr_tms > t_min) { os_printf("\t!!! file write, sz:%d, t:%d ms\n", wr_sz, *wr_tms); }
    }
}


#ifdef DBG_REC_AS
#define DBG_WAV_SIN_LEN 48
const static int16_t g_sin_data_16bit[DBG_WAV_SIN_LEN] = {
0x0000, 0x0BD3, 0x1774, 0x22AD, 0x2D4E, 0x372A, 0x4013, 0x47E4, 0x4E7A, 0x53B8, 0x5787, 0x59D7,
0x5A9D, 0x59D7, 0x5787, 0x53B8, 0x4E7A, 0x47E4, 0x4013, 0x372A, 0x2D4E, 0x22AD, 0x1774, 0x0BD3,
0x0000, 0xF42C, 0xE88B, 0xDD52, 0xD2B1, 0xC8D5, 0xBFEC, 0xB81B, 0xB185, 0xAC47, 0xA878, 0xA628,
0xA562, 0xA628, 0xA878, 0xAC47, 0xB185, 0xB81B, 0xBFEC, 0xC8D5, 0xD2B1, 0xDD52, 0xE88B, 0xF42C};

void audio_stream_debug(int16_t *pcm, int samples)
{
    // char *p = (uint8_t*)pcm;
    // int i; for(i = 0; i < RECORD_FRAME_SAMPLES*4/16; i++) memcpy(&p[16*i], "123456789ABCDEF\n", 16);
    int i;
    static int k = 0;

    for(i = 0; i < RECORD_FRAME_SAMPLES; i++)
    {
        pcm[2*i] = g_sin_data_16bit[k];
        pcm[2*i + 1] = g_sin_data_16bit[k];
        if(++k >= DBG_WAV_SIN_LEN) k = 0;
    }
}
#endif //DBG_REC_AS

