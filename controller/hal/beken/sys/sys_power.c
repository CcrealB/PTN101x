/***********************************************************************
 *
 * MODULE NAME:    sys_power.c
 * PROJECT CODE:   Bluetooth
 * DESCRIPTION:    Low Power Clock Activation
 * MAINTAINER:     Tom Kerwick
 * CREATION DATE:  10 Jan 2011
 *
 * LICENSE:
 *     This source code is copyright (c) 2011 Ceva Inc.
 *     All rights reserved.
 *
 ***********************************************************************/
#include "sys_config.h"
#include "sys_types.h"
#include "sys_const.h"
#include "sys_power.h"

#include "hw_delay.h"
#include "uslc_chan_ctrl.h"
#include "lslc_irq.h"
#include "sys_hal_features.h"
#include "sys_irq.h"

#include "sys_features.h"
#include "lmp_config.h"
#include "lmp_acl_container.h"

//#include "bautil.h"
//#include "driver_gpio.h"
//#include "driver_flash.h"
//#include "driver_icu.h"
#include "sys_init.h"
#include "app_beken_includes.h"
#if (BT_DUALMODE_RW == 1)
#include "rwip.h"
#endif

extern t_queue BTQueues[];
extern t_bt_ctrl_irq_cbs g_ctrl_irq_cbs;
extern t_bt_ctrl_sleep_cbs g_ctrl_sleep_cbs;
extern t_bt_beken_config g_beken_initial_config;
extern uint32_t  Check_Inquiry_Is_Busy(void);
extern RAM_CODE BOOL LMtmr_Timer_Expired(void);
#if (TRA_HCIT_GENERIC_SUPPORTED==1)
extern void (*hci_generic_exit_hc_callback)(void);
#endif

static BOOL syspwr_low_power_mode_active_;
static uint8_t LowFrequencyOscillatorAvailable = 1;

static uint32_t syspwr_num_frames_to_sleep_;
volatile uint32_t syspwr_num_frames_to_sleep_bak;

/* can be increased if CPU is slown down and execution takes time */
#define SYS_PWR_MIN_FRAMES_TO_SLEEP      4
#define SYS_PWR_AUX_TIMER_MIN_RESOLUTION 8

static volatile BOOL b_bt_wakeup_running  = FALSE;
#if (BK3000_RF_DPLL_OPTIMIZATION == 1)
DRAM_CODE void BK3000_Open_DPLL(uint8_t enable)
{
    if(enable)
    {
       REG_XVR_0x09 = XVR_analog_reg_save[9]&(~(1<<11));
    }
    else
    {
       REG_XVR_0x09 = XVR_analog_reg_save[9]|(1<<11);
    }
}
#endif
void Enable_Bk3000_GPIO_EINT_wakeup()
{
    unsigned int oldmask;
    oldmask = get_spr(SPR_VICMR(0));
    oldmask |= (1<<VIC_IDX_GPIO);
    set_spr(SPR_VICMR(0), oldmask);

    b_bt_wakeup_running=TRUE;
}

RAM_CODE void Disable_Bk3000_GPIO_EINT_wakeup()
{
    unsigned int oldmask;
    b_bt_wakeup_running=FALSE;
    oldmask = get_spr(SPR_VICMR(0));
    oldmask &= ~(1<<VIC_IDX_GPIO);
    set_spr(SPR_VICMR(0), oldmask);
}
void SYSpwr_Exit_LF_Mode(void)
{
    /* Step 1:CEVA wakup Req */
    REG_PMU_0x02 |= (1<<24);
	/* Step 2:Set LF MODE = 0 */
	HW_set_use_lf(0);
	/* Step 3:Wait on SLEEP STATUS != 0 */
	while (HW_get_sleep_status() != 0);
    /* Step 4:CEVA wakup Clear */
    REG_PMU_0x02 &= ~(1<<24);
	LSLCacc_Disable_Low_Power_Mode();
}

/*
 * FUNCTION:   SYSpwr_Is_Available_Sleep_System()
 * PURPOSE:    Returns TRUE if system is available to sleep.
 */
int SYSpwr_Is_Available_Sleep_System(void)
{
    /* No need to monitor LMP_OUT, L2CAP_OUT and SCO_OUT queue
     * because once placed data there, dequeued in LC ISR
     * so that halt scheduler at this point is no problem
     * even if containing any data
     */

    t_queue *q = BTQueues;

    /*
     * Check for HCI_COMMAND_Q, HCI_EVENT_Q and LMP_IN_Q
     */
    do
    {
        if(q->num_entries)      return(0/*Not Empty*/);
        q++;
    }
    while(q != &BTQueues[LMP_OUT_Q]);

    /*
     * Check for L2CAP_IN_Q
     */
    q = BTQueues + L2CAP_IN_Q;
    do
    {
        if(q->num_entries)      return(0/*Not Empty*/);
        q++;
    }
    while(q != &BTQueues[L2CAP_OUT_Q]);

    /*
     * Check for SCO_IN_Q
     */
#if (PRH_BS_CFG_SYS_SCO_VIA_HCI_SUPPORTED==1)
    q = BTQueues + SCO_IN_Q;
    do
    {
        if(q->num_entries)      return(0/*Not Empty*/);
        q++;
    }
    while(q != &BTQueues[SCO_OUT_Q]);
#endif

    if(Check_Inquiry_Is_Busy())
    {
        return 0;
    }

    if (!HW_get_sleep_status())
    {
        return 0;
    }
    /*
     * Allow sleep if timers not expired
     */
    return(!LMtmr_Timer_Expired());
}

/*****************************************************************************
 * FUNCTION:   SYSpwr_Halt_System
 * PURPOSE:    Turn off entire chip / Deep Sleep
 ****************************************************************************/
uint8_t syspwr_cpu_halt;
extern void gpio_button_wakeup_enable(void);
extern void SYSpwr_Wakeup_From_Sleep(void);
void SYSpwr_Halt_System(void)
{
    uint32_t cpu_flags, mask;
   
    if(SYSpwr_Is_Available_Sleep_System() && g_ctrl_sleep_cbs.bt_host_sleep_allowed()) 
    {
        if(!syspwr_cpu_halt)
        {
            if(syspwr_low_power_mode_active_) //for the bug:aux_timer wakeup interrupt happen before Close_26M_Clock() 
            {
                if(g_beken_initial_config.g_enable_32k_sleep && g_beken_initial_config.g_CPU_Halt_mode)
                {
                    #if (BT_DUALMODE_RW_SLEEP == 1)
                    g_ctrl_irq_cbs.SYSirq_Disable_Interrupts_Save_Flags_C(&cpu_flags, &mask);
                    rwip_sleep();
                    g_ctrl_irq_cbs.SYSirq_Interrupts_Restore_Flags_C(cpu_flags, mask);
                    if (!rwip_prevent_sleep_get(RW_DEEP_SLEEP))
                    return;
                    #endif
                                    
                    Enable_Bk3000_GPIO_EINT_wakeup();
                    g_ctrl_irq_cbs.SYSirq_Disable_Interrupts_Save_Flags_C(&cpu_flags, &mask);
                    
                    gpio_button_wakeup_enable();
                    g_ctrl_sleep_cbs.bt_set_cpu_sleep_mode();
                    ///debug_show_gpio(0x11);
                    syspwr_cpu_halt = 1;
                    g_ctrl_irq_cbs.SYSirq_Interrupts_Clear_Trig_Flags_C();
                    g_ctrl_irq_cbs.SYSirq_Interrupts_Restore_Flags_C(cpu_flags, mask);

                    /* ASIC Processing Sleep precedure... */
                    cpu_set_interrupts_enabled(0);
                    system_cpu_halt();
                    os_delay_us(20);
                    cpu_set_interrupts_enabled(1);

                    //debug_show_gpio(0x12);
                    SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);  
                    //debug_show_gpio(0x15);
                    os_delay_us(20);
                    CLEAR_WDT;                    
                    SYSpwr_Wakeup_From_Sleep();
                    
                    SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);    
                }
            }
        }
    }
    syspwr_low_power_mode_active_ = TRUE;
}

/*****************************************************************************
 * FUNCTION:   SYSpwr_Exit_Halt_System()
 * PURPOSE:    Wake entire chip back up (re-enable any gated clocks etc.)
 ****************************************************************************/
void SYSpwr_Exit_Halt_System(void)
{
    if(b_bt_wakeup_running)
        Disable_Bk3000_GPIO_EINT_wakeup();

    syspwr_low_power_mode_active_ = FALSE;
}

/*****************************************************************************
 * FUNCTION:   SYSpwr_Is_Available_Halt_System()
 * PURPOSE:    Returns TRUE if system is available to halt.
 ****************************************************************************/
#if (PRH_BS_CFG_SYS_LOW_POWER_MODE_SUPPORTED==1)
int SYSpwr_Is_Available_Halt_System(void)
{
    return (syspwr_low_power_mode_active_);
}
#endif

/*****************************************************************************
 * FUNCTION:   SYSpwr_LowFrequencyOscillatorAvailable()
 * PURPOSE:    Returns enum on low freq oscillator type available.
 ****************************************************************************/
extern uint32_t Get_Active_Acl_Num(void);
t_sys_lf_osc_type SYSpwr_LowFrequencyOscillatorAvailable(void)
{
    if(g_beken_initial_config.g_Disable_ACL_active_check_when_Sleep==0)
    {
        if(Get_Active_Acl_Num()>0)
            return SYS_LF_OSC_NONE;
    }

    return LowFrequencyOscillatorAvailable;
}

/*****************************************************************************
 * FUNCTION:   SYSpwr_Setup_Sleep_Timer()
 * PURPOSE:    Generic Function to setup sleep timer - can be implemented
 *             using any clock source capable of generating interrupt
 *
 *             this version uses AUX_TIMER
 ****************************************************************************/

void SYSpwr_Setup_Sleep_Timer(uint32_t max_num_frames_to_sleep)
{
#define NUM_FRAMES_TO_SLEEP (0x3FFF << 1)
#define AUX_T_SHIFT 1

    HW_set_aux_tim_intr_clr(1);
    if (max_num_frames_to_sleep == 1)
    {
        HW_set_aux_timer(SYS_PWR_AUX_TIMER_MIN_RESOLUTION>>AUX_T_SHIFT);  // 1*2.5ms
        max_num_frames_to_sleep = 0;
		syspwr_num_frames_to_sleep_bak = SYS_PWR_AUX_TIMER_MIN_RESOLUTION>>AUX_T_SHIFT;
    }
    else if (max_num_frames_to_sleep > NUM_FRAMES_TO_SLEEP)
    {
        HW_set_aux_timer(NUM_FRAMES_TO_SLEEP >> AUX_T_SHIFT); // aux_timer = n*2.5ms
        max_num_frames_to_sleep -= NUM_FRAMES_TO_SLEEP + SYS_PWR_MIN_FRAMES_TO_SLEEP;
		syspwr_num_frames_to_sleep_bak = NUM_FRAMES_TO_SLEEP >> AUX_T_SHIFT;
    }
    else
    {
        if (max_num_frames_to_sleep > (SYS_PWR_MIN_FRAMES_TO_SLEEP + SYS_PWR_AUX_TIMER_MIN_RESOLUTION))
        {
            HW_set_aux_timer((max_num_frames_to_sleep - SYS_PWR_MIN_FRAMES_TO_SLEEP) >> AUX_T_SHIFT); // aux_timer = n*2.5ms
			syspwr_num_frames_to_sleep_bak = (max_num_frames_to_sleep - SYS_PWR_MIN_FRAMES_TO_SLEEP) >> AUX_T_SHIFT;
        }
        else
        {
            HW_set_aux_timer(SYS_PWR_AUX_TIMER_MIN_RESOLUTION>>AUX_T_SHIFT); // aux_timer = n*2.5ms
			syspwr_num_frames_to_sleep_bak = SYS_PWR_AUX_TIMER_MIN_RESOLUTION>>AUX_T_SHIFT;
        }

        max_num_frames_to_sleep = 0;
    }

    HW_set_aux_tim_intr_mask(0);
    syspwr_num_frames_to_sleep_ = max_num_frames_to_sleep;
}

/*****************************************************************************
 * FUNCTION:   SYSpwr_Initialise
 * PURPOSE:
 ****************************************************************************/
#if (PRH_BS_CFG_SYS_LOW_POWER_MODE_SUPPORTED==1)
void SYSpwr_Initialise(void)
{
    syspwr_num_frames_to_sleep_ = 0;
    syspwr_low_power_mode_active_ = FALSE;

    /*
     * Leave low power oscillator disabled until enabled by TCI.
     */
    LowFrequencyOscillatorAvailable = 1;
}
#endif

/*****************************************************************************
 * FUNCTION:   SYSpwr_Handle_Early_Wakeup
 * PURPOSE:
 ****************************************************************************/
void SYSpwr_Handle_Early_Wakeup(void)
{
    /* wake chip back up to check on UART status */
    SYSpwr_Exit_Halt_System();

    /*
     * DEPLOYMENT TEAM need to put code into this if() to check for HCI transport
     * activity. If TRUE, then subsequent 'naps' for this sleep period will be aborted
     */
    if (!syspwr_num_frames_to_sleep_)
    {
        /*
         * disable the sleep timer
         * as Setup_Sleep_Timer above uses AUX_TIMER, must disable it here
         */
        HW_set_aux_tim_intr_mask(1);
        LSLCirq_Disable_Aux_Tim_Intr();

        USLCchac_Wakeup(); /* move onto next state, wake system up... */
    }
    else
    {
        SYSpwr_Setup_Sleep_Timer(syspwr_num_frames_to_sleep_);
        SYSpwr_Halt_System();
    }
}

/*****************************************************************************
 * FUNCTION:   SYSpwr_Is_Low_Power_Mode_Active
 * PURPOSE:
 ****************************************************************************/
#if (PRH_BS_CFG_SYS_LOW_POWER_MODE_SUPPORTED==1)
BOOL SYSpwr_Is_Low_Power_Mode_Active(void)
{
    return (syspwr_low_power_mode_active_);
}
#endif

/*****************************************************************************
 * FUNCTION:   SYSpwr_Set_LowFrequencyOscillatorAvailable_Value()
 * PURPOSE:    set if oscillator is available.
 *             Returns TRUE if request was processed.
 ****************************************************************************/
BOOL SYSpwr_Set_LowFrequencyOscillatorAvailable_Value(uint8_t osc_available)
{
    if((SYS_LF_OSCILLATOR_PRESENT==0) && (osc_available==1))
        return FALSE;
    LowFrequencyOscillatorAvailable = osc_available;
    return TRUE;
}

/*****************************************************************************
 * SYSpwr_Get_Min_Frames_To_Sleep
 * minimum amount of frames that device can be put in deep sleep
 ****************************************************************************/
uint32_t SYSpwr_Get_Min_Frames_To_Sleep(void)
{
    return (SYS_PWR_MIN_FRAMES_TO_SLEEP + SYS_PWR_AUX_TIMER_MIN_RESOLUTION);
}
