/**
 * **************************************************************************************
 * @file    driver_pwm.c
 * 
 * @author  Borg Xiao
 * @date    20230405
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#include "bkreg.h"
#include "drv_system.h"
#include "driver_gpio.h"
#include "driver_pwm.h"
#include "port_generic.h"

#if CONFIG_DRIVER_PWM

// -------------------------------------------------------- debug

// #define PWM_PRINTF          os_printf
// #define PWM_LOG_I(fmt,...)  PWM_PRINTF("[PWM|I]"fmt, ##__VA_ARGS__)
// #define PWM_LOG_W(fmt,...)  PWM_PRINTF("[PWM|W:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #define PWM_LOG_E(fmt,...)  PWM_PRINTF("[PWM|E:%s:%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
// #define PWM_LOG_D(fmt,...)  PWM_PRINTF("[PWM|D]"fmt, ##__VA_ARGS__)

// -------------------------------------------------------- pwm sources ctrl

//////// pwm group on/off ctrl

#if (defined(CONFIG_USE_PWM0) || defined(CONFIG_USE_PWM1) || defined(CONFIG_USE_PWM2))
    #define PWM_GRP0_ENABLE          1
#else
    #define PWM_GRP0_ENABLE          0
#endif

#if (defined(CONFIG_USE_PWM3) || defined(CONFIG_USE_PWM4) || defined(CONFIG_USE_PWM5))
    #define PWM_GRP1_ENABLE          1
#else
    #define PWM_GRP1_ENABLE          0
#endif

#if (defined(CONFIG_USE_PWM6) || defined(CONFIG_USE_PWM7) || defined(CONFIG_USE_PWM8))
    #define PWM_GRP2_ENABLE          1
#else
    #define PWM_GRP2_ENABLE          0
#endif

#if (defined(CONFIG_USE_PWM9) || defined(CONFIG_USE_PWM10) || defined(CONFIG_USE_PWM11))
    #define PWM_GRP3_ENABLE          1
#else
    #define PWM_GRP3_ENABLE          0
#endif


// -------------------------------------------------------- platform adaptive

#ifndef CEVAX2
    // #define DSP_RAM_BASIC_ADDR          0

    #define REG_GET(addr)           (*(volatile unsigned int*)(addr))
    #define REG_SET(addr, val)      (*(volatile unsigned int*)(addr) = (val))
    /* #define REG_SET_AND(addr, val)  (*(volatile unsigned int*)(addr) &= (val))
     #define REG_SET_OR(addr, val)   (*(volatile unsigned int*)(addr) |= (val))
     #define REG_BITS_SET(addr,bit,msk,val)  do{\
         (*(volatile unsigned int*)(addr)) &= ((unsigned int)~(msk));\
         (*(volatile unsigned int*)(addr)) |= (((val) << (bit)) & (msk));}while(0)*/

    #define gpio_config_pwm             gpio_config_new
    #define system_peri_xxx_irq_enable  system_peri_mcu_irq_enable
    #define gpio_output_set             gpio_output
#else
    // #define DSP_RAM_BASIC_ADDR          (0x03000000)

    #define REG_GET(addr)           (_in(((unsigned int)addr)))
    #define REG_SET(addr,val)       (_out((unsigned int)(val), (unsigned int)(addr)))
    // #define REG_SET_AND(addr,val)   (_out(_in(((unsigned int)addr)) & (unsigned int)(val), (unsigned int)(addr)))
    // #define REG_SET_OR(addr,val)    (_out(_in(((unsigned int)addr)) | (unsigned int)(val),(unsigned int)(addr)))
    // #define REG_BITS_SET(addr,bit,msk,val)  (_out(_in(((unsigned int)addr)) & ((unsigned int)~(msk)) | (((val)<<(bit))&(msk)), (unsigned int)(addr)))

    #define gpio_config_pwm             gpio_config
    #define system_peri_xxx_irq_enable  system_peri_dsp_irq_enable
#endif

// -------------------------------------------------------- pwm register struct
typedef union pwm_cfg{
    struct{
        volatile uint32_t ch0_en        : 1;
        volatile uint32_t ch0_int_en    : 1;
        volatile uint32_t ch0_mode      : 3; /* 0:IDLE    1:PWM Mode   2:TIMER    4:Capture    5:Counter Negedge*/
        volatile uint32_t ch1_en        : 1;
        volatile uint32_t ch1_int_en    : 1;
        volatile uint32_t ch1_mode      : 3;
        volatile uint32_t ch2_en        : 1;
        volatile uint32_t ch2_int_en    : 1;
        volatile uint32_t ch2_mode      : 3;
        volatile uint32_t rfu           : 1;
        volatile uint32_t div           : 4; /* the divider multiples = clk_div+1,  (0~0xFFFF -> div2~65536) */
        volatile uint32_t reserved      :12;
    };
    volatile uint32_t fill;
}pwm_cfg_t;

typedef union pwm_stat{
    struct{
        volatile uint32_t ch0_int   : 1;
        volatile uint32_t ch1_int   : 1;
        volatile uint32_t ch2_int   : 1;
        volatile uint32_t reserved  :29;
    };
    volatile uint32_t fill;
}pwm_stat_t;

//PWM Update immideatly ;  it can be used when you want to update pwm_end_value  during  pwm working
typedef union pwm_updt_cycle{
    struct{
        volatile uint32_t en        : 1;
        volatile uint32_t reserved  :31;
    };
    volatile uint32_t fill;
}pwm_updt_cycle_t;

typedef struct pwm_argv{
    volatile uint32_t   cycle;
    volatile uint32_t   duty;
    volatile uint32_t   cap_val;
}pwm_argv_t;

typedef union pwm_cnt_rd_cfg{
    struct{
        volatile uint32_t  val_updt : 1;//Software write '1', and hardware clear it after finish read
        volatile uint32_t  ch       : 2;//pwm ch(2~0) Index For Read
        volatile uint32_t  reserved :29;
    };
    volatile uint32_t fill;
}pwm_cnt_rd_cfg_t;

typedef struct pwm_reg{
    pwm_cfg_t           cfg;        /* 0x00 */
    pwm_stat_t          stat;       /* 0x01 */
    pwm_argv_t          argv[3];
    pwm_updt_cycle_t    updt[3];  
    pwm_cnt_rd_cfg_t    read;
    volatile uint32_t   count; //Timer(2~0) Counter
}pwm_reg_t;

// -------------------------------------------------------- pwm logic related

//register base address
#if 1
#define REG_PWM_GP(x)        ((volatile pwm_reg_t*)(MDU_PWMG0_BASE_ADDR + 0x100 * x))
static volatile pwm_reg_t* REG_PWM_G[4] = {REG_PWM_GP(0), REG_PWM_GP(1), REG_PWM_GP(2), REG_PWM_GP(3)};
#define REG_PWMG(x)        REG_PWM_G[x]
#else
#define REG_PWMG(x)        ((volatile pwm_reg_t*)(MDU_PWMG0_BASE_ADDR + 0x100 * x))
#endif

typedef struct _pwm_ctx{
    void (*cbk)(void);
}pwm_ctx;

//intrrupt callback pointer
void pwm_isr_empty(void) { LOG_E(PWM, "%s()\n", __FUNCTION__); }
static pwm_ctx s_pwm[PWM_GRP_NUM][PWM_CH_NUM] = { { { .cbk = pwm_isr_empty } } };

//get group and channel from gpio
#define ____GRP_    PWM_GRP_NUM
#define ____CH_     PWM_CH_NUM
static const uint8_t pwm_grp_ch[GPIO_NUM][2] = {
    {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {PWM_GRP0, PWM_CH0}, {PWM_GRP0, PWM_CH1}, 
    {PWM_GRP0, PWM_CH2}, {PWM_GRP1, PWM_CH0}, {PWM_GRP1, PWM_CH1}, {PWM_GRP1, PWM_CH2}, {____GRP_, ____CH_}, 
    {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, 
    {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {PWM_GRP2, PWM_CH0}, {PWM_GRP2, PWM_CH1}, 
    {PWM_GRP2, PWM_CH2}, {PWM_GRP3, PWM_CH0}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, 
    {PWM_GRP3, PWM_CH1}, {PWM_GRP2, PWM_CH0}, {PWM_GRP2, PWM_CH1}, {PWM_GRP2, PWM_CH2}, {PWM_GRP3, PWM_CH0}, 
    {PWM_GRP2, PWM_CH0}, {PWM_GRP2, PWM_CH1}, {PWM_GRP2, PWM_CH2}, {PWM_GRP3, PWM_CH0}, {____GRP_, ____CH_}, 
    {PWM_GRP3, PWM_CH2}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, {____GRP_, ____CH_}, 
};
#define pwm_grp_ch_def_by_gpio(io)     uint8_t grp = pwm_grp_ch[io][0];uint8_t ch = pwm_grp_ch[io][1];


// -------------------------------------------------------- pwm function

/* ********************************************************************************** */
/* ********************************************************************************** */
/* ********************************************************************************** */
// hal dirver

// --------------------------------------------- hal public api

void hal_pwm_enable(uint8_t gpio, uint8_t en)
{
    pwm_grp_ch_def_by_gpio(gpio);
    volatile pwm_reg_t *reg = REG_PWMG(grp);
    uint32_t reg_cfg = REG_GET(&reg->cfg.fill);
    uint32_t mask_reg_cfg_en = MAX_PWMG0_0x00_PWM0_EN << (SFT_PWMG0_0x00_PWM0_EN + (5 * ch));
    REG_SET(&reg->cfg.fill, en ? (reg_cfg | mask_reg_cfg_en) : (reg_cfg & ~mask_reg_cfg_en));
}

//26MHz clk src default
uint32_t hal_pwm_cnt_val_get(uint8_t gpio)
{
    pwm_grp_ch_def_by_gpio(gpio);
    volatile pwm_reg_t *reg = REG_PWMG(grp);
    pwm_cnt_rd_cfg_t rd_cfg;
    rd_cfg.ch = ch;
    rd_cfg.val_updt = 1;
    //delay 3~4 cycle of pwm clk at least to avoid digital crash.
    sys_delay_cycle(4 * system_cpu_freq() / 26000000);
    REG_SET(&reg->read, rd_cfg.fill);
    sys_delay_cycle(4 * system_cpu_freq() / 26000000);
    return REG_GET(&reg->count);
}

// --------------------------------------------- hal pwm mode api

//pwm mode init
void hal_pwm_wave_init(
    uint8_t gpio,
    uint8_t gpio_en,
    uint32_t duty,      /* (unit:1/26us), u32, PWM Duty-Cycle. Must smaller or equal to end-value */
    uint32_t cycle,     /* (unit:1/26us), u32, PWM counter end-value */
    void *cbk
    )
{
    pwm_handle_t pwm_ctx = {
        .gpio       = gpio,
        .gpio_en    = gpio_en,
        .mode       = 1,        /* 0:IDLE, 1:PWM Mode, 2:TIMER, 4:Capture, 5:Counter Negedge;*/
        .clk_sel    = CLK_XTAL, /* 0:clk_32KD,  1:clk_XTAL */
        .clk_div    = 0,        /* 0~0xFFFF -> 1~65536 divide */
        .duty       = duty,
        .cycle      = cycle,
        .isr_cbk    = cbk,
    };
    pwm_config(&pwm_ctx);//only config, disabled default
}

//call at pwm working state is allowed
void hal_pwm_duty_set(uint8_t gpio, uint32_t duty)
{
    pwm_grp_ch_def_by_gpio(gpio);
    REG_SET(&REG_PWMG(grp)->argv[ch].duty, duty);
}
//call at pwm working state is allowed
void hal_pwm_cycle_set(uint8_t gpio, uint32_t cycle)
{
    pwm_grp_ch_def_by_gpio(gpio);
    REG_SET(&REG_PWMG(grp)->argv[ch].cycle, cycle);
}


// --------------------------------------------- hal timer mode api

//timer mode init
void hal_pwm_timer_init(
    uint8_t gpio,       /* mapped gpio, used by driver*/
    uint32_t cnt_val,   /* timer max count value (unit:1/26us) */
    void *cbk
    )
{
    pwm_handle_t pwm_ctx = {
        .gpio       = gpio,
        .gpio_en    = 0,
        .mode       = 2,        /* 0:IDLE, 1:PWM Mode, 2:TIMER, 4:Capture, 5:Counter Negedge;*/
        .clk_sel    = CLK_XTAL, /* 0:clk_32KD,  1:clk_XTAL */
        .clk_div    = 0,        /* 0~0xFFFF -> 1~65536 divide */
        .duty       = cnt_val,
        .cycle      = cnt_val,
        .isr_cbk    = cbk,
    };
    pwm_config(&pwm_ctx);//only config, disabled default
}

//update timer max count value
void hal_pwm_timer_cycle_updt(uint8_t gpio, uint32_t cnt_val)
{
    pwm_grp_ch_def_by_gpio(gpio);
    volatile pwm_reg_t *reg = REG_PWMG(grp);
    uint32_t reg_cfg = REG_GET(&reg->cfg.fill);
    uint32_t mask_reg_cfg_en = MAX_PWMG0_0x00_PWM0_EN << (SFT_PWMG0_0x00_PWM0_EN + (5 * ch));
    REG_SET(&reg->cfg.fill, (reg_cfg & ~mask_reg_cfg_en));//disable pwm module
    REG_SET(&REG_PWMG(grp)->argv[ch].duty, cnt_val); //set duty
    REG_SET(&REG_PWMG(grp)->argv[ch].cycle, cnt_val);//set cycle
    REG_SET(&reg->cfg.fill, (reg_cfg | mask_reg_cfg_en));//enable pwm module
}



/* ********************************************************************************** */
/* ********************************************************************************** */
/* ********************************************************************************** */
// low level dirver

//grp: PWM group index, 0~3
//pwm_clk_select， //0:CLK_32KD,  1:CLK_XTAL(26MHz)
void pwm_grp_clk_src_set(uint8_t grp, uint8_t clk_sel)
{
    uint32_t msk_clk_sel[4] = {MSK_SYSTEM_0x01_PWMS0_SEL, MSK_SYSTEM_0x01_PWMS1_SEL, MSK_SYSTEM_0x01_PWMS2_SEL, MSK_SYSTEM_0x01_PWMS3_SEL};
    if(clk_sel){
        REG_SET(&REG_SYSTEM_0x01, REG_GET(&REG_SYSTEM_0x01) | msk_clk_sel[grp]);
    }else{
        REG_SET(&REG_SYSTEM_0x01, REG_GET(&REG_SYSTEM_0x01) & ~msk_clk_sel[grp]);
    }
    // uint32_t msk_clk_sel[4] = {SYS_CTRL_CMD_PWM0_CLK_SEL, SYS_CTRL_CMD_PWM1_CLK_SEL, SYS_CTRL_CMD_PWM2_CLK_SEL, SYS_CTRL_CMD_PWM3_CLK_SEL};
    //system_ctrl(msk_clk_sel[grp], clk_sel);
}

//grp: PWM group index, 0~3
//en: clock power down
void pwm_grp_clk_en(uint8_t grp, uint8_t en)
{
    uint32_t msk_clk_pwd[4] = {MSK_SYSTEM_0x05_PWMS0_PWD, MSK_SYSTEM_0x05_PWMS1_PWD, MSK_SYSTEM_0x05_PWMS2_PWD, MSK_SYSTEM_0x05_PWMS3_PWD};
    uint32_t value = en ? (REG_GET(&REG_SYSTEM_0x05) & ~msk_clk_pwd[grp]) : (REG_GET(&REG_SYSTEM_0x05) | msk_clk_pwd[grp]);
    REG_SET(&REG_SYSTEM_0x05, value);
    // uint32_t msk_clk_pwd[4] = {SYS_PERI_CLK_PWMS0, SYS_PERI_CLK_PWMS1, SYS_PERI_CLK_PWMS2, SYS_PERI_CLK_PWMS3};
    // system_peri_clk_enable(msk_clk_pwd[grp]);
}

// SFT_SYSTEM_0x1A_PWM6_POS
void pwm_peri_init(void)
{
#if PWM_GRP0_ENABLE
    pwm_grp_clk_src_set(PWM_GRP0, 1);
    pwm_grp_clk_en(PWM_GRP0, 1);
#endif
#if PWM_GRP1_ENABLE
    pwm_grp_clk_src_set(PWM_GRP1, 1);
    pwm_grp_clk_en(PWM_GRP1, 1);
#endif
#if PWM_GRP2_ENABLE
    pwm_grp_clk_src_set(PWM_GRP2, 1);
    pwm_grp_clk_en(PWM_GRP2, 1);
#endif
#if PWM_GRP3_ENABLE
    pwm_grp_clk_src_set(PWM_GRP3, 1);
    pwm_grp_clk_en(PWM_GRP3, 1);
#endif
    system_peri_clk_gating_disable(SYS_PERI_CLK_GATING_PWMS);
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_PWM);
}

void pwm_peri_uninit(void)
{
    system_peri_mcu_irq_disable(SYS_PERI_IRQ_PWM);
    system_peri_clk_gating_enable(SYS_PERI_CLK_GATING_PWMS);
#if PWM_GRP0_ENABLE
    pwm_grp_clk_en(PWM_GRP0, 0);
#endif
#if PWM_GRP1_ENABLE
    pwm_grp_clk_en(PWM_GRP1, 0);
#endif
#if PWM_GRP2_ENABLE
    pwm_grp_clk_en(PWM_GRP2, 0);
#endif
#if PWM_GRP3_ENABLE
    pwm_grp_clk_en(PWM_GRP3, 0);
#endif
}

void pwm_config(pwm_handle_t *pwm_h)
{
    pwm_grp_ch_def_by_gpio(pwm_h->gpio);
    volatile pwm_reg_t *reg = REG_PWMG(grp);
    uint32_t reg_cfg = REG_GET(&reg->cfg.fill);
    int cfg_offset = 5 * ch;
    uint8_t intr_en = 0;

    pwm_h->grp = grp;
    pwm_h->ch = ch;

    if(pwm_h->isr_cbk) {
        s_pwm[grp][ch].cbk = pwm_h->isr_cbk;
        intr_en = 1;
    }

    /* config group */
    pwm_grp_clk_src_set(grp, pwm_h->clk_sel);

    //clk div config
    reg_cfg &= ~MSK_PWMG0_0x00_PRE_DIV;
    reg_cfg |= (pwm_h->clk_div << SFT_PWMG0_0x00_PRE_DIV);

    /* config channel of the group */
    reg_cfg &= ~(MAX_PWMG0_0x00_PWM0_EN << (SFT_PWMG0_0x00_PWM0_EN + cfg_offset));
    reg_cfg &= ~(MAX_PWMG0_0x00_PWM0_INT_EN << (SFT_PWMG0_0x00_PWM0_INT_EN + cfg_offset));
    reg_cfg &= ~(MAX_PWMG0_0x00_PWM0_MODE << (SFT_PWMG0_0x00_PWM0_MODE + cfg_offset));
    reg_cfg |= (pwm_h->mode << (SFT_PWMG0_0x00_PWM0_MODE + cfg_offset));
    reg_cfg |= (intr_en << (SFT_PWMG0_0x00_PWM0_INT_EN + cfg_offset));
    // if (enable) reg_cfg |= (MAX_PWMG0_0x00_PWM0_EN << (SFT_PWMG0_0x00_PWM0_EN + cfg_offset));

    REG_SET(&reg->argv[ch].cycle, pwm_h->cycle);
    REG_SET(&reg->argv[ch].duty, pwm_h->duty);
    // REG_SET(&reg->updt[ch].fill, 1);
    REG_SET(&reg->cfg.fill, reg_cfg);
    if(pwm_h->gpio_en) gpio_config_pwm(pwm_h->gpio, GPIO_INOUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);
}

void pwm_enable(pwm_handle_t *pwm_h, uint8_t en)
{
    volatile pwm_reg_t *reg = REG_PWMG(pwm_h->grp);
    uint32_t reg_cfg = REG_GET(&reg->cfg.fill);
    uint32_t mask_reg_cfg_en = MAX_PWMG0_0x00_PWM0_EN << (SFT_PWMG0_0x00_PWM0_EN + (5 * pwm_h->ch));
    REG_SET(&reg->cfg.fill, en ? (reg_cfg | mask_reg_cfg_en) : (reg_cfg & ~mask_reg_cfg_en));
}

//get current counter value
uint32_t pwm_cnt_val_get(pwm_handle_t *pwm_h)
{
    volatile pwm_reg_t *reg = REG_PWMG(pwm_h->grp);
    pwm_cnt_rd_cfg_t rd_cfg;
    rd_cfg.ch = pwm_h->ch;
    rd_cfg.val_updt = 1;
    //delay 3~4 cycle of pwm clk at least to avoid digital crash.
    if(pwm_h->clk_sel)  sys_delay_cycle(4 * system_cpu_freq() / 26000000);
    else                sys_delay_us(4 * 1000000 / 32000);
    REG_SET(&reg->read, rd_cfg.fill);
    if(pwm_h->clk_sel)  sys_delay_cycle(4 * system_cpu_freq() / 26000000);
    else                sys_delay_us(4 * 1000000 / 32000);
    return REG_GET(&reg->count);
}

//call at pwm working state is allowed
void pwm_duty_set(pwm_handle_t *pwm_h)
{
    REG_SET(&REG_PWMG(pwm_h->grp)->argv[pwm_h->ch].duty, pwm_h->duty);
}

//call at pwm working state is allowed
void pwm_cycle_set(pwm_handle_t *pwm_h)
{
    REG_SET(&REG_PWMG(pwm_h->grp)->argv[pwm_h->ch].cycle, pwm_h->cycle);
    // REG_SET(&REG_PWMG(pwm_h->grp)->updt[pwm_h->ch].fill, 1);
}

//set end value update immediatley
void pwm_cycle_updt_immd_en(pwm_handle_t *pwm_h, uint8_t en)
{
    REG_SET(&REG_PWMG(pwm_h->grp)->updt[pwm_h->ch].fill, !!en);
}

#if 1
void pwm_isr(void)
{
#if (PWM_GRP0_ENABLE || PWM_GRP1_ENABLE || PWM_GRP2_ENABLE || PWM_GRP3_ENABLE)
    uint32_t state;
#endif
#if PWM_GRP0_ENABLE
    state = REG_PWMG0_0x01;
    REG_PWMG0_0x01 = state;//Clear interrupt flag
    #if defined(CONFIG_USE_PWM0)
    if(state & MSK_PWMG0_0x01_PWM0_INT) { s_pwm[0][0].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM1)
    if(state & MSK_PWMG0_0x01_PWM1_INT) { s_pwm[0][1].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM2)
    if(state & MSK_PWMG0_0x01_PWM2_INT) { s_pwm[0][2].cbk(); }
    #endif
#endif
#if PWM_GRP1_ENABLE
    state = REG_PWMG1_0x01;
    REG_PWMG1_0x01 = state;//Clear interrupt flag
    #if defined(CONFIG_USE_PWM3)
    if(state & MSK_PWMG1_0x01_PWM0_INT) { s_pwm[1][0].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM4)
    if(state & MSK_PWMG1_0x01_PWM1_INT) { s_pwm[1][1].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM5)
    if(state & MSK_PWMG1_0x01_PWM2_INT) { s_pwm[1][2].cbk(); }
    #endif
#endif
#if PWM_GRP2_ENABLE
    state = REG_PWMG2_0x01;
    REG_PWMG2_0x01 = state;//Clear interrupt flag
    #if defined(CONFIG_USE_PWM6)
    if(state & MSK_PWMG2_0x01_PWM0_INT) { s_pwm[2][0].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM7)
    if(state & MSK_PWMG2_0x01_PWM1_INT) { s_pwm[2][1].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM8)
    if(state & MSK_PWMG2_0x01_PWM2_INT) { s_pwm[2][2].cbk(); }
    #endif
#endif
#if PWM_GRP3_ENABLE
    state = REG_PWMG3_0x01;
    REG_PWMG3_0x01 = state;//Clear interrupt flag
    #if defined(CONFIG_USE_PWM9)
    if(state & MSK_PWMG3_0x01_PWM0_INT) { s_pwm[3][0].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM10)
    if(state & MSK_PWMG3_0x01_PWM1_INT) { s_pwm[3][1].cbk(); }
    #endif
    #if defined(CONFIG_USE_PWM11)
    if(state & MSK_PWMG3_0x01_PWM2_INT) { s_pwm[3][2].cbk(); }  
    #endif
#endif
}
#else
void pwm_isr(void)
{
    for(int grp = 0; grp < 4; grp++)
    {
        uint32_t state = REG_GET(&REG_PWMG(grp)->stat);
        REG_SET(&REG_PWMG(grp)->stat, state);
        for(int ch = 0; ch < 3; ch++){ if(state & (1 << ch)) { s_pwm[grp][ch].cbk(); } }
    }
}
#endif

#endif //CONFIG_DRIVER_PWM

