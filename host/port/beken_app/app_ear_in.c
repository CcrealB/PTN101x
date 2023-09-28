#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "config.h"

#if (CONFIG_EAR_IN == 1)
enum
{
	EAR_OP_NONE = 0,
	EAR_SEND_OP = 1,
	OP_DURING_EAROUT = 2
};

#define EAR_IN_GPIO_DEBOUNCE_COUNT          (50)

#define EAR_PLAY_OPCODE_ACTIVE_FLAG         0x01
#define EAR_PAUSE_OPCODE_ACTIVE_FLAG        0x02

static earin_cfg_t earin_cfg;
static uint8 pp_oper_flag = 0;
static uint8 last_earin_uninit_status = 0;

static uint8 ear_opcode_activing = 0;
#ifdef CONFIG_BLUETOOTH_IAP
extern void iap_update_earphone_icon(uint8 role); //Update earphone icon:ear in/out
#endif
static void Set_ear_playopcode_activeflag(uint8 status, uint8 active)
{
    if(active)
        ear_opcode_activing |= status;
    else
        ear_opcode_activing &= ~status;
}

static uint8 Get_ear_playopcode_activeflag(void)
{
    return ear_opcode_activing;
}

#if (EARIN_DEV_TYPE == DEVICE_HY2751)

#define EAR_IN_DETECT_GAP			50
#define EAR_IN_DETECT_CNT_THRE		3

#define PLUS_HIGH_LEVEL_LATCH_US		1	//20*10us
#define PLUS_LOW_LEVEL_LATCH_MS			10

void earin_set_plusdetect_count(void)
{
	earin_cfg.plus_det_count++;
    if(!earin_cfg.wakeup_dly)
    {
        earin_cfg.wakeup_dly = 1;
        sniffmode_wakeup_dly = 1000;
    }
}

uint32 earin_get_plusdetect_count(void)
{
	return earin_cfg.plus_det_count;
}

static uint32 earin_plus_check_handle(uint32 step)
{
	static uint32 t0;
	static uint32 plug1,plug2;
	static uint8 ear_status = 0;

	if(plug1 == 0 && plug2 == 0)
	{
		plug1 = earin_get_plusdetect_count();
		t0 = 0;
	}
	else
	{
		plug2 = earin_get_plusdetect_count();
        t0 += step;
		if(t0 >= EAR_IN_DETECT_GAP)
		{
			t0 = 0;
			if(plug1 != plug2)
				ear_status = 1;		//ear in
			else
				ear_status = 0;		//ear out
			plug1 = plug2;
		}
	}
	return ear_status;
}

#if (CONFIG_DRIVER_PWM == 0)
static uint8 flag_earin_sleep_mode = 0;
void earin_generate_start_from_sleep_mode(uint8 flag)
{
    flag_earin_sleep_mode = flag;
}
uint8 earin_start_get_status_from_sleep_mode(void)
{
    return flag_earin_sleep_mode;
}

void earin_set_plusdetect_count_timeisr(void)
{
    if(app_env_get_pin_enable(PIN_earInDet))
    {
        if(earin_cfg.high_flag)
        {
            if(gpio_input(earin_cfg.Det_pin))
            {
                earin_set_plusdetect_count();
            }
        }
        else
        {
            if(!gpio_input(earin_cfg.Det_pin))
            {
                earin_set_plusdetect_count();
            }
        }

        gpio_output(earin_cfg.Cs_pin, 0);
    }
}

void ear_in_plus_generate_timeisr(uint8 enable, uint8 step)
{
    static uint16 tick_cnt = 0;
    static uint8 ear_in_plus_sniff = 0;
    
    if(app_env_get_pin_enable(PIN_earInDet))
    {
   		if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || app_charge_is_powerdown())
        {
            return;
        }

        if(enable)
        {
            tick_cnt += step;
            if(earin_start_get_status_from_sleep_mode())
            { 
                if((tick_cnt >= 1)&&(!ear_in_plus_sniff))
                {
                    tick_cnt = 0;
                    gpio_output(earin_cfg.Cs_pin, 1); 
                    ear_in_plus_sniff = 1;
                }
                
            }
            else
            {
                if((tick_cnt >=100)&&(!ear_in_plus_sniff))
                {
                    tick_cnt = 0;
                    gpio_output(earin_cfg.Cs_pin, 1);
                    ear_in_plus_sniff = 1;
                }
            }
        }
        else
        {
            ear_in_plus_sniff = 0;
            earin_set_plusdetect_count_timeisr();
            earin_generate_start_from_sleep_mode(0); 
        }
    }
}

static void ear_in_plus_generate(uint32 plus_h, uint32 plus_l)
{
	static uint8 plus_high_flag = 1;
	static uint32 t0;
	static uint8 latch_cnt = 0;
	static uint8 start_flag = 0;

	if(start_flag == 0)
	{
		t0 = os_get_tick_counter();
		start_flag = 1;
	}

	if(plus_high_flag)
	{
		latch_cnt += os_get_tick_counter() - t0;
		t0 = os_get_tick_counter();
		if(latch_cnt >= plus_h)
		{
			plus_high_flag = 0;
			latch_cnt  = 0;
			gpio_output(earin_cfg.Cs_pin, 0);
			gpio_int_enable(earin_cfg.Det_pin,2); //RISE EDGE
		}
	}
	else
	{
		latch_cnt += os_get_tick_counter() - t0;
		t0 = os_get_tick_counter();
		if(latch_cnt >= plus_l)
		{
			plus_high_flag = 1;
			latch_cnt  = 0;
			gpio_output(earin_cfg.Cs_pin, 1);
		}
	}
}

#else

static void app_earin_pwm_init(void)
{
    pwm_ear_in_detect(TRUE);
    pwm_set_isr_busy(1);
}

void app_earin_pwm_handler(uint32_t step)
{
    static uint16 earin_pwm_cnt = 0;
    /* earin pwm process */
    if(app_env_get_pin_enable(PIN_earInDet))
    {
   		if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || app_charge_is_powerdown())
        {
            return;
        }
      
        earin_pwm_cnt += step;
        if (earin_pwm_cnt > 10)
        {
            app_earin_pwm_init();
            earin_pwm_cnt = 0;
        }
    }
}

void earin_set_plusdetect_count_pwmisr(void)
{
    if(app_env_get_pin_enable(PIN_earInDet))
    {
        if(earin_cfg.high_flag)
        {
            if(gpio_input(earin_cfg.Det_pin))
            {
                earin_set_plusdetect_count();
            }
        }
        else
        {
            if(!gpio_input(earin_cfg.Det_pin))
            {
                earin_set_plusdetect_count();
            }
        }
    }
}

#endif    /*CONFIG_DRIVER_PWM*/

void app_ear_in_plus_handle(void)
{
#if (CONFIG_DRIVER_PWM == 0)
    if(app_env_get_pin_enable(PIN_earInDet))
    {
   		if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || app_charge_is_powerdown())
        {
            return;
        }

        ear_in_plus_generate(PLUS_HIGH_LEVEL_LATCH_US, PLUS_LOW_LEVEL_LATCH_MS);
    }
#endif    /*CONFIG_DRIVER_PWM*/
}

#endif    /*EARIN_DEV_TYPE == DEVICE_HY2751*/ 

/****************************************************************/
// to solve primary and secondary at the same time out or in 
// can not control the video
static uint8 ear_status_interval = 0;
static uint8 ear_status_change = 0;
static uint8 ear_status_curren = 0;
#if 0
static void other_ear_change_status_init(uint8 val, uint8 status)
{
    ear_status_interval = val;
    ear_status_curren = status;
    ear_status_change = 1;
}
#endif
static void set_other_ear_change_interval(void)
{
    if(ear_status_change)
    {
        if(ear_status_interval > 0)
        {
            if(--ear_status_interval == 0)
                ear_status_change = 0;
        }
        else
            ear_status_change = 0;
    }
}

static uint8 get_other_ear_change_status(uint8 in_or_out)
{
    if(ear_status_curren != in_or_out) //keep prim & sec at same change status
        ear_status_change = 0;
    
    return ear_status_change;
}
/****************************************************************/

uint8 get_earin_cs_pin(void)
{
    return earin_cfg.Cs_pin;
}

uint8 get_earin_det_pin(void)
{
	return earin_cfg.Det_pin;
}

uint8 app_get_earin_cfg(void)
{
	if(app_env_get_pin_enable(PIN_earInDet))
		return earin_cfg.enable;
	else 
		return 0;
}

void earin_playpause_enable(uint8 enable)
{
    earin_cfg.pp_enable = enable;
    INFO_PRT("earin play_pause enable:%d\n",earin_cfg.pp_enable);
}

static uint8 get_playpause_enable(void)
{
    return earin_cfg.pp_enable;
}

uint8 app_get_earin_tws_switch_status(void)
{
    if(0 == earin_cfg.enable)
        return 0;
    else
        return earin_cfg.tws_pss_switch;
}

static inline void attach_judge(uint32 step)
{
    static uint8 attach_flag = 0;
    static uint32 wait_rps_cnt = 0;
    static uint8 attach_wave_flag = 0;
    
    earin_cfg.wait_plugin_time = 0;
    earin_cfg.pullout_count = 0;

    if(EAR_IN_STATUS_OFF == earin_cfg.status)
    {
        if(earin_cfg.plugin_count & 0x100000)
        {
            if(++wait_rps_cnt > EAR_IN_GPIO_DEBOUNCE_COUNT - 40)
            {
                attach_wave_flag = 1;
                wait_rps_cnt = attach_flag = 0;
                earin_cfg.plugin_count = 0;
                earin_cfg.status = EAR_IN_STATUS_ON;
            }
            if(/*app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE)*/ 
                Get_PlayPause_Flag_Operate(OTHER_EAR_DETECT_STATUS_FLAG)
                && earin_cfg.pre_other)
            {
                attach_wave_flag = 0;
                wait_rps_cnt = attach_flag = 0;
                earin_cfg.plugin_count = EAR_IN_GPIO_DEBOUNCE_COUNT + 40;
                earin_cfg.status = EAR_IN_STATUS_ON;
            }
        }
        else
        {
            earin_cfg.plugin_count += step;
            if(earin_cfg.plugin_count > EAR_IN_GPIO_DEBOUNCE_COUNT - 20)
            {
                INFO_PRT("===EAR attach===\r\n");
                earin_cfg.plugin_count |= 0x100000;
                earin_cfg.tws_pss_switch = 0;
                earin_cfg.pre_other = Get_PlayPause_Flag_Operate(OTHER_EAR_DETECT_STATUS_FLAG);
                attach_wave_flag = wait_rps_cnt = 0;
            // exit sniff mode
            #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                if(!bt_sniff_is_all_active()) 
                {            
                    sniffmode_wakeup_dly = 1000;
					app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
                    bt_sniff_all_exit_mode();
                }
            #endif
            }
        }
    }
    else
    {
        if(!attach_flag)
        {
            earin_cfg.plugin_count++;
            if(earin_cfg.plugin_count > EAR_IN_GPIO_DEBOUNCE_COUNT + 60 )
            {
                attach_flag = 1;
                earin_cfg.plugin_count = 0;
                if(Get_ear_playopcode_activeflag() & EAR_PAUSE_OPCODE_ACTIVE_FLAG)
                    msg_delay_put_handle(200, MSG_EAR_ATTACH);
                else
                    msg_put(MSG_EAR_ATTACH);
            }

            if(attach_wave_flag && (earin_cfg.plugin_count > 10))
            {
                attach_wave_flag = 0;
                msg_put(MSG_EAR_ATTACH_WAVEFILE);
            }
        }
    }
}

static inline void detach_judge(uint32 step)
{
    static uint8 detach_flag = 1;
    static uint32 wait_rps_cnt = 0;

    earin_cfg.plugin_count = 0;
    if(EAR_IN_STATUS_ON == earin_cfg.status)
    {
        earin_cfg.pullout_count += step;
        earin_cfg.wait_plugin_time = 0;
        
        if(earin_cfg.pullout_count > 3)
        {	
            INFO_PRT("===EAR detach===\r\n");
        // exit sniff mode
        #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
            if(!bt_sniff_is_all_active()) 
            {            
                sniffmode_wakeup_dly = 1000;
				app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
                bt_sniff_all_exit_mode();
            }
        #endif
            //set_earin_status(EAR_IN_STATUS_OFF);
            earin_cfg.status = EAR_IN_STATUS_OFF;
            msg_put(MSG_EAR_DETACH_SEND_STATUS);
            earin_cfg.wakeup_dly = 0;
            earin_cfg.tws_pss_switch = 0;
            wait_rps_cnt = detach_flag = 0;
        }
    }
    else
    {
        if(!detach_flag)
        {
            if(++wait_rps_cnt > 6)
            {
                detach_flag = 1;
                wait_rps_cnt = 0;
                if(Get_ear_playopcode_activeflag() & EAR_PLAY_OPCODE_ACTIVE_FLAG)
                    msg_delay_put_handle(50, MSG_EAR_DETACH);
                else
                    msg_put(MSG_EAR_DETACH);
            }
        }
    }
    
	if(!Get_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG)
		&& !app_bt_flag1_get(APP_FLAG_MUSIC_PLAY|APP_BUTTON_FLAG_PLAY_PAUSE))
	{
        earin_cfg.wait_plugin_time += step;
		if(earin_cfg.wait_plugin_time > 1500)
		{
			INFO_PRT("single ear wait plugin timeout\n");
			earin_cfg.wait_plugin_time = 0;
			Set_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG, 1);
		}
	}
	else
	{
		earin_cfg.wait_plugin_time = 0;
	}
}

static void ear_in_detect(uint32 step)
{
#if (EARIN_DEV_TYPE == DEVICE_HY2751)
    uint32 ear_gpio_status = earin_plus_check_handle(step);
#elif (EARIN_DEV_TYPE == DEVICE_VM6320)
    uint32 ear_gpio_status = gpio_input(earin_cfg.Det_pin);
#endif

    if(app_bt_flag1_get(APP_PROFILES_FLAG_SET) && app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
    {
        return;
    }
    
    if(1 == ear_gpio_status)
    {
        attach_judge(step);
    }
    else
    {
        detach_judge(step);
    }
}

void app_ear_in_init(void)
{
    app_env_handle_t  env_h = app_env_get_handle();
    
    memset(&earin_cfg, 0, sizeof(earin_cfg_t));
    if(env_h->env_cfg.used == 0x01)
    {
        earin_cfg.enable = app_env_get_pin_enable(PIN_earInDet);
        if(earin_cfg.enable)
        {
            earin_cfg.high_flag = app_env_get_pin_valid_level(PIN_earInDet);
            earin_cfg.Det_pin = app_env_get_pin_num(PIN_earInDet);
            earin_cfg.Cs_pin = app_env_get_pin_num(PIN_earInCs);

        #if 0//(EARIN_DEV_TYPE == DEVICE_HY2751)
            gpio_config(earin_cfg.Det_pin, 0);
            gpio_int_enable(earin_cfg.Det_pin, 2); //rise edge
        #else
            if(earin_cfg.high_flag)
                gpio_config(earin_cfg.Det_pin,0);
            else
                gpio_config(earin_cfg.Det_pin,3);
        #endif

            if(app_env_get_pin_enable(PIN_earInCs))
            {
                gpio_config(earin_cfg.Cs_pin, 1);
                gpio_output(earin_cfg.Cs_pin, 0);
                gpio_config_capacity(earin_cfg.Cs_pin, 1);
            }

            earin_cfg.detect_func = ear_in_detect;
            earin_playpause_enable(1);  //default: On
            earin_cfg.status = EAR_IN_STATUS_OFF;
            Set_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG, 1);
            last_earin_uninit_status = 0;
        }
        else
        {
            earin_cfg.detect_func = NULL;
        }
    }
    else
    {
        earin_cfg.detect_func = NULL;
        //ear_in_detect_enable(0);//default: On.   Using IOS BT setting interface
    }
}

void app_ear_in_uninit(uint8 enable)
{
    if(app_env_get_pin_enable(PIN_earInDet)) 
    {
        if(last_earin_uninit_status == enable) 
            return;
        INFO_PRT("earin.unit\n");
        earin_cfg.detect_func = NULL;
    #if (CONFIG_DRIVER_PWM == 1)
        pwm_ear_in_detect(FALSE);
    #endif
        last_earin_uninit_status = enable;
    }
}
void app_ear_in_scanning(uint32 step)
{
    static uint8 power_wait_flag = 0;
    static uint8 power_wait_cnt = 0;

    if(!power_wait_flag)
    {
        power_wait_cnt += step;
        if(power_wait_cnt > 100)
        {
            power_wait_cnt = 0;
            power_wait_flag = 1;
        }
        return;
    }
    
    set_other_ear_change_interval();
	if(earin_cfg.detect_func != NULL)
		(*earin_cfg.detect_func)(step);
}
void app_ear_in_wave_file_play(void)
{
#if 0
    if(vusb_dlp_get_putin())
    {
        return;
    }
#endif
    INFO_PRT("SINGLE EAR attach waveplay--%x\n", app_bt_flag1_get(APP_EARIN_WAV_PROMT_FLAG_SET)); 
    if(!app_bt_flag1_get(APP_EARIN_WAV_PROMT_FLAG_SET) && get_playpause_enable())
        app_wave_file_play_start(APP_WAVE_FILE_ID_EARIN_DET);
}

void app_ear_in_handle(uint8_t in_or_out)
{
    uint32_t opcode;
    app_handle_t app_h = app_get_sys_handler();
    uint8_t need_send_flag = 0;
    static uint8 putin_charge_box = 0;

    #if CONFIG_ANC_ENABLE == 1
    app_anc_volume_restore(in_or_out);
    #endif

    if(app_get_earin_cfg())
    {
    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    #endif    
    #ifdef CONFIG_BLUETOOTH_IAP
        iap_update_earphone_icon(TWS_PRIM_SEC_PRIMARY);
    #endif
        if(avrcp_current_is_connected() && !avrcp_current_is_processing())
        {
            if(0 == in_or_out)//ear out
            {
                if(!app_bt_flag1_get(APP_BUTTON_FLAG_PLAY_PAUSE)
                    || putin_charge_box)
                {
                    INFO_PRT("opcode ingored when pausing or incharge box\n");
                    return;
                }

            #ifdef CONFIG_BLUETOOTH_IAP
                if(app_env_check_IAP_profile_enable() && !bt_flag2_is_set(APP_FLAG2_IAP_CONNECTION))
            #endif
                {
                    need_send_flag = 1;
                    opcode = AVC_OP_PAUSE;
                }
                Set_PlayPause_Flag_Operate(EAR_PLAYPAUSE_OPERATE_FLAG, 1);
                Set_ear_playopcode_activeflag(EAR_PAUSE_OPCODE_ACTIVE_FLAG, 1);
                //bt_flag1_operate(APP_BUTTON_FLAG_PLAY_PAUSE, 0);
            }
            else//ear in 
            {
            #if 0
                if(vusb_dlp_get_putin())
                {
                    INFO_PRT("put in charge box\n");
                    putin_charge_box = 1;
                    return;
                }
			#endif
                putin_charge_box = 0;
                if(Get_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG)
                    || app_bt_flag1_get(APP_FLAG_SCO_CONNECTION | APP_FLAG_HFP_CALLSETUP)
                    || app_bt_flag1_get(APP_BUTTON_FLAG_PLAY_PAUSE))   //phone active pause or close box
                {
                    INFO_PRT("opcode ingored when phone pause/hfp action/music playing\n");
                    return;
                }
            #ifdef CONFIG_BLUETOOTH_IAP
                if(app_env_check_IAP_profile_enable() && !bt_flag2_is_set(APP_FLAG2_IAP_CONNECTION))
            #endif                
                {
                    need_send_flag = 1;
                    opcode = AVC_OP_PLAY;
                }
                Set_ear_playopcode_activeflag(EAR_PLAY_OPCODE_ACTIVE_FLAG, 1);
                //bt_flag1_operate(APP_BUTTON_FLAG_PLAY_PAUSE, 1);
            }
            
            if(1 == need_send_flag && get_playpause_enable())
            {
                if(get_other_ear_change_status(in_or_out))
                {
                    INFO_PRT("prim app_h->flag_sm1:%x\n", app_h->flag_sm1);
                #ifdef CONFIG_BLUETOOTH_AVRCP
                    avrcp_current_send_opcode((void *)opcode);
                #endif

                }
                else
                {
                    INFO_PRT("app_h->flag_sm1:%x\n", app_h->flag_sm1);
                #ifdef CONFIG_BLUETOOTH_AVRCP
                    avrcp_current_send_opcode((void *)opcode);
                #endif
                }
            }
        }
        else
        {
            INFO_PRT("ear in/out invalid\n");
        }
    }
}

uint8 get_earin_status(void)
{
	if(earin_cfg.status == EAR_IN_STATUS_ON)
		return 1;
	else
		return 0;
}

uint8 get_earin_sec_status(void)
{
    if(0 == earin_cfg.enable)
        return 0;
    else
        return 1;
}

void Set_PlayPause_Flag_Operate(uint8 flag, uint8 op)
{
	if(op)
	{
		pp_oper_flag |= flag;
	}
	else
	{
		pp_oper_flag &= ~flag;
	}
}

uint8 Get_PlayPause_Flag_Operate(uint8 flag)
{
	return !!(pp_oper_flag & flag);
}

void app_ear_button_playing_syn_status(void)
{
    if(Get_PlayPause_Flag_Operate(SYN_PLAY_PAUSE_STATUS_FLAG))
    {
        Set_ear_playopcode_activeflag(EAR_PLAY_OPCODE_ACTIVE_FLAG, 0);
        return;
    }

    Set_PlayPause_Flag_Operate(SYN_PLAY_PAUSE_STATUS_FLAG, 1);
	app_bt_flag1_set(APP_BUTTON_FLAG_PLAY_PAUSE, 1);
    Set_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG, 0);
    Set_ear_playopcode_activeflag(EAR_PLAY_OPCODE_ACTIVE_FLAG, 0);
}

void app_ear_button_stoppause_syn_status(void)
{    
    if(!Get_PlayPause_Flag_Operate(SYN_PLAY_PAUSE_STATUS_FLAG))
    {
        Set_ear_playopcode_activeflag(EAR_PAUSE_OPCODE_ACTIVE_FLAG, 0);
        return;
    }

    Set_PlayPause_Flag_Operate(SYN_PLAY_PAUSE_STATUS_FLAG, 0);
	app_bt_flag1_set(APP_BUTTON_FLAG_PLAY_PAUSE, 0);
    Set_ear_playopcode_activeflag(EAR_PAUSE_OPCODE_ACTIVE_FLAG, 0);
    if(!Get_PlayPause_Flag_Operate(EAR_PLAYPAUSE_OPERATE_FLAG))
    {
        INFO_PRT("phone active operate stop/pause\n");
        Set_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG, 1);
    }
    else
    {
        INFO_PRT("ear active operate stop/pause\n");
        Set_PlayPause_Flag_Operate(EAR_PLAYPAUSE_OPERATE_FLAG, 0);
        Set_PlayPause_Flag_Operate(PHONE_PLAYPAUSE_OPERATE_FLAG, 0);
    }
}

#endif
