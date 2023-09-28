/**
 * @file   bk3000_rf.c
 * @author  <Lab@beken>
 * @date   Tue Nov 18 09:04:52 2014
 *
 * @brief
 *
 *
 */

#include "sys_types.h"
#include "lc_types.h"
#include "PTN101_calibration.h"
#include "config.h"
#include "app_beken_includes.h"
#include "drv_system.h"

uint32_t XVR_analog_reg_save[16]={0};
uint32_t XVR_reg_0x24_save = 0;

#if (PRH_BS_CFG_SYS_LMP_EDR_SUPPORTED==1)
extern void LSLCacc_Set_EDR_Delay(uint32_t reg);
#endif

void set_XVR_reg_0x24_save(uint32_t val)
{
    //os_printf("set_XVR_reg_0x24_save: 0x%x_0x%x\n",XVR_reg_0x24_save, val);
    XVR_reg_0x24_save = val;
}

void BK3000_RF_Initial(void)
{
    REG_XVR_0x00 = XVR_analog_reg_save[0 ] = 0x99B70113;//RK@20200609
    REG_XVR_0x01 = XVR_analog_reg_save[1 ] = 0x99B70330;
    REG_XVR_0x02 = XVR_analog_reg_save[2 ] = 0x7C501806;//0x7C501806;//0x7C501A06;
    REG_XVR_0x03 = XVR_analog_reg_save[3 ] = 0xC8780FFE;
    REG_XVR_0x04 = XVR_analog_reg_save[4 ] = 0x00000000;
    REG_XVR_0x05 = XVR_analog_reg_save[5 ] = 0x91684460;
    REG_XVR_0x06 = XVR_analog_reg_save[6 ] = 0x065C9004;//0x061CD004;
    REG_XVR_0x07 = XVR_analog_reg_save[7 ] = 0x1F082DFF;//0x06886DFF;
    REG_XVR_0x08 = XVR_analog_reg_save[8 ] = 0x9C10B1C3;
    REG_XVR_0x09 = XVR_analog_reg_save[9 ] = 0x1BEFF6D4;
    REG_XVR_0x0A = XVR_analog_reg_save[10] = 0x24B6886D;//0x24B68869;//0x24968869;
    REG_XVR_0x0B = XVR_analog_reg_save[11] = 0xF08A61F4;//0xF08A61B4;
    REG_XVR_0x0C = XVR_analog_reg_save[12] = 0xF83361E0;
    REG_XVR_0x0D = XVR_analog_reg_save[13] = 0x000007DF;
    REG_XVR_0x0E = XVR_analog_reg_save[14] = 0x00B09358;  //must set when usb int
    REG_XVR_0x0F = XVR_analog_reg_save[15] = 0x00000000;
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
#if (BT_DUALMODE_RW == 1)
    REG_XVR_0x24 = 0x00F4DF61;//bit21~23: RW baseband generate xvr syncwin/txpower/chnn
#else
	REG_XVR_0x24 = 0x0014DF61;
#endif	
    XVR_reg_0x24_save = REG_XVR_0x24;
    REG_XVR_0x25 = 0x00000000;
    REG_XVR_0x26 = 0x08000000;

    /* XVR 27 For software define:
     * bit   0-7: AFH threshold; typical = 0x20;
     * bit  8-11: edr_rx_edr_delay typical = 0x01;
     * bit 12-15: edr_tx_edr_delay typical = 0x04;
     * bit 16-19: Tx power of tws-master
     */
    REG_XVR_0x27 = 0x00044120;
    REG_XVR_0x28 = 0x030C2333;
    REG_XVR_0x29 = 0x0D041100;
    REG_XVR_0x2A = 0x01503042;//0x01503842;
#if (BT_DUALMODE_RW == 1)	
    REG_XVR_0x2B = 0x4A4C0000;//bit16~31: RW baseband slot_ctrl delay txslot_time(us)
#else
	REG_XVR_0x2B = 0x00000000;
#endif	
    REG_XVR_0x2C = 0x00000000;
    REG_XVR_0x2D = 0x002F0847;//0x002F4847;//0x00284841;
    REG_XVR_0x30 = 0xA848181B;//0xA832581B;//0xA848741B;//0xA848741B;//0xA832581B; //improve tx evm
    REG_XVR_0x31 = 0x0000006E;
    REG_XVR_0x32 = 0xF4F808FC;
    REG_XVR_0x33 = 0x00001800;
    REG_XVR_0x38 = 0x2E1603FF;
    REG_XVR_0x39 = 0xA0802860;
    REG_XVR_0x3A = 0x00000000;// Don't bypass tracking DC,0x00000000;0x00004000;
    REG_XVR_0x3B = 0x09345288;//0x09345288;0x09344288
    REG_XVR_0x3C = 0x208403D1;//0x308403D1;
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

    //FIXME@liaixing
    REG_XVR_0x25 = 0x2400;
    sys_delay_ms(1);
    REG_XVR_0x08 = XVR_analog_reg_save[8] | (1 << 18);
    sys_delay_ms(1);
    REG_XVR_0x08 = XVR_analog_reg_save[8];
    sys_delay_ms(1);
    REG_XVR_0x25 = 0;
    

#if 1
    BK3000_set_clock(CPU_CLK_XTAL, 1); 
    tx_calibration_init();
    tx_dc_calibration();
    //tx_dc_calibration();
    tx_output_power_calibration();
    tx_q_const_iqcal_p_calibration();
    tx_iq_gain_imbalance_calibration();
    tx_iq_phase_imbalance_calibration();
    tx_calibration_deinit();
    rf_IF_filter_cap_calibration();    
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV); 
#endif
    LOG_I(XVR,"RF.Cali:XVR 0x32=0x%x,0x33[15:0]=0x%x,(filter_cap)0x05[19:14] =0x%x\r\n", REG_XVR_0x32,REG_XVR_0x33&0xffff,(XVR_analog_reg_save[5]>>14)&0x3f);
    saradc_start_working();
}

void BK3000_RF_Dut_ReWrite(uint8_t option)
{
    if(option == 1)  // RF_FOR_DUT
    {
        BK3000_set_AON_voltage(5);
        REG_XVR_0x00 = XVR_analog_reg_save[0 ] = 0x99370113;
        REG_XVR_0x09 = XVR_analog_reg_save[9 ] = 0x1BEFF6FF;
        REG_XVR_0x0B = XVR_analog_reg_save[11] = 0xF08A61F4;
        REG_XVR_0x0C = XVR_analog_reg_save[12] = 0xf8336120;

        REG_XVR_0x24 = XVR_reg_0x24_save = 0x00F4DF61;  // tx power
        REG_XVR_0x27 = 0x00044020;

        BK3000_set_clock(CPU_CLK_XTAL, 1);
        tx_calibration_init();
        tx_output_power_calibration();
        tx_iq_gain_imbalance_calibration();
        tx_calibration_deinit();
        BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
        LOG_I(XVR,"RF.Cali:XVR 0x32=0x%x,0x33[15:0]=0x%x,(filter_cap)0x05[19:14] =0x%x\r\n", REG_XVR_0x32,REG_XVR_0x33&0xffff,(XVR_analog_reg_save[5]>>14)&0x3f);

        REG_XVR_0x0C = XVR_analog_reg_save[12] = 0xf8326100;  //0xF83261E0;
        REG_XVR_0x24 = XVR_reg_0x24_save = 0x0014DF02;  // tx power

        #if (PRH_BS_CFG_SYS_LMP_EDR_SUPPORTED==1)
        LSLCacc_Set_EDR_Delay(REG_XVR_0x27);
        #endif
    }
    else if(option == 2)   // RF_FOR_PROJECT
    {
        
    }
}

void BKxvr_Set_Tx_Power(uint32_t pwr)
{
    XVR_reg_0x24_save = REG_XVR_0x24;
    XVR_reg_0x24_save &= ~(0x0f << 8);
    XVR_reg_0x24_save |= ((pwr & 0x0f)<<8);
    REG_XVR_0x24 = XVR_reg_0x24_save;   
}

void xvr_reg_if2M_set(uint32_t val)
{
    if(val) // select 2M
        REG_XVR_0x06 = XVR_analog_reg_save[6] |= 1<<28;
    else    // select 1M
        REG_XVR_0x06 = XVR_analog_reg_save[6] &= ~(1<<28);
}