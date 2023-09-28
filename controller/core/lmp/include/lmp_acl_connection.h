#ifndef __LM_ACL_CONNECTION
#define __LM_ACL_CONNECTION

/******************************************************************************
 * MODULE NAME:    lm_acl_connection.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:        
 *
 * SOURCE CONTROL: $Id: lmp_acl_connection.h,v 1.121 2013/09/24 15:05:38 tomk Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/

#include "sys_config.h"    
#include "lmp_sec_types.h"
#include "lmp_types.h"
//#include "lmp_ecc.h"


enum e_lm_acl_state {
       LMP_ACTIVE                 = 0x0001, 
       LMP_HOLD_MODE              = 0x0002,
       LMP_SNIFF_MODE             = 0x0004, 
       LMP_PARK_MODE              = 0x0008,
       LMP_IDLE                   = 0x0010,
       W4_PAGE_OUTCOME            = 0x0020,
       W4_HOST_ACL_ACCEPT         = 0x0040,
       W4_HOST_SCO_ACCEPT         = 0x0080,
       W4_AUTHENTICATION          = 0x0100,
       W4_ENCRYPTION              = 0x0200,
       LMP_W4_SUPERVISION_TIMEOUT = 0x0400,
       LMP_DETACH_ACK_PENDING     = 0x0800,
       W4_LMP_ACTIVE              = 0x1000,
       W4_FEATURES_RES            = 0x2000,
	   W4_HOST_ESCO_ACCEPT        = 0x4000,
	   W4_SNIFF_EXIT			  = 0x8000
};
/*
 * Note: there is a coupling between the
 * size of the largest enum in the list above
 * and the size of t_lm_acl_state below.
 */
typedef uint16_t t_lm_acl_state;

typedef uint32_t  t_policy_settings;

/*******************************************
 *  Start of the Code
 *  
 *******************************************/

typedef struct t_lmp_link 
{    
   /*************************
    * normal ACL information
    *************************/
    /*
     * Encryption key used in baseband, for optimisation must be full word aligned
     */
    t_link_key encry_key;

    t_lm_acl_state  state;
    t_deviceIndex device_index;
    t_role role;
    t_rate rate;
    t_rate peer_rate;
    t_rate preferred_rate;
    t_bd_addr bd_addr;
    t_classDevice device_class;

    t_packetTypes allowed_pkt_types;
    uint16_t proposed_packet_types;
    uint16_t proposed_poll_interval;

    uint16_t operation_pending;
    uint8_t  remote_features[8];

    uint8_t  context;
    uint8_t  setup_complete_bit_mask;
	uint8_t  host_connection_request_sent;
	uint8_t  host_connection_request_accepted;
    uint8_t  features_complete_bit_mask;
    uint8_t  index; /* Index into the ACL container */
    uint8_t  Nbc;
    t_connectionHandle handle; /* Handle for the ACL Link */

   /***********************************
    * tid_tole_last_rcvd_pkt
    *      ROLE_TID(0):      transaction initiated locally
    *      PEER_ROLE_TID(1): transaction initiated by peer
    * current_proc_tid_role
    *      role of currently initiated procedure, 
    *      determines whether proc. initiated locally or by peer
    ***********************************/

    uint8_t tid_role_last_rcvd_pkt; 
    uint8_t current_proc_tid_role;  
    uint8_t sec_current_proc_tid_role;
     
   /***********************************
    * normal ACL Security Information
    ***********************************/
    t_sec_super_state auth_super_state;
    t_sec_sub_state  auth_sub_state;

    t_sec_super_state encr_super_state;
    t_sec_sub_state  encr_sub_state;
    t_link_key link_key; 
    uint8_t link_key_exists;
    t_link_key_type key_type ;          /* COMBINATION_KEY | LOCAL_UNIT_KEY | REMOTE_UNIT_KEY */
    t_link_key_life link_key_persistance;     /* Semi-Permanent or Temporary */
    uint8_t pin_code_length;
    t_pin_code pin_code;
    uint8_t encrypt_key_size;
    uint8_t rand[16];
    uint8_t local_combkey_part[16];
    t_sres sres;
    t_aco aco;
    t_encrypt_mode encrypt_mode;
    t_encrypt_mode restore_current_encrypt_mode; /* 2825 */
    t_encrypt_enable encrypt_enable;
#if (PRH_BS_CFG_SYS_LMP_PAUSE_ENCRYPTION_SUPPORTED==1)
    uint8_t encrypt_paused;
    uint8_t event_status;
    uint8_t encryption_key_refresh_index;
#endif
    t_lm_policy_info pol_info;
    uint8_t switch_state;
   /********************************
    * Indicator of the current power
    * level in the peer 
    ********************************/

    t_power_level  peer_power_status;
    uint8_t max_power_level;
    volatile BOOL peer_power_req_tx_pending;
    int8_t peer_power_counter;
    
   /******************************
    *  QoS Value for an LMP link. 
    ******************************/
    uint8_t service_type;
#if (PRH_BS_CFG_SYS_QOS_SUPPORTED==1)
    uint32_t token_rate;
    uint32_t peak_bandwidth;
    uint32_t latency;
    uint32_t delay_variation;

    uint8_t  proposed_service_type;
    uint32_t proposed_token_rate;
    uint32_t proposed_peak_bandwidth;
    uint32_t proposed_latency;
    uint32_t proposed_delay_variation;
#endif

#if (PRH_BS_CFG_SYS_FLOW_SPECIFICATION_SUPPORTED==1)
    uint8_t direction;

    uint8_t in_service_type;
    uint32_t in_token_rate;
    uint32_t in_peak_bandwidth;
    uint32_t in_latency;
    uint32_t in_token_bucket_size;

    uint8_t  in_proposed_service_type;
    uint32_t in_proposed_token_rate;
    uint32_t in_proposed_peak_bandwidth;
    uint32_t in_proposed_latency;
    uint32_t in_proposed_token_bucket_size;

    uint8_t  out_service_type;
    uint32_t out_token_rate;
    uint32_t out_peak_bandwidth;
    uint32_t out_latency;
    uint32_t out_token_bucket_size;

    uint8_t  out_proposed_service_type;
    uint32_t out_proposed_token_rate;
    uint32_t out_proposed_peak_bandwidth;
    uint32_t out_proposed_latency;
    uint32_t out_proposed_token_bucket_size;

    t_slots transmission_interval;
    int16_t transmission_position;

    uint8_t flow_spec_pending;
    uint16_t proposed_tx_interval;

    uint8_t out_qos_active;
    uint8_t in_qos_active;
#endif

   /*****************
    *  Link timeouts 
    *****************/
   uint16_t channel_quality_timeout;
   uint16_t flush_timeout;
   uint16_t supervision_timeout; /* negotiated by LM */
   uint16_t link_supervision_timeout; /* configured by HCI */
   uint8_t flush_timeout_index;
   uint8_t flush_execut_timeout_index;
   uint8_t supervision_timeout_index;
   uint8_t recent_contact_timeout_index;
   uint8_t channel_quality_timeout_index;
#if (LMP_ESCO_LINK_REQ_TIMEOUT == 1)
   uint8_t lmp_esco_req_timeout_index;
#endif

#ifdef LMP_LINK_L2CAL_TIMEOUT_ENABLE
   uint8_t l2cap_timeout_index;
#endif
#if (PRH_BS_CFG_SYS_LMP_MSS_SUPPORTED==1)
   uint8_t  allow_switch;
   uint8_t change_link_for_role_switch_timer_index;
#endif
   uint8_t flush_status;
   uint8_t flush_type;
   uint8_t queue_flush_pending;
   uint8_t sched_queue_flush_pending;
   uint8_t failed_contact_counter;
   uint8_t flush_packet_type;  // Indicates AUTO_FLUSHABLE / NON_AUTO_FLUSHABLEz
   uint8_t enhanced_queue_flush_pending;
   /***************************
    *  Link Policy Information 
    ***************************/

    t_HCIpacketTypes  packet_types;  /* Master Only */
    t_slots poll_interval;
    int16_t poll_position;

    uint16_t l2cap_pkt_len_pending;
    uint16_t current_l2cap_pkt_length;
    uint8_t  previous_modes; /* Stores the previous modes [ Hold/Sniff/Park ] a link has entered */
    uint8_t packet_type_changed_event;
    uint8_t max_slots_out;
    uint8_t max_slots_out_pending;
    uint8_t max_slots_in;
    uint8_t prev_slots_tx;
    uint8_t l2cap_ack_pending;

    /*************************
     * Timing Related Aspects 
     *************************/
    uint16_t slot_offset;

    uint8_t gen_detach_timeout_index;
    uint8_t gen_switch_timeout_index;
    uint8_t gen_switch_piconet_timeout_index;
    uint8_t gen_security_timeout_index;

    uint16_t link_policy_mode;
    uint8_t msg_timer;
#if (PRH_BS_CFG_SYS_LMP_PARK_SUPPORTED==1)
    t_deviceIndex park_device_index;
    uint8_t num_park_negotiations;
    uint8_t automatic_park_enabled;

#if (PRH_BS_DEV_SLAVE_FORCE_STOP_AUTOPARK_SUPPORTED==1)
    uint8_t force_stop_autopark;
#endif
#endif
    t_packet default_pkt_type;
    t_error detach_reason;
    t_error disconnect_req_reason;
    uint8_t l2cap_tx_enable;
    uint8_t link_tx_quality;
    uint8_t link_quality;
    uint8_t sec_timer;
    uint16_t bcast_enc_key_size_mask;
    uint8_t ptt_req;
	uint8_t connection_request_send_via_hci;

#if (PRH_BS_CFG_SYS_LMP_SECURE_SIMPLE_PAIRING_SUPPORTED==1)
	uint8_t peer_public_key_x[24];
	uint8_t peer_public_key_y[24];
	uint8_t Incoming_Encapsulated_P192_len_pending;
	uint8_t ssp_initiator;
	uint8_t peer_commitment[16];
	uint8_t peer_random[16];
	uint8_t DHkey[24];
	uint8_t DHkeyCheck[16];
	uint8_t peer_key_check[16];

	uint8_t DHkeyCheckComplete;
	uint8_t DHkeyCalculationComplete;
#if (PRH_BS_CFG_SYS_SSP_OOB_SUPPORTED==1)
	uint8_t oob_data_present;
#endif
	uint8_t auth_requirements;
	uint8_t io_cap;
	uint8_t auth_type;
	uint8_t ssp_tid;
	uint8_t peer_user_authenticated;
	uint8_t local_user_authenticated;
#if (PRH_BS_CFG_SYS_SSP_PASSKEY_SUPPORTED==1)
	uint32_t passKey;
	uint8_t passKeyCount;
#endif
	uint8_t Rpeer[16];
#if (PRH_BS_CFG_SYS_SSP_OOB_SUPPORTED==1)
	uint8_t local_oob_verified;
#endif
	uint8_t rxed_simple_pairing_number;
	uint8_t ssp_host_support;
	uint8_t ssp_debug_mode;
	uint8_t ssp_hci_timer_index;
	uint8_t peer_user_dh_key_check_rxed;
#endif
	BOOL  call_lc_wakeup;
    uint8_t LSLCacc_last_rx_LMP_msg;
	
	uint8_t remote_features_pg1[1];
	uint8_t remote_features_pg2[2];
	

} t_lmp_link;

#define PTT_REQ_IDLE 0xFF
#define PTT_REQ_PENDING 0x70

void LMacl_Initialise(t_lmp_link* p_link);
void LMacl_Write_CombKey_Part(t_lmp_link *p_link, uint8_t *local_comb_key_part);
void LMacl_Write_Bd_Addr(t_lmp_link* current_link, t_bd_addr* p_bd_addr);

void LMacl_Add_Sco(t_lmp_link* p_link,t_sco_info* p_sco_link);
void LMacl_Remove_Sco(t_lmp_link* p_link,t_sco_info* p_sco_link);
uint8_t LMacl_Sco_Exists(t_lmp_link* p_link);

void LMacl_Set_Key_Persistance(t_lmp_link* p_link, t_link_key_life key_persistance);
t_link_key_life LMacl_Get_Current_Key_Persistance(t_lmp_link* p_link);
t_link_key_life LMacl_Get_Previous_Key_Persistance(t_lmp_link* p_link);

#endif

