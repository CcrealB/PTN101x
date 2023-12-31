/**
 ****************************************************************************************
 *
 * @file app_fff0.c
 *
 * @brief fff0 Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2016.05.31
 *
 * Copyright (C) Beken 2009-2016
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

#include "rwip_config.h"     // SW configuration

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>
#include "ke_task.h"                 // Kernel
#include "app_fff0.h"              // Battery Application Module Definitions
#include "app.h"                    // Application Definitions
#include "app_task.h"             // application task definitions
#include "fff0s_task.h"           // health thermometer functions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"             // Profile common types definition
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "fff0s.h"
#include "ke_timer.h"
//#include "uart.h"
#include "bk_config.h"

#if (BLE_APP_FFF0S)
/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// fff0 Application Module Environment Structure
struct app_fff0_env_tag app_fff0_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


void app_fff0_init(void)
{
    // Reset the environment
    memset(&app_fff0_env, 0, sizeof(struct app_fff0_env_tag));
}

void app_fff0_add_fff0s(void)
{

   struct fff0s_db_cfg *db_cfg;
		
   struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct fff0s_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl =   0;
    req->prf_task_id = TASK_ID_FFF0S;
    req->app_task = TASK_APP;
    req->start_hdl = 0; //req->start_hdl = 0; dynamically allocated

	 
    // Set parameters
    db_cfg = (struct fff0s_db_cfg* ) req->param;
	 
    // Sending of notifications is supported
    db_cfg->features = FFF0_FFF1_LVL_NTF_SUP;
    // Send the message
    ke_msg_send(req);
}


void app_fff1_send_lvl(uint8_t* buf, uint8_t len)
{
    #ifndef CONFIG_USE_BLE
    bk_printf("%s:len:%d,state:%x\r\n",__func__,len, app_fff0_env.send_status);
    #endif
    if(app_fff0_env.send_status==0)
    {
        app_fff0_env.send_status=1;
        // Allocate the message
        struct fff0s_fff1_level_upd_req * req = KE_MSG_ALLOC(FFF0S_FFF1_LEVEL_UPD_REQ,
                                                            prf_get_task_from_id(TASK_ID_FFF0S),
                                                            TASK_APP,
                                                            fff0s_fff1_level_upd_req);
        // Fill in the parameter structure	
        req->length = len;
    	memcpy(req->fff1_level, buf, len);

        // Send the message
        ke_msg_send(req);
    }
}


static int fff0s_fff1_level_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct fff0s_fff1_level_ntf_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
	bk_printf("param->ntf_cfg = %x\r\n",param->ntf_cfg);
	if(param->ntf_cfg == PRF_CLI_STOP_NTFIND)
	{
		//ke_timer_clear(FFF0S_FFF1_LEVEL_PERIOD_NTF,dest_id);
	}else
	{
		//ke_timer_set(FFF0S_FFF1_LEVEL_PERIOD_NTF,dest_id , 1);
	}
    
    return (KE_MSG_CONSUMED);
}

static int fff1_level_upd_handler(ke_msg_id_t const msgid,
                                      struct fff0s_fff1_level_upd_rsp const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	if(param->status == GAP_ERR_NO_ERROR)
	{
		//uint8_t buf[128];
		//memset(buf, 0xcc, 128);
		//app_fff1_send_lvl(buf, 128);
		app_fff0_env.send_status=0;
	}
	
    return (KE_MSG_CONSUMED);
}



/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_fff0_msg_dflt_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
	bk_printf("%s\r\n", __func__);
	
    // Drop the message
    return (KE_MSG_CONSUMED);
}


static int fff2_writer_req_handler(ke_msg_id_t const msgid,
                                     struct fff0s_fff2_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    #ifdef CONFIG_USE_BLE
    extern void com_ble_recv(uint8_t *buf, uint8_t size);
    com_ble_recv((uint8_t*)param->fff2_value, param->length);
    #else
    // Drop the message
	bk_printf("FFF2 param->length = %d\n", param->length);
	bk_printf("FFF2 param->value = 0x ");
	
	for(uint8_t i = 0;i < param->length;i++)
	{
		bk_printf("%02x ",param->fff2_value[i]);
	}
	bk_printf("\r\n");
	#endif
    return (KE_MSG_CONSUMED);
}


static int fff1_period_ntf_handler(ke_msg_id_t const msgid,
                                               struct fff0s_fff1_level_ntf_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
	uint8_t buf[128];
	memset(buf, 0xff, 128);
	app_fff1_send_lvl(buf, 128);
    ke_timer_set(FFF0S_FFF1_LEVEL_PERIOD_NTF,TASK_APP , 2);
		
    return (KE_MSG_CONSUMED);
}



/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_fff0_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_fff0_msg_dflt_handler},
    {FFF0S_FFF1_LEVEL_NTF_CFG_IND,  (ke_msg_func_t)fff0s_fff1_level_ntf_cfg_ind_handler},
    {FFF0S_FFF1_LEVEL_UPD_RSP,      (ke_msg_func_t)fff1_level_upd_handler},
    {FFF0S_FFF2_WRITER_REQ_IND,		(ke_msg_func_t)fff2_writer_req_handler},
    {FFF0S_FFF1_LEVEL_PERIOD_NTF,	(ke_msg_func_t)fff1_period_ntf_handler},
};

const struct app_subtask_handlers app_fff0_handler = APP_HANDLERS(app_fff0);
#endif
