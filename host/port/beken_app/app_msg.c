#include "app_beken_includes.h"
#include "app_msg.h"
#include "msg_pub.h"

void common_msg_handler(MSG_T *msg_ptr)
{
    app_env_handle_t env_h = app_env_get_handle();
    app_handle_t app_h = app_get_sys_handler();


    switch(msg_ptr->id)
    {
    case MSG_ENV_WRITE_ACTION:
        //os_printf("common_msg_handler. exec->MSG_ENV_WRITE_ACTION \r\n");
		if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_DM_1V1|BT_MODE_BLE))
			app_env_write_action(&(env_h->env_data.default_btaddr),(uint8_t)(msg_ptr->arg));
		else
			app_env_write_flash((uint8_t)(msg_ptr->arg), (uint8_t)(msg_ptr->arg2));

        break;
    case MSG_SD_READ_ERR:
        break;
/*yuan++
#ifdef CONFIG_APP_SDCARD
    case MSG_SD_ATTACH_CHANGE:
        if(1 == sd_is_attached())
        {
            app_sleep_func(0);
        }

        system_mode_shift_for_sd_change();
        break;
#endif
*/
    case MSG_SDADC:
#ifdef CONFIG_APP_HALFDUPLEX
        app_hfp_echo_erase();
#endif
        break;

    case MSG_FLASH_WRITE:
        break;

    case MSG_POWER_DOWN:
		app_button_powerdown();
        break;

    case MSG_POWER_UP:
        LOG_I(MSG,"MSG_POWER_UP\r\n");
        BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV); // xtal
        set_flash_clk(FLASH_CLK_26mHz);      //FLASH:26MHz 
        flash_set_line_mode(FLASH_LINE_2);
        Delay(5);
        BK3000_wdt_reset();
        while(1);
        break;

    case MSG_IRDA_RX:
#ifdef CONFIG_APP_IR_RX
    	{
    		extern void app_ir_rx_proc(void);
    		app_ir_rx_proc();
    	}
#endif
        break;

    case MSG_USER_SarADC_UPDT:
    {
#ifdef SarAdcChVal	//yuan++
        if(saradc_get_chnl_busy()) SADC_PRT("[MSG_USER_SarADC_UPDT] --> saradc busy.\n");
        extern void user_saradc_update_req(void);
        user_saradc_update_req();
#endif
    }break;
    case MSG_LOWPOWER_DETECT:
        if(saradc_get_chnl_busy()) {
            SADC_PRT("[MSG_LOWPOWER_DETECT] --> saradc busy, redo.\n");
            msg_put(MSG_LOWPOWER_DETECT);
        }else{
//yuan++            app_low_power_detect();
        }
        break;
        
#if (CONFIG_TEMPERATURE_DETECT == 1)
    case MSG_TEMPERATURE_DETECT:
        if(saradc_get_chnl_busy()) {
            SADC_PRT("[MSG_TEMPERATURE_DETECT] --> saradc busy, redo.\n");
            msg_put(MSG_TEMPERATURE_DETECT);
        }else{
            temperature_saradc_enable();
        }
        break;
#endif
    case MSG_TEMPRATURE_APLL_TOGGLE:
        system_apll_toggle();
        break;
#if (CPU_CLK_SEL == CPU_CLK_DPLL)
    case MSG_TEMPRATURE_DPLL_TOGGLE:
    {
        uint16_t tempe_data[3];
        temperature_saradc_data_get(&tempe_data[0], 3);
        LOG_I(MSG,"DPLL_toggle, tempe_data:%d, %d, %d\n", tempe_data[0], tempe_data[1], tempe_data[2]);
        int cpu_sel_bakup = (REG_SYSTEM_0x00 & MSK_SYSTEM_0x00_CORE_SEL) >> SFT_SYSTEM_0x00_CORE_SEL;
        int div_sel_bakup =  (REG_SYSTEM_0x00 & MSK_SYSTEM_0x00_CORE_DIV) >> SFT_SYSTEM_0x00_CORE_DIV;
        unsigned char fls_sel_bakup = (REG_FLASH_0x07 & MSK_FLASH_0x07_CLK_CONF) >> SFT_FLASH_0x07_CLK_CONF;
        // uint32_t interrupts_info, mask;
        // SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
        // REG_GPIO_0x20 = 2;// ~180us
        BK3000_set_clock(CPU_CLK_XTAL, 1); // xtal
        set_flash_clk(FLASH_CLK_26mHz);      //FLASH:26MHz
        system_dpll_toggle();
        BK3000_set_clock(cpu_sel_bakup, div_sel_bakup);
        set_flash_clk(fls_sel_bakup);
        // REG_GPIO_0x20 = 0;
        // SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
    }
    break;
#endif

#if 0
    case MSG_LINEIN_ATTACH:
        jtask_stop(app_common_get_task_handle());
        jtask_schedule(app_common_get_task_handle(), 1000, app_linein_enter,NULL);
        break;

    case MSG_LINEIN_DETACH:
        jtask_stop(app_common_get_task_handle());
        jtask_schedule(app_common_get_task_handle(), 500, app_linein_exit,NULL);
        break;
#endif

    case MSG_KEYPRESS:
        CLEAR_SLEEP_TICK;
        button_action((msg_ptr->arg) | ((uint64_t)msg_ptr->arg2 << 32));
        break;

    case MSG_TIMER_PWM1_PT2_HDL:
        if(msg_ptr->hdl)
        {
            (*((jthread_func)msg_ptr->hdl))((void *)msg_ptr->arg);
        }
        break;

    case MSG_SAVE_VOLUME:
        jtask_schedule(app_h->app_save_volume_task, 500, save_volume_task, (void *)msg_ptr->arg);
        break;

    case MSG_ENTER_MATCH_STATE:
        enter_match_state();
        //bt_auto_connect_stop();
        break;
		
    case MSG_POWER_ON_START_CONN:
        //app_bt_flag1_set( APP_FLAG_AUTO_CONNECTION,1);
        //bt_connection_req(&app_h->remote_btaddr,PROFILE_BT_HFP|PROFILE_BT_A2DP_SNK|PROFILE_BT_AVRCP);
        break;
		
	case MSG_SWITCH_POWER_DOWN:
		app_battery_prepare_charge();
		break;

	case MSG_CHARGE_FAKE_POWER_DOWN:
		app_charge_fake_powerdown();
		break;

    case MSG_VUSB_DETECT_JITTER:
    case MSG_VUSB_DETECT:
        vusb_mode_stm();
        break;

	case MSG_CLEAT_MEMORY:
		app_button_sw_action(BUTTON_BT_CLEAR_MEMORY);
        break;
    case MSG_CHANGE_MODE:
        system_work_mode_change_button();
        break;
#if CONFIG_USE_USB
    case MSG_USB_SPK_OPEN:
    // if(!audio_aso_type_get(ASO_TYPE_USB))
    {
        // exit_work_mode(get_app_mode());
        extern void uac_usb_out_open(uint8_t en);
        uac_usb_out_open(1);
    }break;
    case MSG_USB_SPK_CLOSE:
    {
        extern void uac_usb_out_open(uint8_t en);
        uac_usb_out_open(0);
    }break;
    case MSG_USB_MIC_OPEN:
    {
        extern void uac_usb_in_open(uint8_t en);
        uac_usb_in_open(1);
    }break;
    case MSG_USB_MIC_CLOSE:
    {
        extern void uac_usb_in_open(uint8_t en);
        uac_usb_in_open(0);
    }break;
#endif
#if CONFIG_TEMPERATURE_NTC
    case MSG_TEMPRATURE_NTC_DETECT:
        app_temprature_ntc_saradc_enable();
        break;
#endif
#if (CONFIG_EAR_IN == 1)
    case MSG_EAR_ATTACH:
        app_ear_in_handle(1);
        break;	
    case MSG_EAR_DETACH:
        app_ear_in_handle(0);
        break;
    case MSG_EAR_ATTACH_WAVEFILE:
        app_ear_in_wave_file_play();
        break;
    case MSG_EAR_ATTACH_SEND_STATUS:
        //app_set_earin_status(EAR_IN_STATUS_ON);
        break;
    case MSG_EAR_DETACH_SEND_STATUS:
        //app_set_earin_status(EAR_IN_STATUS_OFF);
        break;
#endif		
    default:
        break;
    }

    linein_msg_handler(msg_ptr);

    env_h = app_env_get_handle();
    if( sleep_tick >= env_h->env_cfg.system_para.sleep_timeout )
    {
        app_sleep_func(1);
    }
}

void bt_msg_handler(MSG_T *msg_ptr)
{
}

void sd_music_msg_handler(MSG_T *msg_ptr)
{
#ifdef CONFIG_APP_SDCARD
    switch(msg_ptr->id)
    {
    case MSG_SD_NOTIFY:
        break;

    default:
        break;
    }

#endif
}

#if 0//(defined(CONFIG_APP_UDISK))
void udisk_msg_handler(MSG_T *msg_ptr)
{
    if(udisk_get_pseudo_swi())
    {
        app_udisk_swi_handler();
    }

    udisk_produce_nodes();
    udisk_consume_nodes();
}
#endif

void linein_msg_handler(MSG_T *msg_ptr)
{
/*
    t_linein_state linein_st = LINEIN_NO_ACTIVITY;
    switch(msg_ptr->id)
    {
        case MSG_LINEIN_ATTACH:
            linein_st = app_linein_get_state();
            if(linein_st == LINEIN_NO_ACTIVITY)
            {
                app_linein_set_state(LINEIN_ATTACH);
            }
            else if(linein_st != LINEIN_ATTACH)
            {
                msg_put(MSG_LINEIN_ATTACH);
            }
            app_linein_state_process();
            break;

        case MSG_LINEIN_DETACH:
            linein_st = app_linein_get_state();
            if(linein_st == LINEIN_NO_ACTIVITY)
            {
                app_linein_set_state(LINEIN_DETACH);
            }
            else if(linein_st != LINEIN_DETACH)
            {
                msg_put(MSG_LINEIN_DETACH);
            }
            app_linein_state_process();
            break;
        default:break;
    }
*/
    app_linein_state_process();

}

// EOF
