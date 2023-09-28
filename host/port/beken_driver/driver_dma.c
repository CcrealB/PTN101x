/**
 **************************************************************************************
 * @file    drv_dma.c
 * @brief   Driver API for DMA
 *
 * @author  Jiankun.Liao
 * @version V1.0.1
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */
#include<string.h>
#include <stddef.h>
#include "bkreg.h"
#include "driver_dma.h"
#include "drv_system.h"
#include "drv_mailbox.h"
#include "config.h"

#define DMA_CHANNEL_NUM     (16)
#define dmac                ((volatile DMAContext*)MDU_GENER_DMA_BASE_ADDR)
#define BIT(n)              (1<<(n))
typedef void (*CALLBACK_DMA)();

typedef struct _DMAConfig
{
	union{
		uint32_t cntl;
		struct{
			volatile uint32_t enable             : 1;
			volatile uint32_t finish_int_en      : 1;
			volatile uint32_t half_finish_int_en : 1;
			volatile uint32_t dma_mode           : 1; //0: SINGLE, 1:REPEAT
			volatile uint32_t src_data_width     : 2; //0: 8 bits, 1: 16 bits, 2: 32 bits
			volatile uint32_t dst_data_width     : 2; //0: 8 bits, 1: 16 bits, 2: 32 bits
			volatile uint32_t src_addr_mode      : 1; //0: no change, 1: increase
			volatile uint32_t dst_addr_mode      : 1; //0: no change, 1: increase
			volatile uint32_t src_addr_loop      : 1; //0: no loop, 1: loop
			volatile uint32_t dst_addr_loop      : 1; //0: no loop, 1: loop
			volatile uint32_t prioprity          : 3; //bigger & higher
			volatile uint32_t                    : 1; //bigger & higher
			volatile uint32_t trans_len          :16; //actually len = trans + 1
			volatile uint32_t                    : 0;
		};
	};

    volatile uint32_t dst_start_addr;
    volatile uint32_t src_start_addr;
    volatile uint32_t dst_loop_stop_addr;
    volatile uint32_t dst_loop_start_addr;
    volatile uint32_t src_loop_stop_addr;
    volatile uint32_t src_loop_start_addr;
	union{
		uint32_t reg_0x07;
		struct{
			volatile uint32_t dma_request        :10; //@see DMA_REQ
			volatile uint32_t                    : 2;
			volatile uint32_t src_rd_interval    : 4; //source read operate interval, unit in cycle.
			volatile uint32_t dst_wr_interval    : 4; //destination write operate interval, unit in cycle.
			volatile uint32_t                    : 0;
		};
	};
}DMAConfig;

typedef struct _DMAStatus
{
    volatile uint32_t remain_length         :17;
    volatile uint32_t flush_src_buff        : 1;
    volatile uint32_t reserved              : 6;
    volatile uint32_t finish_int_cnt        : 4;
    volatile uint32_t half_finish_int_cnt   : 4;
}DMAStatus;

typedef struct _DMAContext
{
    volatile DMAConfig config[DMA_CHANNEL_NUM];

    volatile uint32_t  src_address[DMA_CHANNEL_NUM];
    volatile uint32_t  dst_address[DMA_CHANNEL_NUM];
    volatile uint32_t  src_pointer[DMA_CHANNEL_NUM];
    volatile uint32_t  dst_pointer[DMA_CHANNEL_NUM];

    volatile DMAStatus status[DMA_CHANNEL_NUM];

    volatile uint32_t  prio_mode            : 1;//0:round-robin, 1:fixed prioprity
    volatile uint32_t                       :31;

    volatile uint32_t  finish_int_flag;
}DMAContext;

static volatile uint16_t dma_channels_used = 0;

typedef struct{
    dma_channel_int_cb dma_trans_cb;
    void               *uarg;
}dma_int_callback_t;

static dma_int_callback_t dma_int_callbacks[DMA_CHANNEL_NUM];


void dma_init(void)
{
    memset(dma_int_callbacks,0,sizeof(dma_int_callbacks));
}

void* dma_channel_malloc(void)
{
#ifdef DMA_ALLOC_BY_DSP
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_MCU_DMA_MALLOC | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    void* dma = (void*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    // DBG_LOG_INFO("dma%d:0x%08X\n", (((uint32_t)dma - 0x01010000) / 0x20), (uint32_t)dma);
    return dma;
#else
    uint32_t i;

    for(i = 0; i < DMA_CHANNEL_NUM; i++)
    {
        if(!(dma_channels_used & (1 << i)))
        {
            dma_channels_used |= (1 << i);
            break;
        }
    }

    return i < DMA_CHANNEL_NUM ? (void*)&dmac->config[i] : NULL;
#endif
}


void dma_channel_free(void* dma)
{
#ifdef DMA_ALLOC_BY_DSP
    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_MCU_DMA_MALLOC_FREE | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)dma, 0, 0, NULL);
#else
    uint32_t i = ((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);

    if(i < DMA_CHANNEL_NUM && (uint32_t)&dmac->config[i] == (uint32_t)dma)
    {
        dma_channels_used &= (~(1 << i)) & ((1 << DMA_CHANNEL_NUM) - 1);
        dma = NULL;
    }
#endif
}

void dma_channel_config(void*           _dma,
                        DMA_REQ         req,
                        DMA_MODE        mode,
                        uint32_t        src_start_addr,
                        uint32_t        src_stop_addr,
                        DMA_ADDR_MODE   src_addr_mode,
                        DMA_DATA_TYPE   src_data_type,
                        uint32_t        dst_start_addr,
                        uint32_t        dst_stop_addr,
                        DMA_ADDR_MODE   dst_addr_mode,
                        DMA_DATA_TYPE   dst_data_type,
                        uint32_t        data_length
                       )
{
    if(_dma)
    {
        uint32_t   idx = ((uint32_t)_dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);
        DMAConfig* dma = (DMAConfig*)_dma;

        dma->trans_len           = data_length - 1;
        dma->enable              = 0;
        dma->finish_int_en       = 0;
        dma->half_finish_int_en  = 0;
        dma->dma_request         = req;
        dma->dma_mode            = mode;
        dma->src_start_addr      = src_start_addr;
        dma->src_loop_start_addr = src_start_addr & ~3;//4byte aligned
        dma->src_loop_stop_addr  = (src_stop_addr & 3) ? ((src_stop_addr & ~3) + 4) : src_stop_addr;//4byte aligned
        dma->src_addr_loop       = src_addr_mode;
        dma->src_addr_mode       = src_addr_mode;
        dma->src_data_width      = src_data_type;
        dma->dst_start_addr      = dst_start_addr;
        dma->dst_loop_start_addr = dst_start_addr & ~3;//4byte aligned
        dma->dst_loop_stop_addr  = (dst_stop_addr & 3) ? ((dst_stop_addr & ~3) + 4) : dst_stop_addr;//4byte aligned
        dma->dst_addr_loop       = dst_addr_mode;
        dma->dst_addr_mode       = dst_addr_mode;
        dma->dst_data_width      = dst_data_type;

        dmac->src_address[idx] = src_addr_mode == DMA_ADDR_NO_CHANGE ? 0 : src_start_addr;
        dmac->dst_address[idx] = dst_addr_mode == DMA_ADDR_NO_CHANGE ? 0 : dst_start_addr;
    }
}


void dma_channel_ctrl(void* _dma, uint32_t cmd, uint32_t arg)
{
    if(_dma)
    {
        uint32_t   idx = ((uint32_t)_dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);
        DMAConfig* dma = (DMAConfig*)_dma;

        switch(cmd)
        {
        case DMA_CTRL_CMD_PRIOPRITY_SET:
            dma->prioprity = arg;
            break;
        case DMA_CTRL_CMD_FINISH_INT_EN:
            dma->finish_int_en = !!arg;
            break;
        case DMA_CTRL_CMD_HALF_FINISH_INT_EN:
            dma->half_finish_int_en = !!arg;
            break;
        case DMA_CTRL_CMD_FINISH_INT_FLAG_GET:
            *(uint32_t*)arg = (dmac->finish_int_flag >> idx) & 0x1;
            break;
        case DMA_CTRL_CMD_HALF_FINISH_INT_FLAG_GET:
            *(uint32_t*)arg = (dmac->finish_int_flag >> (16 + idx)) & 0x1;
            break;
        case DMA_CTRL_CMD_SRC_RD_INTERVAL_SET:
            dma->src_rd_interval = arg;
            break;
        case DMA_CTRL_CMD_DST_WR_INTERVAL_SET:
            dma->dst_wr_interval = arg;
            break;
        case DMA_CTRL_CMD_REMAIN_LENGTH_GET:
            *(uint32_t*)arg = dmac->status[idx].remain_length;
            break;
        case DMA_CTRL_CMD_SRC_BUFF_FLUSH:
            dmac->status[idx].flush_src_buff = 1;
            break;
        default:
            break;
        }
    }
}

uint32_t dma_get_remain(void* dma)
{
    uint32_t remain_length;
    dma_channel_ctrl(dma, DMA_CTRL_CMD_REMAIN_LENGTH_GET, (uint32_t)&remain_length);
    return remain_length; 
}

void dma_channel_enable(void* dma, uint32_t enable)
{
    ((DMAConfig*)dma)->enable = !!enable;
}

uint32_t dma_channel_enable_get(void* dma)
{
    return ((DMAConfig*)dma)->enable;
}

void dma_channel_src_curr_address_set(void* dma, uint32_t addr)
{
    dmac->src_address[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)] = addr;
}

void dma_channel_dst_curr_address_set(void* dma, uint32_t addr)
{
    dmac->dst_address[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)] = addr;
}

uint32_t dma_channel_src_curr_pointer_get(void* dma)
{
    return dmac->src_pointer[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)];
}

uint32_t dma_channel_dst_curr_pointer_get(void* dma)
{
    return dmac->dst_pointer[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)];
}

void dma_sync_dsp_open(uint32_t mark)
{
    dma_channels_used |= mark;
}

void dma_sync_dsp_close(uint32_t mark)
{
    dma_channels_used &= ~mark;
}


void dma_channel_enable_interrupt(void* dma, DMA_INTERRUPT_TYPE int_type)
{
    if(dma == NULL)
    {
        return;
    }
    DMAConfig* _dma = (DMAConfig*)dma;
    if(int_type & DMA_ALL_FINISH_INT)
    {
        _dma->finish_int_en = 1;
    }
    if(int_type & DMA_HALF_FINISH_INT)
    {
        _dma->half_finish_int_en = 1;
    }
    if(int_type & DMA_INT_BOTH)
    {
        system_peri_mcu_irq_enable(SYS_PERI_IRQ_GENER_DMA);
    }
}


void dma_channel_disable_interrupt(void* dma, DMA_INTERRUPT_TYPE int_type)
{
    if(dma == NULL)
    {
        return;
    }
    DMAConfig* _dma = (DMAConfig*)dma;
    if(int_type & DMA_ALL_FINISH_INT)
    {
        _dma->finish_int_en = 0;
    }
    if(int_type & DMA_HALF_FINISH_INT)
    {
        _dma->half_finish_int_en = 0;
    }
}

void dma_channel_set_int_callback(void* dma,dma_channel_int_cb cb,void *uarg)
{
    uint32_t index = ((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);
    if(index < DMA_CHANNEL_NUM)
    {
        dma_int_callbacks[index].dma_trans_cb = cb;
        dma_int_callbacks[index].uarg = uarg;
    }
}


void dma_hw_interrupt(void){
    int i;
    uint32_t sr=REG_GENER_DMA_0xD1;
    REG_GENER_DMA_0xD1=sr;
    for(i=0;i<DMA_CHANNEL_NUM;i++)
    {
        if(sr & 1)
        {
            if(dma_int_callbacks[i].dma_trans_cb)
            {
                dma_int_callbacks[i].dma_trans_cb((void*)&dmac->config[i],DMA_ALL_FINISH_INT,dma_int_callbacks[i].uarg);
            }
        }
        sr = sr >> 1;
    }
    for(i=0;i<DMA_CHANNEL_NUM;i++)
    {
        if(sr & 1)
        {
            if(dma_int_callbacks[i].dma_trans_cb)
            {
                dma_int_callbacks[i].dma_trans_cb((void*)&dmac->config[i],DMA_HALF_FINISH_INT,dma_int_callbacks[i].uarg);
            }
        }
        sr = sr >> 1;
    }
}


