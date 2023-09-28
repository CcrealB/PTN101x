#ifndef _APP_BT_CONN_H_
#define _APP_BT_CONN_H_

enum conn_inner_state
{
    CONN_START = 0,
    CONN_ACL_FAIL = 1,
    CONN_ACL_CONNECTED = 2,    
    CONN_AUTH_FAIL,
    CONN_AUTH_SUCCESS,
    CONN_ENCYPT_FAIL,
    CONN_ENCYPTED,
    CONN_KEY_MISSING,
    CONN_SDP_FAIL,
    CONN_SDP_FINISHED,
    CONN_HFP_FAIL = 0x11,
    CONN_HFP_CONNECTED,
    CONN_SNK_A2DP_FAIL,
    CONN_SNK_A2DP_CONNECTED ,
    CONN_AVRCP_FAIL,
    CONN_AVRCP_CONNECTED,
    CONN_AGHFP_FAIL,
    CONN_AGHFP_CONNECTED,
    CONN_SRC_A2DP_FAIL,
    CONN_SRC_A2DP_CONNECTED,
    CONN_HID_FAIL,
    CONN_HID_CONNECTED ,
    CONN_LOW_RESOURCES,    
    CONN_WAIT_ACL_CONNECTED = 0x30,
    CONN_WAIT_AUTH,
    CONN_WAIT_ENCYPTED,
    CONN_WAIT_SDP_FINISHED,
    CONN_WAIT_HFP_CONNECTED,
    CONN_WAIT_SNK_A2DP_CONNECTED,
    CONN_WAIT_AVRCP_CONNECTED,
    CONN_WAIT_HID_CONNECTED,
    CONN_WAIT_DISCONNECTED,
    CONN_END = 0x50
};

typedef uint16_t t_conn_inner_state;

enum conn_event
{
    CONN_EVENT_NULL = 0,
    CONN_EVENT_ACL_FAIL = 1,
    CONN_EVENT_ACL_CONNECTED = 2,
    CONN_EVENT_AUTH_FAIL,
    CONN_EVENT_AUTH_SUCCESS,
    CONN_EVENT_ENCYPT_FAIL,
    CONN_EVENT_ENCYPTED,
    CONN_EVENT_SDP_FAIL,
    CONN_EVENT_SDP_FINISHED,
    CONN_EVENT_HFP_DISCONNECTED=0x11,
    CONN_EVENT_HFP_CONNECTED,
    CONN_EVENT_SNK_A2DP_DISCONNECTED,
    CONN_EVENT_SNK_A2DP_CONNECTED ,
    CONN_EVENT_AVRCP_DISCONNECTED,
    CONN_EVENT_AVRCP_CONNECTED,
    CONN_EVENT_AGHFP_DISCONNECTED,
    CONN_EVENT_AGHFP_CONNECTED,
    CONN_EVENT_SRC_A2DP_DISCONNECTED,
    CONN_EVENT_SRC_A2DP_CONNECTED,
    CONN_EVENT_HID_DISCONNECTED, 
    CONN_EVENT_HID_CONNECTED            
};

typedef uint16_t t_conn_event;

enum conn_result
{
    CONN_CONNECTING,
    CONN_SUCCESS,
    CONN_FAIL,
    CONN_FALSE
};
typedef uint8_t t_conn_result;
/*    
#define PROFILE_HFP                0x00000001
#define PROFILE_AGHFP              0x00000002
#define PROFILE_A2DP_SRC           0x00000004
#define PROFILE_A2DP_SNK           0x00000008
#define PROFILE_AVRCP              0x00000010
#define PROFILE_HID                0x00000020
#define PROFILE_HSP                0x00000040
#define PROFILE_SPP                0x00000080
#define PROFILE_PBAP               0x00000100
#define PROFILE_OPUSH              0x00000200
#define PROFILE_SDAP               0x00000400
#define PROFILE_PAN                0x00000800
#define PROFILE_MAP                0x00001000
#define PROFILE_DUN_DT             0x00002000
#define PROFILE_W2AP               0x00004000
*/


#define MAX_INNER_CONN_COUNT     3
#define MAX_INNER_CONN_TIME      530  //unit:10ms
typedef struct auth_status//bugfix : delete link key,page back unlimit,do not open scan
{
	btaddr_t r_addr;
	uint8_t auth_status;
}auth_status;
typedef struct app_conn_state 
{
    hci_link_t * link;
    btaddr_t* addr;
    uint32_t profile;
    uint32_t event;
    t_conn_result result;
    BOOL  conn_active;
    uint8_t conn_cancel_times;
    uint8_t conn_timeout_times;
    int8_t conn_cancel_success;
    t_conn_inner_state state;
} app_conn_state;

void bt_connection_init();
void bt_connection_uninit();
void bt_set_conn_cancel_success(int8_t);
void bt_connection_req(btaddr_t* addr);
BOOL bt_connection_active(void);
t_conn_result get_connection_state();
void set_connection_event(btaddr_t* addr, t_conn_event event);
void set_connection_state(btaddr_t* addr, uint32_t state);
BOOL TickTimer_Is_Expired(uint64_t ptimer);
result_t bt_hf_connect(btaddr_t* addr);
result_t bt_a2dpSnk_connect(btaddr_t* addr);
void bt_connect_scheduler();
void set_avrcp_conn_state(void);
void set_hfp_conn_state(void);
void set_a2dp_conn_state(void);
#endif
