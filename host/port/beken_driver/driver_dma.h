/**
 **************************************************************************************
 * @file    drv_dma.h
 * @brief   Driver API for DMA
 *
 * @author  Aixing.Li
 * @version V1.0.1
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_DMA_H__
#define __DRV_DMA_H__

#include <stdint.h>

typedef enum{
    DMA_ALL_FINISH_INT  = 1,
    DMA_HALF_FINISH_INT = 2,
    DMA_INT_BOTH        = 3,
}DMA_INTERRUPT_TYPE;

/**
 * @brief DMA request definition
 */
typedef enum
{
    DMA_REQ_MEM = 0,
    DMA_REQ_SDIO_RX,
    DMA_REQ_PSRAM_VIDEO_READ,
    DMA_REQ_PSRAM_AUDIO_READ,
    DMA_REQ_UART0_RX,
    DMA_REQ_UART1_RX,
    DMA_REQ_UART2_RX,
    DMA_REQ_SPI0_RX,
    DMA_REQ_SPI1_RX,
    DMA_REQ_SPI2_RX,
    DMA_REQ_I2S0_RX1,
    DMA_REQ_I2S0_RX2,
    DMA_REQ_I2S0_RX3,
    DMA_REQ_I2S0_RX4,
    DMA_REQ_I2S1_RX,
    DMA_REQ_I2S2_RX,
    DMA_REQ_SPDIF_RX,
    DMA_REQ_AUDIO_ADC,
    DMA_REQ_AUDIO_ANCIN,
    DMA_REQ_LOGICAL_ANALYSIS,
    DMA_REQ_SDIO_TX           = 1  << 5,
    DMA_REQ_PSRAM_VIDEO_WRITE = 2  << 5,
    DMA_REQ_PSRAM_AUDIO_WRITE = 3  << 5,
    DMA_REQ_UART0_TX          = 4  << 5,
    DMA_REQ_UART1_TX          = 5  << 5,
    DMA_REQ_UART2_TX          = 6  << 5,
    DMA_REQ_SPI0_TX           = 7  << 5,
    DMA_REQ_SPI1_TX           = 8  << 5,
    DMA_REQ_SPI2_TX           = 9  << 5,
    DMA_REQ_I2S0_TX1          = 10 << 5,
    DMA_REQ_I2S0_TX2          = 11 << 5,
    DMA_REQ_I2S0_TX3          = 12 << 5,
    DMA_REQ_I2S0_TX4          = 13 << 5,
    DMA_REQ_I2S1_TX           = 14 << 5,
    DMA_REQ_I2S2_TX           = 15 << 5,
    DMA_REQ_ANCLPF_TX         = 16 << 5,
    DMA_REQ_AUDIO_DAC         = 17 << 5,
    DMA_REQ_AUDIO_ANCOUT      = 18 << 5,
}DMA_REQ;

/**
 * @brief DMA transfer mode definition
 */
typedef enum
{
    DMA_MODE_SINGLE = 0,
    DMA_MODE_REPEAT,
}DMA_MODE;

/**
 * @brief DMA address mode definition
 */
typedef enum
{
    DMA_ADDR_NO_CHANGE     = 0,
    DMA_ADDR_AUTO_INCREASE = 1,
}DMA_ADDR_MODE;

/**
 * @brief DMA data type (or width) definition
 */
typedef enum
{
    DMA_DATA_TYPE_CHAR = 0,
    DMA_DATA_TYPE_SHORT,
    DMA_DATA_TYPE_LONG,
}DMA_DATA_TYPE;

/**
 * @brief DMA control command definition
 */
typedef enum
{
    DMA_CTRL_CMD_NULL,
    DMA_CTRL_CMD_PRIOPRITY_SET,
    DMA_CTRL_CMD_FINISH_INT_EN,
    DMA_CTRL_CMD_HALF_FINISH_INT_EN,
    DMA_CTRL_CMD_FINISH_INT_FLAG_GET,
    DMA_CTRL_CMD_HALF_FINISH_INT_FLAG_GET,
    DMA_CTRL_CMD_SRC_RD_INTERVAL_SET,
    DMA_CTRL_CMD_DST_WR_INTERVAL_SET,
    DMA_CTRL_CMD_REMAIN_LENGTH_GET,
    DMA_CTRL_CMD_SRC_BUFF_FLUSH,
}DMA_CTRL_CMD;


typedef void (*dma_channel_int_cb)(void *dma,DMA_INTERRUPT_TYPE type,void *uarg);


/**
 * @brief DMA Init
 */
void dma_init(void);

/**
 * @brief DMA channel malloc
 * @return DMA channel handler
 */
void* dma_channel_malloc(void);

/**
 * @brief DMA channel free
 * @param dma DMA channel handler
 */
void  dma_channel_free(void* dma);

/**
 * @brief DMA channel configuration
 * @param dma  DMA channel handler
 * @param idx  DMA channel index
 * @param req  DMA request mode, @see DMA_REQ
 * @param mode DMA transfer mode, @see DMA_MODE
 * @param src_start_addr src start address
 * @param src_stop_addr  src stop  address
 * @param src_addr_mode  src address mode, @see DMA_ADDR_MODE
 * @param src_data_type  src data type, @see DMA_DATA_TYPE
 * @param dst_start_addr dst start address
 * @param dst_stop_addr  dst stop  address
 * @param dst_addr_mode  dst address mode, @see DMA_ADDR_MODE
 * @param dst_data_type  dst data type, @see DMA_DATA_TYPE
 * @param data_length    transfer data length
 */
void dma_channel_config(void*           dma,
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
                       );


/**
 * @brief DMA channel control
 * @param dma DMA channel handler
 * @param dma control command
 * @param dma control argument
 */
void dma_channel_ctrl(void* dma, uint32_t cmd, uint32_t arg);

/**
 * @brief DMA channel enable
 * @param dma DMA channel handler
 * @param enable 0:disable, 1:enable
 */
void dma_channel_enable(void* dma, uint32_t enable);

/**
 * @brief Set current source address
 * @param dma DMA channel handler
 * @param addr address
 */
void dma_channel_src_curr_address_set(void* dma, uint32_t addr);

/**
 * @brief Set current destination address
 * @param dma DMA channel handler
 * @param addr address
 */
void dma_channel_dst_curr_address_set(void* dma, uint32_t addr);

/**
 * @brief Get current source read address
 * @param dma DMA channel handler
 * @return current source read address
 */
uint32_t dma_channel_src_curr_pointer_get(void* dma);

/**
 * @brief Get current destination write address
 * @param dma DMA channel handler
 * @return current destination write address
 */
uint32_t dma_channel_dst_curr_pointer_get(void* dma);


/**
 * @brief Get dma channel enable state
 * @param dma DMA channel handler
 * @return DMA channel enable state
 */
uint32_t dma_channel_enable_get(void* dma);


/**
 * @brief dma channel open mark
 * @param mark which channel is used
 */
void dma_sync_dsp_open(uint32_t mark);

/**
 * @brief dma channel close mark
 * @param mark which channel is closed
 */
void dma_sync_dsp_close(uint32_t mark);


/**
 * @brief enable interrupt of spec DMA requester
 * @param dma DMA channel handler
 * @param int_type interrupt type
 * @return None
 */
void dma_channel_enable_interrupt(void* dma, DMA_INTERRUPT_TYPE int_type);

/**
 * @brief disable interrupt of spec DMA requester
 * @param dma DMA channel handler
 * @param int_type interrupt type
 * @return None
 */
void dma_channel_disable_interrupt(void* dma, DMA_INTERRUPT_TYPE int_type);


/**
 * @brief set DMA transfer finish callback
 * @param dma DMA channel handler
 * @param cb function called when finished
 * @param uarg user data point
 * @return None
 */
void dma_channel_set_int_callback(void* dma,dma_channel_int_cb cb,void *uarg);

/**
 * @brief dma interrupt
 */
void dma_hw_interrupt(void);

#endif//__DRV_DMA_H__

