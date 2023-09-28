/**
 * **************************************************************************************
 * @file    driver_pwm.h
 * 
 * @author  Borg Xiao
 * @date    20230405
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
/*

- pwm drv use step(pwm mode):
    1. call `hal_pwm_wave_init()` to set pwm param.(set & realize isr callback if needed).
    2. call `hal_pwm_enable()` to enable/disable pwm hardware.
    3. call `hal_pwm_duty_set()` to change pwm duty on.

- pwm drv use step(timer mode):
    1. call `hal_pwm_timer_init()` to set timer param.(set & realize isr callback if needed).
    2. call `hal_pwm_enable()` to enable/disable pwm hardware.
    3. call `hal_pwm_timer_cycle_updt()` to change timer max count.

- pwm drv use step(general use):
    1. define & init struct `pwm_handle_t`.
    2. call `pwm_config()` to init pwm hardware.
    3. call `pwm_enable()` or `hal_pwm_enable()` to enable/disable pwm hardware.
    4. call `pwm_duty_set()` or `hal_pwm_duty_set()` to change duty on or cycle.

Note:
- pls check/define pwm channel resource macro before use(refer driver_pwm.h or define in user config, `exp: #define CONFIG_USE_PWM0`)
- call `pwm_peri_init()` to active pwm peripheral when system start.
- about group, channel:(pwm channel clk is same in one group)
    {PWM0, PWM1,  PWM2}, -- group0, ch 0 ~ 2
    {PWM3, PWM4,  PWM5}, -- group1, ch 0 ~ 2
    {PWM6, PWM7,  PWM8}, -- group2, ch 0 ~ 2
    {PWM9, PWM10, PWM11} -- group3, ch 0 ~ 2
*/

#ifndef _DRIVER_PWM_H_
#define _DRIVER_PWM_H_
#include "config.h"

#if CONFIG_DRIVER_PWM

//group 0
#define CONFIG_USE_PWM0    //GPIO3
#define CONFIG_USE_PWM1    //GPIO4
#define CONFIG_USE_PWM2    //GPIO5
//group 1
#define CONFIG_USE_PWM3    //GPIO6
#define CONFIG_USE_PWM4    //GPIO7
#define CONFIG_USE_PWM5    //GPIO8
//group 2
#define CONFIG_USE_PWM6    //GPIO18 / GPIO26 / GPIO30
#define CONFIG_USE_PWM7    //GPIO19 / GPIO27 / GPIO31
#define CONFIG_USE_PWM8    //GPIO20 / GPIO28 / GPIO32
//group 3
#define CONFIG_USE_PWM9    //GPIO21 / GPIO29 / GPIO33
#define CONFIG_USE_PWM10   //GPIO25
#define CONFIG_USE_PWM11   //GPIO35

// #define PWMx_2_GRPx(PWMx)      ((PWMx) / 3) // PWMx:0~11
// #define PWMx_2_CHx(PWMx)       ((PWMx) % 3) // PWMx:0~11

typedef enum _PWM_IO_e
{
    PWM0_IO     = GPIO3,
    PWM1_IO     = GPIO4,
    PWM2_IO     = GPIO5,

    PWM3_IO     = GPIO6,
    PWM4_IO     = GPIO7,
    PWM5_IO     = GPIO8,

    PWM6_IO     = GPIO18,
    PWM7_IO     = GPIO19,
    PWM8_IO     = GPIO20,

    PWM9_IO     = GPIO21,
    PWM10_IO    = GPIO25,
    PWM11_IO    = GPIO35,
    
    PWM6_IO_1   = GPIO26,
    PWM7_IO_1   = GPIO27,
    PWM8_IO_1   = GPIO28,
    PWM9_IO_1   = GPIO29,
    
    PWM6_IO_2   = GPIO30,
    PWM7_IO_2   = GPIO31,
    PWM8_IO_2   = GPIO32,
    PWM9_IO_2   = GPIO33,
}PWM_IO_e;

typedef enum _PWM_CLK_SRC_e{
    CLK_32KD,   //32KHz
    CLK_XTAL    //26MHz
}PWM_CLK_SRC_e;

typedef enum _PWM_GRPx_e{
    PWM_GRP0,
    PWM_GRP1,
    PWM_GRP2,
    PWM_GRP3,
    PWM_GRP_NUM
}PWM_GRPx_e;

typedef enum _PWM_CHx_e{
    PWM_CH0,
    PWM_CH1,
    PWM_CH2,
    PWM_CH_NUM
}PWM_CHx_e;

typedef struct _pwm_handle_t{
////init by internal  driver
    uint8_t     grp;    /* group 0 ~ 3, no need to config when define for auto init in pwm_config() */
    uint8_t     ch;     /* channel 0 ~ 2, no need to config when define for auto init in pwm_config() */
////init by user
    uint8_t     gpio;   /* mapped gpio, refer enum GPIO_PIN */
    uint8_t     gpio_en;/* enable/disable config gpio to pwm function, invalid for TIMER mode */
    uint8_t     mode;   /* 0:IDLE, 1:PWM Mode, 2:TIMER, 4:Capture, 5:Counter Negedge;*/
    uint8_t     clk_sel;/* 0:clk_32KD/1:clk_XTAL (Note: valid to all channel in the group) */
    uint16_t    clk_div;/* 0~0xFFFF -> 1~65536 divide */
    uint32_t    duty;   /* u32, PWM Duty-Cycle. Must smaller or equal to end-value */
    uint32_t    cycle;  /* u32, PWM counter end-value */
    void*       isr_cbk;/* intrrupt callback, interrupt disabled if set NULL. (Note: intr occurred at duty time)*/
} pwm_handle_t;


/* ********************************************************************************** */
/* ********************************************************************************** */
/* ********************************************************************************** */
// hal dirver

// hal public api
void hal_pwm_enable(uint8_t gpio, uint8_t en);
uint32_t hal_pwm_cnt_val_get(uint8_t gpio);

// pwm mode api
void hal_pwm_wave_init(
    uint8_t gpio,
    uint8_t gpio_en,
    uint32_t duty,      /* (unit:1/26us), u32, PWM Duty-Cycle. Must smaller or equal to end-value */
    uint32_t cycle,     /* (unit:1/26us), u32, PWM counter end-value */
    void *cbk
    );
void hal_pwm_duty_set(uint8_t gpio, uint32_t duty);
void hal_pwm_cycle_set(uint8_t gpio, uint32_t cycle);

// timer mode api
void hal_pwm_timer_init(
    uint8_t gpio,       /* mapped gpio, used by driver*/
    uint32_t cnt_val,   /* timer max count value (unit:1/26us) */
    void *cbk
    );
void hal_pwm_timer_cycle_updt(uint8_t gpio, uint32_t cnt_val);

/* ********************************************************************************** */
/* ********************************************************************************** */
/* ********************************************************************************** */
// low level dirver

void pwm_peri_init(void);    //peri clk&ctrl init, use 26MHz clk src default
void pwm_peri_uninit(void);

void pwm_config(pwm_handle_t *pwm_h); //only config, disabled default
void pwm_enable(pwm_handle_t *pwm_h, uint8_t en);

uint32_t pwm_cnt_val_get(pwm_handle_t *pwm_h); //read current counter value
void pwm_duty_set(pwm_handle_t *pwm_h);
void pwm_cycle_set(pwm_handle_t *pwm_h);

void pwm_cycle_updt_immd_en(pwm_handle_t *pwm_h, uint8_t en);
void pwm_isr(void);


#endif //CONFIG_DRIVER_PWM

#endif //_DRIVER_PWM_H_
