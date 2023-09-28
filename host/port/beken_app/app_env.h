#ifndef _APP_ENV_H_
#define _APP_ENV_H_

#include <jos.h>
#include "app_hfp.h"
#include "app_aec.h"
#include "app_equ.h"
#include "app_drc.h"
#include "app_tlv.h"
#include "config.h"
enum
{
    ENV_ERR_PARA = -1,
    ENV_ERR_BUSY = -2
};

#define FLASH_PAGE_LEN                   256
#define FLASH_PAGE_LEN_CRC               272
#define ENV_BLOCK_LEN                    256

//----for PTN101-------------
#define FLASH_ENVCFG_DEF_ADDR_4M_ABS	 0x7B000 //(abso_addr)
#define FLASH_ENVDATA_DEF_ADDR_4M_ABS 	 0x7C000
#define FLASH_OTADATA_DEF_ADDR_4M_ABS    0x7D000
#define FLASH_ENVNVDS_DEF_ADDR_4M_ABS 	 0x7E000
#define FLASH_ENVCALI_DEF_ADDR_4M_ABS 	 0x7F000
#define FLASH_ENVEND_DEF_ADDR_4M_ABS 	 0x7FFFF
#define FLASH_ENVCFG_DEF_ADDR_8M_ABS	 0xFB000 //(abso_addr)
#define FLASH_ENVDATA_DEF_ADDR_8M_ABS 	 0xFC000
#define FLASH_OTADATA_DEF_ADDR_8M_ABS    0xFD000
#define FLASH_ENVNVDS_DEF_ADDR_8M_ABS 	 0xFE000
#define FLASH_ENVCALI_DEF_ADDR_8M_ABS 	 0xFF000
#define FLASH_ENVEND_DEF_ADDR_8M_ABS 	 0xFFFFF
#define FLASH_ENVCFG_DEF_ADDR_16M_ABS	 0x1FB000 //(abso_addr)
#define FLASH_ENVDATA_DEF_ADDR_16M_ABS   0x1FC000
#define FLASH_OTADATA_DEF_ADDR_16M_ABS   0x1FD000
#define FLASH_ENVNVDS_DEF_ADDR_16M_ABS 	 0x1FE000
#define FLASH_ENVCALI_DEF_ADDR_16M_ABS   0x1FF000
#define FLASH_ENVEND_DEF_ADDR_16M_ABS 	 0x1FFFFF
#define FLASH_ENVCFG_DEF_ADDR_32M_ABS    0x3FB000 //(abso_addr)
#define FLASH_ENVDATA_DEF_ADDR_32M_ABS   0x3FC000
#define FLASH_OTADATA_DEF_ADDR_32M_ABS   0x3FD000
#define FLASH_ENVNVDS_DEF_ADDR_32M_ABS 	 0x3FE000
#define FLASH_ENVCALI_DEF_ADDR_32M_ABS   0x3FF000
#define FLASH_ENVEND_DEF_ADDR_32M_ABS    0x3FFFFF

#if (FLASH_INFO_4M == FLASH_INFO)
#define FLASH_ENVCFG_DEF_ADDR_ABS		 FLASH_ENVCFG_DEF_ADDR_4M_ABS
#define FLASH_ENVDATA_DEF_ADDR_ABS 		 FLASH_ENVDATA_DEF_ADDR_4M_ABS
#define FLASH_OTADATA_DEF_ADDR_ABS 		 FLASH_OTADATA_DEF_ADDR_4M_ABS
#define FLASH_ENVNVDS_DEF_ADDR_ABS		 FLASH_ENVNVDS_DEF_ADDR_4M_ABS
#define FLASH_ENVCALI_DEF_ADDR_ABS		 FLASH_ENVCALI_DEF_ADDR_4M_ABS
#define FLASH_ENVEND_DEF_ADDR_ABS		 FLASH_ENVEND_DEF_ADDR_4M_ABS
#elif (FLASH_INFO_8M == FLASH_INFO)
#define FLASH_ENVCFG_DEF_ADDR_ABS		 FLASH_ENVCFG_DEF_ADDR_8M_ABS
#define FLASH_ENVDATA_DEF_ADDR_ABS 		 FLASH_ENVDATA_DEF_ADDR_8M_ABS
#define FLASH_OTADATA_DEF_ADDR_ABS 		 FLASH_OTADATA_DEF_ADDR_8M_ABS
#define FLASH_ENVNVDS_DEF_ADDR_ABS		 FLASH_ENVNVDS_DEF_ADDR_8M_ABS
#define FLASH_ENVCALI_DEF_ADDR_ABS		 FLASH_ENVCALI_DEF_ADDR_8M_ABS
#define FLASH_ENVEND_DEF_ADDR_ABS		 FLASH_ENVEND_DEF_ADDR_8M_ABS
#elif (FLASH_INFO_16M == FLASH_INFO)
#define FLASH_ENVCFG_DEF_ADDR_ABS		 FLASH_ENVCFG_DEF_ADDR_16M_ABS
#define FLASH_ENVDATA_DEF_ADDR_ABS 		 FLASH_ENVDATA_DEF_ADDR_16M_ABS
#define FLASH_OTADATA_DEF_ADDR_ABS 		 FLASH_OTADATA_DEF_ADDR_16M_ABS
#define FLASH_ENVNVDS_DEF_ADDR_ABS		 FLASH_ENVNVDS_DEF_ADDR_16M_ABS
#define FLASH_ENVCALI_DEF_ADDR_ABS		 FLASH_ENVCALI_DEF_ADDR_16M_ABS
#define FLASH_ENVEND_DEF_ADDR_ABS		 FLASH_ENVEND_DEF_ADDR_16M_ABS
#elif (FLASH_INFO_32M == FLASH_INFO)
#define FLASH_ENVCFG_DEF_ADDR_ABS		 FLASH_ENVCFG_DEF_ADDR_32M_ABS
#define FLASH_ENVDATA_DEF_ADDR_ABS 		 FLASH_ENVDATA_DEF_ADDR_32M_ABS
#define FLASH_OTADATA_DEF_ADDR_ABS 		 FLASH_OTADATA_DEF_ADDR_32M_ABS
#define FLASH_ENVNVDS_DEF_ADDR_ABS		 FLASH_ENVNVDS_DEF_ADDR_32M_ABS
#define FLASH_ENVCALI_DEF_ADDR_ABS		 FLASH_ENVCALI_DEF_ADDR_32M_ABS
#define FLASH_ENVEND_DEF_ADDR_ABS		 FLASH_ENVEND_DEF_ADDR_32M_ABS
#endif

enum
{
    ENV_LE_FLAG_LE_ENABLE = 0x01,
    ENV_LE_FLAG_ADDR_NAME_CONFIG_ENABLE = 0x02,   
    ENV_LE_FLAG_ADV_CONFIG_ENABLE = 0x04,
    ENV_LE_FLAG_UPDATE_CONFIG_ENABLE = 0x08,
};

enum
{
    ENV_ACTION_DISCONNECT_NONE = 0,
    ENV_ACTION_DISCONNECT_PAGE = 1,
    ENV_ACTION_DISCONNECT_CONN = 2
};

enum
{
    INTER_WAV = 0,
	INTER_SBC,
	EXT_WAV,
	EXT_SBC
};
enum
{
    PROMPT_CLOSED = 0,
	PROMPT_START,
	PROMPT_WORKING,
    PROMPT_FILL_ZERO,
	PROMPT_EMPTY,
	PROMPT_STOP
};

typedef enum
{
    DEVICE_NONE = 0x00,
    DEVICE_ANDROID = 0x01,
    DEVICE_IPHONE = 0x02,
    DEVICE_OTHERS = 0x03
}Device_Types;
typedef   struct __app_env_key_pair_s
{
    uint8_t   used;
    uint8   a2dp_src_uclass;  /* 1:mac/win book; 0:other */	
    #if (CONFIG_VOLUME_SAVE_IN_ENV == 1)
    int8_t      a2dp_vol;
    #endif
    btaddr_t     btaddr;
    uint8_t    linkkey[16];
    uint8_t    crystal_cali_data;
    uint16_t  aud_dc_cali_data;
}__PACKED_POST__ app_env_key_pair_t;

typedef   struct _app_program_chg_adc_s
{
    uint16_t program_flag;
    uint16_t reversed1;
    uint8_t chg_flag;
    uint8_t chg_val;
    uint8_t adc_flag;
    uint16_t adc_val;
    uint8_t dac_flag;
    int16_t dac_L_val;
    int16_t dac_R_val;
    uint8_t   reserved[18];
}__PACKED_POST__ app_program_chg_adc_t;

typedef struct _env_charge_cali_data_s
{
    uint8_t charger_vlcf;
    uint8_t charger_icp;
    uint8_t charger_vcv;
}__PACKED_POST__  env_charge_cali_data_t;

typedef struct env_saradc_cali_data_s
{
    uint16_t sar_adc_dc;
    uint16_t sar_adc_4p2;
}__PACKED_POST__  env_saradc_cali_data_t;

typedef struct env_lpbg_cali_data_s
{
    uint8_t   cali_lpbg_bandgap;
    uint8_t   cali_lpbg_vddcore;
    uint8_t   cali_lpbg_vddio;
}__PACKED_POST__  env_lpbg_cali_data_t;

typedef struct env_aud_dc_offset_data_s
{
    uint8_t   dac_l_dc_offset[32];
    uint8_t   dac_r_dc_offset[32];
}__PACKED_POST__  env_aud_dc_offset_data_t;

typedef struct env_cali_data_s
{    
    unsigned int bandgap : 6;     
    unsigned int odd_bg: 1;
    unsigned int vddcore : 6;  
    unsigned int odd_vcore: 1;
    unsigned int vddio : 6; 
    unsigned int odd_vio: 1;
    unsigned int vcv : 5;   
    unsigned int icp : 5;  
    unsigned int odd_chgi: 1;

    unsigned int vlcf : 7;         
    unsigned int odd_chgv: 1;
    unsigned int vdc : 6; 
    unsigned int v4p2: 9;
    unsigned int odd_adc: 1;  
    unsigned int tmpr : 7; 
    unsigned int odd_t: 1;

    unsigned int aud_l:8;
    unsigned int aud_r:8;
    unsigned int odd_aud:1;
    unsigned int v4p2_sprt:3;
    unsigned int odd_v4p2_sprt:1;
} __PACKED_POST__ env_cali_data_t;

typedef   struct _app_bt_para_s
{
    uint32_t  bt_flag;

    int8_t    device_name[32];
    int8_t    device_pin[16];
    btaddr_t device_addr;
    uint8_t    device_class;

    int8_t    disconn_action;
    int32_t   disconn_start;
    int32_t   disconn_retry_interval;
    int8_t    disconn_retry_count;

    int8_t    auto_conn_count;
    uint16_t  auto_conn_interval;

    int16_t  match_timeout;
    uint8_t   pd_cond;
    uint8_t   action_disconn;
    uint32_t   reserved[4];
}__PACKED_POST__ app_bt_para_t;

typedef struct _app_ble_para_enh_s{
    uint16_t ble_flag;     //bit0:ble enable, bi1:addr/name_config enable, bit2:adv_config enble, bit3:conn_update enable

    btaddr_t device_addr;
	char   device_name[32];

    uint16_t adv_interval;

    uint16_t conn_update_delay_time;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
	uint32_t reserved[2];
}app_ble_para_enh_t;

#define MAX_KEY_STORE       8

typedef   struct _app_env_data_s
{
    uint8_t env_tag;
    app_env_key_pair_t  key_pair[MAX_KEY_STORE];
    uint8_t   key_index;
    btaddr_t default_btaddr;
    uint8_t disconnect_reason;//0xff:no disconnect, others defined by hci disconnect reason
    uint8_t   lang_sel;
    uint8_t	 chg_Vctrl;
    uint32_t   bt_para_valid_Flag;
	uint8_t    sw_power_down_flag;

#ifdef AUTO_CALIBRATE_FREQ_OFFSET
	uint8_t calib_freq_offset;
#else
	uint8_t calib_freq_offset;
#endif

#ifdef CONFIG_RF_CALI_TYPE
	//offset_bak:{b7:b6} = 11(no data) 10(have data) 00(not use)
	uint8_t offset_bak_8852;
	uint8_t offset_bak_tester;
	uint8_t offset_bak_phone;
#endif
	uint8_t eq_lang_sel;
}__PACKED_POST__ app_env_data_t;

typedef   struct _app_wave_info_s
{
	uint8_t	used;
	uint16_t	page_index;
}__PACKED_POST__ app_wave_info_t;

#define APP_ENV_SYS_FLAG_UARTDBG    0x00001
#define APP_ENV_SYS_FLAG_IIC        0x00002
#define APP_ENV_SYS_FLAG_DAC_DIFFER   0x00040
#define APP_ENV_SYS_FLAG_L_is_LplusR		  0x00080
#define APP_ENV_SYS_FLAG_LED_REVERSE   0x00100
#define APP_ENV_SYS_FLAG_SD_DETECT_ENA		0x00200
#define APP_ENV_SYS_FLAG_AD_LINEIN_ENA   	0x00400
#define APP_ENV_SYS_FLAG_USB_ENA   			0x00800
#define APP_ENV_SYS_FLAG_MIC_BIAS   0x06000 //mic_bias use bit13,bit14

#if (SYS_CFG_BUCK_ON == 1)
#define APP_SYSTEM_FLAG_DEFAULT          (APP_ENV_SYS_FLAG_UARTDBG | APP_ENV_SYS_FLAG_DAC_DIFFER) | ((1 << 13) | (1 << 14))
#else
#define APP_SYSTEM_FLAG_DEFAULT          (APP_ENV_SYS_FLAG_UARTDBG | APP_ENV_SYS_FLAG_DAC_DIFFER) | ((1 << 13) | (1 << 14))
#endif
enum
{
    HW_PWR_KEY = 0,
    SW_PWR_KEY,
	SW_PWR_KEY_SWITCH,
	SW_PWR_KEY_MOS_CTRL,
};

enum{
	PIN_pwrBtn=0, //SW_PWR:pwrOn button pin;;  
	PIN_SDA, //for simu I2C
	PIN_SCL,  //for simu I2C
	PIN_paMute,
	PIN_lineDet,
	PIN_sdDet,
	PIN_ioDet, //detect usb plug in  PIN_chagDet   SW_PWR_SWITCH: pwr switch pin
	PIN_mosCtrl, //ctrl mos
	PIN_earInDet, //ear In detect
	PIN_earInCs,
	PIN_MAX=16
	};
#if 0
typedef union{
	struct{
		uint8 num: 6;//pin脚序�?
		uint8 actLvl : 1;//有效电平
		uint8 en :1;//使能
		};
	uint8 b;
	}__PACKED_POST__
_t_pin_desc;
#endif
typedef unsigned char pin_desc;

typedef   struct _app_system_para1_s
{
    uint8_t	mos_soft_power_flag;
    //HW_PWR_KEY = 0,
    //SW_PWR_KEY = 1,
    //SW_PWR_KEY_SWITCH = 2,
    //SW_PWR_KEY_MOS_CTRL = 3,
    uint8_t charger_current;
    uint8_t pop_icon_index;

    uint32_t  system_flag;

	pin_desc pins[PIN_MAX];

    uint8_t   vol_a2dp;
    uint8_t   vol_hfp;
    uint8_t   vol_wave;
    uint8_t   vol_mic;

    uint32_t  lp_interval;
    uint16_t  lp_threshold;
    uint16_t  lp_pd_threshold;
    uint8_t   lp_channel;
    uint8_t frq_offset;

    uint16_t  sleep_timeout;
    int32   powerdown_timeout;
    int32	pause_powerdown_timeout;

	 uint8_t hfpMicChn; 
	 uint8_t ttMicChn; 
	 uint8_t ancMicChn; 

	 uint8_t reserved2[17];
}__PACKED_POST__ app_system_para_t;

#define APP_ENV_BT_FLAG_A2DP              0x00001
#define APP_ENV_BT_FLAG_HFP               0x00002
#define APP_ENV_BT_FLAG_HSP               0x00004
#define APP_ENV_BT_FLAG_AUTO_CONN         0x00010
#define APP_ENV_BT_FLAG_RECOV             0x00020
#define APP_ENV_BT_FLAG_ADDR_POLL         0x00040
#define APP_ENV_BT_FLAG_ADDR_AUDIO_DIAL   0x00080
#define APP_ENV_BT_FLAG_AUTO_CONN_PERIOD  0x00100
#define APP_ENV_BT_FLAG_APP_BAT_DISPLAY   0x00200

#define APP_ENV_BT_PD_FLAG_NOCONN         0x01
#define APP_ENV_BT_PD_FLAG_PAUSE_TO       0x02

#define APP_BT_FLAG_DEFAULT    (APP_ENV_BT_FLAG_AUTO_CONN|APP_ENV_BT_FLAG_A2DP|APP_ENV_BT_FLAG_HFP|APP_ENV_BT_FLAG_ADDR_POLL)

typedef   struct _app_button_para_s
{
    uint16_t    press;
    uint16_t    repeat;
    uint16_t    longp;
    uint16_t    vlongp;
    uint16_t    doublep;
	uint16_t    reserved;
}__PACKED_POST__ app_button_para_t;

#define APP_ENV_FEATURE_FLAG_INQUIRY_ALWAYS				0x00001
#define APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE			0x00002
#define APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE			0x00004
#define APP_ENV_FEATURE_FLAG_CHARGEING_LOW_I_Vusb		0x00008
#define APP_ENV_FEATURE_FLAG_CHARGE_TIMEOUT_ENABLE		0x00010
#define APP_ENV_FEATURE_FLAG_AUTCONN_TESTER				0x00020
#define APP_ENV_FEATURE_FLAG_CHARGE_MODE_PWR_DOWN		0x00040
#define APP_ENV_FEATURE_FLAG_MIC_SINGLE_ENABLE			0x00080
#define APP_ENV_FEATURE_FLAG_DAC_DC_COMPENSATION		0x00100
#define APP_ENV_FEATURE_FLAG_KEY_LOW_ACTIVE				0x00200
#define APP_ENV_FEATURE_FLAG_DISABLE_IOS_INCOMING_RING	0x00400
#define APP_ENV_FEATURE_FLAG_VOLUME_RESTORE				0x00800
#define APP_ENV_FEATURE_FLAG_VOLUME_SYNC				0x01000
#define APP_ENV_FEATURE_FLAG_PAIRING_AFTER_PWR_ON		0x02000
#define APP_ENV_FEATURE_FLAG_SPP_PROFILE				0x04000
#define APP_ENV_FEATURE_FLAG_HID_PROFILE				0x08000
#define APP_ENV_FEATURE_FLAG_AEC_ENABLE					0x10000
//#define APP_ENV_FEATURE_FLAG_DAC_ALWAYS_ON				0x20000
#define APP_ENV_FEATURE_FLAG_TWS_L_R_PWR_DOWN			0x20000
#define APP_ENV_FEATURE_FLAG_VCOM_OUT					0x40000
#define APP_ENV_FEATURE_FLAG_SNIFF_ENABLE				0x80000
#define APP_ENV_FEATURE_FLAG_FAST_MUTE					0x100000
#define APP_ENV_FEATURE_FLAG_AUX_MODE_PWR_DOWN		    0x200000
#define APP_ENV_FEATURE_FLAG_AUX_MODE_PAUSE_MUTE		0x400000
#define APP_ENV_FEATURE_FLAG_TWS_L_R_DIF_EN			    0x200000
#define APP_ENV_FEATURE_FLAG_TWS_L						0x400000
#define APP_ENV_FEATURE_FLAG_FREE_CONN					0x00020

#define APP_ENV_FEATURE_FLAG_POP_EN                     0x800000
#define APP_ENV_FEATURE_FLAG_MAX_BITPOOL                0x1000000
#define APP_ENV_FEATURE_FLAG_IAP_PROFILE				0x2000000
//#define APP_ENV_FEATURE_FLAG_TESTER					0x4000000
#define APP_ENV_FEATURE_FLAG_ANA_LOW_VOLT				0x8000000

typedef   struct _app_volume_s
{
	aud_volume_t vol[17];
//	uint8_t dig_vol[16];
//	uint8_t ana_vol[16];
}__PACKED_POST__ app_volume_t;

typedef   struct _app_rfpwr_s
{
    uint8_t big_adj;//xvr_0x24{bit10:bit7}:
}__PACKED_POST__ app_rfpwr_t;

typedef   struct _app_bat_display_s
{
	uint16_t bat_level[10];
}__PACKED_POST__ app_bat_display_t;

typedef   struct _app_sw_feature_s
{
    uint32_t  feature_flag;
    int32   charge_timeout;
    uint16_t  bt_mode;
    app_volume_t a2dp_vol;
    app_volume_t hfp_vol;
    app_volume_t linein_vol;
    uint8_t   vol_mic_dig;
    uint8_t   vol_mic_ana;
    uint8_t   vol_linein_dig;
    uint8_t   vol_linein_ana;
    uint8_t	pa_mute_delay_time;
    uint8_t   pa_unmute_delay_time;
    uint16_t   sw_pwr_on_key_time;
    uint16_t   sw_pwr_off_key_time;
    uint16_t   enter_pairing_key_time;
    app_rfpwr_t a2dp_rf_pwr;
    app_rfpwr_t hfp_rf_pwr;
    app_bat_display_t bat_display_level;
    btaddr_t tester_bt_addr;
    uint8 ana_dig_volt;
    uint8 tws_eir;
    uint8_t reserved[64-8];
}__PACKED_POST__ app_sw_feature_t;

typedef struct _app_drc_para_s
{
	uint8_t enable;
	uint8_t flag;
	uint16_t before_vol;
	uint16_t post_vol;
	uint16_t rms_time;
	uint16_t threshold;
	uint16_t attack_time;
	uint16_t release_time;
	uint8_t reserved[2+16];
}__PACKED_POST__ app_drc_para_t;
typedef   struct _app_env_cfg_s
{
    int8_t    used;
    app_system_para_t   system_para;
    app_bt_para_t       bt_para;
    app_ble_para_enh_t		ble_para; //56B
    uint8_t       wave_lang_sel;
    uint8_t        led_map[LED_NUM];
    app_led_info_t  led_info[LED_EVENT_END];
    app_button_para_t  button_para;

    uint64_t   button_code[BUTTON_BT_END];
    app_aud_eq_t aud_eq;
    app_hfp_cfg_t hfp_cfg;
    app_drc_para_t drc;
    app_sw_feature_t feature;
    app_wave_info_t     wave_info[WAVE_EVENT];
    app_wave_info_t 	wave_info1[WAVE_EVENT];
    app_wave_info_t     wave_info2[WAVE_EVENT];
    app_wave_info_t     wave_info3[WAVE_EVENT];
    app_wave_info_t 	wave_info4[WAVE_EVENT];
}__PACKED_POST__ app_env_cfg_t;

typedef struct _app_env_s
{
    app_env_data_t  env_data;
    app_env_cfg_t	 env_cfg;
}app_env_t;

typedef app_env_t * app_env_handle_t;
env_lpbg_cali_data_t *app_get_env_lpbg_cali_data(void);
env_saradc_cali_data_t * app_get_env_saradc_cali_data(void);
env_charge_cali_data_t * app_get_env_charge_cali_data(void);
env_aud_dc_offset_data_t* app_get_env_dc_offset_cali(void);
uint32_t app_get_env_aud_cali_valid(void);

void app_env_cali_data_init(void);
#if (CONFIG_ANC_ENABLE == 1)
void app_anc_cali_data_init(void);
#endif
void app_env_cali_data_prt(void);
#if EFUSE_EN
uint8_t eFuse_get_cali_data(void);
#endif
void app_poweron_xtal_self_test(void);
void app_env_init( void );
void app_env_post_init( void );
void app_init( void );
void app_post_init( void );
void app_env_reconfig_ana_reg(int8_t ana_offset,int8_t dig_offset);
void app_env_reconfig_ana_voltage(int8_t ana_offset);
void app_env_reconfig_dig_voltage(int8_t dig_offset);
void app_reset(void);
app_env_t * app_env_get_handle( void );
int app_env_key_stored( btaddr_t *addr );
int app_env_key_delete( btaddr_t *addr );
int app_env_keypair_used_delete( btaddr_t *addr );
int8_t app_get_env_conn_retry_count(void);
int8_t app_get_env_conn_auto_count(void);
uint32_t app_get_env_profile_attrs(void);
int app_get_env_key_num( void );
int app_get_env_key_num_total( void );
int app_get_active_linkey( int Seq );
btaddr_t *app_get_active_remote_addr(uint8_t seq );
int app_env_store_autoconn_info( btaddr_t * remote_addr,  uint8_t link_key[16] );
btaddr_t* app_env_get_key_free( void );
int app_env_write_action( btaddr_t *raddr,uint8_t connected);
void app_env_power_on_check(void);
void app_flash_read_env_data(void);
void app_flash_read_env_cfg(void);
void app_flash_write_env_data(void);
int app_env_write_flash(uint8_t connected, uint8_t id);
void app_env_unit_info_init( char *name, uint8_t *dev_class, char *pin );
btkey_t * app_env_get_linkkey( btaddr_t *addr );
void app_env_clear_key_info( btaddr_t *addr );
void app_env_clear_all_key_info( void );
int app_env_get_wave_type( int wave_id );
int app_env_get_wave_page_index( int wave_id );
int app_env_get_wave_param( int wave_id, uint32_t *param1, uint32_t *param2 );
uint8_t app_env_get_wave_max_lang_num(void);
void app_env_dump(void);
int env_get_auto_conn_info(int start_index, int *id);
int env_get_auto_conn_info_max_id(void);
void app_env_pwrctrl_by_switch(uint32_t step);
int app_env_check_feature_flag(uint32_t flag);
int app_env_check_inquiry_always(void);
int app_env_check_SPP_profile_enable(void);
int app_env_check_HID_profile_enable(void);
void app_env_rf_pwr_set(uint8_t type);
int app_env_check_pwrCtrl_mode(void);
int app_env_check_DAC_POP_handle_enable(void);
int app_env_check_Charge_Mode_PwrDown(void);
int app_env_check_Charge_Det_use_gpio(void);
int app_env_check_Charge_Time_Out_Enable(void);
int app_env_check_Charge_Mode_HighEfficiency(void);
int app_env_check_sniff_mode_Enable(void);
int app_env_check_Use_ext_PA(void);
int app_env_check_bat_display(void);
uint32_t app_env_get_flash_addr(t_TYPE_SECTOR type);
int app_env_system_uart_dbg_enable(void);
void app_env_core_pm_init(void);

int app_env_get_pin_enable(uint8_t pin_index);
int app_env_get_pin_valid_level(uint8_t pin_index);
int app_env_get_pin_num(uint8_t pin_index);
void app_env_set_pin_value(uint8_t pin_index,uint8_t val);
#if POWER_ON_OUT_CHARGE_ENABLE
int app_env_check_auto_pwr_on_enable(void);
#endif
#endif
btaddr_t* app_env_local_bd_addr(void);
int8_t *app_env_local_bd_name(void);

