/**
****************************************************************************************
*
* @file rf_xvras.c
*
* @brief Atlas radio initialization and specific functions
*
* Copyright (C) Beken 2009-2015
*
* $Rev: $
*
****************************************************************************************
*/

/**
****************************************************************************************
* @addtogroup RF_BK
* @ingroup RF
* @brief Radio Driver
*
* This is the driver block for radio
* @{
****************************************************************************************
*/

/**
 *****************************************************************************************
 * INCLUDE FILES
 *****************************************************************************************
 */
#include "reg_ipcore.h"        // DM core registers
#include "rwip_config.h"        // RW SW configuration
#include <string.h>             // for memcpy
#include "co_utils.h"           // common utility definition
#include "co_math.h"            // common math functions
#include "rw_rf.h"                 // RF interface
////#include "plf.h"                // Platform functions
#include "rwip.h"               // for RF API structure definition
#include "em_map.h"
	////#include "reg_ipcore.h"        // DM core registers

#if (BLE_EMB_PRESENT)
	#include "reg_blecore.h"        // ble core registers
	#include "reg_em_ble_cs.h"      // control structure definitions
#endif //BLE_EMB_PRESENT
#if (BT_EMB_PRESENT)
	#include "reg_btcore.h"         // bt core registers
	#include "reg_em_bt_cs.h"       // control structure definitions
#endif //BT_EMB_PRESENT

#include "reg_access.h"			//// added
//#include "BK3000_reg.h"				//// added
#include "reg_btcore.h"
#include "user_config.h"		////
#include "bk_config.h"

/**
 ****************************************************************************************
 * DEFINES
 ****************************************************************************************
 **/
#define RF_GAIN_TBL_SIZE           0x0F


// EM RF SPI address
#define RF_EM_SPI_ADRESS        (EM_BASE_ADDR + EM_RF_SW_SPI_OFFSET)

#define RF_SPIRD                   0x00
#define RF_SPIWR                   0x80
#define RF_RFPLL_TBL_SIZE          0x50

/* The offset value given below is the offset to add to the frequency table index to
   get the value to be programmed in the radio for each channel                      */
#define RF_FREQTAB_OFFSET          0   // Offset for Ripple radio

/// Radio skew compensation (round trip delay)
#define RF_RADIO_SKEW              12L

#define RFLOIF                      0x00

#define RF_RSSI_20dB_THRHLD        -20
#define RF_RSSI_45dB_THRHLD        -45
#define RF_RSSI_48dB_THRHLD        -48
#define RF_RSSI_55dB_THRHLD        -55
#define RF_RSSI_60dB_THRHLD        -60
#define RF_RSSI_70dB_THRHLD        -70

// EDR Control value
#define RF_EDRCNTL                 18 // Default value is set to 18us


////==> RF_TX_PW_CONV_TBL[X]
#if(RF_POWER_CONTROL_BY_STEP == 1)
	#define RF_PWR_TBL_SIZE        0x20
	#define RF_POWER_MAX           0x1F
	#define RF_POWER_MIN           0x00	//// ????
	#define RF_POWER_MSK           0x1F	//// ????
#else
	////#define RF_POWER_MAX               0x0c	//// ????
	#define RF_PWR_TBL_SIZE        0x0F
	#define RF_POWER_MAX           0x06
	#define RF_POWER_MIN           0x01	//// ????
	#define RF_POWER_MSK           0x07	//// ????
#endif

// Generic RSSI Threshold
#define RF_RF_RSSI_THR             0x29

#define EMPTY_BLE_RADIO_TXRXTIM1   0x00000908 //2M bps //LL/CON/MAS/BV-77-C

/**
 ****************************************************************************************
 * GLOBAL VARIABLE DEFINITIONS
 *****************************************************************************************
 **/
volatile uint32_t XVR_ANALOG_REG_BAK[16] = {0};

// Power table	////Need Update
#if(RF_POWER_CONTROL_BY_STEP == 1)
	static const int8_t RF_TX_PW_CONV_TBL[RF_PWR_TBL_SIZE] =
	{								//// [dbm][RegindexVal]
		-30,	////-30.1
		-21,	////-21.2
		-16,	////-16.9
		-14,	////-14.0
		-11,	////-11.8
		-10,	////-10.1
		-8 ,	////-8.6
		-7 ,	////-7.3
		-6 ,	////-6.2
		-5 ,	////-5.1
		-4 ,	////-4.2
		-3 ,	////-3.4
		-2 ,	////-2.6
		-1 ,	////-1.9
		-1 ,	////-1.3
		 0 ,	////-0.6
		 0 ,	//// 0.0
		 0 ,	//// 0.5
		 1 ,	//// 1.0
		 1 ,	//// 1.5
		 2 ,	//// 2.0
		 2 ,	//// 2.4
		 2 ,	//// 2.8
		 3 ,	//// 3.2
		 3 ,	//// 3.6
		 3 ,	//// 3.9
		 4 ,	//// 4.2
		 4 ,	//// 4.5
		 4 ,	//// 4.8
		 5 ,	//// 5.0
		 5 ,	//// 5.3
		 5 		//// 5.5
	};
#else
	static const int8_t RF_TX_PW_CONV_TBL[RF_PWR_TBL_SIZE] =
	{
	    [0] = -23,
	    [1] = -20,
	    [2] = -17,
	    [3] = -14,
	    [4] = -11,
	    [5] = -8,
	    [6] = -5,
	    [7] = -2
	};
#endif

/*****************************************************************************************
 * FUNCTION DEFINITIONS
 ****************************************************************************************/

/*****************************************************************************************
 * @brief Read access
 *
 * @param[in] addr    register address
 *
 * @return uint32_t value
 ****************************************************************************************/
static uint32_t rf_rpl_reg_rd (uint32_t addr)
{
    uint32_t ret;
	ret = REG_PL_RD(addr);
	return ret;
}

/*****************************************************************************************
 * @brief Write access
 *
 * @param[in] addr    register address
 * @param[in] value   value to write
 ****************************************************************************************/
static void rf_rpl_reg_wr (uint32_t addr, uint32_t value)
{
	REG_PL_WR(addr, value);
}
#if 0
/*****************************************************************************************
 * @brief Initialize frequency table in the exchange memory
 ****************************************************************************************/
static void rf_em_init(void)
{
    uint8_t idx = 0;
    uint8_t temp_freq_tbl[EM_RF_FREQ_TABLE_LEN];

    #if (BT_EMB_PRESENT)
    // First half part of frequency table is for the even frequencies
    while(idx < (EM_RF_FREQ_TABLE_LEN/2))
    {
      temp_freq_tbl[idx] = 2*idx;    //// + RF_FREQTAB_OFFSET;
      idx++;
    }
    while(idx < EM_RF_FREQ_TABLE_LEN)
    {
      temp_freq_tbl[idx] = 2*(idx-(EM_RF_FREQ_TABLE_LEN/2)) + 1;    ///// + RF_FREQTAB_OFFSET;
      idx++;
    }
    em_wr(&temp_freq_tbl[0], EM_FT_OFFSET, EM_RF_FREQ_TABLE_LEN);
    #elif (BLE_EMB_PRESENT)
    while(idx < EM_RF_FREQ_TABLE_LEN)
    {
      temp_freq_tbl[idx] = 2*idx;    //// + RF_FREQTAB_OFFSET;
      idx++;
    }
    em_wr(&temp_freq_tbl[0], EM_FT_OFFSET, EM_RF_FREQ_TABLE_LEN);
    #endif //(BT_EMB_PRESENT/BLE_EMB_PRESENT)
}
#endif
/**
 *****************************************************************************************
 * @brief Convert RSSI to dBm
 *
 * @param[in] rssi_reg RSSI read from the HW registers
 *
 * @return The converted RSSI
 *****************************************************************************************
 */
static int8_t rf_rssi_convert (uint8_t rssi_reg)
{
    uint8_t RssidBm = 0;

    RssidBm = ((rssi_reg) >> 1) - 118;

    return(RssidBm);
}

/**
 *****************************************************************************************
 * @brief Get the TX power as control structure TX power field from a value in dBm.
 *
 * @param[in] txpwr_dbm   TX power in dBm
 * @param[in] high        If true, return index equal to or higher than requested
 *                        If false, return index equal to or lower than requested
 *
 * @return The index of the TX power
 *****************************************************************************************
 */
static uint8_t rf_txpwr_cs_get (int8_t txpwr_dbm, bool high)
{
    uint8_t i;
#if(RF_POWER_CONTROL_BY_STEP == 1)
	 for (i = RF_POWER_MIN; i <= RF_POWER_MAX; i++)
    {
        // Loop until we find a power just higher or equal to the requested one
        if (RF_TX_PW_CONV_TBL[i] >= txpwr_dbm)
            break;
    }

    // If equal to value requested, do nothing
    // Else if 'high' is false and index higher than the minimum one, decrement by one
    if ((RF_TX_PW_CONV_TBL[i] != txpwr_dbm) && (!high) && (i > RF_POWER_MIN))
    {	////TODO, FIXME
        i--;
    }
#else
    for (i = RF_POWER_MIN; i <= RF_POWER_MAX; i++)
    {
        // Loop until we find a power just higher or equal to the requested one
        if (RF_TX_PW_CONV_TBL[i] >= txpwr_dbm)
            break;
    }

    // If equal to value requested, do nothing
    // Else if 'high' is false and index higher than the minimum one, decrement by one
    if ((RF_TX_PW_CONV_TBL[i] != txpwr_dbm) && (!high) && (i > RF_POWER_MIN))
    {
        i--;
    }
#endif
    return(i);
}

/**
 *****************************************************************************************
 * @brief Init RF sequence after reset.
 *****************************************************************************************
 */
static void rf_reset(void)
{
    return;
}

#if (BT_EMB_PRESENT)
/**
 *****************************************************************************************
 * @brief Decrease the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be decreased
 *
 * @return true when minimum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_dec(uint8_t link_id)
{

#if(RF_POWER_CONTROL_BY_STEP == 1)
	// Get current TX power value
    uint8_t tx_pwr = em_bt_pwrcntl_txpwr_getf(EM_BT_CS_ACL_INDEX(link_id)) & RF_POWER_MSK;
	// Check if value can be decreased
	if (tx_pwr > RF_POWER_MIN)
    {
        // Decrease the TX power value
        em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), tx_pwr-1);
    }
#else
	// Get current TX power value
    uint8_t tx_pwr = em_bt_pwrcntl_txpwr_getf(EM_BT_CS_ACL_INDEX(link_id)) & RF_POWER_MSK;
    // Check if value can be decreased
	if (tx_pwr > RF_POWER_MIN)
    {
        // Decrease the TX power value
        em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), tx_pwr-1);
    }
#endif
    return (tx_pwr > RF_POWER_MIN);
}

/**
 *****************************************************************************************
 * @brief Increase the TX power by one step
 *
 * @param[in] link_id Link ID for which the TX power has to be increased
 *
 * @return true when maximum power is reached, false otherwise
 *****************************************************************************************
 */
static bool rf_txpwr_inc(uint8_t link_id)
{
    // Get current TX power value
    uint8_t tx_pwr = em_bt_pwrcntl_txpwr_getf(EM_BT_CS_ACL_INDEX(link_id)) & RF_POWER_MSK;
#if(RF_POWER_CONTROL_BY_STEP == 1)
	// Check if value can be increased
	if (tx_pwr < RF_POWER_MAX)
    {
        //Increase the TX power value
        em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), tx_pwr+1);
    }
#else
    // Check if value can be increased
	if (tx_pwr < RF_POWER_MAX)
    {
        //Increase the TX power value
        em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), tx_pwr+1);
    }
#endif
    return (tx_pwr < RF_POWER_MAX);
}

/**
 ****************************************************************************************
 * @brief Set the TX power to max
 *
 * @param[in] link_id     Link Identifier
 ****************************************************************************************
 */
static void rf_txpwr_max_set(uint8_t link_id)
{
    // Set max TX power value
#if(RF_POWER_CONTROL_BY_STEP == 1)
	em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), RF_POWER_MAX);
#else
    em_bt_pwrcntl_txpwr_setf(EM_BT_CS_ACL_INDEX(link_id), RF_POWER_MAX);
#endif
}
#endif //(BT_EMB_PRESENT)

#if (BLE_EMB_PRESENT)
/**
 *****************************************************************************************
 * @brief Enable/disable force AGC mechanism
 *
 * @param[in]  True: Enable / False: disable
 *****************************************************************************************
 */
static void rf_force_agc_enable(bool en)
{
	#if defined(CFG_BT)
    ip_radiocntl1_forceagc_en_setf(en);
    #else
    ip_radiocntl1_forceagc_en_setf(en);
    #endif //CFG_BLE
}
#endif //(BLE_EMB_PRESENT)

/**
 *****************************************************************************************
 * @brief Get TX power in dBm from the index in the control structure
 *
 * @param[in] txpwr_idx  Index of the TX power in the control structure
 * @param[in] modulation Modulation: 1 or 2 or 3 MBPS
 *
 * @return The TX power in dBm
 *****************************************************************************************
 */
static uint8_t rf_txpwr_dbm_get(uint8_t txpwr_idx, uint8_t modulation)
{	////TODO, FIXME, WHY return data type is "uint8_t" ????
    // power table is the same for BR and EDR
#if(RF_POWER_CONTROL_BY_STEP == 1)
	return (RF_TX_PW_CONV_TBL[txpwr_idx]);
#else
    return (RF_TX_PW_CONV_TBL[txpwr_idx]);
#endif
}

/**
 *****************************************************************************************
 * @brief Sleep function for the RF.
 *****************************************************************************************
 */
static void rf_sleep(void)
{
    ip_deepslcntl_set(ip_deepslcntl_get() |
                      IP_DEEP_SLEEP_ON_BIT |     // RW BT Core sleep
                      IP_RADIO_SLEEP_EN_BIT |    // Radio sleep
                      IP_OSC_SLEEP_EN_BIT);      // Oscillator sleep
}

////
//extern void sys_delay_ms(uint32_t ms);
void xvr_reg_init(void) {
#if 0 /// for test
    REG_XVR_0x2B = 0x4A4C0000;//recover(fix BLE NOT Rx)
#endif /// for test

#if 0  ///org
#if 1	////PTN101 CEVA Setting
  	REG_XVR_0x00 = XVR_ANALOG_REG_BAK[0 ] = 0x99B70113;
	REG_XVR_0x01 = XVR_ANALOG_REG_BAK[1 ] = 0x99B7030C;
	REG_XVR_0x02 = XVR_ANALOG_REG_BAK[2 ] = 0x7C501006;
	REG_XVR_0x03 = XVR_ANALOG_REG_BAK[3 ] = 0xC8780FFE;
	REG_XVR_0x04 = XVR_ANALOG_REG_BAK[4 ] = 0x00000000;
	REG_XVR_0x05 = XVR_ANALOG_REG_BAK[5 ] = 0x9166B4A0;
	REG_XVR_0x06 = XVR_ANALOG_REG_BAK[6 ] = 0x060CD004;
	REG_XVR_0x07 = XVR_ANALOG_REG_BAK[7 ] = 0x7EC871FF;
	REG_XVR_0x08 = XVR_ANALOG_REG_BAK[8 ] = 0x558081C0;
	REG_XVR_0x09 = XVR_ANALOG_REG_BAK[9 ] = 0x1BEFF6D4;	////0x1BEFC6D4;  ////Fix(RWIP)
	REG_XVR_0x0A = XVR_ANALOG_REG_BAK[10] = 0x2496C9BC;
	REG_XVR_0x0B = XVR_ANALOG_REG_BAK[11] = 0x002A8302;
	REG_XVR_0x0C = XVR_ANALOG_REG_BAK[12] = 0xFCFD6000;
	REG_XVR_0x0D = XVR_ANALOG_REG_BAK[13] = 0x00000000;
	REG_XVR_0x0E = XVR_ANALOG_REG_BAK[14] = 0x00000000;
	REG_XVR_0x0F = XVR_ANALOG_REG_BAK[15] = 0x00000000;

    REG_XVR_0x10 = 0x00003262;
    REG_XVR_0x11 = 0x00000000;
    REG_XVR_0x12 = 0x00000000;
    REG_XVR_0x13 = 0x00000000;
    REG_XVR_0x14 = 0x00000000;
    REG_XVR_0x15 = 0x0041B8F1;
    REG_XVR_0x16 = 0x0000357D;
    REG_XVR_0x17 = 0x00000000;
    REG_XVR_0x18 = 0x00000000;
    REG_XVR_0x20 = 0x446475C6;
    REG_XVR_0x21 = 0x4E431080;
    REG_XVR_0x22 = 0xCB082409;
    REG_XVR_0x23 = 0x7B13B14B;
#if(PTN101_CHANNEL_TEST == 1)
	REG_XVR_0x24 = 0x00741802; 	/////20191112 FixCh02
#else
    REG_XVR_0x24 = 0x00F41861;	////0x00F41861;==> DSP Control	////0x00141861;
#endif
    REG_XVR_0x25 = 0x00000000;
    REG_XVR_0x26 = 0x08000000;
    REG_XVR_0x27 = 0x00004120;
    REG_XVR_0x28 = 0x03132333;
    REG_XVR_0x29 = 0x0D041100;
    //REG_XVR_0x2A = 0x13504042;	//used CEVA setting ////REG_XVR_0x2A = 0x01503842;
    REG_XVR_0x2B = 0x4A4C0000;//recover(fix BLE NOT Rx)
    REG_XVR_0x2C = 0x00000000;
    REG_XVR_0x2D = 0x00284841;
    REG_XVR_0x30 = 0xA432981B;
    REG_XVR_0x31 = 0x0000006E;
    REG_XVR_0x32 = 0xF4F4000A;	////0xF4F808FC;	////Fix(for RWIP, will poweron calibration in the future)
    REG_XVR_0x33 = 0x00000000;	////0x00001800;	////Fix(for RWIP, will poweron calibration in the future)
    REG_XVR_0x38 = 0x2E1603FF;
    REG_XVR_0x39 = 0xA4802860;
    REG_XVR_0x3A = 0x00000000;
    REG_XVR_0x3B = 0x09345288;
    REG_XVR_0x3C = 0x308403D1;
    REG_XVR_0x3D = 0x626A0027;
    REG_XVR_0x40 = 0x01000000;
    REG_XVR_0x41 = 0x07050402;
    REG_XVR_0x42 = 0x120F0C0A;
    REG_XVR_0x43 = 0x221E1A16;
    REG_XVR_0x44 = 0x35302B26;
    REG_XVR_0x45 = 0x4B45403A;
    REG_XVR_0x46 = 0x635D5751;
    REG_XVR_0x47 = 0x7C767069;
    REG_XVR_0x48 = 0x968F8983;
    REG_XVR_0x49 = 0xAEA8A29C;
    REG_XVR_0x4A = 0xC5BFBAB4;
    REG_XVR_0x4B = 0xD9D4CFCA;
    REG_XVR_0x4C = 0xE9E5E1DD;
    REG_XVR_0x4D = 0xF5F3F0ED;
    REG_XVR_0x4E = 0xFDFBFAF8;
    REG_XVR_0x4F = 0xFFFFFFFE;

    REG_XVR_0x25 = 0x2400;
    REG_XVR_0x08 = XVR_ANALOG_REG_BAK[8] | (1 << 18);
    sys_delay_ms(1);
    REG_XVR_0x08 = XVR_ANALOG_REG_BAK[8];
    sys_delay_ms(1);
    REG_XVR_0x25 = 0;
#else
	//// PTN101_XVR_TXOK_BESTEVM.txt, 20191025
	// Analog & Testing Registers
	REG_XVR_0x00 = 0x99A1F912  ;
	REG_XVR_0x01 = 0X99B7030C  ;
	REG_XVR_0x02 = 0x7C501006  ;
	REG_XVR_0x03 = 0xCE780FFE  ;
	REG_XVR_0x04 = 0x40000000  ;
	REG_XVR_0x05 = 0x00004602  ;
	REG_XVR_0x06 = 0x6E080620  ;
	REG_XVR_0x07 = 0x2A1B0E00  ;
	REG_XVR_0x08 = 0x558081C0  ;
	REG_XVR_0x09 = 0x13EFF6C0  ;
	REG_XVR_0x0A = 0x6DB6087D  ;
	REG_XVR_0x0B = 0x01E901B4  ;
	REG_XVR_0x0C = 0x0C306000  ;
	REG_XVR_0x0D = 0x00000000  ;
	REG_XVR_0x0E = 0x00000000  ;
	REG_XVR_0x0F = 0x00000000  ;
	REG_XVR_0x10 = 0x000C0E04  ;
	REG_XVR_0x11 = 0x00000000  ;
	REG_XVR_0x12 = 0x00000000  ;
	REG_XVR_0x13 = 0x00000000  ;
	REG_XVR_0x14 = 0x00000000  ;
	REG_XVR_0x15 = 0x00000000  ;
	REG_XVR_0x16 = 0x00000000  ;
	REG_XVR_0x17 = 0x00000000  ;
	REG_XVR_0x18 = 0x00000000  ;
	// General RF Setting
	REG_XVR_0x20 = 0x446475c6;// REG_20
	REG_XVR_0x21 = 0x4e431080;// REG_21
	REG_XVR_0x22 = 0xCB082409;// REG_22
	REG_XVR_0x23 = 0x7B13B14B;////0x2E276276;// REG_23
	REG_XVR_0x24 = 0x00141820;// REG_24
	REG_XVR_0x25 = 0x00007800;// REG_25
	REG_XVR_0x26 = 0x08000000;// REG_26
	REG_XVR_0x27 = 0x0008f100;// REG_27
	REG_XVR_0x28 = 0x03132333;// REG_28
	REG_XVR_0x29 = 0x0D041100;// REG_29
	REG_XVR_0x2A = 0x01504042;// REG_2A
	REG_XVR_0x2B = 0x00000000;// REG_2B
    REG_XVR_0x2C = 0x00000000;// REG_2C
	REG_XVR_0x2D = 0x00284841;// REG_2D
////REG_XVR_0x2E = 0x00000000;// REG_2E
////REG_XVR_0x2F = 0X00000000;// REG_2F
	// TX Modulate
	REG_XVR_0x30 = 0xA432981B;// REG_30
	REG_XVR_0x31 = 0x0000006E;// REG_31
    REG_XVR_0x32 = 0xFFFFFFFF;// REG_32
    REG_XVR_0x33 = 0x00000000;// REG_33
////REG_XVR_0x34 = 0X00000000;// REG_34
////REG_XVR_0x35 = 0X00000000;// REG_35
////REG_XVR_0x36 = 0X00000000;// REG_36
////REG_XVR_0x37 = 0X00000000;// REG_37
	// RX Demodulate
    REG_XVR_0x38 = 0x2E1603FF;// REG_38
    REG_XVR_0x39 = 0xA4802860;// REG_39
    REG_XVR_0x3A = 0x00000000;// REG_3A
    REG_XVR_0x3B = 0x49345288;// REG_3B
    REG_XVR_0x3C = 0xB08403D1;// REG_3C
    REG_XVR_0x3D = 0x626A0027;// REG_3D
////REG_XVR_0x3E = 0X00000000;// REG_3E
////REG_XVR_0x3F = 0X00000000;// REG_3F

	REG_XVR_0x40 = 0x01000000;// REG_40
	REG_XVR_0x41 = 0x07050402;// REG_41
	REG_XVR_0x42 = 0x120F0C0A;// REG_42
	REG_XVR_0x43 = 0x221E1A16;// REG_43
	REG_XVR_0x44 = 0X35302B26;// REG_44
	REG_XVR_0x45 = 0X4B45403A;// REG_45
	REG_XVR_0x46 = 0X635D5751;// REG_46
	REG_XVR_0x47 = 0X7C767069;// REG_47
	REG_XVR_0x48 = 0x968F8983;// REG_48
	REG_XVR_0x49 = 0xAEA8A29C;// REG_49
	REG_XVR_0x4A = 0xC5BFBAB4;// REG_4A
	REG_XVR_0x4B = 0xD9D4CFCA;// REG_4B
	REG_XVR_0x4C = 0xE9E5E1DD;// REG_4C
	REG_XVR_0x4D = 0xF5F3F0ED;// REG_4D
	REG_XVR_0x4E = 0XFDFBFAF8;// REG_4E
	REG_XVR_0x4F = 0XFFFFFFFE;// REG_4F
#endif
#endif
}
////

void rw_rf_init(struct rwip_rf_api *api)
{
#if defined(CFG_BT)
	uint8_t length = PARAM_LEN_RSSI_THR;
#endif //CFG_BT

	// Initialize the RF driver API structure
	api->reg_rd = rf_rpl_reg_rd;
	api->reg_wr = rf_rpl_reg_wr;
	api->txpwr_dbm_get = rf_txpwr_dbm_get;
	api->txpwr_min = RF_POWER_MIN;
	api->txpwr_max = RF_POWER_MAX;
	api->sleep = rf_sleep;
	api->reset = rf_reset;

#if defined(CFG_BLE)
	api->force_agc_enable = rf_force_agc_enable;
#endif //CFG_BLE

	api->rssi_convert = rf_rssi_convert;
	api->txpwr_cs_get = rf_txpwr_cs_get;

#if defined(CFG_BT)
	api->txpwr_dec = rf_txpwr_dec;
	api->txpwr_inc = rf_txpwr_inc;
	api->txpwr_max_set = rf_txpwr_max_set;
	// Initialize the RSSI thresholds (high, low, interference)
	// These are 'real' signed values in dBm
	if (   (rwip_param.get(PARAM_ID_RSSI_HIGH_THR, &length, (uint8_t*)&api->rssi_high_thr) != PARAM_OK)
		|| (rwip_param.get(PARAM_ID_RSSI_LOW_THR, &length, (uint8_t*)&api->rssi_low_thr) != PARAM_OK)
		|| (rwip_param.get(PARAM_ID_RSSI_INTERF_THR, &length, (uint8_t*)&api->rssi_interf_thr) != PARAM_OK) )
	{
		api->rssi_high_thr = (int8_t)RF_RSSI_20dB_THRHLD;
		api->rssi_low_thr = (int8_t)RF_RSSI_60dB_THRHLD;
		api->rssi_interf_thr = (int8_t)RF_RSSI_70dB_THRHLD;
	}
#endif //CFG_BT

	//xvr_reg_init(); 	////

#if 1 /// Charles PTN101_BQB
////	ip_timgencntl_set(0x00000120);		////Beken,

	////0x01100070: No Setting ????
	ip_radiocntl0_pack(/*uint16_t spiptr*/	 (EM_RF_SW_SPI_OFFSET >> 2),
						/*uint8_t  spicfg*/   0,
						/*uint8_t  spifreq*/  1,
						/*uint8_t  spigo*/	  0);

	/* IP RADIOCNTL1 */			////0x00000020
	//// 0x01100074 : 0x00000020
  #if 1
	ip_radiocntl1_set(0x00000020);
	//_BK_PRINTF("ip_radiocntl1_get(%x)\r\n",ip_radiocntl1_get());//charles
  #else
	ip_radiocntl1_pack(/*uint8_t  forceagcen*/		 0,
						/*uint8_t  forceiq*/		 0,
						/*uint8_t  rxdnsl*/ 		 0,
						/*uint8_t  txdnsl*/ 		 0,
						/*uint16_t forceagclength*/  0,
						/*uint8_t  syncpulsemode*/	 0,
						/*uint8_t  syncpulsesrc*/	 0,
						/*uint8_t  dpcorren*/		 0,
						/*uint8_t  jefselect*/		 0,
						/*uint8_t  xrfsel*/ 		 0x02,
						/*uint8_t  subversion*/ 	 0x0);
  #endif

#if defined(CFG_BLE)
	/* BLE RADIOCNTL2 */
	ble_radiocntl2_pack(/*uint8_t  lrsynccompmode*/    0x0,
						/*uint8_t  rxcitermbypass*/    0x0,
						/*uint8_t  lrvtbflush*/ 	   0x8,
						/*uint8_t  phymsk*/ 		   0x3, // mark that Coded phy are supported
						/*uint8_t  lrsyncerr*/		   0,
						/*uint8_t  syncerr*/		   0,
						/*uint16_t freqtableptr*/	   (EM_FT_OFFSET));	////(EM_FT_OFFSET >> 2));

	/* BLE RADIOCNTL3 */
	ble_radiocntl3_pack(/*uint8_t rxrate3cfg*/	       0x1, // map on 1 Mbps
						/*uint8_t rxrate2cfg*/	       0x1, // map on 1 Mbps
						/*uint8_t rxrate1cfg*/	       0x0,
						/*uint8_t rxrate0cfg*/	       0x1,
						/*uint8_t getrssidelay*/       0x0,
						/*uint8_t rxsyncrouting*/      0x0,
						/*uint8_t rxvalidbeh*/	       0x0,
						/*uint8_t txrate3cfg*/	       0x1, // map on 1 Mbps
						/*uint8_t txrate2cfg*/	       0x1, // map on 1 Mbps
						/*uint8_t txrate1cfg*/	       0x0,
						/*uint8_t txrate0cfg*/	       0x1,
						/*uint8_t txvalidbeh*/	       0x0);

	/* BLE RADIOPWRUPDN0 */
	ble_radiopwrupdn0_pack(/*uint8_t syncposition0*/   0,		
						   /*uint8_t rxpwrup0*/ 	   0x60,	
						   /*uint8_t txpwrdn0*/ 	   0x00,
						   /*uint8_t txpwrup0*/ 	   0x65);    ///1M bps
    /* BLE RADIOPWRUPDN1 */
    ble_radiopwrupdn1_pack(/*uint8_t syncposition1*/   0,
                           /*uint8_t rxpwrup1*/        0x60,
                           /*uint8_t txpwrdn0*/        0x00,
                           /*uint8_t txpwrup1*/        0x65);    ///2M bps
	/* BLE RADIOPWRUPDN2 */
	ble_radiopwrupdn2_pack(/*uint8_t syncposition2*/   0,
						   /*uint8_t rxpwrup2*/ 	   0x60,
						   /*uint8_t txpwrdn2*/ 	   0x00,
						   /*uint8_t txpwrup2*/ 	   0x65);    ///long rang S8 125k
	/* BLE RADIOPWRUPDN3 */
	ble_radiopwrupdn3_pack(/*uint8_t txpwrdn3*/ 	   0x00,
						   /*uint8_t txpwrup3*/ 	   0x65);    ///long rang S2 500k

    
	/* BLE RADIOTXRXTIM0 */
	//// 0x01100890 : ?????
	ble_radiotxrxtim0_pack(/*uint8_t rfrxtmda0*/       0,
						   /*uint8_t rxpathdly0*/      0x06,
						   /*uint8_t txpathdly0*/      0x08);    ///1M bps
    /* BLE RADIOTXRXTIM1 */
    ble_radiotxrxtim1_pack(/*uint8_t rfrxtmda1*/       0x00,
                           /*uint8_t rxpathdly1*/      0x05,
                           /*uint8_t txpathdly1*/      0x06);    ///2M bps
	/* BLE RADIOTXRXTIM2 */
	ble_radiotxrxtim2_pack(/*uint8_t rxflushpathdly2*/ 0x10,
						   /*uint8_t rfrxtmda2*/	   0x00,
						   /*uint8_t rxpathdly2*/	   0x49,
						   /*uint8_t txpathdly2*/	   0x03);    ///long rang S8 125k
	/* BLE RADIOTXRXTIM3 */
	ble_radiotxrxtim3_pack(/*uint8_t rxflushpathdly3*/ 0x10,
						   /*uint8_t rfrxtmda3*/	   0x00,
						   /*uint8_t txpathdly3*/	   0x03);    ///long rang S2 500k

#endif // defined CFG_BLE
#if 0
//for RW ip reg read write test
bt_radiopwrupdn_set(0x00656565);////20191112,  	(0x00756565)	////(0x00710271);
_BK_PRINTF("bt_radiopwrupdn_get()==%x\r\n",bt_radiopwrupdn_get());
bt_radiotxrxtim_set(0x00000608);		////(0x00000403);
_BK_PRINTF("bt_radiotxrxtim_get()==%x\r\n",bt_radiotxrxtim_get());
bt_radiocntl3_set(0x39003900);
_BK_PRINTF("bt_radiocntl3_get()==%x\r\n",bt_radiocntl3_get());
ip_radiocntl1_forceiq_setf(1);
_BK_PRINTF("ip_radiocntl1_forceiq_getf()==%x\r\n",ip_radiocntl1_forceiq_getf());
bt_intcntl_set(0x55AA55AA);
_BK_PRINTF("bt_intcntl_get()==%x\r\n",bt_intcntl_get());
#endif





////////
#if defined(CFG_BT)
	/* EDRCNTL */
  #if 1		////BEKEN
	//// 0x01100400 : 0x0000010A
////	bt_rwbtcntl_set(0x0000010A);		////20191104
  #else
	bt_rwbtcntl_nwinsize_setf(0);
  #endif
	//// 0x01100428 : ?????
	bt_edrcntl_rxgrd_timeout_setf(RF_EDRCNTL);

	/* BT RADIOPWRUPDN */
	#if 1		//// BEKEN
	//// 0x0110048C : 0x00710271
	bt_radiopwrupdn_set(0x00656565);////20191112,  	(0x00756565)	////(0x00710271);
	#else
	bt_radiopwrupdn_rxpwrupct_setf(0x42);
	bt_radiopwrupdn_txpwrdnct_setf(0x07);
	bt_radiopwrupdn_txpwrupct_setf(0x56);
	#endif
	/* BT RADIOCNTL 2 */
	#if 0		////BEKEN
	//// 0x01100478 : 0x04070100
	bt_radiocntl2_set(0x04070100);		////(0x04070080); ????
	#else
	bt_radiocntl2_freqtable_ptr_setf((EM_FT_OFFSET));		////((EM_FT_OFFSET >> 2));
////	bt_radiocntl2_syncerr_setf(0x7);
	#endif
	/* BT RADIOTXRXTIM */
	#define PRL_TX_PATH_DLY 4
	#define PRL_RX_PATH_DLY (RF_RADIO_SKEW - PRL_TX_PATH_DLY)
	#if 1		////BEKEN
		//// 0x01100490 : 0x00000403
		bt_radiotxrxtim_set(0x00000608);		////(0x00000403);
	#else
		bt_radiotxrxtim_rxpathdly_setf(PRL_RX_PATH_DLY);
		bt_radiotxrxtim_txpathdly_setf(PRL_TX_PATH_DLY);
		bt_radiotxrxtim_sync_position_setf(0x38); // Default is 0x10
	#endif
	/* BT RADIOCNTL 3*/
	#if 1		////BEKEN
	//// 0x0110047C : 0x39003900
	bt_radiocntl3_set(0x39003900);
	#else
	bt_radiocntl3_pack( /*uint8_t rxrate2cfg*/	  3,
						/*uint8_t rxrate1cfg*/	  2,
						/*uint8_t rxrate0cfg*/	  1,
						/*uint8_t rxserparif*/	  0,
						/*uint8_t rxsyncrouting*/ 0,
						/*uint8_t rxvalidbeh*/	  0,
						/*uint8_t txrate2cfg*/	  3,
						/*uint8_t txrate1cfg*/	  2,
						/*uint8_t txrate0cfg*/	  1,
						/*uint8_t txserparif*/	  0,
						/*uint8_t txvalidbeh*/	  0);
	#endif

	////BT Timer General Control
	//// 0x011004E0 : 0x01900140
	bt_timgencntl_set(0x01900140);		////will be update in "ld_core_init()"

  #if(RWIP_WHITEN_DISABLE == 1)
	bt_rwbtcntl_whitdsb_setf(1);		////Turn Whiten off
  #endif

#endif //CFG_BT

////
////xvr_reg_init();		//// Move up
	// Settings for proper reception
#if defined(CFG_BLE)
	ip_radiocntl1_forceiq_setf(1);
	ip_radiocntl1_dpcorr_en_setf(0x0);
	ASSERT_ERR(ip_radiocntl1_dpcorr_en_getf() == 0x0);
#endif // CFG_BLE

#if defined(CFG_BT)
	ip_radiocntl1_dpcorr_en_setf(0x1);
	ASSERT_ERR(ip_radiocntl1_dpcorr_en_getf() == 0x1);
#endif // CFG_BT

#if defined(CFG_BLE)
	// Force IQ mode for BLE only
	ip_radiocntl1_forceiq_setf(1);
#endif //CFG_BLE

#endif /// from Charles PTN101_BQB



#if 0 /// from BK3633
	/// Initialize Exchange Memory
	//rf_em_init();		////20191112

//		ip_radiocntl0_pack(/*uint16_t spiptr*/	 (EM_RF_SW_SPI_OFFSET >> 2),
//							/*uint8_t  spicfg*/   0,
//							/*uint8_t  spifreq*/  1,
//							/*uint8_t  spigo*/	  0);

		/* ip RADIOCNTL1 */
		//bk_printf("RW common reg init\r\n");
		#if 1	//// Beken Version
		ip_radiocntl1_pack(/*uint8_t  forceagcen*/		 0,
							/*uint8_t  forceiq*/		 0,
							/*uint8_t  rxdnsl*/ 		 0,
							/*uint8_t  txdnsl*/ 		 0,
							/*uint16_t forceagclength*/  0,
							/*uint8_t  syncpulsemode*/	 0,
							/*uint8_t  syncpulsesrc*/	 0,
							/*uint8_t  dpcorren*/		 0,
							/*uint8_t  jefselect*/		 0,
							/*uint8_t  xrfsel*/ 		 0x02,
							/*uint8_t  subversion*/ 	 0x0);


		ip_radiocntl1_set(0x00000020);
		//bk_printf("ip RADIOCNTL1 addr:0x%08x,val:0x%08x\r\n",ip_RADIOCNTL1_ADDR,ip_radiocntl1_get());
		ip_timgencntl_set(0x01df0120);		////Beken,
		//bk_printf("ip_TIMGENCNTL addr:0x%08x,val:0x%08x\r\n",ip_TIMGENCNTL_ADDR,ip_timgencntl_get());
	#endif



	#if defined(CFG_BLE)
		//bk_printf("RW BLE reg init\r\n");
		/* BLE RADIOCNTL2 */
		ble_radiocntl2_pack(/*uint8_t  lrsynccompmode*/ 0x0,
							/*uint8_t  rxcitermbypass*/ 0x0,
							/*uint8_t  lrvtbflush*/ 	0x8,
							/*uint8_t  phymsk*/ 		0x2, // mark that Coded phy are supported
							/*uint8_t  lrsyncerr*/		0,
							/*uint8_t  syncerr*/		0,
							/*uint16_t freqtableptr*/	(EM_FT_OFFSET >> 2));
		ble_radiocntl2_set(0x00C000C0);
		//bk_printf("BLE_RADIOCNTL2 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOCNTL2_ADDR,ble_radiocntl2_get());

		/* BLE RADIOCNTL3 */
		#if 0
		ble_radiocntl3_pack(/*uint8_t rxrate3cfg*/	  0x1, // map on 1 Mbps
							/*uint8_t rxrate2cfg*/	  0x1, // map on 1 Mbps
							/*uint8_t rxrate1cfg*/	  0x0,
							/*uint8_t rxrate0cfg*/	  0x1,
							/*uint8_t rxsyncrouting*/ 0x0,
							/*uint8_t rxvalidbeh*/	  0x0,
							/*uint8_t txrate3cfg*/	  0x1, // map on 1 Mbps
							/*uint8_t txrate2cfg*/	  0x1, // map on 1 Mbps
							/*uint8_t txrate1cfg*/	  0x0,
							/*uint8_t txrate0cfg*/	  0x1,
							/*uint8_t txvalidbeh*/	  0x0);
		ble_radiocntl3_set(ble_radiocntl2_get());
		//bk_printf("BLE_RADIOCNTL3 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOCNTL2_ADDR,ble_radiocntl2_get());
		#endif


		/* BLE RADIOPWRUPDN0 */
		ble_radiopwrupdn0_pack(/*uint8_t syncposition0*/ 0,
							   /*uint8_t rxpwrup0*/ 	 0x65,
							   /*uint8_t txpwrdn0*/ 	 0x00,
							   /*uint8_t txpwrup0*/ 	 0x65);//1M bps
		 ble_radiopwrupdn0_set(0x00500050);
		//bk_printf("BLE_RADIOPWRUPDN0 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN0_ADDR,ble_radiopwrupdn0_get());

		/* BLE RADIOPWRUPDN1 */
		ble_radiopwrupdn1_pack(/*uint8_t syncposition1*/ 0,
							   /*uint8_t rxpwrup1*/ 	 0x65,
							   /*uint8_t txpwrdn0*/ 	 0x00,
							   /*uint8_t txpwrup1*/ 	 0x65);//2M bps
		ble_radiopwrupdn1_set(0x00650050);

		//bk_printf("BLE_RADIOPWRUPDN1 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN1_ADDR,ble_radiopwrupdn1_get());

		/* BLE RADIOPWRUPDN2 */
		ble_radiopwrupdn2_pack(/*uint8_t syncposition2*/ 0,
							   /*uint8_t rxpwrup2*/ 	 0x50, // 50
							   /*uint8_t txpwrdn2*/ 	 0x07,
							   /*uint8_t txpwrup2*/ 	 0x55);
		ble_radiopwrupdn2_set(0x00500050);//ble_radiopwrupdn2_set(0x00650041);//this value is ok //125K bps

		//bk_printf("BLE_RADIOPWRUPDN2 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN2_ADDR,ble_radiopwrupdn2_get());


		/* BLE RADIOPWRUPDN3 */
		ble_radiopwrupdn3_pack(/*uint8_t txpwrdn3*/ 	 0x07,
							   /*uint8_t txpwrup3*/ 	 0x55);
		ble_radiopwrupdn3_set(0x00000050);//500K//ble_radiopwrupdn3_set(0x00000050);//500K


		//bk_printf("BLE_RADIOPWRUPDN3 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOPWRUPDN3_ADDR,ble_radiopwrupdn3_get());

		/* BLE RADIOTXRXTIM0 */
		ble_radiotxrxtim0_pack(/*uint8_t rfrxtmda0*/   0,
							   /*uint8_t rxpathdly0*/  0x6,
							   /*uint8_t txpathdly0*/  0x6);
		ble_radiotxrxtim0_set(0x00000706);//1M initial ble_radiotxrxtim0_set(0x00000607);
		//1M
		//bk_printf("BLE_RADIOTXRXTIM0 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM0_ADDR,ble_radiotxrxtim0_get());

		/* BLE RADIOTXRXTIM1 */
		ble_radiotxrxtim1_pack(/*uint8_t rfrxtmda1*/ 0x00,
							   /*uint8_t rxpathdly1*/	   0x04,
							   /*uint8_t txpathdly1*/	   0x04);
		//ble_radiotxrxtim1_set(0x00000506);//150us

		ble_radiotxrxtim1_set(EMPTY_BLE_RADIO_TXRXTIM1);//2M initial value




		/* BLE RADIOTXRXTIM2 */
		//bk_printf("# 09\r\n");
		#if 0
		ble_radiotxrxtim2_pack(/*uint8_t rxflushpathdly2*/ 0x20 /*0x10*/,
							   /*uint8_t rfrxtmda2*/	   0x00,
							   /*uint8_t rxpathdly2*/	   0x49,
							   /*uint8_t txpathdly2*/	   0x03);
		#endif
		//ble_radiotxrxtim2_pack(0x15,0x00,0x15,0x2D);//this value advertising and connect is ok but phy change not ok(1M->125Kbps)
		ble_radiotxrxtim2_pack(0x15,0x00,0x15,0x2D);//125K
		//125K
		//bk_printf("BLE_RADIOTXRXTIM2 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM2_ADDR,ble_radiotxrxtim2_get());

		/* BLE RADIOTXRXTIM3 */
		//ble_radiotxrxtim3_pack(/*uint8_t rxflushpathdly3*/ 0x10,
		//					   /*uint8_t rfrxtmda3*/	   0x00,
		//					   /*uint8_t txpathdly3*/	   0x03);
		  ble_radiotxrxtim3_set(0x0C00000C);//500K initial value


		//bk_printf("BLE_RADIOTXRXTIM3 addr:0x%08x,val:0x%08x\r\n",BLE_RADIOTXRXTIM3_ADDR,ble_radiotxrxtim3_get());
		//bk_printf("init_1M_txrx_dly(%x)\r\n",ble_radiotxrxtim0_get());
		//bk_printf("init_2M_txrx_dly(%x)\r\n",ble_radiotxrxtim1_get());
		//bk_printf("init_125K_txrx_dly(%x)\r\n",ble_radiotxrxtim2_get());
		//bk_printf("init_500K_txrx_dly(%x)\r\n",ble_radiotxrxtim3_get());

	  #if (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
		// Init the DF CNTL
		ble_dfcntl0_1us_pack(/*uint8_t rxsampstinst01us*/ 0x08, /*uint8_t rxswstinst01us*/ 0x18, /*uint8_t txswstinst01us*/ 0x19);
		ble_dfcntl0_2us_pack(/*uint8_t rxsampstinst02us*/ 0x08, /*uint8_t rxswstinst02us*/ 0x18, /*uint8_t txswstinst02us*/ 0x19);
		ble_dfcntl1_1us_pack(/*uint8_t rxsampstinst11us*/ 0x08, /*uint8_t rxswstinst11us*/ 0x18, /*uint8_t txswstinst11us*/ 0x19);
		ble_dfcntl1_2us_pack(/*uint8_t rxsampstinst12us*/ 0x08, /*uint8_t rxswstinst12us*/ 0x18, /*uint8_t txswstinst12us*/ 0x19);
		ble_dfantcntl_pack(/*uint8_t rxprimidcntlen*/ 1, /*uint8_t rxprimantid*/ 0, /*uint8_t txprimidcntlen*/ 1, /*uint8_t txprimantid*/ 0);
	  #endif // (BLE_CON_CTE_REQ | BLE_CONLESS_CTE_RX)
	#endif // defined CFG_BLE

	#if defined(CFG_BT)

		/* EDRCNTL */
	  #if 1		////BEKEN
		bt_rwbtcntl_set(0x0000010A);
	  #else
		bt_rwbtcntl_nwinsize_setf(0);
	  #endif
		bt_edrcntl_rxgrd_timeout_setf(RPL_EDRCNTL);

		/* BT RADIOPWRUPDN */
		#if 1		//// BEKEN
		bt_radiopwrupdn_set(0x00710271);
		#else
		bt_radiopwrupdn_rxpwrupct_setf(0x42);
		bt_radiopwrupdn_txpwrdnct_setf(0x07);
		bt_radiopwrupdn_txpwrupct_setf(0x56);
		#endif
		/* BT RADIOCNTL 2 */
		#if 1		////BEKEN
		//bk_printf("# 14\r\n");
		bt_radiocntl2_set(0x04070100);
		#else
		bt_radiocntl2_freqtable_ptr_setf((EM_FT_OFFSET >> 2));
		bt_radiocntl2_syncerr_setf(0x7);
		#endif
		/* BT RADIOTXRXTIM */
		#define PRL_TX_PATH_DLY 4
		#define PRL_RX_PATH_DLY (RPL_RADIO_SKEW - PRL_TX_PATH_DLY)
		#if 1		////BEKEN
		//bk_printf("# 15\r\n");
			//bt_radiotxrxtim_set(0x00000403);//wrong value
			//bt_radiotxrxtim_set(0x00000903);//BK3633 tunning BT tx delay , correct 625 us
			bt_radiotxrxtim_set(0x00000706);//BK3633 tunning BT tx delay , correct 625 us
		#else
			bt_radiotxrxtim_rxpathdly_setf(PRL_RX_PATH_DLY);
			bt_radiotxrxtim_txpathdly_setf(PRL_TX_PATH_DLY);
			bt_radiotxrxtim_sync_position_setf(0x38); // Default is 0x10
		#endif
		/* BT RADIOCNTL 3*/
		#if 1		////BEKEN
		//bk_printf("# 16\r\n");
		bt_radiocntl3_set(0x39003900);
		#else
		bt_radiocntl3_pack( /*uint8_t rxrate2cfg*/	  3,
							/*uint8_t rxrate1cfg*/	  2,
							/*uint8_t rxrate0cfg*/	  1,
							/*uint8_t rxserparif*/	  0,
							/*uint8_t rxsyncrouting*/ 0,
							/*uint8_t rxvalidbeh*/	  0,
							/*uint8_t txrate2cfg*/	  3,
							/*uint8_t txrate1cfg*/	  2,
							/*uint8_t txrate0cfg*/	  1,
							/*uint8_t txserparif*/	  0,
							/*uint8_t txvalidbeh*/	  0);
		#endif
	#endif //CFG_BT


		// Settings for proper reception
	#if defined(CFG_BLE)
		ip_radiocntl1_forceiq_setf(1);
		ip_radiocntl1_dpcorr_en_setf(0x0);
		ASSERT_ERR(ip_radiocntl1_dpcorr_en_getf() == 0x0);
	#endif // CFG_BLE

	#if defined(CFG_BT)
		ip_radiocntl1_dpcorr_en_setf(0x1);
		ASSERT_ERR(ip_radiocntl1_dpcorr_en_getf() == 0x1);
	#endif // CFG_BT

	#if defined(CFG_BLE)
		// Force IQ mode for BLE only
		ip_radiocntl1_forceiq_setf(1);
	#endif //CFG_BLE
#endif /// from BK3633
}

void Delay_us(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 10; x++);
    }
}

/*void Delay(int num)
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 50; x++);
    }
}*/

void Delay_ms(int num) //sync from svn revision 18
{
    int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 3260; x++);
    }

}


