#ifndef LMP_SSP_H
#define LMP_SSP_H

/******************************************************************************
 * MODULE NAME:    lmp_ssp.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:   
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:        
 *
 * SOURCE CONTROL: $ $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2009 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/


void LMssp_Generate_New_Public_Private_Key_Pair(BOOL blocking);
uint8_t LMssp_SSP_Enabled_On_Link(t_lmp_link* p_link);

t_error LMspp_LMP_IO_Capability_Response(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMspp_LMP_IO_Capability_Request(t_lmp_link* p_link, t_p_pdu p_pdu);

t_error LMssp_LMP_Encapsulated_Header(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_LMP_Encapsulated_Payload(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_LMP_Simple_Pairing_Confirm(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_LMP_Simple_Pairing_Number(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_LMP_DHkey_Check(t_lmp_link* p_link, t_p_pdu p_pdu);

t_error LMssp_LMP_Accepted(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_LMP_Not_Accepted(t_lmp_link* p_link, uint16_t return_opcode, t_error reason);
#if (PRH_BS_CFG_SYS_SSP_PASSKEY_SUPPORTED==1)
t_error LMssp_LMP_Passkey_Entry_Failed(t_lmp_link* p_link,t_p_pdu p_pdu);
#endif
#if (PRH_BS_CFG_SYS_SSP_OOB_SUPPORTED==1)
t_error LMssp_LMP_OOB_Failed(t_lmp_link* p_link, t_p_pdu p_pdu);
#endif
t_error LMssp_LMP_Numeric_Comparison_Failure(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_LMP_Keypress_Notification(t_lmp_link* p_link, t_p_pdu p_pdu);
t_error LMssp_Send_Keypress_Notification(t_lmp_link* p_link,uint8_t notification_type);

t_error LMssp_LM_IO_Capability_Request_Reply(t_lmp_link* p_link,uint8_t io_cap,uint8_t oob_data_present,uint8_t auth_requirements);
t_error LMssp_LM_IO_Capability_Request_Negative_Reply(t_lmp_link* p_link,t_error reason);
t_error LMssp_LM_User_Confirmation_Request_Reply(t_lmp_link* p_link);
t_error LMssp_LM_User_Confirmation_Request_Negative_Reply(t_lmp_link* p_link);
#if (PRH_BS_CFG_SYS_SSP_PASSKEY_SUPPORTED==1)
t_error LMssp_LM_User_Passkey_Request_Reply(t_lmp_link* p_link,uint32_t passKey);
t_error LMssp_LM_User_Passkey_Request_Negative_Reply(t_lmp_link* p_link);
#endif
#if (PRH_BS_CFG_SYS_SSP_OOB_SUPPORTED==1)
t_error LMssp_LM_Read_Local_OOB_Data(void);
t_error LMssp_LM_Remote_OOB_Data_Request_Reply(t_lmp_link* p_link,uint8_t* p_C, uint8_t* p_R);
t_error LMssp_LM_Remote_OOB_Data_Request_Negative_Reply(t_lmp_link* p_link);
#endif
t_error LMssp_LM_Write_SSP_Debug_Mode(uint8_t mode);
t_error LMssp_CallBack_DH_Key_Complete(t_lmp_link* p_link);
void LMssp_HCI_Timeout(t_lmp_link* p_link);
#endif
