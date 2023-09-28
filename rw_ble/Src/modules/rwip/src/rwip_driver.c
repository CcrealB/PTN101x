/**
****************************************************************************************
*
* @file rwip_driver.c
*
* @brief RW IP Driver SW module used to manage common IP features.
*
* Copyright (C) RivieraWaves 2009-2015
*
****************************************************************************************
*/

/**
 ****************************************************************************************
 * @addtogroup RW IP SW main module
 * @ingroup ROOT
 * @brief The RW IP SW main module.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // RW SW configuration

#include <string.h>          // for mem* functions
#include "rwip.h"            // RW definitions
#include "rwip_int.h"        // RW internal definitions
#include "arch.h"            // Platform architecture definition

#if (NVDS_SUPPORT)
#include "nvds.h"            // NVDS definitions
#endif // NVDS_SUPPORT

#if (H4TL_SUPPORT)
#include "h4tl.h"            // H4TL definition
#endif //H4TL_SUPPORT

#include "dbg.h"             // debug definition
#include "ke_mem.h"          // for AES client management

#include "ke.h"              // To check if a kernel event is programmed
#include "ke_event.h"

#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
#include "sch_alarm.h"       // for the half slot target ISR
#include "sch_arb.h"         // for the half us target ISR
#include "sch_prog.h"        // for the fifo/clock ISRs
//#include "led.h"
#include "reg_ipcore.h"
#include "aes.h"             // AES result function

#if (BLE_EMB_PRESENT)
#include "rwble.h"           // for sleep and wake-up specific functions
#include "lld.h"             // for AES encryption handler
#endif // (BLE_EMB_PRESENT)
#if (BT_EMB_PRESENT)
#include "rwbt.h"            // for sleep and wake-up specific functions
#include "ld.h"              // for clock interrupt handler
#endif // (BT_EMB_PRESENT)
#elif  (BLE_HOST_PRESENT)
#include "timer.h"
#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

#include "co_math.h"         // min/max macros

#include "reg_blecore.h"
#include "reg_ipcore.h"

#include "app_beken_includes.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
/// Sleep Duration Value in periodic wake-up mode in Half slots
#define MAX_SLEEP_DURATION_PERIODIC_WAKEUP      0x0640  // 0.5s

/// Sleep Duration Value in external wake-up mode
#define MAX_SLEEP_DURATION_EXTERNAL_WAKEUP      0x7D00  //10s

/**
 * Inverse an intra-half-slot value (in half-us), from/to following formats:
 *   - A: elapsed time from the previous half-slot (in half-us)
 *   - B: remaining time to the next half-slot (in half-us)
 * The function from A to B or from B to A.
 *  ____________________________________________________________________________________________________
 *     Half-slot N-1            |             Half-slot N              |             Half-slot N+1
 *  ____________________________|______________________________________|________________________________
 *                              |<---------- A ---------->|<---- B --->|
 *  ____________________________|______________________________________|________________________________
 */
#define HALF_SLOT_INV(x)  (HALF_SLOT_SIZE - x - 1)

#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

/// RW SW environment
struct rwip_env_tag        rwip_env;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
/**
 ****************************************************************************************
 * @brief Converts a duration in lp cycles into a duration in half us.
 *
 * The function converts a duration in lp cycles into a duration is half us, according to the
 * low power clock frequency (32768Hz or 32000Hz).
 *
 * To do this the following formula are applied:
 *
 *   Tus = (x*30.517578125)*2 = (30*x + x/2 + x/64 + x/512)*2 = (61*x + (x*8 + x)/256) for a 32.768kHz clock or
 *   Tus = (x*31.25)*2        = (31*x + x/4) * 2              = (62*x + x/2)           for a 32kHz clock
 *
 * @param[in]     lpcycles    duration in lp cycles
 * @param[in|out] error_corr  Insert and retrieve error created by truncating the LP Cycle Time to a half us (32kHz: 1/2 half us | 32.768kHz: 1/256 half-us)
 *
 * @return duration in half us
 ****************************************************************************************
 */
__STATIC uint32_t rwip_lpcycles_2_hus(uint32_t lpcycles, uint32_t *error_corr)
{
    uint32_t res;

    // Sanity check: The number of lp cycles should not be too high to avoid overflow
    ASSERT_ERR(lpcycles < 2000000);

    #if (HZ32000)
    // Compute the sleep duration in us - case of a 32kHz clock and insert previous computed error
    *error_corr = lpcycles + *error_corr;
    // get the truncated value
    res = *error_corr >> 1;
    // retrieve new inserted error
    *error_corr = *error_corr - (res << 1);
    // finish computation
    res = 62 * lpcycles + res;
    #else //HZ32000
    // Compute the sleep duration in half us - case of a 32.768kHz clock and insert previous computed error
    *error_corr = (lpcycles << 3) + lpcycles + *error_corr;
    // get the truncated value
    res = *error_corr >> 8;
    // retrieve new inserted error
    *error_corr = *error_corr - (res << 8);
    // finish computation
    res = 61 * lpcycles + res;
    #endif //HZ32000

    return(res);
}

/**
 ****************************************************************************************
 * @brief Converts a duration in half slots into a number of low power clock cycles.
 * The function converts a duration in half slots into a number of low power clock cycles.
 * Sleep clock runs at either 32768Hz or 32000Hz, so this function divides the value in
 * slots by 10.24 or 10 depending on the case.
 * To do this the following formulae are applied:
 *
 *   N = x * 10.24 = (1024 * x)/100 for a 32.768kHz clock or
 *   N = x * 10                     for a 32kHz clock
 *
 * @param[in] hs_cnt    The value in half slot count
 *
 * @return The number of low power clock cycles corresponding to the slot count
 *
 ****************************************************************************************
 */
__STATIC int32_t rwip_slot_2_lpcycles(int32_t hs_cnt)
{
    int32_t lpcycles;

    #if HZ32000
    // Sanity check: The number of slots should not be too high to avoid overflow
    ASSERT_ERR(hs_cnt < (0xFFFFFFFF / 10));

    // Compute the low power clock cycles - case of a 32kHz clock
    lpcycles = hs_cnt * 10;
    #else //HZ32000
    // Sanity check: The number of slots should not be too high to avoid overflow
    ASSERT_ERR(hs_cnt < (0xFFFFFFFF >> 10));

    // Compute the low power clock cycles - case of a 32.768kHz clock
    lpcycles = (hs_cnt << 10)/100;
    #endif //HZ32000

    // So reduce little bit sleep duration in order to allow fine time compensation
    // Note compensation will be in range of [1 , 2[ lp cycles if there is no external wake-up
    lpcycles--;

    return(lpcycles);
}



/**
 ****************************************************************************************
 * @brief Converts a duration in us into a duration in lp cycles.
 *
 * The function converts a duration in us into a duration is lp cycles, according to the
 * low power clock frequency (32768Hz or 32000Hz).
 *
 * @param[in] us    duration in us
 *
 * @return duration in lpcycles
 ****************************************************************************************
 */
__STATIC uint32_t rwip_us_2_lpcycles(uint32_t us)
{
    uint32_t lpcycles;

    #if (HZ32000)
    // Compute the low power clock cycles - case of a 32kHz clock
    lpcycles = ((us * 32) + (999)) / 1000;
    #else //HZ32000
    // Compute the low power clock cycles - case of a 32.768kHz clock
    lpcycles = ((us * 32768) + (999999)) / 1000000;
    #endif //HZ32000

    return(lpcycles);
}

/**
 ****************************************************************************************
 * @brief Handles the Half slot timer target
 ****************************************************************************************
 */
__STATIC void rwip_timer_hs_handler(void)
{
    // disable the timer driver
    rwip_env.timer_hs_target = RWIP_INVALID_TARGET_TIME;
    ip_intcntl1_finetgtintmsk_setf(0);

    // call the default half slot call-back
    sch_alarm_timer_isr();
}

/**
 ****************************************************************************************
 * @brief Handles the Half slot timer target
 ****************************************************************************************
 */
__STATIC void rwip_timer_hus_handler(void)
{
    // disable the timer driver
    rwip_env.timer_hus_target = RWIP_INVALID_TARGET_TIME;
    ip_intcntl1_timestamptgt1intmsk_setf(0);
    
#if (BT_DUALMODE_RW == 1)
    HWradio_Setup_bb_sel_rf(0x02);  //rf RW mode
#endif 

    // call the default half slot call-back
    sch_arb_event_start_isr();
}

#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

/**
 ****************************************************************************************
 * @brief Handles the 1 ms timer target
 ****************************************************************************************
 */
__STATIC void rwip_timer_1ms_handler(void)
{
    // disable the timer driver
    rwip_env.timer_1ms_target.hs = RWIP_INVALID_TARGET_TIME;

    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    ip_intcntl1_timestamptgt2intmsk_setf(0);
    #elif (BLE_HOST_PRESENT)
    // Stop timer
    timer_set_timeout(0, NULL);
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    // Mark that 1ms timer is over
    ke_event_set(KE_EVENT_KE_TIMER);
}
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)

/**
 ****************************************************************************************
 * @brief Handles crypto event (to provide results out of interrupt context
 ****************************************************************************************
 */
__STATIC void rwip_crypt_evt_handler(void)
{
    uint8_t aes_result[KEY_LEN];

    // Clear event
    ke_event_clear(KE_EVENT_AES_END);

    // Load AES result
    em_rd(aes_result, EM_ENC_OUT_OFFSET, KEY_LEN);

    // inform AES result handler
    aes_result_handler(CO_ERROR_NO_ERROR, aes_result);
}

/**
 ****************************************************************************************
 * @brief Handles crypto interrupt
 ****************************************************************************************
 */
__STATIC void rwip_crypt_isr_handler(void)
{
    // Prevent going to deep sleep during encryption
    rwip_prevent_sleep_clear(RW_CRYPT_ONGOING);

    // Clear interrupt mask
    ip_intcntl1_cryptintmsk_setf(0);

    // mark that AES is done
    ke_event_set(KE_EVENT_AES_END);
}

/**
 ****************************************************************************************
 * @brief Handles Software requested interrupt
 ****************************************************************************************
 */
__STATIC void rwip_sw_int_handler(void)
{
    // Disable interrupt
    ip_intcntl1_swintmsk_setf(0);

    // call the SW interrupt handler
    sch_arb_sw_isr();
}

/*
 * Bug fix: As a result of watch dog can't reset RW sleeping and need wakeup by external trigger when init,
 *          this used to clear the wakeup interrupt & handle state machine. 
 */
void rwip_1st_wakeup(void)
{
    uint16_t fintetime_correction;
    // duration in half us
    uint32_t dur_hus;
    // duration in half slot
    uint32_t dur_hslot;
    // Get the number of low power sleep period
    uint32_t slp_period = ip_deepslstat_get();

    // And acknowledge any possible pending ones
    ble_intack0_clear(0xFFFFFFFF);
    // ack Sleep wakeup interrupt
    ip_intack1_slpintack_clearf(1);
    // clear CPU rw interrupt
    SYSirq_soft_clear_interrupt(VIC_IDX_RWBT0);
    
    DBG_SWDIAG(SLEEP, SLEEP, 0);
    
    // Sleep is over now
    rwip_prevent_sleep_clear(RW_DEEP_SLEEP);

    // Compensate the base time counter and fine time counter by the number of slept periods
    dur_hus = rwip_lpcycles_2_hus(slp_period, &(rwip_env.sleep_acc_error));
    // Compute the sleep duration (based on number of low power clock cycles)
    dur_hslot = dur_hus / HALF_SLOT_SIZE;

    // retrieve halfslot sleep duration
    fintetime_correction = (HALF_SLOT_SIZE-1) - (dur_hus - dur_hslot*HALF_SLOT_SIZE);

    // The correction values are then deduced from the sleep duration in us
    ip_clkncntcorr_pack(/*absdelta*/ 1, /*clkncntcorr*/ dur_hus / HALF_SLOT_SIZE);

    // The correction values are then deduced from the sleep duration in us
    ip_finecntcorr_setf(fintetime_correction);

    // Start the correction
    ip_deepslcntl_deep_sleep_corr_en_setf(1);
}
/**
 ****************************************************************************************
 * @brief Wake-up from Core sleep.
 *
 * Compute and apply the clock correction according to duration of the deep sleep.
 ****************************************************************************************
 */
__STATIC void rwip_wakeup(void)
{
    uint16_t fintetime_correction;
    // duration in half us
    uint32_t dur_hus;
    // duration in half slot
    uint32_t dur_hslot;
    // Get the number of low power sleep period
    uint32_t slp_period = ip_deepslstat_get();

    DBG_SWDIAG(SLEEP, SLEEP, 0);

    //led_set(6);
    //led_reset(2);

    // Sleep is over now
    rwip_prevent_sleep_clear(RW_DEEP_SLEEP);

    // Prevent going to deep sleep until a slot interrupt is received
    rwip_prevent_sleep_set(RW_WAKE_UP_ONGOING);

    // Compensate the base time counter and fine time counter by the number of slept periods
    dur_hus = rwip_lpcycles_2_hus(slp_period, &(rwip_env.sleep_acc_error));
    // Compute the sleep duration (based on number of low power clock cycles)
    dur_hslot = dur_hus / HALF_SLOT_SIZE;

    // retrieve halfslot sleep duration
    fintetime_correction = (HALF_SLOT_SIZE-1) - (dur_hus - dur_hslot*HALF_SLOT_SIZE);

    // The correction values are then deduced from the sleep duration in us
    ip_clkncntcorr_pack(/*absdelta*/ 1, /*clkncntcorr*/ dur_hus / HALF_SLOT_SIZE);

    // The correction values are then deduced from the sleep duration in us
    ip_finecntcorr_setf(fintetime_correction);

    // Start the correction
    ip_deepslcntl_deep_sleep_corr_en_setf(1);

    // Enable the RWBT slot interrupt
    ip_intcntl1_clknintsrmsk_setf(0);
    ip_intcntl1_clknintmsk_setf(1);
    ip_intack1_clear(IP_CLKNINTACK_BIT);

    #if (H4TL_SUPPORT)
    // Restart the flow on the TL
    h4tl_start();
    #endif //H4TL_SUPPORT

    TRC_REQ_WAKEUP();
}



/**
 ****************************************************************************************
 * @brief Restore the core processing after the clock correction
 *
 * Enable the core and check if some timer target has been reached.
 ****************************************************************************************
 */
__STATIC void rwip_wakeup_end(void)
{
    DBG_SWDIAG(SLEEP, WAKEUP_END, 1);

    // get current time
    rwip_time_t current_time = rwip_time_get();

    // Disable clock interrupt
    ip_intcntl1_clknintmsk_setf(0);

    if(rwip_env.timer_hs_target != RWIP_INVALID_TARGET_TIME)
    {
        // check if half slot timer target is reach
        //if((current_time.hs == rwip_env.timer_hs_target))
        if(CLK_DIFF(current_time.hs, rwip_env.timer_hus_target) <= 0)
        {
            rwip_timer_hs_handler();
        }
    }

    if(rwip_env.timer_hus_target != RWIP_INVALID_TARGET_TIME)
    {
        // check if half us timer target is reach
        //if((current_time.hs == rwip_env.timer_hus_target))
        if(CLK_DIFF(current_time.hs, rwip_env.timer_hus_target) <= 0)
        {
            rwip_timer_hus_handler();
        }
    }

    if(rwip_env.timer_1ms_target.hs != RWIP_INVALID_TARGET_TIME)
    {
        // check if 1ms target is reached
        //if((current_time.hs == rwip_env.timer_1ms_target.hs))
        if(CLK_DIFF(current_time.hs, rwip_env.timer_hus_target) <= 0)
        {
            rwip_timer_1ms_handler();
        }
    }

    // Wake up is complete now, so we allow the deep sleep again
    rwip_prevent_sleep_clear(RW_WAKE_UP_ONGOING);
    rwip_prevent_sleep_clear(RW_EXT_WAKE_UP_START);
    //led_reset(6);

    DBG_SWDIAG(SLEEP, WAKEUP_END, 0);
}

/*
 * GLOBAL FUNCTIONS
 ****************************************************************************************
 */

#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)
rwip_time_t rwip_time_get(void)
{
    rwip_time_t res;

    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    //Sample the base time count
    ip_slotclk_samp_setf(1);
    while (ip_slotclk_samp_getf());
    // get base time and offset - must be read atomically
    GLOBAL_INT_DISABLE();
    res.hs = ip_slotclk_sclk_getf();
    res.hus = HALF_SLOT_INV(ip_finetimecnt_get());
    GLOBAL_INT_RESTORE();
    #elif (BLE_HOST_PRESENT)
    // get base time (10 ms unit)
    res.hs  = timer_get_time() << 5;
    res.hus = 0;
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    return res;
}

#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
#if (BT_EMB_PRESENT)
void rwip_time_set(uint32_t clock)
{
    ip_slotclk_pack(IP_SAMP_RST, 1 /* clk_upd */, clock & 0x0FFFFFFF);
    while(ip_slotclk_clkn_upd_getf());
}
#endif // (BT_EMB_PRESENT)
#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

uint8_t rwip_sleep(void)
{
    uint8_t sleep_res = RWIP_ACTIVE;

    DBG_SWDIAG(SLEEP, FUNC, 1);

    DBG_SWDIAG(SLEEP, ALGO, 0);

    do
    {
        #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
        int32_t sleep_duration;
        rwip_time_t current_time,current_time_temp;
        #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

        /************************************************************************
         **************            CHECK KERNEL EVENTS             **************
         ************************************************************************/
        // Check if some kernel processing is ongoing (during wakeup, kernel events are not processed)
        if (((rwip_env.prevent_sleep & RW_WAKE_UP_ONGOING) == 0) && !ke_sleep_check())
            break;

        // Processor sleep can be enabled
        sleep_res = RWIP_CPU_SLEEP;

        DBG_SWDIAG(SLEEP, ALGO, 1);

        /************************************************************************
         **************              CHECK RW FLAGS                **************
         ************************************************************************/
        // First check if no pending procedure prevent from going to sleep
        if (rwip_env.prevent_sleep != 0)
            break;

        #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
        DBG_SWDIAG(SLEEP, ALGO, 2);

        /************************************************************************
         **************           Retrieve Current time            **************
         ************************************************************************/
        current_time = rwip_time_get();
        current_time_temp=current_time;
        // Consider 2 half-slots for clock correction (worst case: 1 half-slot before correction, 1 half-slot after correction)
        current_time.hs += 2;
        // Remove 1 more slot because next slot will be started at end of function
        if((HALF_SLOT_INV(current_time.hus)) < rwip_env.sleep_algo_dur)
        {
            current_time.hs += 1;
        }
        // Be sure that we don't exceed the clock wrapping time
        current_time.hs &= RWIP_MAX_CLOCK_TIME;

        /************************************************************************
         ******* COMPUTE SLEEP TIME ACCORDING TO 1 MS AND HALF SLOT TIMER ******
         ************************************************************************/

        // put sleep duration to maximum value
        sleep_duration = (rwip_env.ext_wakeup_enable) ? MAX_SLEEP_DURATION_EXTERNAL_WAKEUP : MAX_SLEEP_DURATION_PERIODIC_WAKEUP;

        // check if 1ms timer is active
        if(rwip_env.timer_1ms_target.hs != RWIP_INVALID_TARGET_TIME)
        {
            int32_t duration = CLK_DIFF(current_time.hs, rwip_env.timer_1ms_target.hs);
            // update sleep duration to minimum value
            sleep_duration = co_min_s(sleep_duration, duration);
            
            if (CLK_LOWER_EQ(rwip_env.timer_1ms_target.hs+1,current_time_temp.hs ))
            {
                extern void ke_timer_past_done();
                ke_timer_past_done();
                break;
            }
        }

        // check if Half slot timer is active
        if(rwip_env.timer_hs_target != RWIP_INVALID_TARGET_TIME)
        {
            int32_t duration = CLK_DIFF(current_time.hs, rwip_env.timer_hs_target);
            // update sleep duration to minimum value
            sleep_duration = co_min_s(sleep_duration, duration);
        }

        // check if Half us timer is active
        if(rwip_env.timer_hus_target != RWIP_INVALID_TARGET_TIME)
        {
            int32_t duration = CLK_DIFF(current_time.hs, rwip_env.timer_hus_target);
            // update sleep duration to minimum value
            sleep_duration = co_min_s(sleep_duration, duration);
        }

        // A timer ISR is not yet handled or will be raised soon
        // note the sleep duration could be negative, that's why it's useful to check if a minimum requirement is ok
        // at least one half slot.
        if(sleep_duration <= RWIP_MINIMUM_SLEEP_TIME)
        {
            break;
        }
        sleep_duration-=4;
        DBG_SWDIAG(SLEEP, ALGO, 3);

        /************************************************************************
         **************           CHECK SLEEP TIME                 **************
         ************************************************************************/
        sleep_duration = rwip_slot_2_lpcycles(sleep_duration);

        // check if sleep duration is sufficient according to wake-up delay
        // HW issue, if sleep duration = max(twosc,twext) + 1 the HW never wakes up, so we have to ensure that at least
        // sleep duration > max(twosc,twext) + 1 (all in lp clk cycle)
        if(sleep_duration < rwip_env.lp_cycle_wakeup_delay + 1)
        {
            break;
        }

        DBG_SWDIAG(SLEEP, ALGO, 4);

        #if (H4TL_SUPPORT)
        /************************************************************************
         **************                 CHECK TL                   **************
         ************************************************************************/
        // Try to switch off TL
        if (!h4tl_stop())
        {
            sleep_res = RWIP_ACTIVE;
            break;
        }
        #endif //H4TL_SUPPORT

        DBG_SWDIAG(SLEEP, FUNC, 0);
        sleep_res = RWIP_DEEP_SLEEP;

        TRC_REQ_SLEEP();

        /************************************************************************
         **************          PROGRAM CORE DEEP SLEEP           **************
         ************************************************************************/
        // Program wake-up time
        ip_deepslwkup_set(sleep_duration);

        //led_set(2);

        DBG_SWDIAG(SLEEP, SLEEP, 1);

        // Prevent re-entering sleep, until it effectively sleeps and wakes up
        rwip_prevent_sleep_set(RW_DEEP_SLEEP);

        /************************************************************************
         **************               SWITCH OFF RF                **************
         ************************************************************************/
        rwip_rf.sleep();
        #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    } while(0);

    if(sleep_res != RWIP_DEEP_SLEEP)
    {
        DBG_SWDIAG(SLEEP, FUNC, 0);
    }
    return sleep_res;
}

void rwip_driver_init(uint8_t init_type)
{
    #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)

    switch (init_type)
    {
        case RWIP_INIT:
        {
            // Register AES event
            ke_event_callback_set(KE_EVENT_AES_END, &rwip_crypt_evt_handler);

            // ensure that we will never enter in deep sleep
            rwip_prevent_sleep_set(RW_PLF_DEEP_SLEEP_DISABLED);
        }
        break;

        case RWIP_RST:
        {
            // Do nothing
        }
        // No break

        case RWIP_1ST_RST:
        {
            uint8_t length;
            uint8_t sleep_enable = 0;
            uint8_t ext_wakeup_enable;
            #if (BT_DUAL_MODE)
            uint8_t diag_cfg[PARAM_LEN_DIAG_DM_HW];
            #endif // (BT_DUAL_MODE)

            // initialize environment
            rwip_env.prevent_sleep     = 0;
            // clear target timer
            rwip_env.timer_1ms_target.hs = RWIP_INVALID_TARGET_TIME;
            rwip_env.timer_1ms_target.hus = 0;
            rwip_env.timer_hs_target   = RWIP_INVALID_TARGET_TIME;
            rwip_env.timer_hus_target  = RWIP_INVALID_TARGET_TIME;

            // Reset the IP core
            ip_rwdmcntl_master_soft_rst_setf(1);
            while(ip_rwdmcntl_master_soft_rst_getf());

            // Enable default common interrupts
            ip_intcntl1_set(IP_FIFOINTMSK_BIT | IP_CRYPTINTMSK_BIT | IP_SWINTMSK_BIT | IP_SLPINTMSK_BIT);

            #if (BT_DUAL_MODE)
            // Read diagport configuration from NVDS
            length = PARAM_LEN_DIAG_DM_HW;
            if(rwip_param.get(PARAM_ID_DIAG_DM_HW, &length, diag_cfg) == PARAM_OK)
            {
                ip_diagcntl_pack(1, diag_cfg[3], 1, diag_cfg[2], 1, diag_cfg[1], 1, diag_cfg[0]);
            }
            else
            {
                ip_diagcntl_set(0);
            }
            #endif // (BT_DUAL_MODE)

            // Activate deep sleep feature if enabled in NVDS and in reset mode
            length = PARAM_LEN_SLEEP_ENABLE;
            if(rwip_param.get(PARAM_ID_SLEEP_ENABLE, &length, &sleep_enable) != PARAM_OK)
            {
                sleep_enable = 0;
            }
            
            sleep_enable = 1;  //enable sleep always, yangyang, 2020/12/7

            // check is sleep is enabled
            if(sleep_enable != 0)
            {
                uint32_t twext, twosc, twrm;

                // Set max sleep duration depending on wake-up mode
				length = PARAM_LEN_EXT_WAKEUP_ENABLE;
                if(rwip_param.get(PARAM_ID_EXT_WAKEUP_ENABLE, &length, &ext_wakeup_enable) != PARAM_OK)
                {
                    ext_wakeup_enable = 0;
                }
                rwip_env.ext_wakeup_enable = (ext_wakeup_enable != 0) ? true : false;

                // Set max sleep duration depending on wake-up mode
                length = sizeof(rwip_env.sleep_algo_dur);
                if(rwip_param.get(PARAM_ID_SLEEP_ALGO_DUR, &length, (uint8_t*) &rwip_env.sleep_algo_dur) != PARAM_OK)
                {
                    // set a default duration: 200 us ==> 400 half us
                    rwip_env.sleep_algo_dur = 400;
                }

                // Initialize sleep parameters
                rwip_env.sleep_acc_error   = 0;

                // Get TWrm from NVDS
                length = sizeof(uint16_t);
                if (rwip_param.get(PARAM_ID_RM_WAKEUP_TIME, &length, (uint8_t*)&twrm) != PARAM_OK)
                {
                    // Set default values : 625 us
                    twrm = SLEEP_RM_WAKEUP_DELAY;
                }

                // Get TWosc from NVDS
                length = sizeof(uint16_t);
                if (rwip_param.get(PARAM_ID_OSC_WAKEUP_TIME, &length, (uint8_t*)&twosc) != PARAM_OK)
                {
                    // Set default values : 5 ms
                    twosc = SLEEP_OSC_NORMAL_WAKEUP_DELAY;
                }

                // Get TWext from NVDS
                length = sizeof(uint16_t);
                if (rwip_param.get(PARAM_ID_EXT_WAKEUP_TIME, &length, (uint8_t*)&twext) != PARAM_OK)
                {
                    // Set default values : 5 ms
                    twext = SLEEP_OSC_EXT_WAKEUP_DELAY;
                }

                twrm  = rwip_us_2_lpcycles(twrm);
                twosc = rwip_us_2_lpcycles(twosc);
                twext = rwip_us_2_lpcycles(twext);

                // Program register
                ip_enbpreset_pack(twext, twosc, twext);

                // Configure wake up delay to the highest parameter
                twext = co_max(twext,twrm);
                twext = co_max(twext,twosc);

                // Store wake-up delay in lp cycles
                rwip_env.lp_cycle_wakeup_delay = twext;

                // Set the external wakeup parameter
                ip_deepslcntl_extwkupdsb_setf(!rwip_env.ext_wakeup_enable);
            }
            else
            {
                // ensure that we will never enter in deep sleep
                rwip_prevent_sleep_set(RW_PLF_DEEP_SLEEP_DISABLED);
            }

            // Set prefetch time and anticipated prefetch abort time
            ip_timgencntl_pack((IP_PREFETCHABORT_TIME_US << 1), (IP_PREFETCH_TIME_US << 1));

            // Get programming delay from parameters
            length = PARAM_LEN_PROG_DELAY;
            if(rwip_param.get(PARAM_ID_PROG_DELAY, &length, &rwip_prog_delay) != PARAM_OK)
            {
                rwip_prog_delay = IP_PROG_DELAY_DFT;
            }
        }
        break;

        default:
        {
            // Do nothing
        }
        break;
    }

    #elif (BLE_HOST_PRESENT)
    // initialize environment
    rwip_env.prevent_sleep     = 0;
    rwip_env.timer_10ms_target = RWIP_INVALID_TARGET_TIME;
    // enable timer
    timer_enable(true);
    #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)
}

uint16_t rwip_prevent_sleep_get(uint16_t prv_slp_bit)
{
    return rwip_env.prevent_sleep & prv_slp_bit; 
}

void rwip_prevent_sleep_set(uint16_t prv_slp_bit)
{
    GLOBAL_INT_DISABLE();
    rwip_env.prevent_sleep |= prv_slp_bit;
    GLOBAL_INT_RESTORE();
}

void rwip_prevent_sleep_clear(uint16_t prv_slp_bit)
{
    GLOBAL_INT_DISABLE();
    rwip_env.prevent_sleep &= ~prv_slp_bit;
    GLOBAL_INT_RESTORE();
}

void rw_ext_wakeup(void)
{
    if (rwip_prevent_sleep_get(RW_DEEP_SLEEP))
    {
		os_printf("ext_wakeup\n");
		#if (BT_DUALMODE_RW == 1)
        rw_ext_wakeup_generate();
		#endif
        while(rwip_prevent_sleep_get(RW_DEEP_SLEEP))
        {
            delay_us(200);
        }
    }
}
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
bool rwip_active_check(void)
{
    bool result = true;

    do
    {
        #if BT_EMB_PRESENT
        if(rwip_env.prevent_sleep & (RW_CSB_NOT_LPO_ALLOWED | RW_BT_ACTIVE_MODE))
            break;
        #endif // BT_EMB_PRESENT

        #if BLE_EMB_PRESENT
        if(rwip_env.prevent_sleep & (RW_BLE_ACTIVE_MODE))
            break;
        #endif // BLE_EMB_PRESENT

        result = false;

    } while (0);

    return result;
}
#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

void rwip_timer_1ms_set(rwip_time_t target)
{
    GLOBAL_INT_DISABLE();

    if (target.hs != RWIP_INVALID_TARGET_TIME)
    {
        // save target time
        rwip_env.timer_1ms_target = target;

        #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
         // set the abs timeout in HW
        ASSERT_ERR(target.hs <= RWIP_MAX_CLOCK_TIME);
        ip_clkntgt2_set(target.hs);
        ASSERT_ERR(target.hus <= HALF_SLOT_TIME_MAX);
        ip_hmicrosectgt2_set(HALF_SLOT_TIME_MAX - target.hus);

        // if timer is not enabled, it is possible that the irq is raised
        // due to a spurious value, so ack it before
        ip_intack1_timestamptgt2intack_clearf(1);
        ip_intcntl1_timestamptgt2intmsk_setf(1);
        #elif (BLE_HOST_PRESENT)
        // Start timer
        uint32_t target value_ms = (target.hs*HALF_SLOT_SIZE + target.hus)/2000;
        timer_set_timeout(target value_ms, rwip_timer_1ms_handler);
        #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    }
    else
    {
        // save target time - not set
        rwip_env.timer_1ms_target.hs = RWIP_INVALID_TARGET_TIME;

        #if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
        // disable timer irq
        ip_intcntl1_timestamptgt2intmsk_setf(0);
        #elif (BLE_HOST_PRESENT)
        // Stop timer
        timer_set_timeout(0, NULL);
        #endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)
    }
    GLOBAL_INT_RESTORE();
}

#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
void rwip_timer_hs_set(uint32_t target)
{
    // save target time
    rwip_env.timer_hs_target = target;

    if (target != RWIP_INVALID_TARGET_TIME)
    {
        // set the abs timeout in HW
        ip_finetimtgt_finetarget_setf(target);

        // if timer is not enabled, it is possible that the irq is raised
        // due to a spurious value, so ack it before
        ip_intack1_finetgtintack_clearf(1);
        ip_intcntl1_finetgtintmsk_setf(1);
    }
    else
    {
        // disable timer irq
        ip_intcntl1_finetgtintmsk_setf(0);
    }
}

void rwip_timer_hus_set(uint32_t target, uint32_t half_us_delay)
{
    // save target time
    rwip_env.timer_hus_target = target;

    if (target != RWIP_INVALID_TARGET_TIME)
    {
        ASSERT_INFO(half_us_delay < HALF_SLOT_SIZE, half_us_delay, 0);

        // set the abs timeout in HW
        ip_clkntgt1_setf(target);
        ip_hmicrosectgt1_setf(HALF_SLOT_TIME_MAX - half_us_delay);

        // if timer is not enabled, it is possible that the irq is raised
        // due to a spurious value, so ack it before
        ip_intack1_timestamptgt1intack_clearf(1);
        ip_intcntl1_timestamptgt1intmsk_setf(1);
    }
    else
    {
        // disable timer irq
        ip_intcntl1_timestamptgt1intmsk_setf(0);
    }
}

void rwip_aes_encrypt(const uint8_t *key, const uint8_t* val)
{
    // Prevent going to deep sleep during encryption
    rwip_prevent_sleep_set(RW_CRYPT_ONGOING);

    // Copy data to EM buffer
    em_wr(val, EM_ENC_IN_OFFSET, KEY_LEN);

    // copy the key in the register dedicated for the encryption
    ip_aeskey31_0_set(  co_read32p(&(key[0])));
    ip_aeskey63_32_set( co_read32p(&(key[4])));
    ip_aeskey95_64_set( co_read32p(&(key[8])));
    ip_aeskey127_96_set(co_read32p(&(key[12])));

    // Set the pointer on the data to encrypt.
    ip_aesptr_setf(EM_ENC_IN_OFFSET >> 2);

    // enable crypt interrupt (and clear a previous interrupt if needed)
    ip_intack1_cryptintack_clearf(1);
    ip_intcntl1_cryptintmsk_setf(1);

    // start the encryption
    ip_aescntl_aes_start_setf(1);
}

void rwip_sw_int_req(void)
{
    // enable SW interrupt (and clear a previous interrupt if needed)
    ip_intack1_swintack_clearf(1);
    ip_intcntl1_swintmsk_setf(1);
    // start the SW interrupt
    ip_rwdmcntl_swint_req_setf(1);
}

void rwip_isr(void)
{

#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#endif
    
    DBG_SWDIAG(ISR, RWIP, 1);

    // Check interrupt status and call the appropriate handlers
    uint32_t irq_stat      = ip_intstat1_get();

    // General purpose timer interrupt - half slot accuracy
    if (irq_stat & IP_FINETGTINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, FINETGTINT, 1);
        // Clear the interrupt
        ip_intack1_finetgtintack_clearf(1);

        // handles half slot timer target
        rwip_timer_hs_handler();

        DBG_SWDIAG(IP_ISR, FINETGTINT, 0);
    }

    // General purpose timer interrupt - half us accuracy
    if (irq_stat & IP_TIMESTAMPTGT1INTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, TIMESTAMPINT, 1);
        // Clear the interrupt
        ip_intack1_timestamptgt1intack_clearf(1);
        
        //fixbug:abort handle hus irq when rwip in deepsleep or wakeup-ongoing
        if(!(rwip_env.prevent_sleep & (RW_WAKE_UP_ONGOING|RW_DEEP_SLEEP)))
        {
            // handles half slot timer target
            rwip_timer_hus_handler();
        }

        DBG_SWDIAG(IP_ISR, TIMESTAMPINT, 0);
    }

    // Clock
    if (irq_stat & IP_CLKNINTSTAT_BIT) // clock interrupt
    {
        DBG_SWDIAG(IP_ISR, CLKNINT, 1);

        // Ack clock interrupt
        ip_intack1_clknintack_clearf(1);

        if(rwip_env.prevent_sleep & RW_WAKE_UP_ONGOING)
        {
            // Handle end of wake-up
            rwip_wakeup_end();
        }
        #if (BT_EMB_PRESENT)
        else // BT uses clock IRQ to program ACL frames
        {
            // Call Scheduling Programmer
            sch_prog_clk_isr();
        }
        #endif //BT_EMB_PRESENT
        
#if (BT_DUALMODE_RW_SLEEP == 1)
        extern u_int8 syspwr_cpu_halt;
        syspwr_cpu_halt = 0;
#endif

        DBG_SWDIAG(IP_ISR, CLKNINT, 0);
    }

    // FIFO
    if (irq_stat & IP_FIFOINTSTAT_BIT) // FIFO interrupt
    {
        DBG_SWDIAG(IP_ISR, FIFOINT, 1);

        // Call scheduling programmer
        sch_prog_fifo_isr();

        // Ack FIFO interrupt
        ip_intack1_fifointack_clearf(1);

        DBG_SWDIAG(IP_ISR, FIFOINT, 0);
    }

    if (irq_stat & IP_SLPINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, SLPINT, 1);

        // ack Sleep wakeup interrupt
        ip_intack1_slpintack_clearf(1);

        // Handle wake-up
        rwip_wakeup();

        DBG_SWDIAG(IP_ISR, SLPINT, 0);
    }

    // General purpose timer interrupt
    if (irq_stat & IP_TIMESTAMPTGT2INTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, GROSSTGTINT, 1);

        // Clear the interrupt
        ip_intack1_timestamptgt2intack_clearf(1);

        // handles 1 ms timer target
        rwip_timer_1ms_handler();

        DBG_SWDIAG(IP_ISR, GROSSTGTINT, 0);
    }

    // Encryption interrupt
    if (irq_stat & IP_CRYPTINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, CRYPTINT, 1);

        ip_intack1_cryptintack_clearf(1);

        // call the crypto ISR handler
        rwip_crypt_isr_handler();

        DBG_SWDIAG(IP_ISR, CRYPTINT, 0);
    }

    // SW interrupt
    if (irq_stat & IP_SWINTSTAT_BIT)
    {
        DBG_SWDIAG(IP_ISR, SWINT, 1);
        // Clear the interrupt
        ip_intack1_swintack_clearf(1);

        // call SW interrupt handler
        rwip_sw_int_handler();

        DBG_SWDIAG(IP_ISR, SWINT, 0);
    }

    DBG_SWDIAG(ISR, RWIP, 0);
}

#endif // (BLE_EMB_PRESENT || BT_EMB_PRESENT)

///@} RW
