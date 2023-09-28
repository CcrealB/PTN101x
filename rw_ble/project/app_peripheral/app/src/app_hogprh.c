/**
 ****************************************************************************************
 *
 * @file app_hid.c
 *
 * @brief HID Application Module entry point
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

#include "rwip_config.h"            // SW configuration

#include <stdio.h>
#include <string.h>

#if (BLE_APP_HOGPRH)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app.h"                    // Application Definitions
#include "app_sec.h"                // Application Security Module API
#include "app_task.h"               // Application task definitions
#include "app_hogprh.h"             // HOGPRH Application Module Definitions
#include "hogprh_task.h"            // HOGPRH Over GATT Profile Device Role Functions
#include "prf_types.h"              // Profile common types Definition
#include "arch.h"                    // Platform Definitions
#include "prf.h"
#include "ke_timer.h"

#if (NVDS_SUPPORT)
#include "nvds.h"                   // NVDS Definitions
#endif //(NVDS_SUPPORT)

#if (DISPLAY_SUPPORT)
#include "app_display.h"            // Application Display Module
#endif //(DISPLAY_SUPPORT)

#include "co_utils.h"               // Common functions

#if (KE_PROFILING)
#include "ke_mem.h"
#endif //(KE_PROFILING)
#include "bk_config.h"

/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// States of the Application HOGP Module
enum app_hogprh_states
{
    /// Module is disabled (Service not added in DB)
    APP_HOGPRH_DISABLED,
    /// Module is idle (Service added but profile not enabled)
    APP_HOGPRH_IDLE,
    /// Module is enabled (Device is connected and the profile is enabled)
    APP_HOGPRH_ENABLED,
    /// The application can send reports
    APP_HOGPRH_READY,
    /// Waiting for a report
    APP_HOGPRH_WAIT_REP,

    APP_HOGPRH_STATE_MAX,
};

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// HID Application Module Environment Structure
static struct app_hogprh_env_tag app_hogprh_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_hogprh_init(void)
{
    // Reset the environment
    memset(&app_hogprh_env, 0, sizeof(app_hogprh_env));

}

void app_hogprh_add_hogprhs(void)
{
    // Prepare the HOGPD_CREATE_DB_REQ message
    struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                   TASK_GAPM, TASK_APP,
                                                   gapm_profile_task_add_cmd, 0);

    // Fill message
    req->operation   = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl     = PERM(SVC_AUTH, AUTH);
    req->prf_task_id = TASK_ID_HOGPRH;
    req->app_task    = TASK_APP;
    req->start_hdl   = 0;

    // Send the message
    ke_msg_send(req);
}


/*
 ****************************************************************************************
 * @brief Function called when get GAP manager command complete events.
 *
 ****************************************************************************************
 */


/*
 ****************************************************************************************
 * @brief Function called when get connection complete event from the GAP
 *
 ****************************************************************************************
 */
void app_hogprh_enable_prf(uint8_t conidx)
{
	bk_printf("%s\r\n", __func__);

    // Store the connection handle
    app_hogprh_env.conidx = conidx;

    // Allocate the message
    struct hogprh_enable_req * req = KE_MSG_ALLOC(HOGPRH_ENABLE_REQ,
										 prf_get_task_from_id(KE_BUILD_ID(TASK_ID_HOGPRH, conidx)),
										 KE_BUILD_ID(TASK_APP, conidx),
										 hogprh_enable_req);

    // Fill message
    req->con_type = PRF_CON_DISCOVERY;
    req->hids_nb  = 0;
    memset(&req->hids[0], 0, sizeof(struct hogprh_content) * HOGPRH_NB_HIDS_INST_MAX);

    // Go to Enabled state
    app_hogprh_env.state = APP_HOGPRH_ENABLED;

    // Send the message
    ke_msg_send(req);
}

void app_hogprh_send_enable_report(uint8_t conidx, uint8_t hid_idx, uint8_t report_idx)
{
	//bk_printf("%s, conidx:%d, hid_idx:%d, report_idx:%d\r\n", __func__, conidx, hid_idx, report_idx);
	struct hogprh_write_req * req = KE_MSG_ALLOC(HOGPRH_WRITE_REQ,
								 prf_get_task_from_id(KE_BUILD_ID(TASK_ID_HOGPRH, conidx)),
								 KE_BUILD_ID(TASK_APP, conidx),
								 hogprh_write_req);

	// Fill message
	req->info            = HOGPRH_REPORT_NTF_CFG;
	req->hid_idx         = hid_idx;
	req->report_idx      = report_idx;
	req->data.report_cfg = 1; // enable notification

	// Send the message
	ke_msg_send(req);
}

void app_hogprh_read_report_map(uint8_t conidx, uint8_t hid_idx)
{
	bk_printf("%s, conidx:%d, hid_idx:%d\r\n", __func__, conidx, hid_idx);
	struct hogprh_read_info_req * req = KE_MSG_ALLOC(HOGPRH_READ_INFO_REQ,
								 prf_get_task_from_id(KE_BUILD_ID(TASK_ID_HOGPRH, conidx)),
								 KE_BUILD_ID(TASK_APP, conidx),
								 hogprh_read_info_req);

	// Fill message
	req->info       = HOGPRH_REPORT_MAP;
	req->hid_idx    = hid_idx;
	req->report_idx = 0;

	// Send the message
	ke_msg_send(req);
}


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

static int hogprh_enable_rsp_handler(ke_msg_id_t const msgid,
                                     struct hogprh_enable_rsp const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
	uint8_t conidx = KE_IDX_GET(dest_id);
	uint8_t status = param->status;

	if(status == GAP_ERR_NO_ERROR)
	{
		// enable OK, can start normal connection to TH

		app_hogprh_env.hids_nb[conidx] = param->hids_nb;
		app_hogprh_read_report_map(conidx, 0);

		// start enable report
		//app_hogprh_send_enable_report(conidx, 0, 0);
	}
	else
	{
		// enable error
		bk_printf("hogprh_enable error:0x%x\r\n", status);

	}


    return (KE_MSG_CONSUMED);
}

static int hogprh_read_info_handler(ke_msg_id_t const msgid,
                                     struct hogprh_read_info_rsp const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
	uint8_t conidx = KE_IDX_GET(dest_id);
	uint8_t status = param->status;

	if(param->info == HOGPRH_REPORT_MAP)
	{
		uint8_t hid_idx = param->hid_idx;

		if(status == GAP_ERR_NO_ERROR)
		{
			// dump report map
			uint16_t map_len = param->data.report_map.length;
			uint8_t *map_val = (uint8_t *)&param->data.report_map.value[0];

			bk_printf("HID%d report map len:%d\r\n", hid_idx, map_len);
			for(uint16_t i=0; i<map_len; i++)
				bk_printf("%02x ", map_val[i]);
			bk_printf("\r\n");

		}
		else
		{
			// error
			bk_printf("read report map error:0x%x\r\n", status);

		}

		// check to read next report map
		if(hid_idx < app_hogprh_env.hids_nb[conidx]-1)
		{
			hid_idx ++;
			app_hogprh_read_report_map(conidx, hid_idx);
		}
		else
		{
			// start enable report
			app_hogprh_send_enable_report(conidx, 0, 0);
		}
	}

    return (KE_MSG_CONSUMED);
}

static int hogprh_write_rsp_handler(ke_msg_id_t const msgid,
                                     struct hogprh_write_rsp const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
	uint8_t conidx = KE_IDX_GET(dest_id);
	uint8_t status = param->status;

	if(param->info == HOGPRH_REPORT_NTF_CFG)
	{
		if(status == GAP_ERR_NO_ERROR)
		{
			bk_printf("HID %d index%d report enable OK\r\n",
					param->hid_idx, HOGPRH_DESC_REPORT_CFG+param->report_idx);

		}
		else
		{
			// error
			bk_printf("hogprh_write error:0x%x\r\n", status);

		}

		uint8_t hid_idx    = param->hid_idx;
		uint8_t report_idx = param->report_idx;

		// check to enable next report
		if((report_idx < 0x03) || (hid_idx < app_hogprh_env.hids_nb[conidx]-1))
		{
			if(report_idx==0x00)
			{
				report_idx = 0x01; // enable mouse in
			}
			else if(report_idx==0x01)
			{
				report_idx = 0x03;
			}
			else if(report_idx==0x03)
			{
				hid_idx ++;
				report_idx = 0x00; // enable keyboard in
			}
			app_hogprh_send_enable_report(conidx, hid_idx, report_idx);
		}
		else
		{
			bk_printf("All report configuration is enabled\r\n");
		}

	}


    return (KE_MSG_CONSUMED);
}

//extern void USBD_MouseStartTx(unsigned char *pBuf, unsigned long ulLen);
//extern void USBD_KeyboardStartTx(unsigned char *pBuf, unsigned long ulLen);

static int hogprh_boot_report_ind_handler(ke_msg_id_t const msgid,
                                     struct hogprh_report_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Get the address of the environment
    //struct hogprh_env_tag *hogprh_env = PRF_ENV_GET(HOGPRH, hogprh);

    //uint8_t  conidx      = KE_IDX_GET(dest_id);
	//uint8_t  hid_idx     = param->hid_idx;
	uint8_t  report_idx  = param->report_idx;
	uint32_t report_len  = param->report.length;
	uint8_t *report_data = (uint8_t *)&param->report.value[0];
	struct send_report_data_tag report;

#if 0
	bk_printf("HID#%d, idx:%d, len:%d\r\n", hid_idx, report_idx, report_len);
	for(uint8_t i=0; i<report_len; i++)
		bk_printf("%02x ", report_data[i]);
	bk_printf("\r\n");
#endif

	// check USB is connected
	if(1)//is_set_usb_tx_flag(0x01))
	{
		// send report to USB
		if(report_idx == 0x00) // 0x00 for keyboard
		{
			report.report_id = 1;
			memcpy(report.data, report_data, report_len);

			//USBD_KeyboardStartTx((uint8_t*)&report, report_len+1);
		}
		else if(report_idx == 0x01) // 0x01 for mouse
		{
			report.report_id = 2;
			memcpy(report.data, report_data, report_len);

			//USBD_MouseStartTx((uint8_t*)&report, report_len+1);
		}
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
static int app_hogprh_msg_dflt_handler(ke_msg_id_t const msgid,
                                    void const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Set the value of the Report Map Characteristic in the database
 ****************************************************************************************
 */

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_hogprh_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_hogprh_msg_dflt_handler},

    {HOGPRH_ENABLE_RSP,             (ke_msg_func_t)hogprh_enable_rsp_handler},
    {HOGPRH_READ_INFO_RSP,          (ke_msg_func_t)hogprh_read_info_handler},
    {HOGPRH_WRITE_RSP,              (ke_msg_func_t)hogprh_write_rsp_handler},
    {HOGPRH_REPORT_IND,             (ke_msg_func_t)hogprh_boot_report_ind_handler},

};

const struct app_subtask_handlers app_hogprh_handlers = APP_HANDLERS(app_hogprh);

#endif //(BLE_APP_HOGPRH)

/// @} APP
