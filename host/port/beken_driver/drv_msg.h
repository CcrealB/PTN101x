/**
 **************************************************************************************
 * @file    drv_msg.h
 * @brief   Driver for MSG pool
 * 
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2018 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_MSG_H__
#define __DRV_MSG_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "stdint.h"

typedef struct _MsgPoolContext
{
    uint8_t* addr;
    uint32_t size;
    uint32_t count;
    volatile uint32_t rp;
    volatile uint32_t wp;
}MsgPoolContext;

void msg_pool_init(MsgPoolContext* mpc, uint8_t* addr, uint32_t msg_size, uint32_t msg_count);

int32_t msg_pool_inqueue(MsgPoolContext* mpc, void* msg);
int32_t msg_pool_dequeue(MsgPoolContext* mpc, void* msg);

uint32_t msg_pool_get_fill_msgs(MsgPoolContext* mpc);
uint32_t msg_pool_get_free_msgs(MsgPoolContext* mpc);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DRV_MSG_H__
