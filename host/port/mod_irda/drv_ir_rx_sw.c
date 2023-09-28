/**
 * **************************************************************************************
 * @file    driver_ir_rx_sw.c
 * 
 * @author  Borg Xiao
 * @date    20230904 create this file
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * **************************************************************************************
 * */
#include "config.h"

#ifdef CONFIG_DRV_IR_RX_SW
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bkreg.h"
#include "drv_system.h"
#include "driver_icu.h"
#include "driver_gpio.h"
#include "driver_pwm.h"
#include "irda_comm.h"
#include "msg_pub.h"


/* ------------------------------------------------------------------------- */
#define IR_RX_BITS      32

#define TIM_AGC         (9000 * TickPerUs)
#define TIM_AGC1        (4500 * TickPerUs)//some code may with head 4.5msL--4.5msH
#define TIM_LONG        (4500 * TickPerUs)
#define TIM_SHORT       (2500 * TickPerUs)
#define TIM_DATA        ( 560 * TickPerUs)
#define TIM_DATA0       ( 560 * TickPerUs)
#define TIM_DATA1       (1680 * TickPerUs)

#define TIM_THRD        ( 280 * TickPerUs)
#define TIM_THRD1       ( 500 * TickPerUs)


typedef enum _IR_RX_STATE_e{
    ST_IR_RX_Idle,
    ST_IR_RX_Head_L,  //detected a head pos edge // 9ms end
    ST_IR_RX_Head_H,  //detected a head neg edge // 4.5ms/2.5ms end
    ST_IR_RX_Data_L,  //detected a data pos edge // 560us end
    ST_IR_RX_Data_H,  //detected a data neg edge // 560/1680 end
}IR_RX_STATE_e;

enum{
    RT_IR_RX_ERR = -1,
    RT_IR_RX_NONE = 0,
};

/* ------------------------------------------------------------------------- */

#if defined(IRDA_RX_PWM_TIMER)//使用PWM作为定时器

#define TickPerUs       26
#define IR_RX_TIM_PWMIO     IRDA_RX_PWM_TIMER
void irda_rx_timer_init(uint32_t cnt) { hal_pwm_timer_init(IR_RX_TIM_PWMIO, cnt, NULL); }
void irda_rx_timer_enable(uint8_t en) { hal_pwm_enable(IR_RX_TIM_PWMIO, en); }
uint32_t irda_rx_time_get(void) 
{
    return hal_pwm_cnt_val_get(IR_RX_TIM_PWMIO);
}
#else
#define TickPerUs           1
#define irda_rx_time_get()  0
#endif

/* ------------------------------------------------------------------------- */
static uint8_t s_ir_rx_state = ST_IR_RX_Idle;
static uint32_t s_ir_rx_code = -1;


void irda_rx_sw_init(void)
{
	GPIO_PIN ir_rx_io = IRDA_RX_IO;
    gpio_config_new(ir_rx_io, GPIO_INPUT, GPIO_PULL_UP, GPIO_PERI_NONE);
    // gpio_int_enable(ir_rx_io, GPIO_INT_EDGE_NEG);
    gpio_int_enable(ir_rx_io, GPIO_INT_LEVEL_LOW);
#ifdef IRDA_RX_PWM_TIMER
    irda_rx_timer_init(-1);
    irda_rx_timer_enable(1);
#else
    IRDA_LOG_E("#error : the macro IRDA_RX_PWM_TIMER not defined !!!");
    while(1);
#endif
}

/** @return rx data (big endien) */
int irda_rx_code_get_sw(uint8_t *buff, int size)
{
    if(size < 4) return -1;
    uint32_t code = s_ir_rx_code;
    buff[0] = code & 0xFF;
    buff[1] = (code >> 8) & 0xFF;
    buff[2] = (code >> 16) & 0xFF;
    buff[3] = (code >> 24) & 0xFF;
    return 0;
}

void irda_rx_gpio_isr(uint32_t io0_31, uint32_t io32_39)
{
	GPIO_PIN ir_rx_io = IRDA_RX_IO;
    int intr_en = (ir_rx_io < 32) ? (io0_31 & (1 << ir_rx_io)) : (io32_39 & (1 << (ir_rx_io - 32)));
    if(!intr_en) goto RET;

    int err = 0;
    uint8_t next_intr_type;
    static uint32_t prv_time = 0;
    static uint8_t cnt = 0;
    static uint8_t repete_flag = 0;
    uint32_t cur_time = irda_rx_time_get();
    gpio_int_disable(ir_rx_io);
    #if 0
    // gpio_int_enable(ir_rx_io, s_ir_rx_state == 0 ? GPIO_INT_EDGE_POS : GPIO_INT_EDGE_NEG);
    gpio_int_enable(ir_rx_io, s_ir_rx_state == 0 ? GPIO_INT_LEVEL_HIGH : GPIO_INT_LEVEL_LOW);
    s_ir_rx_state = !s_ir_rx_state;
    REG_GPIO_0x0E = s_ir_rx_state ? 2 : 0;
    IRDA_LOG_D("IR_RX io:%d, st:%d\n", ir_rx_io, s_ir_rx_state);
    #endif
    int delta_time = cur_time - prv_time;
    prv_time = cur_time;
    switch (s_ir_rx_state)
    {
    case ST_IR_RX_Idle:
        next_intr_type = GPIO_INT_LEVEL_HIGH;
        s_ir_rx_state = ST_IR_RX_Head_L;
        repete_flag = 0;
        cnt = 0;
        break;
    case ST_IR_RX_Head_L://L->H
        next_intr_type = GPIO_INT_LEVEL_LOW;
        if(((delta_time > (TIM_AGC -TIM_THRD1)) && (delta_time < (TIM_AGC +TIM_THRD1)))
        || ((delta_time > (TIM_AGC1-TIM_THRD1)) && (delta_time < (TIM_AGC1+TIM_THRD1)))){
            s_ir_rx_state = ST_IR_RX_Head_H;
            cnt = 0;
        }else{
            err = RT_IR_RX_ERR;
        }
        break;
    case ST_IR_RX_Head_H://H->L
        next_intr_type = GPIO_INT_LEVEL_HIGH;
        if((delta_time > (TIM_LONG-TIM_THRD1)) && (delta_time < (TIM_LONG+TIM_THRD1))){
            IRDA_LOG_D("IR_RX head ok:%d\n", delta_time / TickPerUs);
            s_ir_rx_state = ST_IR_RX_Data_L;
        }else if((delta_time > (TIM_SHORT-TIM_THRD1)) && (delta_time < (TIM_SHORT+TIM_THRD1))){
            irda_rx_evt_set_repete();
            repete_flag = 1;
            s_ir_rx_state = ST_IR_RX_Data_L;
        }else{
            // err = RT_IR_RX_ERR;
            IRDA_LOG_D("IR_RX restart0:%d\n", delta_time / TickPerUs);
            s_ir_rx_state = ST_IR_RX_Head_L;
        }
        break;
    case ST_IR_RX_Data_L://L->H
        next_intr_type = GPIO_INT_LEVEL_LOW;
        if(cnt >= IR_RX_BITS){
            cnt = 0;
            irda_rx_evt_set_rx_cmp();
            s_ir_rx_state = ST_IR_RX_Idle;
            IRDA_LOG_D("\tIR_RX data:0x%08X\n", s_ir_rx_code);
        }else if((delta_time > (TIM_DATA-TIM_THRD)) && (delta_time < (TIM_DATA+TIM_THRD))){
            if(repete_flag){
                repete_flag = 0;
                s_ir_rx_state = ST_IR_RX_Idle;
                IRDA_LOG_D("IR_RX repete:%d\n", delta_time / TickPerUs);
            }else{
                s_ir_rx_state = ST_IR_RX_Data_H;
            }
        }else{
            err = RT_IR_RX_ERR;
        }
        break;
    case ST_IR_RX_Data_H://H->L
        next_intr_type = GPIO_INT_LEVEL_HIGH;
        if((delta_time > (TIM_DATA1-TIM_THRD)) && (delta_time < (TIM_DATA1+TIM_THRD))){
            IRDA_LOG_D("1");
            s_ir_rx_code |= (1 << cnt);
            cnt++;
            s_ir_rx_state = ST_IR_RX_Data_L;
        }else if((delta_time > (TIM_DATA0-TIM_THRD)) && (delta_time < (TIM_DATA0+TIM_THRD))){
            IRDA_LOG_D("0");
            s_ir_rx_code &= ~(1 << cnt);
            cnt++;
            s_ir_rx_state = ST_IR_RX_Data_L;
        }else{
            // err = RT_IR_RX_ERR;
            IRDA_LOG_D("IR_RX restart1:%d\n", delta_time / TickPerUs);
            s_ir_rx_state = ST_IR_RX_Head_L;
        }
        break;
    
    default: break;
    }

    if(err <= RT_IR_RX_ERR)
    {
        // next_intr_type = GPIO_INT_LEVEL_LOW;
        IRDA_LOG_E("ir rx err:%d, st:%d, dt:%d, cnt:%d\n", err, s_ir_rx_state, delta_time / TickPerUs, cnt);
        s_ir_rx_state = ST_IR_RX_Idle;
    }
    gpio_int_enable(ir_rx_io, next_intr_type);
RET:
    return;
}

#endif /* CONFIG_DRV_IR_RX_SW */
