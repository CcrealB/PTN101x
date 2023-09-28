/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <bluetooth.h>
#include <bt_api.h>
#include <bt_goep.h>
#include <bt_pbap.h>
#include <bt_l2cap_proto.h>
#include <bt_sdp.h>
#include "bt_app_internal.h"
#include "bt_rfcomm_channel.h"
//#include "bt_sco_backend_utils.h" //???
//#include "bluetooth/service/sdp_client.h"
#include "app_beken_includes.h"
#include "driver_beken_includes.h"


#if CONFIG_BLUETOOTH_PBAP

/* PBAP Service-level Notifications */
static void pbap_rfcomm_connected(bt_pbap_session_h session, bt_app_ctx_h app_ctx);
static void pbap_obex_connected(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result);
static void pbap_setpath_cfm(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result);
static void pbap_vl_start(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);
static void pbap_vl_data(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);
static void pbap_vl_complete(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result);
static void pbap_ve_start(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);
static void pbap_ve_data(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);
static void pbap_ve_complete(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result);
static void pbap_pb_start(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);
static void pbap_pb_data(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData);
static void pbap_pb_complete(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result);
static void pbap_disconnected(bt_pbap_session_h session, result_t status, bt_app_ctx_h app_ctx);
static void VcardList(const uint8_t *ptr, uint16_t dataLen);

#ifdef EXTRACT_VCARD
static void extractVcard(const uint8_t *ptr, uint16_t dataLen);
static uint16_t len = 0;
static uint8_t vcardNameBuf[MAX_NAME_LEN] = {0};
static uint8_t vcardTelBuf[MAX_TEL_LEN] = {0};
char last_vCardTel[32] = {0};
#endif

static bt_pbap_conn_cbs_t pbap_conn_cbs = {
    pbap_rfcomm_connected,
    pbap_obex_connected,
    pbap_setpath_cfm,
    pbap_vl_start,
    pbap_vl_data,
    pbap_vl_complete,
    pbap_ve_start,
    pbap_ve_data,
    pbap_ve_complete,
    pbap_pb_start,
    pbap_pb_data,
    pbap_pb_complete,
    pbap_disconnected
};


static void pbap_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
    const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_server_ctx);

void pbap_input(bt_pbap_session_h session, at_result_t *at_result,
    bt_app_ctx_h app_ctx);

void pbap_get_features(bt_pbap_session_h session_h,
    uint16_t *supported_features, bt_app_ctx_h app_ctx);

static const bt_pbap_cbs_t pbap_cbs = {
    pbap_input,
    pbap_get_features
};

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
static result_t pbap_cmd_connect(char *params, unsigned int len);
static result_t pbap_cmd_disconnect(char *params, unsigned int len);

static bt_app_cmd_t pbap_cmds[] = {
    {"connect",
        "connect (unit) (raddr) (channel)",
        "Connect to a remote PBAP device.\n"
		"(channel) - The RFCOMM channel is obtained in the SDAP "
                "search notification.\n"
		"(see 'sdap' help)\n"
		"Example: pce connect 0 11:22:33:44:55:66 13",
        pbap_cmd_connect},
    {"disconnect",
        "disconnect (#service)",
        "Disconnect an active PBAP connection.\n"
		"(#service) is retrieved from a successful connection "
                "notification:\n"
		"[pce connected <#service>]\n"
		"Example: pce disconnect 1",
        pbap_cmd_disconnect},
};

static bt_app_section_t pbap_section = {
    "pce",
    "PBAP backend",
    pbap_cmds,
    UW_ARRAY_SIZE(pbap_cmds)
};
#endif

//disconn -> ready
typedef enum {
	pbap_initializing,
	pbap_ready,
	pbap_Connecting,
	pbap_Connected,
	pbap_busy,
	pbap_disconnecting
}pbap_state_t;

typedef struct {
    uint32_t                    svc_id;
    bt_pbap_server_h          server;
    bt_pbap_session_h         session;

    btaddr_t                    laddr;
    btaddr_t                    raddr;
    //bt_conn_req_h               conn_req;

    BOOL                      is_used;
    uint8_t						action;
    pbap_state_t				pbap_state;
	BOOL						newNameFlag;
	BOOL						newTelFlag;
	BOOL						newLineFlag;
	uint16_t					nameBufPos;
	uint16_t					telBufPos;
	uint16_t					numOfVcard;
    BOOL                        setPathAlready;
    uint8_t                     remote_supported_repositories;
    uint32_t                    remote_supported_features;
} pbap_app_t;

/* XXX: Currently, only single service is supported by this sample */
static pbap_app_t g_pbap_app[2];

uint8_t is_PBAP_ready()
{
    if(g_pbap_app[0].pbap_state == pbap_ready)
    {
        return 0x01;
    }else if(g_pbap_app[1].pbap_state == pbap_ready)
    {
        return 0x01;
    }else
    {
        return 0x00;
    }
}

static void setPbapStatus(pbap_app_t *app, pbap_state_t state)
{
	LOG_I(PBAP,"PBAP state %d --> %d %x\n", app->pbap_state, state, (int)app);
	app->pbap_state = state;
}

static pbap_app_t *getFreePbapApp(void)
{
	if (g_pbap_app[0].pbap_state == pbap_ready)
	{
		return &g_pbap_app[0];
	}
	else if (g_pbap_app[1].pbap_state == pbap_ready)
	{
		return &g_pbap_app[1];
	}
	else
	{
	    LOG_E(PBAP, "no free pbap session %x %x\r\n", g_pbap_app[0].pbap_state, g_pbap_app[1].pbap_state);
		return NULL;
	}
}

static pbap_app_t *getPbapAppFromBtaddr(btaddr_t *raddr)
{
	if (j_memcmp((void*)&g_pbap_app[0].raddr, (void*)raddr, sizeof(btaddr_t)) == 0)
	{
		return &g_pbap_app[0];
	}
	else if (j_memcmp((void*)&g_pbap_app[1].raddr, (void*)raddr, sizeof(btaddr_t)) == 0)
	{
		return &g_pbap_app[1];
	}
	else
	{
		return NULL;
	}
}

static pbap_app_t *getPbapAppFromSession(bt_pbap_session_h session)
{
	if (g_pbap_app[0].session == session)
		return &g_pbap_app[0];
	else if (g_pbap_app[1].session == session)
		return &g_pbap_app[1];
	else
		return NULL;
}

static void pbap_rfcomm_connected(bt_pbap_session_h session, bt_app_ctx_h app_ctx)
{
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap rfcomm connected %lu", app->svc_id);
    #endif
    LOG_I(PBAP, "pbap rfcomm connected\r\n");
}

static void pbap_obex_connected(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result)
{
    pbap_app_t *app = getPbapAppFromSession(session);
    setPbapStatus(app, pbap_Connected);
    LOG_I(PBAP, "pbap obex connected\r\n");
    #if 0
    bt_frontend_notification("pbap obex connected %lu", app->svc_id);
    uint8_t deviceId = app_env_get_index_from_addr(app->raddr);
    pbapConnectionStatusInd(deviceId, BK_SLC_CONNECTED);
    #endif
}

static void pbap_setpath_cfm(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result)
{
    LOG_I(PBPA, "set path cfm\r\n");
    pbap_app_t *app = getPbapAppFromSession(session);
    app->setPathAlready = 1;
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap set path %lu", app->svc_id);
    #endif
}

static void pbap_vl_start(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData)
{
    LOG_I(PBAP, "vl start length %x mb %x\r\n", dataLength, moreData);
    VcardList(str+dataOffset, dataLength);
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap vcard list start %lu", app->svc_id);
    #endif
}

static void pbap_vl_data(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData)
{
    //LOG_I(PBAP, "vl data length %x mb %x\r\n", dataLength, moreData);
    VcardList(str+dataOffset, dataLength);
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap vcard list data %lu", app->svc_id);
    #endif
}

static void pbap_vl_complete(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result)
{
    LOG_I(PBAP, "vl compl %x\r\n", result);
    #if 0
	pbap_app_t *app = getPbapAppFromSession(session);
	bt_frontend_notification("pbap vcard list complete %lu", app->svc_id);
    #endif
}

static void pbap_ve_start(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData)
{
    LOG_I(PBAP, "ve start md %x\r\n", dataOffset, dataLength, moreData);
    #ifdef EXTRACT_VCARD	
    extractVcard(str+dataOffset, dataLength);
    #endif
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap vcard entry start %lu", app->svc_id);
    #endif
}

static void pbap_ve_data(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData)
{
    LOG_I(PBAP, "ve data %x length %x md %x\r\n", dataOffset, dataLength, moreData);
    #ifdef EXTRACT_VCARD	
    extractVcard(str+dataOffset, dataLength);
    #endif
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap vcard entry data %lu", app->svc_id);
    #endif
}

static void pbap_ve_complete(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result)
{
    LOG_I(PBAP, "ve compl %x\r\n", result);
    #if 0
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap vcard entry complete %lu", app->svc_id);
    #endif
}

static void pbap_pb_start(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData)
{
    LOG_I(PBAP, "pb_start length %x mb %x\r\n", dataLength, moreData);
    #if 0 
    BK_GENERAL_MESSAGE_T *indMsg = NULL;
    uint16 msgLen = 0;
    //uint16_t i;
    //for(i=0; i<dataLength; i++)
    //{
    //	jprintf("%c", *(str+dataOffset+i));
    //}
	pbap_app_t *app = getPbapAppFromSession(session);
	//bt_frontend_notification("pbap pb start %lu", app->svc_id);

	MAKE_BK_IND_MESSAGE_WITH_PB_DATA(indMsg, BK_PB_START_IND, msgLen);
	((BK_PB_START_IND_T *)indMsg->data)->deviceId = app_env_get_index_from_addr(app->raddr);
	((BK_PB_START_IND_T *)indMsg->data)->dataLen = dataLength;
	indMsg->header.dataLen += dataLength;
	sendMessageToHost((uint8_t*)indMsg, msgLen);
	uart_debug_send_poll((uint8_t*)(str+dataOffset), dataLength);
	jprintf("pbap_pb_start %x %d\n", *(str+dataOffset), dataLength);
	
    #endif
    #ifdef EXTRACT_VCARD
    extractVcard(str+dataOffset, dataLength);
    #endif
}

static void pbap_pb_data(bt_pbap_session_h session, bt_app_ctx_h app_ctx, uint8_t *str, uint16_t dataOffset, uint16_t dataLength, BOOL moreData)
{
    LOG_I(PBAP, "pb data %d %d %x\r\n", dataOffset, dataLength, moreData);
#if 0
    BK_GENERAL_MESSAGE_T *indMsg = NULL;
    uint16 msgLen = 0;
    pbap_app_t *app = getPbapAppFromSession(session);
    bt_frontend_notification("pbap pb data %lu", app->svc_id);

    MAKE_BK_IND_MESSAGE_WITH_PB_DATA(indMsg, BK_PB_DATA_IND, msgLen);
    ((BK_PB_DATA_IND_T *)indMsg->data)->deviceId = app_env_get_index_from_addr(app->raddr);
    ((BK_PB_DATA_IND_T *)indMsg->data)->dataLen = dataLength;
    indMsg->header.dataLen += dataLength;
    sendMessageToHost((uint8_t*)indMsg, msgLen);
    uart_debug_send_poll((uint8_t*)(str+dataOffset), dataLength);
    //jprintf("pbap_pb_data %x %d\n", *(str+dataOffset), dataLength);
#endif

#ifdef EXTRACT_VCARD	
    extractVcard(str+dataOffset, dataLength);
#endif

}

static void pbap_pb_complete(bt_pbap_session_h session, bt_app_ctx_h app_ctx, result_t result)
{
    #if 0
	BK_GENERAL_MESSAGE_T *indMsg = NULL;
    uint16 msgLen = 0;
	pbap_app_t *app = getPbapAppFromSession(session);;
	bt_frontend_notification("pbap pb complete %lu", app->svc_id);
	
	MAKE_BK_IND_MESSAGE_WITH_DATA(indMsg, BK_PB_COMPLETE_IND, msgLen);
	((BK_PB_COMPLETE_IND_T *)indMsg->data)->deviceId = app_env_get_index_from_addr(app->raddr);
	((BK_PB_COMPLETE_IND_T *)indMsg->data)->status = result;
	sendMessageToHost((uint8_t*)indMsg, msgLen);
    #endif
    LOG_I(PBAP, "pbap bd compl %x\r\n", result);
}

static void pbap_disconnected(bt_pbap_session_h session, result_t err,
    bt_app_ctx_h app_ctx)
{
	pbap_app_t *app = getPbapAppFromSession(session);
	//uint8_t deviceId = app_env_get_index_from_addr(app->raddr);
	
    //bt_frontend_notification("pbap disconnected %lu", app->svc_id);
    LOG_I(PBAP, "pbap disconn\r\n");
    app->svc_id++;
    app->is_used = 0;
    app->setPathAlready = 0;
    
    setPbapStatus(app, pbap_ready);
    //pbapConnectionStatusInd(deviceId, BK_SLC_DISCONNECTED);
}

static void pbap_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
    const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_server_ctx)
{
    ;
}

void pbap_input(bt_pbap_session_h session, at_result_t *at_result,
    bt_app_ctx_h app_ctx)
{
    ;
}

void pbap_get_features(bt_pbap_session_h session_h,
    uint16_t *supported_features, bt_app_ctx_h app_ctx)
{
    if (supported_features)
        *supported_features = PBAP_SUPPORTED_FEATURES;
    LOG_I(PBAP, "pbap feature %x\r\n", *supported_features);
}

static enum {
    UNINIT = 0,
    PBAP_REGISTER,
    SERVER_START,
    BACKEND_REGISTER,
    CONN_REGISTER
} pbap_sample_init_stage = UNINIT;

void pbap_backend_uninit(void);
result_t pbap_backend_init(void)
{
    result_t err;
    DECLARE_FNAME("pbap_backend_init");

    j_memset(&g_pbap_app[0], 0, sizeof(g_pbap_app[0]));
    j_memset(&g_pbap_app[1], 0, sizeof(g_pbap_app[1]));

    err = bt_pbap_register(&pbap_cbs);
    if (err)
        goto Exit;
    pbap_sample_init_stage = PBAP_REGISTER;

    err = bt_pbap_server_start(&g_pbap_app[0].server, BTADDR_ANY,
        RFCOMM_CHANNEL_PBAP, pbap_newconn, NULL);

    err = bt_pbap_server_start(&g_pbap_app[1].server, BTADDR_ANY,
        RFCOMM_CHANNEL_PBAP, pbap_newconn, NULL);
    if (err)
        goto Exit;
    pbap_sample_init_stage = SERVER_START;

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
    err = backend_section_register(&pbap_section);
    if (err)
        goto Exit;
    pbap_sample_init_stage = BACKEND_REGISTER;
#endif

    err = bt_pbap_conn_create(&g_pbap_app[0].session, &pbap_conn_cbs,
        NULL);

    err = bt_pbap_conn_create(&g_pbap_app[1].session, &pbap_conn_cbs,
        NULL);
    if (err)
        goto Exit;
    pbap_sample_init_stage = CONN_REGISTER;

Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s, stage: %d\n", FNAME, uwe_str(err),
        pbap_sample_init_stage));

    if (err)
    {
        LOG_E(PBAP, "pbap init fail %x\r\n", err);
        pbap_backend_uninit();
    }else
    {
	    setPbapStatus(&g_pbap_app[0], pbap_ready);
        setPbapStatus(&g_pbap_app[1], pbap_ready);
    }
    return err;
}

void pbap_backend_uninit(void)
{
    if (pbap_sample_init_stage == CONN_REGISTER)
    {
        bt_pbap_conn_destroy(&g_pbap_app[0].session);
        bt_pbap_conn_destroy(&g_pbap_app[1].session);
        pbap_sample_init_stage = BACKEND_REGISTER;
    }

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
    if (pbap_sample_init_stage == BACKEND_REGISTER)
    {
        backend_section_unregister(&pbap_section);
        pbap_sample_init_stage = SERVER_START;
    }
#endif

    if (pbap_sample_init_stage == SERVER_START)
    {
        bt_pbap_server_stop(&g_pbap_app[0].server);
        bt_pbap_server_stop(&g_pbap_app[1].server);
        pbap_sample_init_stage = PBAP_REGISTER;
    }

    if (pbap_sample_init_stage == PBAP_REGISTER)
    {
        bt_pbap_unregister();
        pbap_sample_init_stage = UNINIT;
    }
}

//this function maybe useless
#if 0
result_t pbap_slc_connect(btaddr_t *raddr, uint8_t action)
{
	result_t err;
	btaddr_t laddr;
	uint32_t unit_id = 0;
	pbap_app_t *app = getFreePbapApp();
	
	if (!app)
		return UWE_NOMEM;
	
	err = backend_unit_get_addr(unit_id, &laddr);
	app->action = action;
	
	sdp_client_connect_device(&laddr, raddr, SDP_SERVICE_CLASS_PHONEBOOK_ACCESS_PSE);
	
    return err;
}
#endif

result_t pbap_pull_phonebook(btaddr_t *raddr, uint16_t start, uint16_t end,
    pbap_phone_repository pbRepositorty, pbap_phone_book pbPath)
{
    result_t err = UWE_OK;
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);

    if(app->pbap_state == pbap_Connected)
    {
        err = bt_pbap_pull_phonebook_start(app->session, pbRepositorty, pbPath, start, end, pbap_format_21);
    }else
    {
        LOG_E(PBAP, "no conned %x\r\n", app->pbap_state);
        err = UWE_NOTCONN;
    }
    return err;
}

result_t pbap_pull_vcard_list(btaddr_t *raddr, uint16_t start_index, uint16_t list_length, uint8_t orderValue, uint8_t search_att)
{
    result_t err = UWE_OK;
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);

    if(app->pbap_state == pbap_Connected)
    {
        err = bt_pbap_pull_vcard_listing_start(app->session, list_length, start_index, orderValue, search_att);
    }else
    {
        LOG_E(PBAP, "no conned %x\r\n", app->pbap_state);
        err = UWE_NOTCONN;
    }
    return err;
}

result_t pbap_pull_target_vcard(btaddr_t *raddr, uint8_t phone_num_len, uint8_t* phone_num_str)
{
    result_t err = UWE_OK;
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);

    if(app->pbap_state == pbap_Connected)
    {
        err = bt_pbap_pull_target_vcard(app->session, phone_num_len, phone_num_str);
    }else
    {
        LOG_E(PBAP, "no conned %x\r\n", app->pbap_state);
        err = UWE_NOTCONN;
    }
    return err;
}


result_t pbap_pull_vcard_entry(btaddr_t *raddr, uint32_t index)
{
    result_t err = UWE_OK;
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);
    if(app->pbap_state == pbap_Connected)
    {
        err = bt_pbap_pull_vcard_entry_start(app->session, index, 0x84, 0x00, pbap_format_21);
    }else
    {
        LOG_E(PBAP, "no conned %x\r\n", app->pbap_state);
        err = UWE_NOTCONN;
    }
    return err;
}

uint8_t go_back_to_root(bt_pbap_session_h session)
{
    pbap_app_t *app = getPbapAppFromSession(session);
    uint8_t ret = 0;
    if(app->setPathAlready)
    {
        app->setPathAlready = 0;
        ret = 1;
    }
    return ret;
}

result_t pbap_set_path(btaddr_t *raddr, pbap_phone_repository pbRepositorty, pbap_phone_book pbPath)
{
	result_t err = UWE_OK;
	pbap_app_t *app = getPbapAppFromBtaddr(raddr);
    if(app->pbap_state == pbap_Connected)
    {
        if(app->setPathAlready)
        {
            //LOG_I(PBAP, "go root\r\n");
            bt_pbap_go_to_root(app->session, pbRepositorty, pbPath);
            
        }else
        {
            //LOG_I(PBAP, "go path\r\n");
            err = bt_pbap_set_path(app->session, pbRepositorty, pbPath);
        }
    }else
    {
        LOG_E(PBAP, "no conned %x\r\n", app->pbap_state);
        err = UWE_NOTCONN;
    }
    return err;
}

#if 0
result_t pbap_slc_disconnect(btaddr_t *raddr)
{
	result_t err;
	pbap_app_t *app = getPbapAppFromBtaddr(raddr);
	
	if (app->pbap_state >= pbap_Connected)
	{
		err = bt_pbap_obex_disconnect(app->session);
	}
	
    return err;
}
#endif

result_t pbap_connect_rfcomm(btaddr_t raddr, uint8_t channel)
{
	result_t err;
	btaddr_t laddr;
	uint32_t unit_id = 0;
	pbap_app_t *app = getFreePbapApp();

    LOG_I(PBAP, "pbap_connect_rfcomm\r\n");
    
	err = backend_unit_get_addr(unit_id, &laddr);
	if (err)
	{
		err = UWE_NODEV;
		goto Exit;
	}
	
	app->laddr = laddr;
	app->raddr = raddr;
	err = bt_pbap_conn_connect(app->session, &laddr, &raddr, channel, app->action);
	if (err)
		goto Exit;

	setPbapStatus(app, pbap_Connecting);
	//bt_frontend_notification("pbap connecting %lu", app->svc_id);
	
Exit:
	DBG_RC_I(err, DBT_APP, ("%s: Done (%s)\n", FNAME, uwe_str(err)));
	return err;
}


result_t pbap_cmd_disconn(btaddr_t *raddr)
{
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);
    if (!app->session)
    {
        LOG_E(PBAP, "no session\r\n")
        return UWE_NODEV;
    }
    if (app->pbap_state != pbap_Connected)
    {
        LOG_E(PBAP, "no conn %x\r\n", app->pbap_state)
        return UWE_NOTCONN;
    }
    //return bt_pbap_conn_disconnect(g_pbap_app[0].session);
    return bt_pbap_obex_disconnect(app->session);
}



#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
static result_t pbap_cmd_connect(char *params, unsigned int len)
{
    result_t err;
    btaddr_t laddr, raddr;
    int btaddr_tmp[6];
    uint32_t channel;
    uint32_t unit_id;
    DECLARE_FNAME("pbap_cmd_connect");

    /*if (g_pbap_app[0].is_used)
    {
        err = UWE_ALREADY;
        goto Exit;
    }*/

    if (j_snscanf(params, NULL, "%u " BTADDR_FORMAT " %u", &unit_id,
        BTADDR_TMP(btaddr_tmp), &channel) != 8)
    {
        err = UWE_INVAL;
        goto Exit;
    }

    TMP_TO_BTADDR(btaddr_tmp, &raddr);

    err = backend_unit_get_addr(unit_id, &laddr);
    if (err)
    {
        err = UWE_NODEV;
        goto Exit;
    }


    /*err = bt_pbap_conn_connect(g_pbap_app[0].session, &laddr, &raddr,
        (uint8_t)channel, g_pbap_app[0].action);*/
    //sdp_client_connect_device(&laddr, SDP_SERVICE_CLASS_PHONEBOOK_ACCESS_PSE);
    if (err)
        goto Exit;

    g_pbap_app[0].is_used = 1;
    bt_frontend_notification("pbap connecting %lu", g_pbap_app[0].svc_id);

Exit:
    DBG_RC_I(err, DBT_APP, ("%s: Done (%s)\n", FNAME, uwe_str(err)));
    return err;
}

static result_t pbap_cmd_disconnect(char *params, unsigned int len)
{
    uint32_t svc_id;

    LOG_I(PBAP,"exec pbap disconn\r\n");
    if (j_snscanf(params, NULL, "%u", &svc_id) != 1)
        return UWE_INVAL;

    /*if (svc_id != g_pbap_app[0].svc_id)
        return UWE_PARAM;*/

    if (!g_pbap_app[0].session)
        return UWE_NODEV;

    //return bt_pbap_conn_disconnect(g_pbap_app[0].session);
    return bt_pbap_obex_disconnect(g_pbap_app[0].session);
}
#endif

static void VcardList(const uint8_t *ptr, uint16_t dataLen)
{
    uint16_t forCount = 0;
    for(forCount = 0; forCount < dataLen; forCount++)
    {
        os_printf("%c", *(ptr+forCount));
        if(*(ptr+forCount) == '>')
        {
            os_printf("\n");
        }
    }
}

void set_remote_supported_features(btaddr_t* raddr, uint32_t features)
{
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);
    app->remote_supported_features = features;
    LOG_I(PBAP, "support fea %x\r\n", app->remote_supported_features);
}
void set_remote_supported_repositories(btaddr_t* raddr, uint8_t repositories)
{
    pbap_app_t *app = getPbapAppFromBtaddr(raddr);
    app->remote_supported_repositories = repositories;
    LOG_I(PBAP, "support repo %x\r\n", app->remote_supported_repositories);
}


#ifdef EXTRACT_VCARD
static void extractVcard(const uint8_t *ptr, uint16_t dataLen)
{
    //LOG_I(PBAP, "extract data len %d\r\n", dataLen);
	uint16_t i = 0;
	uint16_t count = 0;
	for(i=0; i<dataLen; i++)
	{
		if ((*(ptr+i) == '\r') && (*(ptr+i-1) != '='))
		{
			if (g_pbap_app[0].nameBufPos != 0)
			{
				uint16_t j = 0;
				uint16_t k = 0;
				uint16_t colonPos = 0;
				uint16_t nameLen = MAX_NAME_LEN;
				for(j=0; j<MAX_NAME_LEN; j++)
				{
					if (vcardTelBuf[j] == ':')
						colonPos = j;
					if (vcardTelBuf[j] == 0x00)
					{
						nameLen = j;
						break;
					}
					else
					{
						//jprintf("%c", vcardTelBuf[j]);
					}
					
				}
				for(j=nameLen-1; j>=colonPos; j--)
				{
					count++;
					if ((vcardTelBuf[j] == ';') || (j == colonPos))
					{
						if (count != 1)
						{
							if (len != 0)
							{
								j_memcpy(vcardNameBuf+len, " ", 1);
								len += 1;
							}
							j_memcpy(vcardNameBuf+len, vcardTelBuf+j+1, count-1);
							len += count-1;
						}
						count = 0;
					}
					if (j == 0)
						break;
				}
				j = 0;
				while(j<len)
				{
					if (*(vcardNameBuf+j) == '=')
					{
						//jprintf("=%c%c\n", *(vcardNameBuf+j+1), *(vcardNameBuf+j+2));
						if ((*(vcardNameBuf+j+1) >= 'A') && (*(vcardNameBuf+j+1) <= 'F'))
						{
							*(vcardNameBuf+k) = (*(vcardNameBuf+j+1)-'A'+10)<<4;
						}
						else if ((*(vcardNameBuf+j+1) >= '0') && (*(vcardNameBuf+j+1) <= '9'))
						{
							*(vcardNameBuf+k) = (*(vcardNameBuf+j+1)-'0')<<4;
						}
						if ((*(vcardNameBuf+j+2) >= 'A') && (*(vcardNameBuf+j+2) <= 'F'))
						{
							*(vcardNameBuf+k) += *(vcardNameBuf+j+2)-'A'+10;
						}
						else if ((*(vcardNameBuf+j+2) >= '0') && (*(vcardNameBuf+j+2) <= '9'))
						{
							*(vcardNameBuf+k) += *(vcardNameBuf+j+2)-'0';
						}
						if ((*(vcardNameBuf+j+1) == 0x0d) && (*(vcardNameBuf+j+2) == 0x0a))
						{
							k -= 1;
						}
						j += 3;
					}
					else
					{
						*(vcardNameBuf+k) = *(vcardNameBuf+j);
						j++;
					}
					k++;
				}
				len = k;
                os_printf("name:");
				for(j=0; j<len; j++)
				{
					if (vcardNameBuf[j] == 0x00)
						break;
					os_printf("%c", vcardNameBuf[j]);
				}
				os_printf("\n");
			}
			if (g_pbap_app[0].telBufPos != 0)
			{
				uint16_t j = 0;
				uint16_t l = 0;
				uint16_t colonPos = 0;
				for(j=0; j<MAX_TEL_LEN; j++)
				{
					if (vcardTelBuf[j] == ':')
						colonPos = j;
					if (vcardTelBuf[j] == 0x00)
						break;
				}
				j_memcpy(vcardTelBuf, vcardTelBuf+colonPos+1, j-colonPos-1);
				j_memset(vcardTelBuf+j-colonPos-1, 0, MAX_TEL_LEN-(j-colonPos-1));
                os_printf("tel: ");
				for(l=0; l<MAX_TEL_LEN; l++)
				{
					if (vcardTelBuf[l] == 0x00)
						break;
					os_printf("%c", vcardTelBuf[l]);
				}
				os_printf("\n");
				/* Get the name and tel here */
				g_pbap_app[0].numOfVcard++;
                j_memset(last_vCardTel, 0, MAX_TEL_LEN);
                j_memcpy(last_vCardTel, vcardTelBuf,l);
			}
			g_pbap_app[0].newLineFlag = 0;
			g_pbap_app[0].newNameFlag = 0;
			g_pbap_app[0].nameBufPos = 0;
			g_pbap_app[0].telBufPos = 0;

            j_memset(vcardTelBuf, 0, MAX_TEL_LEN);
		}
		else if ((*(ptr+i) == '\n') && (*(ptr+i-2) != '='))
		{
			g_pbap_app[0].newLineFlag = 1;
		}
		else
		{
			if ((*(ptr+i) == ':') && (g_pbap_app[0].newNameFlag))
			{
				vcardTelBuf[g_pbap_app[0].nameBufPos] = *(ptr+i);
				g_pbap_app[0].nameBufPos = 1;
			}
			else if ((*(ptr+i) == 'N') && (g_pbap_app[0].newLineFlag))
			{
				g_pbap_app[0].newLineFlag = 0;
				len = 0;
				g_pbap_app[0].newNameFlag = 1;
			}
			else if ((*(ptr+i) == 'T') && (g_pbap_app[0].newLineFlag))
			{
				vcardTelBuf[g_pbap_app[0].nameBufPos] = *(ptr+i);
				g_pbap_app[0].telBufPos = 1;
				g_pbap_app[0].newLineFlag = 0;
			}
			else if (g_pbap_app[0].nameBufPos)
			{
				if (g_pbap_app[0].nameBufPos < MAX_NAME_LEN)
				{
					vcardTelBuf[g_pbap_app[0].nameBufPos] = *(ptr+i);
					g_pbap_app[0].nameBufPos++;
				}
			}
			else if (g_pbap_app[0].telBufPos)
			{
				if (g_pbap_app[0].telBufPos < MAX_TEL_LEN)
				{
					vcardTelBuf[g_pbap_app[0].telBufPos] = *(ptr+i);
					g_pbap_app[0].telBufPos++;
				}
			}
			else
			{
				g_pbap_app[0].newLineFlag = 0;
				g_pbap_app[0].nameBufPos = 0;
				g_pbap_app[0].telBufPos = 0;
			}
		}
	}
    //LOG_I(PBAP, "parser done %d %d\r\n", dataLen, i);
}
#endif
#endif //CONFIG_BLUETOOTH_PBAP
