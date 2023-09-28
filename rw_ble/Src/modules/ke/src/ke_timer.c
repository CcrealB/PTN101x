/**
 ****************************************************************************************
 *
 * @file ke_timer.c
 *
 * @brief This file contains the scheduler primitives called to create or delete
 * a task. It contains also the scheduler itself.
 *
 * Copyright (C) RivieraWaves 2009-2019
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup KE_TIMER
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stddef.h>              // standard definition
#include <stdint.h>              // standard integer
#include <stdbool.h>             // standard boolean
#include "arch.h"                // architecture

#include "ke_queue.h"            // kernel queue
#include "ke_mem.h"              // kernel memory
#include "ke_env.h"              // kernel environment
#include "ke_event.h"            // kernel event
#include "ke_timer.h"            // kernel timer
#include "ke_task.h"             // kernel task

#include "co_utils.h"
#include "co_bt_defines.h"

#include "dbg_swdiag.h"          // Software diag

#include "rwip.h"                // common driver

#include "dbg.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Invalid timer value
#define KE_TIMER_INVALID ((rwip_time_t) {RWIP_INVALID_TARGET_TIME, HALF_SLOT_SIZE})

/// Maximum timer value (half of the BT clock range in ms)
#define KE_TIMER_DELAY_MAX      (41943039)

/// Timer programming margin (in 312.5 us half-slots)
#define KE_TIMER_PROG_MARGIN      (3)

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Retrieve kernel time.
 *
 * @return time value (in Kernel time (rwip_time_t))
 ****************************************************************************************
 */
static rwip_time_t ke_time(void)
{
    rwip_time_t res;
    GLOBAL_INT_DISABLE();
    res = rwip_time_get();
    GLOBAL_INT_RESTORE();
    return (res);
}

/**
 ****************************************************************************************
 * @brief Check if ke time has passed.
 *
 * @param[in] time     time value to check
 *
 * @return if time has passed or not
 ****************************************************************************************
 */
static bool ke_time_past(rwip_time_t time)
{
    rwip_time_t current_time = ke_time();

    return (CLK_LOWER_EQ_HUS(time.hs, time.hus, current_time.hs, current_time.hus));
}

/**
 ****************************************************************************************
 * @brief Compare timer absolute expiration time.
 *
 * @param[in] timerA Timer to compare.
 * @param[in] timerB Timer to compare.
 *
 * @return true if timerA will expire before timerB.
 ****************************************************************************************
 */
static bool cmp_abs_time(struct co_list_hdr const * timerA, struct co_list_hdr const * timerB)
{
    rwip_time_t timeA = ((struct ke_timer*)timerA)->time;
    rwip_time_t timeB = ((struct ke_timer*)timerB)->time;

    return (CLK_LOWER_EQ_HUS(timeA.hs, timeA.hus, timeB.hs, timeB.hus));
}

/**
 ****************************************************************************************
 * @brief Compare timer and task IDs callback
 *
 * @param[in] timer           Timer value
 * @param[in] timer_task      Timer task
 *
 * @return timer insert
 ****************************************************************************************
 */
static bool cmp_timer_id(struct co_list_hdr const * timer, uint32_t timer_task)
{
    // trick to pack 2 u16 in u32
    ke_msg_id_t timer_id = timer_task >> 16;
    ke_task_id_t task_id = timer_task & 0xFFFF;

    // insert the timer just before the first one older
    return (timer_id == ((struct ke_timer*)timer)->id)
        && (task_id == ((struct ke_timer*)timer)->task);
}

/**
 ****************************************************************************************
 * @brief Schedule the next timer(s).
 *
 * This function pops the first timer from the timer queue and notifies the appropriate
 * task by sending a kernel message. If the timer is periodic, it is set again;
 * if it is one-shot, the timer is freed. The function checks also the next timers
 * and process them if they have expired or are about to expire.
 ****************************************************************************************
 */
static void ke_timer_schedule(void)
{
    DBG_SWDIAG(EVT, TIMER, 1);
    for(;;)
    {
        struct ke_timer *timer;

        ke_event_clear(KE_EVENT_KE_TIMER);

        // check the next timer
        timer = (struct ke_timer*) ke_env.queue_timer.first;

        if(!timer)
        {
            break;
        }

        // Consider a margin of at least 3 half-slots
        if (CLK_LOWER_EQ(ke_time().hs, timer->time.hs - KE_TIMER_PROG_MARGIN))
        //if (!ke_time_past(timer->time))
        {
            // timer will expire in the future, configure the HW
            rwip_timer_1ms_set(timer->time);

            // in most case, we will break here. However, if the timer expiration
            // time has just passed, may be the HW was set too late (due to an IRQ)
            // so we do not exit the loop to process it.
            if (!ke_time_past(timer->time))
            {
                break;
            }
            else
            {
                // remove current timer
                rwip_timer_1ms_set(KE_TIMER_INVALID);
            }
        }

        // at this point, the next timer in the queue has expired or is about to -> pop it
        timer = (struct ke_timer*) ke_queue_pop(&ke_env.queue_timer);

        //Trace the kernel timer
        TRC_REQ_KE_TMR_EXP(timer->time.hs, timer->task, timer->id);

        // notify the task
        ke_msg_send_basic(timer->id, timer->task, TASK_NONE);

        // free the memory allocated for the timer
        ke_free(timer);
    }
    DBG_SWDIAG(EVT, TIMER, 0);
}


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */


void ke_timer_init(void)
{
    // Register timer event
    ke_event_callback_set(KE_EVENT_KE_TIMER, &ke_timer_schedule);
}

void ke_timer_set(ke_msg_id_t const timer_id, ke_task_id_t const task_id, uint32_t delay)
{
    struct ke_timer *timer;
    // Indicate if the HW will have to be reprogrammed
    bool hw_prog = false;
    // Timer time
    rwip_time_t abs_time, delay_time, current_time;

    // Check if requested timer is first of the list of pending timer
    struct ke_timer *first = (struct ke_timer *) ke_env.queue_timer.first;

    // Delay shall not be more than maximum allowed
    if(delay > KE_TIMER_DELAY_MAX)
    {
        delay = KE_TIMER_DELAY_MAX;
    }
    // Delay should not be zero
    else if(delay == 0)
    {
        delay = 1;
    }

    if(first != NULL)
    {
        if ((first->id == timer_id) && (first->task == task_id))
        {
            // Indicate that the HW tid_ke_timemer will have to be reprogrammed
            hw_prog = true;
        }
    }

    // Extract the timer from the list if required
    timer = (struct ke_timer*) ke_queue_extract(&ke_env.queue_timer, cmp_timer_id, (uint32_t)timer_id << 16 | task_id);

    if (timer == NULL)
    {
        // Create new one
        timer = (struct ke_timer*) ke_malloc(sizeof(struct ke_timer), KE_MEM_KE_MSG);
        ASSERT_ERR(timer);
        timer->id = timer_id;
        timer->task = task_id;
    }

    // update characteristics
    delay_time.hs = delay*2000/HALF_SLOT_SIZE;
    delay_time.hus = delay*2000 - delay_time.hs*HALF_SLOT_SIZE;
    current_time = ke_time();
    abs_time.hs = CLK_ADD_2(current_time.hs, delay_time.hs);
    abs_time.hus = current_time.hus + delay_time.hus;
    if (abs_time.hus > HALF_SLOT_TIME_MAX)
    {
        abs_time.hs = CLK_ADD_2(abs_time.hs, 1);
        abs_time.hus -= HALF_SLOT_TIME_MAX;
    }
    timer->time = abs_time;

    // Mask calculated time to be sure it's not greater than time counter
    //timer->time &= KE_TIMER_MASK;

    // insert in sorted timer list
    ke_queue_insert(&ke_env.queue_timer,
                    (struct co_list_hdr*) timer,
                    cmp_abs_time);

    //Trace the kernel timer
    TRC_REQ_KE_TMR_SET(timer->time.hs, task_id, timer_id);

    // retrieve first timer element
    first = (struct ke_timer *) ke_env.queue_timer.first;

    // check if HW timer set needed
    if (hw_prog || (first == timer))
    {
        rwip_timer_1ms_set(first->time);
    }

    // Check that the timer did not expire before HW prog
    if (ke_time_past(abs_time))
    {
        // Timer already expired, so schedule the timer event immediately
        ke_event_set(KE_EVENT_KE_TIMER);
    }
}

void ke_timer_clear(ke_msg_id_t const timer_id, ke_task_id_t const task_id)
{
    struct ke_timer *timer = (struct ke_timer *) ke_env.queue_timer.first;

    if (ke_env.queue_timer.first != NULL)
    {
        if ((timer->id == timer_id) && (timer->task == task_id))
        {
            // timer found and first to expire! pop it
            ke_queue_pop(&ke_env.queue_timer);

            struct ke_timer *first = (struct ke_timer *) ke_env.queue_timer.first;

            // and set the following timer HW if any
            rwip_timer_1ms_set((first) ? first->time : KE_TIMER_INVALID);

            // Check that the timer did not expire before HW prog
            if ((first) && ke_time_past(first->time))
            {
                // Timer already expired, so schedule the timer event immediately
                ke_event_set(KE_EVENT_KE_TIMER);
            }

            // Check that the timer did not expire before HW prog
            //ASSERT_ERR(!ke_time_past(first->time));
        }
        else
        {
            timer = (struct ke_timer *)
                    ke_queue_extract(&ke_env.queue_timer, cmp_timer_id,
                            (uint32_t)timer_id << 16 | task_id);
        }

        if (timer != NULL)
        {
            //Trace the cleared timer
            TRC_REQ_KE_TMR_CLR(timer->time.hs, timer->task, timer->id);

            // free the cleared timer
            ke_free(timer);
        }
    }
}


void ke_timer_past_done()
{
    struct ke_timer *first = (struct ke_timer *) ke_env.queue_timer.first;
    // Check that the timer did not expire before HW prog
    if ((first) 
         //&& ke_time_past(first->time)
       )
    {
        // Timer already expired, so schedule the timer event immediately
        ke_event_set(KE_EVENT_KE_TIMER);
    }
}

bool ke_timer_active(ke_msg_id_t const timer_id, ke_task_id_t const task_id)
{
    struct ke_timer *timer;
    bool result;

    // check the next timer
    timer = (struct ke_timer*) ke_env.queue_timer.first;

    /* scan the timer queue to look for a message element with the same id and destination*/
    while (timer != NULL)
    {
        if ((timer->id == timer_id) &&
            (timer->task == task_id) )
        {
            /* Element has been found                                                   */
            break;
        }

        /* Check  next timer                                                            */
        timer = timer->next;
    }

    /* If the element has been found                                                    */
    if (timer != NULL)
        result = true;
    else
        result = false;

    return(result);

}

void ke_timer_adjust_all(uint32_t delay)
{
    struct ke_timer *timer;

    // check the next timer
    timer = (struct ke_timer*) ke_env.queue_timer.first;

    // iterate through timers, adjust
    while (timer != NULL)
    {
        rwip_time_t delay_time;
        delay_time.hs = delay*2000/HALF_SLOT_SIZE;
        delay_time.hus = delay*2000 - delay_time.hs*HALF_SLOT_SIZE;

        timer->time.hs = CLK_ADD_2(timer->time.hs, delay_time.hs);
        timer->time.hus += delay_time.hus;
        if (timer->time.hus > HALF_SLOT_TIME_MAX)
        {
            timer->time.hs = CLK_ADD_2(timer->time.hs, 1);
            timer->time.hus -= HALF_SLOT_TIME_MAX;
        }

        timer = timer->next;
    }
}


///@} KE_TIMER
