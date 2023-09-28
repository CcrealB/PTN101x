/**
 **************************************************************************************
 * @file    driver_spi.h
 * @brief   Driver SPI
 *
 * @author  Jiankun.Liao
 * @version V1.0.1
 *
    SPI_0_FUN_1
        SPI0_1F_CSN_PIN    ->PIN2
        SPI0_1F_SCK_PIN    ->PIN3
        SPI0_1F_MOSI_PIN   ->PIN4
        SPI0_1F_MISO_PIN   ->PIN5
 
     SPI_1_FUN_1
         SPI1_1F_CSN_PIN    ->PIN22
         SPI1_1F_SCK_PIN    ->PIN23
         SPI1_1F_MOSI_PIN   ->PIN24
         SPI1_1F_MISO_PIN   ->PIN25
         
     SPI_2_FUN_1
         SPI2_1F_CSN_PIN    ->PIN36
         SPI2_1F_SCK_PIN    ->PIN37
         SPI2_1F_MOSI_PIN   ->PIN38
         SPI2_1F_MISO_PIN   ->PIN39
        
     SPI_2_FUN_4
         SPI2_4F_CSN_PIN    ->PIN31
         SPI2_4F_SCK_PIN    ->PIN30
         SPI2_4F_MOSI_PIN   ->PIN33
         SPI2_4F_MISO_PIN   ->PIN32


    const spi_param_t default_spi_master_param = {
        .mode = SPI_Master,
        .bit_wdth = SPI_BIT_8,
        .wire = SPI_WIRE_4,
        .lsb_first = SPI_MSB_FIRST_SEND,
        .clk_polarity = SPI_POL_LOW,
        .clk_pha = SPI_PHA_FIRST,
        .baud_rate = SPI_DEFAULT_BAUD,
    };

     Init SPI : 
        static spi_handle spi0_hdl;
        spi_param_t param;
        memcpy(&param,&default_spi_master_param,sizeof(param));
        spi0_hdl = spi_init(SPI_0_FUN_1,(spi_param_t*)&param);

     SPI Block Write :
        uint8_t tx[]= {1,2,3,4,5};
        spi_transfer_object trans = {
            .trans_mode = SPI_TRANS_BLOCKING | SPI_TRANS_USE_DMA_FLAG,
            .tx_buffer = tx,
            .tx_len = sizeof(tx),
            .rx_buffer = NULL,
            .rx_len = 0,
        };
        spi_transfer(spi0_hdl,&trans,NULL,NULL);
        
     SPI NonBlock Write :
        uint8_t tx[]= {1,2,3,4,5};
        spi_transfer_object trans = {
            .trans_mode = SPI_TRANS_CALLBACK | SPI_TRANS_USE_DMA_FLAG,
            .tx_buffer = tx,
            .tx_len = sizeof(tx),
            .rx_buffer = NULL,
            .rx_len = 0,
        };
        spi_transfer(spi0_hdl,&trans,NULL,spi_trans_cb);
        
        static void spi_trans_cb(spi_handle handle,spi_transfer_object *trans_obj,void *arg)
        {
            //
        }

     SPI Block Write & Read
        uint8_t tx[]= {1,2,3,4,5};
        uint8_t rx[5]= {};
        spi_transfer_object trans = {
            .trans_mode = SPI_TRANS_BLOCKING | SPI_TRANS_USE_DMA_FLAG,
            .tx_buffer = tx,
            .tx_len = sizeof(tx),
            .rx_buffer = rx,
            .rx_len = sizeof(rx),
        };
        spi_transfer(spi0_hdl,&trans,NULL,NULL);
 
 
     SPI NonBlock Write & Read
        uint8_t tx[]= {1,2,3,4,5};
        uint8_t rx[5]= {};
        spi_transfer_object trans = {
            .trans_mode = SPI_TRANS_CALLBACK | SPI_TRANS_USE_DMA_FLAG,
            .tx_buffer = tx,
            .tx_len = sizeof(tx),
            .rx_buffer = rx,
            .rx_len = sizeof(rx),
        };
        spi_transfer(spi0_hdl,&trans,NULL,spi_trans_cb);
        
        static void spi_trans_cb(spi_handle handle,spi_transfer_object *trans_obj,void *arg)
        {
            //
        }
        
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __DRV_SPI_H_
#define __DRV_SPI_H_

#include <stdint.h>
#ifndef CEVAX2
#include "config.h"
#include "config_debug.h"
#else
#include "bk_printf.h"
#include "app_debug.h"
#endif


// #define DEBUG_SPI_PRINTF(fmt,...)    //do{ g_dsp_log_flag = 2; com_printf("[SPI]"fmt, ##__VA_ARGS__); g_dsp_log_flag = 1;}while(0)
#define DEBUG_SPI_PRINTF(fmt,...)       //do{ os_printf("[SPI]"fmt, ##__VA_ARGS__); }while(0)

// #define SPI_SIMPLE_DRV  //borg, bug not fixed in mcu side

#ifdef CONFIG_USER_SPI_FUNC
    #define SPI_0_FUN1_ENABLE   (CONFIG_USER_SPI_FUNC & 1)
    #define SPI_1_FUN1_ENABLE   (CONFIG_USER_SPI_FUNC & 2)
    #define SPI_2_FUN1_ENABLE   (CONFIG_USER_SPI_FUNC & 4)
    #define SPI_2_FUN4_ENABLE   (CONFIG_USER_SPI_FUNC & 8)
#else
    #define SPI_0_FUN1_ENABLE   0
    #define SPI_1_FUN1_ENABLE   0
    #define SPI_2_FUN1_ENABLE   0
    #define SPI_2_FUN4_ENABLE   0
#endif

#if (SPI_2_FUN1_ENABLE == 1) && (SPI_2_FUN4_ENABLE == 1)
#error "SPI_2_FUN1_ENABLE and SPI_2_FUN4_ENABLE can't enable at the same time"
#endif


typedef enum{
#if (SPI_0_FUN1_ENABLE == 1)
    SPI_0_FUN_1,
#endif
#if (SPI_1_FUN1_ENABLE == 1)
    SPI_1_FUN_1,
#endif
#if (SPI_2_FUN1_ENABLE == 1)
    SPI_2_FUN_1,
#endif
#if (SPI_2_FUN4_ENABLE == 1)
    SPI_2_FUN_4,
#endif
    SPI_NUM,
}SPI_Name;

typedef enum{
    SPI_Slave,
    SPI_Master,
    SPI_UNDEFINED,
}SPI_Mode;

typedef enum{
    SPI_TRANS_UNDEFINED = 0x0,
    SPI_TRANS_BLOCKING  = 0x1,
    SPI_TRANS_CALLBACK  = 0x2,
    SPI_TRANS_USE_DMA_FLAG = 0x8,
}SPI_Trans_Mode;

typedef enum{
    SPI_BIT_8,
    SPI_BIT_16
}SPI_Bit_Wdth;

typedef enum{
    SPI_FIFO_1_BYTE,
    SPI_FIFO_16_BYTE,
    SPI_FIFO_32_BYTE,
    SPI_FIFO_48_BYTE,
}SPI_Fifo_int;

typedef enum{
    SPI_WIRE_4,
    SPI_WIRE_3,
}SPI_Wire;

typedef enum{
    SPI_MSB_FIRST_SEND,
    SPI_LSB_FIRST_SEND,
}SPI_Lsb_first;

typedef enum{
    SPI_POL_LOW,
    SPI_POL_HIGH,
}SPI_Clk_Polarity;
    
typedef enum{
    SPI_PHA_FIRST,
    SPI_PHA_SECOND,
}SPI_Clk_Pha;


#define SPI_BAUD_100KHZ                 100000  //26/2/130
#define SPI_BAUD_500KHZ                 500000  //26/2/26
#define SPI_BAUD_1MHZ                   1000000 //26/2/13
#define SPI_BAUD_2M6HZ                  2600000 //26/2/5
#define SPI_BAUD_3M25HZ                 3250000 //26/2/4
#define SPI_BAUD_4M33HZ                 4333333 //26/2/3
#define SPI_BAUD_6M5HZ                  6500000 //26/2/2
#define SPI_BAUD_13MHZ                  13000000//26/2/1
#define SPI_BAUD_26MHZ                  26000000
#define SPI_DEFAULT_BAUD                SPI_BAUD_6M5HZ//26000000


typedef struct{
    volatile uint32_t                is_open:1;
    volatile uint32_t                spi_name:4;
    volatile uint32_t                spi_mode:2;
    volatile uint32_t                is_transfer:2;
    volatile uint32_t                io_cs_en   :1;
    volatile uint32_t                io_clk_en  :1;
    volatile uint32_t                io_miso_en :1;
    volatile uint32_t                io_mosi_en :1;
    volatile uint32_t                reserved   :19;
}spi_object,*spi_handle;

typedef struct{
    SPI_Trans_Mode          trans_mode;
    int                     tx_dma_addr_fixed;//0:dma addr inc, 1:dma addr fixed in tx_buffer with 4byte width, and trans 4byte repeatly
    void *                  tx_buffer;
    void *                  rx_buffer;
    uint32_t                tx_len;
    uint32_t                rx_len;
    uint32_t                tx_offset;
    uint32_t                rx_offset;
}spi_transfer_object;


typedef void (*spi_trans_cb_fxn)(spi_handle handle,spi_transfer_object *trans_obj,void *arg);
typedef struct{
    SPI_Mode                mode;
    SPI_Bit_Wdth            bit_wdth;
    SPI_Wire                wire;
    SPI_Lsb_first           lsb_first;
    SPI_Clk_Polarity        clk_polarity;
    SPI_Clk_Pha             clk_pha;
    uint32_t                baud_rate;
    uint8_t                 io_cs_en;
    uint8_t                 io_clk_en;
    uint8_t                 io_miso_en;
    uint8_t                 io_mosi_en;
}spi_param_t;

typedef enum{
    spi_drv_err_none = 0,
    spi_drv_err_param = -1,
    spi_drv_err_not_init = -2,
    spi_drv_err_busy = -3,
    spi_drv_err_overflow = -4,
    spi_drv_err_dma_alloc_failed = -5,
}spi_dirver_err_code;




/**
 * @brief Init SPI Driver
 * @param spi_name : select hw spi
 * @param param    : config spi param
 * @return spi handle
 */
spi_handle spi_init(SPI_Name spi_name,spi_param_t *param);

/**
 * @brief Uninit SPI Driver
 * @param handle : spi handle
 * @return spi_dirver_err_code
 */
spi_dirver_err_code spi_uninit(spi_handle handle);

/**
 * @brief SPI Transfer Include Write / Read
 * @param handle    : spi handle
 * @param trans_obj : trannsfer object
                      if (trans_obj->tx_buffer != NULL) and (trans_obj->rx_buffer == NULL) -> only spi write
                      if (trans_obj->tx_buffer != NULL) and (trans_obj->rx_buffer != NULL) -> only spi write and read
                      if (trans_obj->tx_buffer == NULL) and (trans_obj->rx_buffer != NULL) -> only spi read
 * @param arg       : user arg
 * @param cb        : trans finish cb fxn
 * @return spi_dirver_err_code
 */
spi_dirver_err_code spi_transfer(spi_handle handle,spi_transfer_object *trans_obj,void *arg,spi_trans_cb_fxn cb);

/**
 * @brief SPI Hardware Interrupt
 * @param spi_name : SPI_Name
 */
void spi_hw_interrupt(SPI_Name spi_name);

#ifdef SPI_SIMPLE_DRV
void hal_spi_init(uint8_t SPIx, SPI_Mode master_en, SPI_Bit_Wdth spi_16bit_en, SPI_Wire wire3_en);
void hal_spi_read_write(uint8_t SPIx, uint8_t* tx_buf, int tx_sz, uint8_t* rx_buf, int rx_sz);
#endif

#endif //__DRV_SPI_H_
