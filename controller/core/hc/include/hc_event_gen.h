#ifndef _PARTHUS_HC_HCI_EVENTS_
#define _PARTHUS_HC_HCI_EVENTS_

/**********************************************************************
 * MODULE NAME:    hc_event_gen.h
 * PROJECT CODE:   BlueStream
 * DESCRIPTION:    Generates the HCI Events
 * MAINTAINER:     Gary Fleming
 * DATE:           28 May 1999
 *
 * SOURCE CONTROL: $Id: hc_event_gen.h,v 1.38 2010/05/11 14:33:42 nicolal Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *    V1.0     xx xxx 1999 -   gf       - Initial Version 
 *
 **********************************************************************/

#include "sys_types.h"
#include "lmp_types.h"
  

typedef struct {
    const t_bufferSize* p_hc_buffer_size;
    } t_Read_Buffer_Size;  

typedef struct {
    const t_versionInfo*  version;
    } t_Read_Local_Version;

typedef struct {
    t_classDevice  cod;
    } t_Read_Class_Device;


typedef struct {
    uint16_t max_num_keys;
    uint16_t  num_keys_read;
} t_Read_Stored_Link_Key;

typedef struct {
     uint8_t page;
     uint8_t max_page;
} t_Read_Local_Extended_Features;

typedef union {
    t_Read_Buffer_Size readBufferSize;
    t_Read_Local_Version readLocalVersion;
    t_Read_Stored_Link_Key  readStoredLinkKey;
    t_Read_Class_Device readClassDevice;
    t_Read_Local_Extended_Features readLocalExtendedFeatures;
} t_command_complete_return_params;

typedef struct {
    uint16_t opcode;
    t_connectionHandle handle;
    t_slots timeout;
    uint8_t mode;
    const uint8_t* p_u_int8;
    t_error status;
    const t_bd_addr* p_bd_addr;
    uint16_t value16bit;
    uint8_t number;
    uint32_t u_int32val;
    t_scanActivity scan_activity;
    t_command_complete_return_params returnParams; 
 } t_cmd_complete_event; 

typedef struct {
    const t_inquiryResult* p_result_list;
    uint8_t Num_Responses;
} t_inquiry_result_event;

typedef struct {
    uint8_t Num_Responses;
    t_error status;
} t_inquiry_complete_event;

void HCeg_Initialise(void);
t_error HCeg_Set_Event_Mask(t_p_pdu p_pdu);

t_error HCeg_Inquiry_Result_Event(uint8_t event_cdoe, t_lm_event_info* p_inq_event_info);
void HCeg_Command_Complete_Event(t_cmd_complete_event* event_nfo);
void HCeg_Command_Status_Event(t_error status, uint16_t opcode);
void HCeg_Loopback_Command_Event(t_p_pdu p_command_buff, uint16_t opcode);

BOOL HCeg_Number_Of_Completed_Packets_Event(uint8_t num_handles, 
     const uint16_t* handles, const uint16_t *completed_packets); 

void HCeg_Hardware_Error_Event(uint8_t hardware_code);

/*
 * Helper function used to determine if an event has been masked.
 */
uint32_t HCeg_Is_Event_Masked_On(uint32_t event_code);
/* 
 * Helper function which prevents overflow of the HCI_Event_Q.
 */
uint8_t HCeg_Prevent_HCI_Event_Queue_from_Overfill(void);


/*
 * Handles all other events
 */
t_error HCeg_Generate_Event(uint8_t event_code,t_lm_event_info* p_event_info);

void _Insert_Uint8(t_p_pdu p_buffer, uint8_t value_8_bit);
void _Insert_Uint16(t_p_pdu p_buffer, uint16_t value_16_bit);
void _Insert_Uint24(t_p_pdu p_buffer, uint32_t value_24_bit);
void _Insert_Uint32(t_p_pdu p_buffer, uint32_t value_32_bit);

#endif
