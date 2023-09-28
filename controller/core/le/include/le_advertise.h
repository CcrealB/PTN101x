
#ifndef _CEVA_LE_ADVERTISE__
#define _CEVA_LE_ADVERTISE__

/*********************************************************************
 * MODULE NAME:     le_advertise.h
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
typedef struct t_adv_params {
	uint32_t min_interval;
	uint32_t max_interval;
	uint8_t  adv_type;
	uint8_t  adv_add_type;
	uint8_t  init_add_type;
	//uint8_t* p_adv_add;
	uint8_t* p_init_add;
	uint8_t  channel_map;
	uint8_t  filter_policy;
} t_adv_params;

t_error LEadv_Set_Advertising_Params(t_p_pdu p_pdu);
uint8_t LEadv_Get_Advertising_Channel_Tx_Power(void);
t_error LEadv_Set_Advertising_Data(uint8_t length,t_p_pdu p_pdu);
t_error LEadv_Set_Scan_Response_Data(uint8_t length,t_p_pdu p_pdu );
t_error LEadv_Set_Advertise_Enable(uint8_t advertise_enable);
void LEadv_Adverting_Event_Begin(uint32_t current_clk);
uint8_t LEadv_Get_Advertising_Channel_Index(void);
t_error LEadv_Init(void);
uint8_t LEadv_Get_Advertising_Own_Address_Type(void);
void LEadv_Disable_Connectable_Advertising(void);
uint8_t LEadv_Get_Advertising_Channel_Index(void);
uint8_t LEadv_Get_Scan_Response_Data_Len(void);
uint8_t LEadv_Get_Advertising_Filter_Policy(void);
void LEadv_Set_Adv_Header(uint8_t pdu_type,uint8_t rxAddr);
void LEadv_Encode_Own_Address_In_Payload(uint8_t* p_payload);
uint8_t LEadv_Write_Adv_Data(uint8_t* p_payload);
uint8_t LEadv_Write_Scan_Resp_Data(uint8_t* p_payload);
uint8_t LEadv_Is_Next_Adv_Due(uint32_t current_clk);
void LEadv_Advertising_Event_Begin(uint32_t current_clk);
uint32_t LEadv_Get_Slots_To_Next_Advertising_Timer(uint32_t current_clk);
t_error LEadv_Handle_Advertising(uint8_t IRQ_Type,uint32_t current_clk);
uint8_t LEadv_Get_Advertising_Enable(void);
uint8_t LEadv_Get_Adv_Channel_Map(void);
uint8_t LEadv_Get_Adv_Type(void);
#if (PRH_BS_CFG_SYS_LE_CSA==4)
void LEadv_GAP_Update_Adv_Interval(uint16_t interval_min, uint16_t interval_max);
#endif
void LEadv_Prep_For_LE_Scan_Resp_Tx(void);
void LEadv_Prep_For_LE_Advert_Tx(void);
void LEadv_Reset_Adv_Frequency(void);
void LEadv_Adjust_Advertising_Event_Start_Timer(uint32_t num);
void LEadv_TCI_Set_Direct_Adv_Timeout(uint16_t timeout);
t_error LEadv_TCI_Write_Advertising_Delta(uint8_t delta);
t_error LEadv_TCI_Read_Advertising_Params(void);

uint8_t LEadv_Get_Direct_Adv_Type(void);
uint8_t* LEadv_Get_Direct_Adv_Address(void);
void Set_Dual_mode(uint8_t value);
uint8_t  Get_Dual_mode(void);
uint8_t Get_ADV_interval_count(void);

void LE_Set_Immediate_Next_Advertising_Event_Start_Timer(uint8_t reset_timer);
t_timer LE_Get_Next_ADV_Timer_value(void);


#if (PRH_BS_CFG_SYS_LE_GAP_INCLUDED==1)
t_error LEadv_Set_GAP_Advertising_Params(t_adv_params* p_adv_params);
uint8_t Get_LE_adv_data_len(void);

#endif
#endif
