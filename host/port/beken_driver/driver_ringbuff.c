#include "hw_memcpy_impl.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "driver_audio.h"

#define RWP_SAFE_INTERVAL   (4)
#define RB_DMA_TYPE_RD      1
#define RB_DMA_TYPE_WR      2

int16_t rb_get_buffer_fill_size(driver_ringbuff_t *rb)
{
     uint16_t rp, wp;
    if (rb->buffp == NULL)
    {
        //os_printf("Buffer_null_1\r\n");
        return -1;
    }

    if(rb->dma_mode == RB_DMA_TYPE_RD)
    {
        rp = rb->rptr = dma_channel_src_curr_pointer_get(rb->dma_handle) - (uint32_t)rb->buffp;
        wp = rb->wptr;
    }
    else if(rb->dma_mode == RB_DMA_TYPE_WR)
    {
        wp = rb->wptr = dma_channel_dst_curr_pointer_get(rb->dma_handle) - (uint32_t)rb->buffp;
        rp = rb->rptr;
    }
    else
    {
        rp = rb->rptr;
        wp = rb->wptr;
    }
    rb->buffer_fill = rp > wp ? rb->buffer_len + wp - rp : wp - rp;

    return rb->buffer_fill;
}

int16_t DRAM_CODE rb_get_buffer_free_size(driver_ringbuff_t *rb)
{
    return rb->buffer_len - rb_get_buffer_fill_size(rb) - RWP_SAFE_INTERVAL;
}

#if 0//(CONFIG_APP_MP3PLAYER == 1)
int16_t rb_get_buffdata_size(driver_ringbuff_t *rb) //add by zjw for mp3decode
{
    if (rb->buffp == NULL)
    {
        //os_printf("Buffer_null_2\r\n");
        return -1;
    }
    
    if( rb->wptr < rb->rptr )
        return (rb->buffer_len - (rb->rptr - rb->wptr));
    else
        return ((rb->wptr - rb->rptr));
}
#endif

void rb_init( driver_ringbuff_t *rb, uint8_t *pbuff, uint16_t len, uint16_t dma_mode, void* dma_handle)
{    
    memset( (uint8_t *)rb, 0, sizeof(driver_ringbuff_t));
    /* To be saturate that the ringbuff data are all '0' */
	memset(pbuff,0,len);
    rb->buffp = pbuff;
    rb->buffer_len = len;
    rb->buffer_fill = 0;

    rb->dma_mode   = dma_mode;
    rb->dma_handle = dma_handle;

    if(rb->dma_mode == RB_DMA_TYPE_RD)
    {
        dma_channel_src_curr_address_set(dma_handle, (uint32_t)pbuff);
    }
    else if(rb->dma_mode == RB_DMA_TYPE_WR)
    {
        dma_channel_dst_curr_address_set(dma_handle, (uint32_t)pbuff + len);
    }

    return;
}

void ring_buffer_clear(driver_ringbuff_t* rb)
{
    rb->wptr = 0;
    rb->rptr = 0;
    rb->buffer_fill = 0;

    if(rb->dma_handle)
    {
        if(rb->dma_mode == RB_DMA_TYPE_RD)
        {
            dma_channel_src_curr_address_set(rb->dma_handle, (uint32_t)&rb->buffp[rb->wptr]);
        }
        else if(rb->dma_mode == RB_DMA_TYPE_WR)
        {
            dma_channel_dst_curr_address_set(rb->dma_handle, (uint32_t)&rb->buffp[rb->wptr] + rb->buffer_len);
        }
    }
}

int DRAM_CODE rb_read_buffer(driver_ringbuff_t *rb, uint8_t *buff, uint16_t len)
{
    int ret = 0;
    int size1, size2;
    uint32_t interrupts_info, mask;
    uint16_t rp, wp;

    if(rb->buffp == NULL) return 0;

    if(rb->dma_mode == RB_DMA_TYPE_WR)
    {
        wp = rb->wptr = dma_channel_dst_curr_pointer_get(rb->dma_handle) - (uint32_t)rb->buffp;
        rp = rb->rptr;
    }
    else
    {
        wp = rb->wptr;
        rp = rb->rptr;
    }
    rb->buffer_fill = rp > wp ? rb->buffer_len + wp - rp : wp - rp;

    if(!rb->buffer_fill) return 0;

    if(rp >= wp)
    {
        size1 = rb->buffer_len - rp;
        size2 = wp;

        if( len < size1 )
        {
            memcpy( buff, &rb->buffp[rp], len );
            ret = len;
            rp += len;
        }
        else
        {
            memcpy( buff, &rb->buffp[rp], size1 );
            if( len - size1 < size2 )
            {
                memcpy( buff + size1, &rb->buffp[0], (len - size1) );//lianxue.liu
                ret = len;
                rp = len - size1;
            }
            else
            {
                memcpy( buff + size1, &rb->buffp[0], size2 ); //lianxue.liu
                ret = size1 + size2;
                rp = size2;
            }
        }
    }
    else
    {
        size1 = wp - rp;

        if( len < size1 )
        {
            memcpy( buff, &rb->buffp[rp], len );
            ret = len;
            rp += len;
        }
        else
        {
            memcpy( buff, &rb->buffp[rp], size1 );
            ret = size1;
            rp += size1;
        }
    }

    rb->rptr = rp;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    rb->buffer_fill -= ret;
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return ret;
}

int rb_write_buffer(driver_ringbuff_t* rb, uint8_t* buffer, uint16_t len)
{
    uint32_t remain_bytes;
    uint32_t write_bytes = len;
    uint32_t rp, wp;

    uint32_t interrupts_info, mask;

    //os_printf("write===%d %d %d %d---\n", rb->wptr, rb->rptr, write_bytes, rb->dma_mode);

    if(write_bytes == 0) return 0;

    if(rb->dma_mode == RB_DMA_TYPE_RD)
    {
        rp = rb->rptr = dma_channel_src_curr_pointer_get(rb->dma_handle) - (uint32_t)rb->buffp;
        wp = rb->wptr;
    }
    else
    {
        rp = rb->rptr;
        wp = rb->wptr;
    }

    if(wp >= rp)
    {
        remain_bytes = rb->buffer_len - wp + rp;
        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            remain_bytes = rb->buffer_len - wp;

            if(remain_bytes >= write_bytes)
            {
                memcpy(&rb->buffp[wp], buffer, write_bytes);
                wp += write_bytes;
            }
            else
            {
                memcpy(&rb->buffp[wp], buffer, remain_bytes);
                wp = write_bytes - remain_bytes;
                memcpy(&rb->buffp[0], &buffer[remain_bytes], wp);
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
            memcpy(&rb->buffp[wp], buffer, write_bytes);
            wp += write_bytes;
        }
        else
        {
            return 0;
        }
    }

    if(wp >= rb->buffer_len && rb->rptr)
    {
        wp = 0;
    }

    if(rb->dma_mode == RB_DMA_TYPE_WR)
    {
        dma_channel_src_curr_address_set(rb->dma_handle, (uint32_t)&rb->buffp[wp]);
    }

    rb->wptr = wp;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    rb->buffer_fill += write_bytes;
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return write_bytes;
}

int16_t DRAM_CODE rb_fill_buffer( driver_ringbuff_t *rb, uint8_t *buff, uint32_t size, int debug )
{
    // aud_dac_volume_control( buff, size);
    #if RWP_SAFE_INTERVAL
    return rb_write_buffer(rb, buff, size);
    #else
    uint16_t buff_space, cpy_size,differ;
    uint32_t interrupts_info, mask;

    if (rb->buffp == NULL)
    {
        //os_printf("Buffer_null_3\r\n");
        return -1;
    }

    buff_space = rb_get_buffer_free_size(rb);

    if((rb->dma_mode == RB_DMA_TYPE_RD) && (buff_space <= 4)) return -1;

    if( buff_space > size )
    {
        if(rb->dma_mode == RB_DMA_TYPE_RD)
        {
            /* For let src_ptr no equal cur_ptr */
            if((buff_space - size) > 3)
                cpy_size = size;
            else
                cpy_size = size - 4;
        }
        else
        {
            cpy_size = size;
        }
    }
    else
    {
        if(rb->dma_mode == RB_DMA_TYPE_RD)
        {
            //buff_space must be greater than 4 and be saturate that src_ptr no equal cur_ptr
            cpy_size = buff_space - 4;
        }
        else
        {
            cpy_size = buff_space;
        }
    }

    if( rb->wptr + cpy_size > rb->buffer_len )
    {
        differ = rb->buffer_len - rb->wptr;

        #if A2DP_ROLE_SOURCE_CODE
            if(buff)
            {
                hw_memcpy8( (uint8_t *)&rb->buffp[rb->wptr],
                    buff, differ);
                hw_memcpy8( (uint8_t *)&rb->buffp[0], buff+differ, cpy_size - differ);
            }
            else
            {
                hw_memset8( (uint8_t *)&rb->buffp[rb->wptr],
                    0, differ);
                hw_memset8( (uint8_t *)&rb->buffp[0], 0, cpy_size - differ);
            }

        #else
            hw_memcpy8( (uint8_t *)&rb->buffp[rb->wptr],
                buff, differ);
            hw_memcpy8( (uint8_t *)&rb->buffp[0], buff+differ, cpy_size - differ);
        #endif

        rb->wptr = cpy_size-differ;
    }
    else
    {
        #if A2DP_ROLE_SOURCE_CODE
            if(buff)
            {
                hw_memcpy8( (uint8_t *)&rb->buffp[rb->wptr],
                    buff, cpy_size);
            }
            else
            {
                hw_memset8( (uint8_t *)&rb->buffp[rb->wptr],
                    0, cpy_size);
            }

        #else
            hw_memcpy8( (uint8_t *)&rb->buffp[rb->wptr],
                buff, cpy_size);
        #endif

        rb->wptr += cpy_size;
    }

    if( rb->wptr >= rb->buffer_len)
        rb->wptr -= rb->buffer_len;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    rb->buffer_fill += cpy_size;

    if(rb->dma_mode == RB_DMA_TYPE_RD) dma_channel_src_curr_address_set(rb->dma_handle, (uint32_t)&rb->buffp[rb->wptr]);

    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    return cpy_size;
    #endif
}

#if (CONFIG_APP_MP3PLAYER == 1)
int rb_get_buffer_mp3(driver_ringbuff_t *rb, uint8_t *buff, uint16_t len )
{
     int ret = 0;
     int size1, size2;
     uint32_t interrupts_info, mask;
     if (rb->buffp == NULL)
     {
        //os_printf("Buffer does not exist!\r\n");
        return -1;
     }  
     if (!rb->buffer_fill)
        goto exit;
     if( rb->rptr >= rb->wptr )
     {
         size1 = rb->buffer_len - rb->rptr;
         size2 = rb->wptr;

         if( len < size1 )
         {
            memcpy( buff, &rb->buffp[rb->rptr], len );
            ret = len;
            rb->rptr += len;
         }
         else
         {
            memcpy( buff, &rb->buffp[rb->rptr], size1 );
            if( len - size1 < size2 )
            {
                memcpy( buff + size1, &rb->buffp[0], (len - size1) );//lianxue.liu
                ret = len;
                rb->rptr = len - size1;
            }
            else
            {
                memcpy( buff + size1, &rb->buffp[0], size2 ); //lianxue.liu
                ret = size1 + size2;
                rb->rptr = size2;
            }
         }
     }
     else
     {
        size1 = rb->wptr - rb->rptr;

        if( len < size1 )
        {
            memcpy( buff, &rb->buffp[rb->rptr], len );
            ret = len;
            rb->rptr += len;
        }
        else
        {
            memcpy( buff, &rb->buffp[rb->rptr], size1 );
            ret = size1;
            rb->rptr += size1;
         }
     }
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    rb->buffer_fill -= ret;
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

exit:
    return ret;
}
#endif

#ifdef CONFIG_APP_HALFDUPLEX
int DRAM_CODE rb_get_one_sample( driver_ringbuff_t *rb, uint16_t *out )
{
    uint32_t interrupts_info, mask;

    if (rb->buffp == NULL)
    {
        //os_printf("Buffer_null_5\r\n");
        return -1;
    }
    
    if (!rb->buffer_fill)
        return -1;

    *out = rb->buffp[rb->rptr] + (rb->buffp[rb->rptr + 1] << 8);

    rb->rptr += 2;
    
    if( rb->rptr >= rb->buffer_len )
        rb->rptr = 0;

    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    rb->buffer_fill -= 2;
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return 0;
}
#endif

#if 0//(CONFIG_APP_MP3PLAYER == 1)
int16_t  rb_fill_buffer_MP3( driver_ringbuff_t *rb, uint8_t *buff, uint16_t size, int debug )
{
    uint16_t /* buff_space,  */cpy_size,differ;
    uint32_t interrupts_info, mask;

    if (rb->buffp == NULL)
    {
        //os_printf("Buffer_null_6\r\n");
        return -1;
    }
    
    cpy_size = size;

   //os_printf("size: %d, buff_space: %d, cpy_size: %d\n", size, buff_space, cpy_size);

    if( rb->wptr + cpy_size > rb->buffer_len ) 
    {
        differ = rb->buffer_len - rb->wptr;
        memcpy( (uint8_t *)&rb->buffp[rb->wptr],
            buff, differ);
        memcpy( (uint8_t *)&rb->buffp[0], buff+differ, cpy_size - differ);
        rb->wptr = cpy_size-differ;
    }
    else
    {
        memcpy( (uint8_t *)&rb->buffp[rb->wptr],
            buff, cpy_size);
        rb->wptr += cpy_size;
    }

    if( rb->wptr >= rb->buffer_len)
        rb->wptr -= rb->buffer_len;

    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    rb->buffer_fill += cpy_size;
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return cpy_size;
}

int RAM_CODE rb_get_one_sample_mp3( driver_ringbuff_t *rb, uint16_t *out )
{
    if (rb->buffp == NULL)
    {
        //os_printf("Buffer_null_7\r\n");
        return -1;
    }
    
    if( rb->rptr == rb->wptr )
        return -1;

    *out = *((uint16_t *)&rb->buffp[rb->rptr]);

    rb->rptr += 2;

    return 0;
}
#endif

#if A2DP_ROLE_SOURCE_CODE
int32_t rbGetSampleNumber(driver_ringbuff_t *rb)
{
    if (rb->buffp == NULL)
    {
        return -1;
    }
    return (rb->buffer_fill);
}

#endif
