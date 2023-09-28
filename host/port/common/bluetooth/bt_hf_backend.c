/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <string.h>
#include <stdlib.h>
#include <jos.h>
#include <bluetooth.h>
#include <bt_api.h>
#include <bt_hfp_hf.h>
#include "bt_app_internal.h"
#include "bt_rfcomm_channel.h"
#include "bt_sco_backend_utils.h"
#include "app_beken_includes.h"


#if 0
//ADC = (VBAT*1024)/4.6
const uint16_t iphone_bat_lab[10]=
{
	712,		//3.2V
	734,		//3.3V
	755,		//3.4V
	767,		//3.45V
	779,		//3.5V
	800,		//3.6V
	822,		//3.7V
	845,		//3.8V
	867,		//3.9V
	890,		//4.0V
};
#endif

#if(CONFIG_HFP17_MSBC_SUPPORTED == 1)
#define HF_SUPPORTED_FEATURES (                                    \
                               HF_CLI_PRESENTATION_CAPABILITY |    \
                               HF_REMOTE_VOLUME_CONTROL |        \
                               HF_VOICE_RECOGNITION_ACTIVATION|        \
                               HF_CODEC_NEGOTIATION|                \
                               HF_EC_NR_FUNCTION|   \
                               HF_CALL_WAITING_AND_3WAY_CALLING)
#else
#define HF_SUPPORTED_FEATURES (                                    \
                               HF_CLI_PRESENTATION_CAPABILITY |    \
                               HF_REMOTE_VOLUME_CONTROL |        \
                               HF_VOICE_RECOGNITION_ACTIVATION|        \
                               HF_CALL_WAITING_AND_3WAY_CALLING)
#endif

#define HFP_VOLUME_MAX        11

volatile uint8_t g_active_sco_and_incoming_flag = 0;

/* HF Service-level Notifications */
void hf_backend_uninit(void);
void hf_audio_restore(void);
static void hf_at_result_print(hfp_hf_app_t *app_ptr, const at_result_t *at_result);
static void hfp_hf_connected(bt_hfp_hf_session_h session, bt_app_ctx_h app_ctx);
static void hfp_hf_disconnected(bt_hfp_hf_session_h session, result_t status, bt_app_ctx_h app_ctx);

static bt_hfp_hf_conn_cbs_t hfp_hf_conn_cbs = {
    hfp_hf_connected,
    hfp_hf_disconnected
};

static void hfp_hf_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
                                  const btaddr_t *raddr, bt_link_modes_mask_t mode,
                                  bt_app_ctx_h app_server_ctx);

void hfp_hf_input(bt_hfp_hf_session_h session, at_result_t *at_result, bt_app_ctx_h app_ctx);

void hfp_get_features(bt_hfp_hf_session_h session_h, uint16_t *supported_features, 
                           bt_app_ctx_h app_ctx);

static const bt_hfp_hf_cbs_t hfp_hf_cbs = {
    hfp_hf_input,
    hfp_get_features
};

result_t hf_cmd_disconnect(void);
result_t hf_cmd_accept_call(void);
result_t hf_cmd_hangup_call(int8_t reason);
result_t hf_cmd_place_call(char number[32]);
result_t hf_cmd_redial(void);
result_t hf_cmd_resp_and_hold(uint32_t command);
result_t hf_cmd_resp_and_hold_read(void);
result_t hf_cmd_set_call_wait_notif(uint32_t enable);
result_t hf_cmd_chld(uint32_t command, uint32_t call_id);
result_t hf_cmd_list_current_calls(void);
result_t hf_cmd_set_cli_notif(uint32_t enable);
result_t hf_cmd_get_operator(void);
result_t hf_cmd_get_subscriber(void);
result_t hf_cmd_set_iphone_batlevel(int8_t level);
#if CONFIG_HFP_CONN_DISCONN
result_t hf_set_disconnect(void);
result_t hf_set_connect(void);
#endif

extern BOOL bt_is_ag_support_feature(bt_hfp_hf_session_h session,uint8_t feature);

/* XXX: Currently, only single service is supported by this sample */
static bt_hfp_hf_session_h g_hfp_srv;
static hfp_hf_app_t *g_hf_current_app_ptr;
static hfp_hf_app_t g_hfp_app_array[2];

void hfp_app_ptr_debug_printf(void)
{
    os_printf("----------------------------------------------------\r\n");
    if(g_hf_current_app_ptr == NULL)
    	os_printf("|         current_hfp:NULL\r\n");
    else
    {
    	os_printf("|           current_hfp:%x(%s)\r\n",g_hf_current_app_ptr,backend_unit_get_name(&(g_hf_current_app_ptr->raddr)));
    	os_printf("|      current_hfp_flag:%x\r\n", g_hf_current_app_ptr->flag);
    	os_printf("|    current_hfp_volume:%d\r\n", g_hf_current_app_ptr->volume);
    	os_printf("|   current_hfp_session:%d\r\n", g_hf_current_app_ptr->session);
    	os_printf("|   current_hfp_is_used:%d\r\n", g_hf_current_app_ptr->is_used);
    }
    os_printf("----------------------------------------------------\r\n");
}

uint32_t hfp_app_init(hfp_hf_app_t *app_ptr)
{
    result_t err;

    err = bt_hfp_hf_conn_create(&app_ptr->session,
                                &hfp_hf_conn_cbs,
                                NULL);
    return err;
}

uint32_t hfp_app_uninit(hfp_hf_app_t *app_ptr)
{
    if(app_ptr->conn_req)
    {
        bt_hfp_hf_conn_reject(app_ptr->conn_req);
    }

    if(app_ptr->session)
    {
        bt_hfp_hf_conn_destroy(&app_ptr->session);
    }
    
    j_memset(app_ptr, 0, sizeof(hfp_hf_app_t));

    return UWE_OK;
}

result_t hf_backend_init(void)
{
    result_t err;

    g_hfp_srv = NULL;
    g_hf_current_app_ptr = NULL;
    
    j_memset(&g_hfp_app_array[0], 0, sizeof(hfp_hf_app_t));
    j_memset(&g_hfp_app_array[1], 0, sizeof(hfp_hf_app_t));
    
    err = bt_hfp_hf_register(&hfp_hf_cbs);
    
    if(err)
    {
        goto Exit;
    }

    err = bt_hfp_hf_server_start(&g_hfp_srv,
                                 BTADDR_ANY,
                                 RFCOMM_CHANNEL_HFP,
                                 hfp_hf_newconn,
                                 NULL);
    
 Exit:
    if(err)
    {
        hf_backend_uninit();
    }
    
    LOG_D(APP,"hf_backend_init\r\n");

    return err;
}

void hf_backend_uninit(void)
{
    uint32_t i;
    hfp_hf_app_t *app_ptr = NULL;

    bt_hfp_hf_unregister();

    if(g_hfp_srv)
    {
        bt_hfp_hf_server_stop(&g_hfp_srv);
    }
        
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_hfp_app_array[i];
        if(app_ptr->is_used)
        {
            hfp_app_uninit(app_ptr);
        }
    }
}

hfp_hf_app_t *hfp_get_app_from_session(bt_hfp_hf_session_h session)
{
    uint32_t i;
    hfp_hf_app_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used) && (session == g_hfp_app_array[i].session))
        {
            app_ptr = &g_hfp_app_array[i];
            break;
        }
    }

    if (!app_ptr)
        LOG_D(APP,"hfp_get_app_from_session is NULL\r\n");

    return app_ptr;
}

void *hfp_get_app_from_priv(sco_utils_h priv)
{
    uint32_t i;
    hfp_hf_app_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_hfp_app_array[i].is_used && (priv == g_hfp_app_array[i].priv))
        {
            app_ptr = &g_hfp_app_array[i];
            break;
        }
    }

    if (!app_ptr)
       LOG_D(APP,"hfp_get_app_from_priv is NULL\r\n");


    return (void*)app_ptr;
}

hfp_hf_app_t *hfp_app_lookup_valid(const btaddr_t *raddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_hfp_app_array[i].is_used 
            && btaddr_same(raddr, &g_hfp_app_array[i].raddr))
        {
            return &g_hfp_app_array[i];
        }
    }


    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(!g_hfp_app_array[i].is_used)
        {
            return &g_hfp_app_array[i];
        }
    }

    return NULL;
}

uint32_t hfp_has_connection(void)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used) && (g_hfp_app_array[i].flag & APP_FLAG_HFP_CONNECTION))
        {
            return 1;
        }
    }

    return 0;
}

uint32_t hfp_is_connection_based_on_raddr(btaddr_t *raddr)
{
    uint32_t i;
    uint32_t connected = 0;
    hfp_hf_app_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_hfp_app_array[i];
        if((app_ptr->is_used)
            && (btaddr_same(raddr, &app_ptr->raddr))
            && (app_ptr->flag & APP_FLAG_HFP_CONNECTION))
        {
            connected = 1;
            break;
        }
    }

    return connected;
}

uint32_t hfp_is_connecting_based_on_raddr(btaddr_t *raddr)
{
    uint32_t i;
    uint32_t connecting = 0;
    hfp_hf_app_t *app_ptr = NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_hfp_app_array[i];
        if((app_ptr->is_used)
            && (btaddr_same(raddr, &app_ptr->raddr)))
        {
            connecting = 1;
            break;
        }
    }

    return connecting;
}

void hf_set_flag(void *arg, uint32_t flag)
{
    hfp_hf_app_t *app_ptr = (hfp_hf_app_t *)arg;
    app_ptr->flag |= flag;
}

void hf_clear_flag(void *arg, uint32_t flag)
{
    hfp_hf_app_t *app_ptr = (hfp_hf_app_t *)arg;
    app_ptr->flag &= ~flag;
}

uint32_t hf_check_flag(void *arg, uint32_t flag)
{
    hfp_hf_app_t *app_ptr = (hfp_hf_app_t *)arg;
    return (app_ptr->flag & flag);
}

uint32_t get_hf_priv_flag(uint8_t idx,uint32_t flag)
{
    if(g_hfp_app_array[idx & 0x01].is_used)
        return(g_hfp_app_array[idx & 0x01].flag & flag);
    
    return 0;
}

uint32_t hf_clear_autoconn_diconned_flag(void *raddr)
{
    uint32_t i;
    uint32_t executed=0;
    hfp_hf_app_t *app_ptr=NULL;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_hfp_app_array[i];
        if((app_ptr->is_used) && (btaddr_same(raddr, &app_ptr->raddr)))
        {
            executed = 1;
            hf_clear_flag(app_ptr, FLAG_HFP_AUTOCONN_DISCONNED);
            break;
        }
    }

    app_bt_flag1_set(FLAG_HFP_AUTOCONN_DISCONNED, 0);

    return executed;
}

uint32_t hfp_get_remote_addr(void *arg, btaddr_t *r_addr)
{
    hfp_hf_app_t *app_ptr = (hfp_hf_app_t *)arg;

    btaddr_copy(r_addr, &app_ptr->raddr);

    return UWE_OK;
}

void hfp_update_current_app(hfp_hf_app_t *app_ptr)
{
    g_hf_current_app_ptr = app_ptr;
}

uint32_t get_current_hfp_freq(void)
{
    if (!g_hf_current_app_ptr)
        return 0;
    return g_hf_current_app_ptr->freq;
}

uint32_t get_current_hfp_flag(uint32_t flag)
{
    if (!g_hf_current_app_ptr)
        return 0;

    return (g_hf_current_app_ptr->flag&flag);
}

void set_current_hfp_flag(uint32_t flag, uint8_t op)
{
    if (!g_hf_current_app_ptr)
        return;
    
    if(op)
        g_hf_current_app_ptr->flag |= flag;
    else
        g_hf_current_app_ptr->flag &= ~flag;
}

hfp_hf_app_t * hfp_find_app_by_addr(btaddr_t *btaddr)
{
	uint8_t i = 0;
	for (i = 0; i < BT_MAX_AG_COUNT; ++i)
	{
		if(btaddr_same(&g_hfp_app_array[i].raddr, btaddr))
			return &g_hfp_app_array[i];
	}
	return NULL;
}

void set_hf_flag_1toN(btaddr_t *btaddr, uint32_t flag)
{
    uint32_t i;

	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if(g_hfp_app_array[i].is_used && btaddr_same(&g_hfp_app_array[i].raddr, btaddr))
			hf_set_flag(&g_hfp_app_array[i],flag);
	}
}

void clear_hf_flag_1toN(btaddr_t *btaddr, uint32_t flag)
{
    uint32_t i;

	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if(g_hfp_app_array[i].is_used && btaddr_same(&g_hfp_app_array[i].raddr, btaddr))
    	{
			hf_clear_flag(&g_hfp_app_array[i], flag);
    	}
	}
}

uint32_t get_hfp_flag_1toN(btaddr_t *btaddr, uint32_t flag)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_hfp_app_array[i].is_used && btaddr_same(&g_hfp_app_array[i].raddr,btaddr))
        {
            return (g_hfp_app_array[i].flag&flag);
        }
    }
    LOG_E(APP,"g_hfp_app_array addr not match!\r\n");
    return 0;
}

uint32_t has_hfp_flag_1toN(uint32_t flag)
{
    uint32_t i;
    uint32_t num = 0;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if(g_hfp_app_array[i].is_used && (g_hfp_app_array[i].flag&flag))
    	{
		    num++;
    	}
    }
    return num;
}

void set_hf_flag_call_seq(btaddr_t *btaddr)
{
    uint8_t i;
    uint8_t index = 0;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if(g_hfp_app_array[i].is_used && btaddr_same(&g_hfp_app_array[i].raddr,btaddr))
    	{
            index = i;
            break;
    	}
    }
    
    if((index == BT_MAX_AG_COUNT) || hf_check_flag(&g_hfp_app_array[index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1|APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2))
    {
        LOG_I(APP,"No change,index:%d,:0x%x, addr:"BTADDR_FORMAT"\r\n",index,hf_check_flag(&g_hfp_app_array[index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1|APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2),BTADDR(btaddr));
        return;
    }
    else
    {
        if(g_hfp_app_array[1-index].is_used && (hf_check_flag(&g_hfp_app_array[1-index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1)))
        {
            hf_set_flag(&g_hfp_app_array[index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2);
        }
        else if(g_hfp_app_array[1-index].is_used && hf_check_flag(&g_hfp_app_array[1-index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2))
        {
            hf_set_flag(&g_hfp_app_array[1-index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1);
            hf_clear_flag(&g_hfp_app_array[1-index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2);
            hf_set_flag(&g_hfp_app_array[index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2);
        }
        else
        {
            hf_set_flag(&g_hfp_app_array[index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1);
        }
    }
    LOG_I(APP,"set seq:0x%x, addr:"BTADDR_FORMAT"\r\n",hf_check_flag(&g_hfp_app_array[index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1|APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2),BTADDR(btaddr));
    if(g_hfp_app_array[1-index].is_used)
        LOG_I(APP,"set seq:0x%x, addr:"BTADDR_FORMAT"\r\n",hf_check_flag(&g_hfp_app_array[1-index],APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1|APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2),BTADDR(&g_hfp_app_array[1-index].raddr));
}

uint8_t hf_exchange_sco_active_flag(btaddr_t *btaddr,t_sco_active_state state)
{
	uint32_t i;
	uint8_t sco_cnt = 0;
    
	for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if(g_hfp_app_array[i].is_used&& (g_hfp_app_array[i].flag & APP_FLAG_SCO_CONNECTION))
    	{
    		sco_cnt++;
    		if(state == SCO_DISCONNECTED)
    		{
	    		if(btaddr_same(&g_hfp_app_array[i].raddr,btaddr))
    				g_hfp_app_array[i].flag &= ~APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE;
    			else
    				g_hfp_app_array[i].flag |= APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE;
    		}
            else if(state == SCO_CONNECTED)
    		{
    			if(btaddr_same(&g_hfp_app_array[i].raddr,btaddr))
    				g_hfp_app_array[i].flag |= APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE;
    			else
    				g_hfp_app_array[i].flag &= ~APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE;
    		}
    	}
	}
	if((state == SCO_SWAP) && (sco_cnt > 1))
	{
		for(i = 0; i < BT_MAX_AG_COUNT; i++)
		{
			g_hfp_app_array[i].flag ^= APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE;
			if(g_hfp_app_array[i].flag & APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE)
			{
				g_hf_current_app_ptr = &g_hfp_app_array[i];
			}
		}
	}
    return sco_cnt;
}

uint16_t get_hf_active_sco_handle(void)
{
    uint8_t i;
	hfp_hf_app_t *app_ptr = NULL;
    hci_link_t * tmp_link = NULL;
    app_handle_t sys_hdl;
    
    sys_hdl = app_get_sys_handler();
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_hfp_app_array[i];
        if (app_ptr->is_used && (app_ptr->flag & APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE))
        {
	    	tmp_link = hci_sco_link_lookup_btaddr(sys_hdl->unit, &app_ptr->raddr, HCI_LINK_eSCO);
	    	if(tmp_link)
	    	{
				return tmp_link->hl_handle;
			}
        }
    }
	return 0;
}

void hfp_2eSCOs_A_B_SWAP(void)
{
	uint16_t sco_handle;
    uint8_t sco_cnt;
    
	sco_cnt = hf_exchange_sco_active_flag(NULL,SCO_SWAP);
    if(sco_cnt > 1)
    {
	    sco_handle = get_hf_active_sco_handle();
        if(sco_handle > 0)
        {
        	bt_exchange_hf_active_by_handle(sco_handle);
            if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
                hf_audio_restore();
        }
    }
}

void set_2hfps_sco_and_incoming_flag(uint8_t val)
{
    g_active_sco_and_incoming_flag = val;
}

uint8_t get_2hfps_sco_and_incoming_flag(void)
{
    return g_active_sco_and_incoming_flag;
}

uint8_t check_2hfps_sco_and_incoming_status(void)
{
    uint8_t ret,ret1 = 0,ret2 = 0;
    uint8_t i;
	hfp_hf_app_t *app_ptr = NULL;
    
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        app_ptr = &g_hfp_app_array[i];
        if(app_ptr->is_used && (app_ptr->flag & APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE)&& (app_ptr->flag & APP_FLAG_CALL_ESTABLISHED))
        {
            ret1++;
            continue;
        }
        if(app_ptr && (!(app_ptr->flag & APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE)) && (app_ptr->flag & APP_FLAG2_HFP_INCOMING))
        {
            ret2++;
        }
    }
    ret = ((ret1 * ret2) > 0);
    set_2hfps_sco_and_incoming_flag(ret);
    return (ret);

}

static hfp_hf_app_t *get_inactive_sco_hfp_ptr(void)
{
    uint8_t i;
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if(g_hfp_app_array[i].is_used
           && (g_hfp_app_array[i].flag & APP_SCO_PRIVATE_FLAG_CONNECTION)
           && !(g_hfp_app_array[i].flag & APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE)
           )
        {
            return &g_hfp_app_array[i];
        }

    }
    return NULL;
}

int hf_sco_disconn_reconn(int oper)
{
    LOG_D(APP,"%s\r\n",__func__);
    hfp_hf_app_t *app_ptr = NULL;
    
    app_ptr = get_inactive_sco_hfp_ptr();
    if(app_ptr == NULL)
        return -1;
    if( oper == 0 )
        return util_sco_connect(app_ptr->priv);
    else if( oper == 1 )
    	return bt_hfp_hf_hangup_call(app_ptr->session);
    else
        return -1;
}

void change_cur_hfp_to_another_call_conn(btaddr_t *btaddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used) && (!btaddr_same(&g_hfp_app_array[i].raddr,btaddr))
			&&(g_hfp_app_array[i].flag&(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED)))
        {
            g_hf_current_app_ptr = &g_hfp_app_array[i];
			break;
        }
    }
}

uint32_t hfp_select_current_app_to_another(btaddr_t *btaddr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used)
			 && (g_hfp_app_array[i].flag & APP_FLAG_HFP_CONNECTION)
			 && (!btaddr_same(&g_hfp_app_array[i].raddr, btaddr)))
        {
            g_hf_current_app_ptr = &g_hfp_app_array[i];
			return 1;
        }
    }
    
    //g_hf_current_app_ptr = NULL;
    LOG_I(APP,"====current_hfp_ptr is null=====\r\n");
    return 0;
}

uint32_t hfp_has_the_connection(btaddr_t addr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used)
            && btaddr_same(&g_hfp_app_array[i].raddr,&addr)
            && (g_hfp_app_array[i].flag & APP_FLAG_HFP_CONNECTION) )
        {
            return 1;
        }
    }

    return 0;
}

uint32_t hfp_has_sco(void)
{
    uint32_t i;
    uint32_t num = 0;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used)
            && (g_hfp_app_array[i].flag & APP_FLAG_SCO_CONNECTION) )
        {
            num++;
        }
    }
    return num;
}

uint32_t hfp_has_the_sco(btaddr_t addr)
{
    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used)
            && btaddr_same(&g_hfp_app_array[i].raddr,&addr)
            && (g_hfp_app_array[i].flag & APP_FLAG_SCO_CONNECTION) )
        {
            return 1;
        }
    }

    return 0;
}

uint32_t hfp_check_autoconn_disconned_flag(btaddr_t *r_addr)
{
    hfp_hf_app_t *app_ptr=NULL;

    app_ptr = hfp_app_lookup_valid(r_addr);

    if (app_ptr)
        return app_ptr->flag & FLAG_HFP_AUTOCONN_DISCONNED;
    else
        // The app_ptr may be freed at this point. Then borrow the system flag
        return app_bt_flag1_get(FLAG_HFP_AUTOCONN_DISCONNED);
}

uint8_t set_iphone_bat_lever(void)
{
    volatile uint8 i = 0;
    static int8 last_level = -1;
    static uint8 last_level_cnt = 0;
    uint16 temp = 0,v_level;
    app_env_handle_t env_h = app_env_get_handle();
    app_bat_display_t *bat_level_tab = &env_h->env_cfg.feature.bat_display_level;

    temp = saradc_get_bat_value(); 
    for(i = 0; i < 10; i ++)
    {
        if(bat_level_tab ->bat_level[i]<500)
            v_level = bat_level_tab ->bat_level[i]*10; //v_data
        else
            v_level = saradc_transf_adc_to_vol(bat_level_tab ->bat_level[i]);

        if(temp <= v_level)
        {
            if(i < 0)
                i = 0;
            if(i > 9)
                i = 9;
            if((i > last_level)&&(-1 != last_level))
            { //advoid saradc deviation/jitter
                if(temp < bat_level_tab ->bat_level[last_level+1]*10 && !app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))
                {
                    i = (uint8)last_level;
                    last_level_cnt = 0;
                }
                else
                {
                    last_level_cnt ++;
                    if(last_level_cnt > 3)
                    {
                        last_level = i;
                        last_level_cnt = 0;
                    }
                    else
                    {
                        i = last_level;
                    }
                }
            }
            else
            {
                last_level = i;
                last_level_cnt = 0;
            }
            //os_printf("bat_level[%d]:%d,%d,  adc:%d\r\n",i,bat_level_tab ->bat_level[i],last_level,temp);
            return i;
        }
    }
    last_level = i;
    return 9;
}

static void hfp_hf_connected(bt_hfp_hf_session_h session, bt_app_ctx_h app_ctx)
{
    hfp_hf_app_t *app_ptr;
	app_env_handle_t env_h = app_env_get_handle();

    app_ptr = hfp_get_app_from_session(session);

    if(NULL == app_ptr) 
    {
        LOG_E(APP,"hfp_hf_connected_null_app_ptr\r\n");
        return;
    }

    hf_set_flag(app_ptr, APP_FLAG_HFP_CONNECTION);
    app_bt_profile_conn_wrap(PROFILE_BT_HFP,&app_ptr->raddr);

    if(hf_check_flag(app_ptr, APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
    {
        //first,if the newest connected ag is calling,we both need to (update the hf_current_app to the newest connected ag) and (enable hfp_button).
        g_hf_current_app_ptr = app_ptr;
        app_button_type_set(BUTTON_TYPE_HFP);
        if(app_bt_flag2_get(APP_FLAG2_HFP_SECOND_CALLSETUP)) /* for twc button mode */
            app_button_type_set(BUTTON_TYPE_TWC);

        LOG_D(CONN," hfp_hf_connected() 3, ag2 is calling,0x%x\r\n",g_hf_current_app_ptr);
    }
    else if (!has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
    {
        //second,if there is no call,we update the hf_current_app to the newest connected ag.
	    g_hf_current_app_ptr = app_ptr;

        LOG_D(CONN,"hfp_hf_connected() 2, updated to ag2,no calling,0x%x\r\n",g_hf_current_app_ptr);
    }

    if(NULL == g_hf_current_app_ptr) 
    {
        //last,if there was no ag connected before, just update the hf_current_app to the newest connected ag.
        g_hf_current_app_ptr = app_ptr;
        LOG_D(CONN,"hfp_hf_connected() 1, first ag connected,0x%x\r\n",g_hf_current_app_ptr);
    }

    app_ptr->volume = env_h->env_cfg.system_para.vol_hfp; //initial the hfp volume

    if(hf_check_flag(app_ptr, APP_HFP_PRIVATE_FLAG_CODEC_NEGOTIATION))
    {
        app_ptr->freq = 8000;
    }
    else
    {
        app_ptr->freq = 8000;
    }
    LOG_I(CONN,"hfp_hf_connected\r\n");
  

    if(app_env_check_bat_display()) 
    {
    	 bt_hfp_hf_xtalapp(session);
    }

#if CONFIG_HFP_CONN_DISCONN
	app_handle_t sys_hdl = app_get_sys_handler();
	if (sys_hdl->hfp_state == APP_HFP_STATE_DISCONN)
	{
		hf_set_disconnect();
	}
	else if (sys_hdl->hfp_state == APP_HFP_STATE_CONN)
	{
		hf_set_connect();
	}
#endif
}

static void hfp_hf_disconnected(bt_hfp_hf_session_h session, result_t err, bt_app_ctx_h app_ctx)
{
    hfp_hf_app_t *app_ptr;

    app_ptr = hfp_get_app_from_session(session);

    if(NULL == app_ptr)
    {
        LOG_E(CONN,"hfp_hf_disconnected_null_app_ptr\r\n");
        return;
    }
    LOG_D(CONN, "hfp_hf_disconnected\r\n");
    //LOG_I(CONN, "hfp_hf_disconnected, addr:"BTADDR_FORMAT"\r\n", BTADDR(&app_ptr->raddr) );
    
    if(app_ptr->is_used)
        util_sco_close(app_ptr->priv);

    app_ptr->is_used = 0;

    /* bt_app_management */
    bt_app_entity_clear_conn_flag_by_addr(&app_ptr->raddr, PROFILE_BT_HFP);
    set_connection_event(&app_ptr->raddr, CONN_EVENT_HFP_DISCONNECTED);
    
    /* bt_app_profile disconnect callback for user*/
    //app_bt_profile_disconn_wrap(PROFILE_ID_HFP, &app_ptr->raddr);

    if(!has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED)) 
        app_button_type_set(BUTTON_TYPE_NON_HFP);

#if CONFIG_BLUETOOTH_PBAP
    extern uint8_t get_remote_device_type(btaddr_t* rtaddr);
    if(get_remote_device_type( &app_ptr->raddr)==1)
    {
    	extern result_t pbap_cmd_disconn(btaddr_t *raddr);
    	pbap_cmd_disconn(&app_ptr->raddr);
    }
#endif

    
    if (hfp_select_current_app_to_another(&app_ptr->raddr) == 0)
    {
		hfp_update_current_app(NULL);
    }
    hfp_app_uninit(app_ptr);
}

static void hfp_hf_newconn(bt_conn_req_h conn_req,
                                  const btaddr_t *laddr,
                                  const btaddr_t *raddr,
                                  bt_link_modes_mask_t mode,
                                  bt_app_ctx_h app_server_ctx)
{
    hfp_hf_app_t *app_ptr;

    app_ptr = hfp_app_lookup_valid(raddr);

    if((0 == app_ptr)
       || (app_ptr->is_used)
       || (app_ptr->conn_req))
    {
        bt_hfp_hf_conn_reject(conn_req);
        return;
    }

    if(!app_ptr->is_used)
    {
        LOG_I(CONN, "hfp_hf_newconn\r\n");
        hfp_app_init(app_ptr);
    }

    j_memcpy((void *)&app_ptr->laddr, (void *)laddr, sizeof(btaddr_t));
    j_memcpy((void *)&app_ptr->raddr, (void *)raddr, sizeof(btaddr_t));
    
    app_ptr->pending_wave_id = APP_WAVE_FILE_ID_NULL;

#if CONFIG_HFP_CONN_DISCONN
	app_handle_t app_h = app_get_sys_handler();
	if (app_h->hfp_state == APP_HFP_STATE_DISCONN)
	{
		bt_hfp_hf_conn_reject(conn_req);
		return;
	}
#endif

    if(bt_hfp_hf_conn_accept(app_ptr->session, conn_req, mode))
    {
        LOG_I(CONN,"hf_newconn %lu failed\r\n", app_ptr->svc_id);
    }
    else
    {
        app_ptr->is_used = 1;
        LOG_I(CONN,"hf_newconn=%lu, %x\r\n", app_ptr->svc_id, app_ptr);
    }

    return;
}

void hfp_hf_input(bt_hfp_hf_session_h session, at_result_t *at_result,
                      bt_app_ctx_h app_ctx)
{
    hfp_hf_app_t *app_ptr;

    CLEAR_SLEEP_TICK;

    app_ptr = hfp_get_app_from_session(session);

    hf_at_result_print(app_ptr, at_result);
}

void hfp_get_features(bt_hfp_hf_session_h session_h, uint16_t *supported_features, 
                           bt_app_ctx_h app_ctx)
{
    if(supported_features)
        *supported_features = HF_SUPPORTED_FEATURES;
}

int8_t get_current_hfp_volume(void)
{
    if (!g_hf_current_app_ptr)
        return 0;
    return g_hf_current_app_ptr->volume;
}

uint16_t hfp_check_features(uint16_t flag)
{
	return HF_SUPPORTED_FEATURES & flag;
}

result_t hf_cmd_connect(char *params, unsigned int len)
{
    result_t err;
    btaddr_t laddr, raddr;
    int btaddr_tmp[6];
    uint32_t channel;
    uint32_t unit_id;
    hfp_hf_app_t *app_ptr;

    LOG_D(CONN,"hf_cmd_connect\r\n");

    if(j_snscanf(params,
                 NULL,
                 "%u " BTADDR_FORMAT " %u",
                 &unit_id,
                 BTADDR_TMP(btaddr_tmp),
                 &channel) != 8)
    {
        err = UWE_INVAL;
        goto Exit;
    }

    TMP_TO_BTADDR(btaddr_tmp, &raddr);

    if(hfp_is_connection_based_on_raddr(&raddr))
    {
        LOG_E(CONN,"hf_connected_raddr, UWE_ALREADY!! \r\n");
        return UWE_ALREADY;
    }

    app_ptr = hfp_app_lookup_valid(&raddr);
    
    if(app_ptr == NULL)
    {
        err = UWE_NODEV;
        LOG_E(CONN,"hf_cmd_connect app_ptr NULL!\r\n");
        goto Exit;
    }

    if(!app_ptr->is_used)
    {
        hfp_app_init(app_ptr);
    }
    else
    {
        err = UWE_ALREADY;
        LOG_E(CONN,"exit1, %d \r\n", err);
        goto Exit;
    }

    err = backend_unit_get_addr(unit_id, &laddr);
    if (err)
    {
        LOG_E(CONN,"exit2, %d \r\n", err);
        err = UWE_NODEV;
        goto Exit;
    }

    j_memcpy((void *)&app_ptr->laddr, (void *)&laddr, sizeof(btaddr_t));
    j_memcpy((void *)&app_ptr->raddr, (void *)&raddr, sizeof(btaddr_t));

    app_ptr->pending_wave_id = APP_WAVE_FILE_ID_NULL;

    err = bt_hfp_hf_conn_connect(app_ptr->session,
                                 &laddr,
                                 &raddr,
                                 (uint8_t)channel);
    if (err)
    {
        LOG_E(CONN,"exit3, %d \r\n", err);
        goto Exit;
    }

    app_ptr->is_used = 1;
 Exit:
    return err;
}

result_t hf_cmd_disconnect(void)
{
    if(!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
    {
        LOG_W(HF,"No hf app\r\n");
        return UWE_NODEV;
    }

    return bt_hfp_hf_conn_disconnect(g_hf_current_app_ptr->session);
}

result_t hf_cmd_accept_call(void)
{
    result_t ret = 0;

    if (!g_hf_current_app_ptr)
        return UWE_NODEV;

    g_hf_current_app_ptr->pending_wave_id = APP_WAVE_FILE_ID_HFP_ACK;
    hf_set_flag(g_hf_current_app_ptr, APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
    ret = bt_hfp_hf_accept_call(g_hf_current_app_ptr->session);

    g_hf_current_app_ptr->flag_establish_call = 1;
    return ret;
}

result_t hf_cmd_hangup_call(int8_t reason) // 0-- cancel , 1-- reject
{
     if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    if(reason == 0)
    {
        g_hf_current_app_ptr->pending_wave_id = APP_WAVE_FILE_ID_HFP_CANCEL;
    }
    else if(reason == 1)
    {
        g_hf_current_app_ptr->pending_wave_id = APP_WAVE_FILE_ID_HFP_REJECT;
    }
    hf_set_flag(g_hf_current_app_ptr, APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
    return bt_hfp_hf_hangup_call(g_hf_current_app_ptr->session);
}

result_t hf_cmd_place_call(char number[32])
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;
    hf_set_flag(g_hf_current_app_ptr, APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
    return bt_hfp_hf_place_call(g_hf_current_app_ptr->session, number);
}

result_t hf_cmd_redial(void)
{
    result_t ret;

    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    g_hf_current_app_ptr->pending_wave_id = APP_WAVE_FILE_ID_REDIAL;

    ret = bt_hfp_hf_redial(g_hf_current_app_ptr->session);

    g_hf_current_app_ptr->flag_establish_call = 1;
    hf_set_flag(g_hf_current_app_ptr, APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
    return ret;
}

result_t hf_cmd_resp_and_hold(uint32_t command)
{
    result_t rc = UWE_PARAM;

    switch (command)
    {
        case AT_BTRH_HOLD_INCOMING:
        case AT_BTRH_ACCEPT_HELD:
        case AT_BTRH_REJECT_HELD:
            rc = bt_hfp_hf_resp_and_hold_set(g_hf_current_app_ptr->session, command);
            break;

        default:
            /* TODO: send error notification */
            break;
    }

    return rc;
}

result_t hf_cmd_resp_and_hold_read(void)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    hf_set_flag(g_hf_current_app_ptr, APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
    return bt_hfp_hf_resp_and_hold_read(g_hf_current_app_ptr->session);
}

result_t hf_cmd_set_call_wait_notif(uint32_t enable)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    if(enable > 1)
        return UWE_PARAM;
    
    hf_set_flag(g_hf_current_app_ptr, APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
    return bt_hfp_hf_set_call_wait_notif(g_hf_current_app_ptr->session, (uint8_t)enable);
}

result_t hf_cmd_chld(uint32_t command, uint32_t call_id)
{
    result_t rc = UWE_OK;

    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    switch (command)
    {
        case AT_CHLD_RELEASE_ALL_ACTIVE_ACCEPT_HELD:
        case AT_CHLD_HOLD_ALL_EXCEPT_INDICATED:
            /* Also catches the following cases */
            /* case AT_CHLD_RELEASE_INDICATED_ACTIVE: */
            /* case AT_CHLD_HOLD_ALL_ACTIVE_ACCEPT_HELD: */
            break;

        case AT_CHLD_RELEASE_ALL_HELD:
        case AT_CHLD_MULTIPARTY:
        case AT_CHLD_EXPLICIT_CALL_TRANSFER:
            break;

        default:
            rc = UWE_PARAM;
            /* TODO: send error notification */
            break;
    }

    if(!rc)
    {
        rc = bt_hfp_hf_chld(g_hf_current_app_ptr->session, (uint8_t)command, (uint8_t)call_id);
    }
    return rc;
}

result_t hf_cmd_list_current_calls(void)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_list_current_calls(g_hf_current_app_ptr->session);
}

result_t hf_cmd_set_cli_notif(uint32_t enable)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_set_cli_notif(g_hf_current_app_ptr->session, enable);
}

BOOL is_ag_support_feature(uint8_t feature) 
{
    LOG_I(APP,"is_ag_support_feature, 0x%x, 0x%x\r\n",g_hf_current_app_ptr,feature);

    return g_hf_current_app_ptr && bt_is_ag_support_feature(g_hf_current_app_ptr->session, feature);
}

/*********************************
* function:hf_cmd_set_voice_recog
* enable:
* 1 ->  open voice_recog in ag
* 0 -> close voice_recog in ag
*********************************/
result_t hf_cmd_set_voice_recog(uint32_t enable) 
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_set_voice_recog(g_hf_current_app_ptr->session, enable);
}

result_t hf_cmd_trans_DTMF(char * code)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_trans_DTMF(g_hf_current_app_ptr->session, code);
}

result_t hf_cmd_get_operator(void)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_get_operator(g_hf_current_app_ptr->session);
}

result_t hf_cmd_get_subscriber(void)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_get_subscriber(g_hf_current_app_ptr->session);
}

result_t hf_cmd_set_iphone_batlevel(int8_t level)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    uint32_t i;

    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used)
        	&&(g_hfp_app_array[i].flag & APP_FLAG_HFP_CONNECTION)
        	&&(g_hfp_app_array[i].flag & APP_FLAG_HFP_BAT_LEVEL)
        	)
        {
            bt_hfp_hf_iphoneaccev(g_hfp_app_array[i].session, level);
        }
    }
    return UWE_OK;
}

result_t hf_cmd_set_vgs(uint8_t oper)
{
    int8_t volume;

    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    if(oper == 0) //minus
    {
        g_hf_current_app_ptr->volume--;
        if(g_hf_current_app_ptr->volume <= AUDIO_VOLUME_MIN)
            g_hf_current_app_ptr->volume = AUDIO_VOLUME_MIN;
    }
    else  //plus
    {
        g_hf_current_app_ptr->volume++;
        if(g_hf_current_app_ptr->volume >= AUDIO_VOLUME_MAX)
            g_hf_current_app_ptr->volume = AUDIO_VOLUME_MAX;
    }

    volume = g_hf_current_app_ptr->volume;
    LOG_I(APP,"hf_cmd_set_vgs volume = %d\r\n",volume);

    if(volume <= AUDIO_VOLUME_MIN)// && (!bt_audio_ch_busy()))
    {
        app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 1);
		if(!bt_audio_ch_busy())
        {
        	aud_volume_mute(1);
		}
    }
    else
    {
        app_bt_flag2_set(APP_FLAG2_VOL_MUTE, 0);
        aud_volume_mute(0);
    }

    aud_dac_set_volume(volume);

    app_wave_file_vol_s_init(volume);//for wave file play over to restore the changed volume

    if(volume == AUDIO_VOLUME_MAX) // max
    {
        LOG_I(APP,"max2\r\n");
//yuan++	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
    }
    else if(volume == AUDIO_VOLUME_MIN) // min
    {
        LOG_I(APP,"min2\r\n");
//yuan++	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
    }

    /*peer only reponse AT_RESULT_OK with below AT_CMD*/
    if(volume <= 0)
        return bt_hfp_hf_vgs(g_hf_current_app_ptr->session, 0);
    else
        return bt_hfp_hf_vgs(g_hf_current_app_ptr->session, g_hf_current_app_ptr->volume-1);

    return UWE_OK;
}

void hf_audio_restore(void)
{
    if(get_current_hfp_flag(APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE)
        || has_hfp_flag_1toN(APP_FLAG2_HFP_INCOMING)
        || hfp_has_sco())
    {
        set_flag_sbc_buffer_play(0);
        aud_dac_close();
        aud_dac_set_volume(g_hf_current_app_ptr->volume);

        aud_dac_config(g_hf_current_app_ptr->freq, 2, 16);
        aud_mic_config(g_hf_current_app_ptr->freq, 1, 16);

        aud_dac_open();
        aud_mic_open(1);
        
        app_wave_file_aud_notify(g_hf_current_app_ptr->freq, 1, g_hf_current_app_ptr->volume);
#if(CONFIG_AUD_FADE_IN_OUT == 1)
        set_aud_fade_in_out_state(AUD_FADE_IN);
#endif
    }
}

void app_bt_incoming_ring_ios( void *arg )
{

	if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
//yuan++	app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_RING);

	if(!app_bt_flag1_get(APP_FLAG_CALL_ESTABLISHED))
		app_bt_shedule_task((jthread_func)app_bt_incoming_ring_ios,
        					(void *)arg,
        					(uint32_t)arg);
}

static void hf_at_result_print(hfp_hf_app_t *app_ptr, const at_result_t *at_result)
{
    app_env_handle_t env_h = app_env_get_handle();

    app_handle_t sys_hdl = app_get_sys_handler();

    if(app_bt_flag2_get(APP_FLAG2_CALL_PROCESS)&&(app_ptr != g_hf_current_app_ptr))
    {
        LOG_I(AT,"=====not current hf:%p\r\n",app_ptr);
    }

    switch (at_result->code)
    {
        case AT_RESULT_RING:
            LOG_I(AT,"AG ringing ...");
            app_bt_flag2_set(APP_FLAG2_HFP_INCOMING, 1);
            app_button_type_set(BUTTON_TYPE_HFP);
            app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP, 1);
            set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG2_HFP_INCOMING);
            set_hf_flag_call_seq(&app_ptr->raddr);
            LOG_I(AT,"~~~call num:%d,%d,current call seq:0x%x,ring count:%d\r\n",
            get_2hfps_sco_and_incoming_flag(),
            has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED),
            get_hfp_flag_1toN(&app_ptr->raddr,APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1 |APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2),
            app_ptr->ring_count);
            
            app_ptr->ring_count++;
            
            if((!get_2hfps_sco_and_incoming_flag()) &&
                (((has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED)>1)
                && (get_hfp_flag_1toN(&app_ptr->raddr, APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2)))
                || (has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED)<=1))
                )
            {
                if((app_ptr->ring_count >= 2) && (env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_ADDR_AUDIO_DIAL))
                {
                    if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
                    {
                        if((app_ptr->vnum[0] != 0xFF) && (app_ptr->vnum[0] != 0x00))
                        {
                            int i = 0;

                            while(app_ptr->vnum[i] < '0' || app_ptr->vnum[i] > '9')
                            {
                                i++;
                            }
                            app_bt_shedule_task((jthread_func)app_wave_file_play_start, 
                                                 (void *)(APP_WAVE_FILE_ID_VOICE_NUM_0+(app_ptr->vnum[i]-'0')), 
                                                 200);
                            app_wave_file_voice_num_set((char *)&app_ptr->vnum[i+1]);
                        }
                    }
                }
                else
                {
                    //if has sco, phone's ring will transfer to handset
                    if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING) )
                    {
    					if((app_check_bt_mode(BT_MODE_1V2) && !hfp_has_the_sco(app_ptr->raddr))
    						||(!app_check_bt_mode(BT_MODE_1V2) && !hfp_has_sco())
    						)
    					{
    						app_bt_shedule_task((jthread_func)app_wave_file_play_start,
    									        (void *)APP_WAVE_FILE_ID_HFP_RING,
                                                200);
    					}
    					else if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DISABLE_IOS_INCOMING_RING))
    					{
    						app_bt_shedule_task((jthread_func)app_bt_incoming_ring_ios,
    											(void *)7000,
    											200);
    					}
    				}
                }
            }
            app_set_led_event_action(LED_EVENT_INCOMING_RINGING);
            break;
            
        case AT_RESULT_BCS:
            LOG_I(AT,"===bcs:%d\r\n",at_result->u.bcs);
            if(at_result->u.bcs == 2)
            {
                bt_hfp_hf_set_codec(app_ptr->session,2);
                app_ptr->freq = 16000;
            }
            else
            {
                bt_hfp_hf_set_codec(app_ptr->session,1);
                app_ptr->freq = 8000;
            }
            break;
            
#ifdef CONFIG_PRODUCT_TEST_INF
        case AT_RESULT_BKCMD:
            if(at_result->u.bkcmd == 1)
            {
                uint32_t val = 0;
                val = 0x40000000 | (uint32_t)( (aver_rssi&0xff)<<16) | ((uint32_t)aver_offset&0x1ff);
                hf_cmd_trans_buttoncode(val);
                LOG_I(AT,"hf_cmd_trans_buttoncode:0x%x,aver_rssi:%x,offset:%x\r\n",val,aver_rssi,aver_offset);
            }
            break;
#endif
        case AT_RESULT_VGS:
            LOG_I(AT,"vol:%d", at_result->u.vgs);
            extern uint8 app_bt_get_phone_type(void);
        
            if(app_bt_get_phone_type() == 2|| at_result->u.vgs <= 0) 
            {
                app_ptr->volume = at_result->u.vgs + 1;
            }
            else
            {
                app_ptr->volume = at_result->u.vgs;
            }
    		aud_dac_set_volume(app_ptr->volume);
            break;

        case AT_RESULT_VGM:
            LOG_I(AT,"AG microphone volume changed %d", at_result->u.vgm);

            aud_mic_set_volume(at_result->u.vgm);
            break;

        case AT_RESULT_BSIR:
            LOG_I(AT,"in-band ringtone is%s audible",
                                     at_result->u.bsir ? "" : " not");
            break;

        case AT_RESULT_BTRH:
            LOG_I(AT,"AG Response and Hold status - %s",
                                     at_result->u.btrh == AT_BTRH_HOLD_INCOMING ?  "call on hold" :
                                     at_result->u.btrh == AT_BTRH_ACCEPT_HELD ?  "held call accepted" :
                                     "held call rejected");
            break;

        case AT_RESULT_BVRA:
            LOG_I(AT," hf_at_result_print, u.bvra=%d\r\n",at_result->u.bvra);
            if(at_result->u.bvra == 0)
            {
                if(app_bt_flag1_get(APP_FLAG_MUSIC_PLAY_SCHEDULE))
                {
                    app_bt_flag1_set(APP_FLAG_MUSIC_PLAY_SCHEDULE, 0);
                    app_button_sw_action(BUTTON_PLAY_PAUSE);
    				app_button_type_set(BUTTON_TYPE_NON_HFP);
                }
            }else if(at_result->u.bvra == 1)
            {
                //workaround for issue 174
                audio_dac_ana_mute(1);
                aud_dac_buffer_clear();
                aud_dac_dig_volume_fade_in();
            }
            break;

        case AT_RESULT_CIEV:
            {
                const at_result_ciev_t *ciev = &(at_result->u.ciev);
                
                if(ciev->cind_code == CIND_CALL)
                {
                    if(ciev->val == 1)
                    {
                        LOG_I(AT,"ciev1\r\n");
                        app_bt_shedule_task(NULL,NULL,0);
                        app_wave_file_voice_num_set(NULL);
                        app_set_led_event_action( LED_EVENT_CALL_ESTABLISHMENT);
                        app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP, 1);
                        app_bt_flag1_set(APP_FLAG_CALL_ESTABLISHED,1);
                        clear_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_OUTGOING|APP_FLAG2_HFP_INCOMING);
                        set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED);
                        app_ptr->pending_wave_id = APP_WAVE_FILE_ID_HFP_ACK;
                        
                        if(get_2hfps_sco_and_incoming_flag())
                        {
                            set_2hfps_sco_and_incoming_flag(0);
                            if(!get_hfp_flag_1toN(&app_ptr->raddr,APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE))
                            {
                                hfp_2eSCOs_A_B_SWAP();
                            }
                        }
                        
                        check_2hfps_sco_and_incoming_status();
                        if(!has_hfp_flag_1toN(APP_FLAG_HFP_OUTGOING))
                        {
                             app_bt_flag1_set(APP_FLAG_HFP_OUTGOING, 0);
                        }
                        
                        if(!has_hfp_flag_1toN(APP_FLAG2_HFP_INCOMING))
                        {
                             app_bt_flag2_set(APP_FLAG2_HFP_INCOMING, 0);
                        }
                        
                        hfp_update_current_app(app_ptr);
#if (CONFIG_CUSTOMER_2PHONES_HUNG_ACCETP == 1)
                        if (app_get_2phones_hung_accetp())
                        {
                            app_set_2phones_hung_accetp(0);
                            hf_sco_disconn_reconn(1);
                        }
#endif
                        app_button_type_set(BUTTON_TYPE_HFP);
                        os_printf("IG\r\n");
                    }
                    else
                    {
                        LOG_I(AT,"ciev0\r\n");
    		      	app_wave_file_voice_num_set(NULL);
                        app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP, 0);
                        app_bt_flag1_set(APP_FLAG_CALL_ESTABLISHED,0);
                        app_bt_flag2_set(APP_FLAG2_CALL_PROCESS, 0);
                        app_ptr->ring_num=0;
                        app_ptr->vnum[0] = 0;
                        app_ptr->pending_wave_id = APP_WAVE_FILE_ID_HFP_CANCEL;
                        app_ptr->flag_establish_call = 0;

                        if(get_2hfps_sco_and_incoming_flag())
                        {
                            set_2hfps_sco_and_incoming_flag(0);
                            app_ptr->pending_wave_id = APP_WAVE_FILE_ID_NULL;
                        }
                        else
                        {
                            app_ptr->pending_wave_id = APP_WAVE_FILE_ID_HFP_CANCEL;
                        }
                        
                        clear_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_OUTGOING|APP_FLAG2_HFP_INCOMING);
                        clear_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED|APP_FLAG2_CALL_PROCESS);
                        clear_hf_flag_1toN(&app_ptr->raddr,APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1|APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2);

                        if(!has_hfp_flag_1toN(APP_FLAG_HFP_OUTGOING))
                        {
                             app_bt_flag1_set(APP_FLAG_HFP_OUTGOING, 0);
                        }
                        
                        if(!has_hfp_flag_1toN(APP_FLAG2_HFP_INCOMING))
                        {
                             app_bt_flag2_set(APP_FLAG2_HFP_INCOMING, 0);
                        }
                        
                        select_play_a2dp_avrcp((void *)app_ptr);
                        
                        if(has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
                        {
                             change_cur_hfp_to_another_call_conn(&app_ptr->raddr);
    					}
    					else
    					{
    					     app_button_type_set(BUTTON_TYPE_NON_HFP);
    					}
                        
    					if (!has_hfp_flag_1toN(APP_FLAG_CALL_ESTABLISHED))
    						app_set_led_event_action( LED_EVENT_CONN );
                    }
                }
                else if(ciev->cind_code == CIND_CALLSETUP)
                {
                    if(ciev->val == 1)  // incoming call
                    {
                        LOG_I(AT,"===CALLSETUP=1\r\n");
                        if(!get_hfp_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
                        {
                            jtask_stop(sys_hdl->app_auto_con_task);
                            set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG2_HFP_INCOMING|APP_FLAG2_CALL_PROCESS);
                            check_2hfps_sco_and_incoming_status();
                            hfp_update_current_app(app_ptr);
#ifdef BT_SD_MUSIC_COEXIST
                            /* app_incoming_call_enter(); */
#endif
                            app_ptr->ring_count = 0;
        		         	app_button_type_set(BUTTON_TYPE_HFP);
                            app_ptr->dir = 0;
                            app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP, 1);
                        	app_bt_flag2_set(APP_FLAG2_HFP_INCOMING|APP_FLAG2_CALL_PROCESS, 1);
                            os_printf("IC\r\n");
                            hf_cmd_list_current_calls();
                        }
                        else
                        {
                            hf_cmd_list_current_calls();
                            set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,1);
                            app_button_type_set(APP_BUTTON_TYPE_TWC);
                        }
                        
                        #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                        if(bt_sniff_is_policy(bt_sniff_get_index_from_raddr(&app_ptr->raddr)))
                        {
                            app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_raddr(&app_ptr->raddr), 0);
                        }
                        #endif
                    }
                    else if(ciev->val == 2) //outgoing call
                    {
                        LOG_I(AT,"===CALLSETUP=2\r\n");
                        app_button_type_set(BUTTON_TYPE_HFP);
                        app_ptr->dir = 1;
                        os_printf("ID\r\n");
                        set_2hfps_sco_and_incoming_flag(0);
                        app_set_led_event_action( LED_EVENT_OUTGOING_RINGING );
    					app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP, 1);
                        jtask_stop(sys_hdl->app_auto_con_task);
                        hfp_update_current_app(app_ptr);
                        app_bt_flag2_set(APP_FLAG2_CALL_PROCESS, 1);
                        set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG_HFP_OUTGOING|APP_FLAG2_CALL_PROCESS);
                        set_hf_flag_call_seq(&app_ptr->raddr);
                        app_bt_flag1_set(APP_FLAG_HFP_OUTGOING, 1);
                        hf_cmd_list_current_calls();
                    }
                    else if(ciev->val == 0)  // call setup = 0
                    {
                        LOG_I(AT,"===CALLSETUP=0\r\n");
                        app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP,0);
                        clear_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP);

                        if(!get_hfp_flag_1toN(&app_ptr->raddr,APP_FLAG_CALL_ESTABLISHED))
                        {
                            app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED, 0);
                            clear_hf_flag_1toN(&app_ptr->raddr,APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1|APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2);

                            if (!app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
                                app_bt_shedule_task(NULL,NULL,0);

                            app_wave_file_voice_num_set(NULL);
                            app_ptr->vnum[0] = 0;

                            if(get_hfp_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_OUTGOING|APP_FLAG2_HFP_INCOMING))
                            {
                                set_2hfps_sco_and_incoming_flag(0);
                            }

                            app_bt_flag2_set(APP_FLAG2_CALL_PROCESS, 0);

                            clear_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG_HFP_OUTGOING|APP_FLAG2_HFP_INCOMING|APP_FLAG2_CALL_PROCESS);

                            if(!has_hfp_flag_1toN(APP_FLAG_HFP_OUTGOING))
                            {
                                app_bt_flag1_set(APP_FLAG_HFP_OUTGOING, 0);
                            }
                            if(!has_hfp_flag_1toN(APP_FLAG2_HFP_INCOMING))
                            {
                                app_bt_flag2_set(APP_FLAG2_HFP_INCOMING, 0);
                            }

                            select_play_a2dp_avrcp((void *)app_ptr);

                            if(has_hfp_flag_1toN(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
                            {
                               change_cur_hfp_to_another_call_conn(&app_ptr->raddr);
                            }
                            else
                            {
                               app_button_type_set(BUTTON_TYPE_NON_HFP);
                            }

                            if (!has_hfp_flag_1toN(APP_FLAG_CALL_ESTABLISHED))
                            	app_set_led_event_action( LED_EVENT_CONN );
#ifdef BT_SD_MUSIC_COEXIST
                            app_incoming_call_exit();
#endif
                        }
                        else // current hfp has APP_FLAG_CALL_ESTABLISHED,Maybe this hfp is TWC state before callsetup = 0
                        {
                            LOG_I(AT,"===clr twc state\r\n");
                            set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,0);
                            set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,0);
                            app_bt_flag2_set(APP_FLAG2_HFP_SECOND_CALLSETUP, 0 );
                            app_button_type_set(BUTTON_TYPE_HFP);
                        }
                    }
                    else if(ciev->val == 3)
                    {
                        LOG_I(AT,"===CALLSETUP=3\r\n");
                        app_bt_flag1_set( APP_FLAG_HFP_CALLSETUP, 1 );
                        set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP|APP_FLAG_HFP_OUTGOING);
                        app_button_type_set(BUTTON_TYPE_HFP);//re-enable hfp_button.
                    }

                }
                else if( ciev->cind_code == CIND_CALLHELD )
                {
                    /*******************************************************
                    0= No calls held
                    1= Call is placed on hold or active/held calls swapped (The AG has both an active AND a held call)
                    2= Call on hold, no active call
                    *******************************************************/
                    LOG_I(AT,"===CIND CALLHELD:%d\r\n",ciev->val);
                    if( ciev->val == 1 )
                    {
                        app_button_type_set(BUTTON_TYPE_TWC);
                        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,1);
                        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,1);
                        app_bt_flag2_set( APP_FLAG2_HFP_SECOND_CALLSETUP, 1 );
                    }
                    else if(ciev->val == 2)
                    {
                        if(!get_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE))
                        {
                            LOG_I(AT,"===AT chld:2\r\n");
                        }
                    }
                    else
                    {
                        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,0);
                        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,0);
                        app_button_type_set(BUTTON_TYPE_HFP);
                        app_bt_flag2_set( APP_FLAG2_HFP_SECOND_CALLSETUP, 0 );
                    }
                }

                if(app_ptr->pending_wave_id >= 0)
                {
                    app_wave_file_play_start(app_ptr->pending_wave_id);
                    app_ptr->pending_wave_id = -1;
                }
                break;
            }

        case AT_RESULT_CLCC:
            {
                const at_result_clcc_t *clcc = &(at_result->u.clcc);

                LOG_I(AT,"AG current calls %d, %d, %d, %d, %d, %s,"
                                         " %d, %s, %d\r\n", clcc->call_id, clcc->dir, clcc->stat,
                                         clcc->mode, clcc->mpty, clcc->number, clcc->type, clcc->alpha,
                                         clcc->priority);

                 if(((clcc->type == 0x91)&&(clcc->stat == 0))
                     ||(!os_memcmp(&clcc->number[2], "000000", 6) || clcc->number[1] == '"'))
                 {
                     LOG_I(AT,"wechat call\r\n");
                     sys_hdl->is_wechat_phone_call = 1;
                 }
                 else
                 {
                     LOG_I(AT,"normal call\r\n");
                     sys_hdl->is_wechat_phone_call = 0;
                 }

                if (app_bt_flag1_get(APP_FLAG_CALL_ESTABLISHED))
                    break;

                if(app_ptr->dir)
                    os_printf("PR-%s\r\n", clcc->number);
                else if( env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_ADDR_AUDIO_DIAL )
                {
                    os_printf("IR-%s\r\n", clcc->number);
                    memcpy(app_ptr->vnum, clcc->number, 32);

                    app_ptr->ring_count = 0;
                    app_ptr->ring_num   = 1;
                    app_wave_file_voice_num_set((char *)clcc->number);
                }
                break;
            }

        case AT_RESULT_CLIP:
            {
                const at_result_clip_t *clip = &at_result->u.clip;

                LOG_I(AT,"AG calling line identification %s, %d, "
                                         "%s, %d, %s, %d", clip->number, clip->type, clip->subaddr,
                                         clip->satype, clip->alpha, clip->cli_validity);

    			if (app_bt_flag1_get(APP_FLAG_CALL_ESTABLISHED))
    				break;

                if(app_ptr->vnum[0] == 0)
                {
                    os_printf("IR-%s\r\n", clip->number);

                    memcpy(app_ptr->vnum, clip->number, 32);
                    app_ptr->ring_count = 0;
                    app_ptr->ring_num = 2;

                    app_wave_file_voice_num_set((char *)clip->number);
                }
                break;
            }

        case AT_RESULT_COPS:
            LOG_I(AT,"AG operator %s", at_result->u.cops.name);
            break;

        case AT_RESULT_CIND_READ:
            {
                /****************
                AT+CIND=?
                call: Standard call status indicator, where:
                    <value>=0 means there are no calls in progress
                    <value>=1 means at least one call is in progress

                callsetup: Bluetooth proprietary call set up status indicator.
                            Support for this indicator is optional for the HF. When supported,
                            this indicator shall be used in conjunction with, and as an extension of the standard call indicator.
                    Possible values are as follows:
                    <value>=0 means not currently in call set up.
                    <value>=1 means an incoming call process ongoing.
                    <value>=2 means an outgoing call set up is ongoing.
                    <value>=3 means remote party being alerted in an outgoing call.

                callheld: Bluetooth proprietary call hold status indicator. Support for this indicator is mandatory for the AG,
                          optional for the HF. Possible values are as follows:
                     <value>=0: No calls held
                     <value>=1: Call is placed on hold or active/held calls swapped
                                (The AG has both an active AND a held call)
                     <value>=2: Call on hold, no active call
                *****************/
                const int8_t *p = at_result->u.cind.data;

                LOG_D(AT,"AG state {service %d, call %d, "
                                         "callsetup %d, callheld %d, signal %d, roam %d}\r\n",
                                         p[CIND_SERVICE], p[CIND_CALL], p[CIND_CALLSETUP],
                                         p[CIND_CALLHELD], p[CIND_SIGNAL], p[CIND_ROAM]);
                
                if( p[CIND_CALL] > 0 || p[CIND_CALLSETUP] > 0 )
                {
                    LOG_D(AT,"===AT+CIND+READ:%d,%d\r\n",p[CIND_CALL],p[CIND_CALLSETUP]);
                    app_bt_flag1_set( APP_FLAG_HFP_CALLSETUP, 1 );
                    set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP);
                    app_button_type_set( BUTTON_TYPE_HFP );
                    
                    if(p[CIND_CALLSETUP] == 1) /* incoming call */
                    {
                        set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG2_HFP_INCOMING);
                    }
                    else if(p[CIND_CALLSETUP] > 1) /* outgoing call */
                    {
                        set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_OUTGOING);
                    }
                    
                    if(p[CIND_CALL] > 0)
                    {
                        app_bt_flag1_set(APP_FLAG_CALL_ESTABLISHED,1);
                        clear_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_OUTGOING|APP_FLAG2_HFP_INCOMING);
                        set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_CALL_ESTABLISHED);
                        if(p[CIND_CALLSETUP] == 1)  /* 1 call and 1 incoming call, TWC MODE*/
                        {
                            set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,1);
                            set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,1);
                            app_bt_flag2_set( APP_FLAG2_HFP_SECOND_CALLSETUP, 1 );
                            app_button_type_set(APP_BUTTON_TYPE_TWC);
                        }
                    }
                }
                
                if( p[CIND_CALLHELD] > 0 )
                {
                    app_button_type_set(BUTTON_TYPE_TWC);
                    app_bt_flag1_set( APP_FLAG_HFP_CALLSETUP, 1 );
                    set_hf_flag_1toN(&app_ptr->raddr,APP_FLAG_HFP_CALLSETUP);
                    app_bt_flag2_set( APP_FLAG2_HFP_SECOND_CALLSETUP, 1 );
                }
                break;
            }

        case AT_RESULT_CNUM:
            {
                LOG_I(AT,"AG subscriber %s, type %d",
                                         at_result->u.cnum.number, at_result->u.cnum.type);
                break;
            }

        case AT_RESULT_CCWA:
            {
                const at_result_ccwa_t *ccwa = &at_result->u.ccwa;

                LOG_I(AT,"AG waiting call notification "
                                         "%s, %d, %d, %s, %d, %s, %d, %d", ccwa->number, ccwa->type,
                                         ccwa->info_class, ccwa->alpha, ccwa->cli_validity,
                                         ccwa->subaddr, ccwa->satype, ccwa->priority);
                set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,1);
                set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,1);
                app_button_type_set(APP_BUTTON_TYPE_TWC);
                break;
            }

        default:
            break;
    }
    hf_clear_flag(app_ptr,APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING);
}

int hf_sco_handle_process(int oper)
{
    hfp_hf_app_t *app_ptr = NULL;

    app_ptr = g_hf_current_app_ptr;
    LOG_I(APP,"===sco toggle:%d\r\n",oper);
    if( oper == 0 )
        return util_sco_connect(app_ptr->priv);
    else if( oper == 1 )
        return util_sco_disconnect(app_ptr->priv);
    else
        return -1;
}

#ifdef CONFIG_PRODUCT_TEST_INF
result_t hf_cmd_trans_buttoncode(uint32_t value)
{
    if (!g_hf_current_app_ptr || !g_hf_current_app_ptr->session)
        return UWE_NODEV;

    return bt_hfp_hf_trans_buttoncode(g_hf_current_app_ptr->session, value);
}
#endif

#if PTS_TESTING
/* create a sco connection from IUT to PTS. */
void pts_util_sco_connect(void)
{
    result_t err = UWE_OK;
    hfp_hf_app_t *app_ptr = NULL;

    app_ptr = g_hf_current_app_ptr;

    err = util_sco_connect_pts(app_ptr->priv);
    LOG_I(APP,"pts_util_sco_connect, 0x%x, %d \r\n", app_ptr, err);
}

void pts_util_sco_disconnect(void)
{
    result_t err = UWE_OK;
    hfp_hf_app_t *app_ptr = NULL;
    
    app_ptr = g_hf_current_app_ptr;

    err = util_sco_disconnect(app_ptr->priv);
    LOG_I(CONN,"pts_util_sco_disconnect, 0x%x, %d \r\n", app_ptr, err);
}

/* initiate a service level connection from IUT to PTS. */
void pts_util_initiate_slc(void)
{
    result_t err = UWE_OK;
    app_env_handle_t env_h = app_env_get_handle();
    btaddr_t *remote_btaddr = &env_h->env_data.default_btaddr;
    btaddr_t *local_btaddr = &env_h->env_cfg.bt_para.device_addr;

    LOG_I(APP,"pts_util_initiate_slc, local = " BTADDR_FORMAT ", remote = " BTADDR_FORMAT "\r\n", BTADDR(local_btaddr), BTADDR(remote_btaddr));

    err = app_bt_active_conn_profile(PROFILE_ID_HFP, (void *)remote_btaddr);

    LOG_I(APP,"pts_util_initiate_slc, Exit, 0x%x, %d \r\n", env_h, err);
}
#endif

#if CONFIG_HFP_CONN_DISCONN
result_t hf_set_disconnect(void)
{
	uint32_t i,ret=UWE_NOTSTARTED;
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
        if((g_hfp_app_array[i].is_used) && (g_hfp_app_array[i].flag & APP_FLAG_HFP_CONNECTION))
        {
        	bt_hfp_hf_conn_disconnect(g_hfp_app_array[i].session);
			ret = UWE_OK;
        }
    }

	return ret;
}

result_t hf_set_connect(void)
{
	uint32_t i,ret=UWE_NOTSTARTED;
    for(i = 0; i < BT_MAX_AG_COUNT; i ++)
    {
    	if (hci_check_acl_link(bt_app_entit_get_rmt_addr(i))
			&& !hfp_is_connecting_based_on_raddr(bt_app_entit_get_rmt_addr(i)))
    	{
			app_bt_active_conn_profile(PROFILE_ID_HFP,(void *)bt_app_entit_get_rmt_addr(i));
			ret = UWE_OK;
			break;
    	}
    }

	return ret;
}
uint8 hf_conn_condition(void)
{
	app_handle_t sys_hdl = app_get_sys_handler();
	return (sys_hdl->hfp_state == APP_HFP_STATE_CONN);
}
#else 
uint8 hf_conn_condition(void)
{
	return 0;
}
#endif
//  EOF
