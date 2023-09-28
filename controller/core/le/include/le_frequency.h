#ifndef _CEVA_LE_FREQUENCY__
#define _CEVA_LE_FREQUENCY__

/*********************************************************************
 * MODULE NAME:     le_frequency.h
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

#include "le_connection.h"

void LEfreq_Init(void);
t_error LEfreq_Set_Host_Channel_Classificiation(t_p_pdu p_pdu);
t_error LEfreq_Update_All_Channel_Maps(void);
uint8_t* LEfreq_Read_Channel_Map(uint16_t handle);
uint8_t LEfreq_Map_Channel_Index_2_RF_Freq(uint8_t channel_index);
DRAM_CODE t_error LEfreq_Update_Channel_Map_Req(t_LE_Connection* p_connection,t_p_pdu p_pdu);
void LEfreq_Advance_Scan_Frequency(void);
void LEfreq_Advance_Initiating_Frequency(void);
void LEfreq_Reset_Adv_Frequency(void);
uint8_t LEfreq_Try_Advance_Adv_Frequency(void);
uint8_t LEfreq_Get_Next_Data_Channel_Frequency(t_LE_Connection* p_connection);
t_error LEfreq_Update_Channel_Map(t_LE_Connection* p_connection);
void LEfreq_Update_Channel_Remapping_Table(t_LE_Connection* p_connection, uint8_t* new_channel_map);
t_error LEfreq_Set_Channel_Bad(uint8_t data_channel);
void LEfreq_Reset_Device_Channel_Map(void);

#endif
