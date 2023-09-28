#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_init.h"
#include "reg_ipcore.h"

static app_conn_state g_conn_st;
static auth_status  g_auth_st;//bugfix : delete link key,page back unlimit,do not open scan



void bt_connection_init()
{
    g_conn_st.conn_active = FALSE;
    g_conn_st.link = NULL;
    g_conn_st.addr = jmalloc(sizeof(btaddr_t),M_ZERO);
    g_conn_st.result = CONN_FALSE;
    g_conn_st.profile = 0;
    g_conn_st.event = CONN_EVENT_NULL;
    g_auth_st.auth_status = 0;//bugfix : delete link key,page back unlimit,do not open scan
    g_conn_st.conn_cancel_times = 0;
    g_conn_st.conn_cancel_success = -1;
}

void bt_connection_uninit()
{
    g_conn_st.conn_active = FALSE;
    jfree(g_conn_st.addr);
    g_conn_st.addr = NULL;
    g_conn_st.result = CONN_FALSE;
    g_conn_st.profile = 0;
    g_conn_st.event = CONN_EVENT_NULL;
    g_conn_st.conn_cancel_times = 0;
    g_conn_st.conn_cancel_success = -1;
}

void bt_set_conn_cancel_success(int8_t sel)
{
    g_conn_st.conn_cancel_success = sel;
}

BOOL bt_connection_active(void)
{
    return g_conn_st.conn_active;
}


void bt_connection_req(btaddr_t* addr)
{
    if(g_conn_st.conn_active || g_conn_st.link)
    {
        g_conn_st.result = CONN_FALSE;
        LOG_I(CONN,"Request to connect "BTADDR_FORMAT"is fail,active:%d,link:%p\r\n",BTADDR(addr),g_conn_st.conn_active,g_conn_st.link);
        return;
    }
    
    g_conn_st.conn_active = TRUE;        
    btaddr_copy(g_conn_st.addr,addr);
    g_conn_st.link = NULL;
//    g_conn_st.profile = profile;
    g_conn_st.result = CONN_CONNECTING;
    g_conn_st.state = CONN_START;
    g_conn_st.event = CONN_EVENT_NULL;
}
void set_a2dp_conn_state(void)
{
    g_conn_st.profile |= PROFILE_BT_A2DP_SNK;
}
void set_avrcp_conn_state(void)
{
    g_conn_st.profile |= PROFILE_BT_AVRCP;
}
void set_hfp_conn_state(void)
{
	extern uint32_t app_env_check_bt_para_flag(uint32_t flag);
	if (app_env_check_bt_para_flag(APP_ENV_BT_FLAG_HFP))
	{
		g_conn_st.profile |= PROFILE_BT_HFP;
	}
}

t_conn_result get_connection_state()
{
     return g_conn_st.result;
}

void set_connection_state(btaddr_t* addr, uint32_t state)
{
    if(btaddr_same(addr,g_conn_st.addr))
        g_conn_st.state = state;
}
void clr_g_auth_st_auth_status(void)//bugfix : delete link key,page back unlimit,do not open scan
{
	g_auth_st.auth_status = 0;
}
btaddr_t* get_auth_fail_addr(void)//bugfix : delete link key,page back unlimit,do not open scan
{
    if(g_auth_st.auth_status != 0)
    {
	 	return(&(g_auth_st.r_addr));
    }
	else
	{
		return NULL;
	}
}

void set_connection_event(btaddr_t* addr, t_conn_event event)
{
    if(btaddr_same(addr,g_conn_st.addr))
    {
        g_conn_st.event = event;
    }
    else
    {
        LOG_D(CONN,"st:"BTADDR_FORMAT"!="BTADDR_FORMAT"\r\n",BTADDR(g_conn_st.addr),BTADDR(addr));
    }

    if((btaddr_same(addr,&(g_auth_st.r_addr))) && (event == CONN_EVENT_ACL_FAIL))
    {
        LOG_I(CONN,"CONN_EVENT_ACL_FAIL==>CONN_EVENT_ACL_WITH_AUTH_FAIL!!!\r\n");
    }	
}

BOOL TickTimer_Is_Expired(uint64_t ptimer)
{
    BOOL expr;

    if (ptimer <= os_get_tick_counter())
    {
        expr = TRUE;
    }
    else
    {
        expr = FALSE;
    }

    return expr;
}

void connection_event_to_state(uint32_t event)
{

    switch(event)
    {
        case CONN_EVENT_NULL:
            break;
        case CONN_EVENT_ACL_CONNECTED:
            if(g_conn_st.state == CONN_WAIT_ACL_CONNECTED)
                g_conn_st.state = CONN_ACL_CONNECTED;
            break;
        case CONN_EVENT_ACL_FAIL:
            #if 1
            LOG_I(APP,"ACL_FAIL:%x\r\n",g_conn_st.state);
            g_conn_st.state = CONN_ACL_FAIL;
            #else
            if(g_conn_st.state == CONN_WAIT_ACL_CONNECTED)
            {
            	LOG_I(APP,"CONN_EVENT_ACL_FAIL:CONN_ACL_FAIL1\r\n");
                g_conn_st.state = CONN_ACL_FAIL;
            }
            if(g_conn_st.state == CONN_WAIT_DISCONNECTED)
            {
            	LOG_I(APP,"CONN_EVENT_ACL_FAIL:CONN_ACL_FAIL2\r\n");
                g_conn_st.state = CONN_ACL_FAIL;
            }
            #endif
            break;
        case CONN_EVENT_AUTH_SUCCESS: 
            if(g_conn_st.state == CONN_WAIT_AUTH)
                g_conn_st.state = CONN_AUTH_SUCCESS;
            break;  
        case CONN_EVENT_AUTH_FAIL:
            if(g_conn_st.state == CONN_WAIT_AUTH)
                g_conn_st.state = CONN_AUTH_FAIL;
            break;  
        case  CONN_EVENT_ENCYPTED:  
            if(g_conn_st.state == CONN_WAIT_ENCYPTED)
                g_conn_st.state = CONN_ENCYPTED;
            break;
        case CONN_EVENT_ENCYPT_FAIL:
            if(g_conn_st.state == CONN_WAIT_ENCYPTED)
                g_conn_st.state = CONN_ENCYPT_FAIL;
            break;
        case CONN_EVENT_SDP_FINISHED:
            if(g_conn_st.state == CONN_WAIT_SDP_FINISHED)
                g_conn_st.state = CONN_SDP_FINISHED;
            break;
        case  CONN_EVENT_SDP_FAIL:  
            if(g_conn_st.state == CONN_WAIT_SDP_FINISHED)
                g_conn_st.state = CONN_SDP_FAIL;
            break;
        case  CONN_EVENT_HFP_CONNECTED:  
            if(g_conn_st.state == CONN_WAIT_HFP_CONNECTED)
                g_conn_st.state = CONN_HFP_CONNECTED;
            break;
        case CONN_EVENT_HFP_DISCONNECTED:
            if(g_conn_st.state == CONN_WAIT_HFP_CONNECTED)
                g_conn_st.state = CONN_HFP_FAIL;
            break;
        case CONN_EVENT_SNK_A2DP_CONNECTED :
            if(g_conn_st.state == CONN_WAIT_SNK_A2DP_CONNECTED)
                g_conn_st.state = CONN_SNK_A2DP_CONNECTED;
            break;
        case  CONN_EVENT_SNK_A2DP_DISCONNECTED:  
            if(g_conn_st.state == CONN_WAIT_SNK_A2DP_CONNECTED)
                g_conn_st.state = CONN_SNK_A2DP_FAIL;
            break;
        case CONN_EVENT_AVRCP_CONNECTED:
            if(g_conn_st.state == CONN_WAIT_AVRCP_CONNECTED)
                g_conn_st.state = CONN_AVRCP_CONNECTED;
            break;
        case CONN_EVENT_AVRCP_DISCONNECTED:
            if(g_conn_st.state == CONN_WAIT_AVRCP_CONNECTED)
                g_conn_st.state = CONN_AVRCP_FAIL;
            break;
        case  CONN_EVENT_HID_CONNECTED:  
            if(g_conn_st.state == CONN_WAIT_HID_CONNECTED)
                g_conn_st.state = CONN_HID_CONNECTED;
            break;
        case CONN_EVENT_HID_DISCONNECTED:
            if(g_conn_st.state == CONN_WAIT_HID_CONNECTED)
                g_conn_st.state = CONN_HID_FAIL;
            break;
        default:
            break;
    }
}

result_t bt_hf_connect(btaddr_t* addr)
{
    char cmd[40];
    app_handle_t sys_hdl = app_get_sys_handler();
    
    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT " %u",0, addr->b[5],
    addr->b[4],addr->b[3],
    addr->b[2], addr->b[1],
    addr->b[0], sys_hdl->hfp_rfcomm_channel);

    return hf_cmd_connect(cmd, sizeof(cmd));
}

result_t bt_a2dpSnk_connect(btaddr_t* addr)
{
    char cmd[40];
    
    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT,0, addr->b[5],
    addr->b[4],addr->b[3],
    addr->b[2], addr->b[1],
    addr->b[0]);

    return a2dp_cmd_connect(cmd, sizeof(cmd));
}

result_t bt_avrcp_connect(btaddr_t* addr)
{
    char cmd[40];
    
    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT,0, addr->b[5],
    addr->b[4],addr->b[3],
    addr->b[2], addr->b[1],
    addr->b[0]);

    return avrcp_cmd_connect(cmd, sizeof(cmd));
}


void bt_connect_scheduler()
{
    static uint16_t inner_state_last=0;
    static uint64_t conn_timer = 0;
    static uint8_t conn_count = 0;
    static t_conn_result s_result = CONN_FALSE;
    t_conn_inner_state inner_state;     
    uint32_t err = 0;
    btaddr_t* addr = NULL;
    app_handle_t sys_hdl = app_get_sys_handler();
    
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN)||app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))
        return;
    
    if(!g_conn_st.conn_active)
        return;    

    addr = g_conn_st.addr;

    connection_event_to_state(g_conn_st.event);
    
    inner_state = g_conn_st.state;  
    if(inner_state_last!=inner_state)
    {
        LOG_I(CONN,"inner_state=%x\r\n",inner_state);
    }
    inner_state_last=inner_state;
    
    switch(inner_state)
    {
        case CONN_START:
            
            g_conn_st.link = hci_link_lookup_btaddr(sys_hdl->unit, addr, HCI_LINK_ACL);
            if(!g_conn_st.link)
            {
                //LOG_D(CONN,"A-conn time:%d,%d, link_connect: "BTADDR_FORMAT"\r\n", conn_count,MAX_INNER_CONN_COUNT,BTADDR(addr));
                g_conn_st.conn_cancel_times = 0;
                g_conn_st.conn_cancel_success = -1;
                err = bt_unit_acl_connect(sys_hdl->unit, addr);                  
                if(!err)
                {
                    s_result = CONN_CONNECTING;                
                    conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();  // >5.12, 6s
                    inner_state = CONN_WAIT_ACL_CONNECTED;
                    LOG_I(CONN,"paging....\r\n");
                }
                else if(conn_count > MAX_INNER_CONN_COUNT) 
                {
                    inner_state = CONN_END;
                    s_result = CONN_FAIL;
                    LOG_I(CONN,"connect fail upto maxtime\r\n");
                }
                else
                {
                    LOG_I(CONN,"bt_unit_acl_connect err:%d\r\n",err);
                    conn_count++;
                }
            }
            else
            {
                //extern uint8_t page_debug_status(void);
                //page_debug_status();
                if((g_conn_st.conn_cancel_times==0) || (g_conn_st.conn_cancel_success>0))  // avoid sending cancel many times
                {
                    g_conn_st.conn_cancel_success = -1;
                    unit_send_cmd(sys_hdl->unit,HCI_CMD_CREATE_CON_CANCEL,addr,BTADDR_LEN);
                    g_conn_st.conn_cancel_times++;
                    LOG_I(CONN,"conn_start,link state:%d,cancel connection\r\n",g_conn_st.link->hl_state);
                }
            }
            break;
        case CONN_WAIT_ACL_CONNECTED:
            if(TickTimer_Is_Expired(conn_timer))
            {                
                g_conn_st.link = hci_link_lookup_btaddr(sys_hdl->unit, addr, HCI_LINK_ACL);  
                LOG_D(CONN,"Connecting is timeout at ACL:%d\r\n", g_conn_st.link);
                if(g_conn_st.link)
                {
                    if((g_conn_st.conn_cancel_times==0) || (g_conn_st.conn_cancel_success>0))  // avoid sending cancel many times
                    {
                        g_conn_st.conn_timeout_times++;
                        if(g_conn_st.conn_timeout_times>3)
                        {
                            extern void LSLCirq_R2P_Turn_On_Intr(void);
                            LSLCirq_R2P_Turn_On_Intr();
                            LOG_I(CONN,"reopen int\r\n");
                        }
                        g_conn_st.conn_cancel_times++;
                        g_conn_st.conn_cancel_success = -1;
                        unit_send_cmd(sys_hdl->unit,HCI_CMD_CREATE_CON_CANCEL,addr,BTADDR_LEN);  // after 5.3S have acl no connection_setup, cancel connection, free link
                        LOG_W(CONN,"link state:%d,cancel connection\r\n",g_conn_st.link->hl_state);
                    }              
                }
                else
                {
                    inner_state = CONN_END;
                    s_result = CONN_FAIL;   
                    extern __INLINE__ void HW_set_XVR_SW_reset(const uint32_t value);
                    HW_set_XVR_SW_reset(1);
                    g_conn_st.conn_timeout_times=0;
                    //debug_gpio_trigger();
                    // LOG_I(CONN,"REG_XVR_0x2A=%x,%x\r\n",REG_XVR_0x2A,ip_deepslcntl_get());
                    LOG_I(CONN,"REG_XVR_0x2A=%x\r\n",REG_XVR_0x2A);//++by Borg, reboot if call&print ip_deepslcntl_get(), why?
                }
            }
            break;
        case CONN_ACL_CONNECTED:
            g_conn_st.conn_timeout_times=0;
            conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
            g_conn_st.link = hci_link_lookup_btaddr(sys_hdl->unit, addr, HCI_LINK_ACL);
            if(!g_conn_st.link)
            {
                inner_state = CONN_END;
                s_result = CONN_FAIL;
            }
            else
            {
                err = hci_acl_send_auth_req( g_conn_st.link,HCI_LINK_AUTH_REQ);
                if(!err)
                {
                    inner_state = CONN_WAIT_AUTH;
                    conn_count = 1;
                }
            }
            break;
        case CONN_WAIT_AUTH:{
            if(TickTimer_Is_Expired(conn_timer))
            {                
                //LOG_I(CONN,"Connecting is timeout at auth\r\n");
                LOG_I(CONN,"CONN_WAIT_AUTH :Connecting is timeout at auth\r\n");
                bt_app_disconnect_connect_acl(addr);
                inner_state = CONN_WAIT_DISCONNECTED;
            }
            break;
        }
        case CONN_AUTH_SUCCESS:{
            conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
            err = hci_acl_send_auth_req( g_conn_st.link,HCI_LINK_ENCRYPT_REQ);
            if(!err)
            {
                inner_state = CONN_WAIT_ENCYPTED;
                conn_count = 1;
            }
            break;
        }
        case CONN_WAIT_ENCYPTED:{
            if(TickTimer_Is_Expired(conn_timer))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at encypt\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state =  CONN_WAIT_DISCONNECTED;
                }
            }
            break;
        }
        case CONN_ENCYPTED: {              
            app_env_handle_t env_h = app_env_get_handle();
            conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
            LOG_D(CONN,"encryted^^^sdp_connect\r\n");
            sdp_connect(&env_h->env_cfg.bt_para.device_addr,addr);
            conn_count = 1;
            inner_state = CONN_WAIT_SDP_FINISHED;
            break;
        }
        case CONN_WAIT_SDP_FINISHED:{
            if(TickTimer_Is_Expired(conn_timer))
            {                
                {
                    LOG_D(CONN,"Connecting is timeout at SDP\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
             }
            break;
        }
        case CONN_SDP_FINISHED:{
            int8_t id = bt_app_entity_find_id_from_raddr((btaddr_t *)addr);
            conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
            if((g_conn_st.profile & PROFILE_BT_HFP)&&(2!=bt_app_entity_get_mac_win_book(id)))
            {
                if(!hfp_has_the_connection(*addr))
                {
                    LOG_D(CONN,"^^^bt_hf_connect\r\n");
                    bt_hf_connect(addr);
                    inner_state = CONN_WAIT_HFP_CONNECTED;
                    conn_count = 1;
                }
                else
                {
                    inner_state = CONN_HFP_CONNECTED;
                    g_conn_st.profile &= ~PROFILE_BT_HFP;
                }
            }
            else if(g_conn_st.profile & PROFILE_BT_A2DP_SNK)
            {
                g_conn_st.profile &= ~PROFILE_BT_HFP;
                if(!a2dp_has_the_connection(addr))
                {
                    LOG_D(CONN,"^^^bt_a2dpSnk_connect\r\n");
                    bt_a2dpSnk_connect(addr);
                    inner_state = CONN_WAIT_SNK_A2DP_CONNECTED;
                    conn_count = 1;
                }
                else
                {
                    inner_state = CONN_SNK_A2DP_CONNECTED;
                    g_conn_st.profile &= ~PROFILE_BT_A2DP_SNK;
                }
            }
            /*
            else if ()
            */
            conn_count = 1;
            break;
        }
        case CONN_WAIT_HFP_CONNECTED:{
            if(hfp_has_the_connection(*addr))
            {
                inner_state = CONN_HFP_CONNECTED;
            }
            else if(TickTimer_Is_Expired(conn_timer))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at HFP\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
                break;
            }
            else
                break;            
        }
        case CONN_HFP_CONNECTED:{            
            g_conn_st.profile &= ~PROFILE_BT_HFP;
            if(!g_conn_st.profile)
            {
                inner_state = CONN_END;
                g_conn_st.result = CONN_SUCCESS;
            }      
            else if(g_conn_st.profile & PROFILE_BT_A2DP_SNK)
            {
                if(!a2dp_has_the_connection(addr))
                {                    
                    LOG_D(CONN,"^^^bt_a2dpSnk_connect\r\n");
                    conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
                    bt_a2dpSnk_connect(addr);
                    inner_state = CONN_WAIT_SNK_A2DP_CONNECTED;
                    conn_count = 1;
                }
                else
                {
                    inner_state = CONN_SNK_A2DP_CONNECTED;
                    g_conn_st.profile &= ~PROFILE_BT_A2DP_SNK;
                }
            }
            /*
            else if(g_conn_st.profile & PROFILE_A2DP_SRC)
            ......*/
                   
            break;
        }
        case CONN_WAIT_SNK_A2DP_CONNECTED:{
            if(a2dp_has_the_connection(addr))
            {
                inner_state = CONN_SNK_A2DP_CONNECTED;
            }
            else if(TickTimer_Is_Expired(conn_timer))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at Sink A2DP\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
                break;
            }
            else
                break;
        }
        case CONN_SNK_A2DP_CONNECTED:{
            g_conn_st.profile &= ~PROFILE_BT_A2DP_SNK;
            if(g_conn_st.profile & PROFILE_BT_AVRCP)
            {
                if(!avrcp_has_the_connection(*addr))
                {
                    LOG_D(CONN,"^^^bt_avrcp_connect\r\n");
                    conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
                    bt_avrcp_connect(addr);
                    inner_state = CONN_WAIT_AVRCP_CONNECTED;
                    conn_count = 1;
                }
                else
                {
                    inner_state = CONN_AVRCP_CONNECTED;
                    g_conn_st.profile &= ~PROFILE_BT_AVRCP;                    
                }
            }
            else if(!g_conn_st.profile)
            {
                inner_state = CONN_END;
                s_result = CONN_SUCCESS;
            }   /*   
            else if()
                */
            break;
        }
        case CONN_WAIT_AVRCP_CONNECTED:{
            if(avrcp_has_the_connection(*addr))
            {
                inner_state = CONN_AVRCP_CONNECTED;
            }
            else if(TickTimer_Is_Expired(conn_timer))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at Sink AVRCP\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
                break;
            }
            else
                break;
        }
        case CONN_AVRCP_CONNECTED:{
            g_conn_st.profile &= ~PROFILE_BT_AVRCP;
            if(!g_conn_st.profile)
            {
                inner_state = CONN_END;
                s_result = CONN_SUCCESS;
            }   /*   
            else if()
                */
            break;
        }
        case CONN_WAIT_DISCONNECTED:
            if(!hci_check_acl_link(addr))
            {
            	LOG_I(CONN,"CONN_WAIT_DISCONNECTED=CONN_FAIL:addr:"BTADDR_FORMAT"\r\n",BTADDR(addr));
                inner_state = CONN_END;
                s_result = CONN_FAIL;
            }
            break;
        case CONN_ACL_FAIL:// FAIL PUT END
            inner_state =  CONN_END;
            s_result = CONN_FAIL;
            break;
        case CONN_KEY_MISSING:
                bt_app_disconnect_connect_acl(addr);
                inner_state =  CONN_WAIT_DISCONNECTED;
                LOG_I(CONN,"CONN_KEY_MISSING:Connecting is timeout at auth:addr\r\n");
                g_auth_st.auth_status = 1;//charles mean fail 
                memcpy(&g_auth_st.r_addr.b[0],&addr->b[0],6);
        	break;
        case CONN_LOW_RESOURCES:
                bt_app_disconnect_connect_acl(addr);
                inner_state =  CONN_WAIT_DISCONNECTED;
                LOG_I(CONN,"CONN_LOW_RESOURCES\r\n");
                g_auth_st.auth_status = 1;
			
        	break;    
        case CONN_AUTH_FAIL:{            
            if(TickTimer_Is_Expired(conn_timer) || (conn_count >= MAX_INNER_CONN_COUNT))
            { 		
                g_auth_st.auth_status = 1;//charles mean fail 
                memcpy(&g_auth_st.r_addr.b[0],&addr->b[0],6);
                LOG_I(CONN,"CONN_AUTH_FAIL:Connecting is timeout at auth:addr:"BTADDR_FORMAT"\r\n",BTADDR(&g_auth_st.r_addr));
                bt_app_disconnect_connect_acl(addr);
                inner_state =  CONN_WAIT_DISCONNECTED;               
             }
            else if(conn_count < MAX_INNER_CONN_COUNT)
            {
                LOG_I(CONN,"^^^Try Auth again,%d\r\n",conn_count);
                err = hci_acl_send_auth_req( g_conn_st.link,HCI_LINK_AUTH_REQ);
                g_conn_st.event = CONN_EVENT_NULL;
                conn_count++;
                inner_state = CONN_WAIT_AUTH;
            }
            break;
        }
        case CONN_ENCYPT_FAIL:
        {            
            if(TickTimer_Is_Expired(conn_timer) || (conn_count >= MAX_INNER_CONN_COUNT))
            { 
                LOG_I(CONN,"Connecting is timeout at encypt\r\n");
                bt_app_disconnect_connect_acl(addr);
                inner_state =  CONN_WAIT_DISCONNECTED;               
            }
            else if(conn_count < MAX_INNER_CONN_COUNT)
            {
                LOG_I(CONN,"^^^Try Encrypt again,%d\r\n",conn_count);
                err = hci_acl_send_auth_req( g_conn_st.link,HCI_LINK_ENCRYPT_REQ);
                g_conn_st.event = CONN_EVENT_NULL;
                conn_count++;
                inner_state = CONN_WAIT_ENCYPTED;
            }
            break;
        }
        case CONN_SDP_FAIL:{
            if(TickTimer_Is_Expired(conn_timer) || (conn_count >= MAX_INNER_CONN_COUNT))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at SDP\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
            }
            else if(conn_count < MAX_INNER_CONN_COUNT)
            {
                LOG_I(CONN,"^^^Try SDP again,%d\r\n",conn_count);
                app_env_handle_t env_h = app_env_get_handle();
                err = sdp_connect(&env_h->env_cfg.bt_para.device_addr,addr);
                g_conn_st.event = CONN_EVENT_NULL;
                conn_count++;
                inner_state = CONN_WAIT_SDP_FINISHED;
            }
            break;
        }
        case CONN_HFP_FAIL:{
            if(hfp_has_the_connection(*addr))
            {
                LOG_E(CONN,"Error connect state for hfp connection!\r\n");
                inner_state = CONN_HFP_CONNECTED;
            }
            else if((TickTimer_Is_Expired(conn_timer) || (conn_count >= MAX_INNER_CONN_COUNT)) )
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at hfp\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
            }
            else if(conn_count < MAX_INNER_CONN_COUNT)
            {
                LOG_I(CONN,"^^^Try hf connect again,%d\r\n",conn_count);
                err = bt_hf_connect(addr);
                g_conn_st.event = CONN_EVENT_NULL;
                inner_state = CONN_WAIT_HFP_CONNECTED;
                conn_count++;
            }
            break;
        }
        case CONN_SNK_A2DP_FAIL:{
            if(a2dp_has_the_connection(addr))
            {
                LOG_E(CONN,"Error connect state for a2dp connection!\r\n");
                inner_state = CONN_SNK_A2DP_CONNECTED;
            }
            else if((TickTimer_Is_Expired(conn_timer) || (conn_count >= MAX_INNER_CONN_COUNT)))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at a2dp\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
            }
            else if(conn_count < MAX_INNER_CONN_COUNT)
            {
                LOG_I(CONN,"^^^Try a2dp connect again,%d\r\n",conn_count);
                err = bt_a2dpSnk_connect(addr);
                g_conn_st.event = CONN_EVENT_NULL;
                inner_state = CONN_WAIT_SNK_A2DP_CONNECTED;
                conn_count++;
            }
            break;
        }  
        case CONN_AVRCP_FAIL:{
            if(avrcp_has_the_connection(*addr))
            {
                LOG_E(CONN,"Error connect state for avrcp connection!\r\n");
                inner_state = CONN_SNK_A2DP_CONNECTED;
            }
            else if((TickTimer_Is_Expired(conn_timer) || (conn_count >= MAX_INNER_CONN_COUNT)))
            {                
                {
                    LOG_I(CONN,"Connecting is timeout at a2dp\r\n");
                    bt_app_disconnect_connect_acl(addr);
                    inner_state = CONN_WAIT_DISCONNECTED;
                }
            }
            else if(conn_count < MAX_INNER_CONN_COUNT)
            {
                ////bug: when authenticating, received avrcp_svc_disconnected  ???
                if(g_conn_st.profile & PROFILE_BT_HFP)
                {
                    conn_timer = MAX_INNER_CONN_TIME + os_get_tick_counter();
                    if(!hfp_has_the_connection(*addr))
                    {
                        LOG_D(CONN,"^^^bt_hf_connect\r\n");
                        bt_hf_connect(addr);
                        inner_state = CONN_WAIT_HFP_CONNECTED;
                        conn_count = 1;
                    }
                    else
                    {
                        inner_state = CONN_HFP_CONNECTED;
                        g_conn_st.profile &= ~PROFILE_BT_HFP;
                    }
                }
                else if(g_conn_st.profile & PROFILE_BT_A2DP_SNK)
                {
                    if(!a2dp_has_the_connection(addr))
                    {
                        LOG_D(CONN,"^^^bt_a2dpSnk_connect\r\n");
                        bt_a2dpSnk_connect(addr);
                        inner_state = CONN_WAIT_SNK_A2DP_CONNECTED;
                        conn_count = 1;
                    }
                    else
                    {
                        inner_state = CONN_SNK_A2DP_CONNECTED;
                        g_conn_st.profile &= ~PROFILE_BT_A2DP_SNK;
                    }
                }
                else
                {
                    LOG_I(CONN,"^^^Try avrcp connect again,%d\r\n",conn_count);
                    err = bt_avrcp_connect(addr);
                    inner_state = CONN_WAIT_AVRCP_CONNECTED;
                    conn_count++;
                }
            }
            break;
        }  
        case CONN_END:{
            g_conn_st.result = s_result;
            g_conn_st.link = NULL;
            g_conn_st.conn_active = FALSE;
            app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
            
            #if (REMOTE_RANGE_PROMPT == 1)
            sys_hdl->flag_soft_reset = 0;
            ///app_bt_flag2_set(APP_FLAG2_NO_OS_PRINTF,0);
            #endif
            if(!g_conn_st.profile)
                LOG_D(CONN,"^^^connection is finished\r\n");
            break;
        }
        default:
            LOG_E(CONN,"^^^error connection state\r\n");
            break;
    }

    if(err)
    {
        if(inner_state > CONN_ACL_CONNECTED)
        {
            LOG_E(CONN,"^^^Has error at state:0x%x,err:%d\r\n",inner_state,err);
//            bt_unit_acl_disconnect(sys_hdl->unit,addr);
//            inner_state =  CONN_WAIT_DISCONNECTED;
        }
    }

    g_conn_st.state = inner_state;
    
}


