#include <stdint.h>
#include <string.h>
#include <math.h>
// #include "bkreg.h"
// #include "config.h"
/* ------------------------------------------------------------ */ //config

// #define CONFIG_FFT_SOFT
#if 1//    defined(__BA2__)
    #define __CORE_BA2__
#endif

/* ------------------------------------------------------------ */ //debug
#define FFT_PRINTF              os_printf
#define FFT_LOG_I(fmt,...)      os_printf("[FFT|I]"fmt, ##__VA_ARGS__)

// #define FFT_RUN_TEST//define @ ui_xxx.h
// #define FFT_RUN_TIME_DBG
#ifdef FFT_RUN_TIME_DBG
    #include "bkreg.h"
    #define FFT_BA2x_RUN_START(en)      REG_GPIO_0x0D = en ? 2 : 0
    #define FFT_SOFT_RUN_START(en)      REG_GPIO_0x0E = en ? 2 : 0
    #define MATH_SQRT_RUN_START(en)     REG_GPIO_0x0D = en ? 2 : 0
#else
    #define FFT_BA2x_RUN_START(en)
    #define FFT_SOFT_RUN_START(en)
    #define MATH_SQRT_RUN_START(en)
#endif

/* ------------------------------------------------------------ */ //macro

#define BK3000_PMU_PERI_PWDS        (*((volatile unsigned long*)(0x01000080 + 0 * 4)))
#define BK3000_FFT_FFT_CONF         (*((volatile unsigned long*)(0x01020000 + 0 * 4)))
#define BK3000_FFT_FIR_CONF         (*((volatile unsigned long*)(0x01020000 + 1 * 4)))
#define BK3000_FFT_DATA_PORT        (*((volatile unsigned long*)(0x01020000 + 2 * 4)))
#define BK3000_FFT_FFT_STATUS       (*((volatile unsigned long*)(0x01020000 + 6 * 4)))
#define BK3000_FFT_FFT_START        (*((volatile unsigned long*)(0x01020000 + 7 * 4)))
#define bit_PMU_FFT_PWD             (1 << 0)
#define bit_FFT_STATUS_FFT_DONE     (1 << 0)
#define sft_FFT_CONF_IFFT           (1)
#define sft_FFT_CONF_FFT_ENABLE     (3)

#define STATIC_CONST                static const __attribute__((section(".rodata")))

/* ------------------------------------------------------------ */ //var

#ifdef CONFIG_FFT_SOFT
STATIC_CONST short AEC_FFT_SIN_TAB[65] =
{
    0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,
    11039,11793,12539,13279,14010,14732,15446,16151,16846,17530,18204,
    18868,19519,20159,20787,21403,22005,22594,23170,23731,24279,24811,
    25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,
    29956,30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,
    32412,32521,32609,32678,32728,32757,32767
};
#endif

/* ------------------------------------------------------------ */ //func
extern int32_t os_printf(const char *fmt, ...);


/** @brief find position of the first set bit 
 * @return 1~32
 * @note Effect: pos[31:0] <- data bit[0] ? 1 : bit[1] ? 2 ... bit[31] ? 32 : 0
*/
static inline int find_first_set_bit(uint32_t data)
{
    int pos;
#if 0//    defined(__CORE_BA2__)
    __asm( "b.ff1 %0, %1;"
                :"=r"(pos)
                :"r"(data)
                :"r26"
                );
#else
    for(pos = 1; pos <= 32; pos++) { if(data & 1) { break; } else { data >>= 1; }}
    if(pos > 32) pos = 0;
#endif
    return pos;
}


static inline int16_t __ssat16(int32_t data)
{
#if     defined(__CORE_BA2__)
    int32_t result;
    __asm("b.lim %0,%1,%2;" : "=r" (result) : "r" (data), "r" (32767) : ); 
    return (int16_t)result;
#else
    return (int16_t)((data > 32767) ? 32767 : ((data < -32767) ? -32767 : 0));
#endif
}

static inline float x_sqrtf(float data)
{
#if 0//    defined(__CORE_BA2__)
    uint32_t data_i = (uint32_t)data;
    uint32_t result;
    __asm("b.sqr %0,%1;" : "=r" (result) : "r" (data_i)); 
    return (float)result;
#else
    return sqrtf(data);
#endif
}








#if     defined(__CORE_BA2__)
/** @brief 
* @param real 
* @param imag 
* @param len <= 256
* @param ifft_flag 0:fft, 1:ifft
* @note 256pts FFT process about 75.2us @150MHz
*/
static inline void fft_ba2x(int16_t* real, int16_t* imag, int len, uint8_t ifft_flag)
{
    FFT_BA2x_RUN_START(1);
    int32_t i, bit_ext;

    BK3000_PMU_PERI_PWDS &= ~bit_PMU_FFT_PWD;
    BK3000_FFT_FIR_CONF   = 0;
    BK3000_FFT_FFT_CONF   = (1 << sft_FFT_CONF_FFT_ENABLE) | ((!!ifft_flag) << sft_FFT_CONF_IFFT);

    for(i = 0; i < len; i++)
    {
        uint32_t temp = (uint16_t)real[i] << 16 | (uint16_t)imag[i];
        BK3000_FFT_DATA_PORT = temp;//(uint32_t)real[i] << 16 | (uint32_t)imag[i];
    }

    BK3000_FFT_FFT_START = 1;

    while(!(BK3000_FFT_FFT_STATUS & bit_FFT_STATUS_FFT_DONE));

    bit_ext = (BK3000_FFT_FFT_STATUS & 0x00001fff) >> 7;
    bit_ext = (bit_ext & 0x20) ? 64 - bit_ext : 0;

    for(i = 0; i < len; i++)
    {
        int32_t t = BK3000_FFT_DATA_PORT;
        real[i] = __ssat16((t >> 16) << bit_ext);
        imag[i] = __ssat16(((t << 16) >> 16) <<  bit_ext);
    }

    BK3000_FFT_FFT_CONF   = 0;
    BK3000_PMU_PERI_PWDS |= bit_PMU_FFT_PWD;
    FFT_BA2x_RUN_START(0);
}
#endif

#ifdef CONFIG_FFT_SOFT
static inline void fft_soft(int16_t* real, int16_t* imag, int len, uint8_t ifft_flag)
{
    short int mr = 0, nn, i, j, k, l, istep, n;
    short qr, qi, tr, tr2, ti, ti2;
    short wr, wi;
    short tind, tsign;
    short b;
    short absshort, absshift;
    long c, c1, absc;
    int m;

    FFT_SOFT_RUN_START(1);

    n = len;
    nn = n - 1;
    m = find_first_set_bit(len) - 1;

    /* decimation in time - re-order data */
    for (m = 1; m <= nn; ++m)
    {
        l = n;
        do
        {
            l >>= 1;
        } while (mr + l > nn);

        mr = (mr & (l - 1)) + l;
        if (mr <= m)
            continue;

        tr = real[m];
        real[m] = real[mr];
        real[mr] = tr;

        ti = imag[m];
        imag[m] = imag[mr];
        imag[mr] = ti;
    }

    l = 1;
    k = 7;

    while (l < n)
    {
        // Variables for multiplication code
        istep = l << 1;
        for (m = 0; m < l; ++m)
        {
            j = m << k;

            tind = j + 64;
            if (tind >= 128)
            {
                tsign = -1;
                tind = 256 - tind;
            }
            else
                tsign = 1;

            if (tind >= 64) tind = 128 - tind;

            wr = AEC_FFT_SIN_TAB[tind] * tsign;

            tind = j;
            if (tind >= 128)
            {
                tsign = -1;
                tind = 256 - tind;
            }
            else
                tsign = 1;

            if (tind >= 64) tind = 128 - tind;

            wi = (ifft_flag) ? -AEC_FFT_SIN_TAB[tind] : AEC_FFT_SIN_TAB[tind];

            absshort =  (wr > 0) ? wr : -wr;

            absshift = (absshort >> 1) + (absshort & 1);

            wr =  (wr > 0) ? absshift : -absshift;

            // wr >>= 1;

            absshort =  (wi > 0) ? wi : -wi;

            absshift = (absshort >> 1) + (absshort & 1);

            wi =  (wi > 0) ? absshift : -absshift;

            // wi >>= 1;

            for (i = m; i < n; i += istep)
            {
                j = i + l;
                // tr = imagX_MPY(wr,real[j]) - imagX_MPY(wi,imag[j]);
                c = wr * real[j];
                absc = (c > 0) ? c : (0 - c);
                c1 = absc >> 14;
                b = c1 & 1;
                tr = (c1 >> 1) + b;
                if(c < 0) tr = (0 - tr);

                c = wi * imag[j];
                absc = (c > 0) ? c : (0 - c);
                c1 = absc >> 14;
                b = c1 & 1;
                tr2 = (c1 >> 1) + b;
                if (c < 0) tr2 = 0 - tr2;
                // tr = tr - tr2;
                c = tr - tr2;
                tr = __ssat16(c);

                // ti = imagX_MPY(wr,imag[j]) + imagX_MPY(wi,real[j]);
                c = wr * imag[j];
                absc = (c > 0) ? c : (0 - c);
                c1 = absc >> 14;
                b = c1 & 1;
                ti = (c1 >> 1) + b;
                if (c < 0) ti = 0 - ti;

                c = wi * real[j];
                absc = (c > 0) ? c : (0 - c);
                c1 = absc >> 14;
                b = c1 & 1;
                ti2 = (c1 >> 1) + b;
                if (c < 0) ti2 = 0 - ti2;
                // ti = ti + ti2;
                c = ti + ti2;
                ti = __ssat16(c);

                qr = real[i];
                qi = imag[i];

                absshort = (qr > 0) ? qr : (0 - qr);
                absshift = (absshort >> 1) + (absshort & 1);
                qr = (qr > 0) ? absshift : (- absshift);

                absshort = (qi > 0) ? qi : (0 - qi);
                absshift = (absshort >> 1) + (absshort & 1);
                qi = (qi > 0) ? absshift : (- absshift);

                // qr >>= 1;
                // qi >>= 1;

                if (ifft_flag)
                {
                    real[j] = __ssat16((qr - tr) * 2);
                    imag[j] = __ssat16((qi - ti) * 2);
                    real[i] = __ssat16((qr + tr) * 2);
                    imag[i] = __ssat16((qi + ti) * 2);
                }
                else
                {
                    real[j] = __ssat16((qr - tr));
                    imag[j] = __ssat16((qi - ti));
                    real[i] = __ssat16((qr + tr));
                    imag[i] = __ssat16((qi + ti));
                }
            }
        }

        --k;
        l = istep;
    }
    FFT_SOFT_RUN_START(0);
}
#endif

/// @brief 
/// @param real 
/// @param imag 
/// @param len must be power of 2
/// @param ifft_flag  0 : fft, 1: ifft
void ba22_fft(int16_t* real, int16_t* imag, int len, uint8_t ifft_flag)
{
#if     defined(__CORE_BA2__)
    fft_ba2x(real, imag, len, ifft_flag);
#elif   defined(CONFIG_FFT_SOFT)
    fft_soft(real, imag, len, ifft_flag);
#endif
}

/// @brief get signal amplitude of idx frequency point
/// @param real 
/// @param imag 
/// @param idx 0 ~ (fft_len-1), freq = (1 * 48K / FFT_FRAME_SAMPLES) * idx;
/// @return signal amplitude
int16_t ba22_cmplx_mag_q15_single(int16_t *real, int16_t *imag, int idx)
{
    int xr = real[idx];
    int yi = imag[idx];
    //use C std lib <math.h>, the sqrtf((xr*xr) + (yi*yi)) run about 2.5us @BA22@150M
    MATH_SQRT_RUN_START(1);
    int16_t mag = (int16_t)x_sqrtf((xr*xr) + (yi*yi));
    MATH_SQRT_RUN_START(0);
    // FFT_PRINTF("%5d,", mag);
    return mag;
}

/// @brief get signal amplitude array in each frequency point
/// @param mag[out] signal amplitude array
/// @param real[in] 
/// @param imag[in] 
/// @param len 
void ba22_cmplx_mag_q15(int16_t *real, int16_t *imag, int16_t* mag, int len)
{
    int i;
    for (i = 0; i < len; i++) real[i] = ba22_cmplx_mag_q15_single(real, imag, i);
}

#ifdef FFT_RUN_TEST
#if 1
#define DBG_SIN_LEN     48
// Audio sin data 1000Hz@(48000Hz,16bit, -3.0dBFS), sample_num: 48 point@(1 period)
STATIC_CONST int16_t fft_test_pcm16_sin[DBG_SIN_LEN] = {
0x0000, 0x0BD3, 0x1774, 0x22AD, 0x2D4E, 0x372A, 0x4013, 0x47E4, 0x4E7A, 0x53B8, 0x5787, 0x59D7,
0x5A9D, 0x59D7, 0x5787, 0x53B8, 0x4E7A, 0x47E4, 0x4013, 0x372A, 0x2D4E, 0x22AD, 0x1774, 0x0BD3,
0x0000, 0xF42C, 0xE88B, 0xDD52, 0xD2B1, 0xC8D5, 0xBFEC, 0xB81B, 0xB185, 0xAC47, 0xA878, 0xA628,
0xA562, 0xA628, 0xA878, 0xAC47, 0xB185, 0xB81B, 0xBFEC, 0xC8D5, 0xD2B1, 0xDD52, 0xE88B, 0xF42C};
#endif


void fft_test_get_pcm_data(int16_t *pcm, int samples)
{
    int i, k = 0;
    for(i = 0; i < samples; i++)
    {
    #if 0
        real[i] = fft_test_pcm16_sin[k] >> 7;//shift right for avoid voerflow [ba22 fft digital hardware bug]
        if(++k >= DBG_SIN_LEN) k = 0;
    #else //down resample
        pcm[i] = fft_test_pcm16_sin[k] >> 7;//shift right for avoid voerflow [ba22 fft digital hardware bug]
        // k += 2;//48K -> 24K
        k += 3;//48K -> 16K
        if(k >= DBG_SIN_LEN) k = 0;
    #endif
    }
}

void fft_test(void)
{
    #define FFT_FRAME_LEN       256// 128/256, don't over 256

    static uint8_t ba22_flag = 0;
	int i;
	int fft_len = FFT_FRAME_LEN;

    int16_t real[FFT_FRAME_LEN];
    int16_t imag[FFT_FRAME_LEN];

    int m = find_first_set_bit(fft_len) - 1;
	FFT_PRINTF(" ---- fft_len:%d, m:%d\n", fft_len, m);
// gen pcm data
    fft_test_get_pcm_data(real, fft_len);
    memset(imag, 0, sizeof(imag));
//fft process
	if(ba22_flag)   { fft_ba2x(real, imag, fft_len, 0); FFT_PRINTF("test ba22\n"); }
    else            { fft_soft(real, imag, fft_len, 0); FFT_PRINTF("test soft\n"); }
    ba22_flag = !ba22_flag;
//print out
	FFT_PRINTF(" ---- real, imag\n");
	for (i = 0; i < fft_len; i++)
	{
        if(i % 8 == 0) FFT_PRINTF("\n");
		//FFT_PRINTF("after: %d\t%d\t%d\r\n", i, real[i],imag[i]);
        int xr = real[i] < 0 ? -real[i] : real[i];
        int yi = imag[i] < 0 ? -imag[i] : imag[i];
		FFT_PRINTF("[%-5d, %-5d], ", xr, yi);
	}
    FFT_PRINTF("\n");
    FFT_PRINTF(" ----- sqrt((xr*xr) + (yi*yi))\n");
#if 0
    for (i = 0; i < fft_len; i++)
    {
        int xr = real[i];
        int yi = imag[i];
        //use C std lib <math.h>, the sqrtf((xr*xr) + (yi*yi)) run about 2.34us @BA22@150M
        MATH_SQRT_RUN_START(1);
        float mag = sqrtf((xr*xr) + (yi*yi));
        MATH_SQRT_RUN_START(0);
    	FFT_PRINTF("%7.1f,", mag);
    }
#else
    int16_t *mag = (int16_t *)real;
    ba22_cmplx_mag_q15(real, imag, real, fft_len);
    for (i = 0; i < fft_len; i++) { FFT_PRINTF("%5d,", mag[i]); }
#endif
	FFT_PRINTF("\n\n\n\n");
}
#endif

