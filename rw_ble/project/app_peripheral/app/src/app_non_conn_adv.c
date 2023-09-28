#include "rwip_config.h"             // SW configuration
#if (BLE_APP_PRESENT)
#include <string.h>
#include "rwapp_config.h"
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "app_non_conn_adv.h"
#include "gap.h"                     // GAP Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API
#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "ke_timer.h"
#include "app_fcc0.h"                 // Application security Definition
#include "app_dis.h"                 // Device Information Service Application Definitions
#include "app_batt.h"                // Battery Application Definitions
#include "nvds.h"                    // NVDS Definitions
#include "uart.h"




/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
 void appm_create_non_conn_advertising(void)
{
    #if(APP_NON_CONN_ADV_ENABLE==1)
    if (app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_IDLE)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_adv_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;

        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        p_cmd->adv_param.type = GAPM_ADV_TYPE_LEGACY;//GAPM_ADV_TYPE_EXTENDED;//;
        p_cmd->adv_param.prop = GAPM_ADV_PROP_NON_CONN_NON_SCAN_MASK ;//| GAPM_ADV_PROP_SCAN_REQ_NTF_EN_BIT;//GAPM_ADV_PROP_BROADCAST_NON_SCAN_MASK;//GAPM_ADV_PROP_UNDIR_CONN_MASK;
        p_cmd->adv_param.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        p_cmd->adv_param.prim_cfg.chnl_map = APP_NON_CONN_ADV_CHMAP;
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;

        
        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_BEACON;//GAPM_ADV_MODE_GEN_DISC;
        p_cmd->adv_param.prim_cfg.adv_intv_min = APP_NON_CONN_ADV_INT_MIN;
        p_cmd->adv_param.prim_cfg.adv_intv_max = APP_NON_CONN_ADV_INT_MAX;
      

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_CREATING;
        // And the next expected operation code for the command completed event
        app_env.non_conn_adv_op = GAPM_CREATE_ADV_ACTIVITY;
        app_env.non_conn_adv_actv_idx=1;
    }
    #endif
}


void appm_build_non_conn_adv_data(uint16_t max_length, uint16_t *p_length, uint8_t *p_buf)
{
    #if(APP_NON_CONN_ADV_ENABLE==1)
    // Remaining Length
    uint8_t rem_len = max_length;
    uint8_t non_conn_adv_data[21]={0x14,0xFF,0xf0,0x05,0x01,0x71,0x33,0x02,0x00,0x00,0x1B,0xD7,0xBC,0x07,0xDA,0x78,0x00,0x00,0x00,0x00,0x00};
    
    // Sanity check
    ASSERT_ERR(rem_len >= max_length);

    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
    rem_len -= *p_length;

    // Check if additional data can be added to the Advertising data - 2 bytes needed for type and length
    if (rem_len > 2)
    {

 #if 1       
        uint8_t dev_name_length = co_min(app_env.dev_name_len, (rem_len - 2));

        // Device name length
        *p_buf = dev_name_length + 1;
        // Device name flag (check if device name is complete or not)
        *(p_buf + 1) = (dev_name_length == app_env.dev_name_len) ? '\x09' : '\x08';
        // Copy device name
        memcpy(p_buf + 2, app_env.dev_name, dev_name_length);

        // Update advertising data length
        *p_length += (dev_name_length + 2);
 #endif
        *p_length=sizeof(non_conn_adv_data);
        memcpy(p_buf, non_conn_adv_data, sizeof(non_conn_adv_data));
    }
    #endif
}



void appm_start_non_conn_advertising(void)
{
    #if(APP_NON_CONN_ADV_ENABLE==1)
    os_printf("%s,app_env.non_conn_adv_actv_idx=%x\r\n",__func__,app_env.adv_actv_idx);
    if( (app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_CREATED )||(app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_SETTING_ADV_DATA) )
    {
        // Prepare the GAPM_ACTIVITY_START_CMD message
        struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_activity_start_cmd);

        p_cmd->operation = GAPM_START_ACTIVITY;
        p_cmd->actv_idx = app_env.non_conn_adv_actv_idx;
        p_cmd->u_param.adv_add_param.duration = 0;
        p_cmd->u_param.adv_add_param.max_adv_evt = 0;//100;

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_STARTING;
        // And the next expected operation code for the command completed event
        app_env.non_conn_adv_op = GAPM_START_ACTIVITY;
    }
    #endif
}


void appm_stop_non_conn_advertising(void)
{
    #if(APP_NON_CONN_ADV_ENABLE==1)
    os_printf("%s\r\n",__func__);
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                      TASK_GAPM, TASK_APP,
                                                      gapm_activity_stop_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_ACTIVITY;
    cmd->actv_idx = app_env.non_conn_adv_actv_idx;

    // Send the message
    ke_msg_send(cmd);

    // Update advertising state
    app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_STOPPING;
    // And the next expected operation code for the command completed event
    app_env.non_conn_adv_op = GAPM_STOP_ACTIVITY;
    #endif
}

void appm_delete_non_conn_advertising(void)
{
    #if(APP_NON_CONN_ADV_ENABLE==1)
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_ACTIVITY;
    p_cmd->actv_idx = app_env.non_conn_adv_actv_idx;
    // Send the message
    ke_msg_send(p_cmd);
    
    app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_DELETE;
    // Keep the current operation
    // And the next expected operation code for the command completed event
    app_env.non_conn_adv_op = GAPM_DELETE_ACTIVITY;
    #endif
}


void appm_set_non_conn_adv_data(void)
{
    #if(APP_NON_CONN_ADV_ENABLE==1)
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_ADV_DATA;
    p_cmd->actv_idx = app_env.non_conn_adv_actv_idx;


    p_cmd->length = 0;
    // GAP will use 3 bytes for the AD Type
    appm_build_non_conn_adv_data(ADV_DATA_LEN, &p_cmd->length, &p_cmd->data[0]);
    
    os_printf("adv_data=");
    for(int i = 0;i < p_cmd->length;i++)
    {
        os_printf("%02x",p_cmd->data[i]);
    }os_printf("\r\n");
    

    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_SETTING_ADV_DATA;
    // And the next expected operation code for the command completed event
    app_env.non_conn_adv_op = GAPM_SET_ADV_DATA;
    #endif
}


uint8_t appm_updata_non_conn_adv_data(uint16_t length, uint8_t *p_data)
{
    uint8_t status = GAP_ERR_INVALID_PARAM;
    #if(APP_NON_CONN_ADV_ENABLE==1)
    if((app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_CREATED) || (app_env.non_conn_adv_state == APP_NON_CONN_ADV_STATE_STARTED))
    {
         // Prepare the GAPM_SET_ADV_DATA_CMD message
        struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                               TASK_GAPM, TASK_APP,
                                                               gapm_set_adv_data_cmd,
                                                               ADV_DATA_LEN);

        // Fill the allocated kernel message
        p_cmd->operation = GAPM_SET_ADV_DATA;
        p_cmd->actv_idx = app_env.non_conn_adv_actv_idx;

    
        p_cmd->length = 0;

            // Remaining Length
        uint8_t rem_len = ADV_DATA_LEN;

        // Check if additional data can be added to the Advertising data - 2 bytes needed for type and length
        if (rem_len >=length)
        {
            // Copy adv data
            memcpy(&p_cmd->data[0], p_data, length);
            // Update advertising data length
            p_cmd->length = length;
                        
        }
        
        os_printf("adv data = ");
        for(int i = 0;i < p_cmd->length;i++)
        {
            os_printf("%02x",p_cmd->data[i]);
        }os_printf("\r\n");

        // Send the message
        ke_msg_send(p_cmd);

        // Update advertising state
        app_env.non_conn_adv_state = APP_NON_CONN_ADV_STATE_UPDATA_ADV_DATA;
        
        status = GAP_ERR_NO_ERROR;
    }
    else
    {
        status = GAP_ERR_COMMAND_DISALLOWED;
    }
    #endif
    return status;
    
}


#endif //


