/**
 * **************************************************************************************
 * @file    driver_pwm.h
 * 
 * @author  Borg Xiao
 * @date    20230415
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include <stdint.h>
#include "bkreg.h"
#include "drv_system.h"
#include "drv_timer.h"

#ifndef CEVAX2
    #define REG_GET(addr)                   (*(volatile unsigned int*)(addr))
    #define REG_SET(addr, val)              (*(volatile unsigned int*)(addr) = (val))
    #define system_peri_xxx_irq_enable      system_peri_mcu_irq_enable
    #define system_peri_xxx_irq_disable     system_peri_mcu_irq_disable
#else
    #define REG_GET(addr)                   (_in(((unsigned int)addr)))
    #define REG_SET(addr,val)               (_out((unsigned int)(val), (unsigned int)(addr)))
    #define system_peri_xxx_irq_enable      system_peri_dsp_irq_enable
    #define system_peri_xxx_irq_disable     system_peri_dsp_irq_disable
#endif

typedef union _tim_reg_cfg_t{
    struct
    {
        volatile uint32_t en                : 3;//ch2 | ch1 | ch0
        volatile uint32_t div               : 4;//the divider multiples = clk_div+1
        volatile uint32_t intr_stat         : 3;//ch2 | ch1 | ch0
        volatile uint32_t rsv0x03_bit31_10  :22;//reserve
    };
    volatile uint32_t fill;
}tim_reg_cfg_t;

typedef union _tim_reg_rd_set_t{
    struct
    {
        volatile uint32_t val_load          : 1;//Software write '1', and hardware clear it after finish read
        volatile uint32_t rsv0x04_bit2      : 1;
        volatile uint32_t ch                : 2;//Timer ch(2~0) Index For Read
        volatile uint32_t rsv0x04_bit31_4   :27;
    };
    volatile uint32_t fill;
}tim_reg_rd_set_t;

typedef struct _timer_reg_t{
    volatile uint32_t           val[3]; /* 0x00 ~ 0x02, end counter value set */
    volatile tim_reg_cfg_t      cfg;    /* 0x03 */
    volatile tim_reg_rd_set_t   rd_set; /* 0x04, read opration set */
    volatile uint32_t           count;  /* 0x05, timer current count read from */
}timer_reg_t;


#define tim_grp0                ((volatile timer_reg_t*)MDU_TIMG0_BASE_ADDR)
#define tim_grp1                ((volatile timer_reg_t*)MDU_TIMG1_BASE_ADDR)
static volatile timer_reg_t*    REG_BASE_TIMG[2] = {tim_grp0, tim_grp1};
#define REG_BASE_TIM(TIMx)      REG_BASE_TIMG[TIMx]//((TIMx == 0) ? tim_grp0 : tim_grp1)

typedef struct _s_timer_ctx_t{
    void (*cbk)(void);
}s_timer_ctx_t;

//intrrupt callback pointer
void timer_isr_empty(void) { /* LOG_E(TIM, "%s()\n", __FUNCTION__); */ }
static s_timer_ctx_t s_timer[TIMER_NUM][TIMER_CH_NUM] = { { { .cbk = timer_isr_empty } } };
// -------------------------------------------------------- timer function

/* ********************************************************************************** */
/* ********************************************************************************** */
/* ********************************************************************************** */

/** @brief timer0 peri clk&ctrl init, 1M clk src default
 * @param TIMx 0~1, group of the timer
 * @param enable 1:enable/0:disable. 
 * @param clk_gate_dis 1:disable/0:enable timer0 clk gating.(Must disable if read counter value frequently)
 * */
void timer_peri_init(uint8_t TIMx, uint8_t enable, uint8_t clk_gate_dis)
{
    if(TIMx == 0)
    {
        if(enable)
        {
            system_peri_clk_enable(SYS_PERI_CLK_TIM0);
            system_peri_xxx_irq_enable(SYS_PERI_IRQ_TIMER0);
            if(clk_gate_dis) system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_TIM0);
        }
        else
        {
            if(clk_gate_dis) system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_TIM0);
            system_peri_xxx_irq_disable(SYS_PERI_IRQ_TIMER0);
            system_peri_clk_disable(SYS_PERI_CLK_TIM0);
        }
    }
    else
    {
        if(enable)
        {
            system_peri_clk_enable(SYS_PERI_CLK_TIM1);
            system_peri_xxx_irq_enable(SYS_PERI_IRQ_TIMER1);
            if(clk_gate_dis) system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_TIM1);//32K clk src
        }
        else
        {
            if(clk_gate_dis) system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_TIM1);//32K clk src
            system_peri_xxx_irq_disable(SYS_PERI_IRQ_TIMER1);
            system_peri_clk_disable(SYS_PERI_CLK_TIM1);
        }
    }
}

/** 
 * @brief set timer clk src div
 * @param div 0~15 -> div1 ~ div16(default div=0 -> div1)
 * */
void timer_div_set(uint8_t TIMx, uint8_t div)
{
    volatile timer_reg_t* timer = REG_BASE_TIM(TIMx);
    uint32_t reg_cfg = REG_GET(&timer->cfg.fill);
    tim_reg_cfg_t* p_cfg = (tim_reg_cfg_t*)&reg_cfg;
    if(div > 15) div = 15;
    p_cfg->div = div;
    REG_SET(&timer->cfg.fill, p_cfg->fill);
}

/** 
 * @param TIMx 0~1, group of the timer refer TIMERx_e
 * @param CHx 0~2 channel of the timer，refer TIMER_CHx_e
 * @param cnt_val: timer count max value(default unit: timer0:1us, timer1:(1/32)ms)
 * */
void timer_config(uint8_t TIMx, uint8_t CHx, uint32_t cnt_val, void* cbk)
{
    volatile timer_reg_t* timer = REG_BASE_TIM(TIMx);
    uint32_t reg_cfg = REG_GET(&timer->cfg.fill);
    REG_SET(&timer->cfg.fill, (reg_cfg & ~(1 << CHx)));
    s_timer[TIMx][CHx].cbk = cbk ? cbk : timer_isr_empty;
    REG_SET(&timer->val[CHx], cnt_val);
}

void timer_enable(uint8_t TIMx, uint8_t CHx, uint8_t en)
{
    volatile timer_reg_t* timer = REG_BASE_TIM(TIMx);
    uint32_t reg_cfg = REG_GET(&timer->cfg.fill);
    reg_cfg = en ? (reg_cfg | (1 << CHx)) : (reg_cfg & ~(1 << CHx));
    REG_SET(&timer->cfg.fill, reg_cfg);
}

/** @brief get intrrupt state of timer. return true/false*/
uint8_t timer_intr_stat_get(uint8_t TIMx, uint8_t CHx)
{
    volatile timer_reg_t* timer = REG_BASE_TIM(TIMx);
    return ((REG_GET(&timer->cfg.fill) >> (7 + CHx)) & 0x1);
}

/** @brief get current counter value. */
uint32_t timer_cnt_val_get(uint8_t TIMx, uint8_t CHx)
{
    uint32_t cnt_val = 0;
    volatile timer_reg_t* timer = REG_BASE_TIM(TIMx);
    //suggest delay 3~4 cycle of timer clk src for each reg access(for clk src 1MHz, delay 2us before two reg access ensure total 4us interval in each reg access).
    //if timer group 0 clk src is divided by 2, then the clk src is 500KHz, so the delay should be total 8us[modify the sys_delay_us(2) to sys_delay_us(4)].
    sys_delay_us(2);
    REG_SET(&timer->rd_set.fill, ((CHx << 2) | 1));
    sys_delay_us(2);
    cnt_val = REG_GET(&timer->count);
    return cnt_val;
}

void timer_timer0_isr(void)
{
    uint32_t reg = REG_TIMG0_0x03;
    REG_TIMG0_0x03 = reg;//Clear interrupt flag
    if(reg & MSK_TIMG0_0x03_TIMER0_INT) { s_timer[0][0].cbk(); }
    if(reg & MSK_TIMG0_0x03_TIMER1_INT) { s_timer[0][1].cbk(); }
    if(reg & MSK_TIMG0_0x03_TIMER2_INT) { s_timer[0][2].cbk(); }
}

void timer_timer1_isr(void)
{
    uint32_t reg = REG_TIMG1_0x03;
    REG_TIMG1_0x03 = reg;//Clear interrupt flag
    if(reg & MSK_TIMG1_0x03_TIMER0_INT) { s_timer[1][0].cbk(); }
    if(reg & MSK_TIMG1_0x03_TIMER1_INT) { s_timer[1][1].cbk(); }
    if(reg & MSK_TIMG1_0x03_TIMER2_INT) { s_timer[1][2].cbk(); }
}

//EOF
