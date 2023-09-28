/**
 ****************************************************************************************
 *
 * @file appm_task.c
 *
 * @brief RW APP Task implementation
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"          // SW configuration


#if (BLE_APP_PRESENT)

#include "rwapp_config.h"
#include "app_task.h"             // Application Manager Task API
#include "app.h"                  // Application Manager Definition
#include "app_init.h"
#include "app_scan.h"
#include "app_adv.h"
#include "app_non_conn_adv.h"
#include "gapc_task.h"            // GAP Controller Task API
#include "gapm_task.h"            // GAP Manager Task API
#include "gattc_task.h"
#include "arch.h"                 // Platform Definitions
#include <string.h>
#include "co_utils.h"
#include "ke_timer.h"             // Kernel timer
#include "app_fff0.h"              // fff0 Module Definition
#if (BLE_APP_FEE0S)
#include "app_fee0.h"              //  Module Definition
#endif //(BLE_APP_FEE0S)

#if (BLE_APP_FCC0S)
#include "app_fcc0.h"              //  Module Definition
#endif //(BLE_APP_FCC0S)

#if (BLE_APP_ELECTRIC)
#include "app_electric.h"               
#endif 
    
#if (BLE_APP_SEC)
#include "app_sec.h"              // Security Module Definition
#endif //(BLE_APP_SEC)

#if (BLE_APP_HT)
#include "app_ht.h"               // Health Thermometer Module Definition
#include "htpt_task.h"
#endif //(BLE_APP_HT)

#include "fff0s_task.h"
#if (BLE_APP_DIS)
#include "app_dis.h"              // Device Information Module Definition
#include "diss_task.h"
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"             // Battery Module Definition
#include "bass_task.h"
#endif //(BLE_APP_BATT)

#if (BLE_APP_HID)
#include "app_hid.h"              // HID Module Definition
#include "hogpd_task.h"
#endif //(BLE_APP_HID)

#if (BLE_APP_HOGPRH)
#include "app_hogprh.h"           // HOGPRH Module Definition
#include "hogprh_task.h"
#endif //(BLE_APP_HOGPRH)

#if (BLE_APP_AM0)
#include "app_am0.h"             // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (DISPLAY_SUPPORT)
#include "app_display.h"          // Application Display Definition
#endif //(DISPLAY_SUPPORT)
#include "bk_config.h"

//#include "cli_api.h"
#if (CONFIG_DRIVER_OTA == 1)
#include "driver_ota.h"
#endif

#if (BLE_APP_ANCS)
#include "app_ancsc.h"
#endif

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#define  GAPM_OPERATION_NUM 55 
#define  GAPC_OPERATION_NUM 30 
#define  BLE_UAPDATA_MIN_INTVALUE  80
// Slave preferred Connection interval Max
#define  BLE_UAPDATA_MAX_INTVALUE  100
// Slave preferred Connection latency
#define BLE_UAPDATA_LATENCY  20
// Slave preferred Link supervision timeout
#define BLE_UAPDATA_TIMEOUT  500
// 2s (500*10ms)

uint8_t unknow_str[] = {"UNKNOW OPERATION"};

uint8_t ltk_buffer[36];

key_str_t gapm_operation_str[GAPM_OPERATION_NUM];
key_str_t gapc_operation_str[GAPC_OPERATION_NUM];

uint8_t *gapm_operation_key2str(uint8_t value)
{
    
    for(int i = 0;i < GAPM_OPERATION_NUM;i++)
    {
        if(gapm_operation_str[i].key == value)
        {
            return gapm_operation_str[i].str;
        }
    }
    return unknow_str;
}
uint8_t *gapc_operation_key2str(uint8_t value)
{
    
    for(int i = 0;i < GAPC_OPERATION_NUM;i++)
    {
        if(gapc_operation_str[i].key == value)
        {
            return gapc_operation_str[i].str;
        }
    }
    return unknow_str;
}
key_str_t gapm_operation_str[GAPM_OPERATION_NUM]=
{
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation.
    {0x0,"GAPM_NO_OP"},
    

    /* Default operations                               */
    /* ************************************************ */
    /// Reset BLE subsystem: LL and HL.
    {0x01,"GAPM_RESET"},

    /* Configuration operations                         */
    /* ************************************************ */
    /// Set device configuration
    {0x03, "GAPM_SET_DEV_CONFIG"},
    /// Set device channel map
    {0x04,"GAPM_SET_CHANNEL_MAP"},

    /* Retrieve device information                      */
    /* ************************************************ */
    /// Get Local device version
    {0x05,"GAPM_GET_DEV_VERSION"},
    /// Get Local device BD Address
    {0x06,"GAPM_GET_DEV_BDADDR"},
    /// Get device advertising power level
    {0x07,"GAPM_GET_DEV_ADV_TX_POWER"},
    /// Get White List Size.
    {0x08,"GAPM_GET_WLIST_SIZE"},
    /// Retrieve Antenna information
    {0x09,"GAPM_GET_ANTENNA_INFO"},

    /* Security / Encryption Toolbox                    */
    /* ************************************************ */
    /// Resolve device address
    {0x17,"GAPM_RESOLV_ADDR"},
    /// Generate a random address
    {0x18,"GAPM_GEN_RAND_ADDR"},
    /// Use the controller's AES-128 block
    {0x19,"GAPM_USE_ENC_BLOCK"}, 
    /// Generate a 8-byte random number
    {0x1A,"GAPM_GEN_RAND_NB"},

    /* Profile Management                               */
    /* ************************************************ */
    /// Create new task for specific profile
    {0x1B,"GAPM_PROFILE_TASK_ADD"},
    /* DEBUG                                            */
    /* ************************************************ */
    /// Get memory usage
    {0x1C,"GAPM_DBG_GET_MEM_INFO"},
    /// Perform a platform reset
    {0x1D,"GAPM_PLF_RESET"}, 

    /* Data Length Extension                            */
    /* ************************************************ */
    /// Set Suggested Default LE Data Length
    {0x1E,"GAPM_SET_SUGGESTED_DFLT_LE_DATA_LEN"},
    /// Get Suggested Default LE Data Length
    {0x1F,"GAPM_GET_SUGGESTED_DFLT_LE_DATA_LEN"},
    /// Get Maximum LE Data Length
    {0x20,"GAPM_GET_MAX_LE_DATA_LEN"},

    /* Operation on Resolving List                      */
    /* ************************************************ */
    /// Get resolving address list size
    {0x21,"GAPM_GET_RAL_SIZE"},
    /// Get resolving local address
    {0x22,"GAPM_GET_RAL_LOC_ADDR"},
    /// Get resolving peer address
    {0x23,"GAPM_GET_RAL_PEER_ADDR"},

    /* Change current IRK                               */
    /* ************************************************ */
    /// Set IRK
    {0x28,"GAPM_SET_IRK"},

    /* LE Protocol/Service Multiplexer management       */
    /* ************************************************ */
    /// Register a LE Protocol/Service Multiplexer
    {0x29,"GAPM_LEPSM_REG"},
    /// Unregister a LE Protocol/Service Multiplexer
    {0x2A,"GAPM_LEPSM_UNREG"},

    /* LE Direct Test Mode                              */
    /* ************************************************ */
    /// Stop the test mode
    {0x2B,"GAPM_LE_TEST_STOP"},
    /// Start RX Test Mode
    {0x2C,"GAPM_LE_TEST_RX_START"}, 
    /// Start TX Test Mode
    {0x2D,"GAPM_LE_TEST_TX_START"},

    /* Secure Connection                                */
    /* ************************************************ */
    /// Generate DH_Key
    {0x2E,"GAPM_GEN_DH_KEY"},
    /// Retrieve Public Key
    {0x2F,"GAPM_GET_PUB_KEY"},

    /* List Management                                  */
    /* ************************************************ */
    /// Set content of white list
    {0x90,"GAPM_SET_WL"},
    /// Set content of resolving list
    {0x91,"GAPM_SET_RAL"},
    /// Set content of periodic advertiser list
    {0x92,"GAPM_SET_PAL"},
    /// Get periodic advertiser list size
    {0x95,"GAPM_GET_PAL_SIZE"},

    /* Air Operations                                   */
    /* ************************************************ */
    /// Create advertising activity
    {0xA0,"GAPM_CREATE_ADV_ACTIVITY"},
    /// Create scanning activity
    {0xA1,"GAPM_CREATE_SCAN_ACTIVITY"}, 
    /// Create initiating activity
    {0xA2,"GAPM_CREATE_INIT_ACTIVITY"}, 
    /// Create periodic synchronization activity
    {0xA3,"GAPM_CREATE_PERIOD_SYNC_ACTIVITY"}, 
    /// Start an activity
    {0xA4,"GAPM_START_ACTIVITY"}, 
    /// Stop an activity
    {0xA5,"GAPM_STOP_ACTIVITY"}, 
    /// Stop all activities
    {0xA6,"GAPM_STOP_ALL_ACTIVITIES"},
    /// Delete an activity
    {0xA7,"GAPM_DELETE_ACTIVITY"},
    /// Delete all activities
    {0xA8,"GAPM_DELETE_ALL_ACTIVITIES"}, 
    /// Set advertising data
    {0xA9,"GAPM_SET_ADV_DATA"}, 
    /// Set scan response data
    {0xAA,"GAPM_SET_SCAN_RSP_DATA"},
    /// Set periodic advertising data
    {0xAB,"GAPM_SET_PERIOD_ADV_DATA"},
    /// Get number of available advertising sets
    {0xAC,"GAPM_GET_NB_ADV_SETS"},
    /// Get maximum advertising data length supported by the controller
    {0xAD,"GAPM_GET_MAX_LE_ADV_DATA_LEN"},
    /// Get minimum and maximum transmit powers supported by the controller
    {0xAE,"GAPM_GET_DEV_TX_PWR"},
    /// Get the RF Path Compensation values used in the TX Power Level and RSSI calculation
    {0xAF,"GAPM_GET_DEV_RF_PATH_COMP"},
    /// Enable/Disable reception of periodic advertising report
    {0xB0,"GAPM_PER_ADV_REPORT_CTRL"}, 
    /// Enable / Disable IQ sampling
    {0xB1,"GAPM_PER_SYNC_IQ_SAMPLING_CTRL"}, 
    /// Enable / Disable CTE transmission
    {0xB2,"GAPM_PER_ADV_CTE_TX_CTL"},

    /* Debug Commands                                   */
    /* ************************************************ */
    /// Configure the Debug Platform I&Q Sampling generator
    {0x50,"GAPM_DBG_IQGEN_CFG"},

    /* Internal Operations                              */
    /* ************************************************ */
    /// Renew random addresses
    {0xF0,"GAPM_RENEW_ADDR"},
};
key_str_t gapc_operation_str[GAPC_OPERATION_NUM]= 
{
    /*                 Operation Flags                  */
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation
    {0x00,"GAPC_NO_OP"},

    /* Connection management */
    /// Disconnect link
    {0x01,"GAPC_DISCONNECT"},

    /* Connection information */
    /// Retrieve name of peer device.
    {0x02,"GAPC_GET_PEER_NAME"},
    /// Retrieve peer device version info.
    {0x03,"GAPC_GET_PEER_VERSION"},
    /// Retrieve peer device features.
    {0x04,"GAPC_GET_PEER_FEATURES"},
    /// Get Peer device appearance
    {0x05,"GAPC_GET_PEER_APPEARANCE"},
    /// Get Peer device Slaved Preferred Parameters
    {0x06,"GAPC_GET_PEER_SLV_PREF_PARAMS"},
    /// Retrieve connection RSSI.
    {0x07,"GAPC_GET_CON_RSSI"},
    /// Retrieve Connection Channel MAP.
    {0x08,"GAPC_GET_CON_CHANNEL_MAP"},

    /* Connection parameters update */
    /// Perform update of connection parameters.
    {0x09,"GAPC_UPDATE_PARAMS"},

    /* Security procedures */
    /// Start bonding procedure.
    {0x0A,"GAPC_BOND"},
    /// Start encryption procedure.
    {0x0B,"GAPC_ENCRYPT"},
    /// Start security request procedure
    {0x0C,"GAPC_SECURITY_REQ"},

    /* LE Ping*/
    /// get timer timeout value
    {0x12,"GAPC_GET_LE_PING_TO"},
    /// set timer timeout value
    {0x13,"GAPC_SET_LE_PING_TO"},

    /* LE Data Length extension*/
    /// LE Set Data Length
    {0x14,"GAPC_SET_LE_PKT_SIZE"},

    /* Central Address resolution supported*/
    {0x15,"GAPC_GET_ADDR_RESOL_SUPP"}, 

    /* Secure Connections */
    /// Request to inform the remote device when keys have been entered or erased
    {0x16,"GAPC_KEY_PRESS_NOTIFICATION"},

    /* PHY Management */
    /// Set the PHY configuration for current active link
    {0x17,"GAPC_SET_PHY"},
    /// Retrieve PHY configuration of active link
    {0x18,"GAPC_GET_PHY"},

    /* Channel Selection Algorithm */
    /// Retrieve Channel Selection Algorithm
    {0x19,"GAPC_GET_CHAN_SEL_ALGO"},

    /* Preferred slave latency */
    /// Set the preferred slave latency (for slave only, with RW controller)
    {0x1A,"GAPC_SET_PREF_SLAVE_LATENCY"},
    /// Set the preferred slave event duration (for slave only, with RW controller)
    {0x1B,"GAPC_SET_PREF_SLAVE_EVT_DUR"},

    /* Periodic Sync Transfer */
    /// Transfer periodic advertising sync information to peer device
    {0x1C,"GAPC_PER_ADV_SYNC_TRANS"},

    /* Constant Tone Extension */
    /// Constant Tone Extension Transmission configuration
    {0x20,"GAPC_CTE_TX_CFG"},
    /// Constant Tone Extension Reception configuration
    {0x21,"GAPC_CTE_RX_CFG"},
    /// Constant Tone Extension request control (enable / disable)
    {0x22,"GAPC_CTE_REQ_CTRL"},
    /// Constant Tone Extension Response control (enable / disable)
    {0x23,"GAPC_CTE_RSP_CTRL"}, 

    // ---------------------- INTERNAL API ------------------------
    /* Packet signature */
    /// sign an attribute packet
    {0xF0,"GAPC_SIGN_PACKET"},
    /// Verify signature or an attribute packet
    {0xF1,"GAPC_SIGN_CHECK"},
};

static uint8_t app_get_handler(const struct app_subtask_handlers *handler_list_desc,
                               ke_msg_id_t msgid,
                               void *param,
                               ke_task_id_t src_id)
{
    // Counter
    uint8_t counter;

    // Get the message handler function by parsing the message table
    for (counter = handler_list_desc->msg_cnt; 0 < counter; counter--)
    {
        struct ke_msg_handler handler
                = (struct ke_msg_handler)(*(handler_list_desc->p_msg_handler_tab + counter - 1));

        if ((handler.id == msgid) ||
            (handler.id == KE_MSG_DEFAULT_HANDLER))
        {
            // If handler is NULL, message should not have been received in this state
            ASSERT_ERR(handler.func);

            return (uint8_t)(handler.func(msgid, param, TASK_APP, src_id));
        }
    }

    // If we are here no handler has been found, drop the message
    return (KE_MSG_CONSUMED);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_CREATED_IND event
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_created_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_created_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    //bk_printf("%s\r\n",__func__);
    if ((app_env.adv_state == APP_ADV_STATE_CREATING) && (p_param->actv_type == GAPM_ACTV_TYPE_ADV))
    {
        // Store the advertising activity index
        app_env.adv_actv_idx = p_param->actv_idx;
        //bk_printf("adv_actv_idx:%d,tx_pwr:%d\r\n",app_env.adv_actv_idx,p_param->tx_pwr);
    }
#if (BLE_CENTRAL || BLE_OBSERVER)
	else if((app_env.scan_state == APP_SCAN_STATE_CREATING) && (p_param->actv_type == GAPM_ACTV_TYPE_SCAN))
    {
        // Store the scaning activity index
        app_env.scan_actv_idx = p_param->actv_idx;
        app_env.scan_state = APP_SCAN_STATE_CREATED;
        bk_printf("scan_actv_idx:%d,scan_state:%d\r\n",app_env.scan_actv_idx,app_env.scan_state);
    }
    else if((app_env.init_state == APP_INIT_STATE_CREATING) && (p_param->actv_type == GAPM_ACTV_TYPE_INIT))
    {
        // Store the scaning activity index
        app_env.init_actv_idx = p_param->actv_idx;
        app_env.init_state = APP_INIT_STATE_CREATED;
        bk_printf("init_actv_idx:%d\r\n",app_env.init_actv_idx);
    }
#endif
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_ACTIVITY_STOPPED_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_activity_stopped_ind_handler(ke_msg_id_t const msgid,
                                             struct gapm_activity_stopped_ind const *p_param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    //bk_printf("gapm_act_stop,p_param->actv_type(%x),state:%x\r\n",p_param->actv_type,app_env.adv_state);
   
    if ((app_env.adv_state == APP_ADV_STATE_STARTED) && (p_param->actv_type == GAPM_ACTV_TYPE_ADV))
    {
        // Act as if activity had been stopped by the application
        app_env.adv_state = APP_ADV_STATE_CREATED;

    }
#if (BLE_CENTRAL || BLE_OBSERVER)
    else if((app_env.scan_state == APP_SCAN_STATE_STARTED) && (p_param->actv_type == GAPM_ACTV_TYPE_SCAN))
    {
         // Act as if activity had been stopped by the application
        app_env.scan_state = APP_SCAN_STATE_CREATED;
     }
    else if((app_env.init_state == APP_INIT_STATE_CONECTED) && (p_param->actv_type == GAPM_ACTV_TYPE_INIT))
    {     
         // Act as if activity had been stopped by the application
        app_env.init_state = APP_INIT_STATE_CREATED;

     }
#endif
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAPM_PROFILE_ADDED_IND event
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance.
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_profile_added_ind_handler(ke_msg_id_t const msgid,
                                          struct gapm_profile_added_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{ 
    // Current State
    ke_state_t state = ke_state_get(dest_id);
    //bk_printf("%s prf_task_id:%x,prf_task_nb:%d,start_hdl:%d\r\n",__func__,param->prf_task_id, param->prf_task_nb,param->start_hdl);
    if (state == APPM_CREATE_DB)
    {
        switch (param->prf_task_id)
        {
            #if (BLE_APP_AM0)
            case (TASK_ID_AM0_HAS):
            {
                app_am0_set_prf_task(param->prf_task_nb);
            } break;
            #endif //(BLE_APP_AM0)

            default: /* Nothing to do */ break;
        }
    }
    else
    {
        ASSERT_INFO(0, state, src_id);
    }

    return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    #if (NVDS_SUPPORT)
	#if (BLE_APP_SEC)
    uint8_t key_len = KEY_LEN;
	#endif
    #endif //(NVDS_SUPPORT)
    bk_printf("%s operation:%x,status:%x\r\n",__func__,param->operation,param->status);
	//bk_printf("cmp.op(%s)\r\n",gapm_operation_key2str(param->operation));
    switch(param->operation)
    {
        // Reset completed
        case (GAPM_RESET)://0
        {
            if(param->status == GAP_ERR_NO_ERROR)
            {
                #if (NVDS_SUPPORT)
                nvds_tag_len_t len = BD_ADDR_LEN;
                #endif //(NVDS_SUPPORT)
                #if (BLE_APP_HID)
                app_hid_start_mouse();
                #endif //(BLE_APP_HID)

                // Set Device configuration
                struct gapm_set_dev_config_cmd* cmd = KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
                                                                   TASK_GAPM, TASK_APP,
                                                                   gapm_set_dev_config_cmd);
                // Set the operation
                cmd->operation = GAPM_SET_DEV_CONFIG;

                // Set the device role
                #if ((BLE_CENTRAL) && (BLE_PERIPHERAL))
                cmd->role = GAP_ROLE_ALL;
                #elif (BLE_CENTRAL)
                cmd->role = GAP_ROLE_CENTRAL;
                #elif (BLE_PERIPHERAL)
                cmd->role = GAP_ROLE_PERIPHERAL;
                #else
                cmd->role = GAP_ROLE_NONE;
                #endif
                
                #if (BLE_APP_SEC_CON)
                // The Max MTU is increased to support the Public Key exchange
                // HOWEVER, with secure connections enabled you cannot sniff the 
                // LEAP and LEAS protocols
                cmd->max_mtu = 160;
                cmd->pairing_mode = GAPM_PAIRING_SEC_CON | GAPM_PAIRING_LEGACY;
                #else // !(BLE_APP_SEC_CON)
                // Do not support secure connections
                cmd->pairing_mode = GAPM_PAIRING_LEGACY;
                #endif //(BLE_APP_SEC_CON)
                
                // Set Data length parameters
                cmd->sugg_max_tx_octets = LE_MAX_OCTETS;
                cmd->sugg_max_tx_time   = LE_MAX_TIME;
                
                cmd->max_mtu = 512;
                
                #if (BLE_APP_HID)
                // Enable Slave Preferred Connection Parameters present 
//                cmd->att_cfg = GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;
                cmd->att_cfg = 0;
                SETF(cmd->att_cfg,GAPM_ATT_SLV_PREF_CON_PAR_EN,1);
                #endif //(BLE_APP_HID)

                // Host privacy enabled by default
                cmd->privacy_cfg = 0;
                

                #if (NVDS_SUPPORT)
                if (nvds_get(NVDS_TAG_BD_ADDRESS, &len, &cmd->addr.addr[0]) == NVDS_OK)
                {
                    // Check if address is a static random address
                    if (cmd->addr.addr[5] & 0xC0)
                    {
                        // Host privacy enabled by default
                        cmd->privacy_cfg |= GAPM_PRIV_CFG_PRIV_ADDR_BIT;
                    }
                }
                else
                {
                    memcpy(&cmd->addr.addr[0],&co_default_bdaddr.addr[0],BD_ADDR_LEN);
                    if (cmd->addr.addr[5] & 0xC0)
                    {
                        // Host privacy enabled by default
                        cmd->privacy_cfg |= GAPM_PRIV_CFG_PRIV_ADDR_BIT;
                    }
                }
                #endif //(NVDS_SUPPORT)

                #if (BLE_APP_AM0)
                cmd->audio_cfg   = GAPM_MASK_AUDIO_AM0_SUP;
                #endif //(BLE_APP_AM0)


                #if (NVDS_SUPPORT)
                #if (BLE_APP_SEC)
                if ((app_sec_get_bond_status()==true) &&
                    (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) == NVDS_OK))
                {
                    memcpy(cmd->irk.key, app_env.loc_irk, 16);
                }
                else
                #endif 
                #endif //(NVDS_SUPPORT)
                {
                    memset((void *)&cmd->irk.key[0], 0x00, KEY_LEN);
                }
                // Send message
                ke_msg_send(cmd);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;

        case (GAPM_PROFILE_TASK_ADD)://0x1b
        { 
            #if (BLE_APP_SEC)
            if (app_sec_get_bond_status()==true) 
            {
                #if (NVDS_SUPPORT)
                // If Bonded retrieve the local IRK from NVDS
                if (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) == NVDS_OK)
                {
                    // Set the IRK in the GAP
                    struct gapm_set_irk_cmd *cmd = KE_MSG_ALLOC(GAPM_SET_IRK_CMD,
                                                                TASK_GAPM, TASK_APP,
                                                                gapm_set_irk_cmd);
                    ///  - GAPM_SET_IRK: 
                    cmd->operation = GAPM_SET_IRK;
                    memcpy(&cmd->irk.key[0], &app_env.loc_irk[0], KEY_LEN);
                    ke_msg_send(cmd);
                }
                else
                #endif //(NVDS_SUPPORT)
               
                {
                     // If cannot read IRK from NVDS ASSERT
                     ASSERT_ERR(0);
                }
            }
            else // Need to start the generation of new IRK
            #endif //(BLE_APP_SEC)
            {
                struct gapm_gen_rand_nb_cmd *cmd = KE_MSG_ALLOC(GAPM_GEN_RAND_NB_CMD,
                                                                TASK_GAPM, TASK_APP,
                                                                gapm_gen_rand_nb_cmd);
                cmd->operation   = GAPM_GEN_RAND_NB;
                app_env.rand_cnt = 1;
                ke_msg_send(cmd);
            }
        }
        break;

        case (GAPM_GEN_RAND_NB) ://0x1a
        {
            if (app_env.rand_cnt == 1)
            {
                // Generate a second random number
                app_env.rand_cnt++;
                struct gapm_gen_rand_nb_cmd *cmd = KE_MSG_ALLOC(GAPM_GEN_RAND_NB_CMD,
                                                                TASK_GAPM, TASK_APP,
                                                                gapm_gen_rand_nb_cmd);
                cmd->operation = GAPM_GEN_RAND_NB;
                ke_msg_send(cmd);
            }
            else
            {
                struct gapm_set_irk_cmd *cmd = KE_MSG_ALLOC(GAPM_SET_IRK_CMD,
                                                        TASK_GAPM, TASK_APP,
                                                        gapm_set_irk_cmd);
                app_env.rand_cnt=0;
                ///  - GAPM_SET_IRK
                cmd->operation = GAPM_SET_IRK;
                memcpy(&cmd->irk.key[0], &app_env.loc_irk[0], KEY_LEN);
                ke_msg_send(cmd);
            }
        }
        break;

        case (GAPM_SET_IRK):
        {
            // ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            #if (BLE_APP_SEC)
            #if (NVDS_SUPPORT)
            //if (app_sec_get_bond_status()==false)
            if (nvds_get(NVDS_TAG_LOC_IRK, &key_len, app_env.loc_irk) != NVDS_OK)
            {               
                if (nvds_put(NVDS_TAG_LOC_IRK, KEY_LEN, (uint8_t *)&app_env.loc_irk) != NVDS_OK)
                {
                    ASSERT_INFO(0, 0, 0);
                }
            }
            #endif 
            #endif //(BLE_APP_SEC)
            app_env.rand_cnt = 0;
            // Add the next requested service
            if (!appm_add_svc())
            {
                // Go to the ready state
                ke_state_set(TASK_APP, APPC_READY);
                #if (NVDS_SUPPORT)
                nvds_tag_len_t len = 2;
                uint8_t ble_enable_data[2];
                if(nvds_get(NVDS_TAG_BLE_ENABLE, &len, ble_enable_data) == NVDS_OK)
                #endif //(NVDS_SUPPORT)
                {
                    // No more service to add, start advertising
                    appm_create_advertising();
#if (BLE_CENTRAL || BLE_OBSERVER)
					appm_update_scan_state(1);
#endif
                }
            }
        }
        break;

        // Device Configuration updated
        case (GAPM_SET_DEV_CONFIG):
        {
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            // Go to the create db state
            ke_state_set(TASK_APP, APPM_CREATE_DB);

            // Add the first required service in the database
            // and wait for the PROFILE_ADDED_IND
            appm_add_svc();
        }
        break;
        case (GAPM_CREATE_ADV_ACTIVITY):
        {
            if(app_env.adv_state == APP_ADV_STATE_CREATING)
                appm_set_adv_data();  
            else if(app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_CREATING)
                appm_set_non_conn_adv_data();
        }break;
        case (GAPM_STOP_ACTIVITY):
        {   
            // Go created state
            if(app_env.adv_state == APP_ADV_STATE_STOPPING)
                app_env.adv_state = APP_ADV_STATE_CREATED;
            else if(app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_STOPPING)
                app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_CREATED;
                
        }break;
        case (GAPM_START_ACTIVITY):
        {
            // Go to started state
            app_env.adv_state = APP_ADV_STATE_STARTED;
            appm_create_non_conn_advertising();
        }break;
        case (GAPM_DELETE_ACTIVITY):
        {
            if(app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_DELETE)
                app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_IDLE;
            else
                app_env.adv_state = APP_ADV_STATE_IDLE;
        }break;
        case (GAPM_SET_ADV_DATA):
        {
            if(app_env.adv_state == APP_ADV_STATE_SETTING_ADV_DATA)
                appm_set_scan_rsp_data();
            else if(app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_SETTING_ADV_DATA)
                appm_start_non_conn_advertising();
            
        }break;
        case (GAPM_SET_SCAN_RSP_DATA):
        {
            // Sanity checks
            ASSERT_INFO(app_env.adv_op == param->operation, app_env.adv_op, param->operation);
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->status, app_env.adv_op);

            // Perform next operation
           // Start advertising activity
            appm_start_advertising();
            
        } break;        
        case (GAPM_CREATE_SCAN_ACTIVITY):
        case (GAPM_START_SCAN_ACTIVITY):
        case (GAPM_STOP_SCAN_ACTIVITY):
        case (GAPM_DELETE_SCAN_ACTIVITY):
        {
            // Sanity checks
            ASSERT_INFO(app_env.scan_op == param->operation, app_env.scan_op, param->operation);
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);

            #if (BLE_CENTRAL || BLE_OBSERVER)
            // Perform next operation 
            appm_scan_fsm_next();
            #endif
        } 
        break;
        
        case (GAPM_CREATE_INIT_ACTIVITY):
        case (GAPM_START_INIT_ACTIVITY):
        case (GAPM_STOP_INIT_ACTIVITY):
        case (GAPM_DELETE_INIT_ACTIVITY):
        {
            // Sanity checks
            ASSERT_INFO(app_env.init_op == param->operation, app_env.init_op, param->operation);
            ASSERT_INFO(param->status == GAP_ERR_NO_ERROR, param->operation, param->status);
            
            #if (BLE_CENTRAL || BLE_OBSERVER)
            // Perform next operation 
            appm_init_fsm_next();
            #endif
        } 
        break;  
        
        case (GAPM_DELETE_ALL_ACTIVITIES) :
        {
            // Re-Invoke Advertising
            app_env.adv_state = APP_ADV_STATE_IDLE;
            appm_create_advertising();
        } 
		break;


        default:
        {
            // Drop the message
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

static int gapc_get_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_get_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    //bk_printf("%s,req:0x%x\r\n",__func__,param->req);
    switch(param->req)
    {
        case GAPC_DEV_NAME:
        {
            struct gapc_get_dev_info_cfm * cfm = KE_MSG_ALLOC_DYN(GAPC_GET_DEV_INFO_CFM,
                                                    src_id, dest_id,
                                                    gapc_get_dev_info_cfm, APP_DEVICE_NAME_MAX_LEN);
            cfm->req = param->req;
            cfm->info.name.length = appm_get_dev_name(cfm->info.name.value);

            //bk_printf("length:%d,name:%s\r\n",cfm->info.name.length,cfm->info.name.value);
            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_APPEARANCE:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                                                             src_id, dest_id,
                                                             gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Set the device appearance
            #if (BLE_APP_HT)
            // Generic Thermometer - TODO: Use a flag
            cfm->info.appearance = 728;
            #elif (BLE_APP_HID)
            // HID Mouse
            cfm->info.appearance = 962;
            #else
            // No appearance
            cfm->info.appearance = 0;
            #endif

            // Send message
            ke_msg_send(cfm);
        } break;

        case GAPC_DEV_SLV_PREF_PARAMS:
        {
            // Allocate message
            struct gapc_get_dev_info_cfm *cfm = KE_MSG_ALLOC(GAPC_GET_DEV_INFO_CFM,
                    src_id, dest_id,
                                                            gapc_get_dev_info_cfm);
            cfm->req = param->req;
            // Slave preferred Connection interval Min
            cfm->info.slv_pref_params.con_intv_min = 8;
            // Slave preferred Connection interval Max
            cfm->info.slv_pref_params.con_intv_max = 10;
            // Slave preferred Connection latency
            cfm->info.slv_pref_params.slave_latency  = 0;
            // Slave preferred Link supervision timeout
            cfm->info.slv_pref_params.conn_timeout    = 200;  // 2s (500*10ms)

            // Send message
            ke_msg_send(cfm);
        } break;

        default: /* Do Nothing */ break;
    }


    return (KE_MSG_CONSUMED);
}
/**
 ****************************************************************************************
 * @brief Handles GAPC_SET_DEV_INFO_REQ_IND message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_set_dev_info_req_ind_handler(ke_msg_id_t const msgid,
        struct gapc_set_dev_info_req_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
    // Set Device configuration
    struct gapc_set_dev_info_cfm* cfm = KE_MSG_ALLOC(GAPC_SET_DEV_INFO_CFM, src_id, dest_id,
                                                     gapc_set_dev_info_cfm);
    // Reject to change parameters
    cfm->status = GAP_ERR_REJECTED;
    cfm->req = param->req;
    // Send message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_connection_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    app_env.conidx = KE_IDX_GET(src_id);
    	
    //bk_printf("%s conidx:%d,dest_id:0x%x\r\n",__func__,app_env.conidx,dest_id);
    
    // Check if the received Connection Handle was valid
    if (app_env.conidx != GAP_INVALID_CONIDX)
    {
        // Retrieve the connection info from the parameters
        app_env.conhdl = param->conhdl;
        
        /// Connection handle
        bk_printf("con_req: interval:%dus, latency:%d, timeout:%dms\r\n", param->con_interval*1250, param->con_latency, param->sup_to*10); 
        #if 0
        bk_printf("peer_addr:0x");
        for(uint8_t i = 0;i < GAP_BD_ADDR_LEN;i++)
        {
            os_printf("%02x",param->peer_addr.addr[i]);
            
        }os_printf("\r\n");
        #endif
        //app_connect_device_info_get();

        gapc_get_bdaddr(app_env.conidx, GAPC_SMP_INFO_PEER)->addr_type = param->peer_addr_type;
        memcpy(&(gapc_get_bdaddr(app_env.conidx, GAPC_SMP_INFO_PEER)->addr),&param->peer_addr,sizeof(bd_addr_t));

        app_env.con_dev_addr[app_env.conidx].addr_type = param->peer_addr_type;
        memcpy(app_env.con_dev_addr[app_env.conidx].addr.addr,param->peer_addr.addr,6);
        app_env.role[app_env.conidx] = param->role;
        bk_printf("condix: %d, role: %s\r\n", app_env.conidx, param->role ? "Slave" : "Master");
        
        // Send connection confirmation
        struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
                KE_BUILD_ID(TASK_GAPC, app_env.conidx), KE_BUILD_ID(TASK_APP,app_env.conidx),
                gapc_connection_cfm);

        #if(BLE_APP_SEC)
        cfm->auth = app_sec_get_bond_status() ? GAP_AUTH_REQ_NO_MITM_BOND : GAP_AUTH_REQ_NO_MITM_NO_BOND; // TODO [FBE] restore valid data
        bk_printf("auth:%s\r\n",cfm->auth ? "REQ_NO_MITM_BOND" : "REQ_NO_MITM_NO_BOND");
        #else // !(BLE_APP_SEC)
        cfm->auth      = GAP_AUTH_REQ_NO_MITM_NO_BOND;
        #endif // (BLE_APP_SEC)
        // Send the message
        ke_msg_send(cfm);

        #if DISPLAY_SUPPORT
        // Update displayed information
        app_display_set_adv(false);
        app_display_set_con(true);
        #endif //(DISPLAY_SUPPORT)

        /*--------------------------------------------------------------
         * ENABLE REQUIRED PROFILES
         *--------------------------------------------------------------*/

        #if (BLE_APP_BATT)
        // Enable Battery Service
        app_batt_enable_prf(app_env.conidx);
        #endif //(BLE_APP_BATT)

        #if (BLE_APP_HID)
        // Enable HID Service
        app_hid_enable_prf(app_env.conidx);
        #endif //(BLE_APP_HID)

    	// We are now in connected State
        ke_state_set(KE_BUILD_ID(TASK_APP,app_env.conidx), APPC_LINK_CONNECTED);


        if(param->role == 0) // master role
        {
            #if (BLE_CENTRAL)
            sdp_discover_all_service(app_env.conidx);
            #endif

            #if(BLE_APP_SEC)
            app_sec_bond_cmd_req(app_env.conidx);  //master SMP the BLE key
            #endif
        }
        
        #if (BLE_APP_SEC && !defined(BLE_APP_AM0))
        if (app_sec_get_bond_status())
        {
            // Ask for the peer device to either start encryption
          //  app_sec_send_security_req(app_env.conidx);
        }
        app_sec_env.bonded = false;
        #endif // (BLE_APP_SEC && !defined(BLE_APP_AM0))


        ke_timer_set(APP_GATTC_EXC_MTU_CMD,TASK_APP,20);
		
        #if (BLE_APP_ANCS)
	    ke_timer_set(APP_ANCS_REQ_IND,TASK_APP,30); 
        #endif
        
        #if (NVDS_SUPPORT)
        uint8_t length = 6;
        uint8_t env_update_param[6];
        if (nvds_get(NVDS_TAG_BLE_UPDATE_CONFIG_ENABLE, &length, env_update_param) == NVDS_OK)
        {
            uint16_t update_delay = co_read16p(env_update_param)*1000;

            ke_timer_set(APP_PARAM_UPDATE_REQ, TASK_APP, update_delay);
        }
        #endif //(NVDS_SUPPORT)
    }
    else
    {
        // No connection has been established, restart advertising
       
     #if (NVDS_SUPPORT)
		 nvds_tag_len_t len = 2;
		 uint8_t ble_enable_data[2];
		 if(nvds_get(NVDS_TAG_BLE_ENABLE, &len, ble_enable_data) == NVDS_OK)
	 #endif //(NVDS_SUPPORT)
    	{
       		 appm_start_advertising();
  		}
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Enable all required profiles
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_update_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_param_update_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{


    bk_printf("%s\r\n",__func__);
    // Check if the received Connection Handle was valid
    if (KE_IDX_GET(src_id)!= GAP_INVALID_CONIDX)
    {
		app_env.conidx = KE_IDX_GET(src_id);
        // Send connection confirmation
        struct gapc_param_update_cfm *cfm = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CFM,
                KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                gapc_param_update_cfm);

        cfm->accept = true;
        cfm->ce_len_min = 0xffff;
        cfm->ce_len_max = 0xffff;

        // Send message
        ke_msg_send(cfm);

        //if((ke_state_get(TASK_APP)==APPC_LINK_CONNECTED)&& (param->con_latency>0))
        {
            //ke_msg_send_basic(APP_PARAM_UPDATE_REQ,TASK_APP,TASK_APP);
        }

	#if 0
		// Requested connection parameters
		appm_update_param(8, 8, 25, 200);
	#endif

    }
    if (KE_IDX_GET(src_id)== GAP_INVALID_CONIDX)
    {
        // No connection has been established, restart advertising
      	bk_printf("%s,%d\r\n",__func__, __LINE__);
     #if (NVDS_SUPPORT)
		 nvds_tag_len_t len = 2;
		 uint8_t ble_enable_data[2];
		 if(nvds_get(NVDS_TAG_BLE_ENABLE, &len, ble_enable_data) == NVDS_OK)
	 #endif //(NVDS_SUPPORT)
    	{
       		 appm_start_advertising();
  		}
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief  GAPC_PARAM_UPDATED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_param_updated_ind_handler (ke_msg_id_t const msgid, 
									const struct gapc_param_updated_ind  *param,
                 					ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    __unused uint8_t conidx = KE_IDX_GET(src_id);
	
    bk_printf("con_param_update: interval:%dus, latency:%d, timeout:%dms\r\n", param->con_interval*1250,param->con_latency,param->sup_to*10);

#if 0
    if((ke_state_get(TASK_APP)==APPC_LINK_CONNECTED)&& (param->con_latency>0))
    {
        ke_msg_send_basic(APP_PARAM_UPDATE_REQ,TASK_APP,TASK_APP);
    }
#endif

#if 0
	//if((ke_state_get(TASK_APP)==APPC_SDP_DISCOVERING) && (param->con_latency>0))
    if((ke_state_get(TASK_APP)==APPC_LINK_CONNECTED) && (param->con_latency>0))
    {
        //ke_msg_send_basic(APP_PARAM_UPDATE_REQ, KE_BUILD_ID(TASK_APP,conidx), dest_id);
    	ke_state_set(KE_BUILD_ID(TASK_APP,conidx), APPC_SDP_DISCOVERING);
    	app_hogprh_enable_prf(conidx);
    }
#endif
	    
	return KE_MSG_CONSUMED;
}

/*******************************************************************************
 * Function: gapc_le_pkt_size_ind_handler
 * Description: GAPC_LE_PKT_SIZE_IND
 * Input: msgid   -Id of the message received.
 *		  param   -Pointer to the parameters of the message.
 *		  dest_id -ID of the receiving task instance
 *		  src_id  -ID of the sending task instance.
 * Return: If the message was consumed or not.
 * Others: void
*******************************************************************************/
static int gapc_le_pkt_size_ind_handler (ke_msg_id_t const msgid, 
									const struct gapc_le_pkt_size_ind  *param,
                 					ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
   	//bk_printf("%s msgid:0x%x,dest_id:0x%x,src_id:0x%x\r\n",__func__,msgid,dest_id,src_id);
	//bk_printf("le_pkt_size MaxRxOctets:%d, MaxRxTime:%d, MaxTxOctets:%d, MaxTxTime:%d\r\n",
	//             param->max_rx_octets, param->max_rx_time, param->max_tx_octets, param->max_tx_time);
	
	return KE_MSG_CONSUMED;
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    //bk_printf("%s conidx:%d,operation:0x%x,%s,status:%x\r\n",__func__,conidx,param->operation,gapc_operation_key2str(param->operation),param->status);
 
    switch(param->operation)
    {
        case (GAPC_UPDATE_PARAMS):
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
//                appm_disconnect();
            }
        } break;
		case (GAPC_DISCONNECT): //0x01
		{
			if(param->status == GAP_ERR_NO_ERROR)
			{
				//bk_printf("bonded = 0x%x\r\n",app_sec_env.bonded);
				//bonding info lost and pairing fail

			}
		}break;
		case (GAPC_SECURITY_REQ): //0x0c
		{
			if (param->status != GAP_ERR_NO_ERROR)
	        {
	            bk_printf("gapc security req fail !\r\n");
	        }
	        else
	        {
	            bk_printf("gapc security req ok !\r\n");


            }
        } break;

        case (GAPC_GET_PEER_FEATURES):
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                //appm_disconnect();
            }
            else
            {
            	appm_get_peer_version(conidx);
            }
        } break;

        case (GAPC_GET_PEER_VERSION):
        {
            if (param->status != GAP_ERR_NO_ERROR)
            {
                //appm_disconnect();
            }
            else
            {
        		ke_state_set(KE_BUILD_ID(TASK_APP,app_env.conidx), APPC_SMP_BONDING);
                #if (BLE_APP_SEC)
        		app_sec_bond_cmd_req(conidx);
                #endif
				//sdp_discover_all_service(app_env.conidx);
            }
        } break;

    	case (GAPC_BOND):
    	{
            #if (BLE_APP_SEC)
    		if (param->status != GAP_ERR_NO_ERROR)
    		{

    		}
    		else
    		{
    			if(app_sec_env.bonded == true)
    			{
    				bk_printf("gapc bond ok\r\n");

					#if (BLE_APP_HOGPRH)
    				ke_state_set(KE_BUILD_ID(TASK_APP,conidx), APPC_SDP_DISCOVERING);
    				app_hogprh_enable_prf(app_env.conidx);
					#endif

                	//appm_disconnect(conidx); // for PTS test
    			}
    			else
    			{
    				bk_printf("gapc bond fail\r\n");
    				appm_disconnect(conidx);
    			}
    		}
            #endif
    	}
    	break;

        default:
        {
        } break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
//extern uint8_t first_flag ;
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                      struct gapc_disconnect_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    
    uint8_t conidx = KE_IDX_GET(src_id);
    extern void	appm_send_gapm_reset_cmd();
    bk_printf("rwble disconnected, idx:%d, reason:0x%x\r\n", conidx, param->reason);
	 // Reset the stack
    
    // Go to the ready state
    ke_state_set(KE_BUILD_ID(TASK_APP, conidx), APPC_LINK_IDLE);

    #if (BLE_APP_HT)
    // Stop interval timer
    app_stop_timer();
    #endif //(BLE_APP_HT)

    #if (DISPLAY_SUPPORT)
    // Update Connection State screen
    app_display_set_con(false);
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_ISO_MODE_0_PROTOCOL)
    app_env.adv_state = APP_ADV_STATE_CREATING;
    #endif //(BLE_ISO_MODE_0_PROTOCOL)
	

	//appm_send_gapm_reset_cmd();//
    //return (KE_MSG_CONSUMED);

    #if (BLE_PERIPHERAL)
	     #if (NVDS_SUPPORT)
		 nvds_tag_len_t len = 2;
		 uint8_t ble_enable_data[2];
		 if(nvds_get(NVDS_TAG_BLE_ENABLE, &len, ble_enable_data) == NVDS_OK)
		 #endif //(NVDS_SUPPORT)
    	{
       		 appm_start_advertising();
  		}
	#endif
    
#if (BLE_FEE0_SERVER)
    //first_flag = 0;
#endif

    if(ke_timer_active(APP_PARAM_UPDATE_REQ, TASK_APP))
    {
        ke_timer_clear(APP_PARAM_UPDATE_REQ, TASK_APP);
    }

    if(ke_timer_active(APP_GATTC_EXC_MTU_CMD, TASK_APP))
    {
        ke_timer_clear(APP_GATTC_EXC_MTU_CMD, TASK_APP);
    }
    
#if (CONFIG_DRIVER_OTA == 1)
    if(driver_ota_is_ongoing())      // ota reboot when RWBLE disconnected with exception
        driver_ota_reboot(0);
#endif

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of all messages sent from the lower layers to the application
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int appm_msg_handler(ke_msg_id_t const msgid,
                            void *param,
                            ke_task_id_t const dest_id,
                            ke_task_id_t const src_id)
{
    // Retrieve identifier of the task from received message
    ke_task_id_t src_task_id = MSG_T(msgid);
    // Message policy
    uint8_t msg_pol = KE_MSG_CONSUMED;
    uint8_t found_flag = 1;
    switch (src_task_id)
    {
        case (TASK_ID_GAPC):
        {
            #if (BLE_APP_SEC)
            //#if ((BLE_APP_SEC) && (SMP_ENCRYPT_EN))
            if ((msgid >= GAPC_BOND_CMD) &&
                (msgid <= GAPC_SECURITY_IND))
            {
                // Call the Security Module
                msg_pol = app_get_handler(&app_sec_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_SEC)
            // else drop the message
        } break;

        case (TASK_ID_GATTC):
        {
            // Service Changed - Drop
        } break;

        #if (BLE_APP_FFF0S)
        case (TASK_ID_FFF0S):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_fff0_handler, msgid, param, src_id);
        } break;
        #endif

        #if (BLE_APP_FEE0S)
        case (TASK_ID_FEE0S):
        {
            // Call the app fee0s Module
            msg_pol = app_get_handler(&app_fee0_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_FEE0S)
            
        #if (BLE_APP_FCC0S)
        case (TASK_ID_FCC0S):
        {
            // Call the app fee0s Module
            msg_pol = app_get_handler(&app_fcc0_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_FCC0S)
        
        #if (BLE_APP_ELECTRIC)
        case (TASK_ID_ELECTRIC):
        {
            // Call the app electric Module
            msg_pol = app_get_handler(&app_electric_handler, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_ELECTRIC)
        
        #if (BLE_APP_HT)
        case (TASK_ID_HTPT):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HT)

        #if (BLE_APP_DIS)
        case (TASK_ID_DISS):
        {
            // Call the Device Information Module
            msg_pol = app_get_handler(&app_dis_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_DIS)

        #if (BLE_APP_HID)
        case (TASK_ID_HOGPD):
        {
            // Call the HID Module
            msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HID)

        #if (BLE_APP_HOGPRH)
        case (TASK_ID_HOGPRH):
        {
            // Call the HOGPRH Module
            msg_pol = app_get_handler(&app_hogprh_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_HOGPRH)

        #if (BLE_APP_BATT)
        case (TASK_ID_BASS):
        {
            // Call the Battery Module
            msg_pol = app_get_handler(&app_batt_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_BATT)

        #if (BLE_APP_AM0)
        case (TASK_ID_AM0):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_handlers, msgid, param, src_id);
        } break;

        case (TASK_ID_AM0_HAS):
        {
            // Call the Audio Mode 0 Module
            msg_pol = app_get_handler(&app_am0_has_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_AM0)
         #if(BLE_APP_ANCS)
        case (TASK_ID_ANCSC):
        {
            // Call the Health Thermometer Module
            msg_pol = app_get_handler(&app_ancsc_handler, msgid, param, src_id);
        } break;
        #endif

        #if (CONFIG_OTA_BLE)
        case (TASK_ID_OTAS):
        {
            // Call the OTA Module
            extern const struct app_subtask_handlers app_otas_handlers;
            msg_pol = app_get_handler(&app_otas_handlers, msgid, param, src_id);
        } break;
        #endif //(BLE_APP_BATT)

        default:
        {
            #if (BLE_APP_HT)
            if (msgid == APP_HT_MEAS_INTV_TIMER)
            {
                msg_pol = app_get_handler(&app_ht_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_HT)

            #if (BLE_APP_HID)
            if (msgid == APP_HID_MOUSE_TIMEOUT_TIMER)
            {
                msg_pol = app_get_handler(&app_hid_handlers, msgid, param, src_id);
            }
            #endif //(BLE_APP_HID)
            found_flag = 0;
        } break;
    }
    if(!found_flag)
    {
        bk_printf("%s,src_task_id:0x%x,dest_id:0x%x,src_id:0x%x,msgid:0x%x\r\n",__func__,src_task_id,dest_id,src_id,msgid);  
    }
 
    return (msg_pol);
}

/**
 ****************************************************************************************
 * @brief Handles reception of random number generated message
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_gen_rand_nb_ind_handler(ke_msg_id_t const msgid, struct gapm_gen_rand_nb_ind *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
	if (app_env.rand_cnt==1)      // First part of IRK
    {
        memcpy(&app_env.loc_irk[0], &param->randnb.nb[0], 8);
    }
    else if (app_env.rand_cnt==2) // Second part of IRK
    {
        memcpy(&app_env.loc_irk[8], &param->randnb.nb[0], 8);
    }

    return KE_MSG_CONSUMED;
}

#if (BLE_OBSERVER || BLE_CENTRAL )
static int gapm_ext_adv_report_ind_handler(ke_msg_id_t const msgid, struct gapm_ext_adv_report_ind *param,
                                        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
#if 0
    bk_printf("%s\r\n",__func__);
    bk_printf("actv_idx:%x\r\n",param->actv_idx);
    bk_printf("info:%x\r\n",param->info);
    bk_printf("ch:%d\r\n",param->channel_idx);
    bk_printf("rssi:%d\r\n",param->rssi);
    bk_printf("addr_type:%x\r\n",param->trans_addr.addr_type);
#endif

#if 0
    bk_printf("adv addr:");
    for(int i = 0; i < 6; i++)
    {
        os_printf("%02x ", param->trans_addr.addr.addr[i]);
    }os_printf("\r\n");
    
    bk_printf("adv data(len:%d):",param->length);
    for(int i = 0; i < param->length; i++)
    {
        os_printf("%02x ", param->data[i]);
    }os_printf("\r\n\r\n");
#endif

	uint8_t keyword[] = "PTN101_RW_ble";
    //uint8_t addr1[6] = {0x88, 0x99, 0x84, 0x94, 0x86, 0x96};
    //uint8_t addr2[6] = {0x66, 0x55, 0x48, 0x3e, 0x2c, 0x1e};

    if((app_env.init_state <= APP_INIT_STATE_CREATED) 
        && appm_adv_data_decode(param->length, param->data, keyword, sizeof(keyword) - 1)
        && !app_check_connect_by_addr(param->trans_addr.addr))
    //if((memcmp(param->trans_addr.addr.addr, addr1, 6) == 0)||(memcmp(param->trans_addr.addr.addr, addr2, 6) == 0))
    {
    	os_printf("\r\n!!!Find device:%02x:%02x:%02x:%02x:%02x:%02x!!!\r\n", param->trans_addr.addr.addr[0],param->trans_addr.addr.addr[1],
                   param->trans_addr.addr.addr[2],param->trans_addr.addr.addr[3],param->trans_addr.addr.addr[4],param->trans_addr.addr.addr[5]);

    	/// scan active to stop
    	appm_scan_fsm_next();

    	/// connect active start
#if 1
        struct gap_bdaddr conn_bdaddr;
        memcpy(&conn_bdaddr, &param->trans_addr, sizeof(struct gap_bdaddr));
        appm_set_connect_dev_addr(conn_bdaddr);
        appm_update_init_state(true);
#else
        struct gap_bdaddr conn_bdaddr = {BLE_DIRECT_CONN_BDADDR, ADDR_PUBLIC};
        appm_set_connect_dev_addr(conn_bdaddr);
        appm_update_init_state(true);
#endif
    }
    
    return KE_MSG_CONSUMED;
}
#endif

#if (BLE_SDP_CLIENT)
#if 0
static int gattc_sdp_svc_ind_handler(ke_msg_id_t const msgid,
                                     struct gattc_sdp_svc_ind const *ind,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    bk_printf("appc %s msgid:0x%x,dest_id:0x%x,src_id:0x%x\r\n",__func__,msgid,dest_id,src_id);
   // struct prf_sdp_db_env *prf_db_env = NULL;
     
    bk_printf("ind uuid len:%d,uuid:",ind->uuid_len);
    for(int i = 0;i < ind->uuid_len;i++)
    {
        bk_printf("%02x ",ind->uuid[ind->uuid_len - i - 1]);
    }
    bk_printf("\r\n");

	//sdp_extract_svc_info(conidx, ind);

    return (KE_MSG_CONSUMED);
}
#endif
static int app_gattc_event_ind_handler(ke_msg_id_t const msgid,
                                   struct gattc_event_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    //uint8_t state = ke_state_get(dest_id);
    uint8_t conidx = KE_IDX_GET(src_id);
    bk_printf("gattc_event_ind conidx:%d\r\n",conidx);
    bk_printf("NOTIF RECIVE length:%d,value = \r\n",param->length);
    for(int i = 0; i< param->length; i++)
    {
        bk_printf("%02x ",param->value[i]);
    }bk_printf("\r\n");

   return (KE_MSG_CONSUMED);  
}

static int app_gattc_event_req_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_event_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    //uint8_t state = ke_state_get(dest_id);
    //uint8_t w_data[20];
    //bk_printf("appm %s \r\n",__func__);
    //bk_printf("type = 0x%x,length = 0x%x,handle = 0x%02x\r\n",param->type,param->length,param->handle);
    bk_printf("RECIVE value =  \r\n");
    for(int i = 0; i< param->length; i++)
    {
        bk_printf("%02x ",param->value[i]);
    }
    bk_printf("\r\n");

    if(param->type == GATTC_INDICATE)
    {
        bk_printf("app_gattc_event_req_ind: INDICATE\r\n");

        struct gattc_event_cfm *cfm  = KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
        cfm->handle = param->handle;
        ke_msg_send(cfm);
    }
    else
    {
        bk_printf("app_gattc_event_req_ind: NOTIFY\r\n");
    }
   return (KE_MSG_CONSUMED);  
}
#endif

/**
 ****************************************************************************************
 * @brief  GATTC_MTU_CHANGED_IND
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_gattc_mtu_changed_ind_handler(ke_msg_id_t const msgid,
                                     struct gattc_mtu_changed_ind const *ind,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    //uint8_t conidx = KE_IDX_GET(src_id);
	bk_printf("mtu size changed:%d\r\n", ind->mtu);
	ke_timer_clear(APP_GATTC_EXC_MTU_CMD,TASK_APP);
 	return (KE_MSG_CONSUMED);
}

#if (BLE_BROADCASTER) 
static int gapm_scan_request_ind_handler(ke_msg_id_t const msgid,
                                 struct gapm_scan_request_ind const *p_ind,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    bk_printf("%s\r\n",__func__);
    bk_printf("actv_idx:%d,addr_type:%d\r\n",p_ind->actv_idx,p_ind->trans_addr.addr_type);
    bk_printf("addr ");
    for(int i = 0;i < 6;i++)
    {
        bk_printf(":%02x ",p_ind->trans_addr.addr.addr[i]);
    }bk_printf("\r\n");
    return (KE_MSG_CONSUMED);  
}
#endif

//extern uint8_t send_data_enable_flag[BLE_CONNECTION_MAX];
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    //uint8_t state = ke_state_get(dest_id);
    
    //uint8_t find;
    uint8_t conidx = KE_IDX_GET(src_id);
    //bk_printf("app %s dest_id = %x,conidx:%d\r\n",__func__,dest_id,conidx);
    //bk_printf("operation = 0x%x,status = 0x%x,seq_num = 0x%x\r\n",param->operation,param->status,param->seq_num);
    
    if(((param->operation == GATTC_WRITE_NO_RESPONSE) || (param->operation == GATTC_WRITE)) && (param->seq_num != 0xa5))
	{ 	
        bk_printf("\r\n~~~~~~set send_data_enable_flag 1\r\n");

	}
    if((param->operation == GATTC_SDP_DISC_SVC_ALL))
	{
        bk_printf("APPC_SERVICE_CONNECTED, idx:%d\r\n", conidx);

    	ke_state_set(KE_BUILD_ID(TASK_APP,conidx),APPC_SERVICE_CONNECTED);

        //find = sdp_enable_all_server_ntf_ind(conidx,1);
        //bk_printf("find1= %d\r\n",find);


        //app_hogprh_enable_prf(app_env.conidx);
#if 0
        if(sdp_enable_all_server_ntf_ind(conidx,1))
        {
        	bk_printf("enable all server ntf OK!\r\n");
        }
        else
		{
        	bk_printf("enable all server ntf FAIL!\r\n");
		}
#endif
	}
    
    if((param->operation == GATTC_WRITE) && (param->seq_num == 0xaa))
	{
    	
        bk_printf("\r\nGATTC_WRITE\r\n");
        //sdp_enable_all_server_ntf_ind(conidx,0);
	}    
    	   
    if(param->operation == GATTC_MTU_EXCH)
	{
		//set_flash_clk(0x1);
	#if (SMP_ENCRYPT_EN)
        #if (BLE_APP_SEC)
		if(!app_sec_get_bond_status())
		{
			//sdp_service_status_reset(app_env.conidx);
			//ke_timer_set(APP_START_SMP_REQ_TIMER, KE_BUILD_ID(TASK_APP, app_env.conidx),5);
		}
		else
		{
			//ke_timer_set(APP_START_ENCRYPT_TIMER,TASK_APP,5);
		}
        #endif
	#else
		ke_state_set(KE_BUILD_ID(TASK_APP,conidx), APPC_SDP_DISCOVERING);
		//sdp_discover_all_service(app_env.conidx);
		//app_hogprh_enable_prf(app_env.conidx);
	#endif
    
    #if (BLE_CENTRAL)
        //sdp_discover_all_service(conidx);
    #endif
	}

    return (KE_MSG_CONSUMED);
}

static int gapc_conn_rssi_ind_handler(ke_msg_id_t const msgid,
        struct gapc_con_rssi_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{

	//if(ke_state_get(dest_id) == APPM_CONNECTED)
	{
		bk_printf("get rssi = %d\r\n",param->rssi);
	}

	return (KE_MSG_CONSUMED);
}

static int gapc_peer_version_ind_handler(ke_msg_id_t const msgid,
        struct gapc_peer_version_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
	uint8_t conidx = KE_IDX_GET(dest_id);
	//uint8_t support = true;

	app_env.device_ver[conidx] = param->lmp_vers;
	switch(param->lmp_vers)
	{
	case RW_BT40_VERSION:
		bk_printf("device version:4.0\r\n");
		break;
	case RW_BT41_VERSION:
		bk_printf("device version:4.1\r\n");
		break;
	case RW_BT42_VERSION:
		bk_printf("device version:4.2\r\n");
		break;
	case RW_BT50_VERSION:
		bk_printf("device version:4.0\r\n");
		break;
	case RW_BT51_VERSION:
		bk_printf("device version:5.1\r\n");
		break;
	default:
		bk_printf("no support device version (#%d)\r\n", param->lmp_vers);
		//support = false;
		break;
	}

	return (KE_MSG_CONSUMED);
}

static int gapc_peer_features_ind_handler(ke_msg_id_t const msgid,
        struct gapc_peer_features_ind const *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{

	return (KE_MSG_CONSUMED);
}

static int app_update_conn_param_req_handler (ke_msg_id_t const msgid,
        const struct gapc_param_update_req_ind  *param,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
#if (NVDS_SUPPORT)
    uint8_t length = 6;
    uint8_t env_update_param[6];
#endif //(NVDS_SUPPORT)

	struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = 6;
    cmd->intv_max   = 6;
    cmd->latency    = 0;
    cmd->time_out   = 300;

    // not used by a slave device
    cmd->ce_len_min = 10;
    cmd->ce_len_max = 20;
    
#if (NVDS_SUPPORT)
    if (nvds_get(NVDS_TAG_BLE_UPDATE_CONFIG_ENABLE, &length, env_update_param) == NVDS_OK)
    {
        cmd->intv_min   = co_read16p(&env_update_param[2]);
        cmd->intv_max   = co_read16p(&env_update_param[4]);
        cmd->latency    = 0;
        cmd->time_out   = 600;
    }
#endif //(NVDS_SUPPORT)

    bk_printf("conn_param_req: intv_min = %d,intv_max = %d,latency = %d,time_out = %d\r\n",cmd->intv_min,cmd->intv_max,cmd->latency,cmd->time_out);

    // Send the message
    ke_msg_send(cmd);

	return KE_MSG_CONSUMED;
}

static int app_mtu_exchange_req_handler(ke_msg_id_t const msgid,
        struct gattc_exc_mtu_cmd const *req,
        ke_task_id_t const dest_id,
        ke_task_id_t const src_id)
{
	//bk_printf("%s \r\n", __func__);
	struct gattc_exc_mtu_cmd *cmd = KE_MSG_ALLOC(GATTC_EXC_MTU_CMD,
	                                KE_BUILD_ID(TASK_GATTC, app_env.conidx),
	                                TASK_APP,gattc_exc_mtu_cmd);
	cmd->operation = GATTC_MTU_EXCH;
	cmd->seq_num = 0;
	ke_msg_send(cmd);

	return (KE_MSG_CONSUMED);
}

static int app_get_rssi_timer_handler(ke_msg_id_t const msgid,
                                    void const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
	bk_printf("%s\r\n", __func__);

	if(ke_state_get(dest_id) >= APPC_LINK_CONNECTED)
	{
		appm_get_conn_rssi(app_env.conidx);
	}

	ke_timer_set(APP_GET_RSSI_TIMER,TASK_APP,500);

	return KE_MSG_CONSUMED;

}

static int app_start_smpc_handler(ke_msg_id_t const msgid,
                                    void *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)

{
    #if (BLE_APP_SEC)
	uint8_t conidx = KE_IDX_GET(dest_id);

	bk_printf("%s, ke_state\r\n", __func__, ke_state_get(TASK_APP));

	ke_state_set(KE_BUILD_ID(TASK_APP,app_env.conidx), APPC_SMP_BONDING);
	app_sec_bond_cmd_req(conidx);
    #endif
	return KE_MSG_CONSUMED;
}


static int app_start_encrypt_handler(ke_msg_id_t const msgid,
                                    struct gapm_profile_added_ind *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
	bk_printf("%s\r\n", __func__);
	//uint8_t conidx = KE_IDX_GET(src_id);

	//bk_printf("app set encrypt res status\r\n");
	//app_sec_encry_cmd_req();

	return (KE_MSG_CONSUMED);
}
static int app_send_security_req_handler(ke_msg_id_t const msgid, 
										void const *param,
        								ke_task_id_t const dest_id, 
        								ke_task_id_t const src_id)
{   
    #if (BLE_APP_SEC)
	app_sec_send_security_req(app_env.conidx);
    #endif
    return KE_MSG_CONSUMED;
}

#if (BLE_APP_ANCS)
static int app_ancs_req_handler(ke_msg_id_t const msgid, void const *param,
        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    //bk_printf("app_ancs_req_handler\r\n");
    app_ancsc_enable_prf(app_env.conhdl);
    return KE_MSG_CONSUMED;
}
#endif

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

/* Default State handlers definition. */
KE_MSG_HANDLER_TAB(appm)
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,    (ke_msg_func_t)appm_msg_handler},

    // GAPM messages
    {GAPM_PROFILE_ADDED_IND,    (ke_msg_func_t)gapm_profile_added_ind_handler},
    {GAPM_ACTIVITY_CREATED_IND, (ke_msg_func_t)gapm_activity_created_ind_handler},
    {GAPM_ACTIVITY_STOPPED_IND, (ke_msg_func_t)gapm_activity_stopped_ind_handler},
    {GAPM_CMP_EVT,              (ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPM_GEN_RAND_NB_IND,      (ke_msg_func_t)gapm_gen_rand_nb_ind_handler},
    #if (BLE_BROADCASTER)    
    {GAPM_SCAN_REQUEST_IND,      (ke_msg_func_t)gapm_scan_request_ind_handler},
    #endif
    
    #if (BLE_OBSERVER || BLE_CENTRAL )
    {GAPM_EXT_ADV_REPORT_IND,   (ke_msg_func_t)gapm_ext_adv_report_ind_handler},
    #endif    
       

    // GAPC messages
    {GAPC_GET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_get_dev_info_req_ind_handler},
    {GAPC_SET_DEV_INFO_REQ_IND, (ke_msg_func_t)gapc_set_dev_info_req_ind_handler},
    {GAPC_CONNECTION_REQ_IND,   (ke_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_PARAM_UPDATE_REQ_IND, (ke_msg_func_t)gapc_param_update_req_ind_handler},
    {GAPC_PARAM_UPDATED_IND,    (ke_msg_func_t)gapc_param_updated_ind_handler},
    {GAPC_LE_PKT_SIZE_IND,      (ke_msg_func_t)gapc_le_pkt_size_ind_handler},
    {GAPC_CMP_EVT,              (ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t)gapc_disconnect_ind_handler},
	{GAPC_CON_RSSI_IND, 		(ke_msg_func_t)gapc_conn_rssi_ind_handler},
	{GAPC_PEER_VERSION_IND,		(ke_msg_func_t)gapc_peer_version_ind_handler},
	{GAPC_PEER_FEATURES_IND,	(ke_msg_func_t)gapc_peer_features_ind_handler},
     // GATTC messages
    #if (BLE_SDP_CLIENT)
    //{GATTC_SDP_SVC_IND,         (ke_msg_func_t)gattc_sdp_svc_ind_handler},
    {GATTC_EVENT_IND,           (ke_msg_func_t)app_gattc_event_ind_handler},
    {GATTC_EVENT_REQ_IND,       (ke_msg_func_t)app_gattc_event_req_ind_handler},
    #endif
    {GATTC_MTU_CHANGED_IND,     (ke_msg_func_t)app_gattc_mtu_changed_ind_handler},
    {GATTC_CMP_EVT,             (ke_msg_func_t)gattc_cmp_evt_handler},

	// APPM messages
	{APP_PARAM_UPDATE_REQ, 		(ke_msg_func_t)app_update_conn_param_req_handler},
	{APP_SEND_SECURITY_REQ,     (ke_msg_func_t)app_send_security_req_handler},
    #if (BLE_APP_ANCS)
	{APP_ANCS_REQ_IND,			(ke_msg_func_t)app_ancs_req_handler},
    #endif

	{APP_GATTC_EXC_MTU_CMD,		(ke_msg_func_t)app_mtu_exchange_req_handler},
	{APP_GET_RSSI_TIMER,		(ke_msg_func_t)app_get_rssi_timer_handler},
	{APP_START_SMP_REQ_TIMER,   (ke_msg_func_t)app_start_smpc_handler},
    {APP_START_ENCRYPT_TIMER,   (ke_msg_func_t)app_start_encrypt_handler},
    
};

/* Defines the place holder for the states of all the task instances. */
ke_state_t appm_state[APP_IDX_MAX];

// Application task descriptor
const struct ke_task_desc TASK_DESC_APP = {appm_msg_handler_tab, appm_state, APP_IDX_MAX, ARRAY_LEN(appm_msg_handler_tab)};

#endif //(BLE_APP_PRESENT)

/// @} APPTASK
