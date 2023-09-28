/**
 * **************************************************************************************
 * @file    driver_pwm.h
 * 
 * @author  Borg Xiao
 * @date    20230415
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * example use timer channel0 of timer0 [call timer0_isr() with 1ms intrrupt]:
 *  1. timer_peri_init(TIMER0, TIMER_CH0, 1);               // group timer0 public init
 *  2. timer_config(TIMER0, TIMER_CH0, 1000, timer0_isr);   // set timer count end value(1000us)
 *  3. timer_enable(TIMER0, TIMER_CH0, 1);                  // enable timer
 * */
#ifndef _DRV_TIMER_H_
#define _DRV_TIMER_H_

#include <stdint.h>

typedef enum _TIMERx_e{
    TIMER0,
    TIMER1,
    TIMER_NUM
}TIMERx_e;

typedef enum _TIMER_CHx_e{
    TIMER_CH0,
    TIMER_CH1,
    TIMER_CH2,
    TIMER_CH_NUM
}TIMER_CHx_e;


/** @brief timer0 peri clk&ctrl init, 1M clk src default
 * @param TIMx group of the timer.
 * @param enable timer0 enable/disable. 
 * @param clk_gate_dis timer0 clk gating disable/enable.(Must disable if read counter value frequently)
 * */
void timer_peri_init(uint8_t TIMx, uint8_t enable, uint8_t clk_gate_dis);

/** 
 * @brief set timer clk src div
 * @param div 0~15 -> div1 ~ div16(default div=0 -> div1)
 * */
void timer_div_set(uint8_t TIMx, uint8_t div);

/** 
 * @param TIMx 0~1, group of timer
 * @param CHx 0~2 channel of the timer， refer TIMER_CHx_e
 * @param cnt_val: timer count max value
*/
void timer_config(uint8_t TIMx, uint8_t CHx, uint32_t cnt_val, void* cbk);

void timer_enable(uint8_t TIMx, uint8_t CHx, uint8_t en);

/** @brief get intrrupt state of timer. return true/false*/
uint8_t timer_intr_stat_get(uint8_t TIMx, uint8_t CHx);
/** @brief get current counter value. */
uint32_t timer_cnt_val_get(uint8_t TIMx, uint8_t CHx);

void timer_timer0_isr(void);
void timer_timer1_isr(void);

#endif /* _DRV_TIMER_H_ */
