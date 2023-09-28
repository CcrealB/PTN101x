#ifndef _APP_LINEIN_H_
#define _APP_LINEIN_H_

//#define LINEIN_DEBUG

#ifdef LINEIN_DEBUG
    #define LINEIN_PRT      os_printf
#else
    #define LINEIN_PRT		os_null_printf 
#endif

//#define LINEIN_BUTTON_CONFIG

#define LINEIN_STATUS_ON                    (0x11)
#define LINEIN_STATUS_OFF                   (0x22)

#define LINEIN_GPIO_DETECT_ID               2
#define LINEIN_GPIO_PLUG_IN                 0
#define LINEIN_GPIO_PLUG_OUT                1
#define LINEIN_GPIO_DEBOUNCE_COUNT          40

#define LINEIN_MAX_VOL                      28
#define LINEIN_MIN_VOL                      2
#define LINEIN_VOL_STEP                     2

typedef void (*LINEIN_DETECT)(void);

typedef struct _linein_cfg_s
{
    uint8_t enable;
    uint8_t high_flag;
    uint8_t pin;
    uint8_t status;
    LINEIN_DETECT detect_func;
    uint32_t plugin_count;
    uint32_t pullout_count;
}linein_cfg_t;

typedef enum
{
    LINEIN_NULL = 0,
    LINEIN_NO_ACTIVITY = 0x01,
    LINEIN_W4_ATTACH = 0x02,
    LINEIN_ATTACH = 0x04,
    LINEIN_W4_DETACH = 0x08,
    LINEIN_DETACH = 0x10,
    LINEIN_POWER_DOWN = 0x20
}t_linein_state;

void app_playwav_resumelinein(uint32_t fieldId);

extern void linein_audio_close(void);
extern void linein_audio_open(void);
extern void app_linein_enter(void *arg);
extern void app_linein_exit(void *arg);
extern void app_linein_scanning( void );
extern int app_linein_vol_up( void );
extern int app_linein_vol_down( void );
extern int app_linein_mute_unmute(void);
extern int app_linein_powerup_check(void);

void app_linein_set_curr_state(t_linein_state state);
t_linein_state app_linein_get_state(void);
void app_linein_set_state(t_linein_state state);
void app_linein_state_switch_complete(void);
void app_linein_state_process(void);
void app_linein_init(void);
void app_linein_para_init(void);

#endif
// EOF
