/***********************************************************************
 * MODULE NAME:    hw_codec.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Code to interface I2C CODEC DRIVER on Terasic
 * MAINTAINER:     Tom Kerwick
 * DATE:           24 May 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *    
 **********************************************************************/
#include "sys_config.h"
#include "hc_const.h"
#include "hw_codec.h"
#include "hw_lc.h"
#include "lmp_sco_container.h"
#include "hw_delay.h"

static uint16_t _HWcodec_voice_setting;

/******************************************************************************
 *
 * FUNCTION:  HWcodec_Initialise
 * PURPOSE:   Initialises the I2C CODEC and related Tabasco REGS.
 *
 ***********************************************************************/
void HWcodec_Initialise(void)
{
    _HWcodec_voice_setting =  (
        ( (LINEAR_IC)        << 8) |
        ( (SIGN_MAG_IDF)     << 6) |
        ( (SIXTEEN_BIT_ISS)  << 5) |
        ( (3)                << 2) |
        ( (CVSD_ACF)         << 0) | 
		(PRH_BS_HCI_VOICE_SETTING_FOR_SCO_VIA_HCI)
		);
}

/***********************************************************************
 *
 * FUNCTION:  HWcodec_Set_Voice_Setting
 * PURPOSE:   Reconfigure the CODEC to the specified INPUT CODING.
 *
 ***********************************************************************/
void HWcodec_Set_Voice_Setting(uint16_t new_voice_setting)
{
	_HWcodec_voice_setting = new_voice_setting;
}

/***********************************************************************
 *
 * FUNCTION:  HWcodec_Is_Valid_SCO_Conversion
 * PURPOSE:   Indicate whether HW supports the requested conversions.
 *
 ***********************************************************************/
BOOL HWcodec_Is_Valid_SCO_Conversion(uint16_t voice_setting)
{
    BOOL is_valid_sco_conversion = FALSE;

    t_input_coding input_coding =   LMscoctr_Get_Input_Coding(voice_setting);
    uint8_t linear_pcm_sample_size = LMscoctr_Get_Input_Sample_Size(voice_setting);
    t_air_coding_format air_coding = LMscoctr_Get_Air_Coding(voice_setting);

    if (voice_setting & PRH_BS_HCI_VOICE_SETTING_FOR_SCO_VIA_HCI)
    {
   	    if (air_coding == TRANS_ACF)
   	    {
   	    	is_valid_sco_conversion = TRUE;
   	    }
   	    else if ((input_coding == LINEAR_IC) && (air_coding == CVSD_ACF)
        		&& (linear_pcm_sample_size >= 12))
        {
        	is_valid_sco_conversion = TRUE;
        }
        else if ((input_coding == LINEAR_IC) && (linear_pcm_sample_size==16))
        {
			is_valid_sco_conversion = TRUE;
        }
        else if ((input_coding == U_LAW_IC) || (input_coding == A_LAW_IC))
		{
			is_valid_sco_conversion = TRUE;
		}
		else if ((input_coding == CVSD_IC) && (air_coding == CVSD_ACF))
		{
			is_valid_sco_conversion = TRUE;
		}
    }
    else /* i2c codec supports 16 bit linear input coding only */
    {
		if ((input_coding == LINEAR_IC) && (linear_pcm_sample_size==16))
			is_valid_sco_conversion = TRUE;
    }

    return is_valid_sco_conversion;
}

/***********************************************************************
 *
 * FUNCTION: HWcodec_Get_VCI_Clk_Sel
 *
 ***********************************************************************/
uint8_t HWcodec_Get_VCI_Clk_Sel(void)
{
    return 2;
}

/***********************************************************************
 *
 * FUNCTION: HWcodec_Get_VCI_Clk_Sel_Map
 *
 ***********************************************************************/
uint8_t HWcodec_Get_VCI_Clk_Sel_Map(void)
{
    return 1;
}

/*******************************************************************************
 *
 * FUNCTION: HWcodec_Enable
 *
 ******************************************************************************/
void HWcodec_Enable(void)
{

#if (PRH_BS_CFG_SYS_ESCO_VIA_VCI_SUPPORTED==1)
	/*
	 * If support SCO and eSCO via VCI need to enable the VCI FIFO functionality
	 * else can use the standard VCI interface for SCO connections only.
	 */
	/* uint32_t _dummy_read; */

	HW_set_vci_rgf_fifo_reset(1);
	HW_set_vci_rgf_mode_enable(1);
	HW_set_vci_rgf_fifo_reset(0);

	HW_set_vci_rgf_fifo_16bit_mode(1);

	HW_set_vci_tx_fifo_threshold(10);
	HW_set_vci_rx_fifo_threshold(10);

	/* The first read access from the VCI FIFO is garbage - as the reading of the
	 * register triggers a new read from the FIFO, i.e. data is from previous read.
	 */
	/* _dummy_read =  */HW_read_vci_tx_fifo_data();
#endif
    mSetHWBit(JAL_SYNC_DIR);
}

/*******************************************************************************
 *
 * FUNCTION: HWcodec_Disable
 *
 ******************************************************************************/
void HWcodec_Disable(void)
{
#if (PRH_BS_CFG_SYS_ESCO_VIA_VCI_SUPPORTED==1)
	HW_set_vci_rgf_mode_enable(0);
#endif

}

/*******************************************************************************
 *
 * FUNCTION: HWcodec_Increase_Volume
 *
 ******************************************************************************/
void HWcodec_Increase_Volume(void)
{
    // feature not required at present
}

/*******************************************************************************
 *
 * FUNCTION: HWcodec_Decrease_Volume
 *
 ******************************************************************************/
void HWcodec_Decrease_Volume(void)
{
    // feature not required at present
}
uint16_t HWcodec_Get_Voice_Setting(void)
{
    return _HWcodec_voice_setting;
}
