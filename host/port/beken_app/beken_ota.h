#ifndef _APP_OTA_H_
#define _APP_OTA_H_

#include "config.h"
#include <stdint.h>
#include "app_beken_includes.h"

#if (BEKEN_OTA == 1)
#if (CONFIG_OTA_BLE == 1)
void app_ota_add_otas(void);
void app_ota_ble_send(uint8 *pValue, uint16_t length);
#endif
#if (CONFIG_OTA_SPP == 1)
void beken_ota_spp_pkt_reframe(uint8_t *pValue, uint16_t length);
#endif
void beken_ota_update_size_req(uint16_t size);
#endif

#endif
