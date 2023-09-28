#ifndef _CEVA_LE_GAP_H__
#define _CEVA_LE_GAP_H__

/*********************************************************************
 * MODULE NAME:     le_gap.h
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
#include "includes.h"
#ifdef BT_DUALMODE
#include "le_link_layer.h"
#define GAP_NON_DISCOVERABLE     0x00
#define GAP_LIMITED_DISCOVERABLE 0x01
#define GAP_GENERAL_DISCOVERABLE 0x02
#define GAP_TURNOFF_DISCOVERABLE 0x03

#define GAP_NON_DISCOVERY        0x00
#define GAP_LIMITED_DISCOVERY    0x01
#define GAP_GENERAL_DISCOVERY    0x02


typedef struct t_gap_event_params {
	uint8_t event;
	uint16_t handle;
	uint8_t status;
	uint8_t* p_address;
	uint8_t valid_reconn_addr;
	uint8_t encrypt;
	uint8_t role;

} t_gap_event_params;

extern uint32_t gCurrent_ADV_interval;

void GAP_Set_AD_Data(uint8_t length, uint8_t* p_data);


void GAP_Set_ScanResp_Data(uint8_t length, uint8_t* p_data);

uint8_t GAP_Discoverable_Mode(uint8_t mode,uint8_t adv_type,uint8_t Dual_mode);
uint8_t GAP_Discovery_Mode(uint8_t mode);

uint8_t GAP_Set_Broadcast_Data(uint8_t length,uint8_t* p_data);
uint8_t GAP_Set_Broadcast_Mode(uint8_t mode, uint8_t type);

uint8_t GAP_Connectable_Mode(uint8_t mode, uint8_t peer_addr_type,uint8_t own_addr_type, uint8_t* p_bdaddr);

#if (PRH_BS_CFG_SYS_LE_CENTRAL_DEVICE == 1)
uint8_t GAP_Connection_Establishment(uint8_t mode, uint8_t* peer_address,uint8_t peer_address_type,
		                            uint16_t max_conn_int, uint16_t min_conn_int, uint16_t latency,uint16_t super_to);
//FLASH_LE_CODE uint8_t GAP_Connection_Establishment(uint8_t mode, volatile uint8_t* peer_address,uint8_t peer_address_type,
//		                            uint16_t max_conn_int, uint16_t min_conn_int, uint16_t latency,uint16_t super_to);

#endif
uint8_t  GAP_Find_Connectable_Devices(void);
uint8_t  GAP_Connection_Complete(t_LE_Connection* p_connection, uint8_t status);
t_error LEgap_Handle_Encryption_Change_Event(t_LE_Connection* p_connection,uint8_t status, uint8_t mode );

uint8_t GAP_Init(void);
void   GAP_Check_GAP_Timers(void);
void   GAP_Register_Callbacks(void* event_funct_ptr, void* command_complete_funct_ptr);
void   GAP_DeRegister_Callbacks(void);
uint8_t GAP_Handle_Advertising_Events(t_advert_event* p_Advert_Event);
void   GAP_Clear_Connect_Params_Update_Pending(void);
uint8_t GAP_Set_Connect_Params_Update_Pending(uint16_t handle);
void GAP_Disconnect(uint16_t handle);
t_error GAP_Set_Privacy(uint8_t privacy);
void Set_LEgap_Config_state(uint8_t state);
void LEadv_Advertise_Disable_Enable(unsigned char enable);
uint8_t GAP_Set_Advertising_Interval(uint32_t min, uint32_t max);
#endif
#endif
