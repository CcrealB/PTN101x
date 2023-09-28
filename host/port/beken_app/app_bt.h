#ifndef _APP_BT_H_
#define _APP_BT_H_

#include "config.h"

typedef enum 
{
    BT_DEVICE_NONE,
    BT_DEVICE_PHONE,
    BT_DEVICE_COMPUTER,
    BT_DEVICE_TV,
    BT_DEVICE_OTHER,
}t_app_bt_device_type;

typedef struct test_control_s
{
    uint32  tester_lap_nap;
    uint8   test_scenarios;
    uint8   hop_mode;
    uint8   tx_chnl;
    uint8   rx_chnl;
    uint8   interval;
    uint8   packet_type;
    uint8   tx_power;
}__PACKED_POST__ t_TestControl;

typedef struct _bt_entity_s
{
    uint32_t init_flag;
    btaddr_t btaddr_def;
}BT_ENTITY_T;

typedef struct _bt_sniff_s
{
    btaddr_t    remote_btaddr;
    uint16_t    link_handle;
    uint8_t     is_active;
    uint8_t     is_used;
    uint8_t     is_policy;
}BT_SNIFF_T, *bt_sniff_t;

typedef enum
{
	SCO_DISCONNECTED = 0,
	SCO_CONNECTED	 = 1,
	SCO_SWAP	 = 2
}t_sco_active_state;

/* A2DP Notifications */
typedef struct {
    uint32_t               svc_id;
    bt_a2dp_sink_session_h svc;
    bt_a2dp_sink_session_h srv;
    uint32_t               sdp;
    audio_out_ctx_h        audio_out;

    uint32_t 		       freq;
    uint32_t		       channel;

    uint32_t               flag;
    bt_a2dp_codec_t        codec;

    uint8_t                codec_type;
    int8_t                 volume;
    int8_t                 is_used;

    bt_link_modes_mask_t   conn_mode;
    bt_conn_req_h          conn_req;

    btaddr_t               raddr;

#if defined(A2DP_VENDOR_DECODE) || defined(A2DP_MPEG_AAC_DECODE)
    uint32_t	bps;
#endif //A2DP_VENDOR_DECODE
    uint8_t		avrcp_play_pause; //0 = pause, 1 = play
} a2dp_backend_t;

enum
{
    BT_DISCONNECTED = 0,
    BT_DISCONNECTING,
};

#define INITIATIVE_CONNECT_START        1
#define INITIATIVE_CONNECT_STOP         0
#define INITIATIVE_CONNECT_HID         2

typedef struct _system_s
{
    /*led part */
    uint8_t        led_onoff[LED_NUM];
    uint16_t       led_repeat_count[LED_NUM];
    uint16_t       led_repeat[LED_NUM];
    uint8_t        led_num_flash_count[LED_NUM];
    uint8_t        led_num_flash[LED_NUM];
    uint16_t       led_timeout[LED_NUM];
    uint16_t       led_ontime[LED_NUM];
    uint16_t       led_offtime[LED_NUM];
    uint8_t        led_map[LED_NUM];
    uint16_t       led_onoff_count[LED_NUM];
    app_led_info_t led_info[LED_EVENT_END];
    uint8_t        led_event_save;
    uint8_t        led_event;

    //button part */
    uint64_t button_mode;
    int     button_press_count;
    int     button_long_press_count;
    APP_BUTTON_STATE button_state;
    uint64_t   button_commit;
    uint64_t   button_type;
    uint32_t   button_count;
    //const uint32_t *button_code;
    //const button_action_func *button_handler;
	uint64_t   button_code[BUTTON_BT_END];
    button_action_func  button_handler[BUTTON_BT_END];

    uint64_t  button_mask;
    uint64_t  btn_init_map;

    uint8_t  button_mode_adc_enable;
    uint8_t  button_mode_adc_chan;
    uint8_t  button_mode_adc_sum;

    int8_t     volume_store;
    int8_t     mic_volume_store;

    jtask_h   app_auto_con_task;
    jtask_h   app_reset_task;
    jtask_h   app_match_task;
    jtask_h   app_audio_task;
	jtask_h	  app_a2dp_task;
	jtask_h	  app_only_a2dp_hfp_link_to_scan_task;
    #if (defined(BT_SD_MUSIC_COEXIST))
    jtask_h   app_coexist_task;
    #endif

    uint32_t   low_detect_count;
    int32_t   powerdown_count;
    uint16_t   linein_count;
    int8_t     low_detect_channel;
	uint32_t	 charg_timeout_cnt;
    uint16	 charg_jitter_cnt;
	
    uint16_t   press_thre;
    uint16_t   double_thre;
    uint16_t   long_thre;
    uint16_t   vlong_thre;
    uint16_t   repeat_thre;

    hci_unit_t *unit;
    uint16_t   link_handle;
    btaddr_t   remote_btaddr;

    btaddr_t   reconn_btaddr;
    btaddr_t   reconn_btaddr2;
    uint8_t     reconn_num_flag;

    uint8_t   auto_conn_id;
    uint8_t   auto_conn_id1;
    uint8_t   auto_conn_id2;
    uint8_t   auto_conn_status;

    uint8_t    hfp_rfcomm_channel;

    int8_t     linein_vol;
#if A2DP_ROLE_SOURCE_CODE
	btaddr_t   stereo_btaddr;
	uint8_t 	inquiry_ticks;
    uint8_t   num_stereo_cnt;
	jtask_h 	app_stereo_task;
	jtask_h 	app_sync_task;
#endif

    uint32_t   flag_sm1;
    uint32_t   flag_sm2;

	uint32_t adc_threshold_cal_count;
//#if (CONFIG_CHARGE_EN == 1)
	uint8_t  flag_charge;
//#endif
    //add by zjw for cov common&protocol
    jtask_h   app_common_task; //add by zjw for sdcard test
    uint8_t HF_establish_call;
    uint32_t sys_work_mode;
    uint32_t bt_mode;

    int32_t pause_powerdown_count;

    jtask_h app_save_volume_task; // save volume info to flash
    jtask_h app_dlp_task;
#if (CONFIG_DRIVER_OTA == 1)
    jtask_h ota_reboot_task;
#endif
    int8 iphone_bat_lever;
    int8 iphone_bat_lever_bak;
#if (CONFIG_OTHER_FAST_PLAY == 1)
	uint16_t otherplay;
    int8 fast_state;
#endif
    uint8_t is_wechat_phone_call;
#if CONFIG_A2DP_CONN_DISCONN
	uint8_t a2dp_state;
#endif
#if CONFIG_HFP_CONN_DISCONN
	uint8_t hfp_state;
#endif
#if (REMOTE_RANGE_PROMPT == 1)
    uint8 flag_soft_reset; 
#endif
    uint8_t phone_type; //0: not check yet, 1:andriod, 2:iphone, 3: ios lower than 10, 4:ios lower than 13
}APP_SYSTEM_T, *app_handle_t;

#define WAKEUP_GPIO                   (15)

#define APP_LOWPOWER_DETECT_INTERVAL         1000   // 30s

#define AUTO_RE_CONNECT_SCHED_DELAY_HFP				311
#define AUTO_RE_CONNECT_SCHED_DELAY_A2DP_SWITCH		112
#define AUTO_RE_CONNECT_SCHED_DELAY_A2DP_NOSWITCH	102
#define AUTO_RE_CONNECT_SCHED_DELAY_AVRCP			103
#define AUTO_RE_CONNECT_SCHED_DELAY_AVDTP			104


#define APP_BUTTON_FLAG_BT_ENABLE           0x00000001
#define APP_FLAG_MUSIC_PLAY                 0x00000002
#define APP_BUTTON_FLAG_PLAY_PAUSE          0x00000004
#define APP_FLAG_AUTO_CONNECTION            0x00000008

#define APP_FLAG_AVCRP_CONNECTION           0x00000010
#define APP_FLAG_AVCRP_PROCESSING           0x00000020
#define APP_FLAG_A2DP_CONNECTION            0x00000040
#define APP_FLAG_ACL_CONNECTION             0x00000080

#define APP_FLAG_RECONNCTION		        0x00000100
#define APP_FLAG_MATCH_ENABLE               0x00000200
#define APP_FLAG_DUT_MODE_ENABLE            0x00000400
#define APP_FLAG_HFP_CONNECTION             0x00000800

#define APP_FLAG_HSP_CONNECTION             0x00001000
#define APP_FLAG_POWERDOWN                  0x00002000
#define APP_FLAG_HFP_CALLSETUP              0x00004000
#define APP_FLAG_SCO_CONNECTION             0x00008000

#define APP_FLAG_FIRST_ENCRYP               0x00010000
#define APP_FLAG_HFP_OUTGOING               0x00020000
#define APP_FLAG_ROLE_SWITCH                0x00040000
#define APP_FLAG_WAVE_PLAYING               0x00080000

#define APP_FLAG_MUSIC_PLAY_SCHEDULE        0x00100000
#define APP_FLAG_HID_CONNECTION             0x00200000
#define APP_FLAG_LINEIN                     0x00400000
#define APP_FLAG_LOWPOWER                   0x00800000

#define APP_FLAG_HFP_BAT_LEVEL              0x01000000
#define APP_FLAG_SNIFF_MODE_CHANGE          0x02000000
#define APP_FLAG_MODE_CHANGE_SETTING        0x04000000
#define APP_FLAG_FCC_MODE_ENABLE		    0x08000000

#define FLAG_HFP_AUTOCONN_DISCONNED 	    0x10000000 // hfp deny
#define APP_FLAG_CALL_ESTABLISHED      	    0x20000000
#define APP_FLAG_DISCONN_RECONN        	    0x40000000
#define APP_FLAG_AVRCP_FLAG         	    0x80000000  // old name:APP_FLAG_DEBUG_FLAG

#define APP_FLAG2_LED_LINEIN_INIT   		0x00000001
#define APP_FLAG2_VOL_MUTE                  0x00000002
#define APP_FLAG2_HFP_INCOMING      		0x00000004    //an incoming call exist(call not exist)
#define APP_FLAG2_MUTE_FUNC_MUTE    		0x00000008

#define APP_FLAG2_LED_LOWPOWER      		0x00000010
//#define APP_FLAG2_DAC_OPEN          		0x00000020
#define APP_FLAG2_5S_PRESS			        0x00000020
#define APP_FLAG2_VOL_SYNC_OK               0x00000040
#define APP_FLAG2_BATTERY_FULL			    0x00000080

//#define APP_FLAG2_CHARGING	            0x00000100
#define APP_FLAG2_CHARGE_POWERDOWN	        0x00000100
#define APP_FLAG2_CALL_PROCESS           	0x00000200
#define APP_FLAG2_RECONN_AFTER_CALL      	0x00000400
#define APP_FLAG2_RECONN_AFTER_DISCONNEC 	0x00000800

#define APP_FLAG2_HFP_SECOND_CALLSETUP   	0x00001000
#define APP_FLAG2_MATCH_BUTTON           	0x00002000
#define APP_FLAG2_WAKEUP_DLY			 	0x00004000
#define APP_FLAG2_RECONN_AFTER_PLAY     	0x00008000
#define APP_FLAG2_VOL_SYNC			 		0x00010000
#define APP_FLAG2_SW_MUTE			 		0x00020000
#define AP_FLAG2_LINEIN_MUTE				0x00040000
#define APP_FLAG2_STEREO_STREAMING        	0x00800000 //0x00040000
#define APP_FLAG2_STEREO_AUTOCONN         	0x00080000
//#define APP_FLAG2_AVRCP_OPCODE_PLAY       0x00800000

#define APP_FLAG2_STEREO_AUTOCONN_RETRY   	0x00100000
#define APP_FLAG2_STEREO_BUTTON_PRESS     	0x00200000
#define APP_FLAG2_STEREO_MATCH_BUTTON       0x00400000
//for APP_FLAG2_TWC_HOLD_CALLID				0x00800000

#define APP_FLAG2_ROLE_SWITCH_FAIL		 	0x01000000
#define APP_FLAG2_STEREO_INQUIRY_RES    	0x02000000
#define APP_FLAG2_STEREO_ROLE_MASTER    	0x04000000
#define APP_FLAG2_STEREO_WORK_MODE      	0x08000000

#define APP_FLAG2_STEREO_CONNECTTING    	0x10000000
//#define APP_FLAG2_STEREO_SLAVE_ADJ_CLK   	0x20000000
#define APP_FLAG2_STEREO_PLAYING   		    0x40000000
#define APP_FLAG2_VUSB_DLP_PRINTF     		0x80000000
#define APP_FLAG2_CHARGE_NOT_POWERDOWN   	0x20000000


#define APP_STEREO_FLAG_SET    (APP_FLAG2_STEREO_WORK_MODE \
            |APP_FLAG2_STEREO_CONNECTTING|APP_FLAG2_STEREO_INQUIRY_RES|APP_FLAG2_STEREO_STREAMING)
//|APP_FLAG2_STEREO_ROLE_MASTER
#define APP_BT_INQUIRY_TICKS			5
#define APP_BT_INQUIRY_DURATION 	6


//--------------------profile private flag--------------------------
//---a2dp flag----
#define APP_A2DP_FLAG_STREAM_START              0x01
#define APP_A2DP_FLAG_MUSIC_PLAY                APP_FLAG_MUSIC_PLAY          //0x02
#define APP_A2DP_FLAG_PLAY_PAUSE                APP_BUTTON_FLAG_PLAY_PAUSE   //0x04

#define APP_A2DP_FLAG_A2DP_CONNECTION           APP_FLAG_A2DP_CONNECTION     //0x40
#define APP_A2DP_PRIVATE_FLAG_FLOW_CTRL			0x08
//hfp flag
#define APP_HFP_PRIVATE_FLAG_AG_IN_BAND_RING    0x00000100
#define APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING     0x00400000
#define APP_HFP_PRIVATE_FLAG_TWC_MODE           0x00800000
#define APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE     0x02000000
#define APP_HFP_PRIVATE_FLAG_CODEC_NEGOTIATION  0x04000000
//#define APP_HFP_PRIVATE_FLAG_ALERTING_STATE   0x0040000
#define APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1     0x00000010
#define APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2     0x00000020


#if (CONFIG_BT_USER_FLAG3 == 1)
#define PWR_ON_BT_SCAN_DIS                      0x00000001U
#define PWR_ON_BT_RECONNECT_DIS                 0x00000002U
#endif

//avrcp flag
//sco flag
#define APP_SCO_PRIVATE_FLAG_CONNECTION                      APP_FLAG_SCO_CONNECTION
#define APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE		0x08

//--------------------profile private flag--------------------------
#define APP_AUDIO_FLAG_SET      (APP_FLAG_MUSIC_PLAY\
                                   |APP_FLAG_AVCRP_CONNECTION\
                                   |APP_FLAG_AVCRP_PROCESSING\
                                   |APP_FLAG_A2DP_CONNECTION\
                                   |APP_FLAG_ACL_CONNECTION\
                                   |APP_FLAG_HFP_CONNECTION\
                                   |APP_FLAG_HSP_CONNECTION\
                                   |APP_FLAG_HFP_CALLSETUP\
                                   |APP_FLAG_SCO_CONNECTION\
                                   |APP_FLAG_FIRST_ENCRYP\
                                   |APP_FLAG_ROLE_SWITCH)
#define APP_AUDIO_WORKING_FLAG_SET  (APP_FLAG_MUSIC_PLAY \
                                    |APP_BUTTON_FLAG_PLAY_PAUSE\
                                    |APP_FLAG_SCO_CONNECTION\
                                    |APP_FLAG_WAVE_PLAYING\
                                    |APP_FLAG_AUTO_CONNECTION\
                                    |APP_FLAG_LINEIN\
                                    |APP_FLAG_RECONNCTION)

#define APP_TV_WORKING_FLAG_SET     (APP_FLAG_WAVE_PLAYING\
                                    |APP_FLAG_AUTO_CONNECTION\
                                    |APP_FLAG_LINEIN\
                                    |APP_FLAG_RECONNCTION)                                    

#define APP_HFP_AT_CMD_FLAG1_SET    (APP_FLAG_HFP_CALLSETUP \
                                    |APP_FLAG_HFP_OUTGOING\
                                    |APP_FLAG_CALL_ESTABLISHED)

#define APP_HFP_AT_CMD_FLAG2_SET    (APP_FLAG2_HFP_INCOMING \
                                    |APP_FLAG2_CALL_PROCESS\
                                    |APP_FLAG2_HFP_SECOND_CALLSETUP)

#define APP_HFP_PRIVATE_FLAG_SET    (APP_HFP_PRIVATE_FLAG_AT_CMD_PENDING \
                                    |APP_HFP_PRIVATE_FLAG_AG_IN_BAND_RING \
                                    |APP_HFP_PRIVATE_FLAG_CODEC_NEGOTIATION \
                                    |APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE \
                                    |APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE1 \
                                    |APP_HFP_PRIVATE_FLAG_CALL_SEQUENCE2 \
                                    |APP_HFP_PRIVATE_FLAG_TWC_MODE)

#define APP_SDP_DISABLE_FLAG_SET  (APP_FLAG_MUSIC_PLAY|APP_FLAG_A2DP_CONNECTION|APP_FLAG_HFP_CONNECTION)

#define APP_MATCH_FLAG_SET    (APP_FLAG_MUSIC_PLAY|APP_FLAG_AVCRP_CONNECTION\
                                   |APP_FLAG_AVCRP_PROCESSING|APP_FLAG_ACL_CONNECTION\
                                   |APP_FLAG_HFP_CALLSETUP|APP_FLAG_SCO_CONNECTION|\
                                   APP_FLAG_FIRST_ENCRYP|APP_FLAG_ROLE_SWITCH)

#define APP_CPU_NOT_HALT_FLAG_SET ( APP_FLAG_FCC_MODE_ENABLE\
                                    |APP_FLAG_AUTO_CONNECTION\
                                    |APP_FLAG_RECONNCTION\
                                    |APP_FLAG_DUT_MODE_ENABLE\
                                    |APP_FLAG_POWERDOWN\
                                    |APP_FLAG_LINEIN\
                                    |APP_FLAG_WAVE_PLAYING\
                                    |APP_FLAG_MUSIC_PLAY\
                                    |APP_FLAG_SCO_CONNECTION)

#define APP_PROFILES_FLAG_SET  (APP_FLAG_A2DP_CONNECTION\
                                   |APP_FLAG_HFP_CONNECTION\
                                   |APP_FLAG_AVCRP_CONNECTION)

#define APP_EARIN_WAV_PROMT_FLAG_SET  (APP_BUTTON_FLAG_PLAY_PAUSE\
                                   |APP_FLAG_HFP_CALLSETUP\
                                   |APP_FLAG_CALL_ESTABLISHED\
                                   |APP_FLAG_SCO_CONNECTION) 
enum
{
    PROFILE_ID_A2DP  = 0,
    PROFILE_ID_AVRCP = 1,
    PROFILE_ID_HFP   = 2,
    PROFILE_ID_HSP   = 3,
    PROFILE_ID_HID   = 4,
#if A2DP_ROLE_SOURCE_CODE
    PROFILE_ID_A2DP_SRC = 5,
    PROFILE_ID_AVRCP_CT = 6
#endif

};


#define APP_RETRY_FOREVER               -1
#define APP_AUTO_RECONN_RETRY_TIMES     3

#define APP_AUTO_CONN_RETRY_TIMES       3

#define APP_A2DP_CONN_ACTIVE_DELAY 20000  // 20s

#define APP_DISCONN_ACTION_RETRY_TIMES  APP_RETRY_FOREVER
#define APP_DISCONN_ACTION_FIRST_DELAY	200 //6000
#define APP_DISCONN_ACTION_TIMEOUT	 	6000 //5120
#define APP_BT_DISCONN_TIMEOUT          5000



__inline app_handle_t app_get_sys_handler(void);
jtask_h app_get_audio_task_handle( void );
void app_bt_enable_non_signal_test(t_TestControl *tc_contents);

void app_sleep_func( int enable );
void app_bt_write_sniff_link_policy(uint16_t link_handle,uint8_t set_link_policy);
void app_bt_enter_sniff_mode(uint16_t link_handle,uint8_t enable);
void app_bt_enable_complete_wrap( hci_unit_t *unit );


int bt_is_reject_new_connect(void);
void bt_create_reconn_timeout_wrap(void *arg);
int app_bt_flag1_get(uint32_t flag);
int app_bt_flag2_get(uint32_t flag);
void app_bt_flag1_set(uint32_t flag, uint8_t oper);
void app_bt_flag2_set(uint32_t flag, uint8_t oper);
#if (CONFIG_BT_USER_FLAG3 == 1)
int app_bt_flag3_get(uint32_t flag);
void app_bt_flag3_set(uint32_t flag, uint8_t op);
#endif

//void bt_select_other_a2dp_avrcp(uint8_t addr_same);

void bt_all_acl_disconnect(hci_unit_t *unit);
uint8 bt_acl_con_get_specific_uclass(void);

result_t bt_hfp_peeraddr(void *hf_session, btaddr_t *raddr);
uint8_t Judge_role(btaddr_t *btaddr,BOOL connecting);
void judge_role_disconnect(btaddr_t* rtaddr);
btaddr_t *get_pcb_raddr(struct sco_pcb *pcb);
result_t get_play_a2dp_avrcp_state(uint32_t *index);
//void get_raddr_from_a2dp(btaddr_t**raddr0,btaddr_t**raddr1);
void select_other_avrcp_app_pause_by_raddr(btaddr_t *raddr);
#if(CONFIG_A2DP_PLAYING_SET_AG_FLOW == 1)
hci_link_t *select_curr_playing_link_handle_by_a2dp(a2dp_backend_t *app_ptr);
hci_link_t *select_flow_ctrl_link_handle_by_raddr(btaddr_t *raddr);
void bt_set_ag_flow_ctrl_by_handle(hci_link_t *link);
void bt_set_ag_flow_ctrl_timeout(void *arg);
hci_link_t *select_flow_ctrl_link_handle(void);
#endif

uint32_t avrcp_get_raddr(void *arg, btaddr_t *raddr);
uint32_t avrcp_update_current_app(const btaddr_t *raddr);
void avrcp_select_current_app_to_another(const btaddr_t *raddr);
uint32_t avrcp_current_is_connected(void);
uint32_t avrcp_current_is_processing(void);
void avrcp_current_send_opcode(void *arg);
void avrcp_send_opcode_by_raddr(const btaddr_t *raddr, void *arg);
btaddr_t *a2dp_get_current_app_remote_addr(void);
btaddr_t *a2dp_get_not_active_remote_addr(void);
uint32_t avrcp_has_connection(void);
uint32_t avrcp_is_cmd_processing(void);
void avrcp_volume_sync();
uint32_t avrcp_has_the_connection(btaddr_t addr);

void select_play_a2dp_avrcp(void* arg);
void app_bt_profile_disconn_wrap(uint8_t profileId, void *app_ptr);
void app_bt_reset_policy_iocap(void);

void app_bt_conn_compl_wrap(hci_link_t * link, const btaddr_t  *rbtaddr);
void app_bt_disable_complete_wrap( void );
int app_is_conn_be_accepted( void );

/**************************A2DP function BEGIN*********************************/
uint32_t a2dp_get_remote_addr(void *arg, btaddr_t *r_addr);
uint32_t a2dp_has_another_music(const btaddr_t *raddr);
uint32_t get_a2dp_priv_flag(uint8_t idx,uint32_t flag);
void a2dp_select_current_app_to_another(btaddr_t *addr);
uint32_t a2dp_all_apps_is_unused(void);
uint32_t a2dp_has_connection(void);
uint32_t a2dp_has_the_music(const btaddr_t *raddr);
uint32_t a2dp_has_the_connection(const btaddr_t *raddr);
uint32_t a2dp_has_another_connection(const btaddr_t *raddr);

void set_a2dp_array_play_pause_status(const btaddr_t *raddr, uint8_t status);
uint8_t get_a2dp_array_play_pause_status(const btaddr_t *raddr);
uint32_t a2dp_has_another_music_avrcp_playing(const btaddr_t *raddr);


void a2dp_current_set_flag(uint32_t flag);
void a2dp_current_clear_flag(uint32_t flag);
uint32_t a2dp_current_check_flag(uint32_t flag);
uint32_t a2dp_check_flag_on_raddr(const btaddr_t *raddr, uint32_t flag);
void a2dp_audio_action(void);
/************************A2DP function END*************************************/
uint8 set_iphone_bat_lever(void);
uint32_t hfp_get_remote_addr(void *arg, btaddr_t *r_addr);
uint32_t hfp_is_connection_based_on_raddr(btaddr_t *raddr);
uint32_t hfp_check_autoconn_disconned_flag(btaddr_t *r_addr);
uint32_t hfp_has_connection(void);
uint32_t hf_clear_autoconn_diconned_flag(void *raddr);
void hf_set_flag(void *arg, uint32_t flag);
uint32_t hf_check_flag(void *arg, uint32_t flag);
uint32_t get_hf_priv_flag(uint8_t idx,uint32_t flag);
void hf_clear_flag(void *arg, uint32_t flag);
result_t hf_cmd_connect(char *params, unsigned int len);
int avrcp_is_connected_based_on_raddr(const btaddr_t *raddr);
result_t hid_int_conn_disconnect(void);
result_t hid_cmd_connect(char *params, unsigned int len);
void Set_remoteDev_role_btaddr(btaddr_t *btaddr,uint32_t role);
void bt_create_conn_status_wrap(uint8_t status);
void otm_current_a2dp_stream_off_handler(void *arg);
uint8_t hci_get_acl_link_role(hci_unit_t *unit,btaddr_t* raddr);
btaddr_t* hci_find_acl_slave_link(hci_unit_t *unit);
int hci_get_acl_link_count(hci_unit_t *unit);
int hci_get_acl_link_addr(hci_unit_t *unit, btaddr_t** raddr);
int hci_check_acl_link(btaddr_t* raddr);
uint32_t get_current_hfp_flag(uint32_t flag);
uint32_t get_current_hfp_freq(void);
int8_t get_current_hfp_volume(void);
void set_current_hfp_flag(uint32_t flag,uint8_t op);
void app_bt_reconn_after_callEnd(void);
void bt_exchange_hf_active_by_handle(uint16_t handle);
uint8_t hf_exchange_sco_active_flag(btaddr_t *btaddr,t_sco_active_state state);
uint16_t  get_hf_active_sco_handle(void);
void hfp_2eSCOs_A_B_SWAP(void);
void set_2hfps_sco_and_incoming_flag(uint8_t val);
uint8_t get_2hfps_sco_and_incoming_flag(void);
uint8_t check_2hfps_sco_and_incoming_status(void);
void set_hf_flag_1toN(btaddr_t *btaddr,uint32_t flag);
void set_hf_flag_call_seq(btaddr_t *btaddr);
void clear_hf_flag_1toN(btaddr_t *btaddr,uint32_t flag);
uint32_t get_hfp_flag_1toN(btaddr_t *btaddr,uint32_t flag);
uint32_t has_hfp_flag_1toN(uint32_t flag);
int hf_sco_disconn_reconn(int oper);
void change_cur_hfp_to_another_call_conn(btaddr_t *btaddr);
uint32_t hfp_has_sco(void);
uint32_t hfp_has_the_sco(btaddr_t addr);
uint32_t hfp_has_the_connection(btaddr_t addr);
void *hfp_get_app_from_priv(sco_utils_h priv);

void bt_sniff_alloc_st(uint8_t idx,uint16_t handle,btaddr_t *rbtaddr);
void bt_sniff_free_st(uint8_t idx);
uint8_t bt_sniff_find_st_available(void);
uint16_t bt_sniff_get_handle_from_raddr(btaddr_t *rbtaddr);
uint8_t bt_sniff_get_index_from_handle(uint16_t handle);
uint8_t bt_sniff_get_index_from_raddr(btaddr_t *rbtaddr);
uint16_t bt_sniff_get_handle_from_idx(uint8_t idx);
btaddr_t bt_sniff_get_rtaddr_from_idx(uint8_t idx);
btaddr_t *bt_sniff_get_addrptr_from_idx(uint8_t idx);

uint8_t bt_sniff_is_used(uint8_t idx);
uint8_t bt_sniff_is_active(uint8_t idx);
void bt_sniff_set_active(uint8_t idx,uint8_t enable);
uint8_t bt_sniff_is_policy(uint8_t idx);
void bt_sniff_set_policy(uint8_t idx,uint8_t enable);
uint8_t bt_sniff_is_all_active(void);
void bt_sniff_all_exit_mode(void);
int8_t app_is_available_sleep_system(void);
int app_is_not_powerdown(void);
void app_bt_pinkey_missing(btaddr_t* addr);
void app_bt_reset_policy_iocap( void );
void app_bt_auto_conn_ok( void );

void app_bt_shedule_task( jthread_func func, void *arg, uint32_t delay_ms );
void app_bt_get_remote_addr( btaddr_t *addr);
void *app_bt_get_handle( uint8_t handle_type );
int app_bt_get_disconn_event( void );
void app_bt_connected_wrap( btaddr_t *raddr);
void app_bt_disconn_wrap(uint8_t reason, btaddr_t *raddr);
void app_bt_acl_time_out_wrap( void );
void app_bt_key_store_wrap( btaddr_t *raddr, uint8_t *key, uint8_t key_type );
void app_bt_profile_conn_wrap(uint8_t profileId,btaddr_t* rtaddr);
result_t app_bt_active_conn_profile( uint8_t profileId, void *arg );

void app_low_power_detect(void );
void enter_match_state(void);
uint8_t app_set_role_accept_con_req(hci_unit_t *unit, const btaddr_t *btaddr);
uint8_t app_set_role_creat_con_req(hci_unit_t *unit, const btaddr_t *btaddr);
void app_low_power_scanning(uint32_t step );
void app_audio_restore(void);
jtask_h app_common_get_task_handle( void );
#if (CONFIG_TEMPERATURE_DETECT == 1)
void app_temprature_scanning(void);
#endif
uint8_t app_get_best_offset(void);
uint8_t app_get_best_offset_level(void);
void app_set_crystal_calibration(uint8_t val);
uint8_t app_bt_a2dp_sbc_default_bitpool(void);
uint8_t app_bt_rmt_device_type_get(uint8_t id);

#ifdef CONFIG_PRODUCT_TEST_INF
result_t hf_cmd_trans_buttoncode(uint32_t value);
#endif
void app_bt_confail_reset_sacn(void);
uint8_t app_get_as_slave_role(void);

#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
uint8_t app_get_power_level(uint8_t index);
#endif

#if (REMOTE_RANGE_PROMPT == 1)
void app_remote_range_judge(void);
void app_remote_range_wave_play(uint16_t reason,uint8_t sel);
void app_remote_set_device_index(uint8_t index);
uint8_t app_remote_get_device_index(void);
#endif

#if A2DP_ROLE_SOURCE_CODE
#define APP_FLAG2_STEREO_STREAMING        	0x00800000 //0x00040000
#define APP_FLAG2_STEREO_AUTOCONN         	0x00080000
//#define APP_FLAG2_AVRCP_OPCODE_PLAY         0x00800000

#define APP_FLAG2_STEREO_AUTOCONN_RETRY   	0x00100000
#define APP_FLAG2_STEREO_BUTTON_PRESS     	0x00200000
#define APP_FLAG2_STEREO_MATCH_BUTTON     0x00400000
//for APP_FLAG2_TWC_HOLD_CALLID				0x00800000

#define APP_FLAG2_SYNC   				 	0x01000000
#define APP_FLAG2_STEREO_INQUIRY_RES    	0x02000000
#define APP_FLAG2_STEREO_ROLE_MASTER    	0x04000000
#define APP_FLAG2_STEREO_WORK_MODE      	0x08000000

#define APP_FLAG2_STEREO_CONNECTTING    	0x10000000
//#define APP_FLAG2_STEREO_SLAVE_ADJ_CLK   	0x20000000
#define APP_FLAG2_STEREO_PLAYING   		0x40000000

#define APP_STEREO_FLAG_SET    (APP_FLAG2_STEREO_WORK_MODE \
            |APP_FLAG2_STEREO_CONNECTTING|APP_FLAG2_STEREO_INQUIRY_RES|APP_FLAG2_STEREO_STREAMING)
//|APP_FLAG2_STEREO_ROLE_MASTER
#define APP_BT_INQUIRY_TICKS			5
#define APP_BT_INQUIRY_DURATION 	6
#endif

#if CONFIG_A2DP_CONN_DISCONN
#define APP_A2DP_STATE_INIT			0
#define APP_A2DP_STATE_DISCONN		1
#define APP_A2DP_STATE_CONN			2
#endif
#if CONFIG_HFP_CONN_DISCONN
#define APP_HFP_STATE_INIT			0
#define APP_HFP_STATE_DISCONN		1
#define APP_HFP_STATE_CONN			2
#endif

#endif
