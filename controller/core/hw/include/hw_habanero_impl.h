#ifndef _PARTHUS_HW_HABANERO_IMPL_
#define _PARTHUS_HW_HABANERO_IMPL_

/******************************************************************************
 * MODULE NAME:    hw_habanero_impl.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    Inline implementation of register accesses of Habanero API
 * MAINTAINER:     Ivan Griffin
 * DATE:           20 March 2000
 *
 * SOURCE CONTROL: $Id: hw_habanero_impl.h,v 1.11 2013/04/08 11:03:01 tomk Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *    20 March 2000          IG           Initial Version
 *
 * NOTES TO USERS:
 ******************************************************************************/
#include "hw_macro_defs.h"

#if (PRAGMA_INLINE==1)
#pragma inline(HWhab_Set_Syncword_Ref,\
HWhab_Set_Sync_Error,\
HWhab_Set_Rx_Mode,\
HWhab_Set_DADC_Bypass,\
HWhab_Set_RADC_Bypass,\
HWhab_Set_RADC_Num,\
HWhab_Set_FDB_DIS,\
HWhab_Set_Sym_Gn,\
HWhab_Set_Sym_Enb,\
HWhab_Set_Sym_Mask,\
HWhab_Set_Cmb_C1,\
HWhab_Set_Cmb_C2,\
HWhab_Set_OCL_DC_Offset,\
HWhab_Set_OCL_Feedback_Mode,\
HWhab_Set_OCL_Low_Gain,\
HWhab_Set_OCL_High_Gain,\
HWhab_Get_Syncword_Ex,\
HWhab_Get_Sync_Error,\
HWhab_Get_Rx_Mode,\
HWhab_Get_DADC_Bypass,\
HWhab_Get_RADC_Bypass,\
HWhab_Get_RADC_Num,\
HWhab_Get_FDB_DIS,\
HWhab_Get_Sym_Gn,\
HWhab_Get_Sym_Enb,\
HWhab_Get_Sym_Mask,\
HWhab_Get_Cmb_C1,\
HWhab_Get_Cmb_C2,\
HWhab_Get_OCL_DC_Offset,\
HWhab_Get_OCL_Feedback_Mode,\
HWhab_Get_OCL_Low_Gain,\
HWhab_Get_OCL_High_Gain,\
HWhab_Set_Gaussian_Coeff,\
HWhab_Set_DAC_Bypass,\
HWhab_Set_DAC_Ref,\
HWhab_Set_Gaussian_Offset,\
HWhab_Get_Gaussian_Coeff,\
HWhab_Get_DAC_Bypass,\
HWhab_Get_DAC_Ref,\
HWhab_Get_Gaussian_Offset,\
HWhab_Set_RIF_Intr_Mask0,\
HWhab_Set_RIF_Intr_Mask1,\
HWhab_Set_RIF_Intr_Mask2,\
HWhab_Set_RIF_Intr_Mask3,\
HWhab_Set_LTR_Intr_Mask,\
HWhab_Set_RIF_Intr_Clr,\
HWhab_Set_LTR_Intr_Clr,\
HWhab_Get_RIF_Intr_Mask0,\
HWhab_Get_RIF_Intr_Mask1,\
HWhab_Get_RIF_Intr_Mask2,\
HWhab_Get_RIF_Intr_Mask3,\
HWhab_Get_LTR_Intr_Mask,\
HWhab_Get_RIF_Intr_Clr,\
HWhab_Get_LTR_Intr_Clr,\
HWhab_Set_PHY_Cfg,\
HWhab_Get_PHY_Cfg,\
HWhab_Get_RADC_RSSI,\
HWhab_Get_LTR_Long_Period,\
HWhab_Get_LTR_Intr,\
HWhab_Get_RIF_Intr,\
HWhab_Get_GIO_Status,\
HWhab_Get_Core_Minor_Rev,\
HWhab_Get_Core_Major_Rev,\
HWhab_Set_GIO_Ctrl,\
HWhab_Set_Rst_Code,\
HWhab_Get_Rst_Code,\
HWhab_Set_GIOs_Hi_Early,\
HWhab_Set_GIOs_Hi_Late,\
HWhab_Set_GIOs_Lo_Early,\
HWhab_Set_GIOs_Lo_Late)
#endif

/***************************************************************************
 * Receive Control Registers
 ***************************************************************************/
__INLINE__ void HWhab_Set_Syncword_Ref(const uint32_t* syncword)
{
    mSetHWEntry64(HAB_SYNC, syncword);
}

__INLINE__ void HWhab_Get_Syncword_Ex(uint32_t *syncword)
{
    mGetHWEntry64_Ex(HAB_SYNC, syncword);
}

__INLINE__ void HWhab_Set_Sync_Error(const uint8_t sync_error)
{
    mSetHWEntry(HAB_SYNC_ERR, sync_error);
}

__INLINE__ uint8_t HWhab_Get_Sync_Error(void)
{
    return (uint8_t)mGetHWEntry(HAB_SYNC_ERR);
}

#define HWradio_Set_Rx_Mode HWhab_Set_Rx_Mode

__INLINE__ void HWhab_Set_Rx_Mode(const t_HWradio_Rx_Mode rx_mode)
{
    mSetHWEntry(HAB_RX_MODE, rx_mode);
}

__INLINE__ t_HWradio_Rx_Mode HWhab_Get_Rx_Mode(void)
{
    return (t_HWradio_Rx_Mode)mGetHWEntry(HAB_RX_MODE);
}

__INLINE__ void HWhab_Set_DADC_Bypass(const BOOL state)
{
    mSetHWEntry(HAB_DADC_BYP, state);
}

__INLINE__ BOOL HWhab_Get_DADC_Bypass(void)
{
    return (BOOL)mGetHWEntry(HAB_DADC_BYP);
}

__INLINE__ void HWhab_Set_RADC_Bypass(const BOOL state)
{
    mSetHWEntry(HAB_RADC_BYP, state);
}

__INLINE__ BOOL HWhab_Get_RADC_Bypass(void)
{
    return (BOOL)mGetHWEntry(HAB_RADC_BYP);
}

__INLINE__ void HWhab_Set_RADC_Num(const BOOL state)
{
    mSetHWEntry(HAB_RADC_NUM, state);
}

__INLINE__ void HWhab_Set_FDB_DIS(const BOOL state)
{
    mSetHWEntry(HAB_FDB_DIS, state);
}

__INLINE__ BOOL HWhab_Get_RADC_Num(void)
{
    return (BOOL)mGetHWEntry(HAB_RADC_NUM);
}

__INLINE__ BOOL HWhab_Get_FDB_DIS(void)
{
    return (BOOL)mGetHWEntry(HAB_FDB_DIS);
}

__INLINE__ void HWhab_Set_Sym_Gn(const uint8_t gain)
{
    mSetHWEntry(HAB_SYM_GN, gain);
}

__INLINE__ uint8_t HWhab_Get_Sym_Gn(void)
{
    return (uint8_t)mGetHWEntry(HAB_SYM_GN);
}

__INLINE__ void HWhab_Set_Sym_Enb(const BOOL state)
{
    mSetHWEntry(HAB_SYM_ENB, state);
}

__INLINE__ BOOL HWhab_Get_Sym_Enb(void)
{
    return (BOOL)mGetHWEntry(HAB_SYM_ENB);
}

__INLINE__ void HWhab_Set_Sym_Mask(const BOOL state)
{
    mSetHWEntry(HAB_SYM_MSK, state);
}

__INLINE__ BOOL HWhab_Get_Sym_Mask(void)
{
    return (BOOL)mGetHWEntry(HAB_SYM_MSK);
}

__INLINE__ void HWhab_Set_Cmb_C1(const uint8_t c1)
{
    mSetHWEntry(HAB_CMB_C1, c1);
}

__INLINE__ uint8_t HWhab_Get_Cmb_C1(void)
{
    return (uint8_t)mGetHWEntry(HAB_CMB_C1);
}

__INLINE__ void HWhab_Set_Cmb_C2(const uint8_t c2)
{
    mSetHWEntry(HAB_CMB_C2, c2);
}

__INLINE__ uint8_t HWhab_Get_Cmb_C2(void)
{
    return (uint8_t)mGetHWEntry(HAB_CMB_C2);
}

__INLINE__ void HWhab_Set_OCL_DC_Offset(const uint8_t offset)
{
    mSetHWEntry(HAB_OCL_DC, offset);
}

__INLINE__ uint8_t HWhab_Get_OCL_DC_Offset(void)
{
    return (uint8_t)mGetHWEntry(HAB_OCL_DC);
}

__INLINE__ void HWhab_Set_OCL_Feedback_Mode(const BOOL state)
{
    mSetHWEntry(HAB_OCL_FB_MODE, state);
}

__INLINE__ BOOL HWhab_Get_OCL_Feedback_Mode(void)
{
    return (BOOL)mGetHWEntry(HAB_OCL_FB_MODE);
}

__INLINE__ void HWhab_Set_OCL_Low_Gain(const uint8_t low_gain)
{
    mSetHWEntry(HAB_OCL_LGN, low_gain);
}

__INLINE__ uint8_t HWhab_Get_OCL_Low_Gain(void)
{
    return (uint8_t)mGetHWEntry(HAB_OCL_LGN);
}

__INLINE__ void HWhab_Set_OCL_High_Gain(const uint8_t high_gain)
{
    mSetHWEntry(HAB_OCL_HGN, high_gain);
}

__INLINE__ uint8_t HWhab_Get_OCL_High_Gain(void)
{
    return (uint8_t)mGetHWEntry(HAB_OCL_HGN);
}



/***************************************************************************
 * Transmit Control Registers
 ***************************************************************************/
__INLINE__ void HWhab_Set_Gaussian_Coeff(const uint8_t index, const uint8_t value) 
{
}

__INLINE__ void HWhab_Set_DAC_Bypass(const BOOL state) 
{
    mSetHWEntry(HAB_DAC_BYP, state);
}

__INLINE__ void HWhab_Set_DAC_Ref(const uint8_t voltage_ref) 
{
    mSetHWEntry(HAB_DACREF, voltage_ref);
}

__INLINE__ void HWhab_Set_Gaussian_Offset(const uint16_t offset) 
{
    mSetHWEntry(HAB_GAU_OFFSET, offset);
}

#if 0
__INLINE__ uint8_t HWhab_Get_Gaussian_Coeff(const uint8_t index)
{
}
#endif

__INLINE__ BOOL HWhab_Get_DAC_Bypass(void)
{
    return mGetHWEntry(HAB_DAC_BYP);
}

__INLINE__ uint8_t HWhab_Get_DAC_Ref(void)
{
    return mGetHWEntry(HAB_DACREF);
}

__INLINE__ uint16_t HWhab_Get_Gaussian_Offset(void)
{
    return mGetHWEntry(HAB_GAU_OFFSET);
}



/***************************************************************************
 * Interrupt Control Registers
 ***************************************************************************/
__INLINE__ void HWhab_Set_RIF_Intr_Mask0(const BOOL state) 
{
    mSetHWEntry(HAB_RIF_INTR_MSK0, state);
}

__INLINE__ BOOL HWhab_Get_RIF_Intr_Mask0(void)
{
    return (BOOL)mGetHWEntry(HAB_RIF_INTR_MSK0);
}

__INLINE__ void HWhab_Set_RIF_Intr_Mask1(const BOOL state) 
{
    mSetHWEntry(HAB_RIF_INTR_MSK1, state);
}

__INLINE__ BOOL HWhab_Get_RIF_Intr_Mask1(void)
{
    return (BOOL)mGetHWEntry(HAB_RIF_INTR_MSK1);
}

__INLINE__ void HWhab_Set_RIF_Intr_Mask2(const BOOL state) 
{
    mSetHWEntry(HAB_RIF_INTR_MSK2, state);
}

__INLINE__ BOOL HWhab_Get_RIF_Intr_Mask2(void)
{
    return (BOOL)mGetHWEntry(HAB_RIF_INTR_MSK2);
}

__INLINE__ void HWhab_Set_RIF_Intr_Mask3(const BOOL state) 
{
    mSetHWEntry(HAB_RIF_INTR_MSK3, state);
}

__INLINE__ BOOL HWhab_Get_RIF_Intr_Mask3(void)
{
    return (BOOL)mGetHWEntry(HAB_RIF_INTR_MSK3);
}

__INLINE__ void HWhab_Set_LTR_Intr_Mask(const BOOL state) 
{
    mSetHWEntry(HAB_LTR_INTR_MSK, state);
}

__INLINE__ BOOL HWhab_Get_LTR_Intr_Mask(void)
{
    return (BOOL)mGetHWEntry(HAB_LTR_INTR_MSK);
}


__INLINE__ uint8_t HWhab_Get_RIF_Intr_Clr(void)
{
    return (uint8_t)mGetHWEntry(HAB_RIF_INTR_CLR);
}

__INLINE__ void HWhab_Set_RIF_Intr_Clr(const uint8_t clear_value) 
{
    mSetHWEntry(HAB_RIF_INTR_CLR, clear_value);
}

__INLINE__ void HWhab_Set_LTR_Intr_Clr(const uint8_t value)
{
    mSetHWEntry(HAB_LTR_INTR_CLR, value);
}

__INLINE__ uint8_t HWhab_Get_LTR_Intr_Clr(void)
{
    return (uint8_t)mGetHWEntry(HAB_LTR_INTR_CLR);
}


/*
 * Interface Configuration Registers
 */
__INLINE__ void HWhab_Set_PHY_Cfg(const uint32_t magic_phy_value)
{
    mSetHWEntry32(HAB_PHY_CFG, magic_phy_value);
}

__INLINE__ uint32_t HWhab_Get_PHY_Cfg(void)
{
    return (uint32_t)(mGetHWEntry32(HAB_PHY_CFG));
}



/***************************************************************************
 * Status Registers
 ***************************************************************************/
__INLINE__ uint8_t HWhab_Get_RADC_RSSI(void) 
{
    return (uint8_t)mGetHWEntry(HAB_RADC_RSSI);
}

__INLINE__ uint8_t HWhab_Get_LTR_Long_Period(void) 
{
    return (uint8_t)mGetHWEntry(HAB_LTR_LONG_PERIOD);
}

__INLINE__ uint8_t HWhab_Get_LTR_Intr(void) 
{
    return (uint8_t)mGetHWEntry(HAB_LTR_INTR);
}

__INLINE__ uint8_t HWhab_Get_RIF_Intr(void) 
{
    return (uint8_t)mGetHWEntry(HAB_RIF_INTR);
}

__INLINE__ uint8_t HWhab_Get_GIO_Status(void)
{
    return (uint8_t)mGetHWEntry(HAB_GIO_STATUS);
}

__INLINE__ uint8_t HWhab_Get_Core_Minor_Rev(void)
{
    return (uint8_t)mGetHWEntry(HAB_MINOR_REV);
}

__INLINE__ uint8_t HWhab_Get_Core_Major_Rev(void)
{
    return (uint8_t)mGetHWEntry(HAB_MAJOR_REV);
}


/***************************************************************************
 * GIO Control Registers
 ***************************************************************************/
/* #error GIO Control Registers to be done */
__INLINE__ uint8_t HWhab_Set_GIO_Ctrl(const uint8_t index, const uint8_t value)
{
    return 0; /* not done yet */
}


/***************************************************************************
 * Reset Control Registers
 ***************************************************************************/
__INLINE__ void HWhab_Set_Rst_Code(const uint8_t code)
{
    mSetHWEntry(HAB_RST_CODE, code);
}

__INLINE__ uint8_t HWhab_Get_Rst_Code(void)
{
    return (uint8_t)mGetHWEntry(HAB_RST_CODE);
}

#if (PRH_BS_CFG_SYS_HW_WINDOW_WIDENING_SUPPORTED==1)
/***************************************************************************
 * WIN_EXT GIO Early/Late Control Registers
 ***************************************************************************/
__INLINE__ void HWhab_Set_GIOs_Hi_Early(const uint16_t mask)
{
	mSetHWEntry(HAB_GIO_HIGH_EARLY_EN, mask);
}

__INLINE__ void HWhab_Set_GIOs_Hi_Late(const uint16_t mask)
{
	mSetHWEntry(HAB_GIO_HIGH_LATE_EN, mask);
}

__INLINE__ void HWhab_Set_GIOs_Lo_Early(const uint16_t mask)
{
	mSetHWEntry(HAB_GIO_LOW_EARLY_EN, mask);
}

__INLINE__ void HWhab_Set_GIOs_Lo_Late(const uint16_t mask)
{
	mSetHWEntry(HAB_GIO_LOW_LATE_EN, mask);
}
#endif

#endif
