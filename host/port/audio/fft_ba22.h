#ifndef _FFT_BA22_H_
#define _FFT_BA22_H_

#include <stdint.h>

// #define FFT_Idx2Freq(fs,len,idx)    ((int)(idx * fs / len))
// #define FFT_Freq2Idx(fs,len,freq)   ((int)(freq / (fs / len)))

void ba22_fft(int16_t* real, int16_t* imag, int len, uint8_t ifft_flag);
int16_t ba22_cmplx_mag_q15_single(int16_t *real, int16_t *imag, int idx);
void ba22_cmplx_mag_q15(int16_t *real, int16_t *imag, int16_t* mag, int len);

void fft_test(void);

#endif