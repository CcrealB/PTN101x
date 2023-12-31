#ifndef _PARTHUS_HW_RADIO_
#define _PARTHUS_HW_RADIO_

/******************************************************************************
 * MODULE NAME:    hw_radio.h
 * PROJECT CODE:   BlueStream
 * DESCRIPTION:    Generic RF/DSP hardware abstraction layer
 * MAINTAINER:     Ivan Griffin
 * DATE:           20 March 2000
 *
 * SOURCE CONTROL: $Id: hw_radio.h,v 1.72 2014/10/07 16:25:09 vladimirf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 ******************************************************************************/

#include "sys_config.h"
#include "lc_types.h"
#include "hw_macro_defs.h"

/***************************************************************************
 *
 * HWradio Typedefs
 *
 ***************************************************************************/
typedef enum
{
    HWradio_RXm_NORMAL = 0,
    HWradio_RXm_FULL_WIN = 1
} t_HWradio_Rx_Mode;

typedef enum
{
    Power_Down_Mode_Active,
    Power_Down_Mode_Not_Active
} t_HWradio_Driver_Status;

typedef uint8_t t_radio_freq_mask;

#define RADIO_FREQ_MASK(X)    ((t_radio_freq_mask)(1 << (X)))

#define TX_START_FREQ    RADIO_FREQ_MASK(TX_START)
#define TX_MID_FREQ        RADIO_FREQ_MASK(TX_MID)
#define RX_START_FREQ    RADIO_FREQ_MASK(RX_START)
#define RX_MID_FREQ        RADIO_FREQ_MASK(RX_MID)

typedef enum {
    RADIO_MODE_STANDBY,
    RADIO_MODE_FULL_RX,
    RADIO_MODE_MASTER_DOUBLE_WIN,
    RADIO_MODE_TX_RX,
    RADIO_MODE_SLAVE_PAGE_RESP,
    RADIO_MODE_LE_TIFS_TX_RX,
    RADIO_MODE_LE_INITIAL_TX,
    RADIO_MODE_LE_FULL_RX,
    RADIO_MODE_LE_INITIAL_RX,
    RADIO_MODE_LE_TESTMODE_TX,
	RADIO_MODE_LE_TESTMODE_RX
}t_RadioMode;

#if 0
typedef enum
{
    HWradio_SERV_RX = 0,
    HWradio_SERV_DISABLE_RX ,
    HWradio_SERV_TX,
    HWradio_SERV_DISABLE_TX ,
    HWradio_SERV_RX_TX
}t_RadioServiceType;
#endif
        
#define HW_RADIO_MAX_CHANNEL 80
#define HW_RADIO_SETTING_INVALID (HW_RADIO_MAX_CHANNEL+1)

/***************************************************************************
 *
 * HWradio Interface Functions
 *
 ***************************************************************************/
void HWradio_Initialise(void);
/* void HWradio_Reset(void); */
DRAM_CODE void HWradio_Set_Syncword(t_syncword const);

DRAM_CODE void HWradio_SetRadioMode(t_RadioMode in_RadioMode);
RAM_CODE t_RadioMode HWradio_GetRadioMode(void);
RAM_CODE t_RadioMode HWradio_GetReqRadioMode(void);

DRAM_CODE void HWradio_SetFrequency(t_freq *io_Channels, t_radio_freq_mask freq_mask);
/* void HWradio_Rx_PacketExtend(void); */
/* void HWradio_Tx_PacketExtend(void); */
/* void HWradio_TxComplete(void);  */
/* void HWradio_RxComplete(void); */ 
#ifdef BT_DUALMODE
void HWradio_LE_RxComplete(void); 
#endif
//void HWradio_LE_TxComplete(void); 
//void HWradio_LE_Set_Active_Mode(uint8_t mode); 
/* void HWradio_Setup_Radio_For_Next_Half_Slot(t_frame_pos frame_posn); */

RAM_CODE void HWradio_Set_Tx_Power_Level(uint8_t power_level);

uint32_t HWradio_Get_Radio_Register(uint32_t reg); 
void HWradio_Set_Radio_Register(uint32_t reg, uint32_t val);

int8_t HWradio_Read_RSSI(void); /* N == RSSI Return, -128 <= N <= 127 */
RAM_CODE BOOL HWradio_Update_Internal_RSSI_Cache(BOOL late_read_opportunity);
void HWradio_Enter_Low_Power_Mode_Request(void);
void HWradio_Exit_Low_Power_Mode_Request(void);
t_HWradio_Driver_Status HWradio_Get_Radio_Driver_Status(void);
/* void HWradio_Enter_Low_Power_Mode_Forced(void); */

#if (BUILD_TYPE==UNIT_TEST_BUILD)
uint8_t HWradio_Get_Last_Tx_Power_Level(void);
#endif

void HWradio_Trim_LPO(void);

void HWradio_Set_Tx_Power_Level(uint8_t power_level);

int8_t HWradio_Read_RSSI(void); /* N == RSSI Return, -128 <= N <= 127 */
int8_t HWradio_Read_Background_RSSI(void);

/* void HWradio_Setup_Radio_For_Next_Half_Slot(t_frame_pos frame_posn); */
RAM_CODE BOOL HWradio_Update_Internal_RSSI_Cache(BOOL late_read_opportunity);

uint8_t HWradio_Convert_Tx_Power_to_Tx_Power_Level_Units(int8_t power_level);
int8_t HWradio_Convert_Tx_Power_Level_Units_to_Tx_Power(uint8_t power_level);

/* void HWradio_Program_Dummy_Rx(uint8_t io_Channel); */
/* void HWradio_LE_Setup_Radio_For_Next_TXRX(t_frame_pos next_slot_posn); */
#ifdef BT_DUALMODE
int8_t HWradio_LE_Read_RSSI(void);
 BOOL HWradio_LE_Service(t_RadioMode in_RadioMode, uint8_t io_Channel, t_radio_freq_mask freq_mask); 

void HWradio_Switch_To_Classic_BT_Mode(void); 
 void HWradio_Switch_To_LE_Mode(void); 
#endif
t_error HWradio_Inc_Tx_Power_Level(void);
t_error HWradio_Dec_Tx_Power_Level(void);

void HWradio_Setup_For_TIFS_Event(uint8_t io_Channel); 
void HWradio_LE_Service_TestMode( uint8_t io_Channel,  uint8_t pkt_Size);
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
#if (PRH_BS_CFG_SYS_LMP_POWER_CONTROL_SUPPORTED == 1)
RAM_CODE uint8_t GET_BK3000_POWER_LEVEL(void);
RAM_CODE void WRITE_BK3000_TX_POWER(uint8_t power_level);
#endif
#endif

DRAM_CODE void BK3000_Write_RF_CH(uint8_t channel_nb);

#ifdef BT_DUALMODE
/*-----------------------DualMode use only-------------------------*/
RAM_CODE void HWradio_Setup_cur_cfg_win(uint8_t cur_cfg_win);//bit 11~15 /* charles modify */
/*-----------------------DualMode use only-------------------------*/
#endif
#endif

