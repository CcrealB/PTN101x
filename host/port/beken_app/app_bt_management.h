/*
 * bt_app_management.h
 *
 *  Created on: 2017-8-18
 *      Author: Durian
 */

#ifndef BT_APP_MANAGEMENT_H_
#define BT_APP_MANAGEMENT_H_
enum MNG_ERR
{
    MNG_NO_ERROR         = 0,
    MNG_ERROR_NO_ENTITY  = -1,
    MNG_ERROR_2          = -2
};
typedef enum
{	
    SYS_BT_UNUSED_STATE = 0,
    SYS_BT_SCANNING_STATE = 1,
    SYS_BT_WORKING_STATE = 2,
    SYS_BT_CONNECTING_STATE = 3,
    SYS_BT_SNIFF_MODE_STATE = 4,
    SYS_BT_DUT_MODE_STATE = 5,
    SYS_BT_STANDBY_STATE = 6,
    SYS_BT_DEEPSLEEP_STATE = 7,
    SYS_BT_END_STATE
}t_sys_bt_state;

typedef enum
{
    SYS_BT_INVALID_EVENT = 0,
    SYS_BT_ENABLE_COMPLETE_EVENT  = 1,
    SYS_BT_DISABLE_COMPLETE_EVENT = 2,
    SYS_BT_NO_SCAN_EVENT = 3,
    SYS_BT_BOTH_SCANNING_EVENT = 4,
    SYS_BT_DISCONNECTED_EVENT = 5,
    SYS_BT_DISCONN_CONN_EVENT = 6,
    SYS_BT_CONNECTED_EVENT = 7,
    SYS_BT_UNSNIFF_EVENT = 8,
    SYS_BT_DUT_MODE_EVENT
}t_sys_bt_event;
typedef enum
{
    ROLE_INVALID = 0,
    BT_ACL_SLAVE   = 1,
    BT_ACL_MASTER  = 2,
    TWS_ROLE_SLAVE = 4,
    TWS_ROLE_MASTER = 8
}t_bt_role;
typedef struct system_bt_mng_s
{
    uint16_t        entity_id;
    uint16_t        link_handle;
    uint32_t        bt_connecting_flag;
    uint32_t        bt_working_flag;
    uint32_t        bt_profile_attrs;
    uint32_t        timer;
    btaddr_t        bt_remote_addr;
    t_bt_role       bt_role;
    uint8_t         is_used;
    uint8_t         detach_reason;
    uint8_t         conn_failure_times;
    uint8_t         role_switch_times;
	uint8_t			mac_win_book;
    uint8_t         xxxxxxxx;         
    uint8_t         sys_flag;
    uint8_t         bt_connected_wrap;
    t_sys_bt_state  sys_bt_state;
    t_sys_bt_event  sys_bt_event;
}t_bt_app_entity,*p_bt_app_entity;

#define SYS_CONFIG_MODE_1V1             0x00000001
#define SYS_CONFIG_MODE_1V2             0x00000002
#define SYS_CONFIG_MODE_TWS             0x00000004
#define SYS_CONFIG_MODE_TWS_DM          0x00000008
#define SYS_CONFIG_MODE_1V1_DM          0x00000010
#define SYS_CONFIG_MODE_1V2_DM          0x00000020


#ifdef CONFIG_BLUETOOTH_HFP
#define PROFILE_BT_HFP                0x00000001
#else
#define PROFILE_BT_HFP                0
#endif
#ifdef CONFIG_BLUETOOTH_AGHFP
#define PROFILE_BT_AGHFP              0x00000002
#else
#define PROFILE_BT_AGHFP              0
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
#define PROFILE_BT_A2DP_SRC           0x00000004
#else
#define PROFILE_BT_A2DP_SRC           0
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
#define PROFILE_BT_A2DP_SNK           0x00000008
#else
#define PROFILE_BT_A2DP_SNK           0
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP
#define PROFILE_BT_AVRCP              0x00000010
#else
#define PROFILE_BT_AVRCP              0
#endif
#ifdef CONFIG_BLUETOOTH_HID
#define PROFILE_BT_HID                0x00000020
#else
#define PROFILE_BT_HID                0
#endif
#ifdef CONFIG_BLUETOOTH_HSP
#define PROFILE_BT_HSP                0x00000040
#else
#define PROFILE_BT_HSP                0
#endif
#ifdef CONFIG_BLUETOOTH_SPP
#define PROFILE_BT_SPP                0x00000080
#else
#define PROFILE_BT_SPP                0
#endif
#if CONFIG_BLUETOOTH_PBAP
#define PROFILE_BT_PBAP               0x00000100
#else
#define PROFILE_BT_PBAP               0
#endif
#ifdef CONFIG_BLUETOOTH_OPUSH
#define PROFILE_OPUSH                 0x00000200
#else
#define PROFILE_BT_OPUSH              0
#endif
#ifdef CONFIG_BLUETOOTH_SDAP 
#define PROFILE_SDAP                  0x00000400
#else
#define PROFILE_BT_SDAP               0
#endif
#ifdef CONFIG_BLUETOOTH_PAN
#define PROFILE_PAN                   0x00000800
#else
#define PROFILE_BT_PAN                0
#endif
#ifdef CONFIG_BLUETOOTH_MAP
#define PROFILE_BT_MAP                0x00001000
#else
#define PROFILE_BT_MAP                0
#endif
#ifdef CONFIG_BLUETOOTH_DUN_DT
#define PROFILE_BT_DUN_DT             0x00002000
#else
#define PROFILE_BT_DUN_DT             0
#endif
#ifdef CONFIG_BLUETOOTH_W2AP
#define PROFILE_BT_W2AP               0x00004000
#else
#define PROFILE_BT_W2AP               0
#endif

#define BT_CONNECT_STATE_CONN_PENDING           0x10000000
#define BT_CONNECT_STATE_SCAN_PENDING           0x20000000
#define BT_CONNECT_STATE_DUT_PENDING            0x40000000
#define BT_CONNECT_STATE_DISCONN_PENDING        0x80000000
#define BT_CONNECT_STATE_RECONNECTING           0x00000100
#define BT_CONNECT_STATE_RECONN_FAILED          0x00001000

#define BT_ALL_PROFILE_CONNECTED_SET            (PROFILE_BT_HFP|PROFILE_BT_A2DP_SNK|PROFILE_BT_AVRCP)

 
#define BT_WORK_SCO_CONNECTED           0x00000001
#define BT_WORK_MUSIC_PLAYING           0x00000002
#define BT_WORK_AT_CMD_PROCESS          0x00000004    

        
#define BT_WORKING_FLAG_SET     ( BT_WORK_SCO_CONNECTED     \
                                 |BT_WORK_MUSIC_PLAYING     \
                                 |BT_WORK_AT_CMD_PROCESS    \
                                )

#define SYS_HOST_FEATURE_AEC          0x00000001
#define SYS_HOST_FEATURE_FLOW_CTRL    0x00000002
#define SYS_HOST_FEATURE_SNIFF        0x00000004
#define SYS_HOST_FEATURE_MSBC         0x00000008

#define SYS_SLEEP_INTERVAL              (100*5)
#define SYS_WORK_WAVE_PLAY            0x00000001


extern t_bt_app_entity g_bt_app_entity_container[];

static inline uint8_t bt_app_entity_get_used(uint8_t id)
{
    return g_bt_app_entity_container[id].is_used;
}
static inline t_sys_bt_state bt_app_entity_get_state(uint8_t id)
{
    return g_bt_app_entity_container[id].sys_bt_state;
}
static inline void bt_app_entity_set_state(uint8_t id,t_sys_bt_state state)
{
    static uint8_t state_buf[2];
    if(state_buf[id]!=state)
    {    
        LOG_I(MNG,"state_buf[%d]=%x,state=%x\r\n",id,state_buf[id],state);
        state_buf[id]=state;
    }
    g_bt_app_entity_container[id].sys_bt_state = state;
}
static inline t_sys_bt_event bt_app_entity_get_event(uint8_t id)
{
    return g_bt_app_entity_container[id].sys_bt_event;
}
static inline void bt_app_entity_set_event(uint8_t id,t_sys_bt_event event)
{
    static uint8_t event_buf[2];
    if(event_buf[id]!=event)
    {    
        LOG_I(MNG,"event_buf[%d]=%x,event=%x\r\n",id,event_buf[id],event);
        event_buf[id]=event;
    }
    g_bt_app_entity_container[id].sys_bt_event = event ;
}
static inline t_bt_role bt_app_entity_get_role(uint8_t id)
{
    return g_bt_app_entity_container[id].bt_role;
}
static inline void bt_app_entity_set_role(uint8_t id,t_bt_role role)
{
	g_bt_app_entity_container[id].bt_role = role;
}

static inline void bt_app_entity_set_mac_win_book(uint8_t id, uint8_t mac_win)
{
	g_bt_app_entity_container[id].mac_win_book = mac_win;
}
static inline uint8_t bt_app_entity_get_mac_win_book(uint8_t id)
{
	return g_bt_app_entity_container[id].mac_win_book;
}
static inline uint32_t bt_app_entity_get_connect_flag(uint8_t id,uint32_t flag)
{
    return  g_bt_app_entity_container[id].bt_connecting_flag & flag;
}
static inline uint32_t bt_app_entity_get_working_flag(uint8_t id,uint32_t flag)
{
    return  g_bt_app_entity_container[id].bt_working_flag & flag;
}
static inline void bt_app_entity_set_connect_flag(uint8_t id,uint32_t flag)
{
    g_bt_app_entity_container[id].bt_connecting_flag |= flag;
}
static inline void bt_app_entity_set_working_flag(uint8_t id,uint32_t flag)
{
    g_bt_app_entity_container[id].bt_working_flag |= flag;
}
static inline void bt_app_entity_clear_connect_flag(uint8_t id,uint32_t flag)
{
    g_bt_app_entity_container[id].bt_connecting_flag &= (~flag);
}
static inline void bt_app_entity_clear_working_flag(uint8_t id,uint32_t flag)
{
    g_bt_app_entity_container[id].bt_working_flag &= (~flag);
}
static inline btaddr_t *bt_app_entit_get_rmt_addr(uint8_t id)
{
    return &g_bt_app_entity_container[id].bt_remote_addr;
}
static inline uint16_t bt_app_entit_get_link_handle(uint8_t id)
{
    return g_bt_app_entity_container[id].link_handle;
}
static inline void bt_app_entity_set_sys_flag(uint8_t id,uint32_t flag)
{
    g_bt_app_entity_container[id].sys_flag |= flag;
}
static inline void bt_app_entity_clear_sys_flag(uint8_t id,uint32_t flag)
{
    g_bt_app_entity_container[id].sys_flag &= (~flag);
}
static inline uint32_t bt_app_entity_get_sys_flag(uint8_t id,uint32_t flag)
{
    return  g_bt_app_entity_container[id].sys_flag & flag;
}
static inline uint32_t bt_app_entity_get_bt_connected_wrap(uint8_t id)
{
    return g_bt_app_entity_container[id].bt_connected_wrap;
}
static inline void bt_app_entity_set_bt_connected_wrap(uint8_t id)
{
    g_bt_app_entity_container[id].bt_connected_wrap = 1;
}
static inline void bt_app_entity_clear_bt_connected_wrap(uint8_t id)
{    
    g_bt_app_entity_container[id].bt_connected_wrap = 0;
}

void bt_app_entity_print_state(void);

uint8_t bt_app_entity_alloc(void);
void bt_app_entity_attach(uint8_t id,uint16_t handle,btaddr_t *rmt_addr,uint8_t detach_reason);
void bt_app_entity_free(uint8_t id,uint8_t detach_reason);
void bt_app_entity_bind(uint16_t handle,btaddr_t *rmt_addr);
void bt_app_entity_init(void);
void bt_app_entity_free_by_addr(btaddr_t *rmt_addr,uint8_t detach_reason);

void bt_app_entity_set_event_by_addr(btaddr_t *rmt_addr,t_sys_bt_event event);
void bt_app_entity_set_event_by_handle(uint16_t handle,t_sys_bt_event event);
void bt_app_entity_set_role_by_addr(btaddr_t *rmt_addr,t_bt_role role);
void bt_app_entity_set_role_by_handle(uint16_t handle,t_bt_role role);

void bt_app_entity_set_conn_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag);
void bt_app_entity_set_work_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag);
void bt_app_entity_clear_conn_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag);
void bt_app_entity_clear_work_flag_by_addr(btaddr_t *rmt_addr,uint32_t flag);
uint32_t bt_app_check_all_entity_connect_flag(uint32_t flag);
uint32_t bt_app_check_entity_connect_flag_by_id(int8_t idx,uint32_t flag);
uint32_t bt_app_check_all_entity_work_flag(uint32_t flag);
void bt_app_clear_all_entity_connect_flag(uint32_t flag);
void bt_app_clear_all_entity_work_flag(uint32_t flag);

uint16_t bt_app_entity_find_handle_from_raddr(btaddr_t *rmt_addr);
int8_t bt_app_entity_find_id_from_raddr(btaddr_t *rmt_addr);
int8_t bt_app_entity_find_id_from_handle(uint16_t handle);
void bt_app_disconnect_acl_link_by_raddr(btaddr_t *rmt_addr);
void bt_app_disconnect_connect_acl(btaddr_t *rmt_addr);
uint8_t bt_app_entity_get_role_switch_times_from_raddr(btaddr_t *raddr);
void bt_app_entity_set_role_switch_times_from_raddr(btaddr_t *raddr,uint8_t times);

void bt_app_get_conn_addr_from_env(void);
BOOL bt_app_device_scan_is_disallowed(void);
void bt_app_set_device_scanning(uint8_t scan_enable);
void bt_app_unused_procedure(uint8_t id);
void bt_app_scanning_procedure(uint8_t id);
void bt_app_standby_procedure(uint8_t id);
void bt_app_working_procedure(uint8_t id);
void bt_app_connecting_procedure(uint8_t id);
void bt_app_sniff_mode_procedure(uint8_t id);
void bt_app_dut_mode_procedure(uint8_t id);
void bt_app_management_sched(void);

#endif /* BT_APP_MANAGEMENT_H_ */
