#ifndef _APP_LED_H_
#define _APP_LED_H_

enum
{
    LED0    = 0,     //led 0
    LED1    = 1,     //led 1
    LEDLP   = 2,     //led lowpower
    LED_NUM = 3
};

enum
{
    LED_NO_BLINK        = 0,
    LED_BLINK_FAST      = 1,
    LED_BLINK_SLOW      = 2,
    LED_BLINK_VERY_FAST = 3,
    LED_BLINK_VERY_SLOW = 4
};

typedef enum
{
    LED_EVENT_AUTO_CONNECTION = 0,
    LED_EVENT_MATCH_STATE,
    LED_EVENT_NO_MATCH_NO_CONN_STATE,
    LED_EVENT_CONN,
    LED_EVENT_BT_PAUSE,
    LED_EVENT_BT_PLAY,
    LED_EVENT_INCOMING_RINGING,
    LED_EVENT_OUTGOING_RINGING,
    LED_EVENT_CALL_ESTABLISHMENT,
    LED_EVENT_LOW_BATTERY,
    LED_EVENT_CHARGING,
    LED_EVENT_BATTERY_FULL,
    LED_EVENT_POWER_ON,
    LED_EVENT_POWER_OFF,
    LED_EVENT_LINEIN_PLAY,
    LED_EVENT_LINEIN_PAUSE,
    LED_EVENT_MUSIC_PLAY,
    LED_EVENT_MUSIC_PAUSE,
    LED_EVENT_FM_PLAY,
    LED_EVENT_FM_PAUSE,
    LED_EVENT_FM_SCAN,
    LED_EVENT_TEST_MODE,
    LED_EVENT_STEREO_CONN_MODE,
    LED_EVENT_STEREO_MATCH_MODE,
    LED_EVENT_STEREO_RECON_MODE,
    LED_EVENT_RESERVED1,
    LED_EVENT_RESERVED2,
    LED_EVENT_RESERVED3,
    LED_EVENT_END
}APP_LED_EVENT;

typedef   struct _app_led_info_s
{
	uint8_t index;
	uint8_t number_flash;
	uint16_t led_ontime;
	uint16_t led_offtime;
	uint16_t repeat;
	int32_t timeout;
}__PACKED_POST__ app_led_info_t;

void app_led_init( void );
void app_set_led_event_action( APP_LED_EVENT event );
void app_led_action(uint32_t step);
uint8_t app_set_led_low_battery_all_in(uint8_t flag);
void app_clear_led_event_action( int index);
uint8_t app_get_led_low_battery_index(void);
#endif
