#ifndef LE_L2CAP_H_
#define LE_L2CAP_H_
/*********************************************************************
 * MODULE NAME:     le_l2cap.h
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
#include "config.h"
#ifdef BT_DUALMODE
typedef struct t_l2cap_event_params {
	uint8_t event;
	uint16_t handle;
	uint8_t status;
}t_l2cap_event_params;


#define L2CAP_CONNECTION_PARAMETER_UPDATE_REQUEST    0x12
#define L2CAP_CONNECTION_PARAMETER_UPDATE_RESPONSE   0x13
#define L2CAP_COMMAND_REJECT                         0x01 /* TP/LE/CPU/BI-02-C,TP/LE/REJ/BI-01-C L2cap part*/

void LE_L2CAP_Init(void);
uint8_t LE_L2CAP_Connection_Update(uint16_t handle, uint16_t max_interval,uint16_t min_interval, uint16_t latency,uint16_t timeout);
t_error LE_L2CAP_Decode_Incoming_PDU(t_LE_Connection* p_link,uint8_t* p_pdu, uint8_t length);
t_error LE_L2CAP_Generate_Response_Event(uint16_t handle,uint8_t status);
void LE_L2CAP_Connection_Update_Complete(t_LE_Connection* p_link,uint8_t status);
void LE_L2CAP_Register_Callbacks(void* event_funct_ptr, void* command_complete_funct_ptr);
void LE_L2CAP_DeRegister_Callbacks(void);

#endif
#endif