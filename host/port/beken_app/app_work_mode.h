#ifndef _APP_WORKMODE_H_
#define _APP_WORKMODE_H_

enum
{
    SYS_WM_BT_MODE = 0,
    SYS_WM_SDCARD_MODE,
    SYS_WM_UDISK_MODE,
	SYS_WM_LINEIN1_MODE,
	SYS_WM_LINEIN2_MODE,
	SYS_WM_SPDIF_MODE,
    SYS_WM_NULL,
};

//default vol is 8,  but 12 for linein becasue 8 is lower
#define  PLAYER_VOL_MEDIUM          8
#define  PLAYER_VOL_MEDIUM_LINEIN   12
#define  PLAYER_VOL_MEDIUM_HFP      13

// extern int8_t player_vol_bt;
extern int8_t player_vol_hfp;
extern int8_t player_vol_music;
extern int8_t player_vol_fm;
extern int8_t player_vol_linein;


uint8_t app_check_bt_mode(uint8_t mode);
uint8_t get_bt_dev_priv_work_flag(void);

uint32_t app_is_mp3_mode(void);
uint32_t app_is_bt_mode(void);
uint8_t app_is_sdcard_mode(void);
uint8_t app_is_udisk_mode(void);
uint32_t app_is_line_mode(void);

void udisk_mode_auto_sw_set(uint8_t en);
uint8_t udisk_mode_auto_sw_get(void);
void udisk_mode_auto_play_set(uint8_t en);
uint8_t udisk_mode_auto_play_get(void);
void usbh_udisk_auto_op_init(void);
void usbh_udisk_init_cmp_callback(void);
void usbh_udisk_lost_callback(void);
void user_udisk_ready_callback(void);
void user_udisk_lost_callback(void);


void spdif_mode_auto_sw_set(uint8_t en);
uint8_t spdif_mode_auto_sw_get(void);
void spdif_mode_auto_exit_set(uint8_t en);
uint8_t spdif_mode_auto_exit_get(void);
void spdif_mode_auto_play_set(uint8_t en);
uint8_t spdif_mode_auto_play_get(void);


void system_work_mode_set_button(uint8_t mode);	//yuan++
int system_work_mode_change_button(void);

unsigned int get_app_mode();
unsigned int get_bt_mode();
void set_bt_mode(unsigned int mode);
void app_incoming_call_enter(void);
void app_incoming_call_exit(void);

void app_bt_enable_dut_mode( int8_t enable);
void app_bt_enable_fcc_mode(int8_t mode, uint8_t chnl, uint8_t pwr);
void app_bt_enable_qos(uint8_t enable);

#endif
