#ifndef _APP_BUTTON_H_
#define _APP_BUTTON_H_

//#define BTN_DEBUG
#ifdef BTN_DEBUG
    #define BTN_PRT      os_printf
#else
    #define BTN_PRT      os_null_printf
#endif

#define BUTTON_CLICK                (uint64_t)0x8000000000000000
#define BUTTON_DOUBLE_CLICK         (uint64_t)0x4000000000000000
#define BUTTON_LONG_PRESS           (uint64_t)0x0200000000000000 //LONG_PRESS_UP
#define BUTTON_WAIT_DOUBLE_CLICK    (uint64_t)0x1000000000000000
#define BUTTON_CONTINUE             (uint64_t)0x0800000000000000
#define BUTTON_LONG_LONG_PRESS      (uint64_t)0x0400000000000000
#define BUTTON_LONG_PRESS_NO_UP     (uint64_t)0x2000000000000000

#define BUTTON_MUL_CLICK_MASK 	    (uint64_t)(((uint64_t)1<<53)|((uint64_t)1<<52)|BUTTON_DOUBLE_CLICK) //认为没有产品同时用"长按"和"双击"控制同一个事件
#define BUTTON_MUL_CLICK_FLAG 	    (uint64_t)(((uint64_t)1<<53)|BUTTON_DOUBLE_CLICK)

#define BUTTON_THREE_CLICK_FLAG     (uint64_t)(BUTTON_MUL_CLICK_FLAG|BUTTON_CLICK)
#define BUTTON_FOUR_CLICK_FLAG      (uint64_t)(BUTTON_MUL_CLICK_FLAG|BUTTON_LONG_LONG_PRESS)
#define BUTTON_FIVE_CLICK_FLAG      (uint64_t)(BUTTON_MUL_CLICK_FLAG|BUTTON_CLICK|BUTTON_LONG_LONG_PRESS)
#define BUTTON_SIX_CLICK_FLAG       (uint64_t)(BUTTON_MUL_CLICK_FLAG|BUTTON_LONG_PRESS)
#define BUTTON_SEVEN_CLICK_FLAG     (uint64_t)(BUTTON_MUL_CLICK_FLAG|BUTTON_CLICK|BUTTON_LONG_PRESS)
#define BUTTON_EIGHT_CLICK_FLAG     (uint64_t)(BUTTON_MUL_CLICK_FLAG|BUTTON_LONG_LONG_PRESS|BUTTON_LONG_PRESS)

#define BUTTON_ACTION_MASK			(uint64_t)0xFE00000000000000

#define BUTTON_TYPE_NON_HFP			(uint64_t)0x0040000000000000
#define BUTTON_TYPE_HFP				(uint64_t)0x0080000000000000
#define BUTTON_TYPE_TWC				(uint64_t)0x0100000000000000
#define BUTTON_TYPE_ALL				(uint64_t)0x01C0000000000000
#define BUTTON_TYPE_MASK			(uint64_t)0x01C0000000000000

#define BUTTON_MODE_ADC_SHIFT       19
#define BUTTON_MODE_ADC_CHAN_SHIFT  16
#define BUTTON_MODE_ADC_MASK        (uint64_t)0x0038000000000000
#define BUTTON_MODE_ADC_CHAN_MASK   (uint64_t)0x0007000000000000
#define BUTTON_MODE_MASK            (uint64_t)0x000000FFFFFFFFFF //0x003FFFFF

//more click
#define BUTTON_MORE_INTERVAL_TIME   50
#define BUTTON_TWO_CLICK            0
#define BUTTON_THREE_CLICK          1
#define BUTTON_FOUR_CLICK           2
#define BUTTON_FIVE_CLICK           3
#define BUTTON_SIX_CLICK            4
#define BUTTON_SEVEN_CLICK          5
#define BUTTON_EIGHT_CLICK          6

enum
{
    BUTTON_PRESS_NONE        = 0,
    BUTTON_PRESS_DOWN        = 1,
    BUTTON_PRESS_UP          = 2,
    BUTTON_DOUBLE_PRESS_DOWN = 3,
    BUTTON_DOUBLE_PRESS_UP   = 4,
    BUTTON_PRESS_DOWN_LONG1  = 5,
    BUTTON_PRESS_DOWN_LONG2  = 6
};

enum
{
    BUTTON_NONE = 0,
    BUTTON_PLAY_PAUSE,
    BUTTON_NEXT,
    BUTTON_PREV,
    BUTTON_REWIND,
    BUTTON_FORWARD,
    BUTTON_VOL_P,
    BUTTON_VOL_M,
    BUTTON_VOL_MUTE,
    BUTTON_MATCH,
    BUTTON_POWERDOWN, // soft power down
    BUTTON_HFP_ACK,
    BUTTON_HFP_NACK,
    BUTTON_HFP_DIAL,
    BUTTON_MIC_P,
    BUTTON_MIC_M,
    BUTTON_MIC_MUTE,
    BUTTON_VOICE_DIAL_INIT,
    BUTTON_TRANSFER_TOGGLE,
    BUTTON_CONN_DISCONN,
    BUTTON_STOP,
    BUTTON_CLEAR_MEMORY,
    BUTTON_TEST_MODE,
    BUTTON_LANG_CHANGE,
    BUTTON_TWC_HOLD_ACCEPT,
    BUTTON_TWC_HUNG_ACCEPT,
    BUTTON_TWC_REJECT_HOLD,
    BUTTON_END
};

enum
{
	BUTTON_BT_NONE = 0,
	BUTTON_BT_PLAY_PAUSE,
	BUTTON_BT_NEXT,
	BUTTON_BT_PREV,
	BUTTON_BT_REWIND,
	BUTTON_BT_FORWARD,
	BUTTON_BT_VOL_P,
	BUTTON_BT_VOL_M,
	BUTTON_BT_VOL_MUTE,
	BUTTON_BT_MATCH,
	BUTTON_BT_POWERDOWN, // soft power down
	BUTTON_BT_HFP_ACK,
	BUTTON_BT_HFP_NACK,
	BUTTON_BT_HFP_DIAL,
	BUTTON_BT_MIC_P,
	BUTTON_BT_MIC_M,
	BUTTON_BT_MIC_MUTE,
	BUTTON_BT_VOICE_DIAL_INIT,
	BUTTON_BT_TRANSFER_TOGGLE,
	BUTTON_BT_CONN_DISCONN,
	BUTTON_BT_STOP,
	BUTTON_BT_CLEAR_MEMORY,
	BUTTON_BT_TEST_MODE,
	BUTTON_BT_LANG_CHANGE,
	BUTTON_BT_TWC_HOLD_ACCEPT,
	BUTTON_BT_TWC_HUNG_ACCEPT,
	BUTTON_BT_TWC_REJECT_HOLD,
	BUTTON_BT_TWC_SWTCH_WAY,
	BUTTON_BT_2PHONES_SWTCH,
	BUTTON_STEREO,
	BUTTON_RESERVED1,
	BUTTON_WMicPower,	//yuan++
	BUTTON_MODE_CHG,	//yuan++
	BUTTON_BT_END
};

#define BUTTON_PRESS_COUNT_THRE                  2
#define BUTTON_PRESS_REPEAT_THRE                 15
#define BUTTON_LONG_PRESS_COUNT_THRE             60
#define BUTTON_LONG_PRESS_COUNT_THRE2            500
#define BUTTON_DOUBLE_CLICK_WAIT_COUNT_THRE      20

#define APP_BUTTON_ACTION_CLICK                  (uint64_t)0x8000000000000000
#define APP_BUTTON_ACTION_DOUBLE_CLICK           (uint64_t)0x4000000000000000
#define APP_BUTTON_ACTION_LONG_PRESS             (uint64_t)0x0200000000000000 //LONG_PRESS_UP
#define APP_BUTTON_ACTION_WAIT_DOUBLE_CLICK      (uint64_t)0x1000000000000000
#define APP_BUTTON_ACTION_CONTINUE               (uint64_t)0x0800000000000000
#define APP_BUTTON_ACTION_LONG_LONG_PRESS        (uint64_t)0x0400000000000000
#define APP_BUTTON_ACTION_LONG_PRESS_NO_UP       (uint64_t)0x2000000000000000
#define APP_BUTTON_ACTION_MASK					 (uint64_t)0xFE00000000000000

#define APP_BUTTON_TYPE_A2DP					 (uint64_t)0x0040000000000000
#define APP_BUTTON_TYPE_HFP						 (uint64_t)0x0080000000000000
#define APP_BUTTON_TYPE_TWC						 (uint64_t)0x0100000000000000
#define APP_BUTTON_TYPE_MASK				     (uint64_t)0x01C0000000000000

#define APP_BUTTON_MODE_MASK                     (uint64_t)0x000000FFFFFFFFFF //0x003FFFFF

#define BTN_CODE(action, gpio, type)             (((uint64_t)1 << (gpio-GPIO0)) + (action) + (type))

typedef uint8_t APP_BUTTON_STATE;
typedef int (*button_action_func)(void);

void button_action(uint64_t commit_info);
void button_scanning(void);
uint64_t button_get_gpio_map( int *gpio_cnt );
#ifdef CONFIG_IRDA
 void button_set_irkey_handler(button_action_func *handler);
#endif
void button_common_register(const uint64_t *button_code,
                                     const button_action_func *button_handler,
                                     int button_num );
void button_bt_common_register(const button_action_func *button_handler,
	                                     int button_num );
void app_button_type_set( uint64_t type );
int app_button_sw_action( uint8_t button_event );
int app_button_playpause_action_caller( int play_pause );
int app_button_conn_disconn_caller( void );
#ifdef CONFIG_BLUETOOTH_HFP
int app_button_clear_at_pending(uint16_t step);
int app_button_hfack_action( void );
int app_button_reject_action(void);
#endif
void app_bt_button_setting(void);
void app_linein_button_setting(void);
int app_button_powerdown( void );
int app_button_match_action(void);
void app_set_powerdown_flag(void *arg);
void app_button_led_action( uint32_t step );
void app_powerdown( void );
void app_powerdown_action( void );
#endif // _APP_BUTTON_H_
