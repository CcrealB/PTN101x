/**
 * **************************************************************************************
 * @file    spdif_comm.h
 * 
 * @author  Borg Xiao
 * @date    20230330
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * **************************************************************************************
 * */
#ifndef _SPDIF_COMM_H_
#define _SPDIF_COMM_H_

/* ---------------------------------------------------------------------- *///debug

#define SPDIF_LOG_MSK_AS            0x1
#define SPDIF_LOG_MSK_AS_STD        0x2
#define SPDIF_LOG_MSK_DBG           0x80
#define SPDIF_PRINTF                os_printf
#define SPDIF_LOG_E(fmt,...)        SPDIF_PRINTF("[SPD|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define SPDIF_LOG_W(fmt,...)        SPDIF_PRINTF("[SPD|W:%d]"fmt, __LINE__, ##__VA_ARGS__)
#define SPDIF_LOG_I(fmt,...)        SPDIF_PRINTF("[SPD|I]"fmt, ##__VA_ARGS__)
#define SPDIF_LOG_D(fmt,...)        do{ if(spdif_log_flag_get(SPDIF_LOG_MSK_DBG)) SPDIF_PRINTF("[SPD|D]"fmt, ##__VA_ARGS__); }while(0)
#define SPDIF_LOG_AS(fmt,...)       do{ if(spdif_log_flag_get(SPDIF_LOG_MSK_AS)) SPDIF_PRINTF("[SPD|AS]"fmt, ##__VA_ARGS__); }while(0)
#define SPDIF_LOG_AS_STD(fmt,...)   do{ if(spdif_log_flag_get(SPDIF_LOG_MSK_AS_STD)) SPDIF_PRINTF(fmt, ##__VA_ARGS__); }while(0)


// #define SPDIF_IRQ_NEST_DBG //irq nest
// #define SPDIF_AS_DEBUG
#ifdef SPDIF_AS_DEBUG
    #define SPDIF_AS_PROC_RUN(en)       REG_GPIO_0x20 = en ? 2 : 0;
    #define SPDIF_SRC_CODE_RUN(en)      REG_GPIO_0x1F = en ? 2 : 0;
#else
    #define SPDIF_AS_PROC_RUN(en)
    #define SPDIF_SRC_CODE_RUN(en)
#endif

/* ---------------------------------------------------------------------- *///macro

//please define in "u_config.h"
// #define SPDIF_GPIO              GPIO12//GPIO11, GPIO12, GPIO14

//defined in config.c
// #define CONFIG_SPDIF_DRV

#define SPDIF_RINGBUF_MALLOC_EN     1

#define SPDIF_FS_OUT                48000
#define SPDIF_FRAME_TIMms           4
#define SPDIF_FRAME_SMPS            192//4ms
#define AUDIO_SPDIF_RB_SIZE         (SPDIF_FRAME_SMPS * 4 * 2 * 2)//32bit2ch*2frame + 4byte interval, 32bit for 16/20/24bit compatible

#define FS_DET_DEBOUNCEms           (500U)//(unit:1ms) same sample rate detect for consecutive configed time to lock
#define CONFIG_SPDIF_SRC_AT_MCU     0//define/undefine if mcu src unused
// #define CONFIG_AUD_SYNC_SPDIF       1 // 0:disable/1:spc by mcu/2:spc by dsp //don't uncomment for defined @ config.h

#if (SPDIF_TRANS_MODE == SPDIF_TRANS_DMA)
    #define SPDIF_FIFO_THRD_DEF     8//dma move 8(4 smps/ch) sample at each action
#elif (SPDIF_TRANS_MODE == SPDIF_TRANS_INTR)
    #define SPDIF_FIFO_THRD_DEF     96
#else
    #define SPDIF_FIFO_THRD_DEF     128//1~128 @ only write x & y buffer, [exp:96 -> 1ms@48KHz2ch]
#endif

#define SPDIF_ASI_BIT_WIDTH         24 //16/20/24bit width, audio reg read out fixd to 24bit, don't modify
#define SPDIF_ASO_BIT_WIDTH         def_aso_bit_width//16/24bit, 【Note:16bit not complete yet】
#define SPDIF_ASO_BUFF_SIZE         def_aso_rb_size
#define spdif_Byte2Smp(size)        ((size) >> (2 + ((SPDIF_ASI_BIT_WIDTH) != 16)))/*16/32bit2ch*/
#define spdif_Smp2Byte(smps)        ((smps) << (2 + ((SPDIF_ASI_BIT_WIDTH) != 16)))
#define spdif_aso_Byte2Smp(size)    ((size) >> (2 + ((SPDIF_ASO_BIT_WIDTH) != 16)))
#define spdif_aso_Smp2Byte(smps)    ((smps) << (2 + ((SPDIF_ASO_BIT_WIDTH) != 16)))

#ifndef UNUSED
    #define UNUSED(v)    (void)v
#endif


extern void spdif_log_flag_set(uint32_t flag, uint8_t en);
extern uint32_t spdif_log_flag_get(uint32_t flag);

#endif /* _SPDIF_COMM_H_ */