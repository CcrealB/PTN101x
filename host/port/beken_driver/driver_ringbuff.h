#ifndef _DRIVER_RINGBUFF_H_
#define _DRIVER_RINGBUFF_H_

typedef struct _driver_ringbuff_s
{
    uint8_t *  buffp;
    uint16_t buffer_len;
    uint16_t buffer_fill;
    uint16_t   wptr;
    uint16_t   rptr;

    uint32_t  dma_mode;
    void*   dma_handle;

}driver_ringbuff_t;

// /**
//  * @brief DMA type definition
//  */
// typedef enum
// {
//     RB_DMA_TYPE_NULL  = 0,
//     RB_DMA_TYPE_READ  = 1,
//     RB_DMA_TYPE_WRITE = 2,
// }RB_DMA_TYPE;


enum
{
    NONE_FILL = 0,
    AEC_RX_FILL = 1,
    AEC_TX_FILL = 2,
    ADC_FILL  = 3,
    CVSD_FILL  = 4,
    WAV_FILL = 5,
    SBC_FILL = 6,
    LINEIN_FILL = 7,
    DAC_FILL_WITH_COMP = 8
};

//API

int16_t rb_get_buffer_fill_size(driver_ringbuff_t *rb);
int16_t rb_get_buffer_free_size(driver_ringbuff_t *rb);
int16_t rb_fill_buffer( driver_ringbuff_t *rb, uint8_t *buff, uint32_t size, int debug );

void rb_init( driver_ringbuff_t *rb, uint8_t *pbuff, uint16_t len, uint16_t dma_mode, void* dma_handle);
void ring_buffer_clear(driver_ringbuff_t* rb);
int  rb_read_buffer(driver_ringbuff_t *rb, uint8_t *buff, uint16_t len );
int rb_write_buffer(driver_ringbuff_t* rb, uint8_t* buffer, uint16_t len);

int rb_get_one_sample( driver_ringbuff_t *rb, uint16_t *out );

int16_t rb_get_buffdata_size(driver_ringbuff_t *rb);//add by zjw for mp3decode
int rb_get_one_sample_mp3( driver_ringbuff_t *rb, uint16_t *out );
int rb_get_buffer_mp3(driver_ringbuff_t *rb, uint8_t *buff, uint16_t len );


int16_t  rb_fill_buffer_MP3( driver_ringbuff_t *rb, uint8_t *buff, uint16_t size, int debug );
#endif
