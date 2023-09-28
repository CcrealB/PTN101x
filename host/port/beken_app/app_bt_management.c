/*
 * app_bt_management.c
 *
 *  Created on: 2017-8-18
 *      Author: Durian
 */
#include "lc_interface.h"
#include "app_beken_includes.h"
#include "app_bt_management.h"
#include "lmp_scan.h"

t_bt_app_entity g_bt_app_entity_container[2];

/* Function name:   bt_app_entity_alloc
 * Description:     allocate BT application entity;
 * Parameters:      none;
 * Return value:    BT application entity ID,
 *                  if ID >= BT_MAX_AG_COUNT,it means that container has allocated all;
 */
uint8_t bt_app_entity_alloc(void)
{
    uint8_t i=0;
    while((g_bt_app_entity_container[i].is_used == 1) && (i < (BT_MAX_AG_COUNT-1)))
    {
        i++;
    }
    if(g_bt_app_entity_container[i].is_used == 0)
    {
        g_bt_app_entity_container[i].is_used = 1;
        return i;
    }
    return i;
}

/* Function name:   bt_app_entity_attach
 * Description:     attach value to application entity,for example: link handle,remote BT address;
 * Parameters:
 *              id(in):     entity ID,
 *          handle(in):     link handle,
 *        rmt_addr(in):     remote BT address;    
 *
 * Return value:  none 
 */
void bt_app_entity_attach(uint8_t id,uint16_t handle,btaddr_t *rmt_addr,uint8_t detach_reason)
{
    app_env_handle_t env_h = app_env_get_handle();
    uint8_t index = 0;
    g_bt_app_entity_container[id].is_used = 1;
    g_bt_app_entity_container[id].link_handle = handle;
    g_bt_app_entity_container[id].detach_reason = detach_reason;
    g_bt_app_entity_container[id].role_switch_times = 0;
    if(rmt_addr != NULL)
        memcpy((uint8_t *)(&g_bt_app_entity_container[id].bt_remote_addr),(uint8_t *)rmt_addr,sizeof(btaddr_t));        
    
    index = app_env_key_stored(&g_bt_app_entity_container[id].bt_remote_addr);
    if(index != 0)
    {
#if CONFIG_RF_CALI_TYPE        
        if(app_get_best_offset_level()>1)
            env_h->env_data.key_pair[index-1].crystal_cali_data = app_get_best_offset();
        system_set_0x4d_reg(env_h->env_data.key_pair[index-1].crystal_cali_data);
#else
        system_set_0x4d_reg(env_h->env_data.key_pair[index-1].crystal_cali_data);
#endif
    }
}

/* Function name:   bt_app_entity_free
 * Description:     free BT application entity,and reset partial members of entity struct;
 * Parameters:  
 *              id(in):     entity ID,    
 *   detach_reason(in):     detach reason of BT acl link ;
 * Return value:  none  
 */
void bt_app_entity_free(uint8_t id,uint8_t detach_reason)
{
    g_bt_app_entity_container[id].is_used = 0;
    g_bt_app_entity_container[id].link_handle = 0;
    g_bt_app_entity_container[id].bt_connecting_flag = 0;
    g_bt_app_entity_container[id].bt_working_flag = 0;
    g_bt_app_entity_container[id].detach_reason = detach_reason;
    g_bt_app_entity_container[id].role_switch_times = 0;
}

/* Function name:   bt_app_entity_init
 * Description:     BT application entities initialise; 
 * Parameters:      none;
 * Return value:    none;   
 */
void bt_app_entity_init(void)
{
    uint8_t i;
    for(i=0;i<BT_MAX_AG_COUNT;i++)
        memset(&g_bt_app_entity_container[i],0,sizeof(t_bt_app_entity));
}

/* Function name:   bt_app_entity_print_state
 * Description:     for debug bt_app_entity state machine;
 * Parameters:      none;
 * Return value:    noe;
 */
void bt_app_entity_print_state(void)
{
    uint8_t i;
    os_printf("|----------------------------------------------\r\n");
    for(i=0;i<BT_MAX_AG_COUNT;i++)
    {
        os_printf("|       is_used-%d:%d\r\n",i,g_bt_app_entity_container[i].is_used);
        os_printf("|   link_handle-%d:%04x\r\n",i,g_bt_app_entity_container[i].link_handle);
        os_printf("|   remote_addr-%d:" BTADDR_FORMAT "\r\n",i,BTADDR(&g_bt_app_entity_container[i].bt_remote_addr));
        os_printf("|         state-%d:%d\r\n",i,g_bt_app_entity_container[i].sys_bt_state);
        os_printf("|         role-%d:%d\r\n",i,g_bt_app_entity_container[i].bt_role);
        os_printf("|         event-%d:%d\r\n",i,g_bt_app_entity_container[i].sys_bt_event);
        os_printf("|     conn_flag-%d:%8x\r\n",i,g_bt_app_entity_container[i].bt_connecting_flag);
        os_printf("|     work_flag-%d:%8x\r\n",i,g_bt_app_entity_container[i].bt_working_flag);
        os_printf("| detach_reason-%d:%8x\r\n",i,g_bt_app_entity_container[i].detach_reason);
		os_printf("| mac-win-%d:%8x\r\n",i,g_bt_app_entity_container[i].mac_win_book);
        os_printf("|----------------------------------------------\r\n");
    }

}

/* Function name:   bt_app_entity_get_role_switch_times_from_raddr
 * Description:     get role_switch_times from remote BT addr;   
 * Parameters:      
 *            raddr(in):   remote BT addr;
 * Return value:    role_switch_times;  
 */
uint8_t bt_app_entity_get_role_switch_times_from_raddr(btaddr_t *raddr)
{
    uint8_t idx = 0;
    for(; idx<BT_MAX_AG_COUNT; idx++)
    {
        if(btaddr_same(raddr,&g_bt_app_entity_container[idx].bt_remote_addr))
        {
            return g_bt_app_entity_container[idx].role_switch_times;
        }
    }
    return 0;
}
/* Function name:   bt_app_entity_set_role_switch_times_from_raddr
 * Description:     set role_switch_times from remote BT addr;   
 * Parameters:      
 *            raddr(in):   remote BT addr;
 *            times(in):   times
 * Return value:    role_switch_times;  
 */
void bt_app_entity_set_role_switch_times_from_raddr(btaddr_t *raddr,uint8_t times)
{
    uint8_t idx = 0;
    for(; idx<BT_MAX_AG_COUNT; idx++)
    {
        if(btaddr_same(raddr,&g_bt_app_entity_container[idx].bt_remote_addr))
        {
            g_bt_app_entity_container[idx].role_switch_times = times;
        }
    }
}

/* Function name:   bt_app_entity_find_handle_from_raddr
 * Description:     get HCI link handle from remote BT addr;   
 * Parameters:      
 *            rmt_addr(in):   remote BT addr;
 * Return value:    HCI link handle;if zero,means HCI link is null;  
 */
uint16_t bt_app_entity_find_handle_from_raddr(btaddr_t *rmt_addr)
{
    uint8_t idx = 0;
    for(; idx<BT_MAX_AG_COUNT; idx++)
    {
        if(btaddr_same(rmt_addr,&g_bt_app_entity_container[idx].bt_remote_addr))
        {
            return g_bt_app_entity_container[idx].link_handle;
        }
    }
    return 0;
}

/* Function name:  bt_app_entity_find_id_from_raddr
 * Description:    get application entity ID from remote BT addr;  
 * Parameters:  
 *      rmt_addr(in):   remote BT addr;
 * Return value:  application entity ID,if ID not find,return MNG_ERROR_NO_ENTITY;
 */
int8_t bt_app_entity_find_id_from_raddr(btaddr_t *rmt_addr)
{
    uint8_t idx = 0;
    for(; idx<BT_MAX_AG_COUNT; idx++)
    {
        if(btaddr_same(rmt_addr,&g_bt_app_entity_container[idx].bt_remote_addr))
        {
            return idx;
        }
    }
    return MNG_ERROR_NO_ENTITY;
}

/* Function name:  bt_app_entity_find_id_from_handle
 * Description:    get application entity ID from HCI link handle;  
 * Parameters:  
 *      handle(in):   HCI link handle;
 * Return value:  application entity ID,if ID not find,return MNG_ERROR_NO_ENTITY;
 */
int8_t bt_app_entity_find_id_from_handle(uint16_t handle)
{
    uint8_t idx = 0;
    for(; idx<BT_MAX_AG_COUNT; idx++)
    {
        if(g_bt_app_entity_container[idx].link_handle == handle)
        {
            return idx;
        }
    }
    return MNG_ERROR_NO_ENTITY;
}

/* Function name:   bt_app_get_conn_addr_from_env
 * Description:     get auto connect BT addrs from Flash;
 * Parameters:      none;
 * Return value:    none;   
 */
void bt_app_get_conn_addr_from_env(void)
{
    uint8_t entity_id,i,env_store_num;
    btaddr_t *rmt_addr;
    extern void app_print_linkkey(void);
    env_store_num = app_get_env_key_num();
    LOG_I(MNG,"get_conn_addr,key:%d\r\n",env_store_num);
    app_print_linkkey();
    if(env_store_num > 0)
    {
        for(i=0; i<env_store_num;i++)
        {
            rmt_addr = app_get_active_remote_addr(i);
            if(rmt_addr != NULL)
            {    
                entity_id = bt_app_entity_alloc(); 
                if(entity_id < BT_MAX_AG_COUNT)
                {
                    bt_app_entity_attach(entity_id,0,rmt_addr,0);
                    if(app_get_best_offset_level()>1)
	                app_set_crystal_calibration(0);
                }
            }
        }
    }
}

/* Function name:  bt_app_entity_bind
 * Description:    bind link handle with remote BT addr for entity ID;
 * Parameters:
 *       handle(in):   HCI link handle,
 *     rmt_addr(in):   remote BT addr;
 * Return value:    none;  
 */
void bt_app_entity_bind(uint16_t handle,btaddr_t *rmt_addr)
{
    int8_t entity_id;
    entity_id = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(entity_id >= 0)
    {
        bt_app_entity_attach(entity_id,handle,NULL,0);
    }
    else
    {
        entity_id = bt_app_entity_alloc(); 
        if(entity_id < BT_MAX_AG_COUNT)
        {
            bt_app_entity_attach(entity_id,handle,rmt_addr,0);
        }
    }
}

/* Function name:  bt_app_entity_free_by_addr
 * Description:    when acl disconnected,free entity by remote BT addr; 
 * Parameters:
 *      rmt_addr(in):   remote BT addr,
 * detach_reason(in):   detach reason of BT acl link;
 * Return value:   
 */
void bt_app_entity_free_by_addr(btaddr_t *rmt_addr,uint8_t detach_reason)
{
    int8_t idx = 0;
    btaddr_t* get_auth_fail_addr(void);//bugfix : delete link key,page back unlimit,do not open scan
    void clr_g_auth_st_auth_status(void);//bugfix : delete link key,page back unlimit,do not open scan
    btaddr_t* auth_fail_addr;//bugfix : delete link key,page back unlimit,do not open scan
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(get_auth_fail_addr()!=NULL)//bugfix : delete link key,page back unlimit,do not open scan
    {
        auth_fail_addr = get_auth_fail_addr();
        if(btaddr_same(rmt_addr,auth_fail_addr))
        {
            LOG_I(MNG,"!!!auth_fail in bt_app_entity_free_by_addr\r\n!!!");
            if(idx >= 0)
            {
                clr_g_auth_st_auth_status();
                bt_app_entity_free(idx,detach_reason);  
                bt_app_entity_set_event(idx, SYS_BT_DISCONNECTED_EVENT);
                return;
            }
        }
    	
    }
    if(idx >= 0)
    {
        /*   
         *   1. reconnect request when detach reason = 0x08,HCI_ERR_CONNECTION_TIMEOUT;
         *   2. reconenct request when detach reason = 0x22,HCI_ERR_LMP_RESPONSE_TIMEOUT;
         *   3. when bt connections is scatternet,disconnect this link and reconnect again,SYS_BT_DISCONN_CONN_EVENT;
         */
        t_sys_bt_event bt_event;
        bt_event = bt_app_entity_get_event(idx);
        LOG_I(MNG,"==entity free:%d,event:%d,detach_reason:0x%x,reconn:%x\r\n",idx,bt_event,detach_reason,bt_app_entity_get_connect_flag (idx,BT_CONNECT_STATE_RECONNECTING));
        if((detach_reason == HCI_ERR_CONNECTION_TIMEOUT) 
            || (detach_reason == HCI_ERR_LMP_RESPONSE_TIMEOUT) 
            || (bt_event == SYS_BT_DISCONN_CONN_EVENT)
            || (bt_app_entity_get_connect_flag (idx,BT_CONNECT_STATE_RECONNECTING)))
        {
            bt_app_entity_attach(idx,0,rmt_addr,detach_reason);
            
            if((detach_reason == HCI_ERR_CONNECTION_TIMEOUT) || (detach_reason == HCI_ERR_LMP_RESPONSE_TIMEOUT) )
                bt_app_entity_set_connect_flag(idx,BT_CONNECT_STATE_RECONNECTING);
        }
        else if(bt_app_entity_get_state(idx) != SYS_BT_CONNECTING_STATE)
        {
            bt_app_entity_free(idx,detach_reason);  
        }
        
        //if(!bt_event)
            bt_app_entity_set_event(idx, SYS_BT_DISCONNECTED_EVENT);
    }
}

/* Function name:   bt_app_entity_set_event_by_addr
 * Description:     set events of application entity from remote BT addr;        
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *          event(in):   events;
 * Return value:  none 
 */
void bt_app_entity_set_event_by_addr(btaddr_t *rmt_addr,t_sys_bt_event event)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx >= 0)
        bt_app_entity_set_event(idx,event);
}

/* Function name:  bt_app_entity_set_event_by_handle
 * Description:     set events of application entity from link handle;        
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *          event(in):   events;
 * Return value:  none 
 */
void bt_app_entity_set_event_by_handle(uint16_t handle,t_sys_bt_event event)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_handle(handle);
    if(idx >= 0)
        bt_app_entity_set_event(idx,event);
}
/* Function name:   bt_app_entity_set_role_by_addr
 * Description:     set role of application entity from remote BT addr;        
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *          role(in):    role;
 * Return value:  none 
 */
void bt_app_entity_set_role_by_addr(btaddr_t *rmt_addr,t_bt_role role)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx >= 0)        
        bt_app_entity_set_role(idx,role);
}

/* Function name:  bt_app_entity_set_role_by_handle
 * Description:     set role of application entity from link handle;        
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *          event(in):   role;
 * Return value:  none 
 */
void bt_app_entity_set_role_by_handle(uint16_t handle,t_bt_role role)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_handle(handle);
    if(idx >= 0)
        bt_app_entity_set_role(idx,role);
}
/* Function name:  bt_app_entity_set_conn_flag_by_addr
 * Description:    set connection flags by remote addr;
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *          flag(in):   bt profile attributes @config.h, and profile connect flag @bt_app_management.h;
 * Return value:    none; 
 */
void bt_app_entity_set_conn_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx >= 0)
        bt_app_entity_set_connect_flag(idx,flag);
}

/* Function name:  bt_app_entity_clear_conn_flag_by_addr
 * Description:    clear connection flags by remote addr;
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *          flag(in):   bt profile attributes @config.h, and profile connect flag @bt_app_management.h;
 * Return value:    none; 
 */
void bt_app_entity_clear_conn_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx >= 0)
        bt_app_entity_clear_connect_flag(idx,flag);
}

/* Function name:  bt_app_entity_set_work_flag_by_addr
 * Description:    set working flags by remote addr;
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *           flag(in):    bt work flags @bt_app_management.h;
 * Return value:    none; 
 */
void bt_app_entity_set_work_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx >= 0)
        bt_app_entity_set_working_flag(idx,flag);
}
/* Function name:  bt_app_entity_clear_work_flag_by_addr
 * Description:    clear working flags by remote addr;
 * Parameters:
 *       rmt_addr(in):   remote BT addr,
 *           flag(in):    bt work flags @bt_app_management.h;
 * Return value:    none; 
 */

void bt_app_entity_clear_work_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx >= 0)
        bt_app_entity_clear_working_flag(idx,flag);   
}

/* Function name:  bt_app_check_all_entity_connect_flag
 * Description:    check flag of connection for all entities    
 * Parameters:
         flag(in):   bt profile attributes @config.h, and profile connect flag @bt_app_management.h;
 * Return value:   
 */
uint32_t bt_app_check_all_entity_connect_flag(uint32_t flag)
{
    uint8_t i;
    uint32_t ret = 0;
    for(i=0; i<BT_MAX_AG_COUNT; i++)
    {
        ret |= (g_bt_app_entity_container[i].bt_connecting_flag & flag);
    }
    return ret;
}

uint32_t bt_app_check_entity_connect_flag_by_id(int8_t idx,uint32_t flag)
{
    if(idx<BT_MAX_AG_COUNT)
        return (g_bt_app_entity_container[idx].bt_connecting_flag & flag);
        
    return 0;
}

/* Function name:  bt_app_check_all_entity_work_flag
 * Description:    check flag of working for all entities;
 * Parameters:
 *           flag(in):    bt work flags @bt_app_management.h;
 * Return value:    none; 
 */
uint32_t bt_app_check_all_entity_work_flag(uint32_t flag)
{
    uint8_t i;
    uint32_t ret = 0;
    for(i=0; i<BT_MAX_AG_COUNT; i++)
    {
        ret |= (g_bt_app_entity_container[i].bt_working_flag & flag);
    }
    return ret;
}

/* Function name:  bt_app_check_all_entity_connect_flag
 * Description:    clear flag of connection for all entities    
 * Parameters:
         flag(in):   bt profile attributes @config.h, and profile connect flag @bt_app_management.h;
 * Return value:   
 */
void bt_app_clear_all_entity_connect_flag(uint32_t flag)
{
    uint8_t i;
    for(i=0; i<BT_MAX_AG_COUNT; i++)
    {
        bt_app_entity_clear_connect_flag(i, flag);
    }
}

/* Function name:  bt_app_clear_all_entity_work_flag
 * Description:    clear check flag of working for all entities;
 * Parameters:
 *           flag(in):    bt work flags @bt_app_management.h;
 * Return value:    none; 
 */
void bt_app_clear_all_entity_work_flag(uint32_t flag)
{
    uint8_t i;
    for(i=0; i<BT_MAX_AG_COUNT; i++)
    {
        bt_app_entity_clear_working_flag(i, flag);
    }
}

/* Function name:   bt_app_device_scan_is_disallowed
 * Description:     BT device inquiry/page scan is disallowed ?  
 * Parameters:      none;
 * Return value:    if TRUE,scan is disallowed;
 */
BOOL bt_app_device_scan_is_disallowed(void)
{
    int cnt = 0;
    app_handle_t sys_hdl = app_get_sys_handler();
    cnt = hci_get_acl_link_count(sys_hdl->unit);
    if(cnt >= BT_MAX_AG_COUNT)
        return TRUE;
    else
        return FALSE;
}

/* Function name:  bt_app_set_device_scanning
 * Description:    set BT device inquiry/page scan  
 * Parameters:
        scan_enable(in):    0: disable scan,1:inquiry scan,2:page scan,3:both scan;
 * Return value:   none;
 */
void bt_app_set_device_scanning(uint8_t scan_enable)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    uint8_t scan_status = (uint8_t)LMscan_Read_Scan_Enable();
    
    LMscan_Clear_Page_Scan_Disallowed();
    //LOG_I(MNG,"change scan from %d===>>%d\r\n",scan_status,scan_enable);
    if(scan_enable != scan_status)
    {
        bt_unit_set_scan_enable(sys_hdl->unit, scan_enable);
        
        if(scan_enable == (HCI_BOTH_INQUIRY_PAGE_SCAN_ENABLE))
        {
            if (!app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION|APP_FLAG_ACL_CONNECTION) && app_bt_flag1_get(APP_FLAG_MATCH_ENABLE))
            {
    	        LOG_I(MNG,"Paring===>>\r\n");
//yuan++        app_set_led_event_action(LED_EVENT_MATCH_STATE);           // led: match
//yuan++        app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);   // wave: enter paring
        	}
        }
    }
}

/* Function name:   bt_app_disconnect_acl_link
 * Description:     if acl link exist,disconnect acl link;  
 * Parameters:      
 *          id(in):     entity ID;  
 * Return value:    none;
 */
void bt_app_disconnect_acl_link(uint8_t id)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    if(hci_check_acl_link(&g_bt_app_entity_container[id].bt_remote_addr))
    {
        bt_unit_acl_disconnect(sys_hdl->unit,&g_bt_app_entity_container[id].bt_remote_addr);
        bt_app_entity_set_connect_flag(id, BT_CONNECT_STATE_DISCONN_PENDING);
    }
    else
    {
        bt_app_entity_free(id,0);    
    }
}

/* Function name:   bt_app_disconnect_acl_link_by_raddr
 * Description:     if acl link exist,disconnect acl link;  
 * Parameters:      
 * rmt_addr(in):     addr;  
 * Return value:    none;
 */
void bt_app_disconnect_acl_link_by_raddr(btaddr_t *rmt_addr)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(idx < 0)
        return;
    if(hci_check_acl_link(&g_bt_app_entity_container[idx].bt_remote_addr))
    {
        bt_unit_acl_disconnect(sys_hdl->unit,&g_bt_app_entity_container[idx].bt_remote_addr);
        bt_app_entity_set_connect_flag(idx, BT_CONNECT_STATE_DISCONN_PENDING);
    }
    else
    {
        bt_app_entity_free(idx,0);    
    }
}

void bt_app_disconnect_connect_acl(btaddr_t *rmt_addr)
{
    int8_t idx = 0;
    idx = bt_app_entity_find_id_from_raddr(rmt_addr);
    if(!hci_check_acl_link(rmt_addr))
        return;
    if(idx < 0)
    {
        LOG_W(CONN,"Error in finding the addr when reconn/auto conn\r\n");    
        return;
    }
    bt_app_entity_set_event_by_addr(rmt_addr,SYS_BT_DISCONN_CONN_EVENT);
    app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
    bt_app_disconnect_acl_link(idx);
}

/* Function name:   bt_app_unused_procedure
 * Description:     init & no scan no connecting     
 * Parameters:
 *       id(in):     entity ID;  
 * Return value:   
 */
void bt_app_unused_procedure(uint8_t id)
{
    t_sys_bt_event bt_event = SYS_BT_INVALID_EVENT;

    bt_event = bt_app_entity_get_event(id);
    
    if(bt_event == SYS_BT_ENABLE_COMPLETE_EVENT)
    {
        bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
    }
    else if(bt_event == SYS_BT_BOTH_SCANNING_EVENT)
    {
        bt_app_entity_set_event(id, SYS_BT_BOTH_SCANNING_EVENT);
        bt_app_entity_set_state(id, SYS_BT_SCANNING_STATE);
    }
}

/* Function name:   bt_app_scanning_procedure
 * Description:     to be connected by mobile phone;    
 * Parameters:
 *       id(in):     entity ID;  
 * Return value:   
 */
void bt_app_scanning_procedure(uint8_t id)
{
    uint8_t scan_enable = 0;
    t_sys_bt_event bt_event;
	t_sys_bt_state  bt_state;
    app_handle_t sys_hdl = app_get_sys_handler();

    if(app_env_check_inquiry_always())
    {
        scan_enable = HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE;
    }
    else
    {
        scan_enable = HCI_PAGE_SCAN_ENABLE;
    }
    
    bt_event = bt_app_entity_get_event(id);
    
    if(bt_event == SYS_BT_CONNECTED_EVENT)
    {
        bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
        bt_app_clear_all_entity_connect_flag(BT_CONNECT_STATE_SCAN_PENDING);
        
        if(bt_app_device_scan_is_disallowed())
        {
            bt_app_set_device_scanning(HCI_NO_SCAN_ENABLE);
            bt_app_clear_all_entity_connect_flag(BT_CONNECT_STATE_SCAN_PENDING);
        }
        
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        return;
    }
    else if(bt_event ==  SYS_BT_DISCONN_CONN_EVENT)
    {
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        return;
    }
    else if(bt_event == SYS_BT_DUT_MODE_EVENT)
    {
        bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
        app_bt_enable_dut_mode(1);
        bt_app_entity_set_state(id, SYS_BT_DUT_MODE_STATE);
        return;
    }
    else if(bt_event == SYS_BT_BOTH_SCANNING_EVENT)
    {
        scan_enable = HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE;
        
        if(bt_app_entity_get_connect_flag(id, BT_CONNECT_STATE_SCAN_PENDING))
            bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_SCAN_PENDING);
    }
    else if(bt_event == SYS_BT_NO_SCAN_EVENT)
    {
        bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
        bt_app_set_device_scanning(HCI_NO_SCAN_ENABLE);
        bt_app_clear_all_entity_connect_flag(BT_CONNECT_STATE_SCAN_PENDING);
        
        if(!hci_get_acl_link_count(sys_hdl->unit))
        {
            app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);           // led: no match no connnect
            bt_app_entity_set_state(id, SYS_BT_UNUSED_STATE);
        }
        else
        {
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        }
        return;
    }
    
    if(bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_RECONNECTING|BT_CONNECT_STATE_SCAN_PENDING | BT_CONNECT_STATE_CONN_PENDING | BT_CONNECT_STATE_DISCONN_PENDING))
    {
        if(bt_app_check_entity_connect_flag_by_id(id,BT_CONNECT_STATE_RECONNECTING))
        {
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
            return;
        }
        else 
        {
            if(hci_get_acl_link_count(sys_hdl->unit)||LMscan_Read_Scan_Enable())
            {
                return;
            }
        }
    }
    
    if(bt_app_device_scan_is_disallowed())
    {
        bt_app_set_device_scanning(HCI_NO_SCAN_ENABLE);
        bt_app_clear_all_entity_connect_flag(BT_CONNECT_STATE_SCAN_PENDING);
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        return;
    }
    
    bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
    
	if(id == 0)
	{
		bt_state = bt_app_entity_get_state(1);                            //watch the other state
		LOG_I(APP,"bt_entity_id(1).state(%x)\r\n", bt_state);             //charles  
	}
	else
	{
		bt_state = bt_app_entity_get_state(0);                            //watch the other state
		LOG_I(APP,"bt_entity_id(0).state(%x)\r\n", bt_state);             //charles  
	}

    if(scan_enable == HCI_BOTH_INQUIRY_PAGE_SCAN_ENABLE
        && bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_RECONN_FAILED))
    {    
        bt_app_clear_all_entity_connect_flag(BT_CONNECT_STATE_RECONN_FAILED);
    }
    
	LOG_I(APP,"set_device_scanning(%x).id(%x)\r\n", scan_enable, id);     //charles   
#if 1//yuan++
    if(!app_is_bt_mode())//++By Borg@230217 (&QinZhiWei)
    {
        if(app_get_env_key_num()){
            scan_enable = HCI_PAGE_SCAN_ENABLE;
        }else{
            scan_enable = HCI_NO_SCAN_ENABLE;
        }
    }
#endif
    bt_app_set_device_scanning(scan_enable);
    bt_app_entity_set_connect_flag(id, BT_CONNECT_STATE_SCAN_PENDING);
    return;
}

/* Function name:  bt_app_standby_procedure
 * Description:    the BT application entity is standby;   
 * Parameters:
 *       id(in):     entity ID;  
 * Return value:   
 */
void bt_app_standby_procedure(uint8_t id)
{
    t_sys_bt_state next_bt_state = SYS_BT_STANDBY_STATE;
    t_sys_bt_event bt_event;
    bt_event = bt_app_entity_get_event(id);
    int_t conn_max = app_get_env_conn_retry_count();
    app_handle_t sys_hdl = app_get_sys_handler();

    app_env_handle_t env_h = app_env_get_handle();
    uint32_t conn_retry_interval=env_h->env_cfg.bt_para.disconn_retry_interval/10;;
    static uint64_t conn_retry_next_time = 0;
    static uint8_t temp_id = 0;
    if(g_bt_app_entity_container[id].is_used)
    {
        if(bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_DISCONN_PENDING))
        {
            return;
        }
        if(bt_event == SYS_BT_BOTH_SCANNING_EVENT)
        {
            LOG_I(MNG,"standby:SYS_BT_BOTH_SCANNING_EVENT\r\n");
            bt_app_disconnect_acl_link(id);
            return;
        }                 
        if((g_bt_app_entity_container[id].link_handle == 0) && !btaddr_any(&g_bt_app_entity_container[id].bt_remote_addr))
        {
            /* connect request when power on,or detach reason = 0x08,0x22 */
            if(bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_CONN_PENDING))
            {
                return;
            }
            if(app_bt_flag2_get(APP_FLAG2_5S_PRESS))
            {
                return;
            }
#if (CONFIG_DRIVER_OTA == 1)
            if(driver_ota_is_ongoing())
            {
                return;
            }
#endif
            if(g_bt_app_entity_container[id].detach_reason==0)
            {
                conn_max = app_get_env_conn_auto_count();
                conn_retry_interval=env_h->env_cfg.bt_para.auto_conn_interval/10;
                
                #if (REMOTE_RANGE_PROMPT == 1)
                if(sys_hdl->flag_soft_reset)
                {
                    conn_max = app_get_env_conn_retry_count();
                    conn_retry_interval=env_h->env_cfg.bt_para.disconn_retry_interval/10;
                }
                #endif
            }
            LOG_D(MNG,"read_scan(%x),id[%x],failure_time(%d)\r\n",LMscan_Read_Scan_Enable(),id,g_bt_app_entity_container[id].conn_failure_times);//charles debug code
            if(!TickTimer_Is_Expired(conn_retry_next_time))
            {
                return;
            }
            else
            {
                if(!hci_get_acl_link_count(sys_hdl->unit))
                {
                    if((temp_id==id)&&(SYS_BT_STANDBY_STATE==bt_app_entity_get_state(1-id)))
                    {
                        return;
                    }
                    temp_id=id;
                }
            }
            if((g_bt_app_entity_container[id].conn_failure_times < conn_max) || (-1 == conn_max))
            {
                LOG_I(MNG,"time:%d:%d,link_connect: "BTADDR_FORMAT"\r\n", g_bt_app_entity_container[id].conn_failure_times,conn_max,BTADDR(&g_bt_app_entity_container[id].bt_remote_addr));
                app_set_led_event_action(LED_EVENT_AUTO_CONNECTION);
                bt_app_entity_set_connect_flag(id,BT_CONNECT_STATE_RECONNECTING);
                bt_app_entity_set_connect_flag(id, BT_CONNECT_STATE_CONN_PENDING);
                //if(0!=LMscan_Read_Scan_Enable())
                //    bt_unit_set_scan_enable(sys_hdl->unit, 0);
                LOG_I(MNG,"SCAN:%d,conn_retry_interval=%d\r\n",LMscan_Read_Scan_Enable(),conn_retry_interval);

                bt_connection_req(&g_bt_app_entity_container[id].bt_remote_addr);//g_bt_app_entity_container[id].bt_profile_attrs);
                next_bt_state = SYS_BT_CONNECTING_STATE;
                conn_retry_next_time = conn_retry_interval + os_get_tick_counter();
            }
            else
            {
                LOG_I(MNG,"times up:%d\r\n",g_bt_app_entity_container[id].conn_failure_times);
                g_bt_app_entity_container[id].conn_failure_times = 0; 
                bt_app_entity_free(id,0);
                bt_app_entity_set_connect_flag(id,BT_CONNECT_STATE_RECONN_FAILED);
                bt_app_entity_clear_connect_flag(id,BT_CONNECT_STATE_RECONNECTING);
                if(app_env_check_inquiry_always() || (!app_get_env_key_num()))
                {
                    if(1 == app_bt_get_disconn_event()||(!app_bt_flag1_get(APP_FLAG_MATCH_ENABLE)))
                        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
                }
                else
                {
                    if(hci_get_acl_link_count(sys_hdl->unit))
                        app_set_led_event_action(LED_EVENT_CONN);
                    if(!app_bt_flag1_get(APP_FLAG_MATCH_ENABLE))
                        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
                }
            }
        }        
        else if(bt_app_entity_get_working_flag(id,BT_WORKING_FLAG_SET)) 
        {
            if(bt_app_entity_get_connect_flag(id,BT_CONNECT_STATE_SCAN_PENDING))
                bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_SCAN_PENDING);
            /* if sco conneced or avdtp media start */
            #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                uint16_t handle;
                handle = bt_sniff_get_handle_from_raddr(&g_bt_app_entity_container[id].bt_remote_addr);
                app_bt_write_sniff_link_policy(handle, 0);
            #endif
            next_bt_state = SYS_BT_WORKING_STATE;            
        }
        else if(bt_app_entity_get_connect_flag(id,BT_ALL_PROFILE_CONNECTED_SET) == BT_ALL_PROFILE_CONNECTED_SET)
        {
            if(bt_app_entity_get_connect_flag(id,BT_CONNECT_STATE_SCAN_PENDING))
                bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_SCAN_PENDING);
            #if (CONFIG_DRIVER_OTA == 1)
            if(driver_ota_is_ongoing())  return;
            #endif
            /* if all profiles connected and no working,enter sniff mode */            
            g_bt_app_entity_container[id].timer = os_timer_get_current_tick() + SYS_SLEEP_INTERVAL;
            next_bt_state = SYS_BT_SNIFF_MODE_STATE;            
        }
    }
    else
    {
        if(app_bt_flag3_get(PWR_ON_BT_SCAN_DIS)) return;
        next_bt_state = SYS_BT_SCANNING_STATE;
        LOG_I(MNG, "[%d] id:%d, standby->scan\n", __LINE__, id);
        bt_app_entity_set_event(id,SYS_BT_INVALID_EVENT);
    }
    bt_app_entity_set_state(id,next_bt_state);
    return;
}

/* Function name:  bt_app_working_procedure
 * Description:    The BT application entity is working now,include sco connecting,playing music,AT_CMD proecessing,
 * Parameters:
 *       id(in):     entity ID;  
 * Return value:   
 */
void bt_app_working_procedure(uint8_t id)
{
    t_sys_bt_event bt_event;
    bt_event = bt_app_entity_get_event(id);
    switch(bt_event)
    {
        case SYS_BT_DISCONNECTED_EVENT:
            if(bt_app_entity_get_connect_flag(id,BT_CONNECT_STATE_DISCONN_PENDING))
                bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_DISCONN_PENDING);
            bt_app_entity_set_event(id,SYS_BT_INVALID_EVENT);
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
            break;
        case SYS_BT_BOTH_SCANNING_EVENT:
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
            break;
        default:break;
    }
    if(!bt_app_entity_get_working_flag(id,BT_WORKING_FLAG_SET))
    {
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        return;
    }
}

/* Function name:  bt_app_connecting_procedure
 * Description:    The BT application entity is connecting the other BT device;  
 * Parameters:
 *       id(in):     entity ID;  
 * Return value:   none;
 */
void bt_app_connecting_procedure(uint8_t id)
{
    t_conn_result conn_state;
    t_sys_bt_event bt_event;
    conn_state  = get_connection_state();
    bt_event = bt_app_entity_get_event(id);
    btaddr_t* addr = NULL;
    app_handle_t sys_hdl = app_get_sys_handler();

    // if(app_bt_flag3_get(PWR_ON_BT_RECONNECT_DIS))
    // {
    //     bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
    //     // app_bt_flag3_set(PWR_ON_BT_RECONNECT_DIS, 0);
    //     // bt_app_entity_set_event(id, SYS_BT_BOTH_SCANNING_EVENT);
    //     // addr = bt_app_entit_get_rmt_addr(id);
    //     // unit_send_cmd(sys_hdl->unit,HCI_CMD_CREATE_CON_CANCEL,addr,BTADDR_LEN);        
    //     // bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
    //     // bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
    //     // bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_CONN_PENDING);
    //     // bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_RECONN_FAILED);
    //     LOG_I(MNG, "[%d] id:%d, connecting->standby\n", __LINE__, id);
    // }

    if(bt_event == SYS_BT_BOTH_SCANNING_EVENT)
    {
        /* bt_connection_cancel() */
        LOG_I(MNG, "[%d] id:%d, connecting->standby\n", __LINE__, id);
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
    }
    else if(bt_event == SYS_BT_DUT_MODE_EVENT)
    {
        addr = bt_app_entit_get_rmt_addr(id);
        unit_send_cmd(sys_hdl->unit,HCI_CMD_CREATE_CON_CANCEL,addr,BTADDR_LEN);        
        bt_app_entity_set_event(id, SYS_BT_INVALID_EVENT);
        app_bt_enable_dut_mode(1);
        bt_app_entity_set_state(id, SYS_BT_DUT_MODE_STATE);
        bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_CONN_PENDING);
        bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_RECONN_FAILED);
        return;
    }    
    if(conn_state == CONN_FAIL)
    {
    	g_bt_app_entity_container[id].conn_failure_times++;
        LOG_I(MNG,"id[%x],fail_times:%x\r\n",id,g_bt_app_entity_container[id].conn_failure_times);
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_CONN_PENDING);
    }
    else if (conn_state == CONN_SUCCESS)
    {
        g_bt_app_entity_container[id].conn_failure_times = 0;
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_CONN_PENDING);
        bt_app_entity_clear_connect_flag(id,BT_CONNECT_STATE_RECONNECTING);
        bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_RECONN_FAILED);
    }
    // if finish connecting, the entity will from connecting state->standby state->sniff state, no scanning, so the scan pending won't be clear
    if(bt_app_entity_get_connect_flag(1-id,BT_CONNECT_STATE_SCAN_PENDING))
        bt_app_entity_clear_connect_flag(1-id, BT_CONNECT_STATE_SCAN_PENDING);

}

/* Function name:  bt_app_sniff_mode_procedure
 * Description:    The BT application entity enter sniff mode,when it has no working;
 * Parameters:
 *       id(in):     entity ID;  
 * Return value:   
 */
void bt_app_sniff_mode_procedure(uint8_t id)
{
    t_sys_bt_event bt_event;
    app_env_handle_t env_h = app_env_get_handle();
    bt_event = bt_app_entity_get_event(id);
    switch(bt_event)
    {
        case SYS_BT_DISCONNECTED_EVENT:
            if(bt_app_entity_get_connect_flag(id,BT_CONNECT_STATE_DISCONN_PENDING))
                bt_app_entity_clear_connect_flag(id, BT_CONNECT_STATE_DISCONN_PENDING);
            
            bt_app_entity_set_event(id,SYS_BT_INVALID_EVENT);
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
            break;
        case SYS_BT_BOTH_SCANNING_EVENT:
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
            break;
        case SYS_BT_UNSNIFF_EVENT:
            bt_app_entity_set_event(id,SYS_BT_INVALID_EVENT);
            bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
            break;
        default:break;
    }
    /* check working flag */
    if(bt_app_entity_get_working_flag(id,BT_WORKING_FLAG_SET))
    {
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        return;
    }
    
#if (CONFIG_DRIVER_OTA == 1)
    if(driver_ota_is_ongoing())
    {
        bt_app_entity_set_state(id, SYS_BT_STANDBY_STATE);
        return;
    }
#endif

    if(os_timer_is_expired_for_time(g_bt_app_entity_container[id].timer,os_timer_get_current_tick()))
    {    	
        #if 1
        uint8_t idx = bt_sniff_get_index_from_handle(g_bt_app_entity_container[id].link_handle);
        if(bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_CONN_PENDING)&&CONFIG_AS_SLAVE_ROLE)
        {
            return;
        }

        if(bt_sniff_is_used(idx))
        {
            if(!bt_sniff_is_policy(idx))
            {
                app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_idx(idx), 1);
            }
            else if(bt_sniff_is_active(idx))
            {
                app_bt_enter_sniff_mode(bt_sniff_get_handle_from_idx(idx), 1);
            }
        }
        #endif
        g_bt_app_entity_container[id].timer = os_timer_get_current_tick() + env_h->env_cfg.system_para.sleep_timeout;
    }
}

/* Function name:   bt_app_dut_mode_procedure
 * Description:     the bt device running in DUT mode,if you will exit DUT mode, power off/on or reset only. :) 
 * Parameters:    
 *          id(in):     entity ID;    
 * Return value:    none 
 */
void bt_app_dut_mode_procedure(uint8_t id)
{
    t_sys_bt_event bt_event;
    bt_event = bt_app_entity_get_event(id);
    if(bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_DUT_PENDING))
    {
        return;
    }

    if(bt_event ==  SYS_BT_DISABLE_COMPLETE_EVENT)
    {   
        bt_app_entity_set_connect_flag(id, BT_CONNECT_STATE_DUT_PENDING);
        CONST static char bluecmdDut0[] = { 0x01, 0x1A, 0x0c, 0x01, 0x03};
        CONST static char bluecmdDut1[] = { 0x01, 0x05, 0x0c, 0x03, 0x02, 0x00, 0x02};
        CONST static char bluecmdDut2[] = { 0x01, 0x03, 0x18, 0x00 };
        uart_send_poll((uint8_t *)bluecmdDut0, sizeof(bluecmdDut0));
        uart_send_poll((uint8_t *)bluecmdDut1, sizeof(bluecmdDut1));
        uart_send_poll((uint8_t *)bluecmdDut2, sizeof(bluecmdDut2));
        LOG_I(MNG,"Enter Dut test mode success!\r\n");
    }
}

/* Function name:  bt_app_management_sched
 * Description:    BT application entity management schedule;   
 * Parameters:     none
 * Return value:   none
 */
void bt_app_management_sched(void)
{
    uint8_t entityID;
    t_sys_bt_state bt_state = SYS_BT_UNUSED_STATE;
    
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))
        return;
    for(entityID = 0; entityID < BT_MAX_AG_COUNT; entityID++)
    {
        bt_state = bt_app_entity_get_state(entityID);
		
        switch(bt_state)
        {
            case SYS_BT_UNUSED_STATE:
                bt_app_unused_procedure(entityID);
                break;
                
            case SYS_BT_SCANNING_STATE:
                bt_app_scanning_procedure(entityID);
                break;
                
            case SYS_BT_WORKING_STATE:
                bt_app_working_procedure(entityID);
                break;
                
            case SYS_BT_CONNECTING_STATE:
                bt_app_connecting_procedure(entityID);
                break;
                
            case SYS_BT_SNIFF_MODE_STATE:
                bt_app_sniff_mode_procedure(entityID);
                break;
                
            case SYS_BT_DUT_MODE_STATE:
                bt_app_dut_mode_procedure(entityID);
                break;
                
            case SYS_BT_STANDBY_STATE:
                bt_app_standby_procedure(entityID);
                break;
                
            default:
                break;
        }
    }
}
