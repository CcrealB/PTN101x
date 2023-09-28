#ifndef __APP_ASYNC_DATA_STREAM_H__
#define __APP_ASYNC_DATA_STREAM_H__


#include <stdint.h>
#include "config.h"
#include "config_debug.h"
#include <stdbool.h>

int async_debug(const char *fmt, ...);
#define DEBUG_ASYNC_DATA_PRINTF(fmt,...)    async_debug("[ASYNC]   "fmt, ##__VA_ARGS__)//async_debug("[APP ASYNC DATA]"fmt, ##__VA_ARGS__)


#define APP_ASYNC_SDCARD_ENABLE  0
#define ASYNC_SDCARD             1
#define ASYNC_SPI_FLASH          0

#define SD_SPLIT_SIZE               65536

#define FAST_READ_OFFSET        4

typedef enum
{
#if (ASYNC_SPI_FLASH == 1)
    ASYNC_DRV_SPI_FLASH,
#endif

#if (ASYNC_SDCARD == 1)
    ASYNC_DRV_SDCARD,
#endif
    ASYNC_DRV_NUM,
}async_drv;

typedef enum
{
    ASYNC_READ,
    ASYNC_WRITE,
}async_opt;

typedef enum
{
    ASYNC_READ_FINISH,
    ASYNC_WRITE_FINISH,
    ASYNC_CANCEL,
}async_opt_result;

typedef enum{
    async_err_none = 0,
    async_err_param = -1,
    async_err_not_init = -2,
    async_err_busy = -3,
    async_err_not_support = -4,
    async_err_alloc_failed = -5,
    async_err_drv = -5,
}async_errcode;

typedef enum 
{
    APP_ASYNC_IDLE = 0,
    APP_ASYNC_INIT,
    APP_ASYNC_OK,
}APP_ASYNC_STATE;


typedef struct
{
    union{
        char *filename;
        uint32_t addr;
    }param;
    uint8_t  *buffer;
    uint32_t size;
    uint32_t split_size;
}async_buffer_t;


typedef void (*async_delegation_cb)(async_opt_result result,uint8_t *buffer,uint32_t size);

/**
 * @brief Init Async Data Stream
 */
void app_async_data_stream_init(void);

/**
 * @brief Async Read/Write Data Stream
 * @param drv    : select driver
 * @param opt    : select Write / Read
 * @param buffer : Data Param
 * @param delegation_cb : Transfer Finish Callback
 */
async_errcode app_async_option(async_drv drv,
                                     async_opt opt,
                                     async_buffer_t *buffer,
                                     async_delegation_cb delegation_cb);

/**
 * @brief Async Read/Write Cancel
 * @param drv    : select driver
 */
async_errcode app_async_data_stream_cancel(async_drv drv);

/**
 * @brief Async Read/Write Handler
 */
void app_async_data_stream_handler(void);

/**
 * @brief Get Async Read/Write state
 */
bool app_async_data_stream_busy(void);


#endif

