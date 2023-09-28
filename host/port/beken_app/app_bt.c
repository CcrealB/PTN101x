#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_init.h"
#include "tra_hcit.h"
#include "karacmd.h"    //cjq++
extern uint8_t g_ad_da_open_flag;

//extern uint32_t XVR_analog_reg_save[16];

static APP_SYSTEM_T g_sys_app;
 
#if (CONFIG_BT_USER_FLAG3 == 1)
static uint32_t g_user_flag_sm3;
#endif

static BT_ENTITY_T Remote_BlueDev = {0,{{0}}};
//BT_ENTITY_T dis_dev = {0,{{0}}};
//static int32_t retry_count = 0;
/* api declaration*/

extern t_scanEnable LMscan_Read_Scan_Enable(void);
extern void BKxvr_Set_Tx_Power(uint32 pwr);
#if A2DP_ROLE_SOURCE_CODE
extern result_t a2dpSrcCmdConnect(char *params, unsigned int len);
extern uint8_t get_a2dp_role(void);
#endif

__inline app_handle_t app_get_sys_handler(void)
{
    return &g_sys_app;
}

jtask_h app_get_audio_task_handle(void)
{
    return g_sys_app.app_audio_task;
}

jtask_h app_common_get_task_handle( void )
{
    return g_sys_app.app_common_task;
}



/*
test scenarios:
    1: '0'
    2: '1'
    3: '1010'
    4: PN9
    5: loopback/acl
    6: loopback/sco
    7: acl/no whiten
    8: sco/no whiten
    9: '11110000'

Packet Type:
    NULLpkt     =  0,   
    POLLpkt     =  1,   
    FHSpkt      =  2,   
    DM1         =  3,   
    DH1         =  4,   
    HV1         =  5,   
    HV2         =  6,   
    HV3         =  7,   
    DV          =  8,   
    AUX1        =  9,   
    DM3         = 10,   
    DH3         = 11,   
    EV4         = 12,   
    EV5         = 13,   
    DM5         = 14,   
    DH5         = 15,   
    IDpkt       = 16,  
    EDR_2DH1    = 20, 
	EV3         = 21,
    EDR_2EV3    = 22, 
    EDR_3EV3    = 23,     
	EDR_3DH1    = 24, 
	EDR_AUX1    = 25, 
    EDR_2DH3    = 26, 
    EDR_3DH3    = 27, 
    EDR_2EV5    = 28, 
    EDR_3EV5    = 29, 
    EDR_2DH5    = 30, 
    EDR_3DH5    = 31  
*/
void app_bt_enable_non_signal_test(t_TestControl *tc_contents)
{
    uint8 tc_pdu[17] = {0x00};
    uint16 payload_len = SYSconst_Get_Packet_Length(tc_contents->packet_type);
    if(!(app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE)))
    {
        LOG_E(DUT,"Non-Signal Test must be on DUT mode\r\n");
        return;
    }

    BKxvr_Set_Tx_Power(tc_contents->tx_power);
    tc_pdu[0] = 0x01;
    tc_pdu[1] = 0x8c;
    tc_pdu[2] = 0xfc;
    tc_pdu[3] = 0x0D;
    tc_pdu[4] =  tc_contents->tester_lap_nap & 0xff;
    tc_pdu[5] =  (tc_contents->tester_lap_nap>>8) & 0xff;
    tc_pdu[6] =  (tc_contents->tester_lap_nap>>16) & 0xff;
    tc_pdu[7] =  (tc_contents->tester_lap_nap>>24) & 0xff;
    tc_pdu[8] = tc_contents->test_scenarios ^ 0x55;
    tc_pdu[9] = tc_contents->hop_mode ^ 0x55;
    tc_pdu[10] = tc_contents->tx_chnl ^ 0x55;
    tc_pdu[11] = tc_contents->rx_chnl ^ 0x55;
    tc_pdu[12] = 0x00 ^ 0x55;
    tc_pdu[13] = tc_contents->interval ^ 0x55;
    tc_pdu[14] = tc_contents->packet_type ^ 0x55;
    tc_pdu[15] = (payload_len & 0xff) ^ 0x55;
    tc_pdu[16] = ((payload_len>>8) & 0x0ff) ^ 0x55;
    uart_send_poll(tc_pdu,sizeof(tc_pdu));

}


#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE

static BT_SNIFF_T  g_sniff_st[NEED_SNIFF_DEVICE_COUNT];
void app_bt_write_sniff_link_policy(uint16_t link_handle,uint8_t set_link_policy)
{
    uint8_t idx;
    app_handle_t app_h = app_get_sys_handler();
    hci_write_link_policy_settings_cp  lp_cp;
    hci_exit_sniff_mode_cp exit_cp;
#if(CONFIG_AS_SLAVE_ROLE == 1)
    btaddr_t *rmt_addr;
#endif
    if(!app_env_check_sniff_mode_Enable())
        return;
    if( app_h->unit == NULL )
        return;
    
    lp_cp.settings = app_h->unit->hci_link_policy;    
    idx = bt_sniff_get_index_from_handle(link_handle);
    if(idx >= NEED_SNIFF_DEVICE_COUNT)
    {
        return;
    }
    bt_sniff_set_policy(idx,set_link_policy);
#if(CONFIG_AS_SLAVE_ROLE == 1)
    rmt_addr = bt_sniff_get_addrptr_from_idx(idx);
#endif
    if(set_link_policy)
    {
        LOG_I(SNIFF, "sniff enable:%d,%04x\r\n",idx,link_handle);
        lp_cp.con_handle = link_handle;
        lp_cp.settings |= HCI_LINK_POLICY_ENABLE_SNIFF_MODE;
        hci_send_cmd( app_h->unit, HCI_CMD_WRITE_LINK_POLICY_SETTINGS, (void *)&lp_cp, sizeof(lp_cp));
#if(CONFIG_AS_SLAVE_ROLE == 1)
        Judge_role(rmt_addr,FALSE);
#endif
    }
    else
    {
        if(!bt_sniff_is_active(idx))
        {
            LOG_I(SNIFF, "exit sniff:%d,%04x\r\n",idx,link_handle);
            exit_cp.con_handle = link_handle;
            hci_send_cmd( app_h->unit, HCI_CMD_EXIT_SNIFF_MODE, (void *)&exit_cp, sizeof(hci_exit_sniff_mode_cp));
            /* bt_sniff_set_active(idx,1);*/
        }
        /* else */
        {
            LOG_I(SNIFF,"sniff disable:%d,%04x\r\n",idx,link_handle);
            lp_cp.con_handle = link_handle;
            lp_cp.settings &= ~HCI_LINK_POLICY_ENABLE_SNIFF_MODE;
            hci_send_cmd( app_h->unit, HCI_CMD_WRITE_LINK_POLICY_SETTINGS, (void *)&lp_cp, sizeof(lp_cp));
#if(CONFIG_AS_SLAVE_ROLE == 1)
            Judge_role(rmt_addr,FALSE);
#endif
        }
    }

}
void app_bt_enter_sniff_mode(uint16_t link_handle,uint8_t enable)
{
    uint8_t idx;
    app_handle_t app_h = app_get_sys_handler();
    hci_sniff_mode_cp sniff_cp;
    hci_exit_sniff_mode_cp exit_cp;
    if( app_h->unit == NULL )
        return;
    idx = bt_sniff_get_index_from_handle(link_handle);
    if(idx >= NEED_SNIFF_DEVICE_COUNT)
    {
        return;
    }
    if(enable)
    {        
        if(a2dp_has_music()||hfp_has_sco())
        {
            return;
        }
        else
        {
            sniff_cp.con_handle = link_handle;
#ifdef LE_SLEEP_ENABLE
            sniff_cp.max_interval = 0x320; /* 0x320=500ms */
            sniff_cp.min_interval = 0x280; /* 0x200=320ms;0x280=400ms;0x2A0=420ms;0x2D0=450ms */
#else
            sniff_cp.max_interval = 0x280; //0x500;   /* 0x0006 ~ 0x0540 */
            sniff_cp.min_interval = 0x200; //0x400;   /* 0x0006 ~ 0x0540 */
#endif
            sniff_cp.attempt = 4;
            sniff_cp.timeout = 4;
            LOG_I(SNIFF,"enter sniff:%d,%04x,slots:%d\r\n",idx,link_handle,sniff_cp.min_interval);
            hci_send_cmd( app_h->unit, HCI_CMD_SNIFF_MODE, (void *)&sniff_cp, sizeof(hci_sniff_mode_cp));
        /* bt_sniff_set_active(idx,0); */
        }
    }
    else
    {
        LOG_I(SNIFF,"exit sniff:%d,%04x\r\n",idx,link_handle);
        exit_cp.con_handle = link_handle;
        hci_send_cmd( app_h->unit, HCI_CMD_EXIT_SNIFF_MODE, (void *)&exit_cp, sizeof(hci_exit_sniff_mode_cp));
        /* bt_sniff_set_active(idx,1); */
    }

}



void bt_sniff_alloc_st(uint8_t idx,uint16_t handle,btaddr_t *rbtaddr)
{
    idx &= 0x01;
    g_sniff_st[idx].is_active = 1;
    g_sniff_st[idx].is_used = 1;
    g_sniff_st[idx].is_policy = 0;
    g_sniff_st[idx].link_handle = handle;

    memcpy((uint8_t *)&g_sniff_st[idx].remote_btaddr,
                        (uint8_t *)rbtaddr,
                        sizeof(btaddr_t));
}

void bt_sniff_free_st(uint8_t idx)
{
    idx &= 0x01;
    g_sniff_st[idx].is_active = 0;
    g_sniff_st[idx].is_used = 0;
    g_sniff_st[idx].link_handle = 0;
    g_sniff_st[idx].is_policy = 0;
}
uint8_t bt_sniff_find_st_available(void)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(g_sniff_st[idx].is_used == 0)
        {
            return idx;
        }
    }
    return 0;
}
uint16_t bt_sniff_get_handle_from_raddr(btaddr_t *rbtaddr)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(btaddr_same(rbtaddr,&g_sniff_st[idx].remote_btaddr))
        {
            return g_sniff_st[idx].link_handle;
        }
    }
    return 0;
}
uint8_t bt_sniff_get_index_from_raddr(btaddr_t *rbtaddr)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(btaddr_same(rbtaddr,&g_sniff_st[idx].remote_btaddr))
        {
            return idx;
        }
    }
    return 0;
}
uint8_t bt_sniff_get_index_from_handle(uint16_t handle)
{
    uint8_t idx = 0;
    for(; idx<NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(g_sniff_st[idx].link_handle == handle)
        {
            return idx;
        }
    }
    return 0;
}
uint16_t bt_sniff_get_handle_from_idx(uint8_t idx)
{
    return g_sniff_st[idx].link_handle;
}

btaddr_t bt_sniff_get_rtaddr_from_idx(uint8_t idx)
{
    return g_sniff_st[idx].remote_btaddr;
}
btaddr_t *bt_sniff_get_addrptr_from_idx(uint8_t idx)
{
    return (btaddr_t *)&g_sniff_st[idx].remote_btaddr;
}
uint8_t bt_sniff_is_used(uint8_t idx)
{
    return(g_sniff_st[idx].is_used);
}
uint8_t bt_sniff_is_active(uint8_t idx)
{
    return(g_sniff_st[idx].is_active);
}
uint8_t bt_sniff_is_policy(uint8_t idx)
{
    return(g_sniff_st[idx].is_policy);
}
void bt_sniff_set_policy(uint8_t idx,uint8_t enable)
{
    g_sniff_st[idx].is_policy = enable;
}
void bt_sniff_set_active(uint8_t idx,uint8_t enable)
{
    g_sniff_st[idx].is_active = enable&0x01;
}
uint8_t bt_sniff_is_all_active(void)
{
    uint8_t idx = 0;

    for(idx = 0; idx < NEED_SNIFF_DEVICE_COUNT; idx++)
    {
        if(!g_sniff_st[idx].is_active)
        {
                return FALSE;
        }
    }

    return TRUE;
}
void bt_sniff_all_exit_mode(void)
{
    uint8 idx =0;

    for(idx=0;idx<NEED_SNIFF_DEVICE_COUNT;idx++)
    {
        if(bt_sniff_is_used(idx))
        {
            if(bt_sniff_is_policy(idx))
            {  
                app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_idx(idx), 0);
            }
        }
    }
}
#else
uint8_t bt_sniff_get_index_from_handle(uint16_t handle)
{
    return 0;
}
uint16_t bt_sniff_get_handle_from_idx(uint8_t idx)
{
    return 0;
}
uint8_t bt_sniff_is_used(uint8_t idx)
{
    return 0;
}
uint8_t bt_sniff_is_active(uint8_t idx)
{
    return 0;
}
void bt_sniff_set_active(uint8_t idx,uint8_t enable)
{

}
void app_bt_write_sniff_link_policy(uint16_t link_handle,uint8_t set_link_policy)
{

}
#endif

void app_sleep_func( int enable)
{

    if(enable==0)
        CLEAR_SLEEP_TICK;

    if(g_ad_da_open_flag) return;

#if (CONFIG_DAC_CLOSE_IN_IDLE == 0)
	if((!app_bt_flag1_get(APP_AUDIO_WORKING_FLAG_SET)))
	{
       aud_dac_close();
	}
#endif

#ifdef CONFIG_APP_UDISK
    if(app_is_udisk_mode())
    {
        return;
    }
#endif

#ifdef CONFIG_APP_SDCARD
//    if(app_is_sdcard_mode())
//    {
//        return;
//    }
#endif

    if( enable&& (app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE|APP_FLAG_LINEIN)|| app_wave_playing()))
    {
        return;
    }
    
#if (CONFIG_DRIVER_OTA == 1)
    if(driver_ota_is_ongoing())
    {
        return;
    }
#endif

#if 0 // CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE  , enter sniff in sniff procedure
    uint8_t idx = 0;
    if(bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_CONN_PENDING)&&CONFIG_AS_SLAVE_ROLE)
    {
        return;
    }
    if(enable && (hfp_has_connection() || avrcp_has_connection() || a2dp_has_connection())
		&&(app_env_check_sniff_mode_Enable())
		&& (!app_bt_flag1_get(APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION)) )
    {
        for(idx=0;idx<NEED_SNIFF_DEVICE_COUNT;idx++)
        {
            btaddr_t raddr = bt_sniff_get_rtaddr_from_idx(idx);
            if(bt_sniff_is_used(idx))
            {
                if(hfp_has_the_sco(raddr)
                    || a2dp_has_the_music(&raddr) )
                {
                    continue;
                }
                if((a2dp_has_the_connection(&raddr)
                	|| hfp_has_the_connection(raddr)))
                {
                    if(!bt_sniff_is_policy(idx))
                    {
                        app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_idx(idx), 1);
                    }
                    else if(bt_sniff_is_active(idx))
                    {
                        os_printf("app_sleep_func\r\n");
                        app_bt_enter_sniff_mode(bt_sniff_get_handle_from_idx(idx), 1);
                    }
                }
            }
        }
    }
#endif
    if(enable)
    {
        if(app_wave_playing())
        {
            app_wave_file_play_stop();
        }
    }
}


int bt_is_reject_new_connect(void)
{
    int ret = 0;

    app_handle_t sys_hdl = app_get_sys_handler();
    int acl_count = hci_get_acl_link_count(sys_hdl->unit);


    if(hfp_has_sco() // app_bt_flag2_get(APP_FLAG2_CALL_PROCESS)
     	|| (acl_count >= BT_MAX_AG_COUNT)
    )
    {
        LOG_I(CONN,"reject flag2:0x%x,music:%d,acl cnt:%d\r\n",sys_hdl->flag_sm2,a2dp_has_music(),acl_count);
        ret = 1;
    }

    if (app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
    {
        LOG_I(CONN,"reject flag:0x%x\r\n",sys_hdl->flag_sm1);

        ret = 1;
    }

    return ret;
}

void app_bt_enable_complete_wrap( hci_unit_t *unit)
{
    uint32_t i;
    uint8_t cmd[256];
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
    if(!(app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE)))
    {
        app_bt_flag1_set(APP_BUTTON_FLAG_BT_ENABLE, 1);
        sys_hdl->unit = unit;

        btaddr_copy(&sys_hdl->remote_btaddr, &env_h->env_data.default_btaddr);
    }

    for(i = 0; i < MAX_KEY_STORE; i++)
    {
        if((env_h->env_data.key_pair[i].used == 0x01) || (env_h->env_data.key_pair[i].used == 0x02))
        {
            memcpy(&cmd[i*22 + 1], (uint8_t *)&env_h->env_data.key_pair[i].btaddr, 6);
            memcpy(&cmd[i*22 + 7], (uint8_t *)env_h->env_data.key_pair[i].linkkey, 16);
			if(env_h->env_data.key_pair[i].a2dp_src_uclass)
            {
            	int8_t idx = bt_app_entity_find_id_from_raddr(&(env_h->env_data.key_pair[i].btaddr));
				if(idx == MNG_ERROR_NO_ENTITY)
				{
					LOG_I(ENV, "MNG_ERROR_NO_ENTITY\r\n")
				}
                
				bt_app_entity_set_mac_win_book(idx, env_h->env_data.key_pair[i].a2dp_src_uclass);
            }
        }
        else
        {
            //break;
        }
    }


    if(i > MAX_KEY_STORE)
    {
        i = MAX_KEY_STORE;
    }

    if(i > 0)
    {
        cmd[0] = i;
        hci_send_cmd(sys_hdl->unit,
                        HCI_CMD_WRITE_STORED_LINK_KEY,
                        cmd,
                        i*22 + 1);
    }
    else
    {
        unit->app_cbs.write_linkkey_complete_cb(unit->app_ctx, 0);
    }

}

#if 0
static void app_bt_match_timeout_timerfunc(void *arg)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    app_sleep_func(0);

    if((env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_AUTO_CONN_PERIOD)
        && (0 == app_bt_get_disconn_event()))
    {
        if(! (app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION)))
        {
            app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION, 1);
            bt_auto_connect_start();
        }
    }
    else
    {
        bt_unit_set_scan_enable(sys_hdl->unit, HCI_PAGE_SCAN_ENABLE);
        app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
    }
}

void app_bt_set_match_timeout(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    app_handle_t sys_hdl = app_get_sys_handler();

    if(env_h->env_cfg.bt_para.match_timeout == -1)
        return;

    jtask_stop(sys_hdl->app_match_task);

    jtask_schedule(sys_hdl->app_match_task,
                    env_h->env_cfg.bt_para.match_timeout*1000,
                    (jthread_func)app_bt_match_timeout_timerfunc,
                    (void *)NULL);
}
#endif

uint8_t app_get_best_offset_level(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    if(0x80 == (env_h->env_data.offset_bak_8852&0xc0))
    	return 3;
    if(0x80 == (env_h->env_data.offset_bak_tester&0xc0))
    	return 2;
    if(0x80 == (env_h->env_data.offset_bak_phone&0xc0))
    	return 1;
    return 0;
}

uint8_t app_get_best_offset(void)
{
    app_env_handle_t env_h = app_env_get_handle();
    uint8 offset = env_h->env_cfg.system_para.frq_offset; //from configer
    //{b7:b6} = 11(no data) 10(have data) 00(not use)
    if(0x80 == (env_h->env_data.offset_bak_phone&0xc0))
    	offset = env_h->env_data.offset_bak_phone;
    if(0x80 == (env_h->env_data.offset_bak_tester&0xc0))
    	offset = env_h->env_data.offset_bak_tester;
    if(0x80 == (env_h->env_data.offset_bak_8852&0xc0))
    	offset = env_h->env_data.offset_bak_8852;
    return offset&0x3f;
}

void bt_create_conn_status_wrap(uint8_t status)
{
    LOG_I(CONN,"create_conn_status_wrap,%x\r\n",status);
    if(app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION) )
    {
        if(status == 0)  // auto connection success
        {
            app_bt_flag1_set(APP_FLAG_ACL_CONNECTION, 1);
        }
        else    //auto connection failed
        {

//            bt_auto_connect_stop();
            if(!app_bt_flag1_get(APP_FLAG_MATCH_ENABLE))
            	  app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);        
        }
    }
    else if(app_bt_flag1_get(APP_FLAG_RECONNCTION))
    {
        LOG_I(CONN,"create_conn1\r\n");
        if(status == 0)
        {
            app_bt_flag1_set(APP_FLAG_ACL_CONNECTION,1);
        }
    }
}


void app_bt_disable_complete_wrap(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    LOG_I(APP,"app_bt_disable_complete_wrap()\r\n");

    jtask_stop(sys_hdl->app_match_task);

    app_bt_flag1_set(APP_BUTTON_FLAG_BT_ENABLE, 0);

    if(app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE))
    {
        CONST static char bluecmdDut0[] = { 0x01, 0x1A, 0x0c, 0x01, 0x03};
        CONST static char bluecmdDut1[] = { 0x01, 0x05, 0x0c, 0x03, 0x02, 0x00, 0x02};
        CONST static char bluecmdDut2[] = { 0x01, 0x03, 0x18, 0x00 };
        //BK3000_GPIO_18_CONFIG = 0x40;   // radio on status;
        //BK3000_GPIO_GPIODCON |= (1<<18);
        uart_send_poll((uint8_t *)bluecmdDut0, sizeof(bluecmdDut0));
        uart_send_poll((uint8_t *)bluecmdDut1, sizeof(bluecmdDut1));
        uart_send_poll((uint8_t *)bluecmdDut2, sizeof(bluecmdDut2));
        LOG_I(APP,"Enter Dut test mode success!\r\n");
        app_set_led_event_action(LED_EVENT_TEST_MODE);
    }
    else if(app_bt_flag1_get(APP_FLAG_POWERDOWN))
    {
        app_set_led_event_action(LED_EVENT_POWER_OFF);
        app_led_action(1);

		if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_DM_1V1|BT_MODE_BLE))
		{
			app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)sys_hdl->unit, 500);
		}
		else
		{
			app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)sys_hdl->unit, 1000);
		}

    }
    else if(app_bt_flag1_get(APP_FLAG_LINEIN))
    {
        jtask_stop(sys_hdl->app_auto_con_task);
        app_bt_flag1_set((APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION),0);
        jtask_stop(sys_hdl->app_reset_task);
        sys_hdl->button_mode = BUTTON_NONE;
        sys_hdl->button_commit = BUTTON_NONE;
        sys_hdl->button_press_count = 0;
        sys_hdl->button_long_press_count = 0;
        sys_hdl->button_state = BUTTON_PRESS_NONE;

        BK3000_Ana_Line_enable(1);
        aud_dac_set_volume(sys_hdl->linein_vol);
        aud_volume_mute(0);
      	aud_PAmute_delay_operation(0);

        LOG_I(APP,"linein_vol:%d\r\n",sys_hdl->linein_vol);
    }
}

#if CONFIG_BLUETOOTH_HID
extern void Set_photoKey(void);
#endif
void app_bt_conn_compl_wrap(hci_link_t *link, const btaddr_t *rbtaddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();

#ifndef CONFIG_BLUETOOTH_SSP
    uint32_t  a2dp_conn_delay;
    app_env_handle_t env_h = app_env_get_handle();
#endif

	int8_t id = bt_app_entity_find_id_from_raddr((btaddr_t *)rbtaddr);
    if(!bt_app_entity_get_connect_flag(id,BT_CONNECT_STATE_RECONNECTING))
    {
        if(bt_acl_con_get_specific_uclass())
        {
			bt_app_entity_set_mac_win_book(id, bt_acl_con_get_specific_uclass());
        }
        else
        {
			bt_app_entity_set_mac_win_book(id, 0);
        }
    }
    if(bt_app_entity_get_mac_win_book(id))
        LOG_I(CONN, "A2DP.SRC is mac|win ID=%x\r\n",bt_app_entity_get_mac_win_book(id));

    bt_app_entity_clear_sys_flag(id,SYS_WORK_WAVE_PLAY);
    app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 0);
    jtask_stop(sys_hdl->app_a2dp_task);
#if CONFIG_A2DP_CONN_DISCONN
	sys_hdl->a2dp_state = APP_A2DP_STATE_INIT;
#endif
#if CONFIG_HFP_CONN_DISCONN
	sys_hdl->hfp_state = APP_HFP_STATE_INIT;
#endif
    LOG_I(APP,"app_bt_conn_compl_wrap 0\r\n");

#if CONFIG_BLUETOOTH_HID
    if(app_env_check_HID_profile_enable())
        Set_photoKey();
#endif

    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;

    app_bt_flag1_set(APP_FLAG_ACL_CONNECTION,1);
    memcpy((uint8_t *)&sys_hdl->remote_btaddr,
                        (uint8_t *)rbtaddr,
                        sizeof(btaddr_t));
    sys_hdl->link_handle = link->hl_handle;

    set_connection_event(&sys_hdl->remote_btaddr,CONN_EVENT_ACL_CONNECTED);

    if(app_bt_flag1_get(APP_FLAG_RECONNCTION) && btaddr_same(rbtaddr,&sys_hdl->reconn_btaddr))
        jtask_stop(sys_hdl->app_auto_con_task);

    jtask_stop(sys_hdl->app_match_task);    
    bt_app_entity_clear_bt_connected_wrap(id); 
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    uint8_t idx=0;
    idx = bt_sniff_find_st_available();
    if(idx < NEED_SNIFF_DEVICE_COUNT)
        bt_sniff_alloc_st(idx, sys_hdl->link_handle, &sys_hdl->remote_btaddr);
#endif

}

void app_bt_shedule_task(jthread_func func, void *arg, uint32_t delay_ms)
{

    app_handle_t sys_hdl = app_get_sys_handler();

    jtask_stop(sys_hdl->app_auto_con_task);

     if(func == NULL)
        return;

     CLEAR_PWDOWN_TICK;
     CLEAR_SLEEP_TICK;
     jtask_schedule(sys_hdl->app_auto_con_task, delay_ms, func, arg);
}


int app_bt_get_disconn_event(void)
{
    app_env_handle_t env_h = app_env_get_handle();

    return env_h->env_cfg.bt_para.action_disconn;
}

void app_bt_reconn_after_callEnd(void)
{
    return;
}

void app_bt_connected_wrap( btaddr_t *raddr)
{
    int ret;
    uint8_t  id=0;
    MSG_T msg;
    app_env_handle_t  env_h = app_env_get_handle();
    uint8_t role=0;
    app_handle_t sys_hdl = app_get_sys_handler();

    memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)raddr, sizeof(btaddr_t));
    env_h->env_data.offset_bak_phone = (system_get_0x4d_reg())|0x80;
    ret = app_env_write_action(&(env_h->env_data.default_btaddr),1);
    id=bt_app_entity_find_id_from_raddr(raddr);

    if(ret)
    {
        msg.id = MSG_ENV_WRITE_ACTION;
        msg.arg = 0x01;
        msg.arg2 = id;
        msg.hdl = 0;
        msg_lush_put(&msg);
    }
    LOG_D(CONN,"all profiles are connected,flash write:%d, id:%d\r\n",ret, id);
    bt_app_entity_clear_connect_flag(id,BT_CONNECT_STATE_RECONNECTING);
    //bt_app_entity_set_sys_flag(id,SYS_WORK_WAVE_PLAY);
    bt_app_entity_set_event_by_addr(raddr,SYS_BT_CONNECTED_EVENT);
    //app_wave_file_play_start(APP_WAVE_FILE_ID_CONN);
    app_set_led_event_action(LED_EVENT_CONN);

    role = hci_get_acl_link_role(sys_hdl->unit,raddr);
    if(role == HCI_ROLE_MASTER)
    {
        bt_app_entity_set_sys_flag(id,SYS_WORK_WAVE_PLAY);
        if(a2dp_has_music()==0){//無播放音乐 yuan++
#if defined MZ_200K                 //cjq++
        	if(SysInf.Lang)
        		app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8);
        	else
#endif
        	app_wave_file_play_start(APP_WAVE_FILE_ID_CONN);
    	}
    }
}

void app_bt_disconn_wrap(uint8_t reason, btaddr_t *raddr)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    app_env_handle_t  env_h = app_env_get_handle();

    int ret;
    MSG_T msg;

    #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    uint8_t  idx=0;
    idx = bt_sniff_get_index_from_raddr(raddr);
    bt_sniff_free_st(idx);
    #endif
	

    app_set_crystal_calibration(0);

    set_connection_event(raddr, CONN_EVENT_NULL);
    bt_app_entity_clear_conn_flag_by_addr(raddr, BT_CONNECT_STATE_DISCONN_PENDING);
    /* bt_app_management */
    bt_app_entity_free_by_addr(raddr,reason);
    env_h->env_data.disconnect_reason = reason;

    // keep the addr printf for confirm problem
    LOG_I(CONN,"bt_disconn_reason: 0x %x, raddr:"BTADDR_FORMAT"\r\n", reason, BTADDR(raddr));
    
    #if (REMOTE_RANGE_PROMPT == 1)
    app_remote_range_wave_play(reason,0);
    #endif

    if(((!app_bt_flag1_get(APP_FLAG_LINEIN)) && (!app_bt_flag2_get(APP_FLAG2_RECONN_AFTER_DISCONNEC)))
		&&(!app_charge_is_powerdown()) && (!app_bt_flag2_get(APP_FLAG2_ROLE_SWITCH_FAIL)) )
    {
        if(bt_app_entity_get_sys_flag(bt_app_entity_find_id_from_raddr(raddr),SYS_WORK_WAVE_PLAY))
        {
            #if (REMOTE_RANGE_PROMPT == 1)
            if(!GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE)
            &&((reason != HCI_ERR_CONNECTION_TIMEOUT)||(reason != HCI_ERR_LMP_RESPONSE_TIMEOUT)))
            #endif
            {
            	app_handle_t sys_hdl = app_get_sys_handler();
            	if(sys_hdl->sys_work_mode == SYS_WM_BT_MODE){	// BT MODE RUN yuan++
	#if defined MZ_200K                 //cjq++
				if(SysInf.Lang)
					app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
				else
	#endif
            		app_wave_file_play_start(APP_WAVE_FILE_ID_DISCONN);
            	}
            }
        }
        else
        {
            LOG_I(CONN,"disconn no wave play\r\n");
        }
    }
    else
    {
        LOG_I(CONN,"line:%d,reconn_disconn:%d,chg_pd:%d\r\n",!(!app_bt_flag1_get(APP_FLAG_LINEIN)),!(!app_bt_flag2_get(APP_FLAG2_RECONN_AFTER_DISCONNEC)),!(!app_charge_is_powerdown()));
        app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 0);
    }
    
    app_bt_flag2_set(APP_FLAG2_VOL_SYNC,0);
    app_bt_flag2_set(APP_FLAG2_VOL_SYNC_OK,0);

    /* app_bt_flag1_set(APP_FLAG_GET_REMOTE_ADDR,0); */
#if CONFIG_BLUETOOTH_HID
    app_bt_flag1_set(APP_FLAG_HID_CONNECTION,0);
#endif
    if(app_check_bt_mode(BT_MODE_1V2))
    {
        memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)raddr, sizeof(btaddr_t));
        ret = app_env_write_action(&(env_h->env_data.default_btaddr),0);
        if(ret)
        {
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = 0x00;
            msg.hdl = 0;
            msg_lush_put(&msg);
        }
    }
    
    app_bt_reset_policy_iocap();
    
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN))
    {
        app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)sys_hdl->unit, 1000);
        LOG_I(CONN,"app_bt_disconn_wrap, Exit 1, 0x%x \r\n", sys_hdl->flag_sm1);
        return;
    }
    
    if(app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN) && get_Charge_state())
    {
        app_bt_shedule_task((jthread_func)app_set_powerdown_flag, (void*)POWERDOWN_CHG_DEEPSLEEP, 500);
        return;
    }

    if(app_bt_flag1_get (APP_FLAG_DUT_MODE_ENABLE))
    {	
        jtask_stop(sys_hdl->app_reset_task);
        jtask_schedule(sys_hdl->app_reset_task, 1000, (jthread_func)hci_disable_task, (void *)sys_hdl->unit);

        env_h->env_data.offset_bak_8852 = system_get_0x4d_reg()|0x80;
        app_env_write_action(&env_h->env_data.default_btaddr,0);
       
        LOG_I(CONN,"DUT,flag1:0x%x,offset:%d\r\n", sys_hdl->flag_sm1,env_h->env_data.offset_bak_8852);
        return;
    }
    
    if (!hfp_has_sco())
    {
        app_bt_flag1_set((APP_FLAG_HFP_CALLSETUP),0);
        app_bt_flag2_set(APP_FLAG2_CALL_PROCESS,0);
    }
    if (!hci_get_acl_link_count(sys_hdl->unit))
    {
        app_bt_flag1_set(APP_AUDIO_FLAG_SET,0);
    }
    
    if(!hci_get_acl_link_count(sys_hdl->unit))
    {
        if(app_env_check_inquiry_always())
        {
            ;//app_set_led_event_action( LED_EVENT_MATCH_STATE );
        }
        else
        {
        	if(!app_bt_flag1_get(APP_FLAG_MATCH_ENABLE))
                app_set_led_event_action(LED_EVENT_NO_MATCH_NO_CONN_STATE);
        }
    }
    else
    {       
        if(a2dp_has_music())
            app_set_led_event_action(LED_EVENT_BT_PLAY);
        else
            app_set_led_event_action(LED_EVENT_CONN);
    }
}

void app_bt_auto_conn_ok(void)
{
    LOG_I(CONN,"app_bt_auto_conn_ok\r\n");

    app_handle_t sys_hdl = app_get_sys_handler();
    jtask_stop( sys_hdl->app_reset_task );

    if(app_bt_flag1_get (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
    {
        if(hfp_has_sco())
        {
            app_bt_flag1_set(APP_FLAG_HFP_CALLSETUP, 1);
        }
        if(a2dp_has_connection())
        {
        	if(a2dp_has_music()==0){//無播放音乐 yuan++
#if defined MZ_200K                 //cjq++
        		if(SysInf.Lang)
        			app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_8);
        		else
#endif
        			app_wave_file_play_start(APP_WAVE_FILE_ID_CONN);
        	}
        }
        app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
    }

    #if (REMOTE_RANGE_PROMPT == 1)
    UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE); 
    sys_hdl->flag_soft_reset = 0;
    //app_bt_flag2_set(APP_FLAG2_NO_OS_PRINTF,0);
    #endif
    
    app_bt_reset_policy_iocap();
    app_linein_state_switch_complete();
}
void a2dp_time_out_to_scan(btaddr_t* rtaddr)
{
    uint32_t id = bt_app_entity_find_id_from_raddr(rtaddr);
    //if(!bt_app_entity_get_bt_connected_wrap(id))
    if((!bt_app_entity_get_bt_connected_wrap(id))&&(!app_bt_flag2_get(APP_FLAG2_ROLE_SWITCH_FAIL)))
    {
        bt_app_entity_set_bt_connected_wrap(id);
        app_bt_connected_wrap(rtaddr);
        bt_app_entity_set_event_by_addr(rtaddr,SYS_BT_CONNECTED_EVENT);
    }
}
//extern uint32_t avrcp_has_the_connection(btaddr_t);
extern void LSLC_crystal_calibration_set(uint8_t calib);
void app_bt_profile_conn_wrap(uint8_t profileId,btaddr_t* rtaddr)
{

    app_handle_t sys_hdl = app_get_sys_handler();


    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;

    if (app_linein_get_state() == LINEIN_W4_ATTACH)
    {
        LOG_D(LINE,"Linein attach, disconnect BT\r\n");
        bt_all_acl_disconnect(sys_hdl->unit);
        return;
    }

    switch(profileId)
    {
        case PROFILE_BT_HFP:
            set_connection_event(rtaddr, CONN_EVENT_HFP_CONNECTED);
            bt_app_entity_set_conn_flag_by_addr(rtaddr,PROFILE_BT_HFP);
            // clear the bat_lever  backup
            sys_hdl->iphone_bat_lever = 0;
            sys_hdl->iphone_bat_lever_bak = -1;
            //sys_hdl->iphone_bat_lever_bak_cnt = 0;
            if(!hfp_has_the_connection(*rtaddr))
                LOG_W(CONN, "Error flag at HFP\r\n");
            break;
        case PROFILE_BT_A2DP_SNK:
            set_connection_event(rtaddr, CONN_EVENT_SNK_A2DP_CONNECTED);
            bt_app_entity_set_conn_flag_by_addr(rtaddr,PROFILE_BT_A2DP_SNK);
            if(!a2dp_has_the_connection(rtaddr))
                LOG_W(CONN, "Error flag at A2DP\r\n");
            if(app_get_best_offset_level()<=1)
            {
                if(!app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE)
                    && app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION))
                {
                    LSLC_crystal_calibration_set(0);
                }
            }
            break; 
        case PROFILE_BT_AVRCP:
            set_connection_event(rtaddr, CONN_EVENT_AVRCP_CONNECTED);
            bt_app_entity_set_conn_flag_by_addr(rtaddr,PROFILE_BT_AVRCP);
            if(!avrcp_has_the_connection(*rtaddr))
                LOG_W(CONN, "Error flag at AVRCP\r\n");
            break;
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        case PROFILE_BT_A2DP_SRC:
            break;
#endif
#ifdef CONFIG_BLUETOOTH_AGHFP
        case PROFILE_BT_AGHFP:
            break;
#endif
        default:
            break;
    }


    uint8_t idx = bt_app_entity_find_id_from_raddr(rtaddr);
#if defined(CONFIG_BLUETOOTH_HFP) //++ by Borg@230224	yuan++
    if(a2dp_has_the_connection(rtaddr) && hfp_has_the_connection(*rtaddr)&& (!avrcp_has_the_connection(*rtaddr)) )
    {
        jtask_schedule(sys_hdl->app_only_a2dp_hfp_link_to_scan_task, 2000, (jthread_func)a2dp_time_out_to_scan, rtaddr);
    }
    if(a2dp_has_the_connection(rtaddr) && avrcp_has_the_connection(*rtaddr) && (!hfp_has_the_connection(*rtaddr)))
    {
        jtask_schedule(sys_hdl->app_only_a2dp_hfp_link_to_scan_task, 2000, (jthread_func)a2dp_time_out_to_scan, rtaddr);
    }    
    if(a2dp_has_the_connection(rtaddr) && hfp_has_the_connection(*rtaddr) && avrcp_has_the_connection(*rtaddr))
#else
    if(a2dp_has_the_connection(rtaddr) && (!avrcp_has_the_connection(*rtaddr)))
    {
        jtask_schedule(sys_hdl->app_only_a2dp_hfp_link_to_scan_task, 2000, (jthread_func)a2dp_time_out_to_scan, rtaddr);
    }
    if(a2dp_has_the_connection(rtaddr) && avrcp_has_the_connection(*rtaddr))
#endif
    {
        if(!bt_app_entity_get_bt_connected_wrap(idx))
        {
            jtask_stop(sys_hdl->app_only_a2dp_hfp_link_to_scan_task);
            app_bt_connected_wrap(rtaddr);
            bt_app_entity_set_event_by_addr(rtaddr,SYS_BT_CONNECTED_EVENT);
            bt_app_entity_set_bt_connected_wrap(idx);
        }        
        
        #if SDP_PNP_QUERY_ENABLE
        app_env_handle_t env_h = app_env_get_handle();
        extern uint8_t get_pnp_info(btaddr_t* rtaddr);
        if(get_pnp_info(rtaddr) == 0)
        {
            sdp_connect(&env_h->env_cfg.bt_para.device_addr ,rtaddr);
        }
        #elif CONFIG_BLUETOOTH_PBAP
        app_env_handle_t env_h = app_env_get_handle();
        extern uint8_t is_PBAP_ready();
        if(is_PBAP_ready())
        {
            sdp_connect(&env_h->env_cfg.bt_para.device_addr ,rtaddr);
        }else
        {
            LOG_I(PBAP, "pass pbap\r\n");
        }
        #else
        #endif
    }
    judge_role_disconnect(rtaddr);
}

void app_bt_profile_disconn_wrap(uint8_t profileId, void *app_ptr)
{
}

void app_bt_flag1_set(uint32_t flag, uint8_t oper)
{
    app_handle_t sys_hdl;
    uint32_t interrupts_info, mask;

    sys_hdl = app_get_sys_handler();

    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    if(oper == 1)
    {
        sys_hdl->flag_sm1 |= flag;
    }
    else if(oper == 0)
    {
        sys_hdl->flag_sm1 &= ~flag;
    }
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return;
}

int app_bt_flag1_get(uint32_t flag)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    return (sys_hdl->flag_sm1 & flag);
}

void app_bt_flag2_set(uint32_t flag, uint8_t oper)
{
    app_handle_t sys_hdl;
    uint32_t interrupts_info, mask;

    sys_hdl = app_get_sys_handler();

    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    if(oper == 1)
    {
        sys_hdl->flag_sm2 |= flag;
    }
    else if(oper == 0)
    {
        sys_hdl->flag_sm2 &= ~flag;
    }
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return;
}


int app_bt_flag2_get(uint32_t flag)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    return (sys_hdl->flag_sm2 & flag);
}

#if (CONFIG_BT_USER_FLAG3 == 1)
void app_bt_flag3_set(uint32_t flag, uint8_t op)
{
    uint32_t interrupts_info, mask;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    g_user_flag_sm3 = op ? (g_user_flag_sm3 | flag) : (g_user_flag_sm3 & ~flag);
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);

    return;
}


int app_bt_flag3_get(uint32_t flag)
{
    return (g_user_flag_sm3 & flag);
}
#endif
void app_bt_get_remote_addr(btaddr_t *addr)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    btaddr_copy(addr, &sys_hdl->remote_btaddr);
}

void *app_bt_get_handle(uint8_t handle_type)
{
    void *param;
    app_handle_t sys_hdl;

    param = NULL;
    sys_hdl = app_get_sys_handler();
    switch(handle_type)
    {
        case 0:
            param = (void *)sys_hdl->unit;
            break;


        default:
            break;
    }

    //os_printf("%s, sys_hdl->stereo_btaddr: "BTADDR_FORMAT"\r\n", __func__, BTADDR(&sys_hdl->stereo_btaddr));
    return param;
}

void app_bt_reset_policy_iocap(void)
{
#if(CONFIG_AS_SLAVE_ROLE == 0)
    app_handle_t sys_hdl = app_get_sys_handler();

    sys_hdl->unit->hci_link_policy &= ~HCI_LINK_POLICY_ENABLE_ROLE_SWITCH;
#endif
#ifdef CONFIG_BLUETOOTH_SSP
//    if(!app_bt_flag1_get(APP_FLAG_DEBUG_FLAG2))
    {
        bt_sec_set_io_caps(HCI_IO_CAPABILITY_NO_IO);
    }
#endif
}

void app_bt_acl_time_out_wrap(void)
{
    app_handle_t sys_hdl = app_get_sys_handler();

    LOG_I(APP,"app_bt_acl_time_out_wrap\r\n");
    if(sys_hdl->unit != NULL)
    {
        app_bt_flag1_set(APP_AUDIO_FLAG_SET,0);
        jtask_stop(sys_hdl->app_auto_con_task);
        jtask_stop(sys_hdl->app_reset_task);
	    //BK3000_stop_wdt();
	    //aud_volume_mute(1);
        bt_unit_disable(sys_hdl->unit);
        hci_disable_task(sys_hdl->unit);

        Delay(100);
        app_reset();
        app_bt_flag1_set (APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION,1);
    }
}

int app_is_conn_be_accepted(void)
{
    int ret;

    if(app_bt_flag1_get(APP_SDP_DISABLE_FLAG_SET))
    {
        ret = -1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}

result_t app_bt_active_conn_profile(uint8_t profileId, void *arg)
{
    char cmd[40];
    app_handle_t sys_hdl = app_get_sys_handler();

    #ifdef CONFIG_BLUETOOTH_HFP
    app_env_handle_t env_h = app_env_get_handle();
    #endif

    btaddr_t *remote_btaddr = (btaddr_t *)arg;

    if(sys_hdl->unit == NULL)
    {
        return UWE_NODEV;
    }

    if(NULL == remote_btaddr)
    {
        remote_btaddr = &sys_hdl->remote_btaddr;
    }

    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT,
                0,
                remote_btaddr->b[5],
                remote_btaddr->b[4],
                remote_btaddr->b[3],
                remote_btaddr->b[2],
                remote_btaddr->b[1],
                remote_btaddr->b[0]);

    LOG_I(CONN,"active:" BTADDR_FORMAT ",profileId:%d\r\n", BTADDR(remote_btaddr),profileId);

#if A2DP_ROLE_SOURCE_CODE
    if(profileId == PROFILE_ID_A2DP)
    {
        if(get_a2dp_role() == A2DP_ROLE_AS_SRC)
            return a2dpSrcCmdConnect(cmd, sizeof(cmd));
        else if(get_a2dp_role() == A2DP_ROLE_AS_SNK)
            return a2dp_cmd_connect(cmd, sizeof(cmd));
    }

#else

    #ifdef CONFIG_BLUETOOTH_A2DP
        if(profileId == PROFILE_ID_A2DP)
        {
            return a2dp_cmd_connect(cmd, sizeof(cmd));
        }

        else
    #endif

#endif

#ifdef CONFIG_BLUETOOTH_AVRCP
    if(profileId == PROFILE_ID_AVRCP)
        return avrcp_cmd_connect(cmd, sizeof(cmd));
    else
#endif

/*temp assign channel to test, may add sdp sender to get channel for connection sooner*/
#ifdef CONFIG_BLUETOOTH_HFP
    if(profileId == PROFILE_ID_HFP)
    {
        extern uint32_t hfp_is_connecting_based_on_raddr(btaddr_t *);
        if (hfp_is_connecting_based_on_raddr(remote_btaddr))
        {
            LOG_I(CONN,"hfp exit1\r\n");
            return UWE_ALREADY;
        }

        return sdp_connect(&env_h->env_cfg.bt_para.device_addr, remote_btaddr);
    }
    else
#endif

#ifdef CONFIG_BLUETOOTH_HSP
    if(profileId == PROFILE_ID_HSP)
    {
        uint32_t channel = (uint32_t)arg;

        if(channel == 0)
            channel = sys_hdl->hfp_rfcomm_channel - 1;
//        remote_btaddr = &sys_hdl->remote_btaddr;

        memset(cmd, 0 , 40);
        sprintf(cmd,"%u " BTADDR_FORMAT " %u",0, remote_btaddr->b[5],
            remote_btaddr->b[4],remote_btaddr->b[3],
            remote_btaddr->b[2], remote_btaddr->b[1],
            remote_btaddr->b[0], (unsigned int)channel);
        return hs_cmd_connect(cmd, sizeof(cmd));
    }
    else
#endif

#if CONFIG_BLUETOOTH_HID
    if((profileId == PROFILE_ID_HID)
		&& app_env_check_HID_profile_enable()
		)
    {
        return hid_cmd_connect(cmd, sizeof(cmd));
    }
    else
#endif
        return 0;
}

void app_bt_key_store_wrap(btaddr_t *raddr, uint8_t *key, uint8_t key_type)
{
    CLEAR_PWDOWN_TICK;
    CLEAR_SLEEP_TICK;
    if(key_type <= 0x06)   // HCI_LINK_KEY_TYPE_CHANGED_COMBINATION_KEY=6,combination key type used for standard pairing
    {
        app_env_store_autoconn_info(raddr, key);
    }
}

__INLINE__ uint8_t app_led_is_pairing(void)
{
	app_handle_t app_h = app_get_sys_handler();
    APP_LED_EVENT event = app_h->led_event;
    uint8_t scan_status = (uint8_t)LMscan_Read_Scan_Enable();

    if((((event == LED_EVENT_MATCH_STATE) && (scan_status == BOTH_SCANS_ENABLED)) 
         || ((event == LED_EVENT_NO_MATCH_NO_CONN_STATE) && (scan_status == NO_SCANS_ENABLED))) 
         && ((app_h->led_info[event].led_ontime != 0) || (app_h->led_info[event].led_offtime != 0)))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int8_t app_is_available_sleep_system(void)
{
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    if(!app_env_check_sniff_mode_Enable())
        return 0;
    
    return !(app_bt_flag1_get(APP_CPU_NOT_HALT_FLAG_SET) || bt_connection_active() || saradc_get_chnl_busy()
            ||bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_RECONNECTING)
            ||bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_RECONN_FAILED)
			||app_bt_flag2_get(APP_FLAG2_WAKEUP_DLY)||app_bt_flag2_get(APP_FLAG2_HFP_INCOMING)
			||vusb_get_mode()
			||g_ad_da_open_flag
			||get_Charge_state() ||app_led_is_pairing()
            #if CONFIG_ANC_ENABLE
            ||app_anc_status_get()
            #endif
            );
#else
	return 0;
#endif
}

int app_is_not_powerdown(void)
{
    return !(app_bt_flag1_get(APP_FLAG_POWERDOWN));
}


#if (CONFIG_AS_SLAVE_ROLE == 1)
extern void hci_cmd_role_switch(uint8_t role, btaddr_t *raddr);

uint8_t Judge_role(btaddr_t *btaddr,BOOL connecting)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    uint8_t role;
    btaddr_t *bt_rmtaddr = NULL;
    uint8_t times = 0;
    
    
    if(btaddr == NULL)
    {
        bt_rmtaddr = hci_find_acl_slave_link(sys_hdl->unit);
        if((bt_rmtaddr != NULL) && connecting)
        {
            LOG_I(ROLE,"NoAddr,disconn_reconn\r\n");
            bt_app_entity_set_event_by_addr(bt_rmtaddr,SYS_BT_DISCONN_CONN_EVENT);
            bt_app_disconnect_acl_link_by_raddr(bt_rmtaddr);
            app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
            return 0;
        }
    }    
    role = hci_get_acl_link_role(sys_hdl->unit,btaddr);
    if(!app_check_bt_mode(BT_MODE_1V2)) // not 1v2 config
    {
        if(!app_check_bt_mode(BT_MODE_1V1))
        {
            if(role == HCI_ROLE_MASTER) // master
            {
                LOG_I(ROLE,"1V1==rmt addr:%x,"BTADDR_FORMAT"\r\n", BTADDR(btaddr));
                times = bt_app_entity_get_role_switch_times_from_raddr(btaddr);
                if(times>3)
                {
                    bt_app_entity_set_event_by_addr(btaddr,SYS_BT_DISCONN_CONN_EVENT);
                    bt_app_disconnect_acl_link_by_raddr(btaddr);
                    app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
                    return 0;
                }
                hci_cmd_role_switch(HCI_ROLE_SLAVE,btaddr);
                bt_app_entity_set_role_switch_times_from_raddr(btaddr,times+1);
            }
        }
        return 0;
    }
    else              // 1v2 config
    {
        if(hci_get_acl_link_count(sys_hdl->unit) > 1)  // have two links;
        {
            #if 0
            if((role == HCI_ROLE_SLAVE) && connecting) // slave role, disconnect this link and re-connect;
            {
                LOG_I(ROLE,"1V2,disconn_reconn\r\n");
                bt_app_entity_set_event_by_addr(btaddr,SYS_BT_DISCONN_CONN_EVENT);
                bt_app_disconnect_acl_link_by_raddr(btaddr);
                app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
                /*
                if(app_check_bt_mode(BT_MODE_1V1))
                {
                	app_env_handle_t  env_h = app_env_get_handle();
                	btaddr_copy(&env_h->env_data.default_btaddr,&sys_hdl->remote_btaddr);
                	MSG_T msg;
                	msg.id = MSG_ENV_WRITE_ACTION;
                	msg.arg = 0x01;
                	msg.hdl = 0;
                	msg_lush_put(&msg);
                }*/
                return 1;
            }
            else        // check all links, once has one's link is slave role, then send hci cmd role swtich;
            {
                bt_rmtaddr = hci_find_acl_slave_link(sys_hdl->unit);
                if(bt_rmtaddr != NULL)
                {
                    // hci cmd role switch;
                    LOG_I(ROLE,"===rmt addr:%x,"BTADDR_FORMAT"\r\n", BTADDR(bt_rmtaddr));
                    times = bt_app_entity_get_role_switch_times_from_raddr(bt_rmtaddr);
                    if(times>3)
                    {
                        bt_app_entity_set_event_by_addr(bt_rmtaddr,SYS_BT_DISCONN_CONN_EVENT);
                        bt_app_disconnect_acl_link_by_raddr(bt_rmtaddr);
                        app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
                        return 0;
                    }
                    hci_cmd_role_switch(HCI_ROLE_MASTER,bt_rmtaddr);
                    bt_app_entity_set_role_switch_times_from_raddr(bt_rmtaddr,times+1);
                }
                return 0;
            }
            #else
            bt_rmtaddr = hci_find_acl_slave_link(sys_hdl->unit);
            
            if((bt_rmtaddr != NULL) && connecting)
            {
                LOG_I(ROLE,"disconn_reconn\r\n");
                bt_app_entity_set_event_by_addr(bt_rmtaddr,SYS_BT_DISCONN_CONN_EVENT);
                bt_app_disconnect_acl_link_by_raddr(bt_rmtaddr);
                app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);  
            }
            return 0;
            #endif
        }
        else                                          // only one link;
        {
            #if 0
            if(role == HCI_ROLE_MASTER) // master
            {
                LOG_I(ROLE,"~~~rmt addr:%x,"BTADDR_FORMAT"\r\n", BTADDR(btaddr));
                times = bt_app_entity_get_role_switch_times_from_raddr(btaddr);
                if(times>3)
                {
                    bt_app_entity_set_event_by_addr(btaddr,SYS_BT_DISCONN_CONN_EVENT);
                    bt_app_disconnect_acl_link_by_raddr(btaddr);
                    app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
                    return 0;
                }
                hci_cmd_role_switch(HCI_ROLE_SLAVE,btaddr);
                bt_app_entity_set_role_switch_times_from_raddr(btaddr,times+1);
            }
            #endif
            return 0;

        }

    }
}
#endif
#if (CONFIG_AS_SLAVE_ROLE == 0)
//uint8_t Judge_role(void)
uint8_t Judge_role(btaddr_t *btaddr,BOOL connecting)
{
    return 0;
    if(1 == Remote_BlueDev.init_flag)
    {
        app_handle_t sys_hdl = app_get_sys_handler();
        LOG_I(ROLE,"=======Judge_role:%x========\r\n",Remote_BlueDev.init_flag);
        bt_unit_acl_disconnect(sys_hdl->unit, &(Remote_BlueDev.btaddr_def));
        app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_DISCONNEC, 1);
        if(app_check_bt_mode(BT_MODE_1V1))
        {
            app_env_handle_t  env_h = app_env_get_handle();
            btaddr_copy(&env_h->env_data.default_btaddr,&sys_hdl->remote_btaddr);
            MSG_T msg;
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = 0x01;
            msg.hdl = 0;
            msg_lush_put(&msg);
        }
        return 1;
    }
    return 0;
}
#endif
void judge_role_disconnect(btaddr_t* rtaddr)
{
    app_bt_flag2_set(APP_FLAG2_ROLE_SWITCH_FAIL,0);
    if(app_check_bt_mode(BT_MODE_1V1))
        return;
    #if (CONFIG_AS_SLAVE_ROLE == 0)
    extern uint8_t sdp_server_state;
    app_handle_t sys_hdl = app_get_sys_handler();
    uint8_t role;
    role = hci_get_acl_link_role(sys_hdl->unit,rtaddr);
    if(sdp_server_state==0)
    {
        if(role == HCI_ROLE_SLAVE)   // self slave
        {
            LOG_I(ROLE,"role slave,disconnected\r\n");
            jtask_stop(sys_hdl->app_only_a2dp_hfp_link_to_scan_task);
            app_bt_flag2_set(APP_FLAG2_ROLE_SWITCH_FAIL,1);
            bt_app_entity_set_event_by_addr(rtaddr,SYS_BT_DISCONN_CONN_EVENT);
            bt_app_disconnect_acl_link_by_raddr(rtaddr);
            return;
        }
    }
    else
    {
        if(role == HCI_ROLE_SLAVE)   // self slave
            app_bt_shedule_task((jthread_func)judge_role_disconnect, rtaddr, 200);
    }
    #else
    Judge_role(rtaddr,TRUE);
    #endif
}
void Set_remoteDev_role_btaddr(btaddr_t *btaddr,uint32_t role)
{
	memcpy((uint8_t*)&(Remote_BlueDev.btaddr_def),(uint8_t*)btaddr,sizeof(btaddr_t));
	Remote_BlueDev.init_flag = role;
	LOG_I(ROLE,"===set remote devaddr role:role=%x=====\r\n",Remote_BlueDev.init_flag);
}


void bt_all_acl_disconnect(hci_unit_t *unit)
{
    btaddr_t *raddr[2];
    int i,count = 0;
    count = hci_get_acl_link_addr(unit,raddr);
    for(i=0;i<count;i++)
    {
        bt_unit_acl_disconnect(unit,(const btaddr_t *)raddr[i]);
        Delay(100);
    }
}

void bt_exchange_hf_active_by_handle(uint16_t handle)
{
    uint8_t tc_cmd[7] = {0x01,0xe0,0xfc,0x03,0x84,0x00,0x00};
    tc_cmd[5] =  (uint8_t)(handle & 0xff);
    tc_cmd[6] =  (uint8_t)((handle>>8) & 0xff);
    uart_send_poll(tc_cmd,sizeof(tc_cmd));
}

#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
/* Link flow clrl by handle,where:
 * <flow_ctrl> = 0: no flow control;
 * <flow_ctrl> = 1(bit0): Controller flow control,saved air bandwith;
 * <flow_ctrl> = 2(bit1): Host layer flow control,saved sbc node buffer;
 */
void bt_set_ag_flow_ctrl_by_handle(hci_link_t *link)
{
    uint8_t tc_cmd[8] = {0x01,0xe0,0xfc,0x04,0x83,0x00,0x00,0x01};
    tc_cmd[5] =  (uint8_t)(link->hl_handle & 0xff);
    tc_cmd[6] =  (uint8_t)((link->hl_handle>>8) & 0xff);
    tc_cmd[7] = (link->flow_ctrl & 0x03);
    uart_send_poll(tc_cmd,sizeof(tc_cmd));
    jtask_schedule(link->hl_expire, 500, bt_set_ag_flow_ctrl_timeout, link);
}
void bt_set_ag_flow_ctrl_timeout(void *arg)
{
    hci_link_t *link = (hci_link_t *)arg;
    bt_set_ag_flow_ctrl_by_handle(link);
}
#endif




void app_bt_pinkey_missing(btaddr_t* addr)
{
    app_env_handle_t  env_h = app_env_get_handle();
    LOG_I(APP,"pin or key missing!Won't reconnect again!\r\n");
    app_env_keypair_used_delete(addr);
    memcpy( (uint8_t *)&env_h->env_data.default_btaddr, (uint8_t *)addr, sizeof(btaddr_t));
    MSG_T msg;
    if(!bt_connection_active())
    {
        int ret = 0;
        ret = app_env_write_action(&(env_h->env_data.default_btaddr),2); // 2 means write flash whatever
        if(ret)
        {
            msg.id = MSG_ENV_WRITE_ACTION;
            msg.arg = 0x00;
            msg.hdl = 0;
            msg_lush_put(&msg);
        }
    }
}


void app_low_power_scanning(uint32_t step )
{
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    /**************** Battery level detect ******************/
    app_h->low_detect_count += step;
    if((app_h->low_detect_count >= 200) // Min:>2s
        && (app_h->low_detect_count >= env_h->env_cfg.system_para.lp_interval>>2)
        && !app_wave_playing ()
        )
    {
        app_h->low_detect_count = 0;
    #if (CONFIG_TEMPERATURE_NTC == 1)
        if(app_charge_is_powerdown())
        {
            msg_put(MSG_TEMPRATURE_NTC_DETECT);
        }
        else
    #endif
        {
            msg_put(MSG_LOWPOWER_DETECT);
        }
    }
}
#if (CONFIG_TEMPERATURE_DETECT == 1)
void app_temprature_scanning()
{
    static uint64_t s_temperature_timer_cnt = 0;
    uint64_t t1;

    t1 = os_get_tick_counter();
    if((t1-s_temperature_timer_cnt) >= 1000)  // 10s
    {
        s_temperature_timer_cnt = t1;
        msg_put(MSG_TEMPERATURE_DETECT);
    }
}
#endif
//void set_iphone_bat_level_result(void)
//{
//    app_handle_t app_h = app_get_sys_handler();
//    app_h->iphone_bat_lever_bak = app_h->iphone_bat_lever;
//}

#if 0	//yuan++
void app_low_power_detect(void )
{
    app_handle_t app_h = app_get_sys_handler();
    uint8_t lowpower_flag = 0;

    //app_sleep_func(0);
   
    if(saradc_get_chnl_busy()|| app_bt_flag1_get(APP_FLAG_POWERDOWN))  
        return;

    //app_sleep_func(0);
    if(app_env_check_bat_display())
    {
        if(hfp_has_connection())
        {
            app_h->iphone_bat_lever = set_iphone_bat_lever();
            if(app_h->iphone_bat_lever != app_h->iphone_bat_lever_bak)
            {
                //if we don't rev AT_RESULT_OK, send it three times per 2s
                //if (++iphone_bat_lever_bak_cnt >= 3)
                {
                    //app_h->iphone_bat_lever_bak_cnt = 0;
                    app_h->iphone_bat_lever_bak = app_h->iphone_bat_lever;
                }
                //os_printf("iphone_bat_lever:%d\r\n",iphone_bat_lever);
                hf_cmd_set_iphone_batlevel(app_h->iphone_bat_lever);
            }
        }
    }

    lowpower_flag = saradc_lowpower_status();
    //os_printf("lf:%d,adc:%d,mv:%d\r\n",lowpower_flag,saradc_get_value(),((saradc_get_value()*4600)>>10));
    if( !app_bt_flag1_get( APP_FLAG_LOWPOWER) && lowpower_flag )
    {
        app_bt_flag1_set(APP_FLAG_LOWPOWER,1);

        if (app_get_led_low_battery_index() == LED_EVENT_END)
        {
			app_set_led_event_action(LED_EVENT_LOW_BATTERY);
	        app_bt_flag2_set(APP_FLAG2_LED_LOWPOWER, 1);
        }
        else if (get_Charge_state()==0)
        {
			app_set_led_low_battery_all_in(0);
        }
    }
    else if(app_bt_flag1_get(APP_FLAG_LOWPOWER) && saradc_normalpower_status() )
    {
        app_bt_flag1_set(APP_FLAG_LOWPOWER,0);
        app_bt_flag2_set(APP_FLAG2_LED_LOWPOWER, 0);
    	if (get_Charge_state() == 0)
        	app_set_led_low_battery_all_in(1);
        app_set_led_event_action(app_h->led_event_save);
    }

    if( app_bt_flag1_get(APP_FLAG_LOWPOWER ))
    {
        if( lowpower_flag == 2 )
        {
            lowpower_flag=0;
            LOG_I(APP,"low power,power down\r\n");
            /*powerdown and disable the saradc*/
            REG_SYSTEM_0x05 |= MSK_SYSTEM_0x05_SADC_PWD;
            start_wave_and_action(APP_WAVE_FILE_ID_POWEROFF, app_powerdown);

            return;
        }

        if(lowpower_flag==1)
        {
			if(!app_bt_flag2_get(APP_FLAG2_LED_LOWPOWER)
				&&(app_get_led_low_battery_index()==LED_EVENT_END)
				)
			{
				app_bt_flag2_set(APP_FLAG2_LED_LOWPOWER, 1);
		        app_set_led_event_action(LED_EVENT_LOW_BATTERY);
			}
			saradc_reset();
            switch(app_h->sys_work_mode)
            {
#if (CONFIG_APP_MP3PLAYER == 1)
            case    SYS_WM_SDCARD_MODE:
                app_playwav_resumeMp3(APP_WAVE_FILE_ID_LOW_BATTERY);
                break;
#endif
            case    XZX_S10_LINEIN_MODE:
                app_playwav_resumelinein(APP_WAVE_FILE_ID_LOW_BATTERY);
                break;
            case    SYS_WM_BT_MODE:
                app_wave_file_play_start(APP_WAVE_FILE_ID_LOW_BATTERY);
                break;

            default:
                break;
            }
            LOG_I(APP,"voice play low power!!!,lowpower_flag=%d\r\n",lowpower_flag);
        }
    }
    SARADC_CRITICAL_CODE(1);
    saradc_init(SARADC_MODE_CONTINUE, app_h->low_detect_channel, 4);
    saradc_set_chnl_busy(1);
    SARADC_CRITICAL_CODE(0);
}
#endif


void app_audio_restore(void)
{
    uint32_t freq;
    uint32_t channel;
    uint32_t vol_s;
    app_handle_t app_h = app_get_sys_handler();
  
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN|APP_FLAG_WAVE_PLAYING))
        return;
    app_get_wave_info(&freq, &channel, &vol_s);
    if(app_bt_flag1_get(APP_FLAG_LINEIN))
    {
        //app_handle_t app_h = app_get_sys_handler();
        if (!app_bt_flag2_get(AP_FLAG2_LINEIN_MUTE))
        {
            aud_dac_close();
            linein_audio_open();

            if(AUDIO_VOLUME_MIN == app_h->linein_vol)
            aud_volume_mute(1);
        }

        return;
    }
    else if(app_is_bt_mode())
    {
    	if(!a2dp_has_music() && !hfp_has_sco())
        {
            aud_dac_close();
            aud_volume_mute(1);
        }
        else
        {
            /*
            Because both freq and chnanel are parameters of promt wave,this restore is wrong;
            and there are much delay in function aud_dac_config(),ADC isr can't be processed by CPU in time
            so MSG pool will be full,and printf("F").
            */
            /*
            Fixed Bug#1385
            Both freq and chnanel have been saved in function a2dp_audio_restore,
            so these parameters are a2dp config param
            */
            if(a2dp_has_music() && !hfp_has_sco())
            {
                a2dp_audio_action();
            }
            else
            {
                aud_dac_set_volume(vol_s );
            }

            if(vol_s == AUDIO_VOLUME_MIN )
            {
                aud_volume_mute(1);
            }

            if(hfp_has_sco())
            {
                hf_audio_restore();
            }

            if(hfp_has_sco() || app_bt_flag1_get(APP_FLAG_CALL_ESTABLISHED|APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE))
                aud_PAmute_delay_operation(0);

            if((app_h->mic_volume_store&0x80) && hfp_has_sco())
            {
#ifdef CONFIG_DRIVER_ADC
                aud_mic_mute(1);
#endif
            }
        }
    }
#if (CONFIG_APP_MP3PLAYER == 1) && (!defined(AUD_WAV_TONE_SEPARATE))
    else if(app_is_mp3_mode())
    {
        extern uint8_t mp3_need_pause;
        if(mp3_need_pause)
            app_player_button_play_pause();
        mp3_need_pause = 0;
    }
#endif
}

void enter_match_state(void)
{
     app_button_match_action();
}

uint8_t app_set_role_accept_con_req(hci_unit_t *unit, const btaddr_t *btaddr)
{
#if (CONFIG_AS_SLAVE_ROLE == 1)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    if(!app_check_bt_mode(BT_MODE_1V2))
        return HCI_ROLE_SLAVE;
    else
    {
        if(hci_get_acl_link_count(sys_hdl->unit) < 1)
            return HCI_ROLE_SLAVE;
        else
            return HCI_ROLE_MASTER;
    }

}
#endif
#if (CONFIG_AS_SLAVE_ROLE == 0)
		if(app_check_bt_mode(BT_MODE_TWS|BT_MODE_DM_TWS))
		{
			if( btaddr_same(btaddr, app_bt_get_handle(2)) )
			{
				return HCI_ROLE_SLAVE;
			}
			else
				return HCI_ROLE_MASTER;
		}
		///else if(app_check_bt_mode(BT_MODE_1V2|BT_MODE_1V1)&&(BT_ONE2MULTIPLE_AS_SCATTERNET == 0))
		else if(app_check_bt_mode(BT_MODE_1V2))
        {
            //return HCI_ROLE_MASTER; // HCI_ROLE_MASTER;
            if(bt_acl_con_get_specific_uclass())
                return HCI_ROLE_SLAVE;
            else
                return HCI_ROLE_MASTER;
        }
        else if(app_check_bt_mode(BT_MODE_1V1))
		{
            if(app_env_check_Use_ext_PA())
            {
                if(unit->hci_link_policy & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)
    	    		return HCI_ROLE_MASTER;
    		    else
    	    		return HCI_ROLE_SLAVE;
            }
            else
            {
                #if (BT_ONE2MULTIPLE_AS_SCATTERNET == 0)
    			    return HCI_ROLE_MASTER; // HCI_ROLE_MASTER;
    			#else
    			    if(unit->hci_link_policy & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)
    	    			return HCI_ROLE_MASTER;
    				else
    	    			return HCI_ROLE_SLAVE;
    			#endif
            }
		}
		else
		{
			if(unit->hci_link_policy & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)
				return HCI_ROLE_MASTER;
			else
				return HCI_ROLE_SLAVE;
		}
#endif

}
// 1=accept_role_switch;
uint8_t app_set_role_creat_con_req(hci_unit_t *unit, const btaddr_t *btaddr)
{
#if (CONFIG_AS_SLAVE_ROLE == 1)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    if(!app_check_bt_mode(BT_MODE_1V2))
        return 1;
    else
    {
        if(hci_get_acl_link_count(sys_hdl->unit) < 1)
            return 1;
        else
            return 0;
    }
}
#endif
#if (CONFIG_AS_SLAVE_ROLE == 0)
    if(app_check_bt_mode(BT_MODE_1V2))
    {
        return 0;
    }
    else if(app_check_bt_mode(BT_MODE_1V1))
    {
        if(app_env_check_Use_ext_PA())
        {
                return (unit->hci_link_policy & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)?1:0;
        }
        else
        {
            #if (BT_ONE2MULTIPLE_AS_SCATTERNET == 0)
    		    return 0;
    		#else
                return (unit->hci_link_policy & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)?1:0;
    		#endif
        }
    }
    else
        return (unit->hci_link_policy & HCI_LINK_POLICY_ENABLE_ROLE_SWITCH)?1:0;
#endif



}
uint8_t app_get_as_slave_role(void)
{
#if (CONFIG_AS_SLAVE_ROLE == 1)
	return 1;	
#else
	return 0;	
#endif
}
void app_bt_confail_reset_sacn(void)
{
    app_handle_t sys_hdl = app_get_sys_handler(); 
    LOG_I(CONN,"reset.sacn:%d\r\n",hci_get_acl_link_count(sys_hdl->unit));
    if(app_check_bt_mode(BT_MODE_1V2))
    {
        if(hci_get_acl_link_count(sys_hdl->unit) < 2 )
        {   
            bt_unit_set_scan_enable(app_bt_get_handle(0), HCI_PAGE_SCAN_ENABLE|HCI_INQUIRY_SCAN_ENABLE);
        }
    }
}

uint8_t app_bt_a2dp_sbc_default_bitpool(void)
{
	return 53;
}

uint8_t app_bt_rmt_device_type_get(uint8_t id)
{
    uint8_t mac_win_book = 0;
    btaddr_t *addr = bt_app_entit_get_rmt_addr(id);

    if(id == 0xff)
        return BT_DEVICE_NONE;

    mac_win_book = bt_app_entity_get_mac_win_book(id);
    if(mac_win_book)
        return BT_DEVICE_COMPUTER;

    if(a2dp_has_the_connection(addr) && hfp_has_the_connection(*addr) && avrcp_has_the_connection(*addr))
        return BT_DEVICE_PHONE;
    else if(a2dp_has_the_connection(addr) && !hfp_has_the_connection(*addr) && !avrcp_has_the_connection(*addr))
        return BT_DEVICE_TV;
    else
        return BT_DEVICE_OTHER;
}

#if (REMOTE_RANGE_PROMPT == 1)
uint8_t flag_remote_range_1 = 0;
uint8_t flag_remote_range_2 = 0;
volatile uint8_t g_device_index=0;
//extern uint8_t LMPrssi_Get_Device_Link_Quality(t_deviceIndex device_index);
extern char LMPrssi_Get_Device_RSSI(t_deviceIndex device_index);
// g_device_index was set directly in controller
void app_remote_set_device_index(uint8_t index)
{
	g_device_index = index; 
}

uint8_t app_remote_get_device_index(void)
{
	return g_device_index;
}

void app_remote_range_judge(void)
{
    app_handle_t sys_hdl = app_get_sys_handler(); 
    //uint8 link_quality;
    char rssi_1 = 0, rssi_2 = 0;;
    static uint8_t remote_detect_cnt1 = 0,remote_detect_cnt2 = 0;
    static uint8_t flag_clr_detect_cnt1 = 0,flag_clr_detect_cnt2 = 0;

    if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || app_charge_is_powerdown())
    {
        return ;
    }

    if(a2dp_has_connection()||hfp_has_connection())
    {
        
        {
            // remote_detect_cnt1 = 0;
            if(hci_get_acl_link_count(sys_hdl->unit) > 1)
            {
			    rssi_1 = LMPrssi_Get_Device_RSSI(1); 
			    rssi_2 = LMPrssi_Get_Device_RSSI(2); 
    			if(rssi_2 < -85)
                {
                    flag_clr_detect_cnt2 = 0;
    				if((++remote_detect_cnt2 > 4) && !flag_remote_range_2)
                        flag_remote_range_2 = 1;
                }
                else
                {
                    flag_clr_detect_cnt2 ++;
                    if(flag_clr_detect_cnt2 >4)
                    {
                        flag_clr_detect_cnt2 = 0;
                        flag_remote_range_2 = 0;
                    }
                }
			}
			else
			{
                rssi_1 = LMPrssi_Get_Device_RSSI(app_remote_get_device_index()); 
			}
            ///os_printf("rssi:%d,%d\r\n",rssi_1,rssi_2);

            if(rssi_1 < -85)
            {
                flag_clr_detect_cnt1 = 0;
				if((++remote_detect_cnt1 > 4) && !flag_remote_range_1)
                    flag_remote_range_1 = 1;
            }
            else
            {
                flag_clr_detect_cnt1 ++;
                if(flag_clr_detect_cnt1 >4)
                {
                    flag_clr_detect_cnt1 = 0;
                    flag_remote_range_1 = 0;
                }
            }
        }
    }
    else
    {
        flag_clr_detect_cnt1 = 0;
        flag_clr_detect_cnt2 = 0;
        remote_detect_cnt1 = 0;
        remote_detect_cnt2 = 0;
		flag_remote_range_1 = 0;
		flag_remote_range_2 = 0;
    }
}

void app_remote_range_wave_play(uint16_t reason,uint8_t sel)
{
    app_handle_t sys_hdl = app_get_sys_handler();
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || app_charge_is_powerdown())
    {
        return ;
    }
	sys_hdl->flag_soft_reset = 0;
	
    os_printf("remote range:%d,%d,%d\n",flag_remote_range_1,flag_remote_range_2,sel);
    if((flag_remote_range_1||flag_remote_range_2)
		/*&& (reason != HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION)*/
        && ((reason == HCI_ERR_CONNECTION_TIMEOUT)||(reason == HCI_ERR_LMP_RESPONSE_TIMEOUT))
    )
    {        
       SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);
#if defined MZ_200K                 //cjq++
       if(SysInf.Lang)
        app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_7);
       else
#endif
       app_wave_file_play_start(APP_WAVE_FILE_ID_DISCONN);
    }
    else if (sel)
    {
        if(flag_remote_range_1||flag_remote_range_2)
            SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);
    }
	if(flag_remote_range_1)    flag_remote_range_1 = 0;
	if(flag_remote_range_2)    flag_remote_range_2 = 0;
}
void app_get_hci_discon_reason(uint8_t reason)
{
    #if 1
    INFO_PRT("hci reason:%d,%d\r\n",reason,flag_remote_range_1,flag_remote_range_2);
    if(((!flag_remote_range_1&&!flag_remote_range_2) 
		/*&& (reason != HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION)*/
        && ((reason == HCI_ERR_CONNECTION_TIMEOUT)||(reason == HCI_ERR_LMP_RESPONSE_TIMEOUT))
        && (!app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION)))
        )
    {
        //app_bt_flag2_set(APP_FLAG2_NO_OS_PRINTF,1);
    }
    #endif
}

#endif
void app_hci_discon_compl_cb(uint8_t reason)
{
#if (REMOTE_RANGE_PROMPT == 1)
    app_get_hci_discon_reason(reason);
#endif
}

uint8 app_bt_get_phone_type(void)
{
    app_handle_t app_h = app_get_sys_handler();
    // 1:android    2:iphone
    if(DEVICE_IPHONE == app_h->phone_type)
            return DEVICE_IPHONE;
    else if(DEVICE_ANDROID == app_h->phone_type)
            return 1;
	return DEVICE_NONE;
}
//EOF
