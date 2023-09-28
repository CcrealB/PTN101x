/**
 ****************************************************************************************
 *
 * @file app_hid.h
 *
 * @brief HID Application Module entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef APP_HOGPRH_H_
#define APP_HOGPRH_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief HID Application Module entry point
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_APP_HOGPRH)

#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"         // Kernel Task Definition

#include "hogprh.h"

#if (PS2_SUPPORT)
#include "ps2.h"             // PS2 Mouse Driver
#endif //(PS2_SUPPORT)

/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */

/// send report data
struct send_report_data_tag
{
	uint8_t report_id;
	uint8_t data[12];
};

/// HID Application Module Environment Structure
struct app_hogprh_env_tag
{
    /// Connection handle
    uint8_t conidx;
    /// Internal state of the module
    uint8_t state;
    /// HID number
    uint8_t hids_nb[HOGPRH_IDX_MAX];
};


/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */

/// Table of message handlers
extern const struct app_subtask_handlers app_hogprh_handlers;

/*
 * GLOBAL FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * Health Thermometer Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize HID Application Module
 ****************************************************************************************
 */
void app_hogprh_init(void);

/**
 ****************************************************************************************
 * @brief Add a HID Service instance in the DB
 ****************************************************************************************
 */
void app_hogprh_add_hogprhs(void);

/**
 ****************************************************************************************
 * @brief Enable the HID Over GATT Profile device role
 *
 * @param[in]:  conhdl - Connection handle for the connection
 ****************************************************************************************
 */
void app_hogprh_enable_prf(uint8_t conidx);

#endif //(BLE_APP_HOGPRH)

/// @} APP

#endif // APP_HOGPRH_H_
