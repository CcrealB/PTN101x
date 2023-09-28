/**
 * @file   app_charge.c
 * @author
 *
 * @brief
 *
 * (c) 2018 BEKEN Corporation, Ltd. All Rights Reserved
 */
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "app_charge.h"
#include "PTN101_calibration.h"

#if ((CONFIG_CHARGE_EN == 1))
//volatile uint8_t det_charge_flag = 0;
//volatile uint8_t first_charge_flag = 0;
static uint8_t is_usb_plug_in =0;
static uint16_t charge_det_cnt = 0;
static t_battery_prepare s_charge_prepare = CHARGE_PREPARE_NONE;
static t_battery_charge_state charge_state = BATTERY_CHARGE_PREPARE;
uint8  is_charge_repeat = 0;

void usb_plug_isr(void)
{
	is_usb_plug_in = 1;
	app_charge_disable_usb_plug_detect();
}
uint8_t app_charge_is_usb_plug_in(void)
{ //TODO
    return (vusb_is_plug_in()||VUSB_IS_CHARGE_MODE);//is_usb_plug_in;
}


void app_charge_reset_usb_plug_in(void)
{
//have to disable interrupt?
    is_usb_plug_in = 0;
    REG_SYSTEM_0x09 |= (1 << VIC_IDX_USBPLUG);
}
void app_charge_disable_usb_plug_detect(void)
{
    REG_SYSTEM_0x09 &= ~(1 << VIC_IDX_USBPLUG);
}

/*FUNCTION*----------------------------*
* Function Name  : app_charge_init
* Input para     : No
* Returned Value : No
* Comments       :
*  Initialize charge module*
*------------*/
void app_charge_init(void)
{
    app_bt_flag2_set(APP_FLAG2_CHARGE_POWERDOWN,0);
  #if (CONFIG_EXT_CHARGE_IO_DETECT==1)  
    gpio_config(EXT_CHARGE_DETECT_IO,3);
  #endif
    //app_charge_reset_usb_plug_in();
}

/*FUNCTION*----------------------------
*
* Function Name  : app_charge_powerOn_handler
* Input para     : ��
* Returned Value : 1=detect Vusb 5V, 0=not detect Vusb 5V
* Comments       :
*     when poweon��handle charge
*
*------------*/
uint8_t app_charge_powerOn_handler(void)
{
    if (is_usb_plug_in)
    {
        if(app_env_check_Charge_Mode_PwrDown())
        {
            //LOG_I(APP,"app_charge_powerOn_handler\r\n");
            //bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,1);
        }
        else
        {
            //bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
        }
    }

    return is_usb_plug_in;
}

/*FUNCTION*----------------------------
*
* Function Name  : app_charge_handler
* Input para     : the step between 2 timer interrupts
* Returned Value : ��
* Comments       :
*     be called at timer interrupt
*
*------------*/
void app_charge_handler(uint32_t step )
{
    /* battery charge state process */
    app_charge_process();  /* battery charge state */
    charge_det_cnt += step;
    if (charge_det_cnt > 50)  /* charge level detect interval = 1s */
    {
        charge_det_cnt = 0;
        //app_charge_reset_usb_plug_in();
        #if (REMOTE_RANGE_PROMPT == 1)
        app_remote_range_judge();
        #endif
    }
}

/*FUNCTION*----------------------------
*
* Function Name  : app_charge_powerOff_handler
* Input para     : ��
* Returned Value : 1=detect Vusb 5V, 0= not detect Vusb 5V
* Comments       :
*     when poweoff��handle charge
*
*------------*/
uint8_t app_charge_powerOff_handler(void)
{
//todo:
    return is_usb_plug_in;
}

 /*FUNCTION*----------------------------
 *
 * Function Name  : app_charge_button_handle
 * Input para	  : no
 * Returned Value : no
 * Comments 	  :
 *	   called when detect button
 *
 *------------*/
void app_charge_button_handle(void)
{
#if 0
    app_env_handle_t  env_h = app_env_get_handle();
    uint8_t wakup_pin = env_h->env_cfg.system_para.wakup_pin + GPIO0;

    if( gpio_input(wakup_pin))
    {
        if(app_charge_is_powerdown())
        {
            bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
            os_printf("app_charge_button_handle\r\n");
            BK3000_start_wdt(0xfff);
            while(1);
        }
    }
#endif
}
  /*FUNCTION*----------------------------
  *
  * Function Name  : app_charge_is_powerdown
  * Input para	   : no
  * Returned Value : 
  * Comments	   :
  * 	get the status of charge_powerdown
  *
  *------------*/
uint8_t app_charge_is_powerdown( void )
{
    return (app_env_check_Charge_Mode_PwrDown() && app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN));
}

/* Battery charge state machine for CHARGE_HARDWARE */
t_battery_charge_state get_Charge_state(void)
{
    return charge_state;
}
uint8_t get_charge_is_preparing(void)
{
    return (s_charge_prepare == CHARGE_PREPARING); 
}
/* Notice: It's Fake power down.  */
 /*FUNCTION*----------------------------
 *
 * Function Name  : app_charge_fake_powerdown
 * Input para	  : no
 * Returned Value : no
 * Comments 	  :
 *	   set lower clock to decrease the power // ������ʱ�ӣ�Ŀ���ǽ��͹���
 *
 *------------*/
void app_charge_fake_powerdown( void )
{
    app_handle_t app_h = app_get_sys_handler();
   /* 
    if(!app_charge_is_powerdown()
		||(!get_Charge_state())
		||app_wave_playing()
		||(app_h->flag_sm1 & APP_FLAG_LINEIN)
		)
    {        
       return;
    }*/
        
    INFO_PRT("Charge.fake.powerdown\r\n");
    CLEAR_PWDOWN_TICK;
    sniffmode_wakeup_dly = 400;
    app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
    app_bt_flag2_set(APP_FLAG2_CHARGE_POWERDOWN,1);
    app_wave_file_play_stop();
    bt_unit_set_scan_enable(app_h->unit, 0);
    bt_unit_disable(app_h->unit );
    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV);
    set_flash_clk(FLASH_CLK_26mHz);      /**< ��FLASHʱ���л���26MHz */
    flash_set_line_mode(FLASH_LINE_2);
    s_charge_prepare = CHARGE_PREPARED;

}

uint8_t app_get_charge3v_powerdown(void)
{
#if 1
	return 0;
#else
    if (app_env_check_auto_pwr_on_enable())
    {
        gpio_config(app_env_get_pin_num(PIN_ioDet), 4);
        if ((gpio_input(app_env_get_pin_num(PIN_ioDet))==app_env_get_pin_valid_level(PIN_ioDet))
            && !(USB_IS_PLUG_IN) && !get_Charge_state())
        {
            return 0; //for test
            //return 1;
        }
    }
    return 0;
#endif	
}
#if (CONFIG_EXT_CHARGE_IO_DETECT==1)
uint8_t app_external_charge_detect(void){
  return gpio_input(EXT_CHARGE_DETECT_IO);
}
#endif
void app_battery_prepare_charge(void)
{
    app_handle_t app_h = app_get_sys_handler();
    //static uint8 advertising_state=0;
    //static uint8 advertising_enable_state=0;
    
    if(s_charge_prepare == CHARGE_PREPARE_NONE)
    {
        s_charge_prepare = CHARGE_PREPARING;
        if(app_env_check_Charge_Mode_PwrDown() 
            ||(!app_bt_flag2_get(APP_FLAG2_CHARGE_NOT_POWERDOWN))
            )
        {
            LOG_I(CHG,"Charge.prepare0:0x%x,0x%x\r\n",app_h->flag_sm1,app_h->flag_sm2);
        #if (CONFIG_EAR_IN == 1)
            app_ear_in_uninit(1);
        #endif
            app_bt_flag2_set(APP_FLAG2_CHARGE_POWERDOWN,1);
            app_h->charg_jitter_cnt = 0;
            app_h->charg_timeout_cnt = 0;
            if(app_h->flag_sm1 & APP_AUDIO_FLAG_SET )
            {
                bt_all_acl_disconnect(app_h->unit);
                app_h->flag_sm1 &= ~(APP_AUDIO_FLAG_SET|APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION);
                /* Waiting for BT disconnect */
                //app_bt_shedule_task((jthread_func)app_charge_fake_powerdown, (void *)app_h->unit, 500);//1000
            }
            else
            {
                INFO_PRT("No BT.connected\r\n");
                /* Fake power down at once */
                //app_charge_fake_powerdown();
            }
        }
        else
        {
            INFO_PRT("Charge.prepare1:0x%x,0x%x\r\n",app_h->flag_sm1,app_h->flag_sm2);
            s_charge_prepare = CHARGE_PREPARED;
        }
    }
}

/*FUNCTION*----------------------------
*
* Function Name  : app_charge_process
* Input para     : ��
* Returned Value : ��
* Comments       :
*     �����̴���
*
*------------*/
#if (CONFIG_EXT_CHARGE_IO_DETECT==1) 
static uint8_t ext_charge_flag=0;
#endif
CONST uint16_t chg_current_idx[8] = {0x02,0x04,0x06,0x08,0x17,0x19,0x1b,0x1d}; /* 40 60 80 100 160 180 200 220 mA*/
void app_charge_process(void)
{
	uint8_t cali_data;
    env_charge_cali_data_t *charge_cali;
    app_env_handle_t env_h = app_env_get_handle();
    app_handle_t app_h = app_get_sys_handler();

    switch(charge_state)
    {
        case BATTERY_CHARGE_PREPARE: // USB cable plug in
            if(VUSB_IS_CHARGE_MODE
                || app_get_charge3v_powerdown()  
  #if (CONFIG_EXT_CHARGE_IO_DETECT==1) 
                ||	!app_external_charge_detect() 
  #endif
                )  
            {
            // 1st: exit sniff mode
            #ifdef CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
                sniffmode_wakeup_dly = 1000;
                app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
            #endif
                app_h->charg_jitter_cnt ++;
            // 2nd: start switch | acl disconn
                if(app_h->charg_jitter_cnt >= CONFIG_CHARGE_JITTER_CNT)
                {
                    if((s_charge_prepare==CHARGE_PREPARE_NONE) && (app_env_check_Charge_Mode_PwrDown()))
                    {
                        msg_put(MSG_SWITCH_POWER_DOWN);
                    }
                    app_h->charg_jitter_cnt = 0;
                }
                app_h->charg_timeout_cnt ++;
                // 3rd: enter fake power down 
                if(app_h->charg_timeout_cnt >= CONFIG_CHARGE_PREPARE_CNT)
                {
                    if((s_charge_prepare == CHARGE_PREPARING)|| (s_charge_prepare == CHARGE_PREPARE_NONE))
                    {
                        if(app_env_check_Charge_Mode_PwrDown())
                        {
                            msg_put(MSG_CHARGE_FAKE_POWER_DOWN);
                        }
                        else
                        {
                            s_charge_prepare = CHARGE_PREPARED;
                        }
                    }
                    app_h->charg_timeout_cnt = 0;
                }
                
                charge_state = ((s_charge_prepare == CHARGE_PREPARED) ? BATTERY_CHARGE_ACTIVE : BATTERY_CHARGE_PREPARE);
            }
            else  //USB cable pull out 
            {
                if(app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))// one or more devices have not be connected, then set scan enable.
                {
                    app_h->charg_jitter_cnt ++;
                    if(app_h->charg_jitter_cnt >=CONFIG_CHARGE_JITTER_CNT) 
                    {
                        charge_state = ((s_charge_prepare != CHARGE_PREPARE_NONE) ? BATTERY_CHARGE_ABORTED : BATTERY_CHARGE_PREPARE); 
                        s_charge_prepare = CHARGE_PREPARE_NONE;
                        app_h->charg_jitter_cnt = 0;
                        app_h->charg_timeout_cnt = 0;
                    }
                }
                else
                {
                    charge_state = BATTERY_CHARGE_PREPARE; // ((s_charge_prepare != CHARGE_PREPARE_NONE) ? BATTERY_CHARGE_ACTIVE : BATTERY_CHARGE_PREPARE); 
                    s_charge_prepare = CHARGE_PREPARE_NONE;
                    app_h->charg_jitter_cnt = 0;
                    app_h->charg_timeout_cnt = 0;
                }  
            }
            break;
            
        case BATTERY_CHARGE_ACTIVE:
            if(VUSB_IS_CHARGE_MODE|| app_get_charge3v_powerdown())                        //USB cable plug in
            {
                INFO_PRT("VUSB.charge active:0x%x,0x%x\r\n",REG_SYSTEM_0x43,REG_SYSTEM_0x44);
            #if CONFIG_TEMPERATURE_NTC
                gpio_config(TEMPERATURE_NTC_PWR_CTRL_PIN, 1); //power ctrl NTC 
                gpio_output(TEMPERATURE_NTC_PWR_CTRL_PIN, 1);
                app_temperature_ntc_set_status(TEMPERATURE_NTC_INIT);
            #endif
                s_charge_prepare = CHARGE_PREPARE_NONE;
                charge_cali = app_get_env_charge_cali_data();
                REG_SYSTEM_0x44 |= (1<<11); //en vcv cali
                sys_delay_cycle(6);
                REG_SYSTEM_0x44 &= ~(1<<19);//calEn=0
                sys_delay_cycle(6);
                REG_SYSTEM_0x44 &= ~(1<<17);//modeSel1v=0
                REG_SYSTEM_0x43 &= ~(0x07 << 29);
                sys_delay_cycle(6);
                REG_SYSTEM_0x43 |= (3<<29);  // delay cnt = 1;
                sys_delay_cycle(6);
                REG_SYSTEM_0x43 |= (1<<28); // enable vcv mode delay 
                REG_SYSTEM_0x44 |= (1<<25); //CVen1v=1
                sys_delay_cycle(6);
                REG_SYSTEM_0x44 |= (1<<10); //enchg1v=1
                sys_delay_cycle(6);
                REG_SYSTEM_0x44 &= ~(1<<9); //extchen1v=0
                REG_SYSTEM_0x43 |= (1<<19); //vlcfsel1v=1
                sys_delay_cycle(6);
                REG_SYSTEM_0x43 |= (1<<18); //Icalsel1v=1
                sys_delay_cycle(6);
                REG_SYSTEM_0x43 |= (1<<17); //vcvsel1v=1
                sys_delay_cycle(6);
                REG_SYSTEM_0x43 |= MSK_SYSTEM_0x43_CHARGE_END_CTRL;
                sys_delay_cycle(6);
                //cali_data = chg_current_idx[env_h->env_cfg.system_para.charger_current & 0x07];
                cali_data = env_h->env_cfg.system_para.charger_current & 0x1f;
                mSET_USB_CHARGE_CURRENT(cali_data);
                sys_delay_cycle(6);
                cali_data = charge_cali->charger_vlcf+CONFIG_CHARGER_VLCF_OFFSET;
                if(cali_data > 0x7f) cali_data = 0x7f;
                mSET_USB_CHARGE_VLCF(cali_data);
                if(cali_data == 0)
                    cali_data += 1;
                REG_PMU_0x0D = cali_data;
                sys_delay_cycle(6);
                cali_data = charge_cali->charger_icp;
                mSET_USB_CHARGE_ICP(cali_data);
                sys_delay_cycle(6);
                cali_data = charge_cali->charger_vcv;
                mSET_USB_CHARGE_VCV(cali_data);
                sys_delay_cycle(6);
                mSET_USB_CHARGE_ENABLE(1);
                sys_delay_cycle(6);
                if (app_get_charge3v_powerdown())
                {
                    //set_tws_flag(TWS_FLAG_STEREO_CHARGE3V_PWR);
                    app_clear_led_event_action(0);
                    app_clear_led_event_action(1); 
                    app_clear_led_event_action(2);
                    app_led_action(1);
                    if(app_bt_flag1_get( APP_FLAG_WAVE_PLAYING))
                        app_wave_file_play_stop();				
                }
                else
                {
                    app_set_led_low_battery_all_in(1);
                    app_set_led_event_action(LED_EVENT_CHARGING);
                }
                LOG_I(CHARGE,"VUSB.charge active:0x%x,0x%x\r\n",REG_SYSTEM_0x43,REG_SYSTEM_0x44);
                charge_state =  BATTERY_CHARGING;
            }
      #if (CONFIG_EXT_CHARGE_IO_DETECT==1) 
            else if(!app_external_charge_detect())
            { 
            	mSET_USB_CHARGE_ENABLE(0);
             app_set_led_low_battery_all_in(1);
             app_set_led_event_action(LED_EVENT_CHARGING);
             INFO_PRT("external charge io done\r\n");
             ext_charge_flag=1;
		charge_state =  BATTERY_CHARGING;
            }
     #endif
            else
            {
                charge_state =  BATTERY_CHARGE_ABORTED;
            }
            break;
        case BATTERY_CHARGING:
            if(!VUSB_IS_CHARGE_MODE
        #if (CONFIG_EXT_CHARGE_IO_DETECT==1) 
            &&(app_external_charge_detect())
        #endif
            ) //USB cable pull out 
            {
			INFO_PRT("VUSB.pull out\r\n");
#if (CONFIG_EXT_CHARGE_IO_DETECT==1)
			ext_charge_flag=0;
#endif
	        	charge_state =  BATTERY_CHARGE_ABORTED;			
            }
            else if(IS_USB_CHARGE_FULL)                  //battery capacity full detect 
            {
                #if (REMOTE_RANGE_PROMPT == 1)
                UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);  
                #endif
                charge_state = BATTERY_CHARGE_FULL;
                if(app_charge_is_powerdown())
                {
                    app_set_powerdown_flag(POWERDOWN_SELECT);
                }
            }
            else if(app_env_check_Charge_Mode_HighEfficiency())
            {
                // Design for deepsleep mode when charging,according to ENV feature;
                #if (CONFIG_EXT_CHARGE_IO_DETECT==1) 
                 if(ext_charge_flag) break;
                #endif
                
                #if (REMOTE_RANGE_PROMPT == 1)
                UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);  
                #endif
                if(GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP)&&(app_charge_is_wakeup_for_charge_full()&& (!is_charge_repeat)))
                {
                    UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP);
                    charge_state = BATTERY_CHARGE_FULL;
                    if(app_charge_is_powerdown())
                    {
                        app_set_powerdown_flag(POWERDOWN_SELECT);
                    }
                }
                else
                {
                    BK3000_icu_sw_powerdown(app_env_get_pin_num(PIN_pwrBtn), POWERDOWN_CHG_DEEPSLEEP);        
                }			
            }
            break;
            
        case BATTERY_CHARGE_FULL:
            INFO_PRT("Charge.full:%x\r\n",REG_SYSTEM_0x31);
            app_set_led_event_action(LED_EVENT_BATTERY_FULL);
            if(!VUSB_IS_CHARGE_MODE) //USB cable pull out 
            {
                charge_state =  BATTERY_CHARGE_ABORTED;
            }
            else
            {
                charge_state = BATTERY_CHARGE_FINISHED;
            }
            break;
            
        case BATTERY_CHARGE_FINISHED:
            INFO_PRT("Charge.finish\r\n");
            if(!VUSB_IS_CHARGE_MODE) //USB cable pull out 
            {
                charge_state =  BATTERY_CHARGE_ABORTED;
            }
            else
            {
                mSET_USB_CHARGE_ENABLE(0);                  //????     charge disable; 
                charge_state = BATTERY_CHARGING;   //Battery has charged full,but USB not pull out 
                
                #if (REMOTE_RANGE_PROMPT == 1)
                UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);  
                #endif
                if(app_charge_is_powerdown())
                {
                #if POWER_ON_OUT_CHARGE_ENABLE
                    if (app_env_check_auto_pwr_on_enable())
                    {
                        charge_state = BATTERY_CHARGE_FULL_POWER;
                        BK3000_icu_sw_powerdown(app_env_get_pin_num(PIN_ioDet) /*wakup_pin ?? */ + GPIO0, POWERDOWN_DEEPSLEEP_WITH_GPIO);
                    }
                    else
                #endif
                    {
                        charge_state = BATTERY_CHARGE_FULL_POWER;
                        if(SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode())
                        {
                            BK3000_icu_sw_powerdown(app_env_get_pin_num(PIN_ioDet),POWERDOWN_CHG_DEEPSLEEP);
                        }
                        else
                            BK3000_icu_sw_powerdown(app_env_get_pin_num(PIN_pwrBtn), POWERDOWN_CHG_DEEPSLEEP);
                    }
                    //app_set_powerdown_flag(POWERDOWN_SELECT);
                }
            }
            break;
            
        case BATTERY_CHARGE_ABORTED:
            INFO_PRT("Charge.abort:0x%x,0x%x\r\n",REG_SYSTEM_0x43,REG_SYSTEM_0x44);
            mSET_USB_CHARGE_ENABLE(0);                       /* charge disable; */
            charge_state = BATTERY_CHARGE_PREPARE;
            
            #if (REMOTE_RANGE_PROMPT == 1)
            UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);  
            #endif
            app_bt_flag2_set(APP_FLAG2_CHARGE_NOT_POWERDOWN,1);
            if(app_bt_flag2_get(APP_FLAG2_LED_LOWPOWER))
                app_clear_led_event_action(2);
            if(app_bt_flag1_get(APP_FLAG_LOWPOWER))
                app_set_led_low_battery_all_in(0);
            if(app_charge_is_powerdown())
            {
            #if POWER_ON_OUT_CHARGE_ENABLE
            	jtask_stop(app_h->app_common_task);
                BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV); // xtal
                set_flash_clk(FLASH_CLK_26mHz);      //FLASH:26MHz 
                flash_set_line_mode(FLASH_LINE_2);
                if (app_env_check_auto_pwr_on_enable())
                {
                    SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO);
                    Delay(5);				
                    BK3000_wdt_reset();
                    while(1);
                }
                else
            #endif
                {
                    app_set_powerdown_flag(POWERDOWN_SELECT);
                }
            }
            else
            {
                app_h->led_event = LED_EVENT_END;
                app_set_led_event_action(app_h->led_event_save);
            }
            break;
            
        default:
            break;
    }

}
uint16_t app_charge_check_vbat(void)
{
    uint16_t vbat = 0,vadc = 0;
    sar_adc_enable(SARADC_CH_VBAT, 1);
    vadc = sar_adc_read();
    vbat = saradc_transf_adc_to_vol(vadc);
    sar_adc_enable(SARADC_CH_VBAT, 0);
    //LOG_I(CHARGE,"vbat:%d,adc:%x\r\n",vbat,vadc);
    return vbat;
}
void app_charge_mini_active(void)
{
    LOG_I(CHARGE,"VUSB.mini_charge\r\n");
    env_charge_cali_data_t *charge_cali = app_get_env_charge_cali_data();
    uint8_t cali_data;
#if CONFIG_TEMPERATURE_NTC
    gpio_config(TEMPERATURE_NTC_PWR_CTRL_PIN, 1); //power ctrl NTC 
    gpio_output(TEMPERATURE_NTC_PWR_CTRL_PIN, 1);
    app_temperature_ntc_set_status(TEMPERATURE_NTC_INIT);
#endif
    REG_SYSTEM_0x44 &= ~(1<<19);//calEn=0
    sys_delay_cycle(6);
    REG_SYSTEM_0x44 &= ~(1<<17);//modeSel1v=0
#if 0
    sys_delay_cycle(6);
    REG_SYSTEM_0x44 |= (1<<15);// VLCMEN = 1
    sys_delay_cycle(6);
    REG_SYSTEM_0x44 |= (1<<13);// VENDMEN = 1
#endif
    REG_SYSTEM_0x43 &= ~(0x01 << 7);
    sys_delay_cycle(6);


    REG_SYSTEM_0x43 &= ~(0x07 << 29);
    sys_delay_cycle(6);
    REG_SYSTEM_0x43 |= (1<<29);  // delay cnt = 1;
    sys_delay_cycle(6);
    REG_SYSTEM_0x43 |= (1<<28); // enable vcv mode delay 
    REG_SYSTEM_0x44 |= (1<<25); //CVen1v=1
    sys_delay_cycle(6);
    REG_SYSTEM_0x44 |= (1<<10); //enchg1v=1
    sys_delay_cycle(6);
    REG_SYSTEM_0x44 &= ~(1<<9); //extchen1v=0
    REG_SYSTEM_0x43 |= (1<<19); //vlcfsel1v=1
    sys_delay_cycle(6);
    REG_SYSTEM_0x43 |= (1<<18); //Icalsel1v=1
    sys_delay_cycle(6);
    REG_SYSTEM_0x43 |= (1<<17); //vcvsel1v=1
    sys_delay_cycle(6);
    REG_SYSTEM_0x43 |= MSK_SYSTEM_0x43_CHARGE_END_CTRL;
    sys_delay_cycle(6);

    mSET_USB_CHARGE_CURRENT(DEFAULT_CHARGE_CURRENT);
    sys_delay_cycle(6);
    cali_data = charge_cali->charger_vlcf+CONFIG_CHARGER_VLCF_OFFSET;
    if(cali_data > 0x7f) cali_data = 0x7f;
    mSET_USB_CHARGE_VLCF(cali_data);
    sys_delay_cycle(6);
    cali_data = charge_cali->charger_icp;
    mSET_USB_CHARGE_ICP(cali_data);
    sys_delay_cycle(6);
    cali_data = charge_cali->charger_vcv;
    mSET_USB_CHARGE_VCV(cali_data);
    sys_delay_cycle(6);
    mSET_USB_CHARGE_ENABLE(1);
    sys_delay_cycle(6);}

__attribute__((weak)) 
void app_charge_mini_sched(void)
{
    uint16_t vbat = 0;
    vbat = app_charge_check_vbat();
    LOG_I(CHARGE,"VBAT:%d,%d\r\n",vbat,vusb_is_charging());
    if(!((vbat < POWERON_VBAT_THRESHOLD) && vusb_is_charging())) return;
    app_env_cali_data_prt();
    WNG_PRT("Waiting For System Power On...\r\n");
    while(1)
    {
        switch(charge_state)
        {
            case BATTERY_CHARGE_PREPARE:
                charge_state = vusb_is_charging() ? BATTERY_CHARGE_ACTIVE : BATTERY_CHARGE_ABORTED;
                break;
            case BATTERY_CHARGE_ACTIVE:
                BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV*8);
                set_flash_clk(FLASH_CLK_26mHz);
                app_charge_mini_active();
                charge_state = BATTERY_CHARGING;
                break;
            case BATTERY_CHARGING:
                charge_state = vusb_is_charging() ? BATTERY_CHARGING : BATTERY_CHARGE_ABORTED;
                vbat = app_charge_check_vbat();
                if(vbat > (POWERON_VBAT_THRESHOLD + POWERON_VBAT_OFFSET))
                {
                    charge_state = BATTERY_CHARGE_ABORTED;
                }
                else
                {
                    BK3000_icu_sw_powerdown(GPIO15,POWERDOWN_CHG_DEEPSLEEP);  // Default wkup pin == GPIO15
                    //os_delay_ms(5);
                    //CLEAR_WDT;
                }
                break;
            case BATTERY_CHARGE_ABORTED:
                BK3000_wdt_reset();
                while(1);
                break;
            default:
                break;
        }
        
    }
    
}

static u_int32 s_pmu_0x01_before_Wakeup = 0; // for save charging_full flag;
void app_charge_save_flag_before_Wakeup(void)
{
	s_pmu_0x01_before_Wakeup = REG_PMU_0x01;
}

int app_charge_is_wakeup_for_charge_full(void)
{
    return (s_pmu_0x01_before_Wakeup&(1<<1));
}

#endif

