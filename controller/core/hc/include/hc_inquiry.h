#ifndef _PARTHUS_HC_INQUIRY_
#define _PARTHUS_HC_INQUIRY_

/**********************************************************************
 * MODULE NAME:    hc_inquiry.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    Host Controller Inquiry Functionality
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:  21 Jun 1999
 *
 * SOURCE CONTROL: $Id: hc_inquiry.h,v 1.15 2008/10/09 15:32:50 tomk Exp $
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
#include "lc_interface.h"


void HCinq_Initialise(void);

t_error HCinq_Inquiry_Start(t_lap, uint8_t, uint8_t);
t_error HCinq_Inquiry_Result(t_LC_Event_Info* eventInfo);

t_error HCinq_Inquiry_Complete(t_LC_Event_Info* eventInfo);
t_error HCinq_Inquiry_Cancel(void);

t_error HCinq_Periodic_Inquiry(uint16_t max_len, uint16_t min_len, t_lap lap, uint8_t inq_length, uint8_t max_resp);
t_error HCinq_Exit_Periodic_Inquiry_Mode(void);

typedef void (*t_hci_event_callback)(t_hci_event*);

#endif

