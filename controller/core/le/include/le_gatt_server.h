#ifndef _CEVA_LE_GATT_SERVER__
#define _CEVA_LE_GATT_SERVER__

/*********************************************************************
 * MODULE NAME:     LE_gatt_server.h
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
/************************************************************************
 * Handles for Attributes and Characteristic in the Local GATT Server Database
 * This are hardcoded as the layout of our local GATT Database is know.
 *
 * NOTE !! If the handles in the local GATT Database are changed then
 * this values may also need to change.
 *************************************************************************/
/* HID */
#define HID_PROTOCOL_MODE_HANDLE            0x00D2
#define HID_BOOT_KEYBOARD_INPUT_HANDLE      0x00E1
#define HID_BOOT_MOUSE_INPUT_HANDLE         0x00E6
#define HID_BOOT_KEYBOARD_OUTPUT_HANDLE     0x00E4
#define HID_HID_INFO_HANDLE                 0x00E9
#define HID_REPORT_MAP_HANDLE               0x00DE
#define HID_REPORT_INPUT_HANDLE             0x00D4
#define HID_REPORT_OUTPUT_HANDLE            0x00D8
#define HID_REPORT_FEATURE_HANDLE           0x00DB
#define HID_CONTROL_POINT_HANDLE            0x00EB


/* Heart Rate Service */
#define HRS_MEASUREMENT_HANDLE               0x00F2
#define HRS_SENSOR_LOCATION_HANDLE           0x00F5
#define HRS_CONTROL_POINT_HANDLE             0x00F7

/* Health Thermometer */

#define HT_TEMP_MEASUREMENT_HANDLE           0x0102
#define HT_TEMP_TYPE_HANDLE                  0x0105
#define HT_INT_TEMP_MEASUREMENT_HANDLE       0x0107
#define HT_MEASUREMENT_INTERVAL_HANDLE       0x010A

/* Current Time and NDST */
#define CTS_SERVER_CURRENT_TIME_HANDLE                0x00A2
#define CTS_SERVER_LOCAL_TIME_HANDLE                  0x00A5
#define CTS_SERVER_REFERENCE_TIME_INFORMATION_HANDLE  0x00A7
#define NDCS_SERVER_TIME_HANDLE                       0x00C2

/* Battery and Tx Power */
#define BAS_SERVER_BATTERY_LEVEL_HANDLE               0x00A2
#define TPS_SERVER_TX_POWER_HANDLE                    0x0082
#define NOTIFY_PHONE_HANDLE                           0x00B2


/* Alerting */
#define LINK_LOSS_SERVER_IMMEDIATE_ALERT_HANDLE       0x0072
#define IAS_SERVER_IMMEDIATE_ALERT_HANDLE             0x0052
/* Scan Service */
#define SCPS_SERVER_SCAN_REFRESH_HANDLE               0x0094
#define SCPS_SERVER_SCAN_INTERVAL_HANDLE              0x0092


#define CSC_MEASUREMENT_HANDLE                        0x0112
#define CSC_SUPPORTED_FEATURES_HANDLE                 0x0115
#define CSC_SENSOR_LOCATION_HANDLE                    0x0117
#define CSC_SC_CONTROL_POINT_HANDLE                   0x0119

#define EQ_PARAM0_HANDLE                  		  0x00B2
#define EQ_PARAM1_HANDLE                  		  0x00B5
#define EQ_PARAM2_HANDLE                  		  0x00B8
#define EQ_PARAM3_HANDLE                  		  0x00BB
#define EQ_PARAM4_HANDLE                  		  0x00BE

t_error GATTserver_Init(void);

/***********************************************************************/
/* GATT Server Calls exposed via the API to the user
 * These are used to read/write values to the local Server.
 * For test purposes an additional call is included to allow the user
 * to change the permission's on a given attribute.
 *
 ***********************************************************************/

t_GATT_error  GATTserver_Characteristic_Read_Local_Value(uint16_t char_val_handle,uint8_t* p_len,/*const*/ uint8_t** p_value);
t_GATT_error  GATTserver_Characteristic_Write_Local_Value(uint16_t char_val_handle,uint16_t length, uint8_t* p_attrib_value);
uint8_t  GATTserver_Write_Permissions(uint16_t handle,uint8_t permissions);

/***********************************************************************/
/* GATT Server Calls used by the le_gap.c
 * These are used to find a reconnection address and read the privact flag
 * in the Server DB
 *
 ***********************************************************************/
uint8_t  GATTserver_Is_Privacy_Flag_Enabled(void);
uint8_t* GATTserver_Find_Reconnection_Address(void);
uint16_t GATTserver_Get_Reconnection_Address_Handle(void);


/***********************************************************************/
/* GATT Server Calls used by the le_gatt.c
 * These are used to Read/Write and Search the GATT Server database
 *
 ***********************************************************************/

uint8_t GATTserver_Read_By_Group_Type(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle,uint16_t attrib_UUID,uint8_t* p_result,uint8_t* p_result_len,uint8_t* pair_len);
uint8_t  GATTserver_Find_By_Type_And_Value(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle,
										  uint16_t attrib_type,uint16_t attrib_value,uint8_t* p_result,uint8_t* p_result_len);
uint8_t  GATTserver_Find_Included_Services(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle,uint8_t* p_result,uint8_t* p_result_len);
uint16_t Get_GATTserver_DB_Last_Handle(void);
uint8_t  GATTserver_Find_Descriptor_Information(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle,uint8_t* p_result,uint8_t* p_result_len,uint8_t* p_format);
uint8_t  GATTserver_Discover_All_Characteristics_Of_A_Service(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle,uint8_t* p_result,uint8_t* p_result_len, uint8_t* p_attrib_val_len);
uint8_t  GATTserver_Get_Value_Length(uint16_t handle);
//uint8_t  GATTserver_Read_Characteristic_Value_By_UUID(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle,uint16_t attrib_UUID,uint8_t* p_result,uint8_t* p_result_len,uint8_t* p_attrib_len);
uint8_t  GATTserver_Read_Characteristic_Value(uint16_t max_result_len,uint16_t att_handle, uint8_t* p_result,uint8_t* p_result_len);
uint8_t  GATTserver_Read_Long_Characteristic_Value_Offset(uint16_t max_result_len,uint16_t att_handle, uint16_t offset, uint8_t* p_result,uint8_t* p_result_len);
uint8_t GATTserver_Write_Command(uint16_t attrib_handle, uint8_t* p_attrib_value,uint16_t length);
uint8_t  GATTserver_Can_I_Write(uint16_t handle);
uint8_t  GATTserver_UUID_Match(t_UUID* attrib_UUID,uint8_t attrib_len,t_attrib_UUID* p_att_UUID);

uint8_t GATTserver_Read_Characteristic_Value_By_UUID(uint16_t max_result_len,uint16_t start_handle,uint16_t end_handle, t_attrib_UUID* p_att_UUID,uint8_t* p_result,uint8_t* p_result_len,uint8_t* p_attrib_len);

uint8_t _GATTserver_Add_Characteristic_Declaration(uint16_t handle,uint16_t char_UUID,uint8_t properties);
uint8_t _GATTserver_Add_16ByteUUID_Characteristic_Declaration(uint16_t handle, uint8_t* p_UUID,uint8_t properties);
uint8_t _GATTserver_Add_Service(uint8_t service_type,uint16_t handle,uint8_t* p_service_uuid,uint16_t service_uuid);
uint8_t _GATTserver_Add_Included_Service(uint16_t handle, uint16_t start_handle,uint16_t end_handle,uint16_t service_UUID);
uint8_t _GATTserver_Add_Characteristic_Value(uint16_t handle,uint16_t char_UUID,uint8_t local_notify,uint8_t permissions,uint8_t val_len,uint8_t* p_value);
uint8_t _GATTserver_Add_16ByteUUID_Characteristic_Value(uint16_t handle,uint8_t* char_UUID,uint8_t local_notify,uint8_t permissions,uint16_t val_len,uint8_t* p_value);
uint8_t _GATTserver_Add_Characteristic_Extended_Properties(uint16_t handle,uint8_t ext_properties);
uint8_t _GATTserver_Add_Characteristic_Format(uint16_t handle, uint16_t unit_UUID,t_char_format format, int8_t exponent, uint8_t n_sp, uint16_t description);
//uint8_t _GATTserver_Add_Characteristic_User_Description(uint16_t handle,uint8_t string_len,uint8_t* p_string,uint8_t permissions);
uint8_t _GATTserver_Add_Client_Characteristic_Description(uint16_t handle,uint8_t permissions,uint8_t* p_val);
uint8_t _GATTserver_Add_Characteristic_Propietrary_Descriptor(uint16_t handle,uint8_t UUID_len, uint8_t* p_UUID, uint8_t permissions,uint8_t* p_value);
//uint8_t _GATTserver_Add_Characteristic_Configuration(uint16_t handle);
//uint8_t _GATTserver_Add_Characteristic_External_Report_Reference(uint16_t handle,uint16_t external_UUID);
//uint8_t _GATTserver_Add_Characteristic_Report_Reference(uint16_t handle,uint8_t report_id,uint8_t report_ref);
uint8_t _GATTserver_Add_Char_Descriptor(uint16_t handle,uint16_t uuid,uint8_t permission,uint8_t val_len,uint8_t* p_value);
uint16_t Gatt_Get_MTU_Size(void);

#endif

#endif
