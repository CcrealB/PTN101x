

#ifndef _CEVA_LE_CONFIG__
#define _CEVA_LE_CONFIG__

/*********************************************************************
 * MODULE NAME:     le_config.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:
 * MAINTAINER:      Gary Fleming
 * DATE:            Jan-2012
 * SOURCE CONTROL:
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2012 Ceva Inc.
 *     All rights reserved.
 *
 *********************************************************************/
#include "uslc_sleep.h"
#include "sys_features.h"
#include "le_connection.h"
#include "driver_gpio.h"

typedef struct
{
	uint8_t event_mask;
	uint8_t  own_address_type;
	t_bd_addr random_address;
#if (PRH_BS_CFG_SYS_EXTENDED_HCI_COMMANDS_SUPPORTED==1)
	uint8_t auto_advertise;
	uint8_t auto_initiate;
	uint8_t slave_listen_outside_latency;
	uint8_t preConfigured_hop_inc_available;
	uint8_t preConfigured_hop_inc;
	uint8_t preConfigured_access_address_available;
	uint32_t preConfigured_access_address;
#endif
	uint8_t trace_level;
	t_bd_addr advertiser_address;

	uint8_t  state; // Main State for LE device
	uint8_t  sub_state; // Substate is dependent on what the main state is.

// Initiating Information
	uint16_t initiator_scan_interval;
	uint16_t initiator_scan_window;
	uint8_t  initiator_filter_policy;
	uint8_t  initiator_link_id;
	uint8_t  initiating_active;
	t_timer next_initiator_scan_timer;

	uint8_t resuming_scan;
	uint8_t  data_channel_map[5];

#if (PRH_BS_CFG_SYS_CHANNEL_ASSESSMENT_SCHEME_SUPPORTED==1)
	t_timer device_channel_map_update_timer;
	uint32_t device_channel_map_update_timeout; 
#endif

	uint8_t  sleep_clock_accuracy;

	uint32_t  link_id_pool;

	uint8_t active_link; // Currently Active Link
	uint8_t slave_links_active;
	uint8_t master_link_active;

//uint8_t incoming_connection_device_index;
	uint8_t last_adv_pdu_rxed;
	uint32_t next_connection_event_time;

	uint8_t connection_complete_pending;
	t_LE_Connection* p_connection_complete_pending;

	uint8_t TCI_transmit_win_config;
	uint16_t transmit_win_offset;
	uint8_t transmit_win_size;

	uint8_t device_sleep;
	uint8_t tabasco_asleep;
	uint8_t num_le_links;

	uint8_t SMP_present;
	uint8_t AES_complete;
	uint8_t encrypt_pending_flag;
	uint8_t device_index_pending_data_encrypt;
#if (PRH_BS_CFG_SYS_LE_SMP_INCLUDED==1)// SMP Supported - Not in HCI Only Build
	uint8_t io_cap;
	uint8_t oob_data_delivery;
	uint8_t auth_requirements;
	uint8_t max_encry_key_size;
	uint8_t min_encry_key_size;
	uint8_t SMP_InitiatorKeyDistribution;
	uint8_t SMP_ResponderKeyDistribution;
	uint8_t SMP_TK[16];
	uint8_t pending_decrypt_link;
	uint8_t pending_decrypt_len;
#endif

	uint8_t test_mode; //1 bit

#if (PRH_BS_CFG_SYS_ENCRYPTION_SUPPORTED==1)
	// Security variables
	uint32_t link_key_request_timer;
	uint8_t  long_term_key[16];
	uint8_t  random_number[8];
	uint16_t encrypt_diversifier;
	uint8_t  SKD[16]; /* session key identifier */
	uint8_t  device_index_for_ltk;
#endif

	//uint8_t SMP_bonded;
	uint8_t hc_data_tester_present; // Simple flag to indicate if the HC Data Tester is driving the HC
	uint8_t Auto_Widen_MultiSlot;

	uint8_t initiating_enable;

	uint8_t sleep_request_pending;
#if (PRH_BS_CFG_SYS_LE_BQB_TESTER_SUPPORT==1)
	uint32_t tester_test_config;
	uint8_t tester_introduce_mic_error;
	uint8_t tester_introduce_header_error;
#endif


} t_LE_Config;


typedef struct {
	uint8_t echo_tr_win;
	uint8_t num_time_master_tx_first_win;
	uint8_t enable_scan_backoff;
} t_LE_TCI_Config;


typedef enum
{
#if (PRH_BS_CFG_SYS_LE_LOW_DUTY_DIRECT_ADVERTISING==1)
	ADV_IND_TYPE = 0,
	ADV_DIRECT_TYPE_HIGH_DUTY_TYPE = 1,
	ADV_SCAN_IND_TYPE = 2,
	ADV_NONCONN_IND_TYPE = 3,
    ADV_DIRECT_TYPE_LOW_DUTY_TYPE = 4
#else
	ADV_IND_TYPE = 0,
    ADV_DIRECT_TYPE = 1,
	ADV_SCAN_IND_TYPE = 2,
	ADV_NONCONN_IND_TYPE = 3
#endif
} t_LE_advertising_type;

typedef enum
{
	ADV_IND_EVENT_TYPE  = 0,
    ADV_DIRECT_EVENT_TYPE = 1,
	ADV_DISCOVER_IND_EVENT_TYPE = 2,
	ADV_NONCONN_IND_EVENT_TYPE = 3,
	SCAN_RSP_EVENT_TYPE = 4
} t_LE_advertising_report_events;



// Advertising Substates
#define SUBSTATE_IDLE        0x00
#define W4_T2_PRE_ADV_TX     0x01
#define W4_ADV_TX            0x02
#define W4_ADV_RX            0x03
#define W4_SCAN_RESP_TX      0x04
#define W4_N_x_T2            0x05
#define W4_N_x_T0            0x06
#define W4_T0_PRE_ADV_TX     0x07

// Scanning Substates
#define W4_T2_PRE_SCAN       0x10
#define W4_SCAN_RX           0x20
#define W4_SCAN_TX           0x30
#define W4_SCAN_RESP_RX      0x40
#define W4_SCAN_RESUME       0x50
#define W4_SCAN_PKD          0x60
#define W4_SCAN_RESP_PKD     0x70
#define W4_SCAN_DUMMY_TX     0x80
#define W4_DUMMY_SCAN_RESP_TX 0x90
#define W4_RESUME_SCAN       0xA0
#define W4_RESUME_SCAN_NEXT_TIM 0xB0
#define W4_RESUME_FROM_CLASSIC 0xC0

// Connection Event Substates
#define W4_T2_MASTER_CONN_FIRST_TX      0x01
#define W4_MASTER_CONN_FIRST_TX         0x02
#define W4_MASTER_CONN_RX               0x03
#define W4_MASTER_CONN_TX               0x04
#define W4_T0_PRE_MASTER_FIRST_CONN_TX  0x05


#define W4_T0_PRE_SLAVE_CONN_FIRST_RX      0x10
#define W4_T2_SLAVE_CONN_FIRST_RX          0x11
#define W4_SLAVE_CONN_FIRST_RX             0x12
#define W4_SLAVE_CONN_TX                   0x13
#define W4_SLAVE_CONN_RX                   0x14
#define W4_T1_SLAVE_CONN_FIRST_FULL_RX     0x15
#define W4_T2_SLAVE_CONN_FIRST_FULL_RX     0x16
#define W4_T0_PRE_SLAVE_CE                 0x17
#define W4_T2_PRE_SLAVE_CE                 0x18
#define W4_MULTISLOT_WINDOW_WIDENING_RX    0x19
#define W4_T0_PRE_MULTISLOT_WINDOW_WIDENING_RX 0x20
#define W4_T2_PRE_MULTISLOT_WINDOW_WIDENING_RX 0x21

#define TIFS_TX_ADJUSTMENT 31
#define TIMED_SPI_LE
#define LE_INCLUDED 1
#define LE_SW_AES 1
//Feature set

#define DEFAULT_MAX_NUM_LE_LINKS 1


#define EEPROM_BASE_ADDR_FOR_BONDING_INFO 0x10
#define DEFAULT_LE_DEVICE_CHANNEL_MAP_UPDATE_TIMEOUT 0x1000


t_error LEconfig_Init(void);
void LEconfig_Scan_Interval_Timeout(uint32_t current_clk);
uint8_t LEconfig_Check_Current_Address(uint8_t RxAdd,t_p_pdu p_pdu);
uint8_t LEconfig_Get_Addvertising_Own_Address_Type(void);
void LEconfig_Set_Event_Mask(uint8_t* p_pdu);
const uint8_t* LEconfig_Get_LE_Features_Ref(void);
uint8_t LEconfig_Is_Features_Encryption_Supported(void);
void LEconfig_Set_Random_Address(uint8_t* p_random_address);

t_error LEconfig_Set_Scan_Parameters(t_p_pdu p_pdu);
t_error LEconfig_Set_Scan_Enable(uint8_t enable,uint8_t filter_duplicates);
const uint8_t* LEconfig_Read_Supported_States(void);
uint8_t LEconfig_Check_Current_Address(uint8_t RxAdd,t_p_pdu p_pdu);
uint8_t LEconfig_Allocate_Link_Id(uint8_t* link_id);
void LEconfig_Free_Link_Id(t_LE_Connection* p_connection);
void LEconfig_Disable_Connectable_Advertising(void);
#if (PRH_BS_CFG_SYS_LE_DUAL_MODE==1)
void LEconfig_Delay_Next_Initiating(uint8_t delay);
void LEconfig_Switch_To_LE(uint8_t device_index);

void LEconfig_Switch_To_BT_Classic(void);
#if (SEAN_MODIFY_REF_BQB == 0)
void LEconfig_Complete_Switch_To_Classic(void);
#endif
#endif
#if (PRH_BS_CFG_SYS_EXTENDED_HCI_COMMANDS_SUPPORTED==1)
t_error LEconfig_TCI_Set_Search_Window_Delay(uint8_t delay);
t_error LEconfig_TCI_Set_TIFS_Tx_Adjustment(uint8_t delay);
#endif
#define LEconnection_Determine_Connection_Handle(p_connection)  (p_connection->link_id + PRH_BS_CFG_LE_CONN_HANDLE_OFFSET)

extern uint8_t LEconfig_device_address[6];
#endif

