#include "rwip_config.h"             // SW configuration
#if (BLE_APP_PRESENT)
#include "ke_timer.h"
#include "app.h"
#include "app_task.h"                // Application task Definition
#include "app_scan.h"                     // Application Definition
#include "gap.h"                     // GAP Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API
#include "gattc_task.h"
#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "sdp_service_task.h"
#include "sdp_service.h"
#include "prf_utils.h"
#include "bk_config.h"
//#include "uart.h"
//#include "gpio.h"

/// Application Environment Structure
//struct useapp_env_tag useapp_env;
#if (BLE_CENTRAL || BLE_OBSERVER)

/// Scan interval 
//#define APP_SCAN_INTV         (100)
#define APP_SCAN_INTV       (12) // 12 * 0.625us = 7.5ms
/// Scan window 
//#define APP_SCAN_WD         (20)
#define APP_SCAN_WD         (6) // 6 * 0.625us = 3.75ms


#define APPM_SCAN_TIME_10MS   400

uint16_t BLE_SCAN_INTVALUE = 88;
uint16_t BLE_SCAN_WINDOW = 88;

uint8_t gapm_scan_type = 0;///GAPM_SCAN_
extern uint8_t unknow_str[];

key_str_t scan_fsm_state_str[APP_SCAN_STATE_NUM]=
{
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation.
     {0x0,"APP_SCAN_STATE_IDLE"},
     {0x1,"APP_SCAN_STATE_CREATING"},
     {0x2,"APP_SCAN_STATE_CREATED"},
     {0x3,"APP_SCAN_STATE_STARTING"},
     {0x4,"APP_SCAN_STATE_STARTED"},
     {0x5,"APP_SCAN_STATE_STOPPING"},
};
	
uint8_t *scan_fsm_key2str(uint8_t value)
{
	
	for(int i = 0;i < APP_SCAN_STATE_NUM;i++)
	{
		if(scan_fsm_state_str[i].key == value)
		{
			return scan_fsm_state_str[i].str;
		}
	}
	return unknow_str;
}

static void appm_create_scaning(void)
{
    bk_printf("%s,scan_state:%d\r\n",__func__,app_env.scan_state);
    if (app_env.scan_state == APP_SCAN_STATE_IDLE)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_SCAN_ACTIVITY;

        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
         
        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.scan_state = APP_SCAN_STATE_CREATING;
        // And the next expected operation code for the command completed event
        app_env.scan_op = GAPM_CREATE_SCAN_ACTIVITY;
        
        app_env.scan_intv = APP_SCAN_INTV;
        app_env.scan_wd = APP_SCAN_WD;
    }
}

static void appm_start_scaning(void)
{
    bk_printf("%s,scan_state:%d\r\n",__func__,app_env.scan_state);
    if (app_env.scan_state == APP_SCAN_STATE_CREATED)
    {
        // Prepare the GAPM_ACTIVITY_START_CMD message
        struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                             TASK_GAPM, TASK_APP,
                                                             gapm_activity_start_cmd);

        p_cmd->operation = GAPM_START_SCAN_ACTIVITY;
        p_cmd->actv_idx = app_env.scan_actv_idx;
        
        p_cmd->u_param.scan_param.type = GAPM_SCAN_TYPE_CONN_DISC;//GAPM_SCAN_TYPE_GEN_DISC;//GAPM_SCAN_TYPE_OBSERVER;//;
        
        p_cmd->u_param.scan_param.prop = GAPM_SCAN_PROP_PHY_1M_BIT ;//| GAPM_SCAN_PROP_ACTIVE_1M_BIT;//this passive scan,GAPM_SCAN_PROP_ACTIVE_1M_BIT is active scan
        
        uint8_t scan_param_len = 2;
        if(rwip_param.get(NVDS_TAG_SCAN_INTV, &scan_param_len, (uint8_t*) &app_env.scan_intv) != PARAM_OK)
        {
            bk_printf("not found:NVDS_TAG_SCAN_WD err\r\n");
        }
        if(rwip_param.get(NVDS_TAG_SCAN_WD, &scan_param_len, (uint8_t*) &app_env.scan_wd) != PARAM_OK)
        {
            bk_printf("not found:NVDS_TAG_SCAN_WD err\r\n");
        }
        bk_printf("scan_intv:%d,scan_wd:%d\r\n",app_env.scan_intv,app_env.scan_wd);
        p_cmd->u_param.scan_param.scan_param_1m.scan_intv = app_env.scan_intv;
        p_cmd->u_param.scan_param.scan_param_1m.scan_wd = app_env.scan_wd;
        
        //p_cmd->u_param.scan_param.scan_param_1m.scan_intv = 4;//app_env.scan_intv;
        //p_cmd->u_param.scan_param.scan_param_1m.scan_wd = 4;//app_env.scan_wd;
        
        p_cmd->u_param.scan_param.dup_filt_pol = 0;
        
        p_cmd->u_param.scan_param.duration = 0;
        
        p_cmd->u_param.scan_param.period = 10;

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.scan_state = APP_SCAN_STATE_STARTING;
        // And the next expected operation code for the command completed event
        app_env.scan_op = GAPM_START_SCAN_ACTIVITY;
    }
}

static void appm_stop_scaning(void)
{
    bk_printf("%s,scan_state:%d\r\n",__func__,app_env.scan_state);
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                      TASK_GAPM, TASK_APP,
                                                      gapm_activity_stop_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_SCAN_ACTIVITY;
    cmd->actv_idx = app_env.scan_actv_idx;

    // Send the message
    ke_msg_send(cmd);

    // Update advertising state
    app_env.scan_state = APP_SCAN_STATE_STOPPING;
    // And the next expected operation code for the command completed event
    app_env.scan_op = GAPM_STOP_SCAN_ACTIVITY;
}

void appm_delete_scaning(void)
{
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_SCAN_ACTIVITY;
    
    p_cmd->actv_idx = app_env.scan_actv_idx;

    // Send the message
    ke_msg_send(p_cmd);

    // Keep the current operation
    // And the next expected operation code for the command completed event
    app_env.scan_op = GAPM_DELETE_SCAN_ACTIVITY;
}

void appm_scan_fsm_next(void)
{
    //bk_printf("%s.scan_state(%s)\r\n", __func__, scan_fsm_key2str(app_env.scan_state));
    switch (app_env.scan_state)
    {
        case (APP_SCAN_STATE_IDLE)://0
        {
            // Create advertising
            appm_create_scaning();
        } break;

        case (APP_SCAN_STATE_CREATING):  //1      
        case (APP_SCAN_STATE_CREATED):   //2    
        {
            // Start scaning activity
            appm_start_scaning();
        } break;

        case (APP_SCAN_STATE_STARTING)://3
        {
            // Go to started state
            app_env.scan_state = APP_SCAN_STATE_STARTED;
        } break;

        case (APP_SCAN_STATE_STARTED)://4
        {
            // Stop scaning activity  
             appm_stop_scaning();
        } break;

        case (APP_SCAN_STATE_STOPPING)://5
        {
            // Go created state
            app_env.scan_state = APP_SCAN_STATE_CREATED;
        } break;

        default:
        {
            ASSERT_ERR(0);
        } break;
    }
    //bk_printf("End %s.scan_state(%s)\r\n", __func__, scan_fsm_key2str(app_env.scan_state));
}

void appm_update_scan_state(bool start)
{
    // TODO [LT] - Check current advertising state

    // Start or stop advertising
    appm_scan_fsm_next();
}

uint8_t appm_adv_data_decode_name(uint8_t len,const uint8_t *data,uint8_t *name_str)
{
    uint8_t find = 0;
    uint8_t index;
    for(index = 0; index < len;)
    {
        switch(data[index + 1])
        {
        case GAP_AD_TYPE_FLAGS:
        {
//            UART_PRINTF("AD_TYPE : ");
//            for(uint8_t len = 0; len < data[index] - 1; len++)
//            {
//                UART_PRINTF("%02x ",data[index + 2 + len]);
//            }
//            UART_PRINTF("\r\n");
            index +=(data[index] + 1);
        }
        break;
        case GAP_AD_TYPE_SHORTENED_NAME:
        case GAP_AD_TYPE_COMPLETE_NAME:
        {
//            if(strncmp((char*)&data[index + 2],find_str,str_len) == 0 )
//            {
//                find = 1;
//            }
//            UART_PRINTF("ADV_NAME : ");
            for(uint8_t len = 0; len < data[index] - 1; len++)
            {
               // UART_PRINTF("%c",data[index + 2 + len]);
                *name_str++ = data[index + 2 + len];
            }
//            UART_PRINTF("\r\n");
            index +=(data[index] + 1);
            
            find = 1;
        }
        break;
        case GAP_AD_TYPE_MORE_16_BIT_UUID:
        {
//            UART_PRINTF("UUID : ");
//            for(uint8_t len = 0; len < data[index] - 1;)
//            {
//                UART_PRINTF("%02x%02x  ",data[index + 2 + len],data[index + 3 + len]);
//                len+=2;
//            }
//            UART_PRINTF("\r\n");
            index +=(data[index] + 1);
        }
        break;
        default:
        {
            index +=(data[index] + 1);
        }
        break;
        }
    }
    return find;
}

uint8_t appm_adv_data_decode(uint8_t len, const uint8_t *data, uint8_t *find_str, uint8_t str_len)
{
    uint8_t find = 0;
    uint8_t index;
    
    for(index = 0; index < len;)
    {
        switch(data[index + 1])
        {
            case GAP_AD_TYPE_FLAGS:
            {
                #if 0
                bk_printf("AD_TYPE : ");
                
                uint8_t type_data = data[index + 2];

                os_printf("%02x ",type_data);
                
                if(GAP_LE_LIM_DISCOVERABLE_FLG & type_data)
                {
                    os_printf("| GAP_LE_LIM_DISCOVERABLE_FLG ");     
                }
                if(GAP_LE_GEN_DISCOVERABLE_FLG & type_data)
                {
                    os_printf("| GAP_LE_GEN_DISCOVERABLE_FLG ");     
                }
                if(GAP_BR_EDR_NOT_SUPPORTED & type_data)
                {
                    os_printf("| GAP_BR_EDR_NOT_SUPPORTED ");     
                }
                if(GAP_SIMUL_BR_EDR_LE_CONTROLLER & type_data)
                {
                    os_printf("| GAP_SIMUL_BR_EDR_LE_CONTROLLER ");     
                }
                if(GAP_SIMUL_BR_EDR_LE_HOST & type_data)
                {
                    os_printf("| GAP_SIMUL_BR_EDR_LE_HOST ");     
                }
                os_printf("\r\n");
                #endif
            }
            break;
            
            case GAP_AD_TYPE_SHORTENED_NAME:
            case GAP_AD_TYPE_COMPLETE_NAME:
            {
            	if(find_str && (str_len == (data[index]-1)))
            	{
    				if(strncmp((char*)&data[index + 2], (const char *)find_str, str_len) == 0 )
    				{
    					find = 1;
    				}
            	}
                
                #if 0
                bk_printf("ADV_NAME : ");
                for(uint8_t len = 0; len < data[index]-1; len++)
                {
                    os_printf("%c",data[index + 2 + len]);
                }
                os_printf("\r\n");
                #endif
            }
            break;
            
            case GAP_AD_TYPE_MORE_16_BIT_UUID:
            { 
                #if 0
                bk_printf("UUID : ");
                for(uint8_t len = 0; len < data[index]-1;)
                {
                    os_printf("%02x%02x  ",data[index + 2 + len],data[index + 3 + len]);
                    len+=2;
                }
                os_printf("\r\n");
                #endif
            }
            break;
            
            case GAP_AD_TYPE_APPEARANCE:
            {
                #if 0
                uint16_t appearance = data[index + 3]<<8 | data[index + 2];
    			
            	if((appearance == 961)||(appearance == 962))
            	{
            		find += 1;
            	}

                bk_printf("APPEARANCE : %d\r\n", appearance);
                #endif
            }
            break;
            
            case GAP_AD_TYPE_MANU_SPECIFIC_DATA:
            {
            	if(find_str && (str_len == (data[index] - 1)))
            	{
    				if(strncmp((char*)&data[index + 2], (const char *)find_str, str_len) == 0 )
    				{
    					find = 1;
    				}
            	}
    			#if 0
                bk_printf("MANU : ");
                for(uint8_t len = 0; len < data[index]-1; len++)
                {
                    os_printf("%02x ",data[index + 2 + len]);
                }
                os_printf("\r\n");
                #endif
            }
            break;
            
            default:
            break;
        }
        index += data[index]+1;
    }
    
	if(find >= 1)
		return true;
	else
		return false;
    
}
#endif // (#if (BLE_CENTRAL || BLE_OBSERVER))
#endif  //#if (BLE_APP_PRESENT)

