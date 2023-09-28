/**
 * **************************************************************************************
 * @file    drv_spdif.h
 * 
 * @author  Borg Xiao
 * @date    modify @20230629
 * @date    20230217
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */

#ifndef _DRV_SPDIF_H_
#define _DRV_SPDIF_H_

#include <stdint.h>
#include "spdif_comm.h"

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/* -------------------- config -------------------- */
//check swi config in sys_irq.c avoid conflict
#define SPDIF_TRANS_DMA         1
#define SPDIF_TRANS_INTR        2
#define SPDIF_TRANS_MODE        (SPDIF_TRANS_DMA/* | SPDIF_TRANS_INTR*/) // 1:dma, 2:intr(NA), 3:dma & intr

/* -------------------- typedef -------------------- */
typedef enum _SPDIF_ERR_t{
    SPDIF_RET_FAIL  = -1,
    SPDIF_RET_OK    = 0,
}SPDIF_ERR_t;

typedef struct _SPDIF_CFG_CTX_t{
    GPIO_PIN    io;     //GPIO11 / GPIO12 / GPIO14
    uint8_t     cfg_idx;//0 default
    uint8_t     fifo_thrd;//condig fifo thrd (1~128)[exp:96 -> 48sample*2ch -> 1ms@48KHz]
}SPDIF_CFG_CTX_t;

/* -------------------- prototype -------------------- */

void spdif_init(SPDIF_CFG_CTX_t *p_spdif_st);
void spdif_uninit(SPDIF_CFG_CTX_t *p_spdif_st);
void spdif_enable(uint8_t enable);
void spdif_int_enable(uint8_t enable);
void spdif_dma_enable(void* dma, uint8_t enable);
void spdif_dma_init(RingBufferContext *rb_st, uint8_t *rb_buff, int rb_size, uint8_t loop_en);
void spdif_dma_uninit(void **p_dma);

int spdif_is_audio(void);
void spdif_audio_rcv_check(void);
int spdif_fs_det_fine(void);
int spdif_fs_det_course(void);
int spdif_sample_rate_get(void);
void spdif_audio_fmt_cvt(void *pcm_in, void *pcm_out, int samples, int bits_out);

int spdif_fifo_clear(void);
void spdif_isr(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//_DRV_SPDIF_H_
