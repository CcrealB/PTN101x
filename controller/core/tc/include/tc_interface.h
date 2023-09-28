#ifndef _PARTHUS_TC_INTERFACE_
#define _PARTHUS_TC_INTERFACE_

/*****************************************************************************
 *
 * MODULE NAME:     tc_interface.h   
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:     test control interface header
 * MAINTAINER:      Daire McNamara <Daire.McNamara@sslinc.com>
 * CREATION DATE:   27th January 2000     
 *
 * SOURCE CONTROL: $Id: tc_interface.h,v 1.66 2014/05/01 13:52:45 tomk Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *     27 Jan 2000    DMN   Initial Draft - support TCI_Read_Packet_Log
 *     01 Feb 2000    IG    Active IRQ, Build Params
 *     24 Feb 2000    DMN   Renamed and Split into two headers
 *
 ****************************************************************************/


#include "sys_config.h"
#include "sys_types.h"

void    TC_Write_Local_Hardware_Register(uint8_t reg_bank, uint32_t address_offset, uint32_t value, uint32_t mask);

void    TC_Read_Local_Default_Packet_Type(void);
void    TC_Write_Local_Default_Packet_Type(uint8_t packet_type);

void    TC_Activate_Remote_DUT(uint16_t acl_handle);
void    TC_Perform_Test_Control(t_p_pdu test_control);

void    TC_Increase_Remote_Power(uint16_t acl_handle);
void    TC_Decrease_Remote_Power(uint16_t acl_handle);

void    TC_Write_Local_Hop_Frequencies(uint8_t*);
void    TC_Read_Local_Hop_Frequencies(void);

void    TC_Read_Local_Hardware_Version(void);

void    TC_Increase_Local_Volume(void);
void    TC_Decrease_Local_Volume(void);

void    TC_Write_Local_Hopping_Mode(uint8_t*);
void    TC_Read_Local_Hopping_Mode(void);

void    TC_Write_Local_Whitening_Enable(uint8_t*);
void    TC_Read_Local_Whitening_Enable(void);

void    TC_Write_Local_Link_Key_Type(uint8_t*);
void    TC_Read_Local_Link_Key_Type(void);

void    TC_Read_Local_Extended_Features(void);
void    TC_Write_Local_Features(uint8_t* features);
void    TC_Write_Local_Extended_Features(uint8_t* features);

void    TC_Read_Local_Timing_Information(void);
void    TC_Write_Local_Timing_Information(uint8_t* timing_info);
#if (PRH_BS_CFG_SYS_LMP_TIMING_INFO_SUPPORTED==1)
void    TC_Read_Remote_Timing_Information(uint8_t* conn_handle);
#endif

void    TC_Reset_Local_Pump_Monitors(t_p_pdu );
void    TC_Read_Local_Pump_Monitors(t_p_pdu );

void    TC_Write_Local_Encryption_Key_Length(uint8_t* min_enc_key_len, uint8_t* max_enc_key_len);
void    TC_Read_Local_Encryption_Key_Length(void);

void    TC_Set_Local_Failed_Attempts_Counter(uint8_t*);
void    TC_Clear_Local_Failed_Attempts_Counter(uint8_t*);

void    TC_Read_Local_Baseband_Monitors(void);
#if (DEBUG_BASEBAND_MONITORS == 1)
void    TC_Read_Local_Baseband_Monitors_Via_App(void);
void*   TC_Get_Local_Baseband_Monitors(uint16_t *len);
#endif 
void    TC_Read_Local_Native_Clock(void);
void    TC_Write_Local_Native_Clock(t_p_pdu);

void    TC_Read_Local_Relative_Host_Controller_Mips(void);

void    TC_Set_Local_Next_Available_Am_Addr(t_am_addr);

void    TC_Set_Local_Bd_Addr(uint8_t*);

void    TC_Write_Local_Radio_Power(uint16_t handle, uint16_t radio_power);
void    TC_Read_Local_Radio_Power(uint16_t handle);

void    TC_Write_Local_SyncWord(uint8_t* p_sync);

void    TC_Write_Local_Radio_Register(uint8_t reg_addr, uint16_t reg_val);
void    TC_Read_Local_Radio_Register(uint8_t reg_addr);

void    TC_Set_HCIT_UART_Baud_Rate(uint32_t baud_rate);

void    TC_Change_Radio_Modulation(uint8_t* enb_or_not);
void    TC_Read_Radio_Modulation(void);

void    TC_Generate_Local_AVRCP_L2CAP_Control(uint8_t control_byte);

void    TC_Set_Disable_Low_Power_Mode(void);
void    TC_Set_Enable_Low_Power_Mode(void);
void    TC_Read_R2P_Min_Search_Window(void);
void    TC_Write_R2P_Min_Search_Window(uint8_t new_window);

void    TC_Write_Park_Parameters(uint8_t* pdu);
void    TC_Set_Broadcast_Scan_Window(uint8_t* p_pdu);

void    TC_Read_Unused_Stack_Space(void);

void    TC_Write_VCI_CLK_Override(uint8_t length, uint8_t* p_pdu);

void    TC_Send_Encryption_Key_Size_Mask_Req(uint16_t acl_handle);

void    TC_Read_Raw_RSSI(uint16_t handle);
void    TC_Read_BER(void);
void    TC_Read_PER(void);
void    TC_Read_Raw_RSSI_PER_BER(void);

#if (PRH_BS_CFG_SYS_SCO_REPEATER_SUPPORTED==1)
void    TC_Set_Disable_SCO_Repeater_Mode(void);
void    TC_Set_Enable_SCO_Repeater_Mode(void);
#endif

#if (PRH_BS_CFG_SYS_ENHANCED_POWER_CONTROL_SUPPORTED==1)
void TC_Write_EPC_Enable(uint8_t epc_enable);
#endif

#if (PRH_BS_CFG_SYS_PTA_DRIVER_SUPPORTED==1)
void TC_Write_PTA_Enable(uint8_t pta_enable, uint8_t pta_test_mode, 
							uint8_t pta_req_lead_time);
#endif

void TC_Set_Disable_SCO_Via_HCI(void);
void TC_Set_Enable_SCO_Via_HCI(void);

void   TC_Write_eSCO_Retransmission_Mode(uint8_t);
void   TC_Read_eSCO_Retransmission_Mode(void);

void    TC_Write_Security_Timeouts(uint16_t link_key_timeout, uint16_t pin_code_timeout,
            uint8_t pin_code_extend);

void    TC_Write_Features(uint16_t feature, BOOL enable);
#if (PRH_BS_CFG_SYS_LMP_EXTENDED_SCO_SUPPORTED==1)
void    TC_Set_Accept_EV3_With_CRC_ERR(BOOL enable);
#endif

#if(PRH_BS_CFG_SYS_BROADCAST_NULL_IN_INQ_PAGE_SUPPORTED==1)
void TC_Set_Emergency_Poll_Interval(uint16_t emergency_poll);
#endif

void TC_Beken_Hardware_Test(uint8_t* pdu);
void TC_Non_Signal_Test_Control(uint8_t *test_control);

#endif


