
/**
 * **************************************************************************************
 * @file    dsp_include.h
 * 
 * @author  Borg Xiao
 * @date    20230423
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#ifndef _DSP_INCLUDE_H_
#define _DSP_INCLUDE_H_

#include "drv_mailbox.h"
#include "drv_ring_buffer.h"
#include "driver_gpio.h"
// #include "aud_include.h"

/* ---------------------------------------- */// debug
/* Define trace levels */
#define LOG_ON_NONE             0x00     /* No trace messages to be generated    */
#define LOG_ON_ERROR            0x01     /* Error condition trace messages       */
#define LOG_ON_WARNING          0x02     /* Warning condition trace messages     */
#define LOG_ON_INFO             0x04     /* Information traces                   */
#define LOG_ON_DEBUG            0x08     /* Debug messages for events            */
#define LOG_ON_ALL              0xFF

// #define MDSP_LOG_ON_BIT         (LOG_ON_ERROR | LOG_ON_WARNING | LOG_ON_INFO)
#define MDSP_LOG_ON_BIT         LOG_ON_ALL

#define MDSP_PRINTF             os_printf
#define MDSP_LOG_E(fmt, ...)    do{ if(MDSP_LOG_ON_BIT & LOG_ON_ERROR)   MDSP_PRINTF("[MDSP|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); }while(0)
#define MDSP_LOG_W(fmt, ...)    do{ if(MDSP_LOG_ON_BIT & LOG_ON_WARNING) MDSP_PRINTF("[MDSP|W:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); }while(0)
#define MDSP_LOG_I(fmt, ...)    do{ if(MDSP_LOG_ON_BIT & LOG_ON_INFO)    MDSP_PRINTF("[MDSP|I]"fmt, ##__VA_ARGS__); }while(0)
#define MDSP_LOG_D(fmt, ...)    do{ if(MDSP_LOG_ON_BIT & LOG_ON_DEBUG)   MDSP_PRINTF("[MDSP|D]"fmt, ##__VA_ARGS__); }while(0)


/* ---------------------------------------- */// must consistent with dsp side
#define SYS_FLAG_ALL_MASK           0xFFFFFFFF
#define SYS_FLAG_WAIT_MCU_TRIG      0x01
#define SYS_FLAG_MCU_TRIG           0x02
#define SYS_FLAG_SYSTEM_START       0x04

#endif /* _DSP_INCLUDE_H_ */
