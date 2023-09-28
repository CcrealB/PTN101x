#ifndef _APP_EARIN_H_
#define _APP_EARIN_H_
#include <config/config.h>

#if (CONFIG_EAR_IN == 1)

//#define EARIN_DEBUG

#ifdef EARIN_DEBUG
    #define EARIN_PRT      os_printf
#else
    #define EARIN_PRT		os_null_printf 
#endif

#define DEVICE_VM6320 		1
#define	DEVICE_HY2751		2

#define EARIN_DEV_TYPE		DEVICE_HY2751

#define SLAVE_PLAYPAUSE_OPERATE_FLAG			0x01
#define PHONE_PLAYPAUSE_OPERATE_FLAG			0x02
#define EAR_PLAYPAUSE_OPERATE_FLAG				0x04
#define OTHER_EAR_DETECT_STATUS_FLAG			0x08
#define SYN_PLAY_PAUSE_STATUS_FLAG              0x10

#define EAR_IN_STATUS_ON                    (0x55)
#define EAR_IN_STATUS_OFF                   (0xaa)


typedef void (*EAR_IN_DETECT)(uint32 arg);

typedef struct _earin_cfg_s
{
    uint8 enable;
    uint32 high_flag;
	uint8 Cs_pin;
    uint8 Det_pin;
    uint8 status;
    EAR_IN_DETECT detect_func;
    uint32 plugin_count;
    uint32 pullout_count;
    uint32 wait_plugin_time;
    uint8 tws_pss_switch;
	uint32 plus_det_count;
    uint8 pp_enable;
    uint8 pre_other;
    uint8 wakeup_dly;
}earin_cfg_t;
typedef enum
{ 
    TWS_EAR_OUT_STATUS =0,  
    TWS_EAR_IN_STATUS,          //1
    TWS_PAUSE_PHONE_ACTIVE,     //2
    TWS_PAUSE_EAROUT_ACTIVE,    //3
    TWS_PLAY_ACTIIVE,           //4
    TWS_EAR_OP,                 //5
    TWS_BUTTON_OP,              //6
    TWS_READ_EAR_STATUS         //7
} t_tws_earin_cmd;

uint8 get_earin_det_pin(void);
uint8 get_earin_cs_pin(void);
void app_ear_in_init(void);
void app_ear_in_uninit(uint8 enable);
uint8 app_get_earin_cfg(void);
uint8 get_earin_sec_status(void);
uint8 app_get_earin_tws_switch_status(void);
void app_ear_in_scanning(uint32 step);
//void ear_in_detect_enable(uint8 enable);
void earin_playpause_enable(uint8 enable);
void app_ear_in_wave_file_play(void);
void app_ear_in_handle(uint8_t in_or_out);
uint8 get_earin_status(void);
//void app_set_earin_status(uint8 status);
void Set_PlayPause_Flag_Operate(uint8 flag, uint8 op);
uint8 Get_PlayPause_Flag_Operate(uint8 flag);
void app_ear_button_playing_syn_status(void);
void app_ear_button_stoppause_syn_status(void);

void app_ear_in_plus_handle(void);
void earin_set_plusdetect_count(void);
void earin_generate_start_from_sleep_mode(uint8 flag);

void ear_in_plus_generate_timeisr(uint8 enable, uint8 step);
void earin_set_plusdetect_count_timeisr(void);

#if (CONFIG_DRIVER_PWM == 1)
void app_earin_pwm_handler(uint32_t step);
void earin_set_plusdetect_count_pwmisr(void);
#endif

#endif


#endif
// EOF

