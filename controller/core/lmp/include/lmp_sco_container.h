#ifndef _PARTHUS_SCO_CONTAINER_
#define _PARTHUS_SCO_CONTAINER_

/******************************************************************************
 * MODULE NAME:    lm_sco_container.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:   
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:        
 *
 * SOURCE CONTROL: $Id: lmp_sco_container.h,v 1.48 2014/03/11 03:14:05 garyf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/

#include "sys_types.h"
#include "lmp_acl_connection.h"    

void LMscoctr_Initialise(void);

t_sco_info *LMscoctr_Alloc(t_deviceIndex device_index, uint16_t voice_setting, t_linkType link_type);
void LMscoctr_Free(t_sco_info *p_sco_link);
void LMscoctr_Free_SCOs_On_Link(t_deviceIndex device_index);
void LMscoctr_Clear_eSCO_Pending(t_sco_info *p_esco_link);

#if (PRH_BS_CFG_SYS_LMP_SCO_SUPPORTED==1)
RAM_CODE uint8_t  LMscoctr_Get_Number_SCO_Connections(void);
DRAM_CODE uint8_t  LMscoctr_Get_Number_SYN_Connections(void);
uint8_t  LMscoctr_Is_All_Bandwidth_Used_By_SCO(void);
uint8_t  LMscoctr_Is_All_Bandwidth_Used_By_SYN(void);
uint8_t  LMscoctr_Get_Number_SCOs_Active_On_Link(t_deviceIndex device_index);
uint8_t  LMscoctr_Get_Number_SCOs_Active(void);
#else
/*
 * If No SCOs then stub to 0
 */
#define LMscoctr_Get_Number_SCO_Connections()       (0)
#define LMscoctr_Get_Number_SYN_Connections()       (0)
#define LMscoctr_Is_All_Bandwidth_Used_By_SCO()     (0)
#define LMscoctr_Get_Number_SCOs_Active_On_Link(device_index) (0)

#endif

#define ESCO_DONT_CARE_BANDWIDTH 0xFFFFFFFF
#define ESCO_DONT_CARE_LATENCY   0xFFFF
#define ESCO_DONT_CARE_RETRANSMISSION_EFFORT 0xFF

#if (PRH_BS_CFG_SYS_LMP_EDR_SUPPORTED==1)
#define LM_SCO_CTR_NUM_PKT_TYPES 7
#else
#define LM_SCO_CTR_NUM_PKT_TYPES 3
#endif

typedef enum {
    ESCO_NULL = 0,
    ESCO_SETUP,
    ESCO_RECONFIGURE,
    ESCO_REMOVE,
    ESCO_IDLE
} t_esco_transaction_type;

typedef enum {
    ESCO_NGT_INIT,
    ESCO_NGT_PREF,
    ESCO_NGT_RS_VIOL,
    ESCO_NGT_LAT_VIOL,
    ESCO_NGT_UNSUPPORTED_FEATURE
} t_esco_neg_state ;

typedef enum {
    FIRST_NEW_ESCO_PDU,
    CONTINUE_NEW_ESCO_PDU,
    FIRST_CHANGE_ESCO_PDU,
    CONTINUE_CHANGE_ESCO_PDU,
    REJECT_NEW_ESCO_PDU
} t_esco_link_req_context;

typedef struct s_esco_lmp_parameter_set {
    uint8_t t_esco;
    uint8_t w_esco;
    t_packet pkt_type;
    uint16_t pkt_len;
} t_esco_lmp_parameter_set;
	
typedef struct s_lm_esco_pkt_type_desc {
	uint16_t hci_type;
    t_packet bb_type;
    uint16_t max_len;
    uint8_t w_esco_slots;
    uint8_t min_t_esco;
} t_lm_esco_pkt_type_desc;

typedef enum {
    LINEAR_IC,
    U_LAW_IC,
    A_LAW_IC,
    CVSD_IC
} t_input_coding;

typedef enum {
    ONES_COMP_IDF,
    TWOS_COMP_IDF,
    SIGN_MAG_IDF,
    UNSIGNED_IDF
} t_input_data_format;

typedef enum {
    EIGHT_BIT_ISS,
    SIXTEEN_BIT_ISS
} t_input_sample_size;

typedef enum {
    CVSD_ACF,
    U_LAW_ACF,
    A_LAW_ACF,
    TRANS_ACF
} t_air_coding_format;

typedef enum {
    REGULAR_RETRANS,
    SHARED_RETRANS
} t_esco_retrans_mode;

t_sco_info *LMscoctr_Find_SCO_By_Connection_Handle(t_connectionHandle connection_handle);
uint8_t LMscoctr_Find_SCO_Connection_Handle_Is_Used(t_connectionHandle handle);
RAM_CODE uint8_t  LMscoctr_Get_Number_eSCO_Connections(void);
t_sco_info *LMscoctr_Find_SCO_By_SCO_Handle(uint8_t sco_handle);
DRAM_CODE t_sco_info *LMscoctr_Find_SCO_By_SCO_Index(uint8_t index);
t_sco_info *LMscoctr_Find_SCO_By_Device_Index(t_deviceIndex device_index);

uint8_t LMscoctr_Convert_HCI_To_LM_SCO_Packet_Type(t_packetTypes packet_type);

uint8_t  LMscoctr_Is_Bandwidth_Available_For_SCO_Packet_Types(
    t_packetTypes packet_types);

uint8_t  LMscoctr_Get_SCO_Index(const t_sco_info *p_sco_link);

void LMscoctr_Assign_SCO_Parameters(
    t_sco_info *p_sco_link_dest, const t_sco_info *p_sco_link_source);
void LMscoctr_Derive_SCO_Parameters(
    t_sco_info *p_sco_link, t_packetTypes packet_types, t_role role,
    uint8_t use_peer_air_mode_flag);

t_error LMscoctr_Extract_SCO_Parameters_From_PDU(
    t_sco_info *p_sco_link, t_p_pdu p_pdu, t_role role);

uint8_t LMscoctr_Map_AirMode_HCI2LMP(uint8_t hc_air_mode);
uint8_t LMscoctr_Map_AirMode_LMP2HCI(uint8_t lm_air_mode);
uint8_t LMscoctr_Derive_LM_SCO_Packet_Type(
    const t_sco_info *p_sco_link, t_packetTypes packet_types);
uint8_t LMscoctr_Get_Active_LM_SCO_Packet_Type(void);
uint8_t LMscoctr_Get_Used_LM_SCO_Packet_Type(void);
BOOL LMscoctr_Is_LM_Air_Mode_Supported(uint8_t lm_air_mode);
t_sco_info* LMscoctr_Get_SCO_Transitioning_Ref(void);
t_sco_info* LMscoctr_Find_SCO_Under_Negotiation(t_role role);

t_sco_info *LMscoctr_Allocate_Negotation_SCO_Link(
     const t_sco_info *p_sco_link, t_role role, 
     t_packetTypes packet_types, uint8_t lm_air_mode);
void LMscoctr_Free_Negotiation_SCO_Link(void);
void LMscoctr_Accept_And_Free_Negotiated_SCO_Link(t_sco_info *p_sco_link);

DRAM_CODE uint8_t LMeScoctr_Get_Tx_Packet_Type_in_Slots(const t_sco_info *p_sco_link);
DRAM_CODE uint8_t LMeScoctr_Get_Rx_Packet_Type_in_Slots(const t_sco_info *p_sco_link);
RAM_CODE BOOL LMeScoctr_Are_Renegotiated_Params_Available(t_sco_info *p_esco_link);
uint8_t LMscoctr_GetSlotOccupancy(t_packet packet_type);

// dont exist
//uint8_t LMeScoctr_Get_M_to_S_Packet_Type_in_Slots(const t_sco_info *p_sco_link);
//uint8_t LMeScoctr_Get_S_to_M_Packet_Type_in_Slots(const t_sco_info *p_sco_link);

RAM_CODE uint8_t LMeScoctr_Get_Number_eSCO_Connections(void);

uint16_t LMscoctr_Map_eSCO_Pkt_Type_LMP2HCI(uint8_t packet_type);
uint16_t LMscoctr_Map_eSCO_Pkt_Type_BB2HCI(uint8_t packet_type);

t_packet LMscoctr_Get_eSCO_BB_Packet_From_LMP_Type(uint8_t lmp_pkt_type);
uint8_t LMscoctr_Get_eSCO_LMP_Type_From_BB_Packet(t_packet bb_pkt_type);

void LMscoctr_Extract_eSCO_Parameters_From_LMP_PDU(
    t_p_pdu p_pdu);
t_error LMscoctr_Validate_Host_eSCO_Parameters(uint16_t handle,
        uint32_t tx_bandwidth, uint32_t rx_bandwidth,
        uint16_t max_latency, uint16_t voice_setting, uint8_t retransmission_effort, 
        uint16_t* esco_packet_types);
t_error LMscoctr_Validate_Peer_eSCO_Parameters(t_lmp_link* p_link,
        uint8_t* p_pdu);
t_error LMscoctr_Validate_Admission(uint32_t tx_bandwidth);
t_error LMscoctr_Detect_Transaction_Collisions(void);

t_sco_info* LMscoctr_Get_Topical_eSCO_Link(void);
t_esco_transaction_type LMscoctr_Get_Topical_eSCO_Link_Transaction_Type(void);
void LMscoctr_Set_Topical_eSCO_Link(t_sco_info* p_esco_link, t_esco_transaction_type type);

t_lmp_link* LMscoctr_Find_ACL_Link_From_SCO_Link(t_sco_info* p_esco);
void LMscoctr_Commit_to_New_Synchronous_Parameters(t_sco_info* p_esc);

BOOL LMscoctr_Get_LMP_Parameter_Set(t_esco_descr* p_esco_descr);
int LMscoctr_Is_An_LMP_Parameter_Set(uint8_t t_esco, uint8_t w_esco, t_packet pkt_type, uint16_t pkt_len);
void LMscoctr_Initialise_LMP_Paramater_Sets(void);
t_esco_descr* LMscoctr_Get_eSCO_Scratchpad(void);
void LMscoctr_Clean_eSCO_Scratchpad(void);

void LMscoctr_Set_D_eSCO_and_Timing_Flag(t_esco_descr* p_esco_descr);
int16_t LMscoctr_Derive_D_eSCO(t_sco_info* p_esco_link, uint8_t t_esco, 
    uint8_t w_esco, uint8_t timing_ctrl, t_packet tx_packet_type, t_packet rx_packet_type);
int16_t LMscoctr_Derive_D_eSCO_AtLeast(uint8_t init_d_esco,
	t_sco_info* p_esco_link, uint8_t t_esco, uint8_t w_esco,
	uint8_t timing_ctrl, t_packet tx_packet_type, t_packet rx_packet_type);

void LMscoctr_Suspend_Link(t_sco_info* p_esco_link);
void LMscoctr_Resume_Link(t_sco_info* p_esco_link);
uint32_t LMscoctr_Calculate_Bandwidth(t_esco_descr* p_esco_descr);
int LMscoctr_Is_D_eSCO_Acceptable(t_esco_descr* p_esco_descr, t_lmp_link* p_link);
int16_t LMscoctr_Get_Alt_D_eSCO(t_esco_descr* p_esco_descr,t_lmp_link* p_link);
uint16_t LMscoctr_Calculate_Latency_In_uSec(uint8_t t_esco, uint8_t w_esco, 
    t_packet tx_pkt_type, t_packet rx_pkt_type);
void LMscoctr_Clear_D_eSCO_List(void);
void LMscoctr_Add_D_eSCO_Element(uint8_t d_esco);

RAM_CODE uint16_t LMscoctr_Derive_Tx_Dequeue_Length(t_sco_info* p_esco_link);
uint16_t LMscoctr_Derive_Rx_Dequeue_Length(t_sco_info* p_esco_link);

// dont exist
//void LMscoctr_Set_Voice_Setting(t_sco_info* p_esco_link,
//   uint16_t voice_setting);
//uint16_t LMscoctr_Get_Voice_Setting(t_sco_info* p_esco_link);
t_input_coding LMscoctr_Get_Input_Coding(uint16_t voice_setting);
t_input_data_format LMscoctr_Get_Input_Data_Format(uint16_t voice_setting);
uint8_t LMscoctr_Get_Input_Sample_Size(uint16_t voice_setting);
uint8_t LMscoctr_Get_Linear_PCM_Bit_Pos(uint16_t voice_setting);
t_air_coding_format LMscoctr_Get_Air_Coding(uint16_t voice_setting);

/*
 * The following routines are used on slave-initiated SCO when
 * the IUT is a slave - it frees and re-allocates the SCO container
 * (losing the voice setting information. Store that voice
 * information in a holding variable.
 */
void LMscoctr_Store_Topical_Voice_Setting(uint16_t voice_setting);
uint16_t LMscoctr_Restore_Topical_Voice_Setting(void);

/*
 * The following routines are used to set and get the eSCO retransmission
 * mode. Either shared or unshared.
 */
t_esco_retrans_mode LMscoctr_Get_eSCO_Retransmission_Mode(void);
void LMscoctr_Set_eSCO_Retransmission_Mode(t_esco_retrans_mode mode);

t_length LMscoctr_Determine_SCO_Pkt_Length(uint8_t sco_packet);
#if (ESCO_LINK_CRC_ERROR_CHANGE_TO_GOOD == 1)
DRAM_CODE uint32_t LMscoctr_Read_Start_Rx_Transcode_State(t_eSCO_Filt *p_sco_link);
#endif
DRAM_CODE void LMscoctr_Reset_Start_Rx_Transcode_State(t_sco_info *p_sco_link);
DRAM_CODE void LMscoctr_Advance_Tx_Transcode_State(t_sco_info *p_sco_link);
DRAM_CODE void LMscoctr_Advance_Rx_Transcode_State(t_sco_info *p_sco_link);
DRAM_CODE void LMscoctr_Read_End_Tx_Transcode_State(t_sco_info *p_sco_link);
DRAM_CODE void LMscoctr_Read_End_Rx_Transcode_State(t_sco_info *p_sco_link);
DRAM_CODE void LMscoctr_Write_Start_Tx_Transcode_State(t_sco_info *p_sco_link);
DRAM_CODE void LMscoctr_Write_Start_Rx_Transcode_State(t_sco_info *p_sco_link);

DRAM_CODE uint8_t LMscoctr_Current_eSCO_Link_In_eSCO_Window(t_sco_info *p_sco_link);
DRAM_CODE uint8_t LMscoctr_Current_eSCO_Link_In_Reserved_Slot(t_sco_info *p_sco_link);

t_slots LMscoctr_Check_Slots_to_next_eSCO_On_Device(uint32_t current_clk);
t_slots LMscoctr_Check_Slots_to_next_eSCO_Based_Clk(void);
uint8_t LMscoctr_Find_Lowest_Tsco(void);
uint8_t LMscoctr_Find_Largest_Tsco(void);

/*****************************************************************************
 * SCO_LIST_HEAD
 ****************************************************************************/
struct SCO_LIST_HEAD {
    struct list_head list;
    t_sco_info p_sco_link;
};

#endif
