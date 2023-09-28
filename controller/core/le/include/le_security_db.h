#ifndef _CEVA_LE_SECURITY_DB__
#define _CEVA_LE_SECURITY_DB__

/*********************************************************************
 * MODULE NAME:     le_security_db.h
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
uint8_t LEsecdb_Get_IRK_BdAddr_Pairs(uint8_t role,uint8_t max_pairs,uint8_t** p_bd_addr_list,uint8_t**  p_IRK_list);
uint8_t LEsecdb_Add_DB_Entry(uint8_t address_type,uint8_t* p_address, uint8_t role,uint8_t entity,uint16_t* p_ediv, uint8_t* key);
uint8_t LEsecdb_Search_DB(uint8_t address_type, uint8_t* p_address, uint8_t role,uint8_t entity,uint16_t* p_ediv, uint8_t* key );
t_error LEsecdb_Add_Device_To_SecDB(uint8_t address_type, t_p_pdu p_pdu);
t_error LEsecdb_Clear_Security_DB(void);

#endif
#endif
