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
 * Copyright (C) Beken 2019-2022
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
#if (BLE_APP_FEE0S)
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>
#include "ke_task.h"                 // Kernel
#include "app_fee0.h"              // Battery Application Module Definitions
#include "app.h"                   // Application Definitions
#include "app_task.h"              // application task definitions
#include "fee0s_task.h"            // health thermometer functions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"             // Profile common types definition
#include "arch.h"                  // Platform Definitions
#include "prf.h"
#include "fee0s.h"
#include "ke_timer.h"
//#include "uart.h"
#if (BLE_SDP_CLIENT)
#include "sdp_service.h"
#endif
#include "bk_config.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// fff0 Application Module Environment Structure
struct app_fee0_env_tag app_fee0_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */


void app_fee0_init(void)
{

    // Reset the environment
    memset(&app_fee0_env, 0, sizeof(struct app_fee0_env_tag));
}

void app_fee0_add_fee0s(void)
{

   struct fee0s_db_cfg *db_cfg;
		
   struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct fee0s_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl =   0;
    req->prf_task_id = TASK_ID_FEE0S;
    req->app_task = TASK_APP;
    req->start_hdl = 0; //req->start_hdl = 0; dynamically allocated

	 
    // Set parameters
    db_cfg = (struct fee0s_db_cfg* ) req->param;
   
    db_cfg->fee1_desc_len = strlen("fee1_only_read");
   
    memcpy(db_cfg->fee1_desc,"fee1_only_read",strlen("fee1_only_read"));
	 
    // Sending of notifications is supported
    // Send the message
    ke_msg_send(req);
}


uint8_t app_fee4_send_ntf(uint8_t conidx,uint16_t len,uint8_t* buf)
{
    bk_printf("%s:len:%d,state:0x%x\r\n",__func__,len,ke_state_get(KE_BUILD_ID(TASK_APP,conidx)));
    uint8_t ret = APPM_ERROR_NO_ERROR;
    if((ke_state_get(KE_BUILD_ID(TASK_APP,conidx)) == APPC_LINK_CONNECTED))
    {
        if(app_fee0_env.ntf_cfg[conidx] != PRF_CLI_STOP_NTFIND)
        {
            // Allocate the message
        struct fee0s_fee45_val_upd_req * req = KE_MSG_ALLOC(FEE0S_FEE4_VALUE_UPD_REQ,
                                                            prf_get_task_from_id(TASK_ID_FEE0S),
                                                            KE_BUILD_ID(TASK_APP, conidx),
                                                            fee0s_fee45_val_upd_req);
        // Fill in the parameter structure	
        req->length = len;
        memcpy(req->value, buf, len);

        // Send the message
        ke_msg_send(req);
        }else
        {
            ret = APPM_ERROR_NTFIND_DISABLE;
        }
    }
    else
    {
        ret = APPM_ERROR_STATE;
    }

    return ret;
}


uint8_t  app_fee5_send_ind(uint8_t conidx,uint16_t len,uint8_t* buf)
{
    
    uint8_t ret = APPM_ERROR_NO_ERROR;
    // Allocate the message
    struct fee0s_fee45_val_upd_req * req = KE_MSG_ALLOC(FEE0S_FEE5_VALUE_UPD_REQ,
                                                        prf_get_task_from_id(TASK_ID_FEE0S),
                                                        KE_BUILD_ID(TASK_APP, conidx),
                                                        fee0s_fee45_val_upd_req);
    // Fill in the parameter structure	
    req->length = len;
	memcpy(req->value, buf, len);

    // Send the message
    ke_msg_send(req);
    
    return ret;
}



static int fee0s_fee4_val_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct fee0s_fee4_val_ntf_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
	bk_printf("fee4->param->ntf_cfg = %x\r\n",param->ntf_cfg);
    uint8_t conidx = KE_IDX_GET(src_id);
    app_fee0_env.ntf_cfg[conidx] = param->ntf_cfg;
	if(param->ntf_cfg == PRF_CLI_STOP_NTFIND)
	{
		//ke_timer_clear(FEE0S_FEE1_LEVEL_PERIOD_NTF,dest_id);
	}else
	{
		//ke_timer_set(FEE0S_FEE1_LEVEL_PERIOD_NTF,dest_id , 100);
	}
    
    return (KE_MSG_CONSUMED);
}

static int fee0s_fee5_val_ind_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct fee0s_fee5_val_ind_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
	bk_printf("fee5->param->ind_cfg = %x\r\n",param->ind_cfg);
    uint8_t conidx = KE_IDX_GET(src_id);
    app_fee0_env.ind_cfg[conidx] = param->ind_cfg;
    
	if(param->ind_cfg == PRF_CLI_STOP_NTFIND)
	{
		//ke_timer_clear(FEE0S_FEE3_LEVEL_PERIOD_NTF,dest_id);
	}else
	{
		//ke_timer_set(FEE0S_FEE3_LEVEL_PERIOD_NTF,dest_id , 100);
	}
    
    return (KE_MSG_CONSUMED);
}
static int fee4_val_upd_rsp_handler(ke_msg_id_t const msgid,
                                      struct fee0s_fee45_val_upd_rsp const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	bk_printf("%s,status:%x\r\n", __func__,param->status);
	
	if(param->status == GAP_ERR_NO_ERROR)
	{
		
	}
	
    return (KE_MSG_CONSUMED);
}

static int fee5_val_upd_rsp_handler(ke_msg_id_t const msgid,
                                      struct fee0s_fee45_val_upd_rsp const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
	bk_printf("%s,status:%x\r\n", __func__,param->status);
	
	if(param->status == GAP_ERR_NO_ERROR)
	{
		
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
static int app_fee0_msg_dflt_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
	bk_printf("%s\r\n", __func__);
	
    // Drop the message
    return (KE_MSG_CONSUMED);
}

uint8_t first_flag = 0;
//static uint32_t  basetime_cnt,finetime_cnt,basetime_cnt1,finetime_cnt1;

uint8_t test_buf[1000];
static int fee2_writer_cmd_handler(ke_msg_id_t const msgid,
                                     struct fee0s_fee23_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{   
    //uint8_t conidx = KE_IDX_GET(src_id);
   
    // Drop the message
	bk_printf("FEE2 conidx:%d,length:%d,param->value = 0x ",param->conidx,param->length);
	
	for(uint16_t i = 0;i < param->length;i++)
	{
		bk_printf("%02x ",param->value[i]);
	}
    
	bk_printf("\r\n");


    return (KE_MSG_CONSUMED);
}

static int fee3_writer_req_handler(ke_msg_id_t const msgid,
                                     struct fee0s_fee23_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Drop the message
	bk_printf("FEE3 conidx:%d,length:%d,param->value = 0x ",param->conidx,param->length);

	for(uint16_t i = 0;i < param->length;i++)
	{
		bk_printf("%02x ",param->value[i]);
	}
	bk_printf("\r\n");
    return (KE_MSG_CONSUMED);   
   
}


#if (BLE_SDP_CLIENT)
uint8_t app_add_sdp_fee0_srv(uint8_t conidx,uint8_t uuid_len,struct gattc_sdp_svc_ind const *ind)
{
    struct prf_sdp_db_env *prf_db_env = NULL;
    uint8_t stus = 1;
    uint16_t uuid = 0xfee0;
    uint16_t uuid1 = 0xfee3;
    bk_printf("%s\r\n",__func__);
    bk_printf("ind->uuid:%02x,%02x\r\n",ind->uuid[1],ind->uuid[0]);
    if((memcmp((uint8_t*)&uuid,ind->uuid,uuid_len) == 0))   
    {   
        prf_db_env = sdp_extract_svc_info(conidx,ind);
         
        if(prf_db_env != NULL)
        {
            bk_printf("char nb:%d\r\n",prf_db_env->sdp_cont->chars_nb);
            uuid = 0xfee2;
            uuid1 = 0xfee3;
            for(int i = 0;i < prf_db_env->sdp_cont->chars_nb;i++)
            {
                uint16_t val_hdl = prf_db_env->sdp_cont->chars_descs_inf.chars_inf[i].val_hdl;
                if((memcmp((uint8_t*)&uuid,prf_db_env->sdp_cont->chars_descs_inf.chars_inf[i].uuid,2) == 0) )
                {
                    bk_printf("svc_fee2_write_handle[%d]:%d\r\n",conidx,prf_db_env->sdp_cont->chars_descs_inf.chars_inf[i].val_hdl);
                    app_fee0_env.svc_fee2_write_handle[conidx] = val_hdl;
                }
                if((memcmp((uint8_t*)&uuid1,prf_db_env->sdp_cont->chars_descs_inf.chars_inf[i].uuid,2) == 0) )
                {
                    bk_printf("svc_fee3_write_handle[%d]:%d\r\n",conidx,prf_db_env->sdp_cont->chars_descs_inf.chars_inf[i].val_hdl);
                    app_fee0_env.svc_fee3_write_handle[conidx] = val_hdl;
                }
                
            }                                   
            sdp_add_profiles(conidx,prf_db_env);
            stus = 0;
        }     
    
    }
    return  stus;
}
#endif

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_fee0_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_fee0_msg_dflt_handler},
    {FEE0S_FEE4_VALUE_NTF_CFG_IND,  (ke_msg_func_t)fee0s_fee4_val_ntf_cfg_ind_handler},
    {FEE0S_FEE5_VALUE_IND_CFG_IND,  (ke_msg_func_t)fee0s_fee5_val_ind_cfg_ind_handler},
    {FEE0S_FEE4_VALUE_UPD_RSP,      (ke_msg_func_t)fee4_val_upd_rsp_handler},
    {FEE0S_FEE5_VALUE_UPD_RSP,      (ke_msg_func_t)fee5_val_upd_rsp_handler},
    {FEE0S_FEE2_WRITER_CMD_IND,		(ke_msg_func_t)fee2_writer_cmd_handler},
    {FEE0S_FEE3_WRITER_REQ_IND,		(ke_msg_func_t)fee3_writer_req_handler},
    
};

const struct app_subtask_handlers app_fee0_handler = APP_HANDLERS(app_fee0);

#endif // BLE_APP_FEE0S

