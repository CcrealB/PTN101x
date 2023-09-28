#ifndef _CEVA_LE_SCAN__
#define _CEVA_LE_SCAN__

/*********************************************************************
 * MODULE NAME:     le_scan.h
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


t_error LEscan_Init(void);

uint8_t LEscan_Get_Scanning_Channel_Index(void);
uint8_t LEscan_Get_Initiating_Channel_Index(void);
uint8_t LEscan_Get_Scan_Filter_Policy(void);
uint8_t LEscan_Get_Scan_Type(void);
uint8_t LEscan_Get_Scan_Enable(void);
uint8_t LEscan_Get_Scan_Backoff_Count(void);
uint8_t LEscan_Get_Scan_Upper_Limit(void);
uint8_t LEscan_Get_Scanning_Own_Address_Type(void);
uint8_t LEscan_Ok_To_Send_Scan_Req(void);
uint8_t LEscan_Is_Next_Scan_Due(uint32_t current_clk);
uint16_t LEscan_Get_Scan_Window(void);
uint16_t LEscan_Get_Scan_Interval(void);
void LEscan_Prep_For_LE_Scan_Rx(void);
void LEscan_Set_Current_Window_Timer(t_timer end_scan_window);

void LEscan_Update_Initiator_Scan_Freq(void);
void LEscan_Advance_Initiating_Frequency(void);
void LEscan_Advance_Scan_Frequency(void);
void LEscan_Backoff_Successfull_Scan_Rsp_Rxed(void);
void LEscan_Set_Adv_Header(uint8_t pdu_type,uint8_t rxAddr);
void LEscan_Encode_Own_Address_In_Payload(uint8_t* p_payload);
void LEscan_Scan_Interval_Timeout(uint32_t current_clk);

RAM_CODE uint32_t LEscan_Get_Slots_To_Next_Scanning_Timer(uint32_t current_clk);
uint32_t LEscan_Get_Slots_To_Next_Initiating_Scan_Timer(uint32_t current_clk);
t_error LEscan_Check_For_Pending_Advertising_Reports(void);
t_error LEscan_Ctrl_Handle_Scanning(uint8_t IRQ_Type,uint32_t current_clk);
t_error LEscan_Set_Scan_Parameters(t_p_pdu p_pdu);
t_error LEscan_Set_Scan_Enable(uint8_t enable,uint8_t filter_duplicates);

t_error	LEscan_Genertate_Advertising_Report_Event(uint8_t num_reports,uint8_t event_type,uint8_t addressType,
												  uint8_t* p_address,uint8_t lenData,uint8_t* p_data,int8_t rssi);

void LEscan_Set_Window_Expired(uint16_t time_window_expired);
uint16_t LEscan_Get_Window_Expired(void);
void LEscan_Delay_Next_Scan(uint8_t delay);
t_timer LEscan_Get_Scan_Timer(void);
void LEscan_Set_Scan_Timer(t_timer new_value);

#if (PRH_BS_CFG_SYS_EXTENDED_HCI_COMMANDS_SUPPORTED==1)
t_error LEscan_TCI_Write_Scan_Freqs(uint8_t freqs);
t_error LEscan_TCI_Write_Initiating_Freqs(uint8_t freqs);
t_error LEscan_TCI_Read_Scan_Freqs(void);
t_error LEscan_TCI_Read_Initiating_Freqs(void);
t_error LEscan_TCI_Read_Scan_Params(void);
t_error LEscan_TCI_Enable_Scan_Backoff(uint8_t backoff_enable);
t_error LEscan_TCI_Read_Scan_Backoff_Info(void);
#endif
#if (PRH_BS_CFG_SYS_LE_GAP_INCLUDED==1)
t_error LEscan_Set_GAP_Scan_Parameters(uint16_t scan_interval, uint16_t scan_window, uint8_t scan_type, uint8_t scan_own_address_type,
		                           uint8_t scan_filter_policy);
#endif
#endif
