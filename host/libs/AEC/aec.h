#ifndef __AEC_H__
#define __AEC_H__
#include <stdint.h>

#define  AEC_CONFIG             (1)

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


#include <string.h>
#include "aec_fft.h"

#ifdef _MSC_VER
#define STATIC_CONST            static const
#define        CONST                   const
#else
#define STATIC_CONST            static const __attribute__((section(".rodata")))
#define        CONST                   const __attribute__((section(".rodata")))
#endif

#define AEC_EC_ENABLE           (1)
#define AEC_NS_ENABLE           (1)
#define AEC_HPF_ENABLE          (1)
#define AEC_DRC_ENABLE          (1)
#define AEC_CNI_ENABLE          (1)
#define AEC_EQ_ENABLE           (0)

#define AEC_NSP_ENABLE          (1)

#define AEC_EC_FLAG_SFT         (0)
#define AEC_NS_FLAG_SFT         (1)
#define AEC_HPF_FLAG_SFT        (2)
#define AEC_DRC_FLAG_SFT        (3)
#define AEC_CNI_FLAG_SFT        (4)
#define AEC_EQ_FLAG_SFT         (5)

#define AEC_EC_FLAG_MSK         (1 << AEC_EC_FLAG_SFT)
#define AEC_NS_FLAG_MSK         (1 << AEC_NS_FLAG_SFT)
#define AEC_HPF_FLAG_MSK        (1 << AEC_HPF_FLAG_SFT)
#define AEC_DRC_FLAG_MSK        (1 << AEC_DRC_FLAG_SFT)
#define AEC_CNI_FLAG_MSK        (1 << AEC_CNI_FLAG_SFT)
#define AEC_EQ_FLAG_MSK         (1 << AEC_EQ_FLAG_SFT)

#define AEC_MAX_MIC_DELAY       (2480)

#define MaxBand                 (8)
#define MovDc                   (1)

#define MAX_32                  0x7fffffffL
#define MIN_32                  0x80000000L
#define MAX_16                  0x7fff
#define MIN_16                  0x8000

enum AEC_CTRL_CMD
{
    AEC_CTRL_CMD_NULL = 0,
    AEC_CTRL_CMD_GET_MIC_DELAY,
    AEC_CTRL_CMD_SET_MIC_DELAY,
    AEC_CTRL_CMD_GET_EC_DEPTH,
    AEC_CTRL_CMD_SET_EC_DEPTH,
    AEC_CTRL_CMD_GET_FLAGS,
    AEC_CTRL_CMD_SET_FLAGS,
    AEC_CTRL_CMD_SET_PARAMS,
    AEC_CTRL_CMD_GET_RX_BUF,
    AEC_CTRL_CMD_GET_TX_BUF,
    AEC_CTRL_CMD_GET_OUT_BUF,
};

typedef struct
{   int32_t alpha;
    int32_t sm  [3];
    uint16_t G  [FFT_LEN_NB_HF*BD+1];
    int32_t Sc  [FFT_LEN_NB_HF*BD+1];
    int32_t Ss  [FFT_LEN_NB_HF*BD+1];
    int32_t pSNR[FFT_LEN_NB_HF*BD+1];
}NSContext;

typedef struct _AECContext
{
    uint8_t  flags;
    uint8_t  test;
    uint8_t  vol;
    uint8_t  vol_mem;
    int8_t   ec_depth;
    int8_t   ns_depth;
    int8_t   drc_mode;
    int8_t   insert;

    int16_t  fs;
    int16_t  frame_samples;
    int16_t  mic_delay;
    int16_t  delay_offset;
    int16_t  pw_init;
    int16_t  adapt_mu;
    int16_t  is_ec;
    int16_t  cutoff;
    int16_t  cutbin;
    int16_t  sbnum;
    int16_t  freq_scale;
    int16_t  rin_delay_rp;
    int16_t  rin_delay_wp;
    int16_t  TxRxThr;
    int16_t  spcnt;
    uint16_t minG;

    int32_t  max_cnt;
    int32_t  ec_thr;
    int32_t  dc;
    int32_t  cni_floor;
    int32_t  cni_fade;
    int32_t  s_max;
    int32_t  m_max0;
    int32_t  m_max1;
    int32_t  ns_mean;
    int32_t  Fmax;
    int32_t  drc_gain;
    uint32_t frame_cnt;

    int16_t *sin;
    int16_t *rin;
    int16_t *out;

    FFT_INFO FFT_ptr;

    int16_t SubBand[MaxBand];
    #if MEM_ISOLATE              // for  16k  512fft
    int32_t *Rbuf; //512*4=2048  count=512   byte=2048
    int32_t *Sbuf; //512*4=2048  count=1024  byte=4096
    int32_t *Ramp; //257*4=1028  count=1281  byte=5124
    int32_t *Samp; //257*4=1028  count=1538  byte=6152
    int32_t *tmp;  //516*4=2064  count=2054  byte=8216
    int32_t *ang;  //516*4=2064  count=2570  byte=10280
    #else
    int32_t Rbuf[FFT_LEN_NB*BD];
    int32_t Sbuf[FFT_LEN_NB*BD];
    int32_t Ramp[FFT_LEN_NB_HF*BD+1];
    int32_t Samp[FFT_LEN_NB_HF*BD+1];
    int32_t tmp [(FFT_LEN_NB_HF+1)*2*BD];
    int32_t ang [(FFT_LEN_NB_HF+1)*2*BD];
    #endif
    int32_t syn [(48*BD)<<SymWin];
    #if AEC_EC_ENABLE
    int16_t rin_delay[AEC_MAX_MIC_DELAY];
    int16_t pw_fft_mem[FFT_LEN_NB_HF*BD+1];
    int32_t ec_fft_mem[FFT_LEN_NB_HF*BD+1];
    int32_t TxRxMem[MaxBand];
    int8_t  update[MaxBand];
    int8_t  TxRxSm[4];
    #endif

    #if AEC_NS_ENABLE
    int32_t   nspara[8];
    NSContext VADInfo;
    NSContext NsInfo;
    #endif

    #if AEC_EQ_ENABLE
    uint16_t *EQF;
    #endif

    int32_t  div_tab[17];
}AECContext;

uint32_t  aec_ver     (void);
uint32_t  aec_size    (uint32_t delay);
void      aec_init    (AECContext* aec, int16_t fs);
void      aec_mode    (AECContext* aec, int16_t fs);
void      aec_ctrl    (AECContext* aec, uint32_t cmd, uint32_t arg);
void      aec_proc    (AECContext* aec, int16_t* rin, int16_t* sin, int16_t* out);
void      aec_close   (AECContext* aec);

#ifdef _MSC_VER

#define UMULWB(x,w)      ((int32_t)(((int64_t)x*(w&0xffff))>>16))
#define UMULWT(x,w)      ((int32_t)(((int64_t)x*((w&0xffff0000)>>16))>>16))
#define SMULLS(x,w,s)    ((int32_t)(((int64_t)x*w)>>s))
#define SMULLAS(x,w,s)   ((int32_t)(((int64_t)x*w)>>s))
#define SMULL(x,w)       ((int32_t)(((int64_t)x*w)>>32))
#define SMULWB(x,w)      ( (x>>16) * ((w<<16)>>16) + ( ( (x&0xffff) * ((w<<16)>>16))>>16 ) )
#define SMULWT(x,w)      ( (x>>16) * (w>>16      ) + ( ( (x&0xffff) * ( w>>16     ))>>16 ) )
#define SMULBB(x,w)      ((x<<16)>>16)*((w<<16)>>16)
//#define SMULBT(x,w)    ((x<<16)>>16)*(w>>16)
//#define SMULTB(x,w)    (x>>16)*((w<<16)>>16)
//#define SMULTT(x,w)    (x>>16)*(w>>16)

#else

static __inline int32_t CLZ(int32_t a)
{
    int32_t result;
    __asm
    (
        "b.clz %0,%1;"
        : "=r" (result)
        : "r" (a)
    );
    return result;
}

static __inline int32_t SMULWB(int32_t a, int32_t b)
{
    int32_t result;
    __asm
    (
        "b.smulwb %0,%1,%2;"
        : "=r" (result)
        : "r" (a), "r" (b)
    );
    return result;
}

static __inline int32_t SMULWT(int32_t a, int32_t b)
{
    int32_t result;
    __asm
    (
        "b.smulwt %0,%1,%2;"
        : "=r" (result)
        : "r" (a), "r" (b)
    );
    return result;
}

static __inline int32_t UMULWB(int32_t a, uint32_t b)
{
    int32_t result;
    __asm
    (
        "b.umulwb %0,%1,%2;"
        : "=r" (result)
        : "r" (a), "r" (b)
    );
    return result;
}

static __inline int32_t UMULWT(int32_t a, uint32_t b)
{
    int32_t result;
    __asm
    (
        "b.umulwt %0,%1,%2;"
        : "=r" (result)
        : "r" (a), "r" (b)
    );
    return result;
}

static __inline int32_t SMULBB(int32_t a, int32_t b)
{
    int32_t result;
    __asm
    (
        "b.smulbb %0,%1,%2;"
        : "=r" (result)
        : "r" (a), "r" (b)
    );
    return result;
}

static __inline int32_t SMULL(int32_t a, int32_t b)
{
    int32_t result;
    __asm
    (
        "b.mulhsu %0,%1,%2;"
        : "=r" (result)
        : "r" (a), "r" (b)
    );
    return result;
}


static __inline int32_t SMULLAS(int32_t a, int32_t b, uint32_t bits)
{
    int32_t result;
    __asm
    (
        "b.mulas %0,%1,%2,%3;"
        : "=r" (result)
        : "r" (a), "r" (b), "i" (bits)
        :
    );
    return result;
}

#define SMULLS(x,w,s) ((int32_t)(((int64_t)x*w)>>s))

#endif

#define F2IS(f,s)  ((int32_t)((f)*(1<<(s))))
#define F2WS(f,s)  ((int16_t)((f)*(1<<(s))))
#define F2ISR(f,s) ((int32_t)((f>0)?((f)*(1<<(s))+0.5):((f)*(1<<(s))-0.5)))
#define F2WSR(f,s) ((int16_t)((f>0)?((f)*(1<<(s))+0.5):((f)*(1<<(s))-0.5)))
#define F2WUR(f,s) ((uint16_t)((f)*(1<<(s))+0.5))
#define Square(x)  ((x)*(x))

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AEC_H__
