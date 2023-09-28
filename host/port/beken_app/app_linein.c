#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "karacmd.h"  //cjq++
uint32_t pingpong = 0;
static linein_cfg_t linein_cfg;

void playwav_resumelinein(uint32_t fieldId);
#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
extern uint8_t syspwr_cpu_halt;
#endif


#if A2DP_ROLE_SOURCE_CODE

extern uint8_t get_a2dp_role(void);
extern void appStartAuxTx(void *arg);
extern BOOL isA2dpSrcConnected(void);
extern void close_linein2aux_loop(void);
extern void open_linein2aux_loop(void);
extern result_t a2dpSrcCmdStreamSuspend(void);
extern void appExitAuxTx(void);

#endif

void linein_clean_current_system_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    uint32_t mode = app_h->sys_work_mode;
    LOG_D(LINEIN,"linein_clean_current_system_mode\r\n");

    app_sleep_func(0);
    CLEAR_PWDOWN_TICK;

    if(SYS_WM_BT_MODE == mode)
    {
        jtask_stop( app_h->app_auto_con_task );
        jtask_stop( app_h->app_reset_task );

        app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION,0);

        if(!app_bt_flag1_get(APP_FLAG_AVCRP_PROCESSING))
        {
            app_button_playpause_action_caller(0);
            Delay(1000);
            if(app_bt_flag1_get(APP_FLAG_ACL_CONNECTION) )
            {
                app_button_conn_disconn_caller();
            }
        }
        app_bt_flag1_set(APP_AUDIO_FLAG_SET,0);
    }

    Delay(4000);
    linein_audio_open();

    app_bt_flag1_set(APP_FLAG_LINEIN,1);
}

void linein_restore_current_system_mode(void)
{
    app_handle_t app_h = app_get_sys_handler();
    uint32_t mode = app_h->sys_work_mode;
    LOG_D(LINEIN,"linein_restore_current_system_mode, %d \r\n", mode);

    /*linein will impact button configuration and audio setting*/
    switch(mode)
    {
    case SYS_WM_BT_MODE:
    {

        #if A2DP_ROLE_SOURCE_CODE

        if(isA2dpSrcConnected()) {
            a2dpSrcCmdStreamSuspend();
        }

        #else

        app_bt_flag1_set(APP_AUDIO_FLAG_SET,0);

        #endif

        jtask_stop(app_h->app_auto_con_task);
        jtask_stop(app_h->app_reset_task);
        aud_volume_mute(1);
        CLEAR_PWDOWN_TICK;
        CLEAR_SLEEP_TICK;
        app_bt_flag1_set(APP_FLAG_LINEIN,0);
        app_bt_flag2_set( APP_FLAG2_LED_LINEIN_INIT , 0  );
        BK3000_Ana_Line_enable(0);

    #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        #if ( BUTTON_DETECT_IN_SNIFF == 1)
            SYSpwr_Wakeup_From_Sleep_Mode();
            sniffmode_wakeup_dly = 1000;
            sniff_enable_timer0_pt0();
        #endif
    #endif

        app_bt_flag1_set( APP_FLAG_AUTO_CONNECTION,1);
        bt_connection_req(&app_h->remote_btaddr);
#if defined MZ_200K                 //cjq++
       if(SysInf.Lang)
        app_wave_file_play_start(APP_WAVE_FILE_ID_VOICE_NUM_6);
       else
#endif
        app_wave_file_play_start(APP_WAVE_FILE_ID_BT_MODE);

    }
        //linein_audio_close();
        app_bt_button_setting();
        LINEIN_PRT("SYS_WM_BT_MODE\r\n");
        break;

#ifdef CONFIG_APP_UDISK
    case SYS_WM_UDISK_MODE:
#endif

#if (CONFIG_APP_MP3PLAYER == 1)
    case SYS_WM_SDCARD_MODE:
        linein_audio_close();
        app_player_button_setting();

        if (player_get_play_status())
        {
            app_player_play_pause_caller(0);
            app_player_play_pause_caller(1);
        }

        LINEIN_PRT("MP3_MODE or UDISK_MODE\r\n");
        break;
#endif
    default:
        LINEIN_PRT("linein_restore_context_exceptional\r\n");
        break;
    }

    app_bt_flag1_set(APP_FLAG_LINEIN,0);

    return;
}

void linein_audio_open(void)
{
    app_handle_t app_h = app_get_sys_handler();
//    LTX_PRT("linein_audio_open\r\n");

    aud_PAmute_delay_operation(1);

    BK3000_Ana_Line_enable(1);
//yuan    aud_dac_set_volume(app_h->linein_vol); // (player_vol_linein);
    if ( app_h->linein_vol )
    {
        aud_volume_mute(0);
        aud_PAmute_delay_operation(0);
    }
}

void linein_audio_close(void)
{
//    LTX_PRT("linein_audio_close\r\n");
    BK3000_Ana_Line_enable(0);
}

void linein_detect(void)
{
    /* app_env_handle_t env_h = app_env_get_handle(); */
    uint32_t linein_gpio_status;
#if (CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
    //app_handle_t app_h = app_get_sys_handler();
    //static uint8_t linein_det_flag = 0;
#endif
    if(linein_cfg.enable)
    {
        linein_gpio_status = gpio_input(linein_cfg.pin);
        if(linein_cfg.high_flag == linein_gpio_status)
        {
            linein_cfg.plugin_count++;
            linein_cfg.pullout_count = 0;
            CLEAR_PWDOWN_TICK;
            CLEAR_SLEEP_TICK;
        #if (CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
            uint8_t idx = 0;
            if(app_is_available_sleep_system() && syspwr_cpu_halt)
            {
            #if ( BUTTON_DETECT_IN_SNIFF == 1)
                SYSpwr_Wakeup_From_Sleep_Mode();
                sniffmode_wakeup_dly = 1000;
                sniff_enable_timer0_pt0();
            #endif
            }

            if(linein_cfg.plugin_count > (LINEIN_GPIO_DEBOUNCE_COUNT/10)&&( !app_bt_flag1_get( APP_FLAG_LINEIN )))
	    {
                for(idx=0;idx<NEED_SNIFF_DEVICE_COUNT;idx++)
                {
                    if(bt_sniff_is_used(idx))
                    {
                        btaddr_t raddr = bt_sniff_get_rtaddr_from_idx(idx);
                        if(a2dp_has_the_connection(&raddr) || hfp_has_the_connection(raddr))
                        {
                            if(bt_sniff_is_policy(idx))
                            {
                                LOG_I(LINEIN,"exit sniff:%d\r\n",idx);
                                app_bt_write_sniff_link_policy(bt_sniff_get_handle_from_idx(idx), 0);
                            }
                        }
                    }
                }
                syspwr_cpu_halt = 0;
                //linein_det_flag = 1;
	        }
        #endif
        }
        else
        {
            linein_cfg.pullout_count++;
            linein_cfg.plugin_count = 0;        
        }

        if((LINEIN_STATUS_OFF == linein_cfg.status)
           && (linein_cfg.plugin_count > LINEIN_GPIO_DEBOUNCE_COUNT))
        {
        #if (CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE)
            //linein_det_flag = 0;
        #endif
            linein_cfg.status = LINEIN_STATUS_ON;
            app_linein_set_state(LINEIN_W4_ATTACH);

            //msg_put(MSG_LINEIN_ATTACH);
        }

        if((LINEIN_STATUS_ON == linein_cfg.status)
           && (linein_cfg.pullout_count > LINEIN_GPIO_DEBOUNCE_COUNT))
        {
            linein_cfg.status = LINEIN_STATUS_OFF;
            app_linein_set_state(LINEIN_W4_DETACH);

            //msg_put(MSG_LINEIN_DETACH);
        }
    }
}

int app_linein_powerup_check(void)
{
    int ret = 0;
    uint32_t linein_gpio_status;
    /* app_env_handle_t env_h = app_env_get_handle(); */

    if(linein_cfg.enable)
    {
        linein_gpio_status = gpio_input(linein_cfg.pin);
        if(linein_cfg.high_flag == linein_gpio_status)
        {
            ret = 1;
        }
    }

    return ret;
}

#ifdef CONFIG_IRDA
CONST static button_action_func IRKey_LineIn_handler[IR_KEY_END]=
{
        app_linein_mute_unmute,
        app_linein_vol_down,
        app_linein_vol_up,
        NULL,
        NULL,
        system_work_mode_change_button,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};
#endif

#ifdef LINEIN_BUTTON_CONFIG
enum
    {
        BUTTON_LINEIN_NONE = 0,
        BUTTON_LINEIN_MUTE_UNMUTE,
        BUTTON_LINEIN_VOL_UP,
        BUTTON_LINEIN_VOL_DOWN,
        BUTTON_LINEIN_END
    };
CONST static uint64_t linein_button_code[BUTTON_LINEIN_END] =
{
        0,
        BTN_CODE(BUTTON_CLICK, GPIO10, BUTTON_TYPE_NON_HFP),
        BTN_CODE(BUTTON_CONTINUE | BUTTON_LONG_PRESS, GPIO6, BUTTON_TYPE_NON_HFP),
        BTN_CODE(BUTTON_CONTINUE | BUTTON_LONG_PRESS, GPIO23, BUTTON_TYPE_NON_HFP),
};

CONST static button_action_func linein_button_handler[BUTTON_LINEIN_END] =
{
        NULL,
        app_linein_mute_unmute,
        app_linein_vol_up,
        app_linein_vol_down
};

void linein_button_setting(void)
{
#ifdef CONFIG_IRDA
    button_set_irkey_handler(IRKey_LineIn_handler);
#endif

    button_common_register(linein_button_code,
                           linein_button_handler,
                           BUTTON_LINEIN_END );
}

#endif

void app_linein_init(void)
{
    app_env_handle_t env_h = NULL;

    memset(&linein_cfg, 0, sizeof(linein_cfg_t));

    env_h = app_env_get_handle();
    if(env_h->env_cfg.used == 0x01)
    {
        linein_cfg.enable = app_env_get_pin_enable(PIN_lineDet);
        linein_cfg.high_flag = app_env_get_pin_valid_level(PIN_lineDet);
        linein_cfg.pin = app_env_get_pin_num(PIN_lineDet);
    #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
        sniffmode_wakeup_dly = 1000;
    #endif
    }
    else
    {
        linein_cfg.enable = 0;
        linein_cfg.high_flag = LINEIN_GPIO_PLUG_IN;
        linein_cfg.pin = LINEIN_GPIO_DETECT_ID;
    }

    linein_cfg.detect_func= linein_detect;

    if(linein_cfg.enable)
        gpio_config(linein_cfg.pin, linein_cfg.high_flag?0:3);

    linein_cfg.status = LINEIN_STATUS_OFF;
    /* When power on, line in detect status */
    if(linein_cfg.enable)
    {
        LOG_I(LINEIN,"===line init stat:%d,%d\r\n",linein_cfg.high_flag,gpio_input(linein_cfg.pin));
        //if(linein_cfg.high_flag == gpio_input(linein_cfg.pin))
        //{
        //    linein_cfg.status = LINEIN_STATUS_ON;
        //    app_linein_set_state(LINEIN_W4_ATTACH);
        //}
    }
    LOG_I(LINEIN,"app_linein_init\r\n");
}

void app_linein_para_init(void)
{
	linein_cfg.status = LINEIN_STATUS_OFF;
	linein_cfg.plugin_count = 0;
}

void app_linein_uninit(void)
{
    linein_cfg.detect_func= 0;
}

#if A2DP_ROLE_SOURCE_CODE
void setFlag_APP_FLAG_LINEIN(void)
{
    app_handle_t app_h = app_get_sys_handler();
    app_h->flag_sm1 |= APP_FLAG_LINEIN;
}

void unsetFlag_APP_FLAG_LINEIN(void)
{
    app_handle_t app_h = app_get_sys_handler();
    app_h->flag_sm1 &= ~APP_FLAG_LINEIN;
}

BOOL isSetFlag_APP_FLAG_LINEIN(void)
{
    app_handle_t app_h = app_get_sys_handler();
    return app_h->flag_sm1 & APP_FLAG_LINEIN;
}

#endif

void app_linein_enter(void *arg)
{
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN)
		|| app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))
        return;

    if( app_bt_flag1_get( APP_FLAG_LINEIN ))
    {
        app_linein_state_switch_complete();
        return;
    }

    LOG_I(LINEIN,"app_linein_enter\r\n");


    app_bt_flag2_set(AP_FLAG2_LINEIN_MUTE, 0);

#if 1
    app_handle_t app_h = app_get_sys_handler();

    //app_bt_button_setting();
    app_linein_button_setting();

    aud_PAmute_delay_operation(1);
    adcmute_cnt = 80;

    app_sleep_func(0);
    app_wave_file_play_stop();

    app_bt_flag1_set(APP_FLAG_LINEIN,1);
    app_bt_flag2_set(APP_FLAG2_LED_LINEIN_INIT,0);

    if(app_bt_flag1_get(APP_AUDIO_FLAG_SET))  // bt connected
    {
        bt_all_acl_disconnect(app_h->unit);
        app_bt_flag1_set((APP_AUDIO_FLAG_SET|APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
        //jtask_schedule( app_h->app_reset_task, 800, (jthread_func)bt_unit_disable,( void *)app_h->unit);
    }
    else // have no bt connection
    {
        //app_bt_shedule_task((jthread_func)bt_unit_disable, (void *)app_h->unit, 0);
        app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
        //bt_auto_connect_stop();
        //bt_unit_set_scan_enable( app_h->unit, HCI_NO_SCAN_ENABLE);
        jtask_stop(app_h->app_auto_con_task);
        jtask_stop(app_h->app_reset_task);
    }


    //app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
    //app_wave_file_play_stop();

    #if A2DP_ROLE_SOURCE_CODE

    //close_linein2aux_loop(); //Ditial: disable ADC-->DAC test loop for linein mode
    if(isA2dpSrcConnected() && (get_a2dp_role() == A2DP_ROLE_AS_SRC))
    {
	    jtask_schedule(app_h->app_reset_task, 200, (jthread_func)appStartAuxTx,( void *)1000);
    }

    #endif

    //jtask_schedule( app_h->app_reset_task, 2000, (jthread_func)hci_disable_task,( void *)app_h->unit);

	pingpong=0;

    #if A2DP_ROLE_SOURCE_CODE

    #else
        #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_OPT_CLK_SEL, CPU_OPT_CLK_DIV); 
        #endif
    #endif
#else
    #ifdef LINEIN_BUTTON_CONFIG
    //linein_button_setting();
    #endif

    app_bt_button_setting();
    pingpong=0;
    app_wave_file_play_stop();
    start_wave_and_action(APP_WAVE_FILE_ID_LINEIN_MODE, linein_clean_current_system_mode);
#endif
}

void app_linein_exit(void *arg)
{
    if( !app_bt_flag1_get( APP_FLAG_LINEIN ))
    {
        app_linein_state_switch_complete();
        return ;
    }
    adcmute_cnt = 50;
    linein_restore_current_system_mode();

    app_bt_flag2_set(AP_FLAG2_LINEIN_MUTE, 0);

    //Delay(2000);

    #if A2DP_ROLE_SOURCE_CODE

    LOG_D(LINEIN,"app_linein_exit\r\n");
    app_bt_flag2_set(APP_FLAG2_STEREO_WORK_MODE, 0);
    unsetFlag_APP_FLAG_LINEIN();

    if(get_a2dp_role() == A2DP_ROLE_AS_SRC)
        appExitAuxTx();

    #endif
}

void app_linein_scanning( void )
{
    if(linein_cfg.detect_func)
    {
        (*linein_cfg.detect_func)();
    }
}

int app_linein_vol_up(void)
{
    MSG_T msg;

    player_vol_linein ++;

    if(player_vol_linein > AUDIO_VOLUME_MAX)
    {
        player_vol_linein = AUDIO_VOLUME_MAX;
    }

//yuan    aud_dac_set_volume(player_vol_linein);

    extPA_open(0);

    if(AUDIO_VOLUME_MAX == player_vol_linein)
    {
        playwav_resumelinein(APP_WAVE_FILE_ID_VOL_MAX);
    }

    msg.id = MSG_SAVE_VOLUME;
    msg.hdl = 0;
    msg.arg = VOL_INFO_LINEIN;
    msg_lush_put(&msg);

    return 0;
}

void app_playwav_resumelinein(uint32_t fieldId)
{
    playwav_resumelinein(fieldId);
#if 0 // need fix
    BK3000_A5_CONFIG &= (~0xAA0);
    BK3000_A3_CONFIG &=(~0x78);
#endif
    extPA_open(1);
}

void playwav_resumelinein(uint32_t fieldId)
{
    if((!app_wave_playing()) && (0 == check_wave_file_correct(fieldId)))
    {
        /* close linein audio*/
        linein_audio_close();

        /* start wave*/
        start_wave_and_action(fieldId, linein_audio_open);
    }
}

int app_linein_vol_down(void)
{
    MSG_T msg;

    player_vol_linein --;
    if(player_vol_linein < 0)
    {
        player_vol_linein = 0;
    }

//yuan    aud_dac_set_volume(player_vol_linein);

    if(AUDIO_VOLUME_MIN == player_vol_linein)
    {
        extPA_close(1);
    }

    msg.id = MSG_SAVE_VOLUME;
    msg.hdl = 0;
    msg.arg = VOL_INFO_LINEIN;
    msg_lush_put(&msg);

    return 0;
}

int app_linein_mute_unmute(void)
{
    pingpong ++;
#if 0 // need fix
    if(pingpong & 0x01)
    {
        BK3000_A3_CONFIG |= ((1 << 6) | (1 << 7));
        extPA_close(1);
    }
    else
    {
        BK3000_A3_CONFIG &= ~((1 << 6) | (1 << 7));
        extPA_open(1);
    }
#endif
    return 0;
}
static t_linein_state linein_state = LINEIN_NULL;
static t_linein_state current_linein_state = LINEIN_NULL;
t_linein_state app_linein_get_state(void)
{
    return linein_state;
}

void app_linein_set_curr_state(t_linein_state state)
{
    current_linein_state = state;
}

void app_linein_set_state(t_linein_state state)
{
	app_handle_t app_h = app_get_sys_handler();

    if(linein_cfg.enable)
    {
        linein_state = state;
        if ((linein_state==LINEIN_W4_ATTACH) 
        	&& !app_bt_flag1_get(APP_FLAG_ACL_CONNECTION)
        	&& app_bt_flag1_get(APP_FLAG_RECONNCTION|APP_FLAG_AUTO_CONNECTION))
        {
        	jtask_stop(app_h->app_auto_con_task);
        	app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
        	app_bt_shedule_task((jthread_func)app_linein_state_switch_complete, (void *)app_h->unit, 1000);
        }
        LOG_D(LINEIN,"===line in state:%d--->%d\r\n",current_linein_state,linein_state);
    }
}
void app_linein_state_switch_complete(void)
{
    if(linein_cfg.enable)
    {
        if((app_linein_get_state()&(LINEIN_W4_ATTACH|LINEIN_W4_DETACH)) == 0)
            app_linein_set_state(LINEIN_NO_ACTIVITY);
        app_linein_set_curr_state(LINEIN_NO_ACTIVITY);
    }
}
void app_linein_state_process(void)
{
    if(linein_cfg.enable)
    {
        switch(linein_state)
        {
            case LINEIN_NO_ACTIVITY:
                    current_linein_state = LINEIN_NO_ACTIVITY;
                    break;
            case LINEIN_W4_ATTACH:
					#if CONFIG_LINEIN_ATTACH_OFF_POWER
					linein_state = LINEIN_POWER_DOWN;
					#else
                    if(current_linein_state != LINEIN_NO_ACTIVITY)
                    {
                        linein_state = LINEIN_W4_ATTACH;
                    }
                    else
                    {
                        linein_state = LINEIN_ATTACH;
                    }
					#endif
                    break;
            case LINEIN_ATTACH:
                    if(current_linein_state == LINEIN_ATTACH)
                        break;
					if(app_wave_playing()&&app_bt_flag2_get(APP_FLAG2_LED_LINEIN_INIT)) //if not,will miss wave "power on",while power on in AUX mode.
						break;
                    current_linein_state = LINEIN_ATTACH;
                    app_linein_enter(NULL);
                    break;
            case LINEIN_W4_DETACH:
					#if CONFIG_LINEIN_ATTACH_OFF_POWER
					linein_state = LINEIN_NO_ACTIVITY;
					#else
                    if(current_linein_state != LINEIN_NO_ACTIVITY)
                    {
                        linein_state = LINEIN_W4_DETACH;
                    }
                    else
                    {
                        linein_state = LINEIN_DETACH;
                    }
					#endif
                    break;
            case LINEIN_DETACH:
                    if(current_linein_state == LINEIN_DETACH)
                        break;
                    current_linein_state = LINEIN_DETACH;
                    app_linein_exit(NULL);
                    break;
            case LINEIN_POWER_DOWN:
                    if(current_linein_state == LINEIN_POWER_DOWN)
                        break;
                    current_linein_state = LINEIN_POWER_DOWN;
    				LOG_I(LINEIN,"LINEIN attached, will power down\r\n");
    				app_powerdown();
                    break;
            default:break;
        }
    }
}
// EOF
