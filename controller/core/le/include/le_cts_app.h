#ifndef _CEVA_LE_CTS_APP_H__
#define _CEVA_LE_CTS_APP_H__

#include "config.h"
#ifdef BT_DUALMODE
#define DEBUG_FOR_LOW_POWER 0
#define ENABLE_L2CAP_UPDATE_PARAMETER
//#define CONNECT_INTERVAL_300MS
//#define CONNECT_INTERVAL_100MS
//#define CONNECT_INTERVAL_1000MS


#define APP_FIND_ME    0
#define APP_HT_DEVICE  1
#define APP_HR_DEVICE  2
#define APP_DEFINE FIND_ME

#define APP_EQ_BLE_CONFIG  0

enum LE_FLAG_ENV
{
    LE_FLAG_LE_ENBLE = 0x01,
    LE_FLAG_ADDR_NAME_CONFIG_ENBLE = 0x02,   
    LE_FLAG_ADV_CONFIG_ENBLE = 0x04,
    LE_FLAG_UPDATE_CONFIG_ENBLE = 0x08,
};
/*********************************************************************
 * MODULE NAME:     le_cts_app.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:
 * MAINTAINER:      Gary Fleming
 * DATE:            June-2012
 * SOURCE CONTROL:
 *
 * LICENSE:
 *     This source code is copyright (c) 1999-2012 Ceva Inc.
 *     All rights reserved.
 *
 *********************************************************************/

typedef struct t_App_Control
{
	//uint8_t Flash_Type;// no used
	#if (APP_DEFINE == APP_FIND_ME)
	uint8_t Alert_Level;
	//uint32_t TAPP_Alerting_Timer;// no used
	#ifdef  BATTERY_DETECT
	uint32_t TAPP_BatteryDetect_Timer;
	#endif
	//uint32_t TAPP_DISCOVERABLE_Timer;//for connectable mode transfer to discoverable timer
	uint32_t TAPP_POWERON_STANDBY_Timer;
    uint32_t MMI_Timer[3];
	#endif

	uint32_t TAPP_L2CAP_PARA_UPDATE_Timer;
} t_App_Control;

/*-------------ble audio equalizer--------------*/
typedef struct
{
    uint16_t    a[2];
    uint16_t    b[3];
    uint16_t flag_enable;
}ble_aud_equ_t;
/*-------------ble audio equalizer--------------*/
#define LONG_BLE_INTERVAL   800
#define SHORT_BLE_INTERVAL  96

t_error LE_Simple_APP_Info(uint8_t para1,uint8_t para2,uint8_t para3);
t_error LE_CTS_APP_Info(uint8_t command,uint8_t command_val,uint8_t* p_command_params);
t_error LE_NCST_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_command_params);
t_error LE_HID_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);
t_error LE_HT_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);
t_error LE_HRS_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);
t_error LE_CSC_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);
t_error LE_RSC_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);
t_error LE_BP_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);
t_error LE_BG_APP_Info(uint8_t command,uint8_t command_len,uint8_t* p_pdu);

uint16_t App_Setup_GAP_GATT_DB(void);
uint16_t App_Setup_GATT_GATT_DB(void);
uint16_t App_Setup_DEV_INFO_GATT_DB(void);
uint16_t App_Setup_IAS_GATT_DB(void);
uint16_t App_Setup_TX_POWER_GATT_DB(void);
uint16_t App_Setup_LINK_LOSS_GATT_DB(void);
uint16_t App_Setup_BATTERY_SERVICE_GATT_DB(void);
uint16_t App_Setup_SCAN_PARAMETERS_GATT_DB(void);

uint16_t App_Setup_CURRENT_TIME_GATT_DB(void);
uint16_t App_Setup_REFERENCE_TIME_GATT_DB(void);
uint16_t App_Setup_NEXT_DST_GATT_DB(void);

uint16_t App_Setup_HID_GATT_DB(void);
uint16_t App_Setup_HRS_GATT_DB(void);
uint16_t App_Setup_HT_GATT_DB(void);
uint16_t App_Setup_CSC_GATT_DB(void);
uint16_t App_Setup_RSC_GATT_DB(void);
uint16_t App_Setup_BLOOD_PRESSURE_GATT_DB(void);
uint16_t App_Setup_BLOOD_GLUCOSE_GATT_DB(void);
uint16_t App_Setup_FFE0_GATT_DB(void);
uint16_t App_Setup_FFA0_GATT_DB(void);
void LE_App_Discover_Services_By_UUID_Event(uint8_t attrib_len, uint8_t value_payload_len, uint8_t* p_pdu);
void LE_App_Command_Complete(uint16_t handle,uint8_t operation_id,uint8_t status);
void LE_App_Discover_Chars_Event(uint8_t attrib_len, uint8_t value_payload_len, uint8_t* p_pdu);
uint8_t LE_App_Local_Event_Indication(uint16_t attrib_handle, uint8_t status);
uint8_t LE_App_Peer_Event_Indication(uint16_t attrib_handle, uint8_t status, uint8_t length, uint8_t* p_data);
void LE_App_Read_Response(uint8_t length,uint8_t* p_data);
uint8_t LE_App_Local_Disconnect_Indication(t_error reason);
void LE_App_Set_L2cap_Para_Update_Timer(uint32_t time);
void LE_App_Reset_L2cap_Para_Update_Timer(void);
void Trans_ADC_Battery_Cap(void);
void User_App_Check_Timers(void);
#ifdef BATTERY_DETECT
void Restart_Battery_Detect_Timer(void);
#endif
void LE_APP_Init(void);
#ifdef BATTERY_DETECT
void Save_Battery_level(void);
uint16_t Get_Battery(void);
void Save_Power_on_Battery(uint8_t value);
uint8_t Battery_level_to_Percentage(uint16_t level);
#endif


#if (APP_EQ_BLE_CONFIG == 1)
void EQ_test_RVCE_Param(uint8_t ch);
void EQ_Data_Slave_converter( uint8_t *pdu,ble_aud_equ_t *aud);
void Write_EQ_parm2_reg();

int atoH(uint8_t *c);

void aud_filt_enable_use_ble( uint8_t index, uint8_t enable );
int aud_filt_conf_use_ble( uint8_t index, ble_aud_equ_t *conf );
void EQ_Data_Print(void);

int16_t f_sat1(int din);
int16_t f_inv1(int din);
#endif //(APP_EQ_BLE_CONFIG == 1)
uint8_t app_ble_latency_pass_enable(void);

#endif

#endif
