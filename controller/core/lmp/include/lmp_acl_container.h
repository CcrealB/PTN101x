#ifndef _PARTHUS_ACL_CONTAINER_
#define _PARTHUS_ACL_CONTAINER_

/******************************************************************************
 * MODULE NAME:    lm_acl_container.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:   
 * MAINTAINER:     Gary Fleming
 * CREATION DATE:   
 *
 * SOURCE CONTROL: $Id: lmp_acl_container.h,v 1.23 2004/10/04 11:37:45 namarad Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 ******************************************************************************/

#include "sys_types.h"	
#include "lmp_sec_types.h"
#include "lmp_acl_connection.h"


typedef struct {
    uint8_t used;
    t_lmp_link entry;
} t_link_entry;
#if(HOST_A2DP_PLAYING_FLOW_CTRL == 1)
void LMaclctr_set_flow_ctrl_state(t_deviceIndex device_index,uint8_t flow_ctrl);
DRAM_CODE uint8_t LMaclctr_get_flow_ctrl_state(t_deviceIndex device_index);
uint8_t LMaclctr_is_flow_ctrl_state(void);
#endif

void LMaclctr_Initialise(void);
void LMaclctr_Free_Link(t_lmp_link* p_lmp_link);

t_lmp_link* LMaclctr_Alloc_Link(void);  
t_lmp_link* LMaclctr_Find_Bd_Addr(const t_bd_addr* p_bd_addr);
t_lmp_link* LMaclctr_Find_Bd_Addr_Check_State(const t_bd_addr *p_bd_addr);
t_lmp_link* LMaclctr_Find_Peer_Bd_Addr(const t_bd_addr* p_bd_addr);
t_lmp_link* LMaclctr_Find_Peer_Bd_Addr_Check_State(const t_bd_addr *p_bd_addr);
t_lmp_link* LMaclctr_Find_Handle(t_connectionHandle handle);
RAM_CODE t_lmp_link* LMaclctr_Find_Device_Index(t_deviceIndex device_index);
t_lmp_link* LMaclctr_Find_ACL_Handle(t_connectionHandle handle);
t_lmp_link* LMaclctr_Find_ARADDR(uint8_t ar_addr);
uint32_t Get_Active_Acl_Num(void);
uint32_t Get_exist_Acl_Num(void);

#endif

