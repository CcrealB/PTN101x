#ifndef _CEVA_LE_WHITE_LIST__
#define _CEVA_LE_WHITE_LIST__

/*********************************************************************
 * MODULE NAME:     le_white_list.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:     low Energy White List header file
 * MAINTAINER:      Gary Fleming
 * DATE:            August-2011
 * SOURCE CONTROL:
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2012 Ceva Inc.
 *     All rights reserved.
 *
 *********************************************************************/


typedef struct {

	uint8_t  address[6];
	uint8_t  address_type;
	uint8_t  used;
} t_ListEntry;


uint8_t LEwl_Read_White_List_Size(void);
t_error LEwl_Clear_White_List(void);
t_error LEwl_Add_Device_To_White_List(uint8_t address_type, t_p_pdu p_pdu);
t_error LEwl_Remove_Device_From_White_List(uint8_t address_type,t_p_pdu p_pdu);
uint8_t LEwl_Search_White_List(uint8_t address_type,const uint8_t* p_address);
uint8_t LEwl_address_compare(const uint8_t* addr1,const uint8_t* addr2);
uint8_t LEwl_Add_Device_To_Duplicate_List(uint8_t event_type,uint8_t address_type,const uint8_t* p_address);
void LEwl_Init_Duplicates_List(void);
uint8_t LEwl_Can_I_Modify_White_Lists(void);
#endif

