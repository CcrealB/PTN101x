#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "tc_interface.h"
#include "tra_hcit.h"
#include "tc_const.h"
#include "app_debug.h"
#include "bt_at_types.h"

#define LED0_MAP_DEFAULT                  16
#define LED1_MAP_DEFAULT                  15
#define LED_BLINK_FAST_COUNT_THRE         30 // 300ms
#define LED_BLINK_SLOW_COUNT_THRE         100 // 1s
#define LED_BLINK_VERY_SLOW_COUNT_THRE    200
#define LED_BLINK_VERY_FAST_COUNT_THRE    15
#define LED_ON                            1
#define LED_OFF                           0
#define LED_INVALID_INDEX                 0xFF
static uint8_t app_get_led_low_battery_pin(void);

typedef enum
{
    LED_INDEX_LED0        = 0,
    LED_INDEX_LED1        = 1,
    LED_INDEX_LP          = 2,
    LED_INDEX_ALTERN      = 3,
    LED_INDEX_BOTH        = 4,
    LED_INDEX_LED0_ONLY   = 5,
    LED_INDEX_LED1_ONLY   = 6,
    LED_INDEX_LED_LP_ONLY = 7,
    LED_INDEX_END         = 8
}APP_LED_INDEX;

CONST static app_led_info_t default_led_info[LED_EVENT_END] =
{
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_AUTO_CONNECTION = 0,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_MATCH_STATE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_NO_MATCH_NO_CONN_STATE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_CONN,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_BT_PAUSE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_BT_PLAY,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_INCOMING_RINGING,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_OUTGOING_RINGING,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_CALL_ESTABLISHMENT,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_LOW_BATTERY,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_CHARGING,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_BATTERY_FULL,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_POWER_ON,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_POWER_OFF,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_LINEIN_PLAY,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_TEST_MODE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_FM_SEARCH,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_STEREO_CONN_MODE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_STEREO_MATCH_MODE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_STEREO_RECON_MODE,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_RESERVED1,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_RESERVED2,
    { 0, 1, 500, 500, 0, -1},   // LED_EVENT_RESERVED3,
};

static __inline int app_led_onoff(int onoff)
{
    app_env_handle_t env_h = app_env_get_handle();
    int8_t ledon = ((env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_LED_REVERSE) >> 8);

    if(onoff)
        return 1 - ledon;
    else
        return ledon;
}

#define LED_ON_FUNC      app_led_onoff(1)
#define LED_OFF_FUNC     app_led_onoff(0)

static void app_config_led_mode(int index, int initial, uint16_t ontime, uint16_t offtime, 
                                         uint8_t repeat, uint8_t num_flash, int16_t timeout)
{
    app_handle_t app_h = app_get_sys_handler();
    
    if((index > LEDLP) || (initial > 1))
        return;

#if 1
    if((app_set_led_low_battery_all_in(2) && (index != app_get_led_low_battery_pin()))
    	|| (ontime==0))
	{
		initial = LED_OFF_FUNC;
	}
#endif

    app_h->led_onoff[index] = initial;
    app_h->led_ontime[index] = ontime/10;
    app_h->led_offtime[index] = offtime/10;
    app_h->led_onoff_count[index] = 0;
    
    app_h->led_repeat[index] = repeat*100;
    app_h->led_repeat_count[index] = 0;
    
    app_h->led_num_flash[index] = num_flash;
    app_h->led_num_flash_count[index] = 0;
    
    app_h->led_timeout[index] = timeout/10;

    gpio_output(app_h->led_map[index], initial);

    return;
}

void app_led_init( void )
{
    int i;
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    if( env_h->env_cfg.used == 0x01 )
    {
        memcpy((uint8_t *)app_h->led_info, (uint8_t*)&env_h->env_cfg.led_info, sizeof(env_h->env_cfg.led_info));
        app_h->led_map[LED0]  = env_h->env_cfg.led_map[LED0];
        app_h->led_map[LED1]  = env_h->env_cfg.led_map[LED1];
        app_h->led_map[LEDLP] = env_h->env_cfg.led_map[LEDLP];
    }
    else
    {
        memcpy((uint8_t *)app_h->led_info, (uint8_t*)&default_led_info, (sizeof(app_led_info_t)*LED_EVENT_END));
        app_h->led_map[LED0]  = LED0_MAP_DEFAULT;
        app_h->led_map[LED1]  = LED1_MAP_DEFAULT;
        app_h->led_map[LEDLP] = LED1_MAP_DEFAULT;
    }
    app_h->led_event = LED_EVENT_END;
    app_h->led_event_save = LED_EVENT_END;

    for(i = 0; i < LED_NUM; i++)
    {
        gpio_config(app_h->led_map[i], 1);
        app_config_led_mode(i, LED_OFF_FUNC, 0, 100, 0, 1, -1);
    }
}

static uint8_t app_get_led_low_battery_pin(void)
{
    app_handle_t app_h = app_get_sys_handler();
    
    if ((app_h->led_info[LED_EVENT_CONN].index == LED_INDEX_LED0)//以LED_EVENT_CONN为参照
        || (app_h->led_info[LED_EVENT_CONN].index == LED_INDEX_LED0_ONLY))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

uint8_t app_get_led_low_battery_index(void)
{
    app_handle_t app_h = app_get_sys_handler();
    
    if ((app_h->led_info[LED_EVENT_LOW_BATTERY].index > 7)
        && (app_h->led_info[LED_EVENT_LOW_BATTERY].index < 11))
    {
        return (app_h->led_info[LED_EVENT_LOW_BATTERY].index - 8);
    }
    
    return LED_EVENT_END;
}

uint8_t app_set_led_low_battery_all_in(uint8_t flag)
{
    static uint8_t led_map = 0;
    static uint8_t flag_led_low_battery_all_in = 0;
    app_handle_t app_h = app_get_sys_handler();
    
    if (flag == 2)
    {
        if (flag_led_low_battery_all_in)
            return 1;
        else
            return 0;
    }

    if (app_get_led_low_battery_index() != LED_EVENT_END)
    {
        if (flag == 0)//改变LED灯PIN脚
        {
            flag_led_low_battery_all_in = 1;
            led_map = app_h->led_map[app_get_led_low_battery_pin()];//暂存当前PIN脚
            
            gpio_output(app_h->led_map[LED0],LED_OFF_FUNC);
            gpio_output(app_h->led_map[LED1],LED_OFF_FUNC);
            gpio_output(app_h->led_map[LEDLP],LED_OFF_FUNC);
            
            app_h->led_map[app_get_led_low_battery_pin()] = app_h->led_map[app_get_led_low_battery_index()];
            app_h->led_event = LED_EVENT_END;
            app_set_led_event_action(app_h->led_event_save);
        }
        else if (flag_led_low_battery_all_in)//恢复LED灯PIN脚
        {
            flag_led_low_battery_all_in = 0;
            gpio_output(app_h->led_map[app_get_led_low_battery_index()],LED_OFF_FUNC);
            app_h->led_map[app_get_led_low_battery_pin()] = led_map;
        }
    }
    return 0;
}

void app_clear_led_event_action(int index)
{
    app_config_led_mode(index, LED_OFF_FUNC, 0, 100, 0, 0, -1);
}

static uint8 app_charge_led_another_on(void)
{
    app_handle_t app_h = app_get_sys_handler();
    
    if ((app_h->led_info[LED_EVENT_CHARGING].index == 7)
        || (app_h->led_info[LED_EVENT_CHARGING].index == 6)
        || (app_h->led_info[LED_EVENT_CHARGING].index == 5))
    {
        return 1;
    }
    else
    {
        return 0; 
    }
}

static uint8 app_clear_led_event_detect(APP_LED_EVENT event)
{
    app_handle_t app_h = app_get_sys_handler();
    
    if (app_charge_led_another_on())
    {
        if(!get_Charge_state() && (app_h->led_info[event].index < 5)
            && (!app_bt_flag1_get(APP_FLAG_LOWPOWER)
                    ||(app_bt_flag1_get(APP_FLAG_LOWPOWER)
                    &&(app_h->led_info[LED_EVENT_LOW_BATTERY].led_ontime==0)
                    &&(app_h->led_info[LED_EVENT_LOW_BATTERY].led_offtime==0)))
        )
        {
            return 1;
        }
    }
    else if(app_h->led_info[event].index < 5)
    {
        return 1;
    }
    
    return 0;
}

void app_set_led_event_action(APP_LED_EVENT event)
{
    app_handle_t app_h = app_get_sys_handler();
#if 1
    int i;
    int charge_pin = 0, event_pin = 0;
    static int charge_led = LED_EVENT_END;
#endif
    if((event >= LED_EVENT_END)
        ||(app_h->led_event == event)
        ||(app_h->led_event == LED_EVENT_POWER_OFF)
        ||( app_h->led_info[event].led_ontime == 0 && app_h->led_info[event].led_offtime == 0 )
        ||(app_h->led_info[event].index > LED_INDEX_LED_LP_ONLY)
        ||((event != LED_EVENT_LINEIN_PLAY) && app_bt_flag2_get( APP_FLAG2_LED_LINEIN_INIT ))
    )
    {
        return;
    }
    if (app_check_bt_mode(BT_MODE_1V2)
        && a2dp_has_connection()
        && (event < LED_EVENT_CONN))
    {
        return;
    }

    //if the current led_event,will finish in limited time,
    //then waiting for the end
    if((!app_charge_is_usb_plug_in())&&(app_h->led_info[event].timeout<=0))
    {
        for(i = 0; i < LED_NUM; i++)
        {
            if((int16)app_h->led_timeout[i]>0)
            {
                app_h->led_event_save = event;
                return;
            }
        }
    }
    if(app_charge_is_usb_plug_in()
        && (event!=LED_EVENT_CHARGING)
        && (event!=LED_EVENT_BATTERY_FULL)
        && app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))
    {
        return;
    }
    if((event==LED_EVENT_LOW_BATTERY) && app_charge_is_usb_plug_in())
    {
        return;
    }
    if(!( (event == LED_EVENT_LOW_BATTERY)|| (event == LED_EVENT_CHARGING)||(event == LED_EVENT_BATTERY_FULL)||(event == LED_EVENT_FM_SCAN)))
    {
        if (app_bt_flag1_get(APP_FLAG_LINEIN)
            && (event!=LED_EVENT_LINEIN_PLAY)
            && (event!=LED_EVENT_LINEIN_PAUSE)
            && (event!=LED_EVENT_POWER_OFF))
        {
            return;
        }
        
        app_h->led_event_save = event;
        if (!app_charge_led_another_on())
        {
            if(VUSB_IS_CHARGE_MODE)
            {
                return;
            }
        }
    }
    app_h->led_event = event;
    if (app_clear_led_event_detect(event))
    {
        app_clear_led_event_action(LED0);
        app_clear_led_event_action(LED1);
        app_clear_led_event_action(LEDLP);
    }
    
    LOG_I(LED, "event:%d\r\n", event);

#if 1
    if (app_charge_led_another_on())
    {
        if(VUSB_IS_CHARGE_MODE)
        {
            if((event == LED_EVENT_BATTERY_FULL) || (event == LED_EVENT_CHARGING))
            {
                charge_led = event;
                if ((app_h->led_info[charge_led].index == LED_INDEX_LP)//充电灯配置了LP_LED
                    || (app_h->led_info[charge_led].index == 7))
                {
                    if (app_h->led_map[LEDLP] == app_h->led_map[LED_INDEX_LED1])
                        app_clear_led_event_action(0);
                    else if (app_h->led_map[LEDLP] == app_h->led_map[LED_INDEX_LED1])
                        app_clear_led_event_action(1);
                }
            }
            else if(charge_led != LED_EVENT_END)//有配置充电LED事件
            {
                if((app_h->led_info[charge_led].index == LED_INDEX_ALTERN)//交替闪
                    || (app_h->led_info[charge_led].index == LED_INDEX_BOTH))//同时闪
                {
                    return;//当前设置的LED灯事件不导入
                }
                
                charge_pin = app_h->led_info[charge_led].index;//获取充电PIN_LED脚号
                if (charge_pin > LED_INDEX_BOTH)
                    charge_pin -= LED_INDEX_LED0_ONLY;
                
                if ((charge_pin > LED_INDEX_LED1)	//配置了三个PIN_LED脚
                    && (app_h->led_map[charge_pin]!=app_h->led_map[LED_INDEX_LED0])
                    && (app_h->led_map[charge_pin]!=app_h->led_map[LED_INDEX_LED1]))
                {
                //正常导入灯配置
                }
                else
                {
                    if ((app_h->led_info[event].index == LED_INDEX_ALTERN)//当前设置的LED_event为交替或同时闪
                        || (app_h->led_info[event].index == LED_INDEX_BOTH))
                    {
                        if (app_h->led_map[charge_pin] == app_h->led_map[LED_INDEX_LED0])//导入另外一个PIN_LED
                        {
                            app_config_led_mode(LED1, LED_ON_FUNC,
                                                                app_h->led_info[event].led_ontime,
                                                                app_h->led_info[event].led_offtime,
                                                                app_h->led_info[event].repeat,
                                                                app_h->led_info[event].number_flash, 
                                                                app_h->led_info[event].timeout);
                        }
                        else
                        {
                            app_config_led_mode(LED0, LED_ON_FUNC,
                                                                app_h->led_info[event].led_ontime,
                                                                app_h->led_info[event].led_offtime,
                                                                app_h->led_info[event].repeat,
                                                                app_h->led_info[event].number_flash, 
                                                                app_h->led_info[event].timeout);
                        }
                        return;
                    }
                    else
                    {
                        event_pin = app_h->led_info[event].index;
                        
                        if (event_pin > LED_INDEX_BOTH)
                            event_pin -= LED_INDEX_LED0_ONLY;
                        
                        if (app_h->led_map[charge_pin] == app_h->led_map[event_pin])
                        {
                            app_clear_led_event_action(charge_pin^0x01);
                            return;//充电灯事件优先，当前事件不予导入配置
                        }
                    }
                }
            }
        }
    }
#endif

    if(app_h->led_info[event].index == LED_INDEX_ALTERN)
    {
        app_config_led_mode(LED0, LED_OFF_FUNC,
                            app_h->led_info[event].led_ontime,
                            app_h->led_info[event].led_offtime,
                            0,
                            1, 
                           -1);
        
        app_config_led_mode(LED1, LED_ON_FUNC,
                            app_h->led_info[event].led_ontime,
                            app_h->led_info[event].led_offtime,
                            0,
                            1, 
                           -1);
    
    }
    else if(app_h->led_info[event].index == LED_INDEX_BOTH)
    {
        app_config_led_mode(LED0, LED_ON_FUNC,
                            app_h->led_info[event].led_ontime,
                            app_h->led_info[event].led_offtime,
                            app_h->led_info[event].repeat,
                            app_h->led_info[event].number_flash, 
                            app_h->led_info[event].timeout);
        
        app_config_led_mode(LED1, LED_ON_FUNC,
                            app_h->led_info[event].led_ontime,
                            app_h->led_info[event].led_offtime,
                            app_h->led_info[event].repeat,
                            app_h->led_info[event].number_flash, 
                            app_h->led_info[event].timeout);
    }
    else
    {
        app_config_led_mode(app_h->led_info[event].index, LED_ON_FUNC,
                            app_h->led_info[event].led_ontime,
                            app_h->led_info[event].led_offtime,
                            app_h->led_info[event].repeat,
                            app_h->led_info[event].number_flash, 
                            app_h->led_info[event].timeout);
    }
}

void app_led_action(uint32_t step)
{
    int i;
    app_handle_t app_h = app_get_sys_handler();
    uint8_t low_batt_led_index;
    
    low_batt_led_index = app_h->led_info[LED_EVENT_LOW_BATTERY].index;
    
    if(low_batt_led_index >= LED_INDEX_LED0_ONLY)
        low_batt_led_index -= LED_INDEX_LED0_ONLY;
    
    for(i = 0; i < LED_NUM; i++)
    {
        if(app_h->led_map[i] == LED_INVALID_INDEX)
            continue;
        
        if(app_bt_flag2_get(APP_FLAG2_LED_LOWPOWER)
            &&(i!=low_batt_led_index)
            &&(low_batt_led_index <= LED_INDEX_LP)
            &&(app_h->led_map[i]==app_h->led_map[low_batt_led_index])
        )
        {
            continue;
        }
        
        if (app_set_led_low_battery_all_in(2)&& (i!=app_get_led_low_battery_pin()))
        {
            continue;
        }
        
        if(app_h->led_num_flash_count[i] < app_h->led_num_flash[i])
        {
            app_h->led_onoff_count[i] += (uint16_t)step;

            if( app_h->led_onoff[i] == LED_ON_FUNC )
            {
                if(app_h->led_offtime[i] != 0 && app_h->led_onoff_count[i] >= app_h->led_ontime[i])
                {
                    gpio_output_reverse(app_h->led_map[i]);
                    app_h->led_onoff_count[i] = 0;
                    app_h->led_onoff[i] = LED_OFF_FUNC;
                    app_h->led_num_flash_count[i]++;
                }
            }
            else
            {
                if(app_h->led_ontime[i] != 0 && app_h->led_onoff_count[i] >= app_h->led_offtime[i])
                {
                    gpio_output_reverse(app_h->led_map[i]);
                    app_h->led_onoff_count[i] = 0;
                    app_h->led_onoff[i] = LED_ON_FUNC;
                }
            }
        }

        app_h->led_repeat_count[i] += (uint16_t)step;
        if(app_h->led_repeat_count[i] >= app_h->led_repeat[i])
        {
            app_h->led_repeat_count[i] = 0;
            app_h->led_num_flash_count[i] = 0;
        }

        if(app_h->led_timeout[i] > 0)
        {
            if(app_h->led_timeout[i] > step)
                app_h->led_timeout[i]-=(uint16_t)step;
            else
            {
                app_h->led_timeout[i] = 0;
                app_h->led_event = LED_EVENT_END;
                //app_clear_led_event_action(i);
                //if(app_h->led_event_save)
                //    app_set_led_event_action(app_h->led_event_save);
                if(app_h->led_event==LED_EVENT_LOW_BATTERY)
                    app_bt_flag2_set(APP_FLAG2_LED_LOWPOWER, 0);
                if(app_h->led_event == LED_EVENT_POWER_OFF)
                    app_clear_led_event_action(i);
                else
                    app_set_led_event_action(app_h->led_event_save);
            }
        }
    }
}

void app_led_dump(void)
{
    int i;
    app_handle_t app_h = app_get_sys_handler();

    LOG_I(LED, "lp_flag:%d, last:%d, saved:%d\r\n", app_bt_flag2_get(APP_FLAG2_LED_LOWPOWER), 
                                                    app_h->led_event, 
                                                    app_h->led_event_save);
    
    for(i = 0; i < LED_NUM; i++)
    {
        LOG_I(LED, "map%d(GPIO%d): on:%d, off:%d, flash:%d, repeat:%d, timeout:%d\r\n", i, 
                   app_h->led_map[i],
                   app_h->led_ontime[i],
                   app_h->led_offtime[i],
                   app_h->led_num_flash[i],
                   app_h->led_repeat[i],
                   app_h->led_timeout[i]);
    }
}
