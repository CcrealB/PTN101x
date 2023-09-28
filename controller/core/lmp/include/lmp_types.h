#ifndef _PARTHUS_LMP_TYPES_
#define _PARTHUS_LMP_TYPES_

/******************************************************************************
 * MODULE NAME:    lmp_types.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:   
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:        
 *
 * SOURCE CONTROL: $Id: lmp_types.h,v 1.90 2014/03/11 03:14:05 garyf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/

#include "sys_config.h"
#include "sys_types.h"
#include "lmp_sec_types.h"
#include "lmp_const.h"
#if (PRH_BS_CFG_SYS_LMP_EXTENDED_SCO_SUPPORTED==1)
#include "tra_queue.h"
#endif

typedef enum { OFF = 0x00, ON = 0x01 } t_flag;

typedef uint8_t t_lmp_pdu_q[MAX_NUM_LMP_PDUS][MAX_LMP_PDU_SIZE];

typedef struct {
    /**************************
     * Link Hold Informaton
     **************************/
#if (PRH_BS_CFG_SYS_LMP_HOLD_SUPPORTED==1)
    uint32_t hold_instant;            /* Instant at which the hold should occur */
    uint16_t max_hold_time;           /* Max hold time - Upper bound received via HCI  */
    uint16_t min_hold_time;           /* Min hold time - Lower bound received via HCI  */
    uint16_t max_previous_hold;       /* Max time the link was previously put on hold for
                                      * - used to determine if forced hold possible 
                                      */

    uint16_t hold_time;               /* The length of time the device is to go on hold for */
    uint8_t num_hold_negotiations;    /* The max number of rounds of negotiations which take place
                                      * during a negotiated hold
                                      */
    uint8_t hold_timeout_index;       /* An identifier for the hold timer */
#else
    uint8_t dummy;
#endif
 } t_lm_hold_info;

typedef struct {
#if (PRH_BS_CFG_SYS_LMP_SNIFF_SUPPORTED==1)
    t_timer end_current_sniff_window;
    t_timer next_sniff_window;
    uint16_t D_sniff;
    uint16_t T_sniff;
    uint16_t T_sniff_min;
    uint16_t T_sniff_max;
    uint16_t N_sniff;
    uint16_t N_sniff_rem;
    uint16_t sniff_timeout;
#if (PRH_BS_CFG_SYS_LMP_SNIFF_SUBRATING_SUPPORTED==1)
    uint8_t sniff_subrating_req_pending;
    uint16_t max_latency;
    uint16_t min_local_timeout;
    uint16_t min_remote_timeout;
    uint32_t sniff_subrating_instant;
    uint8_t local_sniff_subrate;
    uint8_t max_remote_sniff_subrate;
    uint16_t new_local_sniff_timeout;
    uint16_t local_sniff_timeout;
    uint16_t min_remote_sniff_timeout;
    uint32_t next_subrate_window;
    uint8_t subrate_state;
    uint8_t subrate_timeout_index;
#endif
    uint8_t sniff_state;
    uint8_t num_sniff_negotiations;
    uint8_t timing_ctrl;
    //uint8_t sniff_rx_status;
#else
    uint8_t dummy;
#endif
 } t_lm_sniff_info; 

typedef struct {
#if (PRH_BS_CFG_SYS_LMP_PARK_SUPPORTED==1)
    uint8_t  delta_bcast;      /* Global only                        */
    uint8_t  pm_addr;          /* PM Address of the slave - link specific */
    uint8_t  ar_addr;          /* AR Address of the slave - link specific */
    uint8_t  am_addr;          /* Link specific            */
    uint8_t  park_state;       
    t_deviceIndex  device_index;
    t_deviceIndex  unpark_device_index;
    t_deviceIndex  park_device_index;
    uint8_t  unpark_pending; /* Flag to indicate if there is an unpark pending - link specific           */
    uint8_t timing_ctrl;    /* the timing control to be used in the park   - can be global */
    uint8_t unpark_pdu_rx;  /* Indicates if an LMP_Unpark PDU has been received - link specific */
    uint8_t num_unparks_attempts; /* Number of attempts the master will try to unpark */
    t_timer next_unpark_timer;
    t_timer next_park_timer;
    uint8_t mode_change;
    uint8_t unpark_type;
    uint8_t park_timeout_index;
    uint8_t unpark_timeout_index;
#else
    uint8_t dummy;
#endif
} t_lm_park_info;

typedef enum {
        SWITCH_IDLE = 0,
        SWITCH_PEER_ACTIVATION_PENDING = 1,
        SWITCH_LOCAL_ACTIVATION_PENDING = 2,
        SWITCH_LOCAL_COMPLETION_PENDING = 3,
        SWITCH_COMPLETE_SUCCESS = 4,
        SWITCH_COMPLETE_FAIL = 5
} t_lm_switch_states;
    
typedef struct {
   t_lm_sniff_info sniff_info;
   t_lm_hold_info hold_info;
   t_lm_park_info park_info;
  } t_lm_policy_info; 


typedef enum {
       SCO_UNUSED = 0,
	   SCO_MASTER_DEACTIVATION_PENDING = 1,
       SCO_IDLE = 2,
       SCO_ACTIVATION_PENDING = 3,
       SCO_ACTIVE = 4,
       SCO_CHANGE_PKT_ACCEPT_ACK_PENDING = 5
#if (PRH_BS_CFG_SYS_LMP_EXTENDED_SCO_SUPPORTED==1)
       ,
       eSCO_MASTER_ACTIVATION_PENDING = 6,
       eSCO_MASTER_INACTIVE = 7,    
       eSCO_MASTER_RESERVED_SLOT_TX = 8,
       eSCO_MASTER_RESERVED_SLOT_RX = 9,
       eSCO_MASTER_RETRANSMISSION_WINDOW_TX = 10,
       eSCO_MASTER_RETRANSMISSION_WINDOW_RX = 11,
       eSCO_SLAVE_ACTIVATION_PENDING = 12,
       eSCO_SLAVE_INACTIVE = 13,    
       eSCO_SLAVE_RESERVED_SLOT_TX = 14,
       eSCO_SLAVE_RESERVED_SLOT_RX = 15,
       eSCO_SLAVE_RETRANSMISSION_WINDOW_TX = 16,
       eSCO_SLAVE_RETRANSMISSION_WINDOW_RX = 17,
#ifdef BT_DUALMODE
       eSCO_SLAVE_FROM_BLE = 18,
#endif
#endif
} t_lm_sco_states;

typedef struct {
    uint32_t  next_activity_time;          /* For SCO scheduling             */   
    t_packetTypes packet_types;        /* Bit vector of allowed HV types */
    t_connectionHandle connection_handle; /* SCO Connection Handle          */
    t_linkType link_type;                 /* SCO or eSCO                    */

    uint8_t  timing_ctrl;                  /* SCO Initialisation Mechanism   */
    uint8_t  d_sco;
    uint8_t  t_sco;

    uint8_t  lm_sco_handle;                /* SCO Air Handle                  */
    uint8_t  lm_sco_packet;                /* SCO Packet: sco HV1, HV2, HV3   */
    uint8_t  lm_air_mode;                  /* SCO Air Mode: CVSD, u-Law, A-Law*/
    uint16_t voice_setting;                /* Per-link voice setting          */

    uint8_t  sco_index;                    /* Fast access to index via ptr    */
    uint8_t  sco_cfg_hardware;             /* Baseband configuration (Tabasco)*/
    t_lm_sco_states state;                /* Current State of the SCO rf type*/
    t_deviceIndex device_index;           /* The LC link index for this SCO  */
#if (PRH_BS_CFG_SYS_LMP_EXTENDED_SCO_SUPPORTED==1)
    uint8_t w_esco;
    t_packet tx_packet_type;
    t_packet rx_packet_type;
    t_timer esco_end_retransmission_window_timer;
    uint16_t tx_packet_length;
    uint16_t rx_packet_length;
    t_lt_addr esco_lt_addr;

    /*
     *
     * CM: 22 SEP 03
     * 
     * This is required as the the allocation of the LT container
     * for the device_link (ie the lt_index) is done at a point
     * where the lt_addr is not known. Once the lt_addr is known
     * the LM must ensure that the association between the 
     * lt_index and lt_addr is made.
     * 
     * Would like to get rid of the requirement for this element !
     */
    uint8_t esco_lt_index;

    /*
     * Indicates whether the negotiated packet was transmitted in the reserved 
     * slot. Ie the event that there is no eSCO data available, then a POLL/NULL
     * packet will be substituted accordingly. 
     */
    uint8_t esco_negotiated_pkt_tx_in_reserved_slot;

#if (SYS_DEV_ESCO_SLAVE_TX_AFTER_ESCO_MASTER==1)
	/* 
	 * Indicates whether we have started to receive esco in the reserved slot.
	 */
	uint8_t esco_negotiated_pkt_rx_in_reserved_slot;
#endif
    /*
     * Used by the Master during eSCO window initialisation in the 
     * scenario where the eSCO channel uses the full bandwidth 
     * (ie Width(Reserved Slot) + Width (Retransmission) = TeSCO.
     * The Master will not use the retransmission window
     * until it has received a packet from the slave on the
     * eSCO channel. Not using the retransmission window
     * ensures that the Master will POLL the slave on the ACL
     * channel, thus allowing the ACK (for the LMP_accept)
     * to be sent to the Slave (on receiving this ACK the slave
     * will start his eSCO window).
     * In the scenario where max. bandwidth is not used, this bit will
     * be initialised to "0" (ie no need to wait for esco rx) as there
     * should be an oppotunity to TX the ACK on the ACL channel in 
     * between the eSCO windows.
     */
    t_q_descr *tx_descriptor;
    uint8_t negotiation_state;
    uint32_t tx_bandwidth;
    uint32_t rx_bandwidth;
    uint16_t max_latency;
    uint8_t retransmission_effort;
    uint16_t esco_packet_types;
	//uint16_t esco_packet_types_req; -- Seems redundant

   // t_error reject_reason; -- Not used..

    uint8_t renegotiated_params_available;
#if 1 // NEW_TRANSCODE_CONTROL
	  // GF 18 Feb - eSCO retransmission

	uint32_t	Start_Tx_Hpf_Filt;
	uint32_t Start_Tx_Pf1_Filt_A;
	uint32_t Start_Tx_Pf1_Filt_B;
	uint32_t Start_Tx_Pf2_Filt;
	uint32_t Start_Tx_Pf3_Filt;
	uint32_t Start_Tx_Cvsd_Filt_A;
	uint32_t Start_Tx_Cvsd_Filt_B;

	uint32_t	Start_Rx_Hpf_Filt;
	uint32_t Start_Rx_Pf1_Filt_A;
	uint32_t Start_Rx_Pf1_Filt_B;
	uint32_t Start_Rx_Pf2_Filt;
	uint32_t Start_Rx_Pf3_Filt;
	uint32_t Start_Rx_Cvsd_Filt_A;
	uint32_t Start_Rx_Cvsd_Filt_B;

	uint32_t	End_Tx_Hpf_Filt;
	uint32_t End_Tx_Pf1_Filt_A;
	uint32_t End_Tx_Pf1_Filt_B;
	uint32_t End_Tx_Pf2_Filt;
	uint32_t End_Tx_Pf3_Filt;
	uint32_t End_Tx_Cvsd_Filt_A;
	uint32_t End_Tx_Cvsd_Filt_B;

	uint32_t	End_Rx_Hpf_Filt;
	uint32_t End_Rx_Pf1_Filt_A;
	uint32_t End_Rx_Pf1_Filt_B;
	uint32_t End_Rx_Pf2_Filt;
	uint32_t End_Rx_Pf3_Filt;
	uint32_t End_Rx_Cvsd_Filt_A;
	uint32_t End_Rx_Cvsd_Filt_B;

#endif

#if(HOST_HFP17_MSBC_SUPPORTED == 1)
    uint8_t msbc_seqn;
#endif

#endif
    struct list_head header_list;
}   t_sco_info;
#if (ESCO_LINK_CRC_ERROR_CHANGE_TO_GOOD == 1)
typedef struct s_eSCO_Filt {
#if 0
	uint32_t	Start_Tx_Hpf_Filt;
	uint32_t Start_Tx_Pf1_Filt_A;
	uint32_t Start_Tx_Pf1_Filt_B;
	uint32_t Start_Tx_Pf2_Filt;
	uint32_t Start_Tx_Pf3_Filt;
	uint32_t Start_Tx_Cvsd_Filt_A;
	uint32_t Start_Tx_Cvsd_Filt_B;

	uint32_t	Start_Rx_Hpf_Filt;
	uint32_t Start_Rx_Pf1_Filt_A;
	uint32_t Start_Rx_Pf1_Filt_B;
	uint32_t Start_Rx_Pf2_Filt;
	uint32_t Start_Rx_Pf3_Filt;
	uint32_t Start_Rx_Cvsd_Filt_A;
	uint32_t Start_Rx_Cvsd_Filt_B;

	uint32_t	End_Tx_Hpf_Filt;
	uint32_t End_Tx_Pf1_Filt_A;
	uint32_t End_Tx_Pf1_Filt_B;
	uint32_t End_Tx_Pf2_Filt;
	uint32_t End_Tx_Pf3_Filt;
	uint32_t End_Tx_Cvsd_Filt_A;
	uint32_t End_Tx_Cvsd_Filt_B;

	uint32_t	End_Rx_Hpf_Filt;
	uint32_t End_Rx_Pf1_Filt_A;
	uint32_t End_Rx_Pf1_Filt_B;
	uint32_t End_Rx_Pf2_Filt;
	uint32_t End_Rx_Pf3_Filt;
	uint32_t End_Rx_Cvsd_Filt_A;
	uint32_t End_Rx_Cvsd_Filt_B;
#else
	uint32_t	Start_Rx_Hpf_Filt;
	uint32_t Start_Rx_Pf1_Filt_A;
	uint32_t Start_Rx_Pf1_Filt_B;
	uint32_t Start_Rx_Pf2_Filt;
	uint32_t Start_Rx_Pf3_Filt;
	uint32_t Start_Rx_Cvsd_Filt_A;
	uint32_t Start_Rx_Cvsd_Filt_B;
#endif
    }t_eSCO_Filt;
#endif

typedef struct s_esco_descr {
    uint8_t t_esco;
    uint8_t w_esco;
    uint8_t d_esco;
    uint8_t timing_ctrl_flag;
    t_packet tx_pkt_type;
    t_packet rx_pkt_type;
    uint16_t tx_pkt_len;
    uint16_t rx_pkt_len;
    uint16_t voice_setting;
    uint8_t lm_sco_handle;
    uint8_t esco_lt_addr;
    uint8_t negotiation_state;
} t_esco_descr;

typedef struct t_lm_event_info {
    t_error status;
    t_linkType link_type;
    t_deviceIndex device_index;
    uint8_t number;
    uint8_t  mode;
    uint8_t  role;
    t_connectionHandle handle;
    t_bd_addr* p_bd_addr;
    t_classDevice cod;
    t_error reason;
    uint8_t* p_u_int8;
    uint16_t value16bit;
    uint8_t lmp_version;
    uint16_t lmp_subversion;
    uint16_t comp_id;
    t_inquiryResult* p_result_list;
    uint32_t latency;
    uint32_t token_rate;
	uint32_t numeric_value;
    uint8_t key_type;
#if ((PRH_BS_CFG_SYS_HCI_V12_FUNCTIONALITY_SUPPORTED==1) && (PRH_BS_CFG_SYS_LMP_SCO_SUPPORTED==1))
    uint8_t transmission_interval;
    uint8_t retransmission_window;
    uint16_t rx_packet_length;
    uint16_t tx_packet_length;
    uint8_t air_mode;
#endif
#if (PRH_BS_CFG_SYS_HCI_V12_FUNCTIONALITY_SUPPORTED==1)
    uint8_t page;
    uint8_t max_page;
#endif
    uint8_t ack_required;

	uint8_t io_cap;
	uint8_t oob_data_present;
	uint8_t auth_requirements;
#if (PRH_BS_CFG_SYS_LMP_INQUIRY_RESPONSE_NOTIFICAITON_EVENT_SUPPORTED==1)
	uint32_t lap;
	uint8_t rssi;
#endif
} t_lm_event_info;

typedef t_error (*t_lm_callback)(uint8_t, t_lm_event_info*);


/*
 * t_lmp_pdu_info
 *      Holds all the parameters for LM_Encode_LMP_PDU() to encode the LMP pdu
 */

typedef struct s_lmp_pdu_info {
    /*
     * Only one of these pointers required at a time, hence union
     */
    union {
        const t_versionInfo  *p_local_version;
        const uint8_t         *p_uint8;
        const t_sco_info     *p_sco_link;
        const t_lm_park_info *p_park;
    } ptr;

   t_error  reason;

   uint8_t   tid_role; 
   uint8_t   length;
   uint16_t  return_opcode;   /* Supports extended opcodes */
   uint8_t   name_offset;
   uint8_t   name_length;
   uint8_t   Nbc;
   uint8_t   max_slots;
   uint8_t   key_size;
   uint8_t   drift;
   uint8_t   jitter;  
   uint8_t   mode;
   uint8_t   timing_control;
   uint8_t   number_of_poll_slots;
   uint8_t   number;

   uint16_t  opcode;                 /* Supports extended opcodes */
   uint16_t  slot_offset;
   uint16_t  interval;
   uint16_t  offset;
   uint16_t  attempt;
   uint16_t  timeout;
   uint32_t  instant;

   uint8_t   settings;
   uint8_t   ack_required;
   uint8_t   Db_present;
   uint16_t  enc_key_len_mask;

#if(PRH_BS_CFG_SYS_TEST_MODE_TESTER_SUPPORTED == 1)
   uint8_t   tm_scenario;
   uint8_t   tm_hop_mode;
   uint8_t   tm_tx_freq;
   uint8_t   tm_rx_freq;
   uint8_t   tm_power_mode;
   uint8_t   tm_poll_period;
   uint8_t   tm_pkt_type;
   uint16_t  tm_pkt_len;
#endif
}t_lmp_pdu_info;


typedef enum {
    MAX_POWER,
    MIN_POWER,
    INTERMEDIATE_POWER
 } t_power_level;

typedef enum {
    MIN=0, /* used to signify DM1-only in EDR mode */
    MEDIUM=1, /* with FEC for BR or 2MBPS in EDR mode */
    HIGH=2, /* without FEC for BR or 3MBPS in EDR mode */
    AUTO=3 /* auto rate on all packets in active ptt */
 } t_rate;


typedef enum {
     REJECT = 0,
     DONT_AUTO_ACCEPT = 1,
     AUTO_ACCEPT = 2,
     AUTO_ACCEPT_WITH_MSS = 3
     
} t_filter_settings;


/*
 * Enum to ensure both setup completes processed before sending
 * HCI_Connection_Complete
 */
typedef enum
{
    LMP_Remote_Setup_Complete = 1,
    LMP_Local_Setup_Complete = 2,
    LMP_No_Setup_Complete = 0,
    LMP_Both_Setup_Completes = 3
} t_LMP_Setup_Bitmask;

typedef enum
{
    LMP_No_Features_Complete = 0,
    LMP_Remote_Features_Complete = 1,
    LMP_Local_Features_Complete = 2,
    LMP_Both_Features_Complete = 3
} t_LMP_Features_Bitmask;

typedef enum
{
   LMcontxt_ENTERING_HOLD = 0x01,
   LMcontxt_LEAVING_HOLD = 0x02,
   LMcontxt_ENTERING_SNIFF = 0x04,
   LMcontxt_LEAVING_SNIFF = 0x08,
   LMcontxt_ENTERING_SCO = 0x10,
   LMcontxt_LEAVING_SCO = 0x20,
   LMcontxt_CHANGED_SCO = 0x40
} t_LM_context;

typedef enum
{
    NOT_ACTIVE,
    ACTIVE_RESERVED_SLOT,
    ACTIVE_RETRANSMISSION
} t_eSCO_frame_activity;

#endif

