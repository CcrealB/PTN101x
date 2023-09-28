#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_at_types.h"
#include "bt_hfp_hf.h"

#ifdef CONFIG_IRDA
button_action_func *IRKey_handler;
#endif
volatile uint16_t sniffmode_wakeup_dly = 500; // when poweron, sleep delay 2s for led & printf
//#ifdef CONFIG_PRODUCT_TEST_INF
uint32_t btn_sum[BUTTON_BT_END][2];
//#endif
uint16_t button_adc_value;

#define BUTTON_ADC_FULL_VALUE   1023    //10bit

void button_adc_chan_config(uint8_t channel)
{
    if(channel == 1)
        REG_GPIO_0x06 = 0x0C;
    else if(channel == 2)
        REG_GPIO_0x07 = 0x0C;
    else if(channel == 4)
        REG_GPIO_0x12 = 0x0C;
    else if(channel == 6)
        REG_GPIO_0x14 = 0x0C;
    else if(channel == 7)
        REG_GPIO_0x15 = 0x0C;
}

int button_adc_key_get(uint16_t adc_value)
{
    uint8_t i;
    uint8_t key_step = 0;
    app_handle_t app_h = app_get_sys_handler();

    if((!app_h->button_mode_adc_sum) || (app_h->button_mode_adc_sum > 10)) return -1;

	if(adc_value < 10) return -1; //(adc_value>(BUTTON_ADC_FULL_VALUE-10))

    key_step = (BUTTON_ADC_FULL_VALUE/2)/app_h->button_mode_adc_sum;
    
    BTN_PRT("button_adc_key_step = %d, adc_value = %d\r\n",key_step,adc_value);
    
    for(i = 0; i < app_h->button_mode_adc_sum; i++)
    {
        if((adc_value < key_step) || (i && (adc_value >= key_step*(2*i-1)) && (adc_value < key_step*(2*i+1)))) 
            return (int)i;
    }
    
    return -1;  
}

uint64_t button_get_gpio_map( int *gpio_cnt )
{
    int i;
    uint64_t gpio_map = 0;
    app_handle_t app_h = app_get_sys_handler();
    
    for(i = GPIO0; i < GPIO_NUM; i++)
    {
        if ((app_h->button_mask & ((uint64_t)1 << i)) && (!gpio_input(i)))
        {
            gpio_map |= ((uint64_t)1 << (i-GPIO0));
            if( gpio_cnt != NULL )
                *gpio_cnt += 1;
        }
    }
        
    gpio_map &= app_h->button_mask;

    if(app_h->button_mode_adc_enable)
    {
    	#if 0
        i = button_adc_key_get(button_adc_value);
        if(i != -1)
        {
            gpio_map = 0;
            *gpio_cnt = 0;

            gpio_map |= (uint32)((1 << i) << BUTTON_MODE_ADC_KEY_SHIFT);
            gpio_map |= BUTTON_MODE_ADC_MASK;
            gpio_map |= (((uint32)app_h->button_mode_adc_chan) << BUTTON_MODE_ADC_CHAN_SHIFT);
            gpio_map |= (((uint32)app_h->button_mode_adc_sum) << BUTTON_MODE_ADC_SUM_SHIFT);
            
            if( gpio_cnt != NULL )
                *gpio_cnt += 1;

            BTN_PRT("button_adc_key:%d, value:%d\r\n", i, button_adc_value);
        }
		#endif
        SARADC_CRITICAL_CODE(1);
        saradc_set_chnl_busy(1);
        saradc_init(SARADC_MODE_CONTINUE, app_h->button_mode_adc_chan, 4);
        saradc_refer_select(0);
        SARADC_CRITICAL_CODE(0);
    }
    
    return gpio_map;
}

uint64_t button_detect_handler( void )
{
	app_env_handle_t  env_h = app_env_get_handle();
    uint8_t wakup_pin = app_env_get_pin_num(PIN_pwrBtn);
    static uint8_t button_active_det=0,button_active_press=0;
    uint64_t button_commit_info=0;
    uint64_t gpio_map;
    app_handle_t app_h;
    uint32_t gpio_cnt = 0;
    static uint32_t t0 = 0;
    uint32_t need_parse_flag = 0;
	static uint32_t power_on_5s_cnt = 0;
    

    if(app_bt_flag2_get(APP_FLAG2_5S_PRESS))
    {
		uint16_t enter_pairing_cnt;       
		enter_pairing_cnt = env_h->env_cfg.feature.enter_pairing_key_time;
        power_on_5s_cnt++;
		if( gpio_input(wakup_pin))
		{
            if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_PAIRING_AFTER_PWR_ON))
            {
                if(power_on_5s_cnt >= enter_pairing_cnt)
                {
    				app_bt_flag2_set(APP_FLAG2_5S_PRESS,0);
                    for(uint8_t idx=0; idx<BT_MAX_AG_COUNT; idx++)
                        bt_app_entity_free(idx,0);
                    os_printf("~cnt:%d,trap:%d\r\n",power_on_5s_cnt,enter_pairing_cnt);
    				power_on_5s_cnt = 0;
                    app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION, 0);
    				msg_put(MSG_ENTER_MATCH_STATE);
                }
                else
                {
                    os_printf("!cnt:%d,trap:%d\r\n",power_on_5s_cnt,enter_pairing_cnt);
                    power_on_5s_cnt = 0;
                    //msg_put(MSG_POWER_ON_START_CONN);
                    app_bt_flag2_set(APP_FLAG2_5S_PRESS,0);
                    if(app_get_env_key_num_total()==0)
                    {
                        for(uint8_t idx=0; idx<BT_MAX_AG_COUNT; idx++)
                            bt_app_entity_free(idx,0);
                        msg_put(MSG_ENTER_MATCH_STATE);
                    }
                }
            }
            app_bt_flag2_set(APP_FLAG2_5S_PRESS,0);
		}
      
        return 0;
    }
		
	
    
    app_h = app_get_sys_handler();
    
#if (CONFIG_EAR_IN == 1)
	if( app_h->btn_init_map != 0 
		|| (app_env_get_pin_enable(PIN_earInDet)&&!get_earin_status()))
#else
	if( app_h->btn_init_map != 0)
#endif
    {
        app_h->btn_init_map = button_get_gpio_map( NULL );
        goto exit_no_press;
    }

    gpio_map = button_get_gpio_map( (int *)&gpio_cnt );
    
    if( gpio_cnt == 0)
    {
        gpio_map = BUTTON_NONE;
    }
    else
    {
    #if ((CONFIG_CHARGE_EN == 1))
        app_charge_button_handle();
    #endif
        app_sleep_func(0);
    }

    switch( app_h->button_state )
    {
        case BUTTON_PRESS_NONE:
            if( gpio_map != BUTTON_NONE )
            {
                app_h->button_mode  = gpio_map;
                app_h->button_state = BUTTON_PRESS_DOWN;

                t0                  = os_get_tick_counter();
                BTN_PRT("press_down:%p:%p\r\n", gpio_map, app_h->press_thre);
            }
            app_h->button_press_count = 0;
    		app_h->button_long_press_count = 0;
            app_h->button_commit = BUTTON_NONE;
            break;

        case BUTTON_PRESS_DOWN:
            if( gpio_map != app_h->button_mode )
            {
                if( app_h->button_press_count >= app_h->long_thre)
                {
                    app_h->button_commit = BUTTON_LONG_PRESS | app_h->button_mode;
                    app_h->button_state  = BUTTON_PRESS_NONE;
                    need_parse_flag      = 1;
                    BTN_PRT("press_long:%d,%d\r\n", app_h->button_press_count, app_h->long_thre);
                }
                else if( app_h->button_press_count >= app_h->press_thre )
                {
                    app_h->button_state = BUTTON_PRESS_UP;
                    app_h->button_commit = app_h->button_mode | BUTTON_WAIT_DOUBLE_CLICK;
                    BTN_PRT("press_up:%d:%d\r\n", app_h->button_press_count, app_h->press_thre);
                }
                else
                {
                    app_h->button_state   = BUTTON_PRESS_NONE;
                    BTN_PRT("press_discard:%p:%p\r\n", app_h->button_press_count, app_h->press_thre);
                }
                app_h->button_mode        = BUTTON_NONE;
                app_h->button_press_count = 0;
            }
            else
            {
                app_h->button_press_count += os_get_tick_counter() - t0;
                t0                         = os_get_tick_counter();

                BTN_PRT("down:%p\r\n", app_h->button_press_count);
                if( app_h->button_press_count >= app_h->long_thre )
                {
                    BTN_PRT("long_press:%p:%p\r\n", app_h->button_press_count, app_h->long_thre);
                    app_h->button_commit = BUTTON_LONG_PRESS_NO_UP | app_h->button_mode;
                    app_h->button_state = BUTTON_PRESS_DOWN_LONG1;
                    app_h->button_press_count = 0;
                    need_parse_flag           = 1;
                }
            }
            break;

        case BUTTON_PRESS_UP:
            app_h->button_press_count += os_get_tick_counter() - t0;
            t0                         = os_get_tick_counter();

            if( app_h->button_press_count > app_h->double_thre )
            {
                BTN_PRT("button_click0:%d:%d\r\n", app_h->button_press_count, app_h->double_thre);

                app_h->button_commit     &= ~BUTTON_ACTION_MASK;
                app_h->button_commit     |= BUTTON_CLICK;
                app_h->button_mode        = BUTTON_NONE;
                app_h->button_state       = BUTTON_PRESS_NONE;
                app_h->button_press_count = 0;
                need_parse_flag           = 1;
            }
            else
            {
                if( gpio_map != BUTTON_NONE )
                {
                    if( gpio_map == (app_h->button_commit & BUTTON_MODE_MASK))
                        app_h->button_state = BUTTON_DOUBLE_PRESS_DOWN;
                    else
                    {
                        app_h->button_commit &= ~BUTTON_ACTION_MASK;
                        app_h->button_commit |= BUTTON_CLICK;
                        app_h->button_state   = BUTTON_PRESS_NONE;
                        need_parse_flag           = 1;
                        BTN_PRT("button_click1\r\n");
                    }
                    app_h->button_mode = gpio_map;
                    app_h->button_press_count = 0;
                }
            }
            break;

        case BUTTON_DOUBLE_PRESS_DOWN:
            if( gpio_map != app_h->button_mode )
            {
                if( app_h->button_press_count >= app_h->press_thre )
                {
                    app_h->button_state = BUTTON_DOUBLE_PRESS_UP;
                }
                else
                {
                    app_h->button_state   = BUTTON_NONE;
                    app_h->button_commit &= ~BUTTON_ACTION_MASK;
                    app_h->button_commit |= BUTTON_CLICK;
                    app_h->button_mode    = BUTTON_PRESS_NONE;
                    need_parse_flag       = 1;
                    BTN_PRT("button_click2\r\n");
                }
                app_h->button_press_count = 0;
            }
            else
            {
                app_h->button_press_count += os_get_tick_counter() - t0;
                t0                         = os_get_tick_counter();
            }
            break;

        case BUTTON_DOUBLE_PRESS_UP:
            #if 1//more click
            app_h->button_press_count += os_get_tick_counter() - t0;
            t0						  = os_get_tick_counter();	
            
            if(app_h->button_press_count < BUTTON_MORE_INTERVAL_TIME)
            {
                if(gpio_map != BUTTON_NONE )
                {
                    if(button_active_det == 0)
                    {
                        button_active_det=1;
                        button_active_press++;
                        app_h->button_press_count = 0;
                    }
                }
                else
                    button_active_det=0;
            }
            else
            {
                INFO_PRT("button_active_press=%d\n",button_active_press);
                switch (button_active_press)
                {
                    case BUTTON_TWO_CLICK:
                        app_h->button_press_count = 0;
                        app_h->button_commit = app_h->button_mode | BUTTON_DOUBLE_CLICK;
                        app_h->button_mode  = BUTTON_NONE;
                        app_h->button_state = BUTTON_PRESS_NONE;
                        need_parse_flag     = 1;
                        BTN_PRT("double_click\r\n");
                        break;
                    
                    case BUTTON_THREE_CLICK:
                        BTN_PRT("3_click\r\n");
                        //msg_put_send(MSG_BUTTON_ACTION, BUTTON_BT_PREV);
                        app_h->button_commit = app_h->button_mode |BUTTON_THREE_CLICK_FLAG;
                        break;
                    
                    case BUTTON_FOUR_CLICK:
                        BTN_PRT("4_click\r\n");
                        //msg_put_send(MSG_BUTTON_ACTION,BUTTON_BT_PREV);
                        app_h->button_commit = app_h->button_mode |BUTTON_FOUR_CLICK_FLAG;
                        break;
                    
                    case BUTTON_FIVE_CLICK:
                        BTN_PRT("5_click\r\n");
                        //msg_put_send(MSG_BUTTON_ACTION,BUTTON_BT_VOL_P);
                        app_h->button_commit = app_h->button_mode |BUTTON_FIVE_CLICK_FLAG;
                        break;
                    
                    case BUTTON_SIX_CLICK:
                        BTN_PRT("6_click\r\n");
                        //msg_put_send(MSG_BUTTON_ACTION,BUTTON_BT_VOL_M);
                        app_h->button_commit = app_h->button_mode |BUTTON_SIX_CLICK_FLAG;
                        break;	
                    case BUTTON_SEVEN_CLICK:
                        BTN_PRT("7_click\r\n");
                        //msg_put_send(MSG_BUTTON_ACTION,BUTTON_BT_VOL_M);
                        app_h->button_commit = app_h->button_mode |BUTTON_SEVEN_CLICK_FLAG ;
                        break;
                    case BUTTON_EIGHT_CLICK:
                        BTN_PRT("8_click\r\n");
                        //msg_put_send(MSG_BUTTON_ACTION,BUTTON_BT_VOL_M);
                        app_h->button_commit = app_h->button_mode |BUTTON_EIGHT_CLICK_FLAG;
                        break;
                    
                    default:
                        break;
                }
                
                if (button_active_press > BUTTON_TWO_CLICK)
                {
                    button_active_press = 0;
                    need_parse_flag     = 1;
                    app_h->button_press_count = 0;
                    app_h->button_mode  = BUTTON_NONE;
                    //app_h->button_commit = BUTTON_NONE;
                    app_h->button_state = BUTTON_PRESS_NONE;
                }
            }
            #else
            app_h->button_press_count = 0;
            app_h->button_commit = app_h->button_mode | BUTTON_DOUBLE_CLICK;
            app_h->button_mode  = BUTTON_NONE;
            app_h->button_state = BUTTON_PRESS_NONE;
            need_parse_flag     = 1;
            BTN_PRT("double_click\r\n");
            #endif
            break;

        case BUTTON_PRESS_DOWN_LONG1:
            if( gpio_map != app_h->button_mode )
            {
                app_h->button_commit = BUTTON_LONG_PRESS | app_h->button_mode;
                app_h->button_state       = BUTTON_PRESS_NONE;
                app_h->button_mode        = BUTTON_NONE;
                app_h->button_press_count = 0;
    			need_parse_flag       = 1;
            }
            else
            {
                app_h->button_press_count += os_get_tick_counter() - t0;
                t0                           = os_get_tick_counter();

                app_h->button_commit = BUTTON_NONE;
                if( app_h->button_press_count >= app_h->repeat_thre)
                {
                    app_h->button_commit = BUTTON_CONTINUE | app_h->button_mode;
                    app_h->button_long_press_count += app_h->button_press_count;
                    app_h->button_press_count       = 0;
                    need_parse_flag                 = 1;
                    BTN_PRT("button_continue\r\n");
                }
                if( app_h->button_long_press_count >= app_h->vlong_thre)//(app_h->vlong_thre - app_h->long_thre) )
                {
                    app_h->button_commit |= BUTTON_LONG_LONG_PRESS |
                        app_h->button_mode;
                    app_h->button_state = BUTTON_PRESS_DOWN_LONG2;
                    app_h->button_long_press_count = 0;
                    app_h->button_press_count = 0;
                    need_parse_flag           = 1;
                    BTN_PRT("button_long_long\r\n");
                }
            }
            break;

        case BUTTON_PRESS_DOWN_LONG2:
            if( gpio_map != app_h->button_mode )
            {
                app_h->button_state       = BUTTON_PRESS_NONE;
                app_h->button_mode        = BUTTON_NONE;
                app_h->button_press_count = 0;
                BTN_PRT("long_long_discard\r\n");
            }
            else
            {
                app_h->button_press_count += os_get_tick_counter() - t0;
                t0                         = os_get_tick_counter();

                app_h->button_commit = BUTTON_NONE;
                if( app_h->button_press_count >= app_h->repeat_thre )
                {
                    app_h->button_commit = BUTTON_CONTINUE | app_h->button_mode;
                    app_h->button_press_count = 0;
                    need_parse_flag           = 1;
                }
            }
            break;

        default:
            break;
    }

    if(need_parse_flag)
    {
        button_commit_info = app_h->button_commit;
        BTN_PRT("commit:%p\r\n", button_commit_info);
		sniffmode_wakeup_dly = 1000;
    }

exit_no_press:
    return button_commit_info;
}

static uint64_t button_parse(uint64_t commit_info)
{
    uint32_t i;
    uint32_t btn_cnt;
    uint64_t btn_map;
    uint64_t btn_action;
    app_handle_t app_h;
    uint64_t action_match_flag;
    uint64_t btn_type_match_flag;
    uint64_t btn_pos_map_match_flag;

    app_h = app_get_sys_handler();
    btn_map = commit_info & BUTTON_MODE_MASK;
    btn_action = commit_info & BUTTON_ACTION_MASK;

    if((btn_map == BUTTON_NONE) || (btn_action == BUTTON_WAIT_DOUBLE_CLICK))
    {
        return BUTTON_NONE;
    }

    btn_cnt = app_h->button_count;
    for( i = 0; i < btn_cnt; i++ )
    {
        /* double check button validation*/
        if((BUTTON_MUL_CLICK_FLAG == (commit_info & BUTTON_MUL_CLICK_MASK))
            || (BUTTON_MUL_CLICK_FLAG == (app_h->button_code[i] & BUTTON_MUL_CLICK_MASK)))
        { //more click
            action_match_flag = (btn_action == (app_h->button_code[i] & BUTTON_ACTION_MASK));
        }
        else
        {
            action_match_flag = (btn_action & (app_h->button_code[i] & BUTTON_ACTION_MASK));
        }
        
        btn_type_match_flag = (app_h->button_type & (app_h->button_code[i] & BUTTON_TYPE_MASK));
        btn_pos_map_match_flag = (btn_map == (app_h->button_code[i] & BUTTON_MODE_MASK));

        if(btn_pos_map_match_flag && action_match_flag && btn_type_match_flag)
        {
            BTN_PRT("button_value:%d/%d\r\n", i, btn_cnt);
            return i;
        }

        if(btn_pos_map_match_flag || action_match_flag || btn_type_match_flag)
        {
            BTN_PRT("btn%d:action:%p-%p;type:%p-%p; bit_map:%p-%p;%p\r\n"
                    , i
                    , action_match_flag
                    , btn_action
                    , btn_type_match_flag
                    , app_h->button_type
                    , btn_pos_map_match_flag
                    , btn_map
                    , app_h->button_code[i]);
        }
    }

    return BUTTON_NONE;
}

void button_action(uint64_t commit_info)
{
    app_handle_t app_h;
    uint64_t mode;
    
    app_h = app_get_sys_handler();
    mode  = button_parse(commit_info);

    if(mode)
        LOG_I(BTN,"parse_msg_key:%p,mode:%x\r\n", commit_info,mode);

    if(mode)
    {
        if( app_h->button_handler[mode] )
        {
            app_h->button_handler[mode]();
#ifdef CONFIG_PRODUCT_TEST_INF
            LOG_D(BTN,"button_action,flag:0x%x,avrcp:%d\r\n",app_h->flag_sm1, avrcp_has_connection());

            if(avrcp_has_connection())
            {
                int i = 0;
                
                for(i = 0; i < BUTTON_BT_END; i++)
                {
                    if(btn_sum[i][0] == 0)
                    {
                        btn_sum[i][0] = mode;
                        btn_sum[i][1]++;
                        break;
                    }
                    else if(btn_sum[i][0] == mode)
                    {
                        btn_sum[i][1]++;
                        break;
                    }
                }
            }
            else if(hfp_has_connection())
            {
                LOG_D(BTN,"~~button_action:hf_cmd_trans_buttoncode\r\n");
                hf_cmd_trans_buttoncode(mode);
            }
#endif
        }
        app_h->button_commit = BUTTON_NONE;
    }
}

#ifdef CONFIG_IRDA
CONST static button_action_func IRKey_BT_handler[IR_KEY_END]=
    {
        app_button_playpause_action,
        app_button_volm_action,
        app_button_volp_action,
        app_button_prev_action,
        app_button_next_action,
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

#ifdef CONFIG_IRDA
void button_set_irkey_handler(button_action_func *handler)
{
    IRKey_handler = handler;
}
#endif

void button_scanning(void)
{
    MSG_T msg;
    uint64_t button_commit_info;

    button_commit_info = button_detect_handler();
    if(button_commit_info)
    {
        msg.id = MSG_KEYPRESS;
        msg.hdl = 0;
        msg.arg = button_commit_info & 0xffffffff;
        msg.arg2 = (button_commit_info >> 32) & 0xffffffff;

        msg_lush_put(&msg);

        button_commit_info = 0;
    }
}

void button_common_register(const uint64_t *button_code,
                                     const button_action_func *button_handler,
                                     int button_num )
{
    uint32_t i;
    uint32_t id;
    uint32_t pos;
    uint64_t gpio_map;
    app_handle_t app_h = app_get_sys_handler();

    /*config gpio*/
    app_h->button_mask = 0;
    
    for(i = 0; i < button_num; i ++)
    {
        gpio_map = button_code[i] & BUTTON_MODE_MASK;
        __asm volatile( "b.ff1 %0, %1;"
                        :"=r"(pos)
                        :"r"(gpio_map)
                        :"r26"
                        );
        id = pos - 1;

#ifdef BTN_DEBUG
        if(gpio_map & (~(1 << id)))
        {
            BTN_PRT("combine_button????\r\n");
        }
#endif
        if(!(app_h->button_mask & (1 << id)))
        {
            /* init gpio*/
            gpio_config( id + GPIO0, 0);
            app_h->button_mask |= ( 1 << id );
        }
		app_h->button_code[i]	 = button_code[i];
		app_h->button_handler[i] = button_handler[i];
    }

    /*register*/
    //app_h->button_code    = button_code;
    // app_h->button_handler = button_handler;
    app_h->button_count   = button_num;
}

void button_bt_common_register(const button_action_func *button_handler,
                                         int button_num )
{
    int32_t i;
    int32_t id_L;
    int32_t id_H;
    int32_t pos=0;
    uint32_t adc_enable;
    uint32_t gpio_map_L;
    uint32_t gpio_map_H;
    app_handle_t app_h = app_get_sys_handler();
	app_env_handle_t  env_h = app_env_get_handle();

    LOG_D(BTN,"button_bt_common_register\r\n");
    
    /*config gpio*/
    app_h->button_mask = 0;
    app_h->button_mode_adc_enable = 0;
	app_h->button_mode_adc_chan = 0xff;
    
    for(i = 0; i < button_num; i ++)
    {
    #ifdef CONFIG_PRODUCT_TEST_INF
        btn_sum[i][0] = 0;
        btn_sum[i][1] = 0;
    #endif
        adc_enable = env_h->env_cfg.button_code[i] & BUTTON_MODE_ADC_MASK;
    
        if(adc_enable == BUTTON_MODE_ADC_MASK)
        {
            if(!app_h->button_mode_adc_enable)
            {
                button_adc_value = BUTTON_ADC_FULL_VALUE;
                app_h->button_mode_adc_enable = 1;
                app_h->button_mode_adc_chan = (uint8_t)((env_h->env_cfg.button_code[i] & BUTTON_MODE_ADC_CHAN_MASK) >> BUTTON_MODE_ADC_CHAN_SHIFT);
                //app_h->button_mode_adc_sum = (uint8_t)((env_h->env_cfg.button_code[i] & BUTTON_MODE_ADC_SUM_MASK) >> BUTTON_MODE_ADC_SUM_SHIFT);
                button_adc_chan_config(app_h->button_mode_adc_chan);
                BTN_PRT("ADC_mode buttion code:%x\r\n",env_h->env_cfg.button_code[i]);
            }
        }
        else
        {
            gpio_map_L = (env_h->env_cfg.button_code[i] & BUTTON_MODE_MASK)&0xffffffff;
            gpio_map_H = ((env_h->env_cfg.button_code[i] & BUTTON_MODE_MASK)>>32)&0xffffffff;
			do{
	            __asm volatile( "b.ff1 %0, %1;"
	                            :"=r"(pos)
	                            :"r"(gpio_map_L)
	                            :"r26"
	                            );
	            id_L = pos - 1;
	            __asm volatile( "b.ff1 %0, %1;"
	                            :"=r"(pos)
	                            :"r"(gpio_map_H)
	                            :"r26"
	                            );
	            id_H = pos - 1 + 32;

#ifdef BTN_DEBUG
	            if((gpio_map_L& (~(1 << id_L)))||(gpio_map_H& (~(1 << (id_H-32)))))
	            {
	                BTN_PRT("combine_button????\r\n");
	            }
#endif
	            if((id_L >= 0)&& (!(app_h->button_mask & ((uint64_t)1 << id_L))))
	            {
	                gpio_config( id_L+GPIO0, 3);  // input pull_up
	                app_h->button_mask |= ( (uint64_t)1 << id_L);
	            }
                
	            if((id_H >= 32)&& (!(app_h->button_mask & ((uint64_t)1 << id_H))))
	            {
	                gpio_config( id_H+GPIO0, 3);  // input pull_up
	                app_h->button_mask |= ( (uint64_t)1 << id_H);
	            }
                
				if(id_H >= 32)
					gpio_map_H &= (~(1 << (id_H-32)));
                
				if(id_L >= 0)
					gpio_map_L &= (~(1 << id_L));
                
				//BTN_PRT("Ci:%d,H:%x,L:%x\r\n",i,gpio_map_H,gpio_map_L);
			}while(gpio_map_L|gpio_map_H); //for combine_button
        }
        
		app_h->button_code[i] = env_h->env_cfg.button_code[i];
		app_h->button_handler[i] = button_handler[i];
    }

    /*register*/
    //app_h->button_code    = button_code;
    //app_h->button_handler = button_handler;
    app_h->button_count   = button_num;
}

void app_button_type_set( uint64_t type )
{
    app_handle_t app_h = app_get_sys_handler();

    app_h->button_type = type;
    
    return;
}

static int app_button_volp_action( void );
static int app_button_volm_action( void );
static int app_button_playpause_action( void );
static int app_button_next_action( void );
static int app_button_prev_action( void );
static int app_button_conn_disconn(void);
#if (CONFIG_AUD_EQS_SUPPORT==0)
static int app_button_stop_action( void );
#endif
static  int app_button_rewind_action( void );
static  int app_button_forward_action( void );
static  int app_button_vol_mute_action( void );
static  int app_button_clear_memory( void );
static  int app_button_enter_dut_mode( void );
static  int app_button_lang_change( void );
#ifdef CONFIG_BLUETOOTH_HFP
static  int app_button_redial_last_number( void );
static  int app_button_micvolp_action( void );
static  int app_button_micvolm_action( void );
static  int app_button_micvol_mute_action( void );
static  int app_button_voice_dial_set( void );
//static  int app_button_hf_transfer_toggle( void );
static  int app_button_twc_hold_accept( void );
static  int app_button_twc_hung_accept( void );
static  int app_button_twc_reject_hold( void );
static int app_button_eSCO_A_B_swap(void);
static int app_button_eSCO_A_B_TWC_swap(void);
static  int app_button_hf_transfer_toggle( void );
static int app_button_hold_swtch(void);
#endif
#if (CONFIG_AUD_EQS_SUPPORT==1)
static int app_button_eq_pram_change(void);
#endif
static int app_button_game_mode(void);

//CONST static uint32_t bt_button_code[BUTTON_BT_END] =
//{
//    0,	//NULL,
//    BTN_CODE(BUTTON_CLICK,     GPIO11,   BUTTON_TYPE_ALL),	//app_button_playpause_action,
//    0,	//app_button_next_action,
//    0,	//app_button_prev_action,
//    0,	//app_button_rewind_action,
//    0,	//app_button_forward_action,
//    0,	//app_button_volp_action,
//    0,	//app_button_volm_action,
//    0,	//app_button_vol_mute_action,
//    0,	//app_button_match_action,
//    BTN_CODE(BUTTON_LONG_PRESS,     GPIO8,   BUTTON_TYPE_ALL),//0,	//app_button_powerdown,
//    0,//BTN_CODE(BUTTON_CLICK,     GPIO8,   BUTTON_TYPE_HFP),	//app_button_hfack_action,
//    0,	//app_button_reject_action,
//    0,	//app_button_redial_last_number,
//    0,	//app_button_micvolp_action,
//    0,	//app_button_micvolm_action,
//    0,	//app_button_micvol_mute_action,
//    0,	//app_button_voice_dial_set,
//    0,	//app_button_hf_transfer_toggle,
//    0,	//app_button_conn_disconn,
//#if (defined(BT_SD_MUSIC_COEXIST))
//    0,	//app_button_stop_action,
//#else
//    0,	//null
//#endif
//    0,	//app_button_clear_memory,
//    0,	//app_button_enter_dut_mode,
//    0,	//app_button_lang_change,
//    0,	//app_button_twc_hold_accept,
//    0,	//app_button_twc_hung_accept,
//    0,	//app_button_twc_reject_hold,
//    0,	//BUTTON_BT_TWC_SWTCH_WAY,
//    0,	//BUTTON_BT_2PHONES_SWTCH,
//    0,	//BUTTON_STEREO,
//    0,	//BUTTON_RESERVED1,
//    0,	//BUTTON_RESERVED2,
//    0,	//BUTTON_RESERVED3,
//};

CONST static button_action_func bt_button_handler[BUTTON_BT_END] =
{
    NULL,
    app_button_playpause_action,
    app_button_next_action,
    app_button_prev_action,
    app_button_rewind_action,
    app_button_forward_action,
    app_button_volp_action,
    app_button_volm_action,
    app_button_vol_mute_action,
    app_button_match_action,
    app_button_powerdown,
#ifdef CONFIG_BLUETOOTH_HFP
    app_button_hfack_action,
    app_button_reject_action,
    app_button_redial_last_number,
    app_button_micvolp_action,
    app_button_micvolm_action,
    app_button_micvol_mute_action,
    app_button_voice_dial_set,
    app_button_hf_transfer_toggle,//app_button_hf_transfer_toggle,
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
    app_button_conn_disconn,
 #if (CONFIG_AUD_EQS_SUPPORT==1)
    app_button_eq_pram_change,
 #else
    app_button_stop_action,
  #endif
    app_button_clear_memory,
    app_button_enter_dut_mode,
    app_button_lang_change,
#ifdef CONFIG_BLUETOOTH_HFP
    app_button_twc_hold_accept,
    app_button_twc_hung_accept,
    app_button_twc_reject_hold,
#else
    NULL,
    NULL,
    NULL,
#endif
#ifdef CONFIG_BLUETOOTH_HFP
    app_button_eSCO_A_B_TWC_swap,	//BUTTON_BT_TWC_SWTCH_WAY,
    app_button_eSCO_A_B_swap,//BUTTON_BT_2PHONES_SWTCH,
    system_work_mode_change_button,
    app_button_hold_swtch,	//BUTTON_RESERVED1,
#else
	NULL,
	NULL,
    system_work_mode_change_button,
	NULL,
#endif
    app_button_game_mode,	//BUTTON_RESERVED2,
    NULL,	//BUTTON_RESERVED3,
};

CONST static button_action_func linein_button_handler[BUTTON_BT_END] =
{
    NULL,
	app_button_playpause_action,
    NULL,
    NULL,
    NULL,
    NULL,
    app_button_volp_action,
    app_button_volm_action,
    app_button_vol_mute_action,
    NULL,
    app_button_powerdown,
#ifdef CONFIG_BLUETOOTH_HFP
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,//app_button_hf_transfer_toggle,
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
    NULL,
#if (defined(BT_SD_MUSIC_COEXIST))
    NULL,
#else
    NULL,
#endif
    app_button_clear_memory,
    app_button_enter_dut_mode,
    app_button_lang_change,
#ifdef CONFIG_BLUETOOTH_HFP
    NULL,
    NULL,
    NULL,
#else
    NULL,
    NULL,
    NULL,
#endif
#ifdef CONFIG_BLUETOOTH_HFP
    NULL,	//BUTTON_BT_TWC_SWTCH_WAY,
    NULL,//BUTTON_BT_2PHONES_SWTCH,
#else
	NULL,
	NULL,
#endif
    NULL,	//BUTTON_STEREO,
    NULL,	//BUTTON_RESERVED1,
    NULL,	//BUTTON_RESERVED2,
    NULL,	//BUTTON_RESERVED3,
};
#if (CONFIG_APP_MP3PLAYER == 1)
CONST static button_action_func player_button_handler[BUTTON_BT_END] =
{
    NULL,
    app_player_button_play_pause,
    app_player_button_next,
    app_player_button_prev,
    NULL,
    NULL,
    app_player_button_setvol_up,
    app_player_button_setvol_down,
    app_button_vol_mute_action,
    NULL,
    NULL,//app_button_powerdown,

#ifdef CONFIG_BLUETOOTH_HFP
    app_button_hfack_action,
    app_button_reject_action,
    app_button_redial_last_number,
    app_button_micvolp_action,
    app_button_micvolm_action,
    app_button_micvol_mute_action,
    app_button_voice_dial_set,
    app_button_hf_transfer_toggle,  
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif     

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

#ifdef CONFIG_BLUETOOTH_HFP
    app_button_twc_hold_accept,
    app_button_twc_hung_accept,
    app_button_twc_reject_hold,
    app_button_eSCO_A_B_TWC_swap,	//BUTTON_BT_TWC_SWTCH_WAY,
    app_button_eSCO_A_B_swap,//BUTTON_BT_2PHONES_SWTCH,
    system_work_mode_change_button,
    app_button_hold_swtch,	//BUTTON_RESERVED1,
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    system_work_mode_change_button,
    NULL,
#endif  
    NULL,	//BUTTON_RESERVED2,
    NULL,	//BUTTON_RESERVED3,
};
#endif
extern uint32_t avrcp_current_support_vol_syn(void);
extern void current_send_volume_change_response(uint8_t volume_value);
extern uint8_t get_tg_volume_value(void);
extern BOOL is_ag_support_feature(uint8_t feature);
extern BOOL is_voice_recog_status_on(void);
extern void set_voice_recog_status(BOOL status);
extern result_t bt_a2dp_aac_stream_sync(uint32_t type);

#if 0//*yuan
static void app_game_norm_mode_switch(void)
{
    /*if(!get_aud_mode_flag(AUD_FLAG_GAME_MODE))
    {
        set_aud_mode_flag(AUD_FLAG_GAME_MODE);
    }
    else
    {
        unset_aud_mode_flag(AUD_FLAG_GAME_MODE);
    }*/

    #ifdef A2DP_MPEG_AAC_DECODE
    if(A2DP_CODEC_MPEG_AAC == a2dp_get_codec_type())
    {
        //aud_fade_in_out_state_set(AUD_FADE_OUT, 0);
        //audio_dac_ana_mute(1);
        bt_a2dp_aac_stream_sync(0xE);
    }
    else
    #endif
    {
    	sbc_discard_uselist_node();  //discard all the old sbc note avoid noise
    	sbc_init_adjust_param();
    	set_flag_sbc_buffer_play(0);
    }
    //os_printf("game mode:%d\r\n",get_aud_mode_flag(AUD_FLAG_GAME_MODE));  
}
#endif

static int app_button_game_mode(void)
{
    if(!get_aud_mode_flag(AUD_FLAG_GAME_MODE))
    {
        set_aud_mode_flag(AUD_FLAG_GAME_MODE);
    }
    else
    {
        unset_aud_mode_flag(AUD_FLAG_GAME_MODE);
    }
    os_printf("game mode:%d\r\n",get_aud_mode_flag(AUD_FLAG_GAME_MODE));  
//yuan	start_wave_and_action(APP_WAVE_FILE_ID_ENTER_GAMING_MODE,app_game_norm_mode_switch);
    return 0;
}

int app_button_match_action(void)
{

    uint8_t i;
    
    /* bt_app_management */
    for(i = 0; i < BT_MAX_AG_COUNT; i++)
        bt_app_entity_set_event(i,SYS_BT_BOTH_SCANNING_EVENT);
    return 0;
    
#if 0
    app_handle_t app_h = app_get_sys_handler();
	
	#ifdef BEKEN_DEBUG
	os_printf("app_button_match_action\r\n");
	#endif
	
	if( app_bt_flag1_get( APP_FLAG_LINEIN ) 
		|| app_bt_flag1_get(APP_FLAG_DUT_MODE_ENABLE))
		return 0;
	
    app_sleep_func(0);
    if(app_bt_flag1_get (APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
    {
        jtask_stop(app_h->app_auto_con_task);
        app_h->reconn_num_flag = 0;
        app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
    }

#if 0
    #ifdef CONFIG_BLUETOOTH_AVRCP
        avrcp_cmd_disconnect();
    #endif

    #ifdef CONFIG_BLUETOOTH_A2DP
        a2dp_cmd_disconnect();
    #endif

    #ifdef CONFIG_BLUETOOTH_HFP
        hf_cmd_disconnect();
    #endif

    #ifdef CONFIG_BLUETOOTH_HSP
        hs_cmd_disconnect();
    #endif

    #if CONFIG_BLUETOOTH_HID
        hid_cmd_disconnect();
    #endif
#endif
	//
    if(hci_get_acl_link_count(app_h->unit))
    {
        os_printf("-----------match button set--------------\r\n");
        app_bt_flag2_set( APP_FLAG2_MATCH_BUTTON, 1);
   	    bt_all_acl_disconnect(app_h->unit);
    }
    else
    {
        if(app_h->unit)
            bt_unit_set_scan_enable(app_h->unit,
                    HCI_INQUIRY_SCAN_ENABLE | HCI_PAGE_SCAN_ENABLE);

        app_bt_flag1_set((APP_MATCH_FLAG_SET),0);
    }

    backend_unit_remove_name(); //remove all the remote name on list
    app_set_led_event_action( LED_EVENT_MATCH_STATE);
    app_button_type_set(BUTTON_TYPE_NON_HFP);
	if(app_check_bt_mode(BT_MODE_1V1|BT_MODE_1V2|BT_MODE_DM_1V1|BT_MODE_BLE))
    app_wave_file_play_start(APP_WAVE_FILE_ID_ENTER_PARING);
    return 0;
#endif
}   

//************************************************
static int app_button_volp_action(void)
{
//    app_handle_t app_h = app_get_sys_handler();
    LOG_I(BTN,"volp\r\n");
/*
    if( app_bt_flag1_get( APP_FLAG_LINEIN ) ){
        app_h->linein_vol++;
        if (!app_bt_flag2_get(AP_FLAG2_LINEIN_MUTE)){
        	aud_volume_mute(0);
			aud_PAmute_operation(0);
        }
        if(app_h->linein_vol > AUDIO_VOLUME_MAX){
            app_h->linein_vol = AUDIO_VOLUME_MAX;
		}
        #if A2DP_ROLE_SOURCE_CODE || (CONFIG_CUSTOMER_ENV_SAVE_VOLUME == 1)
			a2dp_volume_init(app_h->linein_vol);
        #endif
        if(! app_wave_playing())	aud_dac_set_volume(app_h->linein_vol);
      	if(app_h->linein_vol == AUDIO_VOLUME_MAX)	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MAX);
    }
*/
#ifdef CONFIG_BLUETOOTH_HFP
    else if(hfp_has_sco())
    {
        hf_cmd_set_vgs(1);
    }
    else
#endif
	#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
    a2dp_volume_adjust(1);	// VOL+
    if(avrcp_has_connection() && app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_VOLUME_SYNC)) {
        current_send_volume_change_response(get_tg_volume_value());
    }
	#else
    if(a2dp_has_music()){
        a2dp_volume_adjust(1);
        if(app_bt_flag1_get(APP_FLAG_MUSIC_PLAY)
		   && app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_VOLUME_SYNC)
		   && avrcp_current_support_vol_syn()){
            current_send_volume_change_response(get_tg_volume_value());
        }
    }
	#endif

    return 0;
}

//************************************************************
static int app_button_volm_action(void)
{
//  app_handle_t app_h = app_get_sys_handler();
    LOG_I(BTN,"volm\r\n");
/*yuan++
    if( app_bt_flag1_get( APP_FLAG_LINEIN ) ){
        app_h->linein_vol --;
        if(app_h->linein_vol <= AUDIO_VOLUME_MIN){
            app_h->linein_vol = AUDIO_VOLUME_MIN;
            if(! app_wave_playing()){
                aud_volume_mute(1);
				aud_PAmute_operation(1);
            }
        }
	#if (CONFIG_CUSTOMER_ENV_SAVE_VOLUME == 1)
		a2dp_volume_init(app_h->linein_vol);
    #endif
        if(! app_wave_playing())	aud_dac_set_volume(app_h->linein_vol);
		if(app_h->linein_vol == AUDIO_VOLUME_MIN)	app_wave_file_play_start(APP_WAVE_FILE_ID_VOL_MIN);
    }
*/
#ifdef CONFIG_BLUETOOTH_HFP
    else if(hfp_has_sco()){
        hf_cmd_set_vgs(0);
    }else
#endif
#ifdef APPLE_IOS_VOL_SYNC_ALWAYS
    a2dp_volume_adjust(0);	// VOL-
    if(avrcp_has_connection() && app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_VOLUME_SYNC)){
        current_send_volume_change_response(get_tg_volume_value());
    }
#else
    if(a2dp_has_music()){
        a2dp_volume_adjust(0);
        if(app_bt_flag1_get(APP_FLAG_MUSIC_PLAY)
		   && app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_VOLUME_SYNC)
		   && avrcp_current_support_vol_syn()) {
            current_send_volume_change_response(get_tg_volume_value());
        }
    }
#endif

    return 0;
}

static int app_button_playpause_action( void )
{
#if (CONFIG_EAR_IN == 0)
    app_handle_t app_h = app_get_sys_handler();
#endif
    uint32_t opcode;
    int ret = -1;
    
    LOG_I(BTN,"app_button_playpause_action\r\n");

    if(is_voice_recog_status_on()) 
    {
        hf_cmd_set_voice_recog(0);

        LOG_I(BTN,"app_button_playpause_action,close voice_recog.\r\n");
        return TRUE;
    }

    if(avrcp_current_is_connected()
       && !avrcp_current_is_processing())
    {
        if(!a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE))
        {
            opcode = AVC_OP_PLAY;
            LOG_I(BTN,"play\r\n");
            a2dp_current_set_flag(APP_BUTTON_FLAG_PLAY_PAUSE);
        }
        else
        {
        #if (CONFIG_EAR_IN == 1)
            Set_PlayPause_Flag_Operate(EAR_PLAYPAUSE_OPERATE_FLAG, 0);
        #endif
            opcode = AVC_OP_PAUSE;
            os_printf("pause\r\n");
            a2dp_current_clear_flag(APP_BUTTON_FLAG_PLAY_PAUSE);
        }
		
		#if (CONFIG_EAR_IN == 0)
		app_h->flag_sm1 ^= APP_BUTTON_FLAG_PLAY_PAUSE;
        #endif
		
        #ifdef CONFIG_BLUETOOTH_AVRCP
        avrcp_current_send_opcode((void *)opcode);
        #endif
        
        #ifdef OP_TIMEOUT_HANDLE
        jtask_stop(app_h->app_auto_con_task);
        jtask_schedule(app_h->app_auto_con_task,
                       2000,
                       app_button_timeout_action,
                       NULL);
        #endif

        ret = 0;
    }
    else if (app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AUX_MODE_PAUSE_MUTE)
    		&& app_bt_flag1_get(APP_FLAG_LINEIN))
    {
    	if (app_bt_flag2_get(AP_FLAG2_LINEIN_MUTE))
    	{
    		app_bt_flag2_set(AP_FLAG2_LINEIN_MUTE, 0);
			aud_dac_close();
			linein_audio_open();
			app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
    	}
    	else
    	{
    		app_bt_flag2_set(AP_FLAG2_LINEIN_MUTE, 1);
			os_delay_ms(100);
			linein_audio_close();
			app_set_led_event_action(LED_EVENT_LINEIN_PAUSE);
    	}
    }
    
    (void)opcode;
    return ret;
}

int app_button_playpause_action_caller( int play_pause )
{
    int ret = -1;

    ret = app_button_playpause_action();

    return ret;
}

static int app_button_common_action( uint32_t opcode )
{
    //app_handle_t app_h = app_get_sys_handler();

    if(avrcp_has_connection()
        && !avrcp_is_cmd_processing())
    {
        //jtask_stop(app_h->app_auto_con_task);

        #ifdef CONFIG_BLUETOOTH_AVRCP
        avrcp_current_send_opcode((void *)opcode);
        #endif

        return 0;
    }
    else
        return -1;
}

static int app_button_next_action( void )
{
    LOG_I(BTN,"app_button_next_action\r\n");

    return app_button_common_action(AVC_OP_NEXT);
}

static  int app_button_prev_action( void )
{
    LOG_I(BTN,"app_button_prev_action\r\n");
    return app_button_common_action(AVC_OP_PREV);
}

static  int app_button_rewind_action( void )
{
    LOG_I(BTN,"app_button_rewind_action\r\n");

    return app_button_common_action(AVC_OP_REWIND);
}

static  int app_button_forward_action( void )
{
    LOG_I(BTN,"app_button_forward_action\r\n");

    return app_button_common_action(AVC_OP_FAST_FORWARD);
}

static  int app_button_vol_mute_action( void )
{
    LOG_I(BTN,"vol_mute\r\n");

#if (CONFIG_CUSTOMER_BUTTON_HID_MODE == 1)
	app_customer_hid_switch_mode();
#else
    app_handle_t app_h = app_get_sys_handler();

    if( app_h->volume_store & 0x40 )
    {
        app_h->volume_store &= ~0x40;
        aud_volume_mute(0);
    }
    else
    {
        app_h->volume_store |= 0x40;
        app_wave_file_play_stop();
        aud_volume_mute(1);
    }
#endif

    return 0;
}

static int app_button_clear_memory( void )
{
    LOG_I(BTN,"app_button_clear_memory\r\n");

    uint8_t cmd[24];
    app_handle_t app_h = app_get_sys_handler();

    app_set_led_event_action( LED_EVENT_FM_SCAN );
//yuan++	app_wave_file_play_start( APP_WAVE_FILE_ID_CLEAR_MEMORY );

    if(app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION))
    {
        jtask_stop(app_h->app_auto_con_task);
        app_bt_flag1_set((APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);
    }

    if(app_bt_flag1_get(APP_MATCH_FLAG_SET))
    {
        app_bt_flag2_set( APP_FLAG2_MATCH_BUTTON, 1);

    #ifdef CONFIG_BLUETOOTH_AVRCP
        avrcp_cmd_disconnect();
    #endif

    #ifdef CONFIG_BLUETOOTH_A2DP
        a2dp_cmd_disconnect();
    #endif

    #ifdef CONFIG_BLUETOOTH_HFP
        hf_cmd_disconnect();
    #endif

    #ifdef CONFIG_BLUETOOTH_HSP
        hs_cmd_disconnect();
    #endif

	#if CONFIG_BLUETOOTH_HID
        if(app_env_check_HID_profile_enable())
            hid_cmd_disconnect();
    #endif
    
        bt_all_acl_disconnect(app_h->unit);
    }

    memcpy( &cmd[0], (uint8_t *)&app_h->remote_btaddr, sizeof(btaddr_t));
    cmd[6] = 0x01;
    hci_send_cmd( app_h->unit, HCI_CMD_DELETE_STORED_LINK_KEY,cmd, 7 );
    memset( (uint8_t *)&app_h->remote_btaddr, 0, sizeof( btaddr_t ));

    app_env_clear_all_key_info();
    return 0;
}

static int app_button_enter_dut_mode( void )
{
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
    LOG_I(BTN,"app_button_enter_dut_mode\r\n");

    bt_app_entity_set_event(0, SYS_BT_DUT_MODE_EVENT);
    if( app_bt_flag1_get( APP_FLAG_DUT_MODE_ENABLE ))
        app_bt_enable_dut_mode(0);
    else
        app_bt_enable_dut_mode(1);
#endif
    return 0;
}

static int app_button_lang_change( void )
{
    LOG_I(BTN,"app_button_lang_change\r\n");

    app_env_handle_t env_h = app_env_get_handle();

    env_h->env_cfg.wave_lang_sel++;
    
    if (env_h->env_cfg.wave_lang_sel >= app_env_get_wave_max_lang_num())
    	env_h->env_cfg.wave_lang_sel = 0;

    env_h->env_data.lang_sel = env_h->env_cfg.wave_lang_sel;

//yuan	app_wave_file_play_start( APP_WAVE_FILE_ID_RESERVED3 );

    app_env_write_action(&env_h->env_data.default_btaddr,0);

    return 0;
}

#ifdef CONFIG_BLUETOOTH_HFP
static int app_button_redial_last_number( void )
{
    if( hfp_has_connection()
        && (!app_bt_flag1_get( APP_FLAG_HFP_CALLSETUP ))
        && (!get_current_hfp_flag(APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING))
        )
    {
        //app_wave_file_play_start( APP_WAVE_FILE_ID_REDIAL );
    	LOG_I(BTN,"app_button_redial_last_number\r\n");

        hf_cmd_redial();

        return 0;
    }

    return -1;
}

static int app_button_micvolp_action( void )
{
    LOG_I(BTN,"app_button_micvolp_action\r\n");

    app_handle_t app_h = app_get_sys_handler();

    if( app_bt_flag1_get( APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED ) )
    {
        app_h->mic_volume_store++;

        if( app_h->mic_volume_store > ( SDADC_VOLUME_MAX + 0x6 ) )
            app_h->mic_volume_store = ( SDADC_VOLUME_MAX + 0x6 );

        if( !( app_h->mic_volume_store & 0x80 ) )  // not mute
            aud_mic_set_volume( app_h->mic_volume_store);
    }

    return 0;
}

static int app_button_micvolm_action( void )
{
    LOG_I(BTN,"app_button_micvolm_action\r\n");

    app_handle_t app_h = app_get_sys_handler();

    if( app_bt_flag1_get( APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED ) )
    {
        app_h->mic_volume_store--;
        
        if( app_h->mic_volume_store < 0 )
            app_h->mic_volume_store = 0;

        if( !( app_h->mic_volume_store & 0x80 ) )  // not mute
            aud_mic_set_volume( app_h->mic_volume_store);
    }

    return 0;
}
#if (CONFIG_AUD_EQS_SUPPORT==1)
extern void app_online_eq_gain_enable(uint16_t enable,int16_t gain);
extern void eq_onlin_set(void);
static int app_button_eq_pram_change(void)
{
 	
	app_env_handle_t  env_h = app_env_get_handle();
	if(++env_h->env_data.eq_lang_sel< APP_EQ_NUM)
		;
	else
		{
		env_h->env_data.eq_lang_sel=0;
		}
	LOG_I(BTN,"app_button_eq_pram_change(%d)\r\n",env_h->env_data.eq_lang_sel);
	app_wave_file_play_start(APP_WAVE_FILE_ID_RESERVED4);	
	app_online_eq_gain_enable(env_h->env_cfg.aud_eq.eq_enable,env_h->env_cfg.aud_eq.eq_gain);
	eq_onlin_set();
	 app_env_write_action(&env_h->env_data.default_btaddr,0);
	 return 0;
}
#endif
static int app_button_micvol_mute_action( void )
{
    LOG_I(BTN,"app_button_micvol_mute_action\r\n");

    app_handle_t app_h = app_get_sys_handler();

    if( app_bt_flag1_get( APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED ) )
    {
        if( app_h->mic_volume_store & 0x80 )
        {
            app_h->mic_volume_store &= ~0x80;
        #ifdef CONFIG_DRIVER_ADC
            aud_mic_mute(0);
        #endif
            app_wave_file_play_start(APP_WAVE_FILE_ID_UNMUTE_MIC);
        }
        else
        {
            app_wave_file_play_start(APP_WAVE_FILE_ID_MUTE_MIC);
            app_h->mic_volume_store |= 0x80;
        #ifdef CONFIG_DRIVER_ADC
            aud_mic_mute(1);
        #endif
        }
    }

    return 0;
}

static int app_button_voice_dial_set( void )
{
    LOG_I(BTN,"app_button_voice_dial_set\r\n");

    if(!is_ag_support_feature(AG_VOICE_RECOGNITION_FUNCTION))
    {
        LOG_I(BTN,"ag does not support feature: AG_VOICE_RECOGNITION_FUNCTION\r\n");
        return FALSE;
    } 
    else 
    {
        LOG_I(BTN,"ag supports feature: AG_VOICE_RECOGNITION_FUNCTION.\r\n");
    }

    if(hfp_has_connection()) 
    {
        if(hfp_has_sco()) 
        {
            hf_cmd_set_voice_recog(0);
            if( app_bt_flag1_get(APP_FLAG_MUSIC_PLAY_SCHEDULE) ) 
            {
                app_bt_flag1_set(APP_FLAG_MUSIC_PLAY_SCHEDULE, 0);
                //app_button_playpause_action();
                app_button_type_set(APP_BUTTON_TYPE_A2DP);
            }
        } 
        else
        {
            if(a2dp_has_music())
            {
                //workaround for issue 174
                audio_dac_ana_mute(1);
                aud_dac_buffer_clear();
                aud_dac_dig_volume_fade_in();

                //app_button_playpause_action();
                //app_bt_flag1_set(APP_FLAG_MUSIC_PLAY_SCHEDULE, 1);
                hfp_hf_app_t *app_ptr=NULL;
                app_ptr = hfp_app_lookup_valid(a2dp_get_current_app_remote_addr());
                if(app_ptr != NULL)
                {
                    hfp_update_current_app(app_ptr);
                }
            }
            
            if (app_wave_file_play_start(APP_WAVE_FILE_ID_HFP_VOICE_DIAL) == WAV_NO_ERROR)
                app_bt_shedule_task((jthread_func)hf_cmd_set_voice_recog, (void *)1, 500);
            else
                hf_cmd_set_voice_recog(1);
        }
    }

    return TRUE;
}

static int app_button_eSCO_A_B_swap(void)
{
    LOG_I(BTN,"app_button_eSCO_A_B_swap\r\n");

    hfp_2eSCOs_A_B_SWAP();
    return 0;
}

static int app_button_eSCO_A_B_TWC_swap(void)
{
    LOG_I(BTN,"app_button_eSCO_A_B_TWC_swap\r\n");

    app_button_eSCO_A_B_swap();
    
    if(get_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE))
    {
       app_button_type_set(APP_BUTTON_TYPE_TWC);
    }
    else
    {
       app_button_type_set(APP_BUTTON_TYPE_HFP);
    }
    return 0;
}

static int app_button_hf_transfer_toggle( void )
{
    LOG_I(BTN,"app_button_hf_transfer_toggle\r\n");

    if( get_current_hfp_flag( APP_FLAG_CALL_ESTABLISHED ))//APP_FLAG_HFP_CALLSETUP
    {
        app_wave_file_play_start( APP_WAVE_FILE_ID_HFP_TRANSFER );

        if(get_current_hfp_flag(APP_FLAG_SCO_CONNECTION))
			app_bt_shedule_task((jthread_func)hf_sco_handle_process, (void *)1, 1000);
        else
            app_bt_shedule_task((jthread_func)hf_sco_handle_process, (void *)0, 1000);
    }
    return 0;
}

static int app_button_hold_swtch(void)
{
    LOG_I(BTN,"app_button_hold_swtch\r\n");

    if(get_current_hfp_flag(APP_FLAG_CALL_ESTABLISHED))
    {
    	set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,1);
    	hf_cmd_chld(2, -1);
    }

    return 0;
}

/***********************************************************************************************
AT+CHLD=<n>
0 = Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call.
1 = Releases all active calls (if any exist) and accepts the other (held or waiting) call.
1x = Releases specified active call only (<idx>).
2 = Places all active calls (if any exist) on hold and accepts the other (held or waiting) call.
2x = Request private consultation mode with specified call (<idx>). (Place all calls on hold EXCEPT the call indicated by <idx>.)
3 = Adds a held call to the conversation.
4 = Connects the two calls and disconnects the subscriber from both calls (Explicit Call Transfer). Support for this value and its associated functionality is optional for the HF.
*************************************************************************************************/
static int app_button_twc_hold_accept( void )
{
    LOG_I(BTN,"app_button_twc_hold_accept\r\n");

    if(hfp_has_connection())
    {
        /* AG not support AT+CHLD=2x command */
        hf_cmd_chld(2, -1);
        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,1);
        //set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,0);
        app_button_type_set(APP_BUTTON_TYPE_TWC);
    }

    return 0;
}

static int app_button_twc_hung_accept( void )
{
    LOG_I(BTN,"app_button_twc_hung_accept\r\n");

    if(hfp_has_connection())
    {
        hf_cmd_chld(1, -1);
        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,0);
        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,0);
        app_button_type_set(APP_BUTTON_TYPE_HFP);
    }

    return 0;
}

static int app_button_twc_reject_hold( void )
{
    LOG_I(BTN,"app_button_twc_reject_hold\r\n");

    if(hfp_has_connection())
    {
        hf_cmd_chld(0, -1);
        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_MODE,0);
        set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE,0);
        app_button_type_set(APP_BUTTON_TYPE_HFP);
    }
    return 0;
}
#endif
#if (CONFIG_AUD_EQS_SUPPORT==0)
static int app_button_stop_action( void )
{
#if CONFIG_BLUETOOTH_HID
    if(app_bt_flag1_get( APP_FLAG_HID_CONNECTION ))
    {
        LOG_I(BTN,"enter press\n");
        //send_Hid_key_press();

        //app_bt_shedule_task((jthread_func)send_Hid_key_releas, (void *)0, 200);
        photo_Key_Atvice();
    }
#if (CONFIG_CUSTOMER_BUTTON_HID_MODE == 1)
    else
        app_button_playpause_action();
#endif
#endif

    return 0;//app_button_common_action(AVC_OP_STOP);
}
#endif
static int app_button_conn_disconn( void )
{
#if (CONFIG_A2DP_CONN_DISCONN==0) && (CONFIG_HFP_CONN_DISCONN==0)
    uint8_t idx=0;
    t_sys_bt_state bt_state = SYS_BT_UNUSED_STATE;
    app_handle_t app_h = app_get_sys_handler();
    LOG_I(BTN,"app_button_conn_disconn\r\n");

    // if still connecting, invalid button
    if(bt_connection_active()||bt_app_check_all_entity_connect_flag(BT_CONNECT_STATE_RECONNECTING))
    {
        LOG_I(BTN,"still connecting\r\n");
        return 0;
    }
    if( app_bt_flag1_get(APP_FLAG_ACL_CONNECTION) )
    {
        for(idx=0; idx<BT_MAX_AG_COUNT; idx++)
            bt_unit_acl_disconnect(app_h->unit, &g_bt_app_entity_container[idx].bt_remote_addr);
    }
    else if(app_get_env_key_num())
    {
        //app_bt_flag1_set(APP_FLAG_AUTO_CONNECTION, 1);
        //bt_connection_req(&app_h->remote_btaddr,PROFILE_BT_HFP|PROFILE_BT_A2DP_SNK|PROFILE_BT_AVRCP);
		t_bt_app_entity last_entites[2];

		for(idx=0; idx<BT_MAX_AG_COUNT; idx++)
        {   
            bt_state  = bt_app_entity_get_state(idx);           
            LOG_I(BTN,"bt_state=%x\r\n",bt_state);
            if(bt_state<SYS_BT_WORKING_STATE)
            {
                app_handle_t sys_hdl = app_get_sys_handler();
                bt_unit_set_scan_enable(sys_hdl->unit, 0);
                
                bt_app_entity_set_state(idx,SYS_BT_STANDBY_STATE);
				last_entites[idx] = g_bt_app_entity_container[idx];
            }
        }
        bt_app_get_conn_addr_from_env();
		for (idx = 0; idx < BT_MAX_AG_COUNT; ++idx)
		{
			int result_id = bt_app_entity_find_id_from_raddr(&(last_entites[idx].bt_remote_addr));
			if(result_id != MNG_ERROR_NO_ENTITY)
			{
				g_bt_app_entity_container[result_id].mac_win_book = last_entites[idx].mac_win_book;
			}
		}
    }
#elif CONFIG_A2DP_CONN_DISCONN 
	app_handle_t app_h = app_get_sys_handler();
	if ((a2dp_has_connection()&&(app_h->a2dp_state==APP_A2DP_STATE_INIT)) || (app_h->a2dp_state==APP_A2DP_STATE_CONN))
	{
		app_h->a2dp_state = APP_A2DP_STATE_DISCONN;
		a2dp_set_disconnect();
	}
	else
	{
		app_h->a2dp_state = APP_A2DP_STATE_CONN;
		a2dp_set_connect();
	}
#elif CONFIG_HFP_CONN_DISCONN
	app_handle_t app_h = app_get_sys_handler();
	if ((hfp_has_connection()&&(app_h->hfp_state==APP_HFP_STATE_INIT)) || (app_h->hfp_state==APP_HFP_STATE_CONN))
	{
		app_h->hfp_state = APP_HFP_STATE_DISCONN;
		hf_set_disconnect();
	}
	else
	{
		app_h->hfp_state = APP_HFP_STATE_CONN;
		hf_set_connect();
	}
#endif
    return 0;
}

//add by zjw for more memory
int app_button_conn_disconn_caller( void )
{
    app_button_conn_disconn();
    return 0;
}


uint8_t PowerDownFg=0;	// yuan++
int app_button_powerdown(void)
{
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN) || hfp_has_sco())
        return 0;

    app_set_led_low_battery_all_in(1);
    app_set_led_event_action(LED_EVENT_POWER_OFF);

    LOG_I(BTN,"app_button_powerdown\r\n");
    PowerDownFg = 1;	// yuan++

#if (CONFIG_CUSTOMER_ENV_SAVE_VOLUME == 1)
	app_env_handle_t env_h = app_env_get_handle();

    env_h->env_data.volume = a2dp_get_volume();
    app_env_write_action(&env_h->env_data.default_btaddr,0);
#endif

    start_wave_and_action(APP_WAVE_FILE_ID_POWEROFF, app_powerdown);
    return 0;
}

#ifdef CONFIG_BLUETOOTH_HFP
int app_button_reject_action(void)
{
    LOG_I(BTN,"app_button_reject_action\r\n");

    if(get_current_hfp_flag(APP_FLAG_HFP_CONNECTION))
    {
        if(get_current_hfp_flag(APP_FLAG2_HFP_INCOMING))
        {
            //app_wave_file_play_start( APP_WAVE_FILE_ID_HFP_REJECT );
            hf_cmd_hangup_call(1);
        }
        else if(get_current_hfp_flag(APP_FLAG_CALL_ESTABLISHED))
        {
            app_button_hf_transfer_toggle();
        }
        
        return 0;
    }
    
    return -1;
}

int app_button_clear_at_pending(uint16_t step)
{
    static uint16_t at_pending_step = 0;
    
    if(get_current_hfp_flag(APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING))
    {
        at_pending_step += step;
        if(at_pending_step > 2000)
        {
            set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING,0);
            set_current_hfp_flag(APP_HFP_AT_CMD_FLAG1_SET | APP_HFP_AT_CMD_FLAG2_SET,0);
            app_button_type_set(BUTTON_TYPE_NON_HFP);
            at_pending_step = 0;
        }
    }
    else
    {
        at_pending_step = 0;
    }
    
    return 0;
}

int app_button_hfack_action( void )
{
    app_handle_t app_h = app_get_sys_handler();

#if (CONFIG_CUSTOMER_2PHONES_HUNG_ACCETP==1)
    if (has_hfp_flag_1toN(APP_FLAG_CALL_ESTABLISHED)
    	&& get_current_hfp_flag(APP_FLAG2_HFP_INCOMING)
    	&& app_check_bt_mode(BT_MODE_1V2))
    {
    	app_set_2phones_hung_accetp(1);
	}
#endif

	if(get_current_hfp_flag(APP_FLAG_HFP_CONNECTION)
       && (!get_current_hfp_flag(APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING))
       )
    {
       	LOG_I(BTN,"app_button_hfack_action\r\n");

        if(get_current_hfp_flag(APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_OUTGOING))
        {
            //app_wave_file_play_start( APP_WAVE_FILE_ID_HFP_REJECT );
            hf_cmd_hangup_call(0);
            app_h->HF_establish_call = 0;
        }
        else
        {
            if(get_current_hfp_flag(APP_FLAG_HFP_CALLSETUP))
            {
                app_h->HF_establish_call = 1;
                //os_printf("===accept\r\n");
                hf_cmd_accept_call();
                //app_wave_file_play_start( APP_WAVE_FILE_ID_HFP_ACK );
            }
        }
        
        return 0;
    }

    return -1;
}
#endif

int app_button_sw_action( uint8_t button_event )
{
    app_handle_t app_h = app_get_sys_handler();
    if((button_event >= BUTTON_BT_END) || (app_h->button_handler[button_event] == NULL)){
        return -1;
    }
    return app_h->button_handler[button_event]();
}

void app_bt_button_setting(void)
{
#ifdef CONFIG_IRDA
    button_set_irkey_handler(IRKey_BT_handler);
#endif

    button_bt_common_register(bt_button_handler, BUTTON_BT_END );
//    gpio_button_wakeup_enable();	//yuan SD Care 退出不關中斷
}

void app_linein_button_setting(void)
{
    button_bt_common_register(linein_button_handler, BUTTON_BT_END );
    gpio_button_wakeup_enable();
}

void app_player_button_setting(void)
{
#if (CONFIG_APP_MP3PLAYER == 1)
    button_bt_common_register(player_button_handler, BUTTON_BT_END );
    // gpio_button_wakeup_enable();
#endif
}
#ifdef OP_TIMEOUT_HANDLE
static void app_button_timeout_action( void *arg)
{
    app_handle_t app_h = app_get_sys_handler();

    if( app_bt_flag1_get (APP_FLAG_AVCRP_PROCESSING ))
    {
        app_bt_flag1_set(APP_FLAG_AVCRP_PROCESSING,0);
    }
}
#endif

static int app_powerdown_condition_addtick( void )
{
    app_env_handle_t env_h = app_env_get_handle();
    int result = 0;

    /* line in / dut mode / charging don't power down */
    if(!(app_bt_flag1_get(APP_FLAG_LINEIN | APP_FLAG_DUT_MODE_ENABLE | APP_FLAG_FCC_MODE_ENABLE) || get_Charge_state()))
    {
        if(env_h->env_cfg.bt_para.pd_cond & APP_ENV_BT_PD_FLAG_NOCONN)
        {
            if(!(app_bt_flag1_get(APP_FLAG_ACL_CONNECTION)))
            {
                result = 1;
            }
        }
        
        if(env_h->env_cfg.bt_para.pd_cond & APP_ENV_BT_PD_FLAG_PAUSE_TO)
        {
            if(a2dp_has_connection())
            {
                if(app_bt_flag1_get(APP_FLAG_MUSIC_PLAY ))
                {
                    result = 0;
                }
                else
                {
                    result = 1;
                }
            }
            
            if(hfp_has_connection())
            {
                if(hfp_has_sco() || app_bt_flag1_get(APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_CALLSETUP))
                {
                    result = 0;
                }
                else
                {
                    result = 1;
                }
            }
        }
    }
    return result;
}

static int app_powerdown_condition_detect(void)
{
	app_handle_t app_h = app_get_sys_handler();
	app_env_handle_t env_h = app_env_get_handle();

	if (((app_h->powerdown_count>0)
			&&(pwdown_tick>=app_h->powerdown_count)
			&&(env_h->env_cfg.bt_para.pd_cond&APP_ENV_BT_PD_FLAG_NOCONN)
			&&!app_bt_flag1_get(APP_FLAG_ACL_CONNECTION)
			)
		|| ((app_h->pause_powerdown_count>0)
			&&(pwdown_tick>=app_h->pause_powerdown_count)
			&&app_bt_flag1_get(APP_FLAG_ACL_CONNECTION)
			&&(env_h->env_cfg.bt_para.pd_cond&APP_ENV_BT_PD_FLAG_PAUSE_TO))
		|| (app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AUX_MODE_PWR_DOWN)
			&&app_bt_flag1_get(APP_FLAG_LINEIN))
		)
	{
		return 1;
	}

	return 0;
}

void app_button_led_action( uint32_t step )
{
	//app_handle_t app_h = app_get_sys_handler();
	//app_env_handle_t env_h = app_env_get_handle();
    app_env_pwrctrl_by_switch(step);

    app_led_action(step);
    //app_low_power_detect();
    
    if( app_powerdown_condition_addtick() )
        INC_PWDOWN_TICK(step);
    else
        CLEAR_PWDOWN_TICK;
    
    if (app_powerdown_condition_detect())
    {
        CLEAR_PWDOWN_TICK;
        msg_put(MSG_POWER_DOWN);
    }
    
    /* line in detect */
    if(app_env_get_pin_enable(PIN_lineDet))
        app_linein_scanning();

	/* button acton */
    if(!app_bt_flag1_get(APP_FLAG_POWERDOWN))
    {
	    button_scanning();

        #ifdef CONFIG_BLUETOOTH_HFP
        app_button_clear_at_pending(step);
        #endif
    }
    
#ifdef CONFIG_PRODUCT_TEST_INF
    average_freqoffset_rssi();
#endif

	//app_button_photograph();

#if (CONFIG_CUSTOMER_EDGE_KEY == 1)
	app_edge_key();
#endif

#if (CONFIG_CUSTOMER_MOTOR_CONTROL == 1)
	app_M_control();
#endif

#if (CONFIG_CUSTOMER_BUTTON_HID_MODE == 1)
	app_customer_hid_disconnect();
#endif

#if (CONFIG_CUSTOMER_1V2_CON_MODE == 1)
	app_customer_1v2_con_close();
#endif
}

void app_powerdown( void )
{
    app_handle_t app_h = app_get_sys_handler();
    uint32_t delay_time = 0;
    
    LOG_I(BTN,"app_powerdown\r\n");
    
    if(app_bt_flag1_get(APP_FLAG_POWERDOWN))
    {
        LOG_I(BTN,"app_powerdown returned for APP_FLAG_POWERDOWN setted\r\n");
        return;
    }
 
    app_set_led_low_battery_all_in(1);
    
    if(!app_bt_flag2_get(APP_FLAG2_CHARGE_POWERDOWN))
        app_set_led_event_action(LED_EVENT_POWER_OFF);
#if 1
    if( app_bt_flag1_get(APP_FLAG_LINEIN ))
    {
        app_bt_flag1_set(APP_FLAG_POWERDOWN, 1);
        // line in power down led off
        BK3000_Ana_Line_enable(0);
        app_powerdown_action();
        return;
    }

    app_sleep_func(0);

    CLEAR_PWDOWN_TICK;

    app_bt_flag1_set(APP_FLAG_DUT_MODE_ENABLE, 0);
    app_bt_flag1_set(APP_FLAG_POWERDOWN, 1);
    //app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);

    if( app_bt_flag1_get (APP_AUDIO_FLAG_SET ))
    {
        #ifdef CONFIG_BLUETOOTH_AVRCP
        if(a2dp_current_check_flag(APP_BUTTON_FLAG_PLAY_PAUSE))
        {     
            uint32_t opcode = 0;
            btaddr_t *other_addr = a2dp_get_not_active_remote_addr();
            opcode = AVC_OP_PAUSE;       
            if(avrcp_current_is_connected()
               && !avrcp_current_is_processing())
            {     
                if( (other_addr!= NULL) && a2dp_has_the_music(other_addr))
                {              
                    avrcp_send_opcode_by_raddr(other_addr,(void *)opcode);
                    delay_time = 1500;   
                } 
                if(delay_time > 0) 
                {                
                    jtask_stop(app_h->app_a2dp_task);
                    jtask_schedule(app_h->app_a2dp_task, delay_time, (jthread_func)avrcp_current_send_opcode, (void *)opcode);   
                }
                else 
                {    
                    avrcp_current_send_opcode((void *)opcode);
                }
                a2dp_current_clear_flag(APP_BUTTON_FLAG_PLAY_PAUSE); 
                delay_time += 1000;
            }
                    
            app_h->flag_sm1 ^= APP_BUTTON_FLAG_PLAY_PAUSE;
            jtask_stop(app_h->app_audio_task);
            jtask_schedule(app_h->app_audio_task, delay_time, (jthread_func)bt_all_acl_disconnect, (void *)app_h->unit);
        }
        else
        #endif
            bt_all_acl_disconnect(app_h->unit);

        app_bt_flag1_set((APP_AUDIO_FLAG_SET|APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION),0);

        app_led_action(1);
        app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)app_h->unit, 2000+delay_time);
    }
    else
    {
        LOG_D(BTN,"===have no BT \r\n");
        app_led_action(1);
        app_powerdown_action();
        //app_bt_shedule_task((jthread_func)app_powerdown_action, (void *)app_h->unit, 1000);
    }
#endif
}

void app_set_powerdown_flag(void *arg)
{
    uint32_t flag = (uint32_t)arg;
    if(SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode())
    {
        BK3000_icu_sw_powerdown(app_env_get_pin_num(PIN_ioDet),flag );
    }
    else
    {
        if(app_env_get_pin_enable(PIN_pwrBtn))
        {
            BK3000_icu_sw_powerdown(app_env_get_pin_num(PIN_pwrBtn),flag );
        }
    }
}

void app_powerdown_action( void )
{
    //app_env_handle_t  env_h = app_env_get_handle();
    /* int gpio = (app_h->button_code[BUTTON_POWERDOWN])& 0x0000ffff; */

    LOG_I(BTN,"app_powerdown_action,pwrctrl_pin:%d,wakeup_pin:%d\r\n",app_env_get_pin_num(PIN_mosCtrl),app_env_get_pin_num(PIN_pwrBtn));

    app_wave_file_play_stop();
#if 0
    //No need to clear the flag. Keep it so the protection will remain till end of the cycle
    //app_bt_flag1_set(APP_FLAG_POWERDOWN, 0);
    app_handle_t app_h = app_get_sys_handler();
    app_h->button_mode = BUTTON_NONE;
    app_h->button_commit = BUTTON_NONE;
    app_h->button_press_count = 0;
    app_h->button_long_press_count = 0;
    app_h->button_state = BUTTON_PRESS_NONE;
    j_stack_uninit();
    app_clear_led_event_action(0);    // LED off
    app_clear_led_event_action(1);    // LED off
    app_clear_led_event_action(2);
    app_led_action(1);

    aud_PAmute_operation(1);
    if(SW_PWR_KEY_MOS_CTRL==app_env_check_pwrCtrl_mode())
    {
//	if (env_h->env_cfg.used ) {
        uint8_t pwrctrl_pin = env_h->env_cfg.system_para.pwrctrl_pin;
        gpio_config( pwrctrl_pin, 1 );
        while (1)
        {
            if (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PWRCTRL_HIGH)
            	gpio_output(pwrctrl_pin, 0);
            else
            	gpio_output(pwrctrl_pin, 1);
            Delay(1000);
            CLEAR_WDT;
        }
    }
#endif
    app_set_powerdown_flag((void*)POWERDOWN_SELECT);
}
