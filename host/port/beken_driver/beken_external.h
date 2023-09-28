#ifndef _BEKEN_EXTERNAL_H_
#define _BEKEN_EXTERNAL_H_

#include "stdio.h"
#include <string.h>
#include <jos.h>
#include <bluetooth.h>
#include <bluetooth/core/hci_internal.h>
#include <bluetooth/bt_hci_types.h>
#include <bt_a2dp_sink.h>

void juart_receive(uint8_t *buf, uint16_t size);

/* AV/C pass through control commands */
#define AVC_OP_VOLUME_UP                0x41
#define AVC_OP_VOLUME_DOWN              0x42
#define AVC_OP_MUTE                     0x43
#define AVC_OP_PLAY                     0x44
#define AVC_OP_STOP                     0x45
#define AVC_OP_PAUSE                    0x46
#define AVC_OP_RECORD                   0x47
#define AVC_OP_REWIND                   0x48
#define AVC_OP_FAST_FORWARD             0x49
#define AVC_OP_EJECT                    0x4A
#define AVC_OP_NEXT                     0x4B
#define AVC_OP_PREV                     0x4C

enum {
    RFCOMM_CHANNEL_HSP = 2,
    RFCOMM_CHANNEL_HFP,
    RFCOMM_CHANNEL_OPUSH,
    RFCOMM_CHANNEL_PBAP,
    RFCOMM_CHANNEL_WIAP,
    RFCOMM_CHANNEL_SPP,
    RFCOMM_CHANNEL_DUN
};
#if CONFIG_BLUETOOTH_HID

//HID Funtion
result_t send_Hid_key_press(void);
result_t send_Hid_key_releas(void);
void photo_Key_Atvice(void);

//void hid_con_connected(void);
result_t hid_cmd_disconnect(void);
result_t hid_int_conn_disconnect(void);
#endif


result_t bt_unit_acl_connect(hci_unit_t *unit, const btaddr_t *raddr);
void bt_unit_acl_disconnect(hci_unit_t *unit, const btaddr_t *raddr);
result_t hci_send_cmd(hci_unit_t *unit, uint16_t opcode, void *buf,
    uint8_t len);
void bt_sec_set_io_caps(uint8_t io_caps);
result_t a2dp_cmd_connect(char *params, unsigned int len);
result_t a2dp_cmd_disconnect(void);
#if CONFIG_A2DP_CONN_DISCONN
result_t a2dp_set_disconnect(void);
result_t a2dp_set_connect(void);
#endif
result_t avrcp_cmd_connect(char *params, unsigned int len);
result_t avrcp_cmd_disconnect(void);
//result_t avrcp_cmd_register_notification(bt_avrcp_ctrl_session_h session,uint8_t event_id, uint32_t playback_pos_changed_interval );
void a2dp_volume_init( int8_t aud_volume);
int8_t a2dp_get_volume( void );
result_t a2dp_volume_adjust( uint8_t oper );
uint32_t a2dp_get_codec_type(void);

/*********HFP API*******************/
result_t hf_cmd_disconnect(void);
result_t hf_cmd_accept_call(void);
result_t hf_cmd_hangup_call(int8_t reason);
result_t hf_cmd_place_call(char number[32]);
result_t hf_cmd_redial(void);
result_t hf_cmd_resp_and_hold(uint32_t command);
result_t hf_cmd_resp_and_hold_read(void);
result_t hf_cmd_set_call_wait_notif(uint32_t enable);
result_t hf_cmd_chld(uint32_t command, uint32_t call_id);
result_t hf_cmd_list_current_calls(void);
result_t hf_cmd_set_cli_notif(uint32_t enable);
result_t hf_cmd_get_operator(void);
result_t hf_cmd_get_subscriber(void);
result_t hf_cmd_set_voice_recog(uint32_t enable);
result_t hf_cmd_trans_DTMF(char * code);
result_t hf_cmd_set_iphone_batlevel( int8_t level );
int hf_sco_handle_process( int oper );
result_t hf_cmd_set_vgs( uint8_t oper );
void hf_audio_restore(void);
#if CONFIG_HFP_CONN_DISCONN
result_t hf_set_disconnect(void);
result_t hf_set_connect(void);
#endif
result_t hs_cmd_connect(char *params, unsigned int len);
result_t hs_cmd_disconnect(void);
result_t hs_cmd_button_pressed(void);

result_t sdp_connect( btaddr_t *laddr, btaddr_t *raddr );
result_t sdp_service_connect(btaddr_t *laddr, btaddr_t *raddr, uint16_t tid);
result_t spp_slc_disconnect(void);


#endif
