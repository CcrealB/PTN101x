#ifndef AEC_FFT_H
#define AEC_FFT_H

#include <stdint.h>
//#include "types.h"

#define    FS_AEC               (16000)

#if (FS_AEC==8000)
#define    MEM_ISOLATE          (0)
#else
#define    MEM_ISOLATE          (0)
#endif

#if (FS_AEC==8000)
#define    BD                   (1)
#else
#define    BD                   (2)
#endif

#define FFT_LEN_NB              (256)
#define FFT_LEN_WB              (FFT_LEN_NB<<1)

#define FFT_LEN_NB_HF           (FFT_LEN_NB>>1)
#define FFT_LEN_WB_HF           (FFT_LEN_WB>>1)

#define FRAME_LEN_NB            (160)
#define FRAME_LEN_WB            (FRAME_LEN_NB<<1)

#define SymWin                  (1)            // 12ms delay
#define AsymWin                 (!SymWin)      // 6ms  delay

#define FFTopt                  (0)

typedef struct FFTINFO
{
    int16_t  N;
    int16_t  M;
#if (FFTopt)
    int16_t  map[128*BD];
    int16_t  fftw[98*BD];
    int16_t  rft_win[128*BD];
    int16_t  ana_win[256*BD];
    int16_t  syn_win[(96<<SymWin)*BD];
#else
    int16_t  *map;
    int16_t  *fftw;
    int16_t  *rft_win;
    int16_t  *ana_win;
    int16_t  *syn_win;
#endif
}FFT_INFO;

void rfft_win (int32_t *cx,int32_t *dx,int16_t *win,FFT_INFO *fftInfo);

void inv_rfft (int32_t *cx,int32_t *dx,FFT_INFO *fftInfo);

void FFT_init (FFT_INFO *FFT_ptr,int16_t band);

#endif
