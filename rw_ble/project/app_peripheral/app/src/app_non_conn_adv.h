/**
 ****************************************************************************************
 *
 * @file app_adv.h
 *
 * @brief Application entry point
 *
 * Copyright (C) Beken 2009-2016
 *
 *
 ****************************************************************************************
 */
 
#ifndef APP_NON_CONN_ADV_H_
#define APP_NON_CONN_ADV_H_

#include "rwip_config.h"     // SW configuration
#include <stdint.h>          // Standard Integer Definition
#include <co_bt.h>           // Common BT Definitions
#include "arch.h"            // Platform Definitions
#include "gapc_task.h"       // GAPC Definitions

#define APP_NON_CONN_ADV_ENABLE    0
#if (BLE_APP_PRESENT) 
/// Advertising channel map - 37, 38, 39
#define APP_NON_CONN_ADV_CHMAP           (0x07)
/// Advertising minimum interval - 50ms (80*0.625ms)
#define APP_NON_CONN_ADV_INT_MIN         (80 )
/// Advertising maximum interval - 50ms (80*0.625ms)
#define APP_NON_CONN_ADV_INT_MAX         (80)
/// Fast advertising interval
#define APP_NON_CONN_ADV_FAST_INT        (32)

/// Advertising state machine
enum app_non_conn_adv_state
{
    /// Advertising activity does not exists
    APP_NON_CONN_ADV_STATE_IDLE = 0,
    /// Creating advertising activity
    APP_NON_CONN_ADV_STATE_CREATING,
    /// Setting advertising data
    APP_NON_CONN_ADV_STATE_SETTING_ADV_DATA,
     /// Updata adv data
    APP_NON_CONN_ADV_STATE_UPDATA_ADV_DATA,
    /// Advertising activity created
    APP_NON_CONN_ADV_STATE_CREATED,
    /// Starting advertising activity
    APP_NON_CONN_ADV_STATE_STARTING,
    /// Advertising activity started
    APP_NON_CONN_ADV_STATE_STARTED,
    /// Stopping advertising activity
    APP_NON_CONN_ADV_STATE_STOPPING,
    APP_NON_CONN_ADV_STATE_DELETE,

};
/**
 ****************************************************************************************
 * @brief
 ****************************************************************************************
 */
void appm_non_conn_adv_fsm_next(void);

/**
 ****************************************************************************************
 * @brief Send to request to update the adv data parameters
 ****************************************************************************************
 */
uint8_t appm_updata_non_conn_adv_data(uint16_t length, uint8_t *p_data);

/**
 ****************************************************************************************
 * @brief Start/stop advertising
 *
 * @param[in] start     True if advertising has to be started, else false
 ****************************************************************************************
 */
void appm_update_non_conn_adv_state(bool start);

/**
 ****************************************************************************************
 * @brief delete advertising
 *
 * @param[in] none
 ****************************************************************************************
 */
void appm_create_non_conn_advertising(void);
void appm_set_non_conn_adv_data(void);
void appm_start_non_conn_advertising(void);
void appm_delete_non_conn_advertising(void);
bool app_gapm_non_conn_adv_check_data_sanity(uint16_t length, uint8_t *p_data);
void appm_non_conn_adv_fsm_next(void);
 
#endif //

#endif //

