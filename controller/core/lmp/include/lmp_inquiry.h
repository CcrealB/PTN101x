#ifndef _PARTHUS_LM_INQUIRY_
#define _PARTHUS_LM_INQUIRY_

/**********************************************************************
 * MODULE NAME:    lmp_inquiry.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    Host Controller Inquiry Functionality
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:  21 Jun 1999
 *
 * SOURCE CONTROL: $Id: lmp_inquiry.h,v 1.10 2011/04/01 16:03:19 garyf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *    xx xxx 1999 -   gf       - Initial Version 
 *
 **********************************************************************/

#include "sys_types.h"
#include "hc_const.h"
#include "bt_fhs.h"
#include "hc_event_gen.h"
#include "lc_types.h"

typedef enum
{
   INQUIRY_IDLE = 1,
   W4_INQUIRY_RESULT,
   W4_INQUIRY_CANCEL_COMPLETE
} t_inquiry_states;

typedef enum
{
	LM_INQ_STANDARD_INQUIRY_RESULT = 0,
	LM_INQ_INQUIRY_RESULT_WITH_RSSI = 1,
    LM_INQ_EXTENDED_INQUIRY_RESULT = 2
} t_inquiry_result_mode;

void LMinq_Initialise(void);

t_error LMinq_Inquiry_Start(t_lap, uint8_t, uint8_t);
t_error LMinq_Inquiry_Result(t_LC_Event_Info* eventInfo);
t_error LMinq_Inquiry_Complete(t_LC_Event_Info* eventInfo);
t_error LMinq_Inquiry_Cancel(void);

t_error LMinq_Periodic_Inquiry(uint16_t max_len, uint16_t min_len, t_lap lap, uint8_t inq_length, uint8_t max_resp);
t_error LMinq_Exit_Periodic_Inquiry_Mode(void);
uint8_t LMinq_Get_Inquiry_Mode(void);
t_slots LMinq_Get_Interval_To_Periodic_Inq(void);
void LMinq_Inquiry_Resume(t_lmp_link* p_link);
void LMinq_Set_Suspend_Timer(uint8_t suspend_time);
typedef void (*t_hci_event_callback)(t_hci_event*);

#if (PRH_BS_CFG_SYS_LMP_RSSI_INQUIRY_RESULTS_SUPPORTED==1)
t_inquiry_result_mode LMinq_Get_Inquiry_Result_Mode(void);
void LMinq_Set_Inquiry_Result_Mode(t_inquiry_result_mode inquiry_result_mode);
#endif

#if (PRH_BS_CFG_SYS_LMP_EXTENDED_INQUIRY_LENGTH==1)
uint16_t LMinq_Read_Ext_Inquiry_Length();
t_error LMinq_Write_Ext_Inquiry_Length(uint16_t ext_Inq_len);
#endif

#endif

