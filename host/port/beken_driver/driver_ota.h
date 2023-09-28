#ifndef _DRIVER_OTA_H_
#define _DRIVER_OTA_H_

#include "config.h"
#include <stdint.h>

#if (CONFIG_DRIVER_OTA == 1) 
#define LO_UINT16(a)             (((a) >> 0) & 0xFF)
#define HI_UINT16(a)             (((a) >> 8) & 0xFF)

#define LO_UINT32(a)             (((a) >> 0 ) & 0xFF)
#define DO_UINT32(a)             (((a) >> 8 ) & 0xFF)
#define UP_UINT32(a)             (((a) >> 16) & 0xFF)
#define HI_UINT32(a)             (((a) >> 24) & 0xFF)

#define OTA_VID                  0x1000
#define OTA_PID                  0x2000

#define OTA_BUFFER_SIZE          1000
#define OTA_PKT_ADDR_LEN         4
#define OTA_ADDR_OFFSET          0x20

/* ota flash operation addr */
#define OTA_DSP_ADDR_POINT       0x000004
#define OTA_MCU_ADDR             0x002024
#define OTA_INFO_ADDR            (FLASH_ENVDATA_DEF_ADDR_ABS + 0x1000)
#define OTA_INFO_BT_ADDR_ADDR    (OTA_INFO_ADDR + 32)
#define OTA_INFO_BT_NAME_ADDR    (OTA_INFO_BT_ADDR_ADDR + 6)
#define OTA_INFO_LEN             18
#define INFO_LEN                 34
#define INFO_VER_OFFSET          8

/* ota location mark */
#define OTA_MARK_INIT            0xFFFF
#define OTA_MARK_ONGO            0xFF55
#define OTA_MARK_FAIL            0x0000
#define OTA_MARK_SUCC            0x5555
#define OTA_MARK_USED            0x0055

/* ota init version */
#define OTA_INIT_VERSION         0x0000

/* image flag */
enum image_flag
{
    IMAGE_MCU        =   0x01,
    IMAGE_DSP        =   0x02,
    IMAGE_ENV        =   0x03,
    IMAGE_ENV_ADDR   =   0x04,
    IMAGE_MCU_ENV    =   0x05,
};

enum ota_result
{
    OTA_SUCCESS                = 0x00,
    OTA_FAIL                   = 0x01
};

typedef struct
{
    uint16_t vid;
    uint16_t pid;
    uint16_t ver;
    uint16_t flag;
    uint16_t crc;
    uint32_t len;
    uint32_t addr1;
    uint32_t addr2;
    uint8_t reserved[10];
}__attribute__((packed)) driver_ota_head_s;

typedef struct
{
    uint16_t mark;
    uint16_t flag;
    uint16_t crc;
    uint32_t len;
    uint32_t addr1;
    uint32_t addr2;
    uint16_t info_crc;
    uint8_t reserved[12];
}__attribute__((packed)) driver_ota_info_s;

typedef struct
{
    uint8_t  update_flag;
    uint8_t  flash_protect_flag;
    uint8_t  tx_arqn_nak_flag;
    uint16_t crc;
    uint32_t flash_addr;
    uint32_t flash_offset; 
    uint8_t  data[OTA_BUFFER_SIZE];
    uint16_t data_len;
    driver_ota_info_s ota_info;
}__attribute__((packed)) driver_ota_param_s;

void driver_ota_init(void);
uint8_t driver_ota_is_ongoing(void);
void driver_ota_set_ongoing(uint8_t flag);
void driver_ota_erase_flash(void);
void driver_ota_write_flash(void);
uint16_t driver_ota_get_mark(uint32_t addr);
uint16_t driver_ota_get_version(uint8_t flag);
uint8_t driver_ota_tx_arqn_nak_flag_get(void);
void driver_ota_tx_arqn_nak_flag_set(uint8_t value);
void driver_ota_start(driver_ota_head_s* head_info);
void driver_ota_save_data(uint8_t* data_ptr, uint16_t data_len);
uint8_t driver_ota_finish_result(void);
void driver_ota_reboot(uint16_t time_us);
void driver_ota_pdu_send(uint8_t *pValue, uint16_t length, uint16_t handle);
#endif

#endif
