/**
 **************************************************************************************
 * @file    drv_ring_buffer.c
 * @brief   An implementation for ring buffer
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include "driver_dma.h"
#include "drv_ring_buffer.h"

#define RWP_SAFE_INTERVAL   (4)

// #define CVT_ADDR_BUS2CPU(bus_addr, addr_base)    ((uint32_t)(bus_addr) - (uint32_t)(addr_base))
// #define CVT_ADDR_CPU2BUS(cpu_addr, addr_base)    ((uint32_t)(cpu_addr) + (uint32_t)(addr_base))
// #define DMA_CUR_RD_ADDR_GET(rb)         (dma_channel_dst_curr_pointer_get(rb->dma) - CVT_ADDR_CPU2BUS(rb->address, rb->addr_base))
// #define DMA_CUR_WR_ADDR_GET(rb)         (dma_channel_dst_curr_pointer_get(rb->dma) - CVT_ADDR_CPU2BUS(rb->address, rb->addr_base))
// #define DMA_CUR_RD_ADDR_SET(rb, offset) dma_channel_src_curr_address_set((rb->dma), CVT_ADDR_CPU2BUS(rb->address, rb->addr_base) + (offset))
// #define DMA_CUR_WR_ADDR_SET(rb, offset) dma_channel_dst_curr_address_set((rb->dma), CVT_ADDR_CPU2BUS(rb->address, rb->addr_base) + (offset))


//dsp ram address, global:0x03000000~0x0307FFFF -> local:0x00000000~0x0007FFFF(512K)
// #define DSP_DAT0_START_ADDR         ((uint32_t)0x03000000)
// #define DSP_DATA_RAM_SIZE_MAX       ((uint32_t)0x00080000)

void ring_buffer_init(RingBufferContext* rb, uint8_t* addr, uint32_t capacity, void* dma, uint32_t dma_type)
{
    rb->address  = addr;
#if   defined(CEVAX2)
    rb->addr_base = ((uint32_t)(rb->address) < (uint32_t)0x00080000) ? ((uint32_t)(0x03000000)) : 0;
#else
    rb->addr_base = 0;
#endif
    rb->capacity = capacity;
    rb->wp       = 0;
    rb->rp       = 0;
    rb->dma      = dma;
    rb->dma_type = dma_type;

    if(dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            dma_channel_src_curr_address_set(dma, (uint32_t)rb->address + rb->addr_base);
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            dma_channel_dst_curr_address_set(dma, (uint32_t)rb->address + rb->addr_base + rb->capacity);
        }
    }
}

void ring_buffer_reset(RingBufferContext* rb)
{
    rb->wp = 0;
    rb->rp = 0;

    if(rb->dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            dma_channel_src_curr_address_set(rb->dma, (uint32_t)rb->address + rb->addr_base);
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            dma_channel_dst_curr_address_set(rb->dma, (uint32_t)rb->address + rb->addr_base + rb->capacity);
        }
    }
}

void ring_buffer_reset_rp(RingBufferContext* rb)
{
    if(rb->dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            rb->wp = dma_channel_dst_curr_pointer_get(rb->dma) - ((uint32_t)rb->address + rb->addr_base);
        }
    }
    rb->rp = rb->wp;
}

void ring_buffer_reset_wp(RingBufferContext* rb)
{
    if(rb->dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            rb->rp = dma_channel_src_curr_pointer_get(rb->dma) - ((uint32_t)rb->address + rb->addr_base);
        }
    }
    rb->wp = rb->rp;
}

uint32_t ring_buffer_read(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t required_bytes = size;
    uint32_t read_bytes;
    uint32_t remain_bytes;
    uint32_t rp, wp;

    if(rb->dma && rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        wp = rb->wp = dma_channel_dst_curr_pointer_get(rb->dma) - ((uint32_t)rb->address + rb->addr_base);
        rp = rb->rp;
    }
    else
    {
        wp = rb->wp;
        rp = rb->rp;
    }

    if(required_bytes == 0) return 0;

    if(wp >= rp)
    {
        remain_bytes = wp - rp;
        read_bytes = (required_bytes > remain_bytes) ? remain_bytes : required_bytes;
        memcpy(buffer, &rb->address[rp], read_bytes);
        rp += read_bytes;
    }
    else
    {
        remain_bytes = rb->capacity - rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
            memcpy(buffer, &rb->address[rp], read_bytes);

            if(required_bytes - read_bytes > wp)
            {
                memcpy(buffer + read_bytes, &rb->address[0], wp);
                rp = wp;
                read_bytes += wp;
            }
            else
            {
                memcpy(buffer + read_bytes, &rb->address[0], required_bytes - read_bytes);
                rp = required_bytes - read_bytes;
                read_bytes = required_bytes;
            }
        }
        else
        {
            read_bytes = required_bytes;
            memcpy(buffer, &rb->address[rp], read_bytes);
            rp += read_bytes;
        }
    }

    #if 0
    if(rb->dma && rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        dma_channel_dst_curr_address_set(rb->dma, (uint32_t)&rb->address[rp] + rb->addr_base);
    }
    #endif

    rb->rp = rp;

    return read_bytes;
}

uint32_t ring_buffer_write(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t remain_bytes;
    uint32_t write_bytes = size;
    uint32_t rp, wp;

    if(write_bytes == 0) return 0;

    if(rb->dma && rb->dma_type == RB_DMA_TYPE_READ)
    {
        rp = rb->rp = dma_channel_src_curr_pointer_get(rb->dma) - ((uint32_t)rb->address + rb->addr_base);
        wp = rb->wp;
    }
    else
    {
        rp = rb->rp;
        wp = rb->wp;
    }

    if(wp >= rp)
    {
        remain_bytes = rb->capacity - wp + rp;

        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            remain_bytes = rb->capacity - wp;

            if(remain_bytes >= write_bytes)
            {
                memcpy(&rb->address[wp], buffer, write_bytes);
                wp += write_bytes;
            }
            else
            {
                memcpy(&rb->address[wp], buffer, remain_bytes);
                wp = write_bytes - remain_bytes;
                memcpy(&rb->address[0], &buffer[remain_bytes], wp);
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        remain_bytes = rp - wp;

        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            memcpy(&rb->address[wp], buffer, write_bytes);
            wp += write_bytes;
        }
        else
        {
            return 0;
        }
    }

    if(wp >= rb->capacity && rp)
    {
        wp = 0;
    }

    if(rb->dma && rb->dma_type == RB_DMA_TYPE_READ)
    {
        dma_channel_src_curr_address_set(rb->dma, (uint32_t)&rb->address[wp] + rb->addr_base);
    }

    rb->wp = wp;

    return write_bytes;
}

uint32_t ring_buffer_get_fill_size(RingBufferContext* rb)
{
    uint32_t rp, wp;
    uint32_t fill_size;

    if(rb->dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            rp = rb->rp = dma_channel_src_curr_pointer_get(rb->dma) - ((uint32_t)rb->address + rb->addr_base);
            wp = rb->wp;
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            rp = rb->rp;
            wp = rb->wp = dma_channel_dst_curr_pointer_get(rb->dma) - ((uint32_t)rb->address + rb->addr_base);
        }
        else
        {
            rp = rb->rp;
            wp = rb->wp;
        }
    }
    else
    {
        rp = rb->rp;
        wp = rb->wp;
    }

    fill_size = wp >= rp ? wp - rp : rb->capacity - rp + wp;

    return fill_size;
}

uint32_t ring_buffer_get_free_size(RingBufferContext* rb)
{
    uint32_t free_size;

    free_size = rb->capacity - ring_buffer_get_fill_size(rb);

    return free_size > RWP_SAFE_INTERVAL ? free_size - RWP_SAFE_INTERVAL : 0;
}

void ring_buffer_dma_write_enable(RingBufferContext* rb, uint32_t enable)
{
    if(rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        dma_channel_dst_curr_address_set(rb->dma, ((uint32_t)rb->address + rb->addr_base) + rb->capacity * (!!enable));
    }
}
