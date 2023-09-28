/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <bluetooth.h>
#include <bt_avrcp_ctrl.h>
#include <bt_api.h>
#include <bt_sdp.h>
#include <bt_sdp_types.h>
#include "bt_app_internal.h"
#include "app_beken_includes.h"
#include "avctp_internal.h"
#if CONFIG_A2DP_CONN_DISCONN
#include "beken_external.h"
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
static sdp_avrcp_profile_t avrcp_profile_ct = {
    BT_SVC_TYPE_AV_REMOTE_CONTROL,        /* Control or Target */
#ifdef CONFIG_BLUETOOTH_AVRCP_BROWSE
    SDP_AVRCP_SUPPORTS_BROWSING |         /* Supported features */
#endif
    SDP_AVRCP_PLAYER_RECORDER,
    "AVRCP-CT"          /* Service name */
};
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TG
static sdp_avrcp_profile_t avrcp_profile_tg = {
    BT_SVC_TYPE_AV_REMOTE_CONTROL_TARGET,  /*Target */
    SDP_AVRCP_MONITOR_AMPLIFIER,
    "AVRCP-TG"          /* Service name */
};
#endif

typedef struct {
    /* Only single service is supported by this sample */
    uint32_t                svc_id;
#ifdef CONFIG_BLUETOOTH_AVRCP_TG
    uint32_t                server_sdp[2];
#else
    uint32_t                server_sdp;
#endif
    uint32_t                flag;

    bt_avrcp_ctrl_server_h  server;
    bt_avrcp_ctrl_session_h session;

    uint32_t                attr_id;
    uint16_t                char_set_id;
    uint8_t                 last_tid;

    avrcp_op_t              continuation_tag;

    bt_conn_req_h           conn_req;

    uint16_t                is_used;
    bt_link_modes_mask_t    conn_req_mode;

    uint8_t                 last_cmd;
    uint8_t                 is_vol_registered;

    btaddr_t                raddr;
	uint8_t                 volume_tid;      //only for volume_change_reponse use who support vol sync
} avrcp_app_t;

uint32_t avrcp_app_uninit(avrcp_app_t *app_ptr);

static void avrcp_send_opcode(avrcp_app_t *app_ptr, void *arg);
static void avrcp_paththrough_rsp(bt_avrcp_ctrl_session_h session,
                                          avrcp_tid_t tid, avc_response_t rsp, bt_app_ctx_h app_ctx);
static void avrcp_getcaps_events_rsp(bt_avrcp_ctrl_session_h session,
                                              avrcp_tid_t tid, avc_response_t response, avrcp_event_id_t *events,
                                              uint8_t count, avrcp_pkt_type_t packet_type,
                                              avrcp_op_t bt_avrcp_continuation_tag, bt_app_ctx_h app_ctx);
static void avrcp_event_rsp(bt_avrcp_ctrl_session_h session, avrcp_tid_t tid,
                                  avc_response_t response, avrcp_event_rsp_t *ev,
                                  avrcp_pkt_type_t packet_type, avrcp_op_t bt_avrcp_continuation_tag,
                                  bt_app_ctx_h app_ctx);
static void avrcp_get_nowplaying_attr_rsp(bt_avrcp_ctrl_session_h session_h,
                                                    avrcp_tid_t tid, avc_response_t rsp, 
                                                    uint32_t attr_id, uint16_t char_set_id,
                                                    uint16_t attr_total_len, char *attr_val, 
                                                    uint16_t attr_val_len, avrcp_pkt_type_t packet_type, 
                                                    avrcp_op_t bt_avrcp_continuation_tag,
                                                    bt_app_ctx_h app_ctx_h);
static void avrcp_get_play_status_rsp(bt_avrcp_ctrl_session_h session, avrcp_tid_t tid, 
                                              avc_response_t rsp, avrcp_get_play_status_rsp_t *status,
                                              avrcp_pkt_type_t packet_type, avrcp_op_t bt_avrcp_continuation_tag,
                                              bt_app_ctx_h app_ctx);
result_t avrcp_cmd_connect(char *params, unsigned int len);
result_t avrcp_cmd_disconnect(void);
static result_t avrcp_cmd_accept(avrcp_app_t *app_ptr, char *params, unsigned int len);
result_t avrcp_cmd_register_notification(bt_avrcp_ctrl_session_h session,uint8_t event_id, 
                                                  uint32_t playback_pos_changed_interval);
static void avrcp_svc_connected(bt_avrcp_ctrl_session_h session, bt_app_ctx_h app_ctx);
static void avrcp_svc_disconnected(bt_avrcp_ctrl_session_h session, result_t status, 
                                            bt_app_ctx_h app_ctx);
static void avrcp_svc_newconn(bt_conn_req_h conn_req,
                                       const btaddr_t *laddr,
                                       const btaddr_t *raddr,
                                       bt_link_modes_mask_t mode,
                                       bt_app_ctx_h app_server_ctx);
static void avrcp_set_flag(avrcp_app_t *app_ptr, uint32_t flag);
static void avrcp_clear_flag(avrcp_app_t *app_ptr, uint32_t flag);

static bt_avrcp_ctrl_cbs_t avrcp_responses = {
    avrcp_paththrough_rsp,
    avrcp_getcaps_events_rsp,
    avrcp_event_rsp,
    avrcp_get_nowplaying_attr_rsp,
    avrcp_get_play_status_rsp,
};

static bt_avrcp_ctrl_conn_cbs_t svc_cbs = {
    avrcp_svc_connected,
    avrcp_svc_disconnected,
};

static bt_avrcp_ctrl_server_h g_avrcp_srv;
static avrcp_app_t *g_avrcp_current_app;
static avrcp_app_t g_avrcp_array[2];

avrcp_app_t *avrcp_get_app_from_session(bt_avrcp_ctrl_session_h session)
{
    uint32_t i;
    avrcp_app_t *app_ptr=NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_avrcp_array[i].is_used && (session == g_avrcp_array[i].session))
        {
            app_ptr = &g_avrcp_array[i];
            break;
        }
    }

    if (!app_ptr)
        LOG_W(CONN, "avrcp_get_app_from_session is NULL\r\n");

    return app_ptr;
}

avrcp_app_t *avrcp_app_lookup_valid(const btaddr_t *raddr)
{
    uint32_t i;
    avrcp_app_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_avrcp_array[i];
        if((app_ptr->is_used) && (btaddr_same(raddr, &app_ptr->raddr)))
        {
            //LOG_E(CONN,"Find same avrcp_app\r\n");
            return app_ptr;
        }
    }

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_avrcp_array[i];
        if(!app_ptr->is_used)
        {
            break;
        }
    }

    return app_ptr;
}

uint32_t avrcp_app_init(avrcp_app_t *app_ptr)
{
    result_t err;

    LOG_D(APP, "avrcp_app_init\r\n");

    err = bt_avrcp_ctrl_conn_create(&app_ptr->session, &svc_cbs, NULL);
    if(err)
    {
        goto Error;
    }
    
#if defined (CONFIG_BLUETOOTH_AVRCP_CT) && defined (CONFIG_BLUETOOTH_AVRCP_TG)
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
    err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AV_REMOTE_CONTROL,
        &avrcp_profile_ct, sizeof(avrcp_profile_ct), &app_ptr->server_sdp[0]);
    if (err)
    {
        goto Error;
    }
#endif
    extern int app_env_check_AVRCP_TG_profile_enable(void);
    if(app_env_check_AVRCP_TG_profile_enable())
    {
        err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AV_REMOTE_CONTROL_TARGET,
            &avrcp_profile_tg, sizeof(avrcp_profile_tg), &app_ptr->server_sdp[1]);
        if (err)
        {
            goto Error;
        }
    }
#else
    err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AV_REMOTE_CONTROL,
        &avrcp_profile_ct, sizeof(avrcp_profile_ct), &app_ptr->server_sdp);
    if (err)
    {
        goto Error;
    }
#endif
   
Error:

    avrcp_app_uninit(app_ptr);
    
    return err;
}

uint32_t avrcp_app_uninit(avrcp_app_t *app_ptr)
{
    if(app_ptr && app_ptr->is_used)
    {  
#ifdef CONFIG_BLUETOOTH_AVRCP_TG
        if(app_ptr->server_sdp[0])
        {
            bt_sdp_service_unregister(app_ptr->server_sdp[0]);
        }
        
        if(app_ptr->server_sdp[1])
        {
            bt_sdp_service_unregister(app_ptr->server_sdp[1]);
        }
#else
        if(app_ptr->server_sdp)
        {
            bt_sdp_service_unregister(app_ptr->server_sdp);
        }
#endif
        if(app_ptr->conn_req)
        {
            bt_avrcp_ctrl_conn_reject(app_ptr->conn_req);
        }

        if(app_ptr->session)
        {
            bt_avrcp_ctrl_conn_destroy(&app_ptr->session);
        }
        
        j_memset(app_ptr, 0, sizeof(avrcp_app_t));
    }
   
    return UWE_OK;
}

result_t avrcp_backend_init(void)
{
    result_t err;
    LOG_D(APP,"avrcp_backend_init\r\n");

    g_avrcp_srv = NULL;
    g_avrcp_current_app = NULL;
    j_memset(&g_avrcp_array[0], 0, sizeof(avrcp_app_t));
    j_memset(&g_avrcp_array[1], 0, sizeof(avrcp_app_t));

    err = bt_avrcp_ctrl_register(&avrcp_responses);
    if(err)
    {
        return err;
    }

    err = bt_avrcp_ctrl_server_start(&g_avrcp_srv,
                                     BTADDR_ANY,
                                     avrcp_svc_newconn,
                                     NULL);

    if(err)
    {
        goto Error;
    }

    #if defined (CONFIG_BLUETOOTH_AVRCP_CT) && defined (CONFIG_BLUETOOTH_AVRCP_TG)
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
    err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AV_REMOTE_CONTROL,
        &avrcp_profile_ct, sizeof(avrcp_profile_ct), &g_avrcp_current_app->server_sdp[0]);
    if (err)
    {
        goto Error;
    }
#endif
#if defined(CONFIG_BLUETOOTH_AVRCP_TG)
	extern int app_env_check_AVRCP_TG_profile_enable(void);
	if(app_env_check_AVRCP_TG_profile_enable())
	{
	    err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AV_REMOTE_CONTROL_TARGET,
	        &avrcp_profile_tg, sizeof(avrcp_profile_tg), &g_avrcp_current_app->server_sdp[1]);
	    if (err)
	    {
	        goto Error;
	    }
	}
#endif
#else
    err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AV_REMOTE_CONTROL,
        &avrcp_profile_ct, sizeof(avrcp_profile_ct), &g_avrcp_current_app->server_sdp);
    if (err)
    {
        goto Error;
    }
#endif

    return UWE_OK;

Error:
    
    bt_avrcp_ctrl_unregister();

    return err;
}

void avrcp_backend_uninit(void)
{
    uint32_t i;

    if(g_avrcp_srv)
    {
        bt_avrcp_ctrl_server_stop(&g_avrcp_array[0].server);
        g_avrcp_srv = NULL;
    }
    
    bt_avrcp_ctrl_unregister();

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_avrcp_array[i].is_used)
        {
            avrcp_app_uninit(&g_avrcp_array[i]);
        }
    }
}

#if (AVRCP_IPHONE_VOL_EVEN == 1)
uint8_t avrcp_is_vol_registered()
{
    return g_avrcp_current_app->is_vol_registered;
}

void avrcp_set_vol_registered(uint8_t vol_register)
{
    g_avrcp_current_app->is_vol_registered = vol_register;
}
#endif

uint32_t avrcp_has_connection(void)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used) && (g_avrcp_array[i].flag & APP_FLAG_AVCRP_CONNECTION))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t avrcp_has_the_connection(btaddr_t addr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used)
            && btaddr_same(&g_avrcp_array[i].raddr, &addr)
            && (g_avrcp_array[i].flag & APP_FLAG_AVCRP_CONNECTION))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t avrcp_is_cmd_processing(void)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used) && (g_avrcp_array[i].flag & APP_FLAG_AVCRP_PROCESSING))
        {
            return 1;
        }
    }

    return 0;
}

int avrcp_is_connected_based_on_raddr(const btaddr_t *raddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used) 
            && (g_avrcp_array[i].session)
            && (btaddr_same(raddr, &g_avrcp_array[i].raddr))
            && (g_avrcp_array[i].flag & APP_FLAG_AVCRP_CONNECTION))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t avrcp_current_is_processing(void)
{
    return (g_avrcp_current_app->flag & APP_FLAG_AVCRP_PROCESSING);
}

uint32_t avrcp_current_is_connected(void)
{
    return (g_avrcp_current_app->flag & APP_FLAG_AVCRP_CONNECTION);
}

avrcp_app_t *avrcp_get_app_ptr_by_raddr(const btaddr_t *raddr)
{
    uint32_t i;
    avrcp_app_t *ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used)
            && (g_avrcp_array[i].session)
            && (btaddr_same(raddr, &g_avrcp_array[i].raddr)))
        {
            ptr = &g_avrcp_array[i];
            break;
        }
    }

    return ptr;
}

#ifdef CONFIG_BLUETOOTH_AVRCP_TG
void AVRCP_TG_VOL_adjust_callback(uint8_t opera);
void AVRCP_TG_VOL_set_callback(uint8_t volume_dat, btaddr_t *raddr);
uint8_t AVRCP_TG_VOL_get_callback(btaddr_t *raddr);
uint8_t get_tg_volume_value(void);
uint32_t avrcp_current_support_vol_syn(void);
void current_send_volume_change_response(uint8_t volume_value);

BOOL avrcp_update_vol_tid_based_on_session(bt_avrcp_ctrl_session_h session, uint8_t volume_tid)
{
    avrcp_app_t *app_ptr = NULL;

    app_ptr = avrcp_get_app_from_session(session);

    if(app_ptr)
    {
        app_ptr->volume_tid = volume_tid ;
        return TRUE;
    }

    LOG_E(AVRCP, "Error:avrcp_update_vol_tid_based_on_session failed!!\r\n");
    return FALSE;
}

uint32_t avrcp_current_support_vol_syn(void)
{
    return (g_avrcp_current_app->flag & APP_FLAG2_VOL_SYNC);
}

BOOL avrcp_set_vol_syn_flag_based_on_session(bt_avrcp_ctrl_session_h session)
{
    avrcp_app_t *app_ptr = NULL;

    app_ptr = avrcp_get_app_from_session(session);

    if(app_ptr)
    {
        avrcp_set_flag(app_ptr,APP_FLAG2_VOL_SYNC);
        return TRUE;
    }

    LOG_E(AVRCP, "Error:avrcp_set_vol_syn_flag_based_on_session failed!!\r\n");
    return FALSE;
}

BOOL avrcp_set_vol_syn_ok_flag_based_on_session(bt_avrcp_ctrl_session_h session)
{
    avrcp_app_t *app_ptr = NULL;

    app_ptr = avrcp_get_app_from_session(session);

    if(app_ptr)
    {
        avrcp_set_flag(app_ptr,APP_FLAG2_VOL_SYNC|APP_FLAG2_VOL_SYNC_OK);
        return TRUE;
    }

    LOG_I(CONN, "Error:avrcp_set_vol_syn_flag_based_on_session failed!!\r\n");
    return FALSE;
}

extern result_t avrcp_send_vendor_dependent_volume_change_response(bt_avrcp_ctrl_session_h session, uint8_t tid, uint8_t volume_value);
void current_send_volume_change_response(uint8_t volume_value)
{
    result_t err;
    if(avrcp_current_support_vol_syn()==0) return;	//yuan++

    err = avrcp_send_vendor_dependent_volume_change_response(g_avrcp_current_app->session,
		                                                     g_avrcp_current_app->volume_tid,
		                                                     volume_value);

    if(err)
       LOG_E(AVRCP,"ERROR:current_send_volume_change_response_failed!!\r\n");
}

extern result_t a2dp_volume_adjust( uint8_t oper );

void AVRCP_TG_VOL_adjust_callback(uint8_t opera)
{
    if(opera == 1)
        a2dp_volume_adjust(1);
    else
        a2dp_volume_adjust(0);
}

//const uint8_t volume_table[16+1]={0,0x08,0x11,0x19,0x22,0x2a,0x33,0x3b,0x44,0x4c,0x55,0x5d,0x66,0x6e,0x77,0x7e,0x7f};
//const uint8_t volume_table[16+1]={0,0x07,0x0f,0x17,0x1f,0x27,0x2f,0x37,0x3f,0x47,0x4f,0x57,0x5f,0x67,0x6f,0x77,0x7f};
#if(AVRCP_IPHONE_VOL_EVEN == 1)
#if (AUDIO_VOLUME_MAX == 16)
static uint8_t iphone_vol_tbl[16+1] = {0x0,0x8,0x10,0x18,0x20,0x28,0x30,0x38,0x40,0x47,0x4f,0x57,0x5f,0x67,0x6f,0x77,0x7f};
#elif (AUDIO_VOLUME_MAX == 32)
static uint8_t iphone_vol_tbl[32+1] = {0x0,	0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,0x40,
											0x44,0x48,0x4C,0x50,0x54,0x58,0x5C,0x60,0x64,0x68,0x6C,0x70,0x74,0x78,0x7C,0x7F};
#endif
#endif

extern btaddr_t *a2dp_get_current_app_remote_addr(void);
extern int8_t a2dp_get_volume_based_on_raddr(const btaddr_t *raddr);
extern void a2dp_volume_init_based_on_raddr(int8_t aud_volume, const btaddr_t *raddr);
extern int8_t a2dp_get_volume( void );
uint8_t get_tg_volume_value(void)
{
    uint32_t vol = a2dp_get_volume();
 #if(AVRCP_IPHONE_VOL_EVEN == 1)    
    return iphone_vol_tbl[vol];
#else   
    return (int8_t)vol <= 0 ? 0: (vol >= 16) ? 0x7f:(vol * 8 + 6);
#endif    
}

//***************************************************************
void sync_ct_tg_volume(uint8_t volume_dat, btaddr_t *raddr)
{
#if (AUDIO_VOLUME_MAX == 16)
    uint8_t vol_idx = ((uint32_t)volume_dat + 1) / 8;
#elif (AUDIO_VOLUME_MAX == 32)
    uint8_t vol_idx = ((uint32_t)volume_dat + 1) / 4;	//yuan 8->4
    LOG_I(AVRCP,"sync_ct_tg_volume:%d  %d\r\n", volume_dat, vol_idx);
#endif
    a2dp_volume_init_based_on_raddr(vol_idx, raddr);

#if 0 //yuan++
    os_printf("ct tg vol=%d\r\n",vol_idx);
    if(btaddr_same(raddr,a2dp_get_current_app_remote_addr()) && (!hfp_has_sco()) && (!bt_audio_ch_busy ())){
        aud_dac_set_volume(vol_idx);
        if (0 == vol_idx){
            aud_PAmute_operation(1);
            aud_volume_mute(1);
            app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 1);
        }
    }
#endif
}

//*********************************************************************
void AVRCP_TG_VOL_set_callback(uint8_t volume_dat, btaddr_t *raddr)
{
#if (AUDIO_VOLUME_MAX == 16)
    uint32_t vol_idx = ((uint32_t)volume_dat + 1) / 8;
#elif (AUDIO_VOLUME_MAX == 32)
    uint32_t vol_idx = ((uint32_t)volume_dat + 1) / 4;	//yuan 8->4
#endif

    if(btaddr_same(raddr,a2dp_get_current_app_remote_addr()) && (!hfp_has_sco()) && (!bt_audio_ch_busy()))
    {
		uint32_t a2dp_vol_idx = a2dp_get_volume();

		if(vol_idx > a2dp_vol_idx)
		{
			a2dp_volume_adjust(1);
		}
		else if(vol_idx < a2dp_vol_idx)
		{
			a2dp_volume_adjust(0);
		}
    }
    
    //a2dp_volume_init_based_on_raddr(vol_idx, raddr);
    sync_ct_tg_volume(volume_dat, raddr);
}

uint8_t AVRCP_TG_VOL_get_callback(btaddr_t *raddr)
{
#if(AVRCP_IPHONE_VOL_EVEN == 0)    
    uint8_t volume_index = a2dp_get_volume_based_on_raddr(raddr);
    
    LOG_I(AVRCP,"volume_sync_get:%d\r\n", volume_index);

	if(volume_index == AUDIO_VOLUME_MAX+1)
        return 0x60;
	else
		return (int8_t)volume_index <= 0 ? 0
			: (volume_index >= 16) ? 0x7f
			:(volume_index * 8 + 6);
	//return volume_index > 0 ? ((volume_index+1) * 8 - 1) : 0;
#else
    return get_tg_volume_value();
#endif    
}

#else

void AVRCP_TG_VOL_adjust_callback(uint8_t opera);
void AVRCP_TG_VOL_set_callback(uint8_t volume_dat, btaddr_t *raddr);
uint8_t AVRCP_TG_VOL_get_callback(btaddr_t *raddr);

void AVRCP_TG_VOL_adjust_callback(uint8_t opera)
{

}
void AVRCP_TG_VOL_set_callback(uint8_t volume_dat, btaddr_t *raddr)
{

}
uint8_t AVRCP_TG_VOL_get_callback(btaddr_t *raddr)
{
    return 0;
}

BOOL avrcp_update_vol_tid_based_on_session(bt_avrcp_ctrl_session_h session, uint8_t volume_tid)
{
	return FALSE;
}
BOOL avrcp_set_vol_syn_flag_based_on_session(bt_avrcp_ctrl_session_h session)
{
    return FALSE;
}
BOOL avrcp_set_vol_syn_ok_flag_based_on_session(bt_avrcp_ctrl_session_h session)
{
    return FALSE;
}
#endif

static void current_send_pass_through_cmd(void * arg)
{
    result_t err;
    uint32_t tmp = (uint32_t)arg;
    avc_op_t avc_op = (avc_op_t)tmp;

    g_avrcp_current_app->last_tid = (g_avrcp_current_app->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;

    err = bt_avrcp_send_pass_through_cmd(g_avrcp_current_app->session,
                                         g_avrcp_current_app->last_tid, avc_op,
                                         AVC_RELEASE);

    if(err)
    {
        LOG_E(AVRCP,"send_cmd_failed:%x:%x\r\n", avc_op, err);
    }
}

static void avrcp_send_pass_through_cmd_by_app_ptr(avrcp_app_t *app_ptr,void * arg)
{
    result_t err;
    uint32_t tmp = (uint32_t)arg;
    avc_op_t avc_op = (avc_op_t)tmp;

    app_ptr->last_tid = (app_ptr->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;

    err = bt_avrcp_send_pass_through_cmd(app_ptr->session,
                                         app_ptr->last_tid, avc_op,
                                         AVC_RELEASE);

    if(err)
    {
        LOG_E(AVRCP,"send_cmd_failed:%x:%x\r\n", avc_op, err);
    }
}

void avrcp_send_opcode_by_raddr(const btaddr_t *raddr, void *arg)
{
    avrcp_app_t *app_avrcp_ptr;

    LOG_I(AVRCP,"avrcp_send_opcode_by_raddr\r\n");
    app_avrcp_ptr = avrcp_get_app_ptr_by_raddr(raddr);
    if(app_avrcp_ptr)
    {
        avrcp_send_opcode(app_avrcp_ptr, arg);
    }
    else
    {
        LOG_E(AVRCP,"no_avrcp_ptr\r\n");
    }
}

void avrcp_send_opcode(avrcp_app_t *app_ptr, void *arg)
{
    result_t err;
    int32_t duration = 500;
    uint32_t tmp = (uint32_t)arg;
    avc_op_t avc_op = (avc_op_t)tmp;
    //app_env_handle_t env_h = app_env_get_handle();

    if(!app_ptr->is_used)
    {
        return;
    }

    app_ptr->last_tid = (app_ptr->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;
    app_ptr->last_cmd = avc_op;
    err = bt_avrcp_send_pass_through_cmd(app_ptr->session,
                                         app_ptr->last_tid,
                                         avc_op,
                                         AVC_PRESS);
    if(err)
    {
        LOG_E(AVRCP,"tx_cmd_failed:%x\r\n", avc_op);
    }

    //duration = env_h->env_cfg.button_para.repeat - 10;

    if((avc_op == AVC_OP_REWIND) || (avc_op == AVC_OP_FAST_FORWARD))
    {
        app_bt_shedule_task(current_send_pass_through_cmd, arg, duration);
    }
    else
    {
        //current_send_pass_through_cmd(arg);
        avrcp_send_pass_through_cmd_by_app_ptr(app_ptr,arg);
    }

    avrcp_set_flag(app_ptr, APP_FLAG_AVCRP_PROCESSING);
}

void avrcp_current_send_opcode(void *arg)
{
    avrcp_send_opcode(g_avrcp_current_app, arg);
}

void send_get_play_status_cmd(avrcp_app_t *app_ptr)
{
	app_ptr->last_tid = (app_ptr->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;

	bt_avrcp_send_get_play_status_cmd(app_ptr->session, app_ptr->last_tid);
    //bt_avrcp_send_get_element_attr_cmd(g_app.session, g_app.last_tid,3);
}
result_t avrcp_cmd_getcapability(avrcp_app_t *app_ptr)
{
    app_ptr->last_tid = (app_ptr->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;

    return bt_avrcp_send_get_capabilities_cmd(app_ptr->session, app_ptr->last_tid,
                                              AVRCP_CAP_EVENTS_SUPPORTED);
}

static void avrcp_svc_connected(bt_avrcp_ctrl_session_h session, bt_app_ctx_h app_ctx)
{
    avrcp_app_t *app_ptr;
    app_handle_t sys_hdl = app_get_sys_handler();

    app_ptr = avrcp_get_app_from_session(session);

    LOG_I(CONN, "avrcp_svc_connected\r\n");

    avrcp_set_flag(app_ptr, APP_FLAG_AVCRP_CONNECTION);

    if(!a2dp_has_music()) //if there is no music,we update the current_avrcp_ptr to the new connection
    	avrcp_update_current_app(&app_ptr->raddr);
    else//a2dp_get_current_app
        avrcp_update_current_app(a2dp_get_current_app_remote_addr());
    
    //app_bt_allpro_conn_start(AUTO_RE_CONNECT_SCHED_DELAY_AVRCP, &app_ptr->raddr);
    avrcp_cmd_getcapability(app_ptr);

    jtask_stop(sys_hdl->app_save_volume_task);
    //jtask_schedule(sys_hdl->app_save_volume_task, 2000, avrcp_volume_sync, (void *)NULL);
    app_bt_profile_conn_wrap(PROFILE_BT_AVRCP,&app_ptr->raddr);
#if CONFIG_A2DP_CONN_DISCONN
	if (sys_hdl->a2dp_state == APP_A2DP_STATE_DISCONN)
	{
		a2dp_set_disconnect();
	}
#endif
}

static void avrcp_svc_disconnected(bt_avrcp_ctrl_session_h session, result_t status, 
                                            bt_app_ctx_h app_ctx)
{
    avrcp_app_t *app_ptr = avrcp_get_app_from_session(session);
    LOG_D(CONN, "avrcp_svc_disconnected\r\n");
   //LOG_I(CONN, "avrcp_svc_disconnected src_id:%d,addr:"BTADDR_FORMAT"\r\n", app_ptr->svc_id,BTADDR(&app_ptr->raddr) );
    
    avrcp_clear_flag(app_ptr, APP_FLAG_AVCRP_CONNECTION|APP_FLAG_AVCRP_PROCESSING|APP_FLAG2_VOL_SYNC|APP_FLAG2_VOL_SYNC_OK);
    /* bt_app_management */
    bt_app_entity_clear_conn_flag_by_addr(&app_ptr->raddr,PROFILE_BT_AVRCP);
    set_connection_event(&app_ptr->raddr, CONN_EVENT_AVRCP_DISCONNECTED);

    avrcp_select_current_app_to_another(&app_ptr->raddr); 
    avrcp_app_uninit(app_ptr);
    g_avrcp_current_app->is_vol_registered = 0;
}

static void avrcp_svc_newconn(bt_conn_req_h conn_req,
                                       const btaddr_t *laddr,
                                       const btaddr_t *raddr,
                                       bt_link_modes_mask_t mode,
                                       bt_app_ctx_h app_server_ctx)
{
    uint32_t len;
    char strbuf[5];
    avrcp_app_t *app_ptr;
    uint32_t err=0;

    app_ptr = avrcp_app_lookup_valid(raddr);
    
    LOG_I(CONN, "avrcp_svc_newconn\r\n");
    
    if(app_ptr == NULL)
    {
        LOG_E(CONN, "avrcp_svc_newconn NULL\r\n");
        return;
    }

    if((app_ptr) && (app_ptr->is_used))
    {
        LOG_E(AVRCP,"avrcp_svc_newconn: app_ptr is used\r\n");
        return;
    }

    if(!app_ptr->is_used)
    {
        err = avrcp_app_init(app_ptr);
        if(err)
            LOG_E(CONN,"avrcp_app_init err: %d\r\n",err);
    }

    if((NULL == app_ptr) || app_ptr->is_used || app_ptr->conn_req)
    {
        LOG_E(CONN, "bt_avrcp_ctrl_conn_reject\r\n");
        bt_avrcp_ctrl_conn_reject(conn_req);
        return;
    }
    
    app_ptr->conn_req      = conn_req;
    app_ptr->conn_req_mode = mode;
    btaddr_copy(&app_ptr->raddr, raddr);

    LOG_D(CONN, "avrcp_svc_newconn, src-id:%x\r\n", app_ptr->svc_id);

    /* Automatic accept any incoming connection */
    len = j_snprintf(strbuf, sizeof(strbuf), "%ld", app_ptr->svc_id);
    avrcp_cmd_accept(app_ptr, strbuf, len);
}

uint8_t avrc_last_tid_save_before_notif = AVRCP_MAX_TRANSACTIONS; //for the bug: button_playpause_action dosen't work sometime
static void avrcp_paththrough_rsp(bt_avrcp_ctrl_session_h session, avrcp_tid_t tid, 
                                          avc_response_t rsp, bt_app_ctx_h app_ctx)
{
    avrcp_app_t *app_ptr;

    app_ptr = avrcp_get_app_from_session(session);

    LOG_I(AVRCP,"avrcp-reponse:%p,tid:%d,last_tid:%d\r\n", app_ctx, tid,app_ptr->last_tid);

    if(tid == app_ptr->last_tid)
    {
        avrcp_clear_flag(app_ptr, APP_FLAG_AVCRP_PROCESSING);
    }
    else if(tid == avrc_last_tid_save_before_notif) //for the bug: button_playpause_action dosen't work sometime
    {
       	avrcp_clear_flag(app_ptr, APP_FLAG_AVCRP_PROCESSING);
    	avrc_last_tid_save_before_notif = AVRCP_MAX_TRANSACTIONS;
    	LOG_E(AVRCP,"===!!!===notif before rsp===!!!===\r\n");
    }
}

static void avrcp_handle_continuation(avrcp_app_t *app_ptr,
                                               avrcp_pkt_type_t packet_type,
                                               avrcp_op_t continuation_tag,
                                               BOOL *continuation_required)
{
#ifdef BEKEN_DEBUG
    char strbuf[NOTIFY_BUFFER_SIZE];
    uint32_t len;

    /* During continuation we expect to receive AVRCP_PKT_CONT or AVRCP_PKT_END
     * packets with the same continuation tag received previously. Receiving of
     * any other continuation tag is considered by the application as an abort
     * from the target device */
    if(app_ptr->continuation_tag && app_ptr->continuation_tag != continuation_tag)
    {
        len = j_snprintf(strbuf, sizeof(strbuf),"Command in progress was "
            "aborted by the target device");

        frontend_notify(strbuf, len);
    }
#endif

    *continuation_required = FALSE;

    /* Update the global context's continuation_tag if we are in continuation or
     * zero it if not */
    switch (packet_type)
    {
        case AVRCP_PKT_START:
            app_ptr->continuation_tag = continuation_tag;
            *continuation_required = TRUE;
            break;
            
        case AVRCP_PKT_CONT:
            *continuation_required = TRUE;
            break;
            
        case AVRCP_PKT_END:
        case AVRCP_PKT_SINGLE:
            app_ptr->continuation_tag = 0;
            break;
            
        default:
            break;
    }
    /* Consider handling errors that can occur during continuation, such as
     * receiving AVRCP_PKT_CONT packet type before receiving AVRCP_PKT_START */
}

extern void a2dp_volume_init(int8_t aud_volume);
void avrcp_volume_sync()
{
	uint8_t volume;
	//static uint8_t vol_flag=0;
	//app_env_handle_t  env_h = app_env_get_handle();
    
    if(g_avrcp_current_app && avrcp_current_support_vol_syn() && (!(g_avrcp_current_app->flag & APP_FLAG2_VOL_SYNC_OK) /*|| (vol_flag==0)*/))
    {
#if 0
    	if (vol_flag == 0)
    	{
    		vol_flag = 1;
			a2dp_volume_init(env_h->env_cfg.system_para.vol_a2dp);
			if(! bt_audio_ch_busy())
                aud_dac_set_volume(env_h->env_cfg.system_para.vol_a2dp);
    	}
#endif
        volume = get_tg_volume_value();

        LOG_I(AVRCP,"avrcp_volume_sync:%x\r\n",volume);

        if(volume == 0x7f)
        {
            volume -= 1;
            current_send_volume_change_response(volume);
            volume += 1;
            current_send_volume_change_response(volume);
        }
        else
        {
            volume += 1;
            current_send_volume_change_response(volume);
            volume -= 1;
            current_send_volume_change_response(volume);
        }
        g_avrcp_current_app->flag |= APP_FLAG2_VOL_SYNC_OK;
    }
}

static void avrcp_getcaps_events_rsp(bt_avrcp_ctrl_session_h session,
                                              avrcp_tid_t tid,
                                              avc_response_t response,
                                              avrcp_event_id_t *events,
                                              uint8_t count,
                                              avrcp_pkt_type_t packet_type,
                                              avrcp_op_t bt_avrcp_continuation_tag,
                                              bt_app_ctx_h app_ctx)
{
    uint8_t i;
    
    for (i = 0; i < count; i++)
    {
       if(events[i]==AVRCP_EVENT_PLAYBACK_STATUS_CHANGED)
       {
           avrcp_cmd_register_notification(session,AVRCP_EVENT_PLAYBACK_STATUS_CHANGED,0);
       }
    }
}
#if (CONFIG_OTHER_FAST_PLAY == 1)
extern result_t  select_current_a2dp_avrcp(btaddr_t *addr);
extern result_t  select_other_a2dp_avrcp(btaddr_t *addr);
#endif
static void avrcp_event_rsp(bt_avrcp_ctrl_session_h session,
                                  avrcp_tid_t tid,
                                  avc_response_t response,
                                  avrcp_event_rsp_t *ev,
                                  avrcp_pkt_type_t packet_type,
                                  avrcp_op_t bt_avrcp_continuation_tag,
                                  bt_app_ctx_h app_ctx)
{
    BOOL dummy;       /* No need to handle continuation in this scenario */
    avrcp_app_t *app_ptr;
#if (CONFIG_OTHER_FAST_PLAY == 1)
	static btaddr_t s_addrtmp;
	app_handle_t sys_hdl = app_get_sys_handler();
#endif
    app_ptr = avrcp_get_app_from_session(session);

    avrcp_handle_continuation(app_ptr, packet_type, bt_avrcp_continuation_tag, &dummy);

    switch (ev->event_id)
    {
        case AVRCP_EVENT_PLAYBACK_STATUS_CHANGED:
        {
            uint8_t status = ev->params.playback_status_changed.play_status;

            if(response==AVC_RESPONSE_CHANGED)
            {
                switch(status)
                {
                case AVRCP_PLAY_STATUS_PLAYING:
                	LOG_I(AVRCP,"%s:---PLAYING---\r\n", backend_unit_get_name(&(app_ptr->raddr)));
                    a2dp_current_set_flag(APP_BUTTON_FLAG_PLAY_PAUSE); //++ by Borg @230220
				#if (CONFIG_A2DP_PLAYING_SET_AG_FLOW == 0)
                	select_other_avrcp_app_pause_by_raddr(&(app_ptr->raddr));
				#else
					#if (CONFIG_OTHER_FAST_PLAY == 1)
                    set_a2dp_array_play_pause_status(&(app_ptr->raddr), 1);
					jtask_stop(sys_hdl->app_auto_con_task);
					if ((sys_hdl->otherplay) && (!sys_hdl->fast_state) && !btaddr_same(&s_addrtmp,&(app_ptr->raddr)))
						jtask_schedule(sys_hdl->app_a2dp_task,2000,(jthread_func)select_current_a2dp_avrcp,(void *)&(app_ptr->raddr));
					sys_hdl->otherplay = 0;
                    sys_hdl->fast_state=0;
					#endif
				#endif
                #if (CONFIG_EAR_IN == 1)
                    app_ear_button_playing_syn_status();
                #else
                    app_bt_flag1_set(APP_BUTTON_FLAG_PLAY_PAUSE, 1);//++ by Borg @230220
				#endif
                	break;

                case AVRCP_PLAY_STATUS_PAUSED:
                case AVRCP_PLAY_STATUS_STOPPED:
                    {
                	    LOG_I(AVRCP,"%s:---PAUSED||STOPPED---\r\n", backend_unit_get_name(&(app_ptr->raddr)));
                        a2dp_current_clear_flag(APP_BUTTON_FLAG_PLAY_PAUSE); //++ by Borg @230220
					#if (CONFIG_OTHER_FAST_PLAY == 1)
                        sys_hdl->fast_state=0;
                        set_a2dp_array_play_pause_status(&(app_ptr->raddr), 0);
						if(a2dp_check_flag_on_raddr(&(app_ptr->raddr), APP_A2DP_PRIVATE_FLAG_FLOW_CTRL))
						{
							//os_printf("self_flow_ctrl\r\n");	
						}
						else if(a2dp_has_another_music(&(app_ptr->raddr)))
						{
							//os_printf("OTHER_play\r\n");
							sys_hdl->otherplay = 500;
							jtask_stop(sys_hdl->app_a2dp_task);
							memcpy((uint8 *)&s_addrtmp, (uint8 *)&(app_ptr->raddr), sizeof(btaddr_t));
							app_bt_shedule_task((jthread_func)select_other_a2dp_avrcp,(void *)&(app_ptr->raddr),2000);
						}
						else if (a2dp_has_another_connection(&(app_ptr->raddr)))
                        {
                            //os_printf("OTHER_conn\r\n");
							sys_hdl->otherplay = 500;
							jtask_stop(sys_hdl->app_a2dp_task);
                            memcpy((uint8 *)&s_addrtmp, (uint8 *)&(app_ptr->raddr), sizeof(btaddr_t));
							app_bt_shedule_task((jthread_func)select_current_a2dp_avrcp,(void *)&(app_ptr->raddr),3000);
                        }
					#endif
                    #if (CONFIG_EAR_IN == 1)
                    	app_ear_button_stoppause_syn_status();
                    #else
                        app_bt_flag1_set(APP_BUTTON_FLAG_PLAY_PAUSE, 0);//++ by Borg @230220
                  	#endif
                    }
                	break;

                case AVRCP_PLAY_STATUS_FWD_SEEK:
                	LOG_I(AVRCP,"%s:---FWD_SEEK---\r\n", backend_unit_get_name(&(app_ptr->raddr)));
                	break;
                    
                case AVRCP_PLAY_STATUS_REV_SEEK:
                	LOG_I(AVRCP,"%s:---REV_SEEK---\r\n", backend_unit_get_name(&(app_ptr->raddr)));
                	break;
                }

                if(app_ptr->flag & APP_FLAG_AVCRP_PROCESSING)  //for the bug: button_playpause_action dosen't work sometime
                {
                    avrc_last_tid_save_before_notif = app_ptr->last_tid;
                    LOG_I(AVRCP,"===avrcp last tid:%d\r\n",avrc_last_tid_save_before_notif);
                    avrcp_clear_flag(app_ptr, APP_FLAG_AVCRP_PROCESSING);
                }

                avrcp_cmd_register_notification(session,AVRCP_EVENT_PLAYBACK_STATUS_CHANGED,0);
            }
            else if(response==AVC_RESPONSE_INTERIM)
            {
                switch(status)
                {
                    case AVRCP_PLAY_STATUS_PLAYING:
                        INFO_PRT("INTERIM PLAYING\r\n");
                    #if (CONFIG_EAR_IN == 1)
                        app_ear_button_playing_syn_status();
                    #endif
                        break;
                    case AVRCP_PLAY_STATUS_PAUSED:
                    case AVRCP_PLAY_STATUS_STOPPED:
                        INFO_PRT("INTERIM PAUSED|STOPPED\r\n");
                    #if (CONFIG_EAR_IN == 1)
                        app_ear_button_stoppause_syn_status();
                    #endif
                        break;
                }
            }
            break;
        }

        case AVRCP_EVENT_TRACK_CHANGED:
            break;

        default:                     
            break;
    }
}

static void avrcp_get_nowplaying_attr_rsp(bt_avrcp_ctrl_session_h session,
    avrcp_tid_t tid, avc_response_t rsp, uint32_t attr_id, uint16_t char_set_id,
    uint16_t attr_total_len, char *attr_val, uint16_t attr_val_len,
    avrcp_pkt_type_t packet_type, avrcp_op_t bt_avrcp_continuation_tag,
    bt_app_ctx_h app_ctx_h)
{
#ifdef BEKEN_DEBUG
    char strbuf[NOTIFY_BUFFER_SIZE];
    uint32_t len;
#endif
    BOOL continuation_required;
    avrcp_app_t *app_ptr;

    app_ptr = avrcp_get_app_from_session(session);

    DECLARE_FNAME("avrcp_get_nowplaying_attr_rsp");

    avrcp_handle_continuation(app_ptr,
                              packet_type,
                              bt_avrcp_continuation_tag,
                              &continuation_required);

   /* Update the global context continuation parameters that are specific to
    * this command. */
    switch (packet_type)
    {
        case AVRCP_PKT_START:
            app_ptr->attr_id = attr_id;
            app_ptr->char_set_id = char_set_id;
            break;

        case AVRCP_PKT_CONT:
            attr_id = app_ptr->attr_id;
            char_set_id = app_ptr->char_set_id;
            break;

        case AVRCP_PKT_END:
            attr_id = app_ptr->attr_id;
            char_set_id = app_ptr->char_set_id;
            app_ptr->attr_id = 0;
            app_ptr->char_set_id = 0;
            break;

        default:
            break;
    }

#ifdef BEKEN_DEBUG
    len = j_snprintf(strbuf, sizeof(strbuf),"    attr_id %s, char_set_id %d, "
                     "attr_len 0x%x attr_val%s: %.*s\n", avrcp_media_attr_id_str(attr_id),
                     char_set_id, attr_val_len, continuation_required ? "[cont']" : "",
                     attr_val_len, attr_val);

    frontend_notify(strbuf, len);
#endif

    if(continuation_required)
    {
        /* XXX: Spec doesn't define behaviour for continuation command Tid */
        app_ptr->last_tid = (app_ptr->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;

        /* Call for continuation */
        bt_avrcp_send_request_continuing_response_cmd(session, app_ptr->last_tid,
                                                      bt_avrcp_continuation_tag);
    }
}

static void avrcp_get_play_status_rsp(bt_avrcp_ctrl_session_h session,
                                              avrcp_tid_t tid, avc_response_t rsp,
                                              avrcp_get_play_status_rsp_t *play_status, 
                                              avrcp_pkt_type_t packet_type,
                                              avrcp_op_t bt_avrcp_continuation_tag, 
                                              bt_app_ctx_h app_ctx)
{
#ifdef BEKEN_DEBUG
    char str[NOTIFY_BUFFER_SIZE];
    uint32_t len;
#endif
    BOOL dummy;       /* No need to handle continuation in this scenario */
    avrcp_app_t *app_ptr;

    app_ptr = avrcp_get_app_from_session(session);

    avrcp_handle_continuation(app_ptr, packet_type, bt_avrcp_continuation_tag, &dummy);

#ifdef BEKEN_DEBUG
    len = j_snprintf(str, sizeof(str), "avrcp play status: \n  "
                     "song length %ld sec\n  song position %ld sec\n  play status %s\n",
                     ua_get_be32(play_status->song_length),
                     ua_get_be32(play_status->song_position),
                     avrcp_play_status_str(play_status->play_status));

    frontend_notify(str, len);
#endif
}

result_t avrcp_cmd_connect(char *params, unsigned int len)
{
    result_t err;
    avrcp_app_t *app_ptr;
    btaddr_t laddr, raddr;
    unsigned int i, unit_id, tmp[6];

    if(j_snscanf(params,
       NULL,
       "%u " BTADDR_FORMAT,
       &unit_id,
       &tmp[5],
       &tmp[4],
       &tmp[3],
       &tmp[2],
       &tmp[1],
       &tmp[0]) != 7)
    {
        return UWE_PARAM;
    }

    for (i = 0; i < 6; i++)
    {
        raddr.b[i] = (uint8_t)tmp[i];
    }

    app_ptr = avrcp_app_lookup_valid(&raddr);

    if(app_ptr == NULL)
    {
        LOG_W(AVRCP,"avrcp_cmd_connect NULL\r\n");
        return UWE_INVAL;
    }

    if((app_ptr) && (app_ptr->is_used))
    {
        LOG_E(AVRCP,"avrcp_cmd_connect: app_ptr is used\r\n");
        return UWE_ALREADY;
    }

    if(app_ptr && !app_ptr->is_used)
    {        
        err = avrcp_app_init(app_ptr);
        if(err)
            LOG_E(AVRCP,"avrcp_cmd_connect:app init err\r\n");
    }

    err = backend_unit_get_addr(unit_id, &laddr);
    if(err)
    {
        LOG_E(AVRCP,"avrcp_cmd_connect err at get_addr\r\n");
        goto Exit;
    }

    app_ptr->svc_id++;

    err = bt_avrcp_ctrl_conn_connect(app_ptr->session, &laddr, &raddr);
    if(err)
    {
        LOG_E(AVRCP,"avrcp_cmd_connect err at connect\r\n");
        goto Exit;
    }

    btaddr_copy(&app_ptr->raddr, &raddr);
    app_ptr->is_used = 1;

    LOG_D(AVRCP,"avrcp_cmd_connect svc-id:%x\r\n", app_ptr->svc_id);

Exit:
    return err;
}

result_t avrcp_cmd_disconnect(void)
{
    if(!g_avrcp_current_app->session)
    {
        LOG_W(AVRCP,"No avrcp app\r\n");
        return UWE_NODEV;
    }
    bt_avrcp_ctrl_conn_disconnect(g_avrcp_current_app->session);

    return UWE_OK;
}

result_t avrcp_cmd_register_notification(bt_avrcp_ctrl_session_h session,avrcp_event_id_t event_id,
                                                  uint32_t playback_pos_changed_interval)
{
    result_t err;
    avrcp_app_t *app_ptr;
    app_ptr = avrcp_get_app_from_session(session);

    app_ptr->last_tid = (app_ptr->last_tid + 1) % AVRCP_MAX_TRANSACTIONS;

    err = bt_avrcp_send_register_notification_cmd(app_ptr->session, app_ptr->last_tid,
                                                  event_id, playback_pos_changed_interval);
    if(err)
        goto Exit;

    //os_printf("avrcp-transaction-label:%x\r\n", app_ptr->last_tid);
Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
    return err;
}

static result_t avrcp_cmd_accept(avrcp_app_t *app_ptr, char *params, unsigned int len)
{
    result_t err;
    uint32_t svc_id;

    if(j_snscanf(params, NULL, "%u", &svc_id) != 1)
    {
        LOG_E(CONN, "avrcp_cmd_accept:%s\r\n",params);
        return UWE_INVAL;
    }

    if(svc_id != app_ptr->svc_id)
    {
        LOG_E(CONN, "avrcp_cmd_accept:%d!=%d\r\n",svc_id,app_ptr->svc_id);
        return UWE_PARAM;
    }

    if(app_ptr->is_used)
    {
        LOG_E(CONN, "avrcp_cmd_accept:app_ptr is used\r\n");
        return UWE_ISCONN;
   }

    if(!app_ptr->conn_req)
    {
        LOG_E(CONN, "avrcp_cmd_accept:wrong state\r\n");
        return UWE_STATE;
    }

    app_ptr->svc_id++;

    err = bt_avrcp_ctrl_conn_accept(app_ptr->session,
                                    app_ptr->conn_req,
                                    app_ptr->conn_req_mode);
    if(err)
        goto Exit;

    app_ptr->is_used = 1;

    LOG_I(CONN,"avrcp_cmd_accept, svc id:%x\r\n", app_ptr->svc_id);

Exit:
    app_ptr->conn_req = NULL;
    return err;
}

static void avrcp_set_flag(avrcp_app_t *app_ptr, uint32_t flag)
{
    app_ptr->flag |= flag;
}

static void avrcp_clear_flag(avrcp_app_t *app_ptr, uint32_t flag)
{
    app_ptr->flag &= ~flag;
}

uint32_t avrcp_get_raddr(void *arg, btaddr_t *raddr)
{
    avrcp_app_t *app_ptr;

    app_ptr = (avrcp_app_t *)arg;
    
    if(!app_ptr->is_used)
    {
        return UWE_INVAL;
    }

    btaddr_copy(raddr, &(app_ptr->raddr));

    return UWE_OK;
}

void avrcp_select_current_app_to_another(const btaddr_t *raddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used)
            && (g_avrcp_array[i].flag & APP_FLAG_AVCRP_CONNECTION)
            && (!btaddr_same(&g_avrcp_array[i].raddr, raddr)))
        {
            g_avrcp_current_app = &g_avrcp_array[i];
            return;
        }
    }
    
    g_avrcp_current_app = NULL;
    LOG_D(AVRCP,"====current_avrcp_ptr is null====\r\n");
}

uint32_t avrcp_update_current_app(const btaddr_t *raddr)
{
    uint32_t i;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_avrcp_array[i].is_used) 
            && (g_avrcp_array[i].flag & APP_FLAG_AVCRP_CONNECTION)
            && (btaddr_same(&g_avrcp_array[i].raddr, raddr)))
        {
            g_avrcp_current_app = &g_avrcp_array[i];
            return 0;
        }
    }

    return 1;
}

void select_other_avrcp_app_pause_by_raddr(btaddr_t *raddr)
{
	uint32_t i;
    avrcp_app_t *app_ptr = NULL;
    BOOL   avrcp_find = FALSE;
        
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
	    if((g_avrcp_array[i].is_used)
            && (g_avrcp_array[i].session)
            && (!btaddr_same(&g_avrcp_array[i].raddr, raddr))
            && a2dp_has_another_music(raddr))
        {
	        avrcp_find = TRUE;
	        app_ptr = &g_avrcp_array[i];
        	break;
        }
    }
    
    if(avrcp_find)
    {
        LOG_I(AVRCP,"===other avrcp send pause\r\n");
        avrcp_send_opcode(app_ptr,(void *)AVC_OP_PAUSE);
    }
}

btaddr_t *avrcp_get_raddr_from_session(bt_avrcp_ctrl_session_h session)
{
    return &avrcp_get_app_from_session(session)->raddr;
}
// EOF
