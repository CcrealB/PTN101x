/**
 ****************************************************************************************
 *
 * @file app_fee0.c
 *
 * @brief fee0 Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2020.03.17
 *
 * Copyright (C) Beken 2020-2022
 *
 *
 ****************************************************************************************
 */
#ifndef APP_FEE0_H_
#define APP_FEE0_H_
/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief fee0 Application Module entry point
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_FEE0S)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"         // Kernel Task Definition
#if (BLE_SDP_CLIENT)
#include "sdp_service.h"
#endif
/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */

/// fee0s Application Module Environment Structure
struct app_fee0_env_tag
{
    /// Connection handle
    uint8_t conidx;
    uint16_t svc_fee2_write_handle[BLE_CONNECTION_MAX];
    uint16_t svc_fee3_write_handle[BLE_CONNECTION_MAX];
    uint16_t svc_notif_handle[BLE_CONNECTION_MAX];
    uint16_t svc_ind_handle[BLE_CONNECTION_MAX];
    uint16_t ntf_cfg[BLE_CONNECTION_MAX];
    uint16_t ind_cfg[BLE_CONNECTION_MAX];
};
/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// fee0s Application environment
extern struct app_fee0_env_tag app_fee0_env;

/// Table of message handlers
extern const struct app_subtask_handlers app_fee0_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * fff0s Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize fff0s Application Module
 ****************************************************************************************
 */
void app_fee0_init(void);
/**
 ****************************************************************************************
 * @brief Add a fee0 Service instance in the DB
 ****************************************************************************************
 */
void app_fee0_add_fee0s(void);
/**
 ****************************************************************************************
 * @brief Enable the fee0 Service
 ****************************************************************************************
 */

uint8_t  app_fee4_send_ntf(uint8_t conidx,uint16_t len,uint8_t* buf);
/**
 ****************************************************************************************
 * @brief Send a fee5  value
 ****************************************************************************************
 */

uint8_t  app_fee5_send_ind(uint8_t conidx,uint16_t len,uint8_t* buf);

#if (BLE_SDP_CLIENT)
uint8_t app_add_sdp_fee0_srv(uint8_t conidx,uint8_t uuid_len,struct gattc_sdp_svc_ind const *ind);
#endif
#endif//

#endif // APP_FEE0_H_
