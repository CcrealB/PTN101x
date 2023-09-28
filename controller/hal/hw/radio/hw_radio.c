/*************************************************************************
 *
 * MODULE NAME:    hw_radio.c
 * PROJECT CODE:   XpertBlue
 * DESCRIPTION:    Rohm Bluetooth Ver.2.0+EDR radio driver for Tabasco.
 * MAINTAINER:     Tom Kerwick
 * CREATION DATE:  07.09.07
 *
 * SOURCE CONTROL: $Id: hw_radio.c,v 1.30 2010/06/21 21:23:13 garyf Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2007 Ceva Inc.
 *     All rights reserved.
 *
 * RECENT REVISION HISTORY:
 *
 *  07.09.07 TK    initial reference version, untested.
 *  28.09.07 TK    modified for rohm bu9468muv radio, still untested.
 *  01.02.08 TK    modified for independent tx/rx setup times.
 *  25.01.11 BK    modified for BK3011
 *
 ********************************************************************************/

/********************************************************************************
 *      Included File
 ********************************************************************************/

#include "sys_config.h"
#include "sys_types.h"
#include "hw_radio.h"
#include "hw_hab_defs.h"
#include "hw_habanero.h" 
#include "hw_habanero_impl.h"
#include "hw_lc.h" 
#include "lslc_access.h"
#include "uslc_chan_ctrl.h"
#include "./include/hw_radio_defs.h" 
#include "sys_mmi.h"
#include "hw_delay.h"
#include "lmp_sco_container.h"
#include "bkreg.h"
#include "sys_init.h"
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
#include "bt_test.h"
#endif

#define USE_TIMED_TX_POWER_WRITE (1 && (PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1))
#define RECOVER_TO_STANDBY  1
#define ENABLE_SYMBOL_RECOVERY_BLOCK    0   /* #2921 */
#define TX_REPREPARE_ON_PKD_SUPPORTED 1
#define VERIFY_HW_RADIO 1

/********************************************************************************
 *      Define the times for GIO line transitions
 ********************************************************************************/

#define HW_RADIO_SLOT_TIME_POS_TX_START         (0)
#define HW_RADIO_SLOT_TIME_POS_TX_MID           (312)
#define HW_RADIO_SLOT_TIME_POS_RX_START         (625)
#define HW_RADIO_SLOT_TIME_POS_RX_MID           (937)
#define HW_RADIO_TOTAL_SLOT_TIME                (1250)
#define HW_RADIO_NORMAL_TIME_FOR_SPI_WRITE      (16)
#define HW_RADIO_USEC_WAIT_FOR_STABLE_RADIO_CLK (HW_RADIO_BLUE_RF_TIME_T_XTL)
#define HW_RADIO_MSEC_WAIT_FOR_STABLE_RADIO_CLK (HW_RADIO_BLUE_RF_TIME_T_XTL/1000)
#define HW_RADIO_POSITION_TIMED_SPI_WRITE_IDLE    (2) /* SER_ESER Position 2 */

/*************************************************************************************
 *
 *  The following is a defines the allocation of GIO lines in the Blue RF driver
 *
 * GIO 0     Used to change BDATA1 direction from receive to transmit.
 * GIO 1     Unused; for trx_mode
 * GIO 3     BXTLEN Enable/Disable Crystal. Sleep mode
 * GIO 2     Unused; for radio_on
 * GIO 4     BnPWR - Set low, places radio in lowest Power state. Config data is lost.
 * GIO 5     Unused; for slot_ctrl 
 * GIO 6     Combine with GIO 8 for Sync Tx pulse for BDATA1 when in page mode.
 * GIO 7     Unused
 * GIO 8     Creates Sync Tx pulse for combination with BDATA1.
 * GIO 9     BPKTCTL - In Transmit mode its a strobe to Enable the PA.
 *                     In receive the baseband generates this to  
 *                       indicate that the access code has been decoded.
 *                       
 * GIO 10     Unused
 * GIO 11     BPKTCTL combine for page mode.
 *
 *
 *************************************************************************************/





#define valid_frequency(FRAME_POS,MASK)    ((MASK & RADIO_FREQ_MASK(FRAME_POS)) != 0)


/********************************************************************************
 *      Local Variables
 ********************************************************************************/

static t_HWradio_Driver_Status radio_status = Power_Down_Mode_Not_Active;
t_RadioMode CurrentRadioMode =RADIO_MODE_STANDBY;
static t_RadioMode Requested_RadioMode = RADIO_MODE_STANDBY;
static uint8_t power_level_now = MAX_POWER_LEVEL_UNITS; /* radio default after reset */

uint32_t rawRSSIvalue=0x00; /* radio default after reset */
#ifdef BT_DUALMODE 
#if (PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1)
static uint8_t _HWradio_Timed_RSSI_Read_Enabled = FALSE;
#endif
//static uint8_t LE_Active_Mode = 1;
extern uint8_t le_mode;
extern uint16_t backup_Encryp_mode;
#endif

#if (TX_REPREPARE_ON_PKD_SUPPORTED==1)
//static uint8_t tx_prepared_on_tim2 = FALSE;
//static uint8_t tx_reprepare_on_pkd = FALSE;
#endif

#if (HW_RADIO_INDEPENDENT_TX_RX_SETUP_TIMES==1)
//static uint32_t jal_ser_eser_tx_times = 0;
//static uint32_t jal_ser_eser_rx_times = 0;
#endif
//const uint16_t Power_control_table[] = { 0x9713 /*-9dBm*/, 
//                                        0x862B /*-4.5 dBm*/,
//                                        0x86AF /* 0 dBm*/
//
//};


#if (PRH_BS_CFG_SYS_ENHANCED_POWER_CONTROL_SUPPORTED==1)
CONST int8_t Power_control_table_dBm[] = {
     -19, 12, -5, 0.3,   /* 1Mbits: # entries = MAX_POWER_LEVEL_UNITS+1*/
     -25, -20, -12, 0.1, 0.5, /* 2Mbits: # entries = MAX_POWER_LEVEL_UNITS_2MBITS+1  */
     -25, -20, -12, 0.1, 0.5, 0.7 /* 3Mbits: # entries = MAX_POWER_LEVEL_UNITS_3MBITS+1 */
};
#endif
extern t_SYS_Config g_sys_config;

/********************************************************************************
 *
 * T1 Hardware SER_MASK Configuration Limitations:
 *
 * To avoid glitches in ESER block, the SER_MASK is double-registered to the
 * ct_1m_clk in ESER block. So when write a new SER_MASK, must wait at least 3us
 * to read back, and the new SER_MASK also takes effect 3us later after writing
 * the new SER_MASK. As all other ESER CTRL signals share the same register map
 * location, writing to any of these signals, must wait for 3us to write new
 * SER_MASK, otherwise the new SER_MASK is not written in.
 *
 * To avoid potential scenarios of SER_MASK not written in, update register to
 * cache, and write to hardware once per radio service call only.
 *
 * As cached method is also MIPS improvement, adopt for all Hardware revisions.
 *
 ********************************************************************************/


/*************************************************************************************
 *  Prototypes and externs
 *************************************************************************************/
void _HWhab_Init_RF(void);
void HWradio_Initialise(void);
uint32_t _HWradio_ReadNow(uint32_t Reg_To_Read);
DRAM_CODE static BOOL HWradio_Service(t_RadioMode in_RadioMode, t_freq *io_Channels, t_radio_freq_mask freq_mask);
void HWradio_Init_Tx_Power_Level(uint8_t power_level);

/********************************************************************************
 *
 * JAL_SER_ESER_DATA:
 * POSITION1 | 0:tx start freq / rssi read  4:tx mid freq   8:rx start freq   12:rx mid freq
 * POSITION2 | 1:tx start idle              5:tx mid idle   9:rx start idle   13:rx mid idle
 * POSITION3 | 2:tx power                   6:              10:               14:rssi read
 * POSITION4 | 3:                           7:              11:               15:
 *
 ********************************************************************************/
#define JAL_SER_ESER_TX_START (0x0007)
#define JAL_SER_ESER_TX_MID   (0x0030)
#define JAL_SER_ESER_RX_START (0x0300)
#define JAL_SER_ESER_RX_MID   (0x3000)
#define JAL_SER_ESER_RSSI     (0x4000)
extern uint32_t XVR_reg_0x24_save;
extern t_bt_beken_config g_beken_initial_config;
//extern unsigned char BK3000_hfp_set_powercontrol(void);
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
extern inline BOOL is_pwr_ctl_tests(void);
#endif
DRAM_CODE void BK3000_Write_RF_CH(uint8_t channel_nb)
{
#if(PRH_BS_CFG_SYS_HOP_MODE != HOP_MODE_SINGLE_FREQ)
    uint32_t v;

    v = REG_XVR_0x24&0xffffff80;
	//LMscoctr_Get_Number_SYN_Connections
    {
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
    if(BTtst_Get_DUT_Mode() == DUT_DISABLED)
#endif
        {
            v = v&0xFFFFF0FF;
            v |= (XVR_reg_0x24_save&0x00000F00);
        }
    }
    channel_nb = channel_nb+2;
    v = v | (channel_nb);

    REG_XVR_0x24 = v;
#endif
}
DRAM_CODE  uint8_t BK3000_Read_RF_CH(void)
{
    return (REG_XVR_0x24 & 0x7f);
}
void Send_Receive_Command_Now(uint8_t channel_nb)
{
    BK3000_Write_RF_CH(channel_nb);
}

t_freq rf_channels_save[4];
DRAM_CODE void TIM0_TX_START_Radio_Process(void)
{
    if((RADIO_MODE_SLAVE_PAGE_RESP==CurrentRadioMode)||(RADIO_MODE_MASTER_DOUBLE_WIN==CurrentRadioMode))
    {
        BK3000_Write_RF_CH(rf_channels_save[TX_MID]);
    }
}

DRAM_CODE void TIM1_TX_Middle_Radio_Process(void)
{
    if((RADIO_MODE_SLAVE_PAGE_RESP==CurrentRadioMode)
       ||(RADIO_MODE_MASTER_DOUBLE_WIN==CurrentRadioMode)
       || (RADIO_MODE_TX_RX == CurrentRadioMode))
    {
        BK3000_Write_RF_CH(rf_channels_save[RX_START]);
    }
}

DRAM_CODE void TIM2_RX_START_Radio_Process(void)
{
    if(RADIO_MODE_MASTER_DOUBLE_WIN==CurrentRadioMode)
    {
        BK3000_Write_RF_CH(rf_channels_save[RX_MID]);
    }
}

DRAM_CODE void TIM3_RX_Middle_Radio_Process(void)
{
    if( (RADIO_MODE_SLAVE_PAGE_RESP==CurrentRadioMode)
        ||(RADIO_MODE_MASTER_DOUBLE_WIN==CurrentRadioMode)
        || (RADIO_MODE_TX_RX == CurrentRadioMode))
    {
        BK3000_Write_RF_CH(rf_channels_save[TX_START]);
    }
}

__INLINE__ void  SetRFSyncWindow(uint8_t win_size)
{
    uint32_t v;

    v=REG_XVR_0x24;

    //v=v&0xFFFF07FF;
    //v|=(win_size<<11);
    v = v& 0xffe0ffff;
    v|=(win_size<<16);

    REG_XVR_0x24=v;
}

/***************************************************************************
 *
 * FUNCTION: _HWradio_Go_To_Idle_State
 * PURPOSE:     Put Radio back to BLUE RF IDLE    state.
 *
 *            1. BnPWR -> GIO_4 Low -> Radio goes to OFF state.
 *
 *            2. BXTLEN -> GIO_3 Low
 *            3. BDATA1 ->  High
 *            4. BnPWR -> GIO_4 High -> Radio goes to PWRON state
 *
 *            5. Wait 10mS
 *            6. BXTLEN -> GIO_3 High
 *
 *            additional: Setup DA of Sapphire when enabled class1 transmit.
 *
 *    Note: See data sheet on HD157100NP and Sapphire
 *
 *        The use of HW Delay function:
 *        - Before the completion of gio setup and tabasco clock from radio
 *          is stabilized, HW delays are used with the pre calibrated value
 *          (SYS_HAL_PRECALIBRATED_DELAY). The calibration cannot be done
 *          before the PHY is configured since the Tabasco interrupts are used
 *          for the calibration. So it is required that tabasco clock is stabilized.
 *        - After tabasco clock is stabilized, the self-calibrating
 *          HW Delay function is initialised and the HW Delays are used for
 *          self diagnosis and immediate SPI read/write while radio initialisation.
 *
 ***************************************************************************/
void _HWradio_Go_To_Idle_State (void)
{
    /*
     * HW delays are used for self diagnosis and immediate SPI read/write
     * while radio initialisation. The calibration cannot be done
     * before the PHY is configured since the Tabasco interrupts are used
     * for the calibration. Here tabasco clock is stabilized in any system
     * clock scheme so that initialise the self-calibrating HW Delay function
     * at this point.
     */
    HWdelay_Initialise();

    HWradio_Init_Tx_Power_Level(MAX_POWER_LEVEL_UNITS);
}

/***********************************************************************
 *
 * FUNCTION:    _HWhab_Init_RF
 * PURPOSE:     Configure the SPI and GIO blocks.
 *
 *
 *
 ************************************************************************/
void _HWhab_Init_RF(void)
{
    /* Set Tx and Rx delays*/
    HW_set_tx_delay(mHWradio_TX_DELAY);
    HW_set_rx_delay(mHWradio_RX_DELAY);
}

/***************************************************************************
 * Radio HAL
 ***************************************************************************/

/***************************************************************************
 *
 * FUNCTION:   HWradio_Initialise
 * PURPOSE:    Initialises Habanero
 *
 ***************************************************************************/
void HWradio_Initialise(void)
{
    _HWhab_Init_RF();                        /* Set default register settings. */

    _HWradio_Go_To_Idle_State();            /* Power Up radio and then Radio needs to start in IDLE state.*/
    radio_status = Power_Down_Mode_Not_Active;
    CurrentRadioMode =RADIO_MODE_STANDBY;
    Requested_RadioMode = RADIO_MODE_STANDBY;
}

/***************************************************************************
 *
 * FUNCTION:   HWradio_Reset
 * PURPOSE:    Resets the Habanero core AND register map by writing special
 *             sequence to reset register
 *               Also resets radio registers.
 *
 *    NOTE:      Reset for Habanero implemented in core (hw_lc.c)
 ***************************************************************************/
/* void HWradio_Reset(void) */
/* { */
/* } */

/*************************************************************************************
 *
 * FUNCTION NAME: _HWradio_ReadNow
 *
 * Use SPI NOW mode to read a radio register. Due to MIPS consumptions on wait/polls
 * required in SPI NOW mode, this should be avoided where possible. Mainly suited to
 * radio initialisation.
 *
 *************************************************************************************/
uint32_t _HWradio_ReadNow(uint32_t Reg_To_Read)
{
    uint32_t val;
    val=*((uint32_t*)(Reg_To_Read));

    return(val);
}



/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_SetRadioMode
 *
 *************************************************************************************/
DRAM_CODE void HWradio_SetRadioMode(t_RadioMode in_RadioMode)
{
    Requested_RadioMode = in_RadioMode;

    if (Requested_RadioMode == RADIO_MODE_STANDBY)
    {
        HWradio_Service(Requested_RadioMode, NULL, 0);
    }
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_GetRadioMode
 *
 *************************************************************************************/
RAM_CODE t_RadioMode HWradio_GetRadioMode(void)
{
    return CurrentRadioMode;
}

RAM_CODE t_RadioMode HWradio_GetReqRadioMode(void)
{
    return Requested_RadioMode;
}
/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_SetFrequency
 * DESCRIPTION:
 *
 * PARAMETER:      None.
 *
 *
 * RETURNS :       None.
 *
 *
 * SPECIAL CONSIDERATIONS:  None.
 *
 *
 *************************************************************************************/
DRAM_CODE void HWradio_SetFrequency(t_freq *io_Channels, t_radio_freq_mask freq_mask)
{
    HWradio_Service(Requested_RadioMode, io_Channels, freq_mask);
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Service
 * DESCRIPTION:   This function is the main Radio service routine. It handles the radio
 *                driver state machine and the change from one mode to another. It is also
 *                called to set the frequencies for the next frame or frame fragment
 *                depending on the current mode.
 *
 * PARAMETER:      in_RadioMode - This is the requested mode for the driver.
 *                 in_Context - the is the device context which defines the current activity
 *                 io_Channels - This is a pointer to the channel settings. When a channel setting
 *                               is used, it is reset to be invalid.
 *
 * RETURNS :       None.
 *
 *
 * SPECIAL CONSIDERATIONS:  None.
 *
 *
 *************************************************************************************/
DRAM_CODE static BOOL HWradio_Service(t_RadioMode in_RadioMode, t_freq *io_Channels, t_radio_freq_mask freq_mask)
{
    BOOL lv_result =  TRUE;

#if (PRH_BS_CFG_SYS_HOP_MODE_EUROPE_USA_ONLY_SUPPORTED==0)
    /* French RF channel range 2454MHz + channel [0..22] */
    if (g_sys_config.hopping_mode==FRANCE_FREQ)
    {
        io_Channels[TX_START]+=52;
        io_Channels[TX_MID]+=52;
        io_Channels[RX_START]+=52;
        io_Channels[RX_MID]+=52;
    }
#endif

    if(in_RadioMode==RADIO_MODE_FULL_RX)
    {
        SetRFSyncWindow(0);
        //REG_XVR_0x3B = 0x09345088;
    }
	else if (in_RadioMode==RADIO_MODE_SLAVE_PAGE_RESP)
	{
		SetRFSyncWindow(0);  //Work around FHS Rx issue after ID received.
		//REG_XVR_0x3B = 0x09345488;
	}
    else
    {
    	//REG_XVR_0x3B = 0x09345488;
#ifdef BT_DUALMODE 
        SetRFSyncWindow(0x1F); //=====0x14
#else
        SetRFSyncWindow(0x14); //=====0x14
#endif
    }
#ifdef BT_DUALMODE 

    /*BLE diable GFSK M0 bits Threshold,avoid BLE receive mistake*/
    if(le_mode && (BTtst_Get_DUT_Mode()==DUT_DISABLED))
    {
        REG_XVR_0x3B &= 0xFFFFF1FF;
    }
#endif
    /* a double on the requested mode and the current mode */
    switch(in_RadioMode)
    {
        /************************/
        /* STANDBY MODE REQUEST */
        /************************/
    case RADIO_MODE_STANDBY :
        switch(CurrentRadioMode)
        {
        case RADIO_MODE_STANDBY :
        case RADIO_MODE_MASTER_DOUBLE_WIN :
        case RADIO_MODE_SLAVE_PAGE_RESP:
        case RADIO_MODE_TX_RX :
        case RADIO_MODE_FULL_RX:
        default:
            break;
        }
        CurrentRadioMode = RADIO_MODE_STANDBY;
        break;

        /************************/
        /* FULL RX MODE REQUEST */
        /************************/
    case RADIO_MODE_FULL_RX :
        switch(CurrentRadioMode)
        {
        case RADIO_MODE_STANDBY :
        case RADIO_MODE_TX_RX:
            /* Take the channel for  RX_START and start scan */
            if(valid_frequency(RX_START,freq_mask))
            {
                Send_Receive_Command_Now(io_Channels[RX_START]);
                CurrentRadioMode = RADIO_MODE_FULL_RX;
            }
            break;

        case RADIO_MODE_FULL_RX:
            /* if the frequency setting for the position  RX_START then assume a change in frequency is required */
            if(valid_frequency(RX_START,freq_mask))
            {
                Send_Receive_Command_Now(io_Channels[RX_START]);
            }
            break;

        case RADIO_MODE_MASTER_DOUBLE_WIN:
            /* Not allowed  go into standby first */
            lv_result = FALSE;
            break;

        default:
            /* Error */
            lv_result = FALSE;
            break;
        }

        break;

        /************************************/
        /* SLAVE PAGE RESPONSE MODE REQUEST */
        /************************************/
    case RADIO_MODE_SLAVE_PAGE_RESP:
        switch(CurrentRadioMode)
        {
        case RADIO_MODE_FULL_RX :
        case RADIO_MODE_SLAVE_PAGE_RESP:
            if (valid_frequency(TX_START, freq_mask))
            {
                rf_channels_save[TX_START]=io_Channels[TX_START];
            }
            /* If we have the SPI writes for the FHS Set them now  - for the second window */
            if (valid_frequency(RX_START, freq_mask))
            {
                rf_channels_save[TX_MID]=io_Channels[RX_START];
                rf_channels_save[RX_START]=io_Channels[RX_START];
            }
            break;

        default:
            lv_result = FALSE;
            break;
        }
        break;

        /************************/
        /* TX RX MODE REQUEST */
        /************************/
    case RADIO_MODE_TX_RX :
        switch(CurrentRadioMode)
        {
        case RADIO_MODE_STANDBY :
        case RADIO_MODE_FULL_RX :
        case RADIO_MODE_SLAVE_PAGE_RESP:
        case RADIO_MODE_MASTER_DOUBLE_WIN :
        case RADIO_MODE_TX_RX :
            if(valid_frequency(TX_START,freq_mask))
            {
                rf_channels_save[TX_START]=io_Channels[TX_START];
            }
            if(valid_frequency(RX_START,freq_mask))
            {
                /* Set the program words for Rx*/
                rf_channels_save[RX_START]=io_Channels[RX_START];
            }
            break;

        default:
            /* Not allowed  go into standby first */
            lv_result = FALSE;
            break;
        }
        break;

        /**********************************/
        /* MASTER DOUBLE WIN MODE REQUEST */
        /**********************************/
    case RADIO_MODE_MASTER_DOUBLE_WIN :
        switch(CurrentRadioMode)
        {
        case RADIO_MODE_TX_RX :
        case RADIO_MODE_STANDBY :
        case RADIO_MODE_MASTER_DOUBLE_WIN :
            if (valid_frequency(TX_START, freq_mask))
                rf_channels_save[TX_START]=io_Channels[TX_START];
            /************************************************ 
            *    comment 'else' by Charles for BQB 8/19/2015
            ************************************************/
            
            if (valid_frequency(TX_MID, freq_mask))
                rf_channels_save[TX_MID]=io_Channels[TX_MID];
            if (valid_frequency(RX_START, freq_mask))
                rf_channels_save[RX_START]=io_Channels[RX_START];
            if (valid_frequency(RX_MID, freq_mask))
                rf_channels_save[RX_MID]=io_Channels[RX_MID];
           
            break;

        case RADIO_MODE_FULL_RX :
            /* Not allowed  go into standby first */
            lv_result = FALSE;
            break;

        default:
            /* Not allowed  go into standby first */
            lv_result = FALSE;
            break;
        }
        break;

    default:
        break;
    }

    if (lv_result == TRUE )
        CurrentRadioMode = in_RadioMode;
#if (RECOVER_TO_STANDBY==1)
    else /* unexpected state: recover to radio mode standby */
    {
        CurrentRadioMode = RADIO_MODE_STANDBY;
    }
#endif

    return(lv_result);
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Setup_Radio_For_Next_Half_Slot
 *
 *************************************************************************************/
/* void HWradio_Setup_Radio_For_Next_Half_Slot(t_frame_pos next_slot_posn) */
/* { */
/* } */
#if (PRH_BS_CFG_SYS_LMP_POWER_CONTROL_SUPPORTED == 1)
extern uint8_t app_get_power_level(uint8_t index);
#define RF_LEVEL0_VAL   app_get_power_level(0)
#define RF_LEVEL1_VAL   app_get_power_level(1)
#define RF_LEVEL2_VAL   app_get_power_level(2)

/*************************************************************************************
 *
 * FUNCTION NAME: WRITE_BK3000_TX_POWER
 *
 *set transmitter power (0 to MAX_POWER_LEVEL_UNITS)
 *
 * 0x9713 *-9dBm*
 * 0x862B *-4.5 dBm*,
 * 0x86AF * 0 dBm*
 *************************************************************************************/
RAM_CODE void WRITE_BK3000_TX_POWER(uint8_t power_level)
{

    if(0 == g_beken_initial_config.g_Enable_TX_Power_Control)
    {
        return;
    }
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
{
    uint32_t regv_0x24;
    uint8_t power_val;
    g_beken_initial_config.g_Enable_TX_Power_Control = 0;
    
    switch(power_level)
    {
    case RF_POWER_LEVEL0:
        power_val = RF_LEVEL0_VAL;
        break;

    case RF_POWER_LEVEL1:
        power_val = RF_LEVEL1_VAL;
        break;

    default:
        power_val = RF_LEVEL2_VAL;
        break;
    }
    regv_0x24 = REG_XVR_0x24 & 0xFFFFF0FF;
    regv_0x24 = regv_0x24 | (power_val << 8);
    
    REG_XVR_0x24 = regv_0x24;
}
#endif
    //os_printf("<<<>%s, 0x%x-%d\r\n", __func__, power_val, power_level);
}
#if (CONFIG_CTRL_BQB_TEST_SUPPORT == 1)
RAM_CODE uint8_t GET_BK3000_POWER_LEVEL( void )
{
	uint32_t power_val = (REG_XVR_0x24 & (~0xFFFFF0FF)) >> 8;//0x00001F00
	uint8_t power_level = 0;
    
	if( power_val > RF_LEVEL1_VAL )
	{
		power_level = RF_POWER_LEVEL2;
	}
	else if( power_val > RF_LEVEL0_VAL )
	{
		power_level = RF_POWER_LEVEL1;
	}
	else
	{
		power_level = RF_POWER_LEVEL0;
	}
    INFO_PRT("Power Ctrl:0x%x-0x%x\r\n",  power_val, power_level);

	return power_level;
}
#endif
#endif

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Init_Tx_Power_Level
 *
 * Initialises the radio power level - uses abstract representation of
 * transmitter power (0 to MAX_POWER_LEVEL_UNITS)
 *
 *************************************************************************************/
void HWradio_Init_Tx_Power_Level(uint8_t power_level)
{
    WRITE_BK3000_TX_POWER(power_level);
    power_level_now = power_level;
}

#if (PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1)
/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Inc_Tx_Power_Level
 *
 *************************************************************************************/
t_error HWradio_Inc_Tx_Power_Level(void)
{
    if (power_level_now != MAX_POWER_LEVEL_UNITS)
    {
        power_level_now++;
        /* Immediate SPI write for Tx power: not recommended */
        WRITE_BK3000_TX_POWER(power_level_now);
        return NO_ERROR;
    }
    else
    {
        return COMMAND_DISALLOWED;
    }
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Dec_Tx_Power_Level
 *
 *************************************************************************************/
t_error HWradio_Dec_Tx_Power_Level(void)
{
    if (power_level_now != 0)
    {
        power_level_now--;

        /* Immediate SPI write for Tx power: not recommended */
        WRITE_BK3000_TX_POWER(power_level_now);
        return NO_ERROR;
    }
    else
    {
        return COMMAND_DISALLOWED;
    }
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Set_Tx_Power_Level
 *
 * Set the radio power level - uses abstract representation of
 * transmitter power (0 to MAX_POWER_LEVEL_UNITS)
 *
 *************************************************************************************/
RAM_CODE void HWradio_Set_Tx_Power_Level(uint8_t power_level)
{
    if(power_level != power_level_now)
    {
        power_level_now = power_level;
#if (USE_TIMED_TX_POWER_WRITE==0)
        /* Immediate SPI write for Tx power: not recommended */
#else
        /* Set timed SPI write of the Tx power level based on next TIM0 */
#endif
        WRITE_BK3000_TX_POWER(power_level);
    }
}

/*************************************************************************************
 *
 * FUNCTION NAME:  HWradio_Update_Internal_RSSI_Cache
 *
 * Opportunities provided by core for radio driver to update RSSI to cache.
 * Early read opportunity: sync_detection.
 * Later read opportunity: pkd processing.
 * Should return TRUE only if RSSI value was read into cache.
 *
 * For timed SPI read only need to read SPI read register, as the SPI read
 * transaction should have occured during the packet header reception.
 *
 *************************************************************************************/
RAM_CODE BOOL HWradio_Update_Internal_RSSI_Cache(BOOL late_read_opportunity)
{
    if (((PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1) &&
         (CurrentRadioMode == RADIO_MODE_TX_RX)) ||
        ((PRH_BS_CFG_SYS_LMP_RSSI_INQUIRY_RESULTS_SUPPORTED==1) &&
         (CurrentRadioMode == RADIO_MODE_MASTER_DOUBLE_WIN)))
    {
#if 0
        if( (!late_read_opportunity) && (BUILD_TYPE!=UNIT_TEST_BUILD))
        {
            rawRSSIvalue = REG_XVR_0x12&0xff;
        }
        /* return TRUE if RSSI read and value validated */
        return (late_read_opportunity && rawRSSIvalue);
#endif
    }
#if(LMP_RSSI_DEVICE_ASSESSMENT == 1)
    if(late_read_opportunity)   
        rawRSSIvalue=REG_XVR_0x12&0xff; 
    return late_read_opportunity;
#endif 

    return FALSE;
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Read_RSSI
 *
 * converts raw RSSI value to a dBm value (signed 8 bit)
 *
 *
 * [7:0]||power
 * -----||-----
 *   15 || -80
 *   35 || -60
 *   55 || -40
 *   75 || -20
 *   95 ||   0
 *
 *************************************************************************************/
int8_t HWradio_Read_RSSI(void)
{
    int16_t tmprssivalue;

    tmprssivalue=rawRSSIvalue;
    tmprssivalue=tmprssivalue-255;

    return (int8_t)(tmprssivalue);

}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Read_Background_RSSI
 *
 *************************************************************************************/
int8_t HWradio_Read_Background_RSSI(void)
{
#ifndef CONFIG_EN_AFH_FUNC
    uint32_t val=0;
    return (int8_t)(-255 + ((int8_t)((val>>8)&0xff)));
#else
	//int8_t tem = HWradio_Read_RSSI();
	uint32_t tem = REG_XVR_0x12&0xff;
	//os_printf("Read RSSI:%d\r\n",tem);
	return (int8_t )( tem - 255);
#endif
}
#endif

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Enter_Low_Power_Mode_Request
 *
 *************************************************************************************/
void HWradio_Enter_Low_Power_Mode_Request(void)
{
    radio_status = Power_Down_Mode_Active;

    /* set XTLEN low to disable the radio clock */
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Exit_Low_Power_Mode_Request
 *
 *************************************************************************************/
DRAM_CODE void HWradio_Exit_Low_Power_Mode_Request(void)
{
    radio_status = Power_Down_Mode_Not_Active;

    /* set XTLEN high to enable the radio clock */
    /* must wait stable clk from radio to clock tabasco after waking up */
}

#if 0
/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Enter_Low_Power_Mode_Forced
 *
 *************************************************************************************/
void HWradio_Enter_Low_Power_Mode_Forced(void) 
{ 
} 
#endif

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Get_Radio_Driver_Status
 *
 *************************************************************************************/
DRAM_CODE t_HWradio_Driver_Status HWradio_Get_Radio_Driver_Status(void)
{
    return(radio_status);
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Get_Radio_Register
 *
 *************************************************************************************/
uint32_t HWradio_Get_Radio_Register(uint32_t reg)
{
    return  *( (uint32_t*)(reg+MDU_XVR_BASE_ADDR));
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Set_Radio_Register
 *
 *************************************************************************************/
void HWradio_Set_Radio_Register(uint32_t reg, uint32_t val)
{
    *( (uint32_t*)(reg+MDU_XVR_BASE_ADDR)) =val;
}


uint8_t HWradio_Convert_Tx_Power_to_Tx_Power_Level_Units(int8_t power_level)
{
    if (power_level >= -2)
        return MAX_POWER_LEVEL_UNITS;
    else if ((power_level > -7) && (power_level < -2))
        return 1;
    else
        return 0;
}

int8_t HWradio_Convert_Tx_Power_Level_Units_to_Tx_Power(uint8_t power_level)
{
    if (power_level == MAX_POWER_LEVEL_UNITS)
    {
        return 0;
    }
    else if (power_level == 1)
    {
        return -5;
    }
    else //    if (power_level == 0)
    {
        return -9;
    }
}
/***************************************************************************
 *
 * FUNCTION:    HWradio_Set_Syncword
 *
 ***************************************************************************/
DRAM_CODE void HWradio_Set_Syncword(t_syncword const syncword)
{
    HWhab_Set_Syncword_Ref((uint32_t const*)&syncword);
}

#ifdef BT_DUALMODE 
#if (PRH_BS_CFG_SYS_LE_CONTROLLER_SUPPORTED==1)

/************************************************************************************* 
 * 
 * FUNCTION NAME: HWradio_LE_RxComplete 
 * 
 *************************************************************************************/ 
void HWradio_LE_RxComplete(void)
{
	HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
}

/************************************************************************************* 
 * 
 * FUNCTION NAME: HWradio_LE_TxComplete 
 * 
 *************************************************************************************/ 
//void HWradio_LE_TxComplete(void)                          
//{ 
//    //HWdelay_Wait_For_us(15+mHWradio_TX_DELAY);          
//}                                                         

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Setup_For_TIFS_Event
 *
 *************************************************************************************/
void HWradio_Setup_For_TIFS_Event(uint8_t io_Channel)
{
    BK3000_Write_RF_CH(io_Channel);
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_LE_Service_TestMode
 *
 *************************************************************************************/
void HWradio_LE_Service_TestMode( uint8_t io_Channel,  uint8_t pkt_Size)
{
    HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
    BK3000_Write_RF_CH(io_Channel);
	CurrentRadioMode = RADIO_MODE_LE_TESTMODE_TX;
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_LE_Service
 * DESCRIPTION:   This function is the main Radio service routine. It handles the radio
 *                driver state machine and the change from one mode to another. It is also
 *                called to set the frequencies for the next frame or frame fragment
 *                depending on the current mode.
 *
 * PARAMETER:      in_RadioMode - This is the requested mode for the driver.
 *                 in_Context - the is the device context which defines the current activity
 *                 io_Channels - This is a pointer to the channel settings. When a channel setting
 *                               is used, it is reset to be invalid.
 *
 * RETURNS :       None.
 *
 *
 * SPECIAL CONSIDERATIONS:  None.
 *
 *
 *************************************************************************************/
BOOL HWradio_LE_Service(t_RadioMode in_RadioMode, uint8_t io_Channel, t_radio_freq_mask freq_mask)
{
	BOOL lv_result =  TRUE;

	/* a double on the requested mode and the current mode */
	switch(in_RadioMode)
	{

		case RADIO_MODE_LE_FULL_RX :

			switch(CurrentRadioMode)
			{
				case RADIO_MODE_STANDBY :
				case RADIO_MODE_LE_TIFS_TX_RX :
				case RADIO_MODE_LE_FULL_RX:
						/* Take the channel for  RX_START and start scan */

					    //_HWradio_LE_DisableSpiWritesExceptTXRXandNOW();

						/* Set the Hab receive mode to full window */
						HWhab_Set_Rx_Mode(HWradio_RXm_FULL_WIN);

						/* set up the gios for full rx */
						//_HWradio_SetGios_FullRx();

						/* set up the gios for first Tx on exit out of Full Rx
						 * This first Tx will be performed based on the TIFs count.
						 */

						//_HWradio_Send_Idle_Now();
						// GF 16 Nov - Changed 12 t0 4;
                        BK3000_Write_RF_CH(io_Channel);
  					    CurrentRadioMode = RADIO_MODE_LE_FULL_RX;
					break;

				default :
					lv_result = FALSE; // not allowed
					break;
			}

			break;

		case RADIO_MODE_LE_INITIAL_RX :

			switch(CurrentRadioMode)
			{

			case RADIO_MODE_STANDBY :
				if(valid_frequency(RX_START,freq_mask))
				{

					/* Set the Hab receive mode to full window */
					HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
                    BK3000_Write_RF_CH(io_Channel);
					CurrentRadioMode = RADIO_MODE_LE_INITIAL_RX;
				}
				break;

				default:
					lv_result = FALSE; // not allowed
					break;
			}
			break;

		case RADIO_MODE_LE_INITIAL_TX:

			switch(CurrentRadioMode)
			{
				case RADIO_MODE_STANDBY :
				case RADIO_MODE_LE_TIFS_TX_RX :

					/* Setup Hab for the normal receive mode */

					HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);

				case RADIO_MODE_LE_INITIAL_TX :

					/* Set the program words for tx */
					if(valid_frequency(TX_START,freq_mask))
					{

						HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
                        BK3000_Write_RF_CH(io_Channel);
					    //_HWradio_ProgNow(WRITE_REG0_TRANSMIT + ((io_Channel + 2)<<1));

    				}
    				CurrentRadioMode = RADIO_MODE_LE_INITIAL_TX;

					break;

				default:
					lv_result = FALSE; // not allowed
					break;
			}
			break;

		case RADIO_MODE_LE_TESTMODE_TX:
				switch(CurrentRadioMode)
				{

				case RADIO_MODE_STANDBY :
				case RADIO_MODE_LE_TIFS_TX_RX :

	                /* Setup Hab for the normal receive mode */

				    HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);

				case RADIO_MODE_LE_TESTMODE_TX:
				case RADIO_MODE_LE_INITIAL_TX :

				{

					/* Set the program words for tx */
					if(valid_frequency(TX_START,freq_mask))
					{
						//_HWradio_LE_DisableSpiWritesExceptTXRXandNOW();

					    HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);

                        BK3000_Write_RF_CH(io_Channel);
                        //_HWradio_ProgNow(WRITE_REG0_TRANSMIT + ((io_Channel + 2)<<1));

					}
    				CurrentRadioMode = RADIO_MODE_LE_TESTMODE_TX;
				}
				break;
				default :
					break;

				}
			break;
			
		case RADIO_MODE_LE_TESTMODE_RX :

			switch(CurrentRadioMode)
			{
				case RADIO_MODE_STANDBY :
				case RADIO_MODE_LE_TESTMODE_RX:
					/* Take the channel for  RX_START and start scan */


					/* Set the Hab receive mode to full window */
					HWhab_Set_Rx_Mode(HWradio_RXm_FULL_WIN);

					/* set up the gios for full rx */

					/* set up the gios for first Tx on exit out of Full Rx
					 * This first Tx will be performed based on the TIFs count.
					 */

                    BK3000_Write_RF_CH(io_Channel);
					//_HWradio_ProgNow(WRITE_REG0_RECEIVE + ((io_Channel + 2)<<1));

					CurrentRadioMode = RADIO_MODE_LE_TESTMODE_RX;
					break;

				default :
					lv_result = FALSE; // not allowed
					break;
			}

			break;

		case RADIO_MODE_LE_TIFS_TX_RX :

			HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
			CurrentRadioMode = RADIO_MODE_LE_TIFS_TX_RX;
			break;


		/************************/
		/* STANDBY MODE REQUEST */
		/************************/

		case RADIO_MODE_STANDBY :

			switch(CurrentRadioMode)
			{

				case RADIO_MODE_STANDBY : 
					/*
					 *	Do nothing if we are already in Standby mode.
					 */
					break;

		    	case RADIO_MODE_LE_TIFS_TX_RX :
					/* Note, the following should only occur from a PKA */

		    	case RADIO_MODE_LE_INITIAL_TX :
				case RADIO_MODE_LE_FULL_RX:

					/* Note that the BPKTCTL pulse should have already been generated */
					/* Create our own BPKTCTL pulse  */

					/* Setup the GIO lines for Standby */
					/* Set the Hab Rx mode to normal */
					HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
																				
					break;


				default:
					break;
			}
			CurrentRadioMode = RADIO_MODE_STANDBY;

			break;

		default:
			lv_result = FALSE; // not allowed
			break;
	}

	if (lv_result == TRUE )
		CurrentRadioMode = in_RadioMode;
#if (RECOVER_TO_STANDBY==1)
    else /* unexpected state: recover to radio mode standby */
    {
		HWhab_Set_Rx_Mode(HWradio_RXm_NORMAL);
        CurrentRadioMode = RADIO_MODE_STANDBY;
    }
#endif

	return(lv_result);
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_LE_Setup_Radio_For_Next_TXRX
 *
 *************************************************************************************/
/*void HWradio_LE_Setup_Radio_For_Next_TXRX(t_frame_pos const next_slot_posn)*/
/*{                                                                          */
/*	//Configure_LE_Spi_For_TxRx_Times();                                     */
/*    return;                                                                */
/*}                                                                          */       

#endif

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_LE_Read_RSSI
 *
 *************************************************************************************/
int8_t HWradio_LE_Read_RSSI(void)
{
#if (PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1)
    return(HWradio_Read_RSSI());  
#else
    return 0;
#endif

#if 0
#if (CHAR_BIT==16)
	return (int8_t)(-95 + ((int8_t)(0xff00|((_HWradio_ReadNow(READ_REG(3))>>8)&0xff))));
#else
	return (int8_t)(-95 + ((int8_t)((_HWradio_ReadNow(READ_REG(3))>>8)&0xff)));
#endif
#endif
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Switch_To_LE_Mode
 *
 *************************************************************************************/
void HWradio_Switch_To_LE_Mode(void)
{
    le_mode = 0x01;
    HW_set_slave(1);//for BLE slave only
    HWle_set_le_mode();
    HWle_clear_adv_state();
    HW_set_page(0);
    HWhab_Set_Sync_Error(0x2);
    HW_set_win_ext(32);

    HWradio_SetRadioMode(RADIO_MODE_STANDBY);
    HW_set_rx_mode(0x00);
    HW_set_tx_mode(0x00);
    HWle_clear_tx_enable();
    HWle_abort_tifs_count();
    HWle_clear_tifs_default();
    backup_Encryp_mode = HW_get_encrypt();
    HW_set_encrypt(0);
    
    /* fully decouple LE power levels from BT Classic power levels  */
    HWradio_Init_Tx_Power_Level(MAX_POWER_LEVEL_UNITS);
    HW_set_whiten(1);
    HWle_set_whitening_enable();
    
#if (PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1)
    _HWradio_Timed_RSSI_Read_Enabled = FALSE;
#endif
}

/*************************************************************************************
 *
 * FUNCTION NAME: HWradio_Switch_To_Classic_BT_Mode
 *
 *************************************************************************************/
void HWradio_Switch_To_Classic_BT_Mode(void)
{
    le_mode = 0;
	//_HWradio_ProgNow(WRITE_REG1);

	HWhab_Set_Sync_Error(0x7);
		
	HW_set_win_ext(40); // note - resets to PRH_BS_CFG_SYS_DEFAULT_WIN_EXT on sync_det

    HW_set_add_bt_clk_relative(1);

	//HWle_clear_le_spi_only();

    // Abort the TIFS count
	HWle_abort_tifs_count();
	HWle_clear_tifs_default();

	// Turn off TX and RX
	// GF 17 Aug 2014
    HW_set_tx_mode(0x00);
    HW_set_rx_mode(0x00);
	HWle_clear_tx_enable();

 	HWle_clear_le_mode();
	HWradio_LE_Service(RADIO_MODE_STANDBY,0,0);

#if (PRH_BS_CFG_SYS_LMP_RSSI_SUPPORTED==1)
	_HWradio_Timed_RSSI_Read_Enabled = FALSE;
#endif

}
#if 0
void HWradio_LE_Set_Active_Mode(uint8_t mode)
{
	LE_Active_Mode = mode;
}
#endif
/*-------------------DualMode use only--------------------*/
RAM_CODE void HWradio_Setup_cur_cfg_win(uint8_t cur_cfg_win)//bit 11~15 charles modify auto sync win 0 low thodhlod
{
    uint32_t v;

    v = REG_XVR_0x24;
    v = v & 0xffe0ffff;
    v |= (cur_cfg_win<<16);

    REG_XVR_0x24 = v;
}
/*------------------DualMode use only--------------------*/
#endif

#if (BT_DUALMODE_RW == 1)
//set Baseband RF arbitor mode, 0x00:auto, 0x01:CEVA, 0x02:RW
void HWradio_Setup_bb_sel_rf(uint8_t value)
{
    if(value == 0x0)
    {
        REG_XVR_0x2A &= ~0x1E000000;   //baseband arbiter automatic mode
    }
    else if(value == 0x01)
    {
        REG_XVR_0x2A &= ~0x1E000000;   
        REG_XVR_0x2A |=  0x14000000;   //baseband arbiter manual mode: CEVA only
    }
    else if(value == 0x02)
    {
        REG_XVR_0x2A &= ~0x1E000000;  
        REG_XVR_0x2A |=  0x12000000;   //baseband arbiter manual mode: RW only
    }
}
#endif
