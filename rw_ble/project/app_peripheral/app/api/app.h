/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef APP_H_
#define APP_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief Application entry point.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration
#include "rwapp_config.h"

#if (BLE_APP_PRESENT)

#include <stdint.h>          // Standard Integer Definition
#include <co_bt.h>           // Common BT Definitions
#include "arch.h"            // Platform Definitions
#include "gapc_task.h"       // GAPC Definitions

#if (NVDS_SUPPORT)
#include "nvds.h"
#endif // (NVDS_SUPPORT)

/*
 * DEFINES
 ****************************************************************************************
 */

/// Maximal length of the Device Name value
#define APP_DEVICE_NAME_MAX_LEN      (18)

#define BLE_DIRECT_CONN_BDADDR  {{0x40, 0x01, 0x00, 0x33, 0x22, 0x11}} // Magic KBD
//#define BLE_DIRECT_CONN_BDADDR  {{0x91, 0x07, 0x06, 0xDC, 0x1B, 0x00}} // PTS test dongle
//#define BLE_DIRECT_CONN_BDADDR  {{0xA4, 0xCC, 0x2A, 0xB6, 0x64, 0xE7}} // MS BT Mouse
//#define BLE_DIRECT_CONN_BDADDR  {{0xA4, 0xCC, 0x2A, 0xB6, 0x64, 0xE7}} // MS BT Mouse

#define BLE_DIRECT_MANUFACTURER  {0x06, 0x00, 0x04, 0x01, 0x4a, 0x02} // Microsoft Manufacturer Specific Data

/// Default Advertising duration - 30s (in multiple of 10ms)
#define APP_DFLT_ADV_DURATION        (3000)

/*
 * MACROS
 ****************************************************************************************
 */

#define APP_HANDLERS(subtask)    {&subtask##_msg_handler_list[0], ARRAY_LEN(subtask##_msg_handler_list)}

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
enum appm_error
{
    APPM_ERROR_NO_ERROR,
    APPM_ERROR_LINK_LOSS,
    APPM_ERROR_STATE,
    APPM_ERROR_NTFIND_DISABLE,
    APPM_ERROR_LINK_MAX
}; 



enum appm_svc_list
{
    #if (BLE_APP_FFF0S)
    APPM_SVC_FFF0S,
    #endif //(BLE_APP_FFF0S)

    #if (BLE_APP_FEE0S)
    APPM_SVC_FEE0S,
    #endif //(BLE_APP_FEE0S)
    
    #if (BLE_APP_FCC0S)
    APPM_SVC_FCC0S,
    #endif //(BLE_APP_FCC0S)
    
    #if (BLE_APP_ELECTRIC)
    APPM_SVC_ELECTRIC ,                
    #endif //(BLE_APP_ELECTRIC)
    
    #if (BLE_APP_HT)
    APPM_SVC_HTS,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    APPM_SVC_DIS,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    APPM_SVC_BATT,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    APPM_SVC_HIDS,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_HOGPRH)
    APPM_SVC_HOGPRHS,
    #endif //(BLE_APP_HOGPRH)
    #if (BLE_APP_AM0)
    APPM_SVC_AM0_HAS,
    #endif //(BLE_APP_AM0)
    
    #if (CONFIG_OTA_BLE)
    APPM_SVC_OTAS,
    #endif//(CONFIG_OTA_BLE)
    #if(BLE_APP_ANCS)
    APPM_SVC_ANCSC,
    #endif

    APPM_SVC_LIST_STOP,
};


#if (NVDS_SUPPORT)
/// List of Application NVDS TAG identifiers
enum app_nvds_tag //see enum PARAM_ID 
{
#if 0
    /// BD Address
    NVDS_TAG_BD_ADDRESS                 = PARAM_ID_BD_ADDRESS,
    NVDS_LEN_BD_ADDRESS                 = PARAM_LEN_BD_ADDRESS,

    /// Device Name
    NVDS_TAG_DEVICE_NAME                = PARAM_ID_DEVICE_NAME,
    NVDS_LEN_DEVICE_NAME                = PARAM_LEN_DEVICE_NAME,
#endif
    /// BLE Application Advertising data
    NVDS_TAG_APP_BLE_ADV_DATA           = 0x90,
    NVDS_LEN_APP_BLE_ADV_DATA           = 32,

    /// BLE Application Scan response data
    NVDS_TAG_APP_BLE_SCAN_RESP_DATA     = 0x91,
    NVDS_LEN_APP_BLE_SCAN_RESP_DATA     = 32,

    /// Mouse Sample Rate
    NVDS_TAG_MOUSE_SAMPLE_RATE          = 0x92,
    NVDS_LEN_MOUSE_SAMPLE_RATE          = 1,

    /// Peripheral Bonded
    NVDS_TAG_PERIPH_BONDED              = 0x93,
    NVDS_LEN_PERIPH_BONDED              = 1,

    /// Mouse NTF Cfg
    NVDS_TAG_MOUSE_NTF_CFG              = 0x94,
    NVDS_LEN_MOUSE_NTF_CFG              = 2,

    /// Mouse Timeout value
    NVDS_TAG_MOUSE_TIMEOUT              = 0x95,
    NVDS_LEN_MOUSE_TIMEOUT              = 2,

    /// Peer Device BD Address
    NVDS_TAG_PEER_BD_ADDRESS            = 0x96,
    NVDS_LEN_PEER_BD_ADDRESS            = 7,

    /// Mouse Energy Safe
    NVDS_TAG_MOUSE_ENERGY_SAFE          = 0x97,
    NVDS_LEN_MOUSE_SAFE_ENERGY          = 2,

    /// EDIV (2bytes), RAND NB (8bytes),  LTK (16 bytes), Key Size (1 byte)
    NVDS_TAG_LTK                        = 0x98,
    NVDS_LEN_LTK                        = 28,

    /// PAIRING
    NVDS_TAG_PAIRING                    = 0x99,
    NVDS_LEN_PAIRING                    = 54,

    /// Local device Identity resolving key
    NVDS_TAG_LOC_IRK                    = 0x9A,
    NVDS_LEN_LOC_IRK                    = KEY_LEN,

    /// Peer device Resolving identity key (+identity address)
    NVDS_TAG_PEER_IRK                   = 0x9B,
    NVDS_LEN_PEER_IRK                   = sizeof(struct gapc_irk),
        
    ///  device SCAN params
    NVDS_TAG_SCAN_INTV                   = 0x9C,
    NVDS_LEN_SCAN_INTV                   = 2,
    
    NVDS_TAG_SCAN_WD                  = 0x9D,
    NVDS_LEN_SCAN_WD                   = 2,
    
    ///  device CONN params
    NVDS_TAG_CONN_INTV                   = 0x9E,
    NVDS_LEN_CONN_INTV                   = 2,
    
    NVDS_TAG_CONN_LAT                  = 0x9F,
    NVDS_LEN_CONN_LAT                   = 2,
    
    NVDS_TAG_CONN_SUP_TO                  = 0xA0,
    NVDS_LEN_CONN_SUP_TO                   = 2,
    
};
#endif // (NVDS_SUPPORT)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Structure containing information about the handlers for an application subtask
struct app_subtask_handlers
{
    /// Pointer to the message handler table
    const struct ke_msg_handler *p_msg_handler_tab;
    /// Number of messages handled
    uint16_t msg_cnt;
};

/// Application environment structure
struct app_env_tag
{
    /// Connection handle
    uint16_t conhdl;
    /// Connection Index
    uint8_t  conidx;

    /// Advertising activity index
    uint8_t adv_actv_idx;
    /// Current advertising state (@see enum app_adv_state)
    uint8_t adv_state;
    /// Next expected operation completed event
    uint8_t adv_op;

    /// Advertising activity index
    uint8_t non_conn_adv_actv_idx;
    // Current advertising state (@see enum app_adv_state)
    uint8_t non_conn_adv_state;
    /// Next expected operation completed event
    uint8_t non_conn_adv_op;
    
    /// Scaning activity index
    uint8_t scan_actv_idx;
    /// Current scaning state (@see enum app_scan_state)
    uint8_t scan_state;
    /// Next expected operation completed event
    uint8_t scan_op;
        /// Scan interval
    uint16_t scan_intv;
    /// Scan window
    uint16_t scan_wd;
    
    /// Init activity index
    uint8_t init_actv_idx;
    /// Current init state (@see enum app_init_state)
    uint8_t init_state;
    /// Next expected operation completed event
    uint8_t init_op;
    /// conn_intv value. Allowed range is 7.5ms to 4s.
    uint16_t conn_intv;
        /// Slave latency. Number of events that can be missed by a connected slave device
    uint16_t conn_latency;
    /// Link supervision timeout (in unit of 10ms). Allowed range is 100ms to 32s
    uint16_t conn_super_to;

    /// Last initialized profile
    uint8_t next_svc;

    /// Bonding status
    bool bonded;

    /// Device Name length
    uint8_t dev_name_len;
    /// Device Name
    uint8_t dev_name[APP_DEVICE_NAME_MAX_LEN];

    /// Local device IRK
    uint8_t loc_irk[KEY_LEN];

    /// Secure Connections on current link
    bool sec_con_enabled;

    /// Counter used to generate IRK
    uint8_t rand_cnt;
    
        
    uint8_t role[BLE_CONNECTION_MAX];
    /// Address information about a device address
    struct gap_bdaddr con_dev_addr[BLE_CONNECTION_MAX];
    uint8_t device_ver[BLE_CONNECTION_MAX];
};
typedef struct key_str
{
    uint8_t key;
    unsigned char str[50];
}key_str_t;

/*
 * GLOBAL VARIABLE DECLARATION
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the BLE demo application.
 ****************************************************************************************
 */
void appm_init(void);

/**
 ****************************************************************************************
 * @brief Add a required service in the database
 ****************************************************************************************
 */
bool appm_add_svc(void);



/**
 ****************************************************************************************
 * @brief Send to request to update the connection parameters
 ****************************************************************************************
 */
void appm_update_param(uint16_t intv_min, uint16_t intv_max, uint16_t latency, uint16_t time_out);

/**
 ****************************************************************************************
 * @brief Send a disconnection request
 ****************************************************************************************
 */
void appm_disconnect(uint8_t conidx);
void appm_disconnect_env(void);

/**
 ****************************************************************************************
 * @brief Retrieve device name
 *
 * @param[out] device name
 *
 * @return name length
 ****************************************************************************************
 */
uint8_t appm_get_dev_name(uint8_t* name);

/**
 ****************************************************************************************
 * @brief Set device name
 *
 * @param[out] device name
 *
 * @return name length
 ****************************************************************************************
 */
uint8_t appm_set_dev_name(uint8_t len,uint8_t* name);

/**
 ****************************************************************************************
 * @brief Return if the device is currently bonded
 ****************************************************************************************
 */
bool app_sec_get_bond_status(void);

/**
 ****************************************************************************************
 * @brief Return if the BLE is connection
 ****************************************************************************************
 */
uint8_t appm_get_connection_num(void);

void appm_get_peer_name(uint8_t conidx);
void appm_get_peer_version(uint8_t conidx);
void appm_get_peer_features(uint8_t conidx);
void appm_get_conn_rssi(uint8_t conidx);
void appm_set_le_pkt_size(uint8_t conidx);
void appm_set_phy_cmd(uint8_t conidx);
uint8_t app_connect_device_info_get(void);
uint8_t app_check_connect_by_addr(bd_addr_t addr);

/// @} APP

#endif //(BLE_APP_PRESENT)

#endif // APP_H_
