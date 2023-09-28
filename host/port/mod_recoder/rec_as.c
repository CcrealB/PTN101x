
#include "config.h"

#ifdef CONFIG_APP_RECORDER

#include "os_common.h"
#include "drv_ring_buffer.h"
#include "rec_include.h"
#include "app_debug.h"

static RingBufferContext *p_as_rec_rb = NULL;



void app_rec_rb_init(void)
{
    REC_LOG_I("%s\n", __FUNCTION__);
    uint8_t *p_as_rec_rb_buf = NULL;
    if(p_as_rec_rb != NULL) 
    {
        REC_LOG_W("rb:%p\n", p_as_rec_rb);
        if(p_as_rec_rb->address != NULL) REC_LOG_W("rb_buf:%p\n", p_as_rec_rb->address);
    }
    p_as_rec_rb = rec_malloc(sizeof(RingBufferContext));
    p_as_rec_rb_buf = rec_malloc(REC_RB_BUF_SIZE);
    if(p_as_rec_rb && p_as_rec_rb_buf){
        ring_buffer_init(p_as_rec_rb, p_as_rec_rb_buf, REC_RB_BUF_SIZE, NULL, RB_DMA_TYPE_NULL);
    }else{
        REC_LOG_E("ringbuf malloc error:%p, %p\n", p_as_rec_rb, p_as_rec_rb_buf);
    }
    // dbg_show_dynamic_mem_info(1);
}

void app_rec_rb_uninit(void)
{
    REC_LOG_I("%s\n", __FUNCTION__);
    if(p_as_rec_rb->address) { rec_free(p_as_rec_rb->address); p_as_rec_rb->address = NULL; }
    if(p_as_rec_rb) { rec_free(p_as_rec_rb); p_as_rec_rb = NULL; }
}

void* get_rec_rb(void)
{
    return (void*)p_as_rec_rb;
}

//call at read rec data isr
void audio_rec_proc_stereo(int16_t *pcm, int samples)
{
    if(p_as_rec_rb == NULL) return;

    int size = samples << 2;
    if(ring_buffer_get_free_size(p_as_rec_rb) >= size)
    {
        ring_buffer_write(p_as_rec_rb, (uint8_t*)pcm, size);
    }
    else
    {
        extern volatile uint8_t rec_rb_overflow_cnt;
        rec_rb_overflow_cnt++;
    }
}


// ---------- for recorder read
// int rec_ringbuf_fill_get(void) { return aud_mic_get_fill_buffer_size(); }
int rec_ringbuf_fill_get(void) { return ring_buffer_get_fill_size(p_as_rec_rb); }
int rec_ringbuf_read(uint8_t* buff, int size)
{
    // return aud_mic_read_buffer(buff, size);
    REC_READ_START();
    int rd_sz = ring_buffer_read(p_as_rec_rb, buff, size);
    REC_READ_STOP();
    return rd_sz;
}

#endif //APP_REC_AS

