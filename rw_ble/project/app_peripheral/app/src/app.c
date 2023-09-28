/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)
#include "rwapp_config.h"
#include <string.h>
#include "reg_blecore.h"
#include "rwip.h"
#include "app_task.h"                // Application task Definition
#include "app.h"                    
#include "app_init.h"
#include "app_scan.h"
#include "app_adv.h"
#include "gap.h"                     // GAP Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API
#include "gattc_int.h"
#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition

#if (BLE_APP_SEC)
#include "app_sec.h"                 // Application security Definition
#endif // (BLE_APP_SEC)

#if (BLE_APP_FEE0S)
#include "app_fee0.h"                  // USER define FEE0S Application Definitions
#endif //(BLE_APP_FEE0S)

#if (BLE_APP_FCC0S)
#include "app_fcc0.h"                  // USER define FCC0S Application Definitions
#endif //(BLE_APP_FCC0S)

#if (BLE_APP_ELECTRIC)
#include "app_electric.h"                  // USER define ELECTRIC Application Definitions
#endif //(BLE_APP_ELECTRIC)


#if (BLE_APP_HT)
#include "app_ht.h"                  // Health Thermometer Application Definitions
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"                 // Device Information Service Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"                // Battery Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h"                 // HID Application Definitions
#endif //(BLE_APP_HID)

#if (BLE_APP_HOGPRH)
#include "app_hogprh.h"              // HOGPRH Application Definitions
#endif //(BLE_APP_HOGPRH)

#if (DISPLAY_SUPPORT)
#include "app_display.h"             // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"                 // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h"                    // NVDS Definitions
#endif //(NVDS_SUPPORT)

#if (CONFIG_OTA_BLE)
#include "beken_ota.h"               // Beken_OTA Definitions
#endif //(CONFIG_OTA_BLE)

#if(BLE_APP_ANCS)
#include "app_ancsc.h"               // Application ancs Definition
#endif


#include "sdp_service.h"
#include "bk_config.h"
#include "app_fff0.h" 

/*
 * DEFINES
 ****************************************************************************************
 */





/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database


/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Task Descriptor
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
    #if (BLE_APP_FFF0S)
	(appm_add_svc_func_t)app_fff0_add_fff0s,
	#endif//(BLE_APP_FFF0S)
    
    #if (BLE_APP_FEE0S)
    (appm_add_svc_func_t)app_fee0_add_fee0s,
    #endif //(BLE_APP_FEE0S)
    
    #if (BLE_APP_FCC0S)
    (appm_add_svc_func_t)app_fcc0_add_fcc0s,
    #endif //(BLE_APP_FCC0S)
    
    #if (BLE_APP_ELECTRIC)
    (appm_add_svc_func_t)app_add_electric,                
    #endif 
    
    #if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_HOGPRH)
    (appm_add_svc_func_t)app_hogprh_add_hogprhs,
    #endif //(BLE_APP_HOGPRH)
    #if (BLE_APP_AM0)
    (appm_add_svc_func_t)app_am0_add_has,
    #endif //(BLE_APP_AM0)
    #if (CONFIG_OTA_BLE)
    (appm_add_svc_func_t)app_ota_add_otas,
    #endif//(CONFIG_OTA_BLE)
	#if (BLE_APP_ANCS)
    (appm_add_svc_func_t)app_ancs_add_ancsc,
    #endif
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */



void appm_send_gapm_reset_cmd(void)
{
    // Reset the stack
    struct gapm_reset_cmd *p_cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                                TASK_GAPM, TASK_APP,
                                                gapm_reset_cmd);

    p_cmd->operation = GAPM_RESET;

    ke_msg_send(p_cmd);
    
    //bk_printf("appm_send_gapm_reset_cmd\r\n");
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void appm_init(void)
{
    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

    // Get the Device Name to add in the Advertising Data (Default one or NVDS one)
    #if (NVDS_SUPPORT)
    app_env.dev_name_len = APP_DEVICE_NAME_MAX_LEN;
    if (nvds_get(NVDS_TAG_DEVICE_NAME, &(app_env.dev_name_len), app_env.dev_name) != NVDS_OK)
    #endif //(NVDS_SUPPORT)
    {
        // Get default Device Name (No name if not enough space)
        memcpy(app_env.dev_name, APP_DFLT_DEVICE_NAME, APP_DFLT_DEVICE_NAME_LEN);
        app_env.dev_name_len = APP_DFLT_DEVICE_NAME_LEN;

        // TODO update this value per profiles
    }


    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/

    // load device information:

    #if (DISPLAY_SUPPORT)
    app_display_init();
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_APP_SEC)
    // Security Module
    app_sec_init();
    #endif // (BLE_APP_SEC)

    #if (BLE_APP_HT)
    // Health Thermometer Module
    app_ht_init();
    #endif //(BLE_APP_HT)

    #if (BLE_APP_DIS)
    // Device Information Module
    app_dis_init();
    #endif //(BLE_APP_DIS)

    #if (BLE_APP_HID)
    // HID Module
    app_hid_init();
    #endif //(BLE_APP_HID)

    #if (BLE_APP_HOGPRH)
    // HOGPRH Module
    app_hogprh_init();
    #endif //(BLE_APP_HOGPRH)

    #if (BLE_APP_BATT)
    // Battery Module
    app_batt_init();
    #endif //(BLE_APP_BATT)

    #if (BLE_APP_AM0)
    // Audio Mode 0 Module
    app_am0_init();
    #endif //(BLE_APP_AM0)
	
    #if (BLE_APP_ANCS)
    app_ancsc_init();
    #endif

    #if (BLE_SDP_CLIENT)
    sdp_service_init();	
    #endif
    // Reset the stack
    appm_send_gapm_reset_cmd();
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }
    //bk_printf("%s more_svc:%d\r\n",__func__,more_svc);
    return more_svc;
}

void appm_disconnect(uint8_t conidx)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, conidx), KE_BUILD_ID(TASK_APP, conidx),
                                                   gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}
void appm_disconnect_env(void)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, app_env.conidx), KE_BUILD_ID(TASK_APP, app_env.conidx),
                                                   gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}

void appm_update_param(uint16_t intv_min, uint16_t intv_max, uint16_t latency, uint16_t time_out)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = intv_min;
    cmd->intv_max   = intv_max;
    cmd->latency    = latency;
    cmd->time_out   = time_out;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}

uint8_t appm_get_dev_name(uint8_t* name)
{
    // copy name to provided pointer
    memcpy(name, app_env.dev_name, app_env.dev_name_len);
    // return name length
    return app_env.dev_name_len;
}

uint8_t appm_set_dev_name(uint8_t len,uint8_t* name)
{
    // copy name to provided pointer
    if(len < APP_DEVICE_NAME_MAX_LEN)
    {   
        app_env.dev_name_len = len;
        memcpy(app_env.dev_name, name, len);
        // return name length
        return app_env.dev_name_len;
    
    }
    return 0;   
}

void appm_get_peer_version(uint8_t conidx)
{
	// connection index has been put in addr_src
	struct gapc_get_info_cmd* info_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                            	        KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                            	        gapc_get_info_cmd);

	// request peer device name.
	info_cmd->operation = GAPC_GET_PEER_VERSION;

	// send command
	ke_msg_send(info_cmd);
}

void appm_get_peer_features(uint8_t conidx)
{
	// connection index has been put in addr_src
	struct gapc_get_info_cmd* info_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                            	        KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                            	        gapc_get_info_cmd);

	// request peer device name.
	info_cmd->operation = GAPC_GET_PEER_FEATURES;

	// send command
	ke_msg_send(info_cmd);
}

void appm_get_conn_rssi(uint8_t conidx)
{
	// connection index has been put in addr_src
	struct gapc_get_info_cmd* info_cmd = KE_MSG_ALLOC(GAPC_GET_INFO_CMD,
                                            	        KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                            	        gapc_get_info_cmd);

	// request peer device name.
	info_cmd->operation = GAPC_GET_CON_RSSI;

	// send command
	ke_msg_send(info_cmd);
}

void appm_set_le_pkt_size(uint8_t conidx)
{
	// connection index has been put in addr_src
	struct gapc_set_le_pkt_size_cmd* info_cmd = KE_MSG_ALLOC(GAPC_SET_LE_PKT_SIZE_CMD,
                                            	        KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                            	        gapc_set_le_pkt_size_cmd);

	// request peer le_pkt_size.
    info_cmd->operation = GAPC_SET_LE_PKT_SIZE;
    
    info_cmd->tx_octets = 0xFB;
    info_cmd->tx_time = 500;     //500us

	// send command
	ke_msg_send(info_cmd);
}

void appm_set_phy_cmd(uint8_t conidx)
{
	// connection index has been put in addr_src
	struct gapc_set_phy_cmd* info_cmd = KE_MSG_ALLOC(GAPC_SET_PHY_CMD,
                                            	        KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                            	        gapc_set_phy_cmd);

	// request peer set_phy
    info_cmd->operation = GAPC_SET_PHY;
    
    info_cmd->tx_phy = GAP_PHY_LE_2MBPS;
    info_cmd->rx_phy = GAP_PHY_LE_2MBPS;
    info_cmd->phy_opt = GAPC_PHY_OPT_LE_CODED_ALL_RATES;

	// send command
	ke_msg_send(info_cmd);
}

uint8_t appm_get_connection_num(void)
{
    int i = 0;
    uint8_t num = 0;

    for(i = 0; i < APP_IDX_MAX; i++)
    {
        if(ke_state_get(KE_BUILD_ID(TASK_APP, i)) >= APPC_LINK_CONNECTED)
        {
            num++;
        }
    }

    return num;
}

uint8_t app_connect_device_info_get(void)
{
    uint8_t num = 0;
    
    bk_printf("\r\n+CHINFO{\r\n");
    for(int i = 0;i < APP_IDX_MAX;i++)
    {     
        bk_printf("idx:%d,state:%x\r\n",i,ke_state_get(KE_BUILD_ID(TASK_APP,i)));        
        if((ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_LINK_CONNECTED) ||(ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_SDP_DISCOVERING)\
            ||(ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_SDP_DISCOVERING) || (ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_SERVICE_CONNECTED) )
        {
            num++;
            for(int j = 0; j < GAP_BD_ADDR_LEN;j ++)
            {
                bk_printf("%02x",app_env.con_dev_addr[i].addr.addr[j]);
            }bk_printf(",%x,3\r\n",app_env.con_dev_addr[i].addr_type); 
        }
    }
    bk_printf("+CHINFO}\r\n\r\nOK\r\n");
    
    return num;
}

uint8_t app_check_connect_by_addr(bd_addr_t addr)
{
    for(int i = 0; i < APP_IDX_MAX; i++)
    {     
        if((ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_LINK_CONNECTED) ||(ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_SDP_DISCOVERING)\
            ||(ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_SDP_DISCOVERING) || (ke_state_get(KE_BUILD_ID(TASK_APP,i)) == APPC_SERVICE_CONNECTED))
        {
            if(!memcmp(app_env.con_dev_addr[i].addr.addr, addr.addr, GAP_BD_ADDR_LEN))
            {
                return 1;
            }
        }
    }
    
    return 0;
}
#endif //(BLE_APP_PRESENT)

/// @} APP
