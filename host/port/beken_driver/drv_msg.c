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

#include <string.h>
#include "drv_msg.h"

void msg_pool_init(MsgPoolContext* mpc, uint8_t* addr, uint32_t msg_size, uint32_t msg_count)
{
	mpc->addr  = addr;
    mpc->size  = msg_size;
    mpc->count = msg_count;
    mpc->rp	   = 0;
    mpc->wp	   = 0;
}

int32_t msg_pool_inqueue(MsgPoolContext* mpc, void* msg)
{
    uint32_t rp = mpc->rp;
    uint32_t wp = mpc->wp;
	uint32_t msgs = wp >= rp ? mpc->count - wp + rp : rp - wp;

    msgs = msgs ? msgs - 1 : 0;

	if(msgs)
	{
		memcpy(mpc->addr + mpc->size * wp, msg, mpc->size);

		if(++wp >= mpc->count)
		{
			wp = 0;
		}
        mpc->wp = wp;

		return 1;
	}
	else
	{
		return 0;
	}
}

int32_t msg_pool_dequeue(MsgPoolContext* mpc, void* msg)
{
    uint32_t rp = mpc->rp;
    uint32_t wp = mpc->wp;

	if(wp != rp)
	{
		memcpy(msg, mpc->addr + mpc->size * rp, mpc->size);

		if(++rp >= mpc->count)
		{
			rp = 0;
		}
        mpc->rp = rp;

		return 1;
	}
	else
	{
		return 0;
	}
}

uint32_t msg_pool_get_fill_msgs(MsgPoolContext* mpc)
{
    uint32_t rp = mpc->rp;
    uint32_t wp = mpc->wp;
	return wp >= rp ? wp - rp : mpc->count - rp + wp;
}

uint32_t msg_pool_get_free_msgs(MsgPoolContext* mpc)
{
    uint32_t rp = mpc->rp;
    uint32_t wp = mpc->wp;
	uint32_t msgs = wp >= rp ? mpc->count - wp + rp : rp - wp;

    return msgs > 0 ? msgs - 1 : 0;
}
