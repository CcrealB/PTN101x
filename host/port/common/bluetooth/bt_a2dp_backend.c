/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <bt_api.h>
#include <bt_sdp.h>
#include <bt_sdp_types.h>
#include <bt_a2dp_sink.h>
#include <audio_out_interface.h>
#include "bt_app_internal.h"
#include "driver_beken_includes.h"
#include "driver_audio.h"
#include "beken_external.h"
#include "app_beken_includes.h"
#include "app_vendor_decode.h"
#include "bt_a2dp_vendor_decode.h"
#include "bt_a2dp_mpeg_aac_decode.h"

uint32_t a2dp_app_uninit(a2dp_backend_t *app_ptr);
static void a2dp_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr, const btaddr_t *raddr,
                                bt_link_modes_mask_t mode, bt_app_ctx_h app_ctx);
static void a2dp_connected(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx);
static void a2dp_disconnected(bt_a2dp_sink_session_h session_h, result_t status, bt_app_ctx_h app_ctx);
static void a2dp_set_configuration_cb(bt_a2dp_sink_session_h session_h, const bt_a2dp_codec_t *codec, 
                                               int32_t local_ep_id, int32_t remote_ep_id, bt_app_ctx_h app_ctx_h);
static void a2dp_stream_start_cb(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx);
static void a2dp_stream_suspend_cb(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx);
static void a2dp_stream_input_cb(bt_a2dp_sink_session_h session_h, void *buf,
                                         uint32_t len, bt_app_ctx_h app_ctx_h);
#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
void init_security_control_r_addr(btaddr_t * input_r_addr);
void uinit_security_control_r_addr(btaddr_t * input_r_addr);
#endif

CONST static bt_a2dp_sink_conn_cbs_t conn_cbs = {
    a2dp_connected,
    a2dp_disconnected,
};

static sdp_a2dp_profile_t a2dp_profile = {
    BT_SVC_TYPE_AUDIO_SINK,             /* Sink or Source */
    SDP_A2DP_SPEAKER|SDP_A2DP_HEADPHONE,/* Supported features */
    "Aud-Snk"                        /* Service name */
};

static const bt_a2dp_sink_cbs_t a2dp_cbs = {
    a2dp_set_configuration_cb,
    a2dp_stream_start_cb,
    a2dp_stream_suspend_cb,
    a2dp_stream_input_cb
};

static bt_a2dp_sink_session_h g_a2dp_srv;
static a2dp_backend_t *g_current_a2dp_ptr;
static a2dp_backend_t g_a2dp_array[2];

a2dp_backend_t *a2dp_get_app_from_svc(bt_a2dp_sink_session_h svc)
{
    uint32_t i;
    a2dp_backend_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i++)
    {
        if(g_a2dp_array[i].is_used && (svc == g_a2dp_array[i].svc))
        {
            app_ptr = &g_a2dp_array[i];
            break;
        }
    }

    if (app_ptr == NULL)
        LOG_I(A2DP,"a2dp_get_app_from_svc is NULL\r\n");

    return app_ptr;
}

uint32_t a2dp_all_apps_is_unused(void)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used))
        {
            return 0;
        }
    }

    return 1;
}

a2dp_backend_t *a2dp_app_lookup_valid(const btaddr_t* raddr)
{
    uint32_t i;
    a2dp_backend_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used) && (btaddr_same(raddr, &g_a2dp_array[i].raddr)))
        {
            return &g_a2dp_array[i];
        }
    }

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(!g_a2dp_array[i].is_used)
        {
            return &g_a2dp_array[i];
        }
    }
    
    return app_ptr;
}

uint32_t a2dp_has_music(void)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used) && (g_a2dp_array[i].flag & APP_FLAG_MUSIC_PLAY))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t a2dp_has_the_music(const btaddr_t *raddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used)
            && (btaddr_same(&g_a2dp_array[i].raddr, raddr))
            && (g_a2dp_array[i].flag & APP_FLAG_MUSIC_PLAY))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t a2dp_has_another_music(const btaddr_t *raddr)
{
    uint32_t i;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if( g_a2dp_array[i].is_used
			&& (!btaddr_same(&g_a2dp_array[i].raddr, raddr))
			&& (g_a2dp_array[i].flag & APP_FLAG_MUSIC_PLAY) )
    	{
            return 1;
    	}
    }
    
    return 0;
}

uint32_t a2dp_has_connection(void)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used) && (g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t a2dp_has_the_connection(const btaddr_t *raddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used)
            && btaddr_same(&g_a2dp_array[i].raddr, raddr)
            && (g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION) )
        {
            return 1;
        }
    }

    return 0;
}

uint32_t a2dp_has_another_connection(const btaddr_t *raddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used)
            && !btaddr_same(&g_a2dp_array[i].raddr, raddr)
            && (g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION) )
        {
            return 1;
        }
    }

    return 0;
}

void set_a2dp_array_play_pause_status(const btaddr_t *raddr, uint8_t status)
{
    uint8_t forCount = 0;

    for(forCount = 0; forCount < BT_MAX_AG_COUNT; forCount ++)
    {
        if((g_a2dp_array[forCount].is_used)
            && btaddr_same(&g_a2dp_array[forCount].raddr, raddr)
            && (g_a2dp_array[forCount].flag & APP_FLAG_A2DP_CONNECTION) )
        {
            g_a2dp_array[forCount].avrcp_play_pause = status;
        }
    }
}

uint8_t get_a2dp_array_play_pause_status(const btaddr_t *raddr)
{
    uint8_t forCount = 0;

    for(forCount = 0; forCount < BT_MAX_AG_COUNT; forCount ++)
    {
        if((g_a2dp_array[forCount].is_used)
            && btaddr_same(&g_a2dp_array[forCount].raddr, raddr)
            && (g_a2dp_array[forCount].flag & APP_FLAG_A2DP_CONNECTION) )
        {
            return g_a2dp_array[forCount].avrcp_play_pause;
        }
    }
    return 0xff; //miss match
}

uint32_t a2dp_has_another_music_avrcp_playing(const btaddr_t *raddr)
{
    uint32_t i;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if( g_a2dp_array[i].is_used
			&& (!btaddr_same(&g_a2dp_array[i].raddr, raddr))
			&& (g_a2dp_array[i].flag & APP_FLAG_MUSIC_PLAY)
		//#if (CONFIG_OTHER_FAST_PLAY == 0)
			&& (g_a2dp_array[i].avrcp_play_pause == 1)
		//#endif
			)
    	{
            return 1;
    	}
    }
    
    return 0;
}



void *a2dp_get_svc_based_on_raddr(const btaddr_t *raddr)
{
    uint32_t i;
    void *svc = NULL;
    a2dp_backend_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_a2dp_array[i];
        if((app_ptr) && (app_ptr->svc) && btaddr_same(&app_ptr->raddr, raddr))
        {
            svc = app_ptr->svc;
            break;
        }
    }

    return svc;
}

uint32_t a2dp_check_flag_on_raddr(const btaddr_t *raddr, uint32_t flag)
{
    int i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_a2dp_array[i].is_used && btaddr_same(raddr, &g_a2dp_array[i].raddr))
        {
            return g_a2dp_array[i].flag & flag;
        }
    }

    return 0;
}

void a2dp_set_flag(a2dp_backend_t *app_ptr, uint32_t flag)
{
    if(app_ptr)
        app_ptr->flag |= flag;
}

void a2dp_clear_flag(a2dp_backend_t *app_ptr, uint32_t flag)
{
    if(app_ptr)
        app_ptr->flag &= ~flag;
}

void a2dp_current_set_flag(uint32_t flag)
{
    if(g_current_a2dp_ptr)
        g_current_a2dp_ptr->flag |= flag;
}

void a2dp_current_clear_flag(uint32_t flag)
{
    if(g_current_a2dp_ptr)
        g_current_a2dp_ptr->flag &= ~flag;
}

uint32_t a2dp_current_check_flag(uint32_t flag)
{
	if(g_current_a2dp_ptr)
    	return g_current_a2dp_ptr->flag & flag;
    
    return 0;
}

a2dp_backend_t *a2dp_get_current_app(void)
{
    return g_current_a2dp_ptr;
}

void a2dp_update_current_app(a2dp_backend_t *app_ptr)
{
    g_current_a2dp_ptr = app_ptr;
}

void a2dp_select_current_app_to_another(btaddr_t *addr)
{
	uint32_t i;
    
	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if((g_a2dp_array[i].is_used)
            && (g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION)
            && (!btaddr_same(addr, &g_a2dp_array[i].raddr)))
        {
            g_current_a2dp_ptr = &g_a2dp_array[i];
        	return;
        }
	}
    
    g_current_a2dp_ptr = NULL;
    LOG_D(A2DP,"====current_a2dp_ptr is null====\r\n");
}

result_t get_play_a2dp_avrcp_state(uint32_t *index)
{
	uint32_t i;
	result_t err = UWE_INVAL;
    
	*index = 0;
	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if((g_a2dp_array[i].is_used)
            && (g_a2dp_array[i].flag & (APP_BUTTON_FLAG_PLAY_PAUSE | APP_FLAG_MUSIC_PLAY))
            && (!(g_a2dp_array[i].flag & APP_A2DP_PRIVATE_FLAG_FLOW_CTRL)))
        {
        	err = UWE_OK;
        	*index = i;
			break;
        }
	}
	return err;
}

void select_play_a2dp_avrcp(void *arg)
{
	uint32_t index;
    
	if(UWE_OK == get_play_a2dp_avrcp_state(&index))
	{
		 g_current_a2dp_ptr = &g_a2dp_array[index];
         sbc_stream_start_init(g_current_a2dp_ptr->freq);
		 app_set_led_event_action(LED_EVENT_BT_PLAY);
	}
}

btaddr_t *a2dp_get_current_app_remote_addr(void)
{
   return &(g_current_a2dp_ptr->raddr);
}

btaddr_t *a2dp_get_not_active_remote_addr(void)
{
    uint32_t i;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if( g_a2dp_array[i].is_used
			&& (!btaddr_same(&g_a2dp_array[i].raddr, &g_current_a2dp_ptr->raddr)) )
    	{
            return &g_a2dp_array[i].raddr;
    	}
    }

   return NULL;
}

uint32_t a2dp_get_remote_addr(void *arg, btaddr_t *r_addr)
{
    a2dp_backend_t *app_ptr = (a2dp_backend_t *)arg;

    btaddr_copy(r_addr, &app_ptr->raddr); 

    return UWE_OK;
}

void *a2dp_get_current_session(void)
{
    bt_a2dp_sink_session_h session_h;

    session_h = 0;
    if(g_current_a2dp_ptr)
    {
        session_h = g_current_a2dp_ptr->svc;
    }

    return session_h;
}

uint32_t get_a2dp_priv_flag(uint8_t idx, uint32_t flag)
{
    if(g_a2dp_array[idx & 0x01].is_used)
        return(g_a2dp_array[idx & 0x01].flag & flag);
    
    return 0;
}

#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
hci_link_t *select_curr_playing_link_handle_by_a2dp(a2dp_backend_t *app_ptr)
{
    hci_link_t * tmp_link = NULL;
    app_handle_t sys_hdl;
    
    sys_hdl = app_get_sys_handler();
    if(app_ptr)
    {
    	a2dp_set_flag(app_ptr,APP_A2DP_PRIVATE_FLAG_FLOW_CTRL);
    	tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &app_ptr->raddr, HCI_LINK_ACL);
		return tmp_link;
    }
	return NULL;
}

hci_link_t *select_flow_ctrl_link_handle_by_raddr(btaddr_t *raddr)
{
    int i;
	a2dp_backend_t *app_ptr=NULL;
    hci_link_t * tmp_link = NULL;
    app_handle_t sys_hdl;
    
    sys_hdl = app_get_sys_handler();
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_a2dp_array[i];
        if (app_ptr && (app_ptr->flag & APP_A2DP_PRIVATE_FLAG_FLOW_CTRL)
			&& btaddr_same(raddr, &(app_ptr->raddr)))
        {
        	//os_printf("addr_clear_flow\r\n");
       		a2dp_clear_flag(app_ptr,APP_A2DP_PRIVATE_FLAG_FLOW_CTRL);
	    	tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &app_ptr->raddr, HCI_LINK_ACL);
			return tmp_link;
        }
    }
	return NULL;
}

hci_link_t *select_flow_ctrl_link_handle(void)
{
    int i;
	a2dp_backend_t *app_ptr=NULL;
    hci_link_t * tmp_link = NULL;
    app_handle_t sys_hdl;
    
    sys_hdl = app_get_sys_handler();
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_a2dp_array[i];
        if (app_ptr && (app_ptr->flag & APP_A2DP_PRIVATE_FLAG_FLOW_CTRL))
        {
        	//os_printf("clear_flow\r\n");
       		a2dp_clear_flag(app_ptr,APP_A2DP_PRIVATE_FLAG_FLOW_CTRL);
	    	tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &app_ptr->raddr, HCI_LINK_ACL);
			return tmp_link;
        }
    }
	return NULL;
}
#endif

void a2dp_volume_init(int8_t aud_volume)
{
#if (CONFIG_CUSTOMER_ENV_SAVE_VOLUME == 1)
	app_handle_t app_h = app_get_sys_handler();

	app_h->linein_vol = aud_volume;
#endif
    if(g_current_a2dp_ptr){
    	g_current_a2dp_ptr->volume = aud_volume;
    }
}

void a2dp_volume_init_based_on_raddr(int8_t aud_volume, const btaddr_t *raddr)
{
	a2dp_backend_t *app_ptr=NULL;
    int i;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_a2dp_array[i];
        if (app_ptr && btaddr_same(raddr, &(app_ptr->raddr)))
        {
            app_ptr->volume = aud_volume;
			break;
        }
    }
}

int8_t a2dp_get_volume(void)
{
	static int8_t VolR=0;
	if(g_current_a2dp_ptr != NULL)	VolR = g_current_a2dp_ptr->volume;
	return VolR;
}

int8_t a2dp_get_volume_based_on_raddr(const btaddr_t *raddr)
{
    int i;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if (g_a2dp_array[i].is_used && btaddr_same(raddr, &(g_a2dp_array[i].raddr)))
        {
            return g_a2dp_array[i].volume;
        }
    }
    return AUDIO_VOLUME_MAX+1;
}

uint32_t a2dp_get_freq(void)
{
    return g_current_a2dp_ptr->freq;
}

//***********************************************
result_t a2dp_volume_adjust(uint8_t oper)
{
    a2dp_backend_t *app_ptr;

    app_ptr = g_current_a2dp_ptr;

    if(oper == 0){ //minus
        app_ptr->volume--;
        if(app_ptr->volume <= AUDIO_VOLUME_MIN){
            app_ptr->volume = AUDIO_VOLUME_MIN;
        }
    }else{  //plus
        app_ptr->volume++;
        if(app_ptr->volume >= AUDIO_VOLUME_MAX){
            app_ptr->volume = AUDIO_VOLUME_MAX;
        }
    }

    if(a2dp_has_music()){
        if(app_ptr->volume <= AUDIO_VOLUME_MIN){
            //os_printf("a2dp_volume_adjust1\r\n");
            if(!bt_audio_ch_busy()){
//yuan++		aud_volume_mute(1);
            }

			app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 1);
        }else{
            //os_printf("a2dp_volume_adjust2\r\n");
//yuan++	aud_volume_mute(0);
			app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 0);
        }
    }

#if (CONFIG_CUSTOMER_ENV_SAVE_VOLUME == 1)
	a2dp_volume_init(app_ptr->volume);
#endif

    if(!bt_audio_ch_busy()){
        aud_dac_set_volume(app_ptr->volume);
    }

    app_wave_file_aud_notify(app_ptr->freq, app_ptr->channel, app_ptr->volume);

    if(app_ptr->volume == AUDIO_VOLUME_MAX){ // max
        LOG_I(VOL,"volume max\r\n");
//yuan++	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
    }else if(app_ptr->volume == AUDIO_VOLUME_MIN){ // min
        LOG_I(VOL,"volume min\r\n");
//yuan++	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
    }
    return 0;
}

static result_t a2dp_server_start(void)
{
    result_t err;
    bt_a2dp_sink_session_h srv = g_a2dp_srv;
    DECLARE_FNAME("a2dp_server_start");

    err = bt_a2dp_sink_server_start(&srv,
                                    BTADDR_ANY,
                                    a2dp_newconn,
                                    NULL);
    if(err)
        goto Exit;

    err = bt_sdp_service_register(BTADDR_ANY,
                                  BT_SVC_TYPE_AUDIO_SINK,
                                  &a2dp_profile,
                                  sizeof(a2dp_profile),
                                  &srv->record_handle);
    if(err)
        goto Exit;
    
Exit:
    if(err && srv)
        bt_a2dp_sink_server_stop(&srv);

    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
    return err;
}

static void a2dp_server_stop(void)
{
    bt_a2dp_sink_session_h srv = g_a2dp_srv;

    if(srv->record_handle)
    {
        bt_sdp_service_unregister(srv->record_handle);
        srv->record_handle = 0;
    }

    if(srv)
    {
        bt_a2dp_sink_server_stop(&srv);
        srv = NULL;
    }
}

uint32_t a2dp_app_init(a2dp_backend_t *app_ptr)
{
    result_t err;

    //os_printf("a2dp_app_init\r\n");
    
    err = bt_a2dp_sink_conn_create(&app_ptr->svc, &conn_cbs, NULL);

    if(err)
    {
        a2dp_app_uninit(app_ptr);
        LOG_E(A2DP,"a2dp_app_init error!!!\r\n");
    }

    return err;
}

uint32_t a2dp_app_uninit(a2dp_backend_t *app_ptr)
{
    if(app_ptr && app_ptr->is_used)
    {
        if(app_ptr->svc)
        {
            bt_a2dp_sink_conn_destroy(&app_ptr->svc);
        }

        if(app_ptr->conn_req)
        {
            bt_a2dp_sink_conn_reject(app_ptr->conn_req);
        }
        
        j_memset(app_ptr, 0, sizeof(a2dp_backend_t));
        LOG_D(A2DP,"a2dp destroy\r\n");
    }
    return UWE_OK;
}

result_t a2dp_backend_init(void)
{
    result_t err;
    
    g_a2dp_srv = NULL;
    g_current_a2dp_ptr = NULL;
    j_memset(&g_a2dp_array[0], 0, sizeof(a2dp_backend_t));
    j_memset(&g_a2dp_array[1], 0, sizeof(a2dp_backend_t));

    err = bt_a2dp_sink_register(&a2dp_cbs);
    if(err)
    {
        goto Exit;
    }

    err = a2dp_server_start();
    if(err)
    {
        goto Exit;
    }
    
Exit:
    if(err)
    {
        a2dp_backend_uninit();
    }

    LOG_D(A2DP,"a2dp_backend_init\r\n");

    return err;
}

void a2dp_backend_uninit(void)
{
    uint32_t i;
    a2dp_backend_t *app_ptr;

    LOG_D(A2DP,"a2dp_backend_uninit\r\n");
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_a2dp_array[i].is_used)
        {
            app_ptr = &g_a2dp_array[i];
            
            audio_out_close(&app_ptr->audio_out);
            a2dp_app_uninit(app_ptr);
        }
    }

    bt_a2dp_sink_unregister();
    a2dp_server_stop();
}

void a2dp_codec_dump(const bt_a2dp_codec_t *codec)
{
    /* Codec information element */
    switch (codec->type)
    {
        case A2DP_CODEC_SBC:
            {
                const bt_a2dp_sbc_t *p = &codec->u.sbc;

                bt_frontend_notification(" SBC [%02x %02x]", p->octet0, p->octet1);

                bt_frontend_notification(" bitpool %d/%d", p->min_bitpool, p->max_bitpool);

                bt_frontend_notification(" 16KHz %s, 32KHz %s, 44KHz %s, 48KHz %s",
                    p->octet0 & SBC_FREQ_16K ? "yes" : "no",
                    p->octet0 & SBC_FREQ_32K ? "yes" : "no",
                    p->octet0 & SBC_FREQ_44K ? "yes" : "no",
                    p->octet0 & SBC_FREQ_48K ? "yes" : "no");

                bt_frontend_notification(" MONO %s, DUAL %s, STEREO %s, "
                    "JOINT STEREO %s",
                    p->octet0 & SBC_CHANNEL_MONO ? "yes" : "no",
                    p->octet0 & SBC_CHANNEL_DUAL ? "yes" : "no",
                    p->octet0 & SBC_CHANNEL_STEREO ? "yes" : "no",
                    p->octet0 & SBC_CHANNEL_JOINT_STEREO ?"yes":"no");

                bt_frontend_notification(" block 4 %s, 8 %s, 12 %s, 16 %s",
                    p->octet1 & SBC_BLOCK_LEN_4 ? "yes" : "no",
                    p->octet1 & SBC_BLOCK_LEN_8 ? "yes" : "no",
                    p->octet1 & SBC_BLOCK_LEN_12 ? "yes" : "no",
                    p->octet1 & SBC_BLOCK_LEN_16 ? "yes" : "no");

                bt_frontend_notification(" subbands 4 %s, 8 %s",
                    p->octet1 & SBC_SUBBANDS_4_MASK ? "yes" : "no",
                    p->octet1 & SBC_SUBBANDS_8_MASK ? "yes" : "no");

                bt_frontend_notification(" SNR %s, Loudness %s",
                    p->octet1 & SBC_ALLOCATION_SNR ? "yes" : "no",
                    p->octet1 & SBC_ALLOCATION_LOUDNESS ?"yes":"no");
            }
            break;

        case A2DP_CODEC_MPEG12:
            {
                const bt_a2dp_mpeg_1_2_audio_t *p = &codec->u.mpeg_1_2_audio;

                bt_frontend_notification(" MPEG-1,2 AUDIO [%02x %02x %02x %02x]",
                    p->octet0, p->octet1, p->octet2, p->octet3);

                bt_frontend_notification(" I %s, II %s, III %s, CRC %s",
                    p->octet0 & MPEG12_LAYER_I ? "yes" : "no",
                    p->octet0 & MPEG12_LAYER_II ? "yes" : "no",
                    p->octet0 & MPEG12_LAYER_III ? "yes" : "no",
                    p->octet0 & MPEG12_CRC ? "yes" : "no");

                bt_frontend_notification(" MONO %s, DUAL %s, STEREO %s, "
                    "JOINT STEREO %s",
                    p->octet0 & MPEG12_CHANNEL_MONO ? "yes" : "no",
                    p->octet0 & MPEG12_CHANNEL_DUAL ? "yes" : "no",
                    p->octet0 & MPEG12_CHANNEL_STEREO ? "yes" : "no",
                    p->octet0 & MPEG12_CHANNEL_JOINT_STEREO ?
                    "yes" : "no");

                bt_frontend_notification(" 16K %s, 22K %s, 24K %s, 32K %s, 44K %s, "
                    "48K %s",
                    p->octet1 & MPEG12_FREQ_16K ? "yes" : "no",
                    p->octet1 & MPEG12_FREQ_22K ? "yes" : "no",
                    p->octet1 & MPEG12_FREQ_24K ? "yes" : "no",
                    p->octet1 & MPEG12_FREQ_32K ? "yes" : "no",
                    p->octet1 & MPEG12_FREQ_44K ? "yes" : "no",
                    p->octet1 & MPEG12_FREQ_48K ? "yes" : "no");

                bt_frontend_notification(" MPF %s, VBR %s",
                    p->octet1 & MPEG12_MPF ? "yes" : "no",
                    p->octet2 & MPEG12_VBR ? "yes" : "no");

                bt_frontend_notification(" bit rate 0x%x, 0x%x",
                    p->octet2 & MPEG12_OCTET2_BIT_RATE_MASK,
                    p->octet3);
            }
            break;

        case A2DP_CODEC_MPEG_AAC:
            {
                const bt_a2dp_mpeg_aac_t *p = &codec->u.mpeg_aac;

                bt_frontend_notification(" MPEG-2,4 AAC "
                    "[%02x %02x %02x %02x %02x %02x]",
                    p->object_type, p->octet1, p->octet2, p->octet3, p->octet4,
                    p->octet5);

                bt_frontend_notification(" MPEG2 LC %s, MPEG4 LC %s, LTP %s, "
                    "SCALABLE %s",
                    p->object_type & MPEG2_AAC_LC ? "yes" : "no",
                    p->object_type & MPEG4_AAC_LC ? "yes" : "no",
                    p->object_type & MPEG4_AAC_LTP ? "yes" : "no",
                    p->object_type & MPEG4_AAC_SCALABLE ? "yes" :"no");

                bt_frontend_notification(" 8K %s, 11K %s, 12K %s, 16K %s, "
                    "22K %s, 24K %s, 32K %s, 44K %s",
                    p->octet1 & MPEG_AAC_FREQ_8K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_11K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_12K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_16K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_22K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_24K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_32K ? "yes" : "no",
                    p->octet1 & MPEG_AAC_FREQ_44K ? "yes" : "no");

                bt_frontend_notification(" 48K %s, 64K %s, 88K %s, 96K %s",
                    p->octet2 & MPEG_AAC_FREQ_48K ? "yes" : "no",
                    p->octet2 & MPEG_AAC_FREQ_64K ? "yes" : "no",
                    p->octet2 & MPEG_AAC_FREQ_88K ? "yes" : "no",
                    p->octet2 & MPEG_AAC_FREQ_96K ? "yes" : "no");

                bt_frontend_notification(" MONO %s, STEREO %s, VBR %s, "
                    "bit rate %ld",
                    p->octet2 & MPEG_AAC_CHANNELS_1 ? "yes" : "no",
                    p->octet2 & MPEG_AAC_CHANNELS_2 ? "yes" : "no",
                    p->octet3 & MPEG_AAC_VBR ? "yes" : "no",
                    MPEG_AAC_BIT_RATE(p));
            }
            break;

        case A2DP_CODEC_ATRAC:
            {
                const bt_a2dp_atrac_t *p = &codec->u.atrac;

                bt_frontend_notification(" ATRAC "
                    "[%02x %02x %02x %02x %02x %02x %02x]",
                    p->octet0, p->octet1, p->octet2, p->octet3, p->octet4,
                    p->octet5, p->octet6);
            }
            break;

        default:
            break;
    }
}

#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS
	extern int8_t GetSysMusicVol(void);
#endif

static void a2dp_connected(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx)
{
    a2dp_backend_t *app_ptr;

#if UPGRADED_VERSION /* here is one choice place to set delay_report value. */
    extern void set_delay_report_ms(uint16_t val_ms);
    extern inline BOOL isAvdtpSrcSupportDR(void);
    if (isAvdtpSrcSupportDR()) 
    {
        set_delay_report_ms(1500);
    }
#endif

#if (CONFIG_AS_SLAVE_ROLE == 0)
#if (BT_ONE2MULTIPLE_AS_SCATTERNET == 0)
	extern int app_env_check_Use_ext_PA(void);
	if(app_check_bt_mode(BT_MODE_1V2)
	||(app_check_bt_mode(BT_MODE_1V1)&&!app_env_check_Use_ext_PA())
	||(	app_check_bt_mode(BT_MODE_TWS|BT_MODE_DM_TWS)
		&&app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE)
		&&app_bt_flag2_get(APP_FLAG2_STEREO_ROLE_MASTER)))
    {

        if(Judge_role(NULL,TRUE))
        {
    	    return;
        }
    }
#endif
#endif
    LOG_I(CONN,"a2dp_connected\r\n");

    app_ptr = a2dp_get_app_from_svc(session_h);

    a2dp_set_flag(app_ptr, APP_FLAG_A2DP_CONNECTION);
    app_bt_profile_conn_wrap(PROFILE_BT_A2DP_SNK,&app_ptr->raddr);
    
    if(!a2dp_has_music()) 
		a2dp_update_current_app(app_ptr);

#if (CONFIG_AS_SLAVE_ROLE == 1)
    //Judge_role(&(app_ptr->raddr),TRUE);
#endif

#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
    //init_security_control_r_addr(&app_ptr->raddr); //initialize the content protection flag's raddr
#endif
#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS
	a2dp_volume_init_based_on_raddr(GetSysMusicVol(), &app_ptr->raddr);
#endif
}

static void a2dp_disconnected(bt_a2dp_sink_session_h session_h,result_t status, bt_app_ctx_h app_ctx)
{
    a2dp_backend_t *app_ptr;

    app_ptr = a2dp_get_app_from_svc(session_h); 
    LOG_I(CONN, "a2dp_disconnected\r\n");
    //LOG_I(CONN, "a2dp_disconnected src_id:%d,addr:"BTADDR_FORMAT"\r\n", app_ptr->svc_id,BTADDR(&app_ptr->raddr) );
    /* bt_app_management */
    #if (CONFIG_VOLUME_SAVE_IN_ENV == 1) 
    app_env_handle_t  env_hdl = app_env_get_handle();
    app_env_key_pair_t *key;  
	int id = app_env_key_stored(&app_ptr->raddr);
	if(id != 0)
	{
		key = &env_hdl->env_data.key_pair[id-1];
		key->a2dp_vol = app_ptr->volume;
	}
    #endif

    bt_app_entity_clear_conn_flag_by_addr(&app_ptr->raddr,PROFILE_BT_A2DP_SNK);

    set_connection_event(&app_ptr->raddr, CONN_EVENT_SNK_A2DP_DISCONNECTED);
    
    if(a2dp_has_connection())
          audio_out_close(&app_ptr->audio_out);
    
    a2dp_clear_flag(app_ptr, APP_FLAG_A2DP_CONNECTION|APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE);
    bt_a2dp_aac_stream_suspend();  // ++ by borg @230113
        
    app_ptr->avrcp_play_pause = 0;

    if(!a2dp_has_music())
    {
        app_bt_flag1_set(APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE, 0);
    }
    
#ifdef CONFIG_BLUETOOTH_AVDTP_SCMS_T
    uinit_security_control_r_addr(&app_ptr->raddr);//uninitialize the content protection flag's raddr
#endif

#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
	hci_link_t *tmp_link;
	#if (CONFIG_OTHER_FAST_PLAY == 1)
	app_handle_t sys_hdl = app_get_sys_handler();
	sys_hdl->otherplay = 0;
    sys_hdl->fast_state=0;
	#endif
	tmp_link = select_flow_ctrl_link_handle();
	if(tmp_link)
	{
        tmp_link->flow_ctrl = 0;
		bt_set_ag_flow_ctrl_by_handle(tmp_link);
	}
#endif

    a2dp_select_current_app_to_another(&app_ptr->raddr);
    avrcp_select_current_app_to_another(&app_ptr->raddr); 
    
    if(!hfp_has_sco() && !bt_audio_ch_busy())
    {
        if(a2dp_has_another_music(&app_ptr->raddr))
        {
            a2dp_audio_action();
        }
        else
        {
            //aud_PAmute_operation(1);
            audio_aso_close(ASO_TYPE_BT);// aud_dac_close();
            aud_volume_mute(1);
        }
    }
    a2dp_app_uninit(app_ptr);
}


static void a2dp_newconn(bt_conn_req_h conn_req,
                                const btaddr_t *laddr,
                                const btaddr_t *raddr,
                                bt_link_modes_mask_t mode,
                                bt_app_ctx_h app_ctx)
{
    result_t err = UWE_OK;
    a2dp_backend_t *app_ptr;
    #ifdef A2DP_MPEG_AAC_DECODE
//yuan1    app_env_handle_t env_h = app_env_get_handle();
    #endif
    app_ptr = a2dp_app_lookup_valid(raddr);

    if(app_ptr == NULL)
    {
        LOG_E(A2DP,"a2dp_unvalid_app\r\n");
        return;
    }
    else if(app_ptr->is_used == 0)
    {
        a2dp_app_init(app_ptr);
        j_memcpy((void *)&app_ptr->raddr, (void *)raddr, sizeof(btaddr_t));
        
        bt_a2dp_endpoint_desc_t ep;
#ifdef A2DP_VENDOR_DECODE
        bt_a2dp_endpoint_desc_t ep1;
#elif defined(A2DP_MPEG_AAC_DECODE)
        bt_a2dp_endpoint_desc_t ep2;
#endif

        ep.codecs_count           = 1;
        ep.codecs[0].type         = A2DP_CODEC_SBC;
        bt_a2dp_sink_get_default_sbc(&ep.codecs[0].u.sbc);

#ifdef A2DP_VENDOR_DECODE
        ep1.codecs_count          = 1;
        ep1.codecs[0].type        = A2DP_CODEC_NON_A2DP;
        bt_a2dp_sink_get_default_vendor(&ep1.codecs[0].u.ladc);
        ep.next = &ep1;
#elif defined(A2DP_MPEG_AAC_DECODE)
        ep2.codecs_count          = 1;
        ep2.codecs[0].type        = A2DP_CODEC_MPEG_AAC;
        bt_a2dp_sink_get_default_aac(&ep2.codecs[0].u.mpeg_aac);
        ep.next = &ep2;
#endif

        LOG_I(A2DP,"a2dp_newconn\r\n");
	#if CONFIG_A2DP_CONN_DISCONN
		app_handle_t app_h = app_get_sys_handler();
		if (app_h->a2dp_state == APP_A2DP_STATE_DISCONN)
		{
	        bt_a2dp_sink_conn_reject(conn_req);
	        err = UWE_STATE;
	        goto Exit;
		}
	#endif
        err = bt_a2dp_sink_conn_accept(app_ptr->svc, conn_req, mode, &ep);
        if(err)
        {
            goto Exit;
        }

        app_ptr->is_used = 1;
        #if (CONFIG_VOLUME_SAVE_IN_ENV == 1)
		app_env_handle_t  env_hdl = app_env_get_handle();
		app_env_key_pair_t *key;  
		int id = app_env_key_stored((btaddr_t *)raddr);
		if(id != 0)
		{
		    key = &env_hdl->env_data.key_pair[id-1];
		    if(key->a2dp_vol != -1)
			{
				app_ptr->volume = key->a2dp_vol;
			}
		}
        #endif
        #ifdef A2DP_MPEG_AAC_DECODE
        //init a2dp vol
	#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS
        app_ptr->volume  = GetSysMusicVol();	//yuan env_h->env_cfg.system_para.vol_a2dp;
	#endif
        #endif
        goto Exit;
    }
    
    if(app_ptr->is_used || app_ptr->conn_req)
    {
        bt_a2dp_sink_conn_reject(conn_req);
        err = UWE_STATE;
        goto Exit;
    }

    LOG_D(A2DP,"a2dp newconn request %lu", app_ptr->svc_id);

    app_ptr->conn_req      = conn_req;
    app_ptr->conn_mode     = mode;

Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
}

#ifndef VENDOR_DECODE_TEST
static uint32_t a2dp_sbc_rate(const bt_a2dp_codec_t *codec)
{
    const bt_a2dp_sbc_t *sbc = &codec->u.sbc;

    if(sbc->octet0 & SBC_FREQ_48K)
        return 48000;
    else if(sbc->octet0 & SBC_FREQ_44K)
        return 44100;
    else if(sbc->octet0 & SBC_FREQ_32K)
        return 32000;
    else if(sbc->octet0 & SBC_FREQ_16K)
        return 16000;
    else
        return 0;
}

static uint32_t a2dp_sbc_bps(const bt_a2dp_codec_t *codec)
{
    const bt_a2dp_sbc_t *sbc = &codec->u.sbc;

    if(sbc->octet1 & SBC_BLOCK_LEN_4)
        return 4;
    else if(sbc->octet1 & SBC_BLOCK_LEN_8)
        return 8;
    else if(sbc->octet1 & SBC_BLOCK_LEN_12)
        return 12;
    else if(sbc->octet1 & SBC_BLOCK_LEN_16)
        return 16;
    else
        return 0;
}

static uint32_t a2dp_sbc_channels(const bt_a2dp_codec_t *codec)
{
    return (codec->u.sbc.octet0 & SBC_CHANNEL_MONO ? 1 : 2);
}
#endif //VENDOR_DECODE_TEST

#ifdef A2DP_SBC_DUMP_SHOW
void a2dp_sbc_info_show(void)
{
	uint8_t i;
    for(i = 0; i < BT_MAX_AG_COUNT; i++)
        a2dp_codec_dump(&(g_a2dp_array[i].codec));
}
#endif

static void a2dp_set_configuration_cb(bt_a2dp_sink_session_h session_h,
                                               const bt_a2dp_codec_t *codec,
                                               int32_t local_ep_id,
                                               int32_t remote_ep_id,
                                               bt_app_ctx_h app_ctx_h)
{
	result_t err;
    a2dp_backend_t *app_ptr;
    uint32_t rate, channels, bps;
//yuan1    app_env_handle_t env_h = app_env_get_handle();

    app_ptr = a2dp_get_app_from_svc(session_h);
    app_ptr->codec_type = codec->type;

#ifdef A2DP_VENDOR_DECODE

#ifndef VENDOR_DECODE_TEST
    if(local_ep_id == 0x08)
#endif
    {
        bt_a2dp_set_configure_cb_vendor_param(codec, &bps, &rate, &channels);

		app_ptr->bps	 = bps;
		app_ptr->freq	 = rate;
		app_ptr->channel = channels;
        app_ptr->volume  = env_h->env_cfg.system_para.vol_a2dp;

	    if(!rate || !bps || !channels)
		{
			err = UWE_INVAL;
			bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
			goto Exit;
		}

		os_printf("vendor a2dp_cfg:%d:%d:%d\r\n", rate, channels, bps);

		if(!(hfp_has_sco() || bt_audio_ch_busy()))
		{
			audio_out_close(&app_ptr->audio_out);
			err = audio_out_open(rate, channels, bps, &app_ptr->audio_out);
			if(err)
			{
				bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
				goto Exit;
			}
		}
	    err = bt_a2dp_sink_set_configuration_rsp(app_ptr->svc,
    											 A2DP_INPUT_METHOD_VENDOR_TO_PCM,
    											 local_ep_id,
    											 remote_ep_id);
    }
#ifndef VENDOR_DECODE_TEST
    else
    {
		rate	 = a2dp_sbc_rate(codec);
		bps 	 = a2dp_sbc_bps(codec);
		channels = a2dp_sbc_channels(codec);

		app_ptr->bps	 = bps;
		app_ptr->freq	 = rate;
		app_ptr->channel = channels;
		app_ptr->volume  = env_h->env_cfg.system_para.vol_a2dp;

#ifdef CONFIG_BLUETOOTH_AVRCP_TG
		//a2dp_volume_init_based_on_raddr(env_h->env_cfg.system_para.vol_a2dp, &app_ptr->raddr);
#endif
#if defined(A2DP_SBC_DUMP_SHOW)
		memcpy( &app_ptr->codec, codec, sizeof(app_ptr->codec) );
#endif

		if(!rate || !bps || !channels)
		{
			err = UWE_INVAL;
			bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
			goto Exit;
		}

		LOG_I(A2DP,"sbc a2dp_cfg:%d:%d:%d\r\n", rate, channels, bps);

		if(!(hfp_has_sco() || bt_audio_ch_busy()))
		{
			audio_out_close(&app_ptr->audio_out);
			err = audio_out_open(rate, channels, bps, &app_ptr->audio_out);
			if(err)
			{
				bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
				goto Exit;
			}
		}

		err = bt_a2dp_sink_set_configuration_rsp(app_ptr->svc,
												 A2DP_INPUT_METHOD_SBC_TO_PCM,
												 local_ep_id,
												 remote_ep_id);
    }
#endif //VENDOR_DECODE_TEST
#elif defined(A2DP_MPEG_AAC_DECODE)
#ifndef VENDOR_DECODE_TEST
    if(local_ep_id == 0x08)
#endif //VENDOR_DECODE_TEST
    {
        bt_a2dp_set_configure_cb_aac_param(codec, &bps, &rate, &channels);

		app_ptr->bps	 = bps;
		app_ptr->freq	 = rate;
		app_ptr->channel = channels;
        #ifdef A2DP_MPEG_AAC_DECODE
        if(app_ptr->volume != 0)
        {
            //LOG_I(A2DP, "don't init vol %x\r\n", app_ptr->volume);
        }else
        #endif
        {
#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS
            app_ptr->volume = GetSysMusicVol();	//yuan env_h->env_cfg.system_para.vol_a2dp;
#endif
        }


	    if(!rate || !bps || !channels)
		{
			err = UWE_INVAL;

			bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
			goto Exit;
		}

		LOG_I(A2DP,"aac a2dp_cfg:%d:%d:%d\r\n", rate, channels, bps);

		if(!(hfp_has_sco() || bt_audio_ch_busy()))
		{
			audio_out_close(&app_ptr->audio_out);
			err = audio_out_open(rate, channels, bps, &app_ptr->audio_out);
			if(err)
			{
				bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
				goto Exit;
			}
		}
	    err = bt_a2dp_sink_set_configuration_rsp(app_ptr->svc,
    											 A2DP_INPUT_METHOD_AAC_TO_PCM,
    											 local_ep_id,
    											 remote_ep_id);
    }
#ifndef VENDOR_DECODE_TEST
    else
    {
		rate	 = a2dp_sbc_rate(codec);
		bps 	 = a2dp_sbc_bps(codec);
		channels = a2dp_sbc_channels(codec);

		app_ptr->bps	 = bps;
		app_ptr->freq	 = rate;
		app_ptr->channel = channels;
#ifdef A2DP_MPEG_AAC_DECODE
        if(app_ptr->volume != 0)
        {
            //LOG_I(A2DP, "don't init vol %x\r\n", app_ptr->volume);
        }else
#endif
        {
#ifdef	APPLE_IOS_VOL_SYNC_ALWAYS
            app_ptr->volume = GetSysMusicVol();	//yuan env_h->env_cfg.system_para.vol_a2dp;
#endif
        }


#ifdef CONFIG_BLUETOOTH_AVRCP_TG
		//a2dp_volume_init_based_on_raddr(env_h->env_cfg.system_para.vol_a2dp, &app_ptr->raddr);
#endif
#if defined(A2DP_SBC_DUMP_SHOW) &&  !defined(CONFIG_TWS)
		memcpy( &app_ptr->codec, codec, sizeof(app_ptr->codec) );
#endif

		if(!rate || !bps || !channels)
		{
			err = UWE_INVAL;

			bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
			goto Exit;
		}

		LOG_I(A2DP,"sbc a2dp_cfg:%d:%d:%d\r\n", rate, channels, bps);

		if(!(hfp_has_sco() || bt_audio_ch_busy()))
		{
			audio_out_close(&app_ptr->audio_out);
			err = audio_out_open(rate, channels, bps, &app_ptr->audio_out);
			if(err)
			{
				bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
				goto Exit;
			}
		}

		err = bt_a2dp_sink_set_configuration_rsp(app_ptr->svc,
												 A2DP_INPUT_METHOD_SBC_TO_PCM,
												 local_ep_id,
												 remote_ep_id);
    }
#endif //VENDOR_DECODE_TEST
#else //A2DP_VENDOR_DECODE
    rate     = a2dp_sbc_rate(codec);
    bps      = a2dp_sbc_bps(codec);
    channels = a2dp_sbc_channels(codec);

    app_ptr->freq    = rate;
    app_ptr->channel = channels;
    app_ptr->volume  = env_h->env_cfg.system_para.vol_a2dp;

#ifdef CONFIG_BLUETOOTH_AVRCP_TG
    //a2dp_volume_init_based_on_raddr(env_h->env_cfg.system_para.vol_a2dp, &app_ptr->raddr);
#endif
#if defined(A2DP_SBC_DUMP_SHOW)
    memcpy( &app_ptr->codec, codec, sizeof(app_ptr->codec) );
#endif

    if(!rate || !bps || !channels)
    {
        err = UWE_INVAL;
        bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
        goto Exit;
    }

    LOG_I(A2DP,"a2dp_cfg:%d:%x:%x\r\n", rate, channels, bps);

    if(!(hfp_has_sco() || bt_audio_ch_busy()))
    {
        audio_out_close(&app_ptr->audio_out);
        err = audio_out_open(rate, channels, bps, &app_ptr->audio_out);
        if(err)
        {
            bt_a2dp_sink_set_configuration_rej(app_ptr->svc, local_ep_id, remote_ep_id);
            goto Exit;
        }
    }

    err = bt_a2dp_sink_set_configuration_rsp(app_ptr->svc,
                                             A2DP_INPUT_METHOD_SBC_TO_PCM,
                                             local_ep_id,
                                             remote_ep_id);
#endif //A2DP_VENDOR_DECODE
#if (CONFIG_VOLUME_SAVE_IN_ENV == 1)
            app_env_handle_t  env_hdl = app_env_get_handle();
            app_env_key_pair_t *key;  
            int id = app_env_key_stored(&app_ptr->raddr);
            if(id != 0)
            {
                key = &env_hdl->env_data.key_pair[id-1];
                if(key->a2dp_vol != -1 && app_ptr->volume == env_h->env_cfg.system_para.vol_a2dp)
                {
                   app_ptr->volume = key->a2dp_vol;
                }
            }        
#endif

    if(err)
    {
        audio_out_close(&app_ptr->audio_out);
        goto Exit;
    }
Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
}

void a2dp_audio_action(void)
{    
    if(a2dp_has_music())
    {
        audio_aso_close(ASO_TYPE_BT);// aud_dac_close();
       // aud_PAmute_operation(0);
        sbc_discard_uselist_node();  //discard all the old sbc note avoid noise
        sbc_init_adjust_param();
        if(g_current_a2dp_ptr->codec_type == A2DP_CODEC_SBC)
        {
            sbc_stream_start_init(g_current_a2dp_ptr->freq);
        }
#ifdef A2DP_MPEG_AAC_DECODE
        else if(g_current_a2dp_ptr->codec_type == A2DP_CODEC_MPEG_AAC)
        {
            bt_a2dp_aac_stream_start(g_current_a2dp_ptr->freq, g_current_a2dp_ptr->channel);
        }
#endif
        audio_aso_config(g_current_a2dp_ptr->freq, g_current_a2dp_ptr->channel, 16, ASO_TYPE_BT);// aud_dac_config
        aud_dac_set_volume(g_current_a2dp_ptr->volume);
        avrcp_volume_sync();
        audio_aso_open(ASO_TYPE_BT);// aud_dac_open();
    }
}

static void a2dp_stream_start_cb(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx)
{
    a2dp_backend_t *app_ptr;
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
    app_handle_t sys_hdl = app_get_sys_handler();
    hci_link_t *tmp_link;
#endif
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    uint16_t handle;
#endif

    LOG_I(A2DP,"MB\r\n");  //print 'MB'

    CLEAR_SLEEP_TICK;
    app_ptr = a2dp_get_app_from_svc(session_h);
    
    /* bt_app_management */
    bt_app_entity_set_work_flag_by_addr(&(app_ptr->raddr),BT_WORK_MUSIC_PLAYING);
    a2dp_set_flag(app_ptr, APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE);
   
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    handle = bt_sniff_get_handle_from_raddr(&(app_ptr->raddr));
    app_bt_write_sniff_link_policy(handle, 0);
#endif

    LOG_I(A2DP,"stream_start_handler:%d, %d, %d\r\n", app_ptr->codec_type, app_ptr->freq, app_ptr->channel);
    #if CONFIG_OTHER_FAST_PLAY
    if(!a2dp_has_another_music_avrcp_playing(&app_ptr->raddr))
    #else
    if(!a2dp_has_another_music(&app_ptr->raddr))
    #endif
    {
        a2dp_update_current_app(app_ptr);
        avrcp_update_current_app(&app_ptr->raddr);
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
    	tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &app_ptr->raddr, HCI_LINK_ACL);
    	if(tmp_link)
    	{
            #if CONFIG_OTHER_FAST_PLAY
            sys_hdl->fast_state=1;
            #endif
            tmp_link->flow_ctrl = 0x02;
    		bt_set_ag_flow_ctrl_by_handle(tmp_link);
    	}
#endif
        if(!hfp_has_sco() && !bt_audio_ch_busy())
            a2dp_audio_action();    

        app_bt_flag1_set(APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE, 1);
        app_set_led_event_action(LED_EVENT_BT_PLAY);
	#if CONFIG_OTHER_FAST_PLAY
		set_a2dp_array_play_pause_status(&(app_ptr->raddr), 1);
	#endif
    }
    else if(btaddr_same(&(g_current_a2dp_ptr->raddr), &(app_ptr->raddr))) //For xiaomi stream_start twice!!!!
    {
        return;
    }
	else /* Device has been playing music , and remote addr is not same.*/
	{
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
		tmp_link = select_curr_playing_link_handle_by_a2dp(app_ptr);
		if(tmp_link)
		{
            tmp_link->flow_ctrl = 0x01;
			bt_set_ag_flow_ctrl_by_handle(tmp_link);
		}
#endif
	}
}

static void a2dp_stream_suspend_cb(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx)
{
    a2dp_backend_t *app_ptr;
    app_handle_t sys_hdl = app_get_sys_handler();
    a2dp_backend_t *last_curr_app_ptr;
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
    hci_link_t *tmp_link;
#endif

    app_ptr = a2dp_get_app_from_svc(session_h);
    last_curr_app_ptr = a2dp_get_current_app();
    /* bt_app_management */
    bt_app_entity_clear_work_flag_by_addr(&app_ptr->raddr,BT_WORK_MUSIC_PLAYING);
    a2dp_clear_flag(app_ptr, APP_BUTTON_FLAG_PLAY_PAUSE|APP_FLAG_MUSIC_PLAY);
    
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
	#if (CONFIG_OTHER_FAST_PLAY == 1)
	sys_hdl->otherplay = 0;
    sys_hdl->fast_state=0;
	set_a2dp_array_play_pause_status(&(app_ptr->raddr), 0);
	#endif
    tmp_link = select_flow_ctrl_link_handle();
    if(tmp_link)
    {
        tmp_link->flow_ctrl &= ~(0x01);
        bt_set_ag_flow_ctrl_by_handle(tmp_link);
    }
    tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &app_ptr->raddr, HCI_LINK_ACL);
    if(tmp_link)
    {
        tmp_link->flow_ctrl &= ~(0x02);
        bt_set_ag_flow_ctrl_by_handle(tmp_link);
    }
#endif
    extern BOOL is_voice_recog_status_on(void);
    if(a2dp_has_music() && !is_voice_recog_status_on())
    {
        a2dp_select_current_app_to_another(&app_ptr->raddr);
        avrcp_select_current_app_to_another(&app_ptr->raddr); 
    }
    else
    {
        sbc_discard_uselist_node();
        sbc_init_adjust_param();
        sbc_mem_free();
#ifdef A2DP_VENDOR_DECODE
        vendor_mem_free();
#endif
#ifdef A2DP_MPEG_AAC_DECODE
        bt_a2dp_aac_stream_suspend();
#endif
        app_bt_flag1_set(APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE, 0);
    }

    if(!hfp_has_sco() && !bt_audio_ch_busy())
    {
        if(a2dp_has_another_music(&app_ptr->raddr))
        {
            if(a2dp_get_current_app() != last_curr_app_ptr)
                a2dp_audio_action();
        }
        else
        {
           // aud_PAmute_operation(1);
            audio_aso_close(ASO_TYPE_BT);// aud_dac_close();
            aud_volume_mute(1);
            app_set_led_event_action(LED_EVENT_BT_PAUSE);
        }
    }

#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
    if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE|APP_FLAG_MUSIC_PLAY))
    {
        tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &g_current_a2dp_ptr->raddr, HCI_LINK_ACL);
        if(tmp_link)
        {
            tmp_link->flow_ctrl |= 0x02;
		    bt_set_ag_flow_ctrl_by_handle(tmp_link);
        }
	#if (CONFIG_OTHER_FAST_PLAY == 1)
        tmp_link = select_flow_ctrl_link_handle_by_raddr(&(g_current_a2dp_ptr->raddr));
		if(tmp_link)
		{
			tmp_link->flow_ctrl &= ~(0x01);
			bt_set_ag_flow_ctrl_by_handle(tmp_link);
		}
	#endif
    }
#endif

    if(!app_bt_flag1_get(APP_FLAG_HFP_CALLSETUP|APP_FLAG_HFP_OUTGOING) && !a2dp_has_music())
    {
        app_set_led_event_action( LED_EVENT_CONN ); 
    }

    LOG_I(A2DP,"MA\r\n" );   //print 'MA'
}

static void a2dp_stream_input_cb(bt_a2dp_sink_session_h session_h, void *buf,
                                         uint32_t len, bt_app_ctx_h app_ctx_h)
{
    result_t err = UWE_OK;
    a2dp_backend_t *app_ptr;

    app_ptr = a2dp_get_app_from_svc(session_h);

    DECLARE_FNAME("a2dp_stream_input_cb");

    err = audio_out_write(app_ptr->audio_out, buf, len);

    UNUSED_VAR(err);
    DBG_RC_X(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
}

result_t a2dp_cmd_connect(char *params, unsigned int len)
{
    result_t err;
    int btaddr_tmp[6];
    uint32_t unit_id;
    btaddr_t laddr, raddr;
    a2dp_backend_t *app_ptr;
    bt_a2dp_endpoint_desc_t ep;
#ifdef A2DP_VENDOR_DECODE
	bt_a2dp_endpoint_desc_t ep1;
#endif
#ifdef A2DP_MPEG_AAC_DECODE
    bt_a2dp_endpoint_desc_t ep2;
#endif

    if(j_snscanf(params,
                  NULL,
                  "%u " BTADDR_FORMAT,
                  &unit_id,
                  BTADDR_TMP(btaddr_tmp)) != UW_ARRAY_SIZE(btaddr_tmp) + 1)
    {
        return UWE_INVAL;
    }
	
	TMP_TO_BTADDR(btaddr_tmp, &raddr);
	
    app_ptr = a2dp_app_lookup_valid(&raddr);

    if(app_ptr == NULL)
    {
        LOG_E(A2DP,"a2dp_cmd_unvalid_app\r\n");
        return UWE_INVAL;
    }
    else if(app_ptr->is_used == 0)
    {
        LOG_D(A2DP,"a2dp_cmd_connect\r\n");
        a2dp_app_init(app_ptr);
    }

    err = backend_unit_get_addr(unit_id, &laddr);
    if(err)
    {
        LOG_E(CONN,"a2dp_cmd_connect,get addr err:%d\r\n",err);
        goto Exit;
    }

    ep.codecs_count = 1;
    ep.codecs[0].type = A2DP_CODEC_SBC;
    bt_a2dp_sink_get_default_sbc(&ep.codecs[0].u.sbc);

#ifdef A2DP_VENDOR_DECODE
    ep1.codecs_count = 1;
    ep1.codecs[0].type = A2DP_CODEC_NON_A2DP;
	bt_a2dp_sink_get_default_vendor(&ep1.codecs[0].u.ladc);
	ep.next = &ep1;
#endif

#ifdef A2DP_MPEG_AAC_DECODE
    ep2.codecs_count = 1;
    ep2.codecs[0].type = A2DP_CODEC_MPEG_AAC;
    bt_a2dp_sink_get_default_aac(&ep2.codecs[0].u.mpeg_aac);
    ep.next = &ep2;
#endif

    err = bt_a2dp_sink_conn_connect(app_ptr->svc, &laddr, &raddr, &ep);
    if(err)
    {
        LOG_E(CONN,"a2dp_cmd_connect,err:%d\r\n",err);
        goto Exit;
    }

    btaddr_copy(&app_ptr->raddr, &raddr);

    app_ptr->is_used = 1;

    LOG_D(CONN,"a2dp_cmd_connect:%lu,app_ptr:0x%x\r\n", app_ptr->svc_id,app_ptr);

Exit:
    return err;
}

result_t a2dp_cmd_disconnect(void)
{
    if(!g_current_a2dp_ptr->svc)
    {
        LOG_W(A2DP,"No a2dp app\r\n");
        return UWE_NODEV;
    }

    return bt_a2dp_sink_conn_disconnect(g_current_a2dp_ptr->svc);
}

#if CONFIG_A2DP_CONN_DISCONN
result_t a2dp_set_disconnect(void)
{
	uint32_t i,ret=UWE_NOTSTARTED;
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_a2dp_array[i].is_used) && (g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION))
        {
			bt_a2dp_sink_conn_disconnect(g_a2dp_array[i].svc);
			ret = UWE_OK;
        }
    }

	return ret;
}

result_t a2dp_set_connect(void)
{
	uint32_t i,ret=UWE_NOTSTARTED;
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if (hci_check_acl_link(bt_app_entit_get_rmt_addr(i)))
    	{
			app_bt_active_conn_profile(PROFILE_ID_A2DP,(void *)bt_app_entit_get_rmt_addr(i));
			ret = UWE_OK;
    	}
    }

	return ret;
}
#endif

#if A2DP_ROLE_SOURCE_CODE
void a2dpSnkConnectRemoteDevice(btaddr_t *remote_btaddr)
{
    if (NULL == remote_btaddr) 
    {
        os_printf("a2dpSnkConnectRemoteDevice: remote_btaddr is NULL! error! \r\n");
        return;
    }

    char cmd[40];
    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT,
            0,
            remote_btaddr->b[5],
            remote_btaddr->b[4],
            remote_btaddr->b[3],
            remote_btaddr->b[2],
            remote_btaddr->b[1],
            remote_btaddr->b[0]);

    os_printf("a2dpSnkConnectRemoteDevice: " BTADDR_FORMAT " \r\n", BTADDR(remote_btaddr));

    a2dp_cmd_connect(cmd, sizeof(cmd));
}
#endif

uint32_t a2dp_get_codec_type(void)
{
    return g_current_a2dp_ptr->codec_type;
}


#if UPGRADED_VERSION
extern result_t a2dp_send_delay_report(bt_a2dp_sink_session_h session, uint16_t dr_ms);
result_t a2dp_send_delayReport(uint16_t dr_ms)
{
    if (!g_current_a2dp_ptr) 
    {
        LOG_I(A2DP,"a2dp_send_delayReport, param error! 0x%x, %d \r\n", g_current_a2dp_ptr, dr_ms);
        return UWE_PARAM;
    }

    LOG_I(A2DP,"a2dp_send_delayReport, 0x%x, %d \r\n", g_current_a2dp_ptr->svc, dr_ms);
    return a2dp_send_delay_report(g_current_a2dp_ptr->svc, dr_ms);
}
#endif

#if (CONFIG_OTHER_FAST_PLAY == 1)
result_t  select_current_a2dp_avrcp(btaddr_t *addr)
{
	uint32 i;
	result_t err = UWE_INVAL;
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
	hci_link_t *tmp_link;
	app_handle_t sys_hdl = app_get_sys_handler();
#endif

    //os_printf("===select_current_a2dp_avrcp: "BTADDR_FORMAT"\r\n", BTADDR(addr));
	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if((g_a2dp_array[i].is_used)
			&& (g_a2dp_array[i].flag & APP_FLAG_MUSIC_PLAY)
            && (g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION))
        {  
        	if(btaddr_same(addr,&(g_a2dp_array[i].raddr)))
        	{
				sys_hdl->otherplay = 0;
                sys_hdl->fast_state=0;
				g_current_a2dp_ptr = &g_a2dp_array[i];
				//os_printf("===addr_current: "BTADDR_FORMAT"\r\n", BTADDR(&(g_a2dp_array[i]->raddr)));
    		#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)						
				if (g_a2dp_array[i^0x01].flag & APP_FLAG_MUSIC_PLAY)
				{
			        tmp_link = select_curr_playing_link_handle_by_a2dp(&g_a2dp_array[i^0x01]);
			        if(tmp_link)
			        {
			            tmp_link->flow_ctrl = 0x01;
			            bt_set_ag_flow_ctrl_by_handle(tmp_link);
			        }
				}
				tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &g_current_a2dp_ptr->raddr, HCI_LINK_ACL);
				if(tmp_link)
				{
					audio_dac_ana_mute(0);
				    tmp_link->flow_ctrl = 0x02;
					bt_set_ag_flow_ctrl_by_handle(tmp_link);
					set_a2dp_array_play_pause_status(&(g_current_a2dp_ptr->raddr), 1);
					select_flow_ctrl_link_handle_by_raddr(&(g_current_a2dp_ptr->raddr));
				}
			#endif			
			    if((!app_bt_flag1_get(APP_FLAG_SCO_CONNECTION)) && (!bt_audio_ch_busy ()))
    			{
    				int8_t another_vol = g_current_a2dp_ptr->volume;
					LOG_I(A2DP, "volume sync: %d\r\n", another_vol);
    				aud_dac_set_volume(another_vol);
					if (0 == another_vol)
        			{
            			aud_PAmute_operation(1);
            			aud_volume_mute(1);
            			app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 1);
        			}
			    }		
                if ((g_current_a2dp_ptr->freq!=g_a2dp_array[i^0x01].freq)
					|| ((g_current_a2dp_ptr->codec_type==A2DP_CODEC_MPEG_AAC)
						&&(g_current_a2dp_ptr->codec_type!=g_a2dp_array[i^0x01].codec_type)))
                {
			        if(!hfp_has_sco() && !bt_audio_ch_busy())
			            a2dp_audio_action(); 
                }
				err = UWE_OK;
				break;
        	}
        }
	}
	
	if(err)
		return err;
	
	return avrcp_update_current_app(&(g_current_a2dp_ptr->raddr));
}

result_t  select_other_a2dp_avrcp(btaddr_t *addr)
{
	uint32 i;
	result_t err = UWE_INVAL;
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
	hci_link_t *tmp_link;
	app_handle_t sys_hdl = app_get_sys_handler();
#endif

    //os_printf("===select_other_a2dp_avrcp: "BTADDR_FORMAT"\r\n", BTADDR(addr));
	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if((g_a2dp_array[i].is_used)&&(g_a2dp_array[i].flag & APP_FLAG_A2DP_CONNECTION))
        {  
        	if(!btaddr_same(addr,&(g_a2dp_array[i].raddr)))
        	{
				sys_hdl->otherplay = 0;
                sys_hdl->fast_state=0;
				g_current_a2dp_ptr = &g_a2dp_array[i];
				os_printf("===addr_other: "BTADDR_FORMAT"\r\n", BTADDR(&(g_a2dp_array[i].raddr)));
    		#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)			
				if (g_a2dp_array[i^0x01].flag & APP_FLAG_MUSIC_PLAY)
				{
			        tmp_link = select_curr_playing_link_handle_by_a2dp(&g_a2dp_array[i^0x01]);
			        if(tmp_link)
			        {
			            tmp_link->flow_ctrl = 0x01;
			            bt_set_ag_flow_ctrl_by_handle(tmp_link);
			        }
				}
				tmp_link = hci_link_lookup_btaddr(sys_hdl->unit, &g_current_a2dp_ptr->raddr, HCI_LINK_ACL);
				if(tmp_link)
			    {
					audio_dac_ana_mute(0);
			    	tmp_link->flow_ctrl = 0x02;
			        bt_set_ag_flow_ctrl_by_handle(tmp_link);
					set_a2dp_array_play_pause_status(&(g_current_a2dp_ptr->raddr), 1);
					select_flow_ctrl_link_handle_by_raddr(&(g_current_a2dp_ptr->raddr));
			    }	
			#endif	
			    if((!app_bt_flag1_get(APP_FLAG_SCO_CONNECTION)) && (!bt_audio_ch_busy ()))
    			{
    				int8_t another_vol = g_current_a2dp_ptr->volume;
					LOG_I(A2DP, "volume sync: %d\r\n", another_vol);
    				aud_dac_set_volume(another_vol);
					if (0 == another_vol)
        			{
            			aud_PAmute_operation(1);
            			aud_volume_mute(1);
            			app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 1);
        			}
			    }
                if ((g_current_a2dp_ptr->freq!=g_a2dp_array[i^0x01].freq)
					|| ((g_current_a2dp_ptr->codec_type==A2DP_CODEC_MPEG_AAC)
						&&(g_current_a2dp_ptr->codec_type!=g_a2dp_array[i^0x01].codec_type)))
                {
			        if(!hfp_has_sco() && !bt_audio_ch_busy())
			            a2dp_audio_action(); 
                }
				err = UWE_OK;
				break;
        	}
        }
	}
	
	if(err)
		return err;
	
	return avrcp_update_current_app(&(g_current_a2dp_ptr->raddr));
}
#endif
// EOF
