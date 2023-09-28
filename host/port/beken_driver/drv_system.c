/**
 **************************************************************************************
 * @file    drv_system.c
 * @brief   Driver API for SYSTEM
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include "bautil.h"
#include "bkreg.h"
#include "drv_system.h"
#include "drv_audio.h"

static uint32_t system_apll_frequency = 0;
static uint32_t system_dpll_frequency = 0;
static uint32_t system_core_frequency = 26000000;
static uint32_t system_cpu_frequency  = 26000000;
static uint32_t system_dsp_frequency  = 26000000;
static uint32_t sys_delay_calib_val   = 26;

#if     defined(__BA2__)
#define NOP()   __asm__("b.nop 5;")
#elif   defined(CEVAX2)
#define NOP()   __asm__("PCU.nop;")
#elif   defined(TEAKLITE4)
#define NOP()   __asm__("nop;")
#else
#define NOP()
#endif

/*
 * True Random Generator module
 */
void sys_trng_init(void) 
{
    REG_TRNG_0x00 |= MAX_TRNG_0x00_EN;  // True Random Generator Enable
}

void sys_trng_uninit(void) 
{
    REG_TRNG_0x00 &= ~MAX_TRNG_0x00_EN;  // True Random Generator disable
}

uint32_t sys_trng_random_get(void) 
{
    return REG_TRNG_0x01;
}

void sys_delay_cycle(uint32_t n)
{
    n /= 6; while(n--) NOP();
}

void sys_delay_us(uint32_t us)
{
    sys_delay_cycle(sys_delay_calib_val * us);
}

void sys_delay_ms(uint32_t ms)
{
    sys_delay_cycle(sys_delay_calib_val * 1000 * ms);
}

void system_freq_update(void)
{
    uint32_t reg, sel, div;

    reg = REG_SYSTEM_0x4B & 0x3FFFFFFF;

    switch(reg)
    {
    case 0x0DE517A9:
        system_apll_frequency = SYS_APLL_90p3168_MHZ;
        break;
    case 0x0F1FAA4C:
        system_apll_frequency = SYS_APLL_98p3040_MHZ;
        break;
    default:
        break;
    }

    reg = (REG_SYSTEM_0x47 >> 12) & 0x1FF;
    system_dpll_frequency = (reg * 52 + 16) / 32 * 1000000;

    reg = REG_SYSTEM_0x00;
    sel = (reg & MSK_SYSTEM_0x00_CORE_SEL) >> SFT_SYSTEM_0x00_CORE_SEL;
    div = (reg & MSK_SYSTEM_0x00_CORE_DIV) >> SFT_SYSTEM_0x00_CORE_DIV;

    switch(sel)
    {
    case SYS_CLK_XTAL:
    case SYS_CLK_XTAL | 0x2:
        system_core_frequency = 26000000;
        break;
    case SYS_CLK_DPLL:
        system_core_frequency = system_dpll_frequency;
        break;
    case SYS_CLK_APLL:
        system_core_frequency = system_apll_frequency;
        break;
    default:
        break;
    }

    system_core_frequency = system_core_frequency / (div ? div : 1);
    system_cpu_frequency  = system_core_frequency >> ((reg & MSK_SYSTEM_0x00_CPU_DIV2) >> SFT_SYSTEM_0x00_CPU_DIV2);
    system_dsp_frequency  = system_core_frequency >> ((reg & MSK_SYSTEM_0x00_DSP_DIV2) >> SFT_SYSTEM_0x00_DSP_DIV2);
    #if defined(__BA2__)
    sys_delay_calib_val   = system_cpu_frequency / 1000000;
    #else
    sys_delay_calib_val   = system_dsp_frequency / 1000000;
    #endif
    sys_delay_calib_val   = sys_delay_calib_val ? sys_delay_calib_val : 1;
}

void system_core_clk_set(SYS_CLK_SRC sel, uint32_t div)
{
    uint32_t reg = REG_SYSTEM_0x00;
    reg &= ~(MSK_SYSTEM_0x00_CORE_SEL | MSK_SYSTEM_0x00_CORE_DIV);
    reg |= (sel << SFT_SYSTEM_0x00_CORE_SEL) | (div << SFT_SYSTEM_0x00_CORE_DIV);
    REG_SYSTEM_0x00 = reg;

    switch(sel)
    {
    case SYS_CLK_XTAL:
        system_core_frequency = 26000000;
        break;
    case SYS_CLK_DPLL:
        system_core_frequency = system_dpll_frequency;
        break;
    case SYS_CLK_APLL:
        system_core_frequency = system_apll_frequency;
        break;
    default:
        break;
    }

    system_core_frequency = system_core_frequency / (div ? div : 1);
    system_cpu_frequency  = system_core_frequency >> ((reg & MSK_SYSTEM_0x00_CPU_DIV2) >> SFT_SYSTEM_0x00_CPU_DIV2);
    system_dsp_frequency  = system_core_frequency >> ((reg & MSK_SYSTEM_0x00_DSP_DIV2) >> SFT_SYSTEM_0x00_DSP_DIV2);
    sys_delay_calib_val   = system_cpu_frequency / 1000000;
    sys_delay_calib_val   = sys_delay_calib_val ? sys_delay_calib_val : 1;
}

uint32_t system_core_freq(void)
{
    return system_core_frequency;
}

uint32_t system_cpu_freq(void)
{
    return system_cpu_frequency;
}
extern void os_delay_us(uint32_t usec) ;
void system_cpu_halt(void)
{
    set_spr(SPR_PM_EVENT_CTRL, 0x01);
    NOP();NOP();NOP();
    NOP();NOP();NOP();
    REG_SYSTEM_0x08 = 1;
    os_delay_us(1);
    set_spr(SPR_PM_EVENT_CTRL, 0x00);
}

uint32_t system_dsp_freq(void)
{
    return system_dsp_frequency;
}

static uint32_t s_system_0x4d_save = 0; // for xtal calibration;
void system_set_0x4d_reg(uint32_t reg_val)
{
    s_system_0x4d_save &= ~(0x3f);    
    s_system_0x4d_save |= reg_val;
    REG_SYSTEM_0x4D = s_system_0x4d_save;
}
uint8_t system_get_0x4d_reg(void)
{
    return (s_system_0x4d_save & 0x3f);
}

void system_analog_regs_init(void)
{
    REG_PMU_0x02 = 0;
    REG_PMU_0x03 = 0;
    REG_PMU_0x11 = 0;
    REG_GPIO_0x30 = 0;
    REG_GPIO_0x31 = 0;
    REG_GPIO_0x32 = 0;
    REG_GPIO_0x33 = 0;
    REG_GPIO_0x34 = 0;
    
    REG_SYSTEM_0x40 = 0x8E14B699;//0x8E14B819;xiaohuihui@02/16
    REG_SYSTEM_0x41 = 0x4E45E6BA;//0x4E45EFFA;
    REG_SYSTEM_0x42 = 0xF814EFC4;//0xF8176fC4;buck 2M->1M;
    REG_SYSTEM_0x43 = 0xFA8E083F;//vlcf fine tune enable,@tenglong 10.29 //0xFA0E083F;//0xFA0E08BF;
    // REG_SYSTEM_0x44 = 0xEB0001AF;//cv threshold calibration enable(bit11),@tenglong, enable after new VCV calibration 0xEB0009AF or use 0xEB0001AF;//0xEB00016F; // AON,=0x07    // REG_SYSTEM_0x44 = 0xEB0001AF;//cv threshold calibration enable(bit11),@tenglong, enable after new VCV calibration 0xEB0009AF or use 0xEB0001AF;//0xEB00016F; // AON,=0x07
    REG_SYSTEM_0x44 = 0xEB0001FF;//VDD12 1.1v->1.2V for DSP crash issue. ++ by Borg @230319
    REG_SYSTEM_0x45 = 0x4449110A;//0x44491108;
    REG_SYSTEM_0x46 = 0xEEFF8103;//0xEE078103; 
    REG_SYSTEM_0x47 = 0x20AF690A;
    REG_SYSTEM_0x48 = 0x3090A989;
    REG_SYSTEM_0x49 = 0x0EB80728;
    REG_SYSTEM_0x4A = 0xE0108686;
    REG_SYSTEM_0x4B = 0x80000000 | AUDIO_DIV_441K;
    REG_SYSTEM_0x4C = 0x282DB896;//0x282838B6;xiaohuihui@02/21
    REG_SYSTEM_0x4D = s_system_0x4d_save = 0x8541900; //0x0914191C;fot xtal self-test
    REG_SYSTEM_0x4E = 0x2A8850C8;  // BIT31:disable the high pssr mode in buff of 26M;
    REG_SYSTEM_0x4F = 0x1F818000;
    REG_SYSTEM_0x50 = 0x00002002;
    REG_SYSTEM_0x51 = 0x00000000;
    REG_SYSTEM_0x52 = 0x00000000;
    REG_SYSTEM_0x53 = 0x00000000;
    REG_SYSTEM_0x54 = 0x00000000;
    REG_SYSTEM_0x55 = 0xD8001400;
    REG_SYSTEM_0x56 = 0x00000000;
    REG_SYSTEM_0x57 = 0x80102000;
    REG_SYSTEM_0x58 = 0x00000100;
    REG_SYSTEM_0x59 = 0x00000100;
    REG_SYSTEM_0x5A = 0x00000000;
    REG_SYSTEM_0x5B = 0x00020204;
    REG_SYSTEM_0x5C = 0x9003DFDF;//0x9003DFC0;//0x9003C0C0;
    REG_SYSTEM_0x5D = 0x00000000;
    REG_SYSTEM_0x5E = 0x00000000;
    REG_SYSTEM_0x5F = 0x00000000;
    sys_delay_cycle(6);
    REG_SYSTEM_0x5C|= (1 << 17);//FIXME
}
void system_apll_toggle(void)
{
	uint32_t reg = REG_SYSTEM_0x4A;	
	REG_SYSTEM_0x4A = (reg | (1 << 18));
	sys_delay_us(200);
	REG_SYSTEM_0x4A = reg & ~(1 << 18);
}
void system_apll_config(uint32_t freq)
{
    uint32_t reg = REG_SYSTEM_0x4A;
    uint8_t is_changed = (freq != system_apll_frequency);
    if(!is_changed) return;
    reg &= ~(3 << 29);
    reg |= (1 << 31);
    /* Sample rate = (fvco) * 2^24/13/4096 */
    switch(freq)
    {
    case SYS_APLL_90p3168_MHZ:
        reg |= (3 << 29);
        REG_SYSTEM_0x4B = 0x80000000 | AUDIO_DIV_441K;//0x0DE517A9;
        REG_SYSTEM_0x4A = (reg | (1 << 18));
        sys_delay_us(200);
        REG_SYSTEM_0x4A = reg & ~(1 << 18);
        break;
    case SYS_APLL_98p3040_MHZ:
        reg |= (3 << 29);
        REG_SYSTEM_0x4B = 0x80000000 | AUDIO_DIV_48K;//0x0F1FAA4C;
        REG_SYSTEM_0x4A = (reg | (1 << 18));
        sys_delay_us(200);
        REG_SYSTEM_0x4A = reg & ~(1 << 18);
        break;
    default:
        //TODO
        break;
    }

    system_apll_frequency = freq;
}

uint32_t system_apll_freq(void)
{
    return system_apll_frequency;
}

void system_apll_output(uint32_t enable, uint32_t div)
{
    uint32_t reg = REG_SYSTEM_0x4A;

    if(enable)
    {
        reg &= ~(3 << 26);
        reg |= (1 << 25) | ((div & 7) << 26);
    }
    else
    {
        reg &= ~(1 << 25);
    }

    REG_SYSTEM_0x4A = reg;
}

void system_dpll_config(uint32_t freq)
{
    uint32_t reg;

    system_dpll_frequency = freq;

    freq /= 1000000;

    reg  = REG_SYSTEM_0x40;
    reg |= (1 << 20);
    REG_SYSTEM_0x40 = reg;
    sys_delay_cycle(32);
    reg &= ~(1 << 27);
    REG_SYSTEM_0x40 = reg;
    sys_delay_cycle(32);

    reg  = REG_SYSTEM_0x47;
    reg &= ~(0x1FF << 12);
    reg &= ~((1 << 0)|(1 << 3));
    reg |= ((32 * freq / 52) << 12)|(1 << 1)|(1 << 2);
    REG_SYSTEM_0x47 = reg;
    sys_delay_cycle(32);

    reg = REG_SYSTEM_0x49;
    reg &= ~(0xF << 19);
    if((freq >= 210) && (freq < 250))
    {
        reg |= (1 << 19);
    }
    else if((freq >= 250) && (freq < 290))
    {
        reg |= (2 << 19);
    }
    else if((freq >= 290) && (freq < 340))
    {
        reg |= (3 << 19);
    }
    else if((freq >= 340) && (freq < 370))
    {
        reg |= (4 << 19);
    }
    else if((freq >= 370) && (freq < 410))
    {
        reg |= (5 << 19);
    }
    else if((freq >= 410) && (freq < 440))
    {
        reg |= (6 << 19);
    }
    else if((freq >= 440) && (freq <= 470))
    {
        reg |= (7 << 19);
    }
    REG_SYSTEM_0x49 = reg;
    sys_delay_cycle(32);

    reg  = REG_SYSTEM_0x4C;
    reg &= ~0x3FF;
    reg |= (freq / 2)|(0x7 << 15);
    REG_SYSTEM_0x4C = reg;
    sys_delay_cycle(32);

    reg  = REG_SYSTEM_0x47;
    reg &= ~(1 << 24);
    reg |= (1 << 23);
    REG_SYSTEM_0x47 = reg;
    sys_delay_us(1);
    REG_SYSTEM_0x47 &= ~(1 << 21);      //spi_rstn
    sys_delay_us(1);
    REG_SYSTEM_0x47 |= (1 << 21);
    sys_delay_us(1);
    REG_SYSTEM_0x47 &= ~(1 << 11);      //cal_trigger
    sys_delay_us(1);
    REG_SYSTEM_0x47 |= (1 << 11);
    sys_delay_us(1);
    REG_SYSTEM_0x47 &= ~(1 << 11);      //cal_trigger
    sys_delay_us(1);
    REG_SYSTEM_0x47 |= (1 << 11);
    sys_delay_us(1);

}

uint32_t system_dpll_freq(void)
{
    return system_dpll_frequency;
}

void system_dpll_toggle(void)
{
    uint32_t reg;
    uint32_t freq = system_dpll_freq()/1000000;

    reg  = REG_SYSTEM_0x47;
    reg &= ~((1 << 0)|(1 << 3));
    reg |= (1 << 1)|(1 << 2);
    REG_SYSTEM_0x47 = reg;
    sys_delay_cycle(32);

    reg = REG_SYSTEM_0x49;
    reg &= ~(0xF << 19);
    if((freq >= 210) && (freq < 250))
    {
        reg |= (1 << 19);
    }
    else if((freq >= 250) && (freq < 290))
    {
        reg |= (2 << 19);
    }
    else if((freq >= 290) && (freq < 340))
    {
        reg |= (3 << 19);
    }
    else if((freq >= 340) && (freq < 370))
    {
        reg |= (4 << 19);
    }
    else if((freq >= 370) && (freq < 410))
    {
        reg |= (5 << 19);
    }
    else if((freq >= 410) && (freq < 440))
    {
        reg |= (6 << 19);
    }
    else if((freq >= 440) && (freq <= 470))
    {
        reg |= (7 << 19);
    }
    REG_SYSTEM_0x49 = reg;
    sys_delay_cycle(32);

    reg  = REG_SYSTEM_0x4C;
    reg |= (0x7 << 15);
    REG_SYSTEM_0x4C = reg;
    sys_delay_cycle(32);

    reg  = REG_SYSTEM_0x47;
    reg &= ~(1 << 24);
    reg |= (1 << 23);
    REG_SYSTEM_0x47 = reg;
    sys_delay_us(1);
    REG_SYSTEM_0x47 &= ~(1 << 11);      //cal_trigger
    sys_delay_us(1);
    REG_SYSTEM_0x47 |= (1 << 11);
    sys_delay_us(1);
    REG_SYSTEM_0x47 &= ~(1 << 11);      //cal_trigger
    sys_delay_us(1);
    REG_SYSTEM_0x47 |= (1 << 11);
    sys_delay_us(1);
}

volatile uint8_t dpll_checked;
volatile uint8_t dpll_result;
volatile uint32_t dpll_gpio10_int_cnt;

uint8_t dpll_pwron_check(void)
{
    uint16_t dpll_check_timeout = 1000;
    dpll_checked = 0;
    dpll_result = 0;
    dpll_gpio10_int_cnt = 0;

    /* set dpll clock gpio10 output */
    REG_GPIO_0x0A = 0x0C;
    REG_SYSTEM_0x47 |= (1 << 10);
    sys_delay_cycle(32);
    REG_SYSTEM_0x4E &= ~(3 << 20);
    sys_delay_cycle(32);
    REG_SYSTEM_0x4C |= (1 << 31);
    sys_delay_cycle(32);
    REG_SYSTEM_0x49 &= ~(1 << 17);
    sys_delay_cycle(32);

    /* enable gpio10 interrupt */
    REG_GPIO_0x33 &= ~(1 << 10);
    REG_GPIO_0x35 |= (1 << 10); // clear status
    REG_GPIO_0x30 &= ~(3 << (10*2));   // Low level detect
    sys_delay_us(4);
    REG_GPIO_0x30 |= (2 << (10*2));   // rising edge
    REG_GPIO_0x33 |= (1 << 10);  // int enable

    /* wait for dpll gpio_isr */
    while((dpll_check_timeout --) && (!dpll_result))
    {
        sys_delay_us(1);
    }

    dpll_checked = 1;
    REG_GPIO_0x33 &= ~(1 << 10);
    REG_GPIO_0x0A = 0x08;
    REG_SYSTEM_0x47 &= ~(1 << 10);
    sys_delay_cycle(32);;
    REG_SYSTEM_0x4C &= ~(1 << 31);
    sys_delay_cycle(32);

    return dpll_result;

}

void dpll_restart(void)
{
    uint32_t reg;
    reg  = REG_SYSTEM_0x40;
    reg |= (1 << 27);
    REG_SYSTEM_0x40 = reg;
    sys_delay_us(1);
    reg &= ~(1 << 27);
    REG_SYSTEM_0x40 = reg;
    sys_delay_us(1);
    system_dpll_toggle();
}

void system_upll_config(void)
{
    uint32_t reg = REG_SYSTEM_0x48;

    reg |= (1 << 18) | (1 << 31);

    REG_SYSTEM_0x48 = reg;
    sys_delay_cycle(100);
    REG_SYSTEM_0x48 = reg & ~(1 << 18);
    sys_delay_cycle(100);
    REG_SYSTEM_0x48 = reg;
}

void system_abuck_enable(uint32_t enable)
{
    if(enable)
    {
#if 1
        REG_SYSTEM_0x42 &= ~(1 << 17);
        sys_delay_cycle(32);
        REG_SYSTEM_0x42 &= ~(1 << 14);
        sys_delay_cycle(32);
#else
        REG_SYSTEM_0x5A |= (1 << 1);
        sys_delay_cycle(6);
        REG_SYSTEM_0x42 &= ~(1 << 14);
        sys_delay_cycle(32);
        REG_SYSTEM_0x42 &= ~(1 << 18);
        sys_delay_cycle(64);
        REG_SYSTEM_0x42 |=  (1 << 18);
        sys_delay_us(100);
        REG_SYSTEM_0x5A &= ~(1 << 1);
#endif
    }
    else
    {
        REG_SYSTEM_0x42 |= (1 << 14);
    }
}

void system_dbuck_enable(uint32_t enable)
{
    if(enable)
    {
#if 1
        REG_SYSTEM_0x42 &= ~(1 << 17);
        sys_delay_cycle(32);
        REG_SYSTEM_0x46 &= ~(1 << 30);
        sys_delay_cycle(32);
#else
        REG_SYSTEM_0x5A |= (1 << 0);
        sys_delay_cycle(6);
        REG_SYSTEM_0x46 &= ~(1 << 30);
        REG_SYSTEM_0x42 &= ~(1 << 18);
        sys_delay_cycle(32);
        REG_SYSTEM_0x42 |=  (1 << 18);
        sys_delay_us(100);
        REG_SYSTEM_0x5A &= ~(1 << 0);
#endif
    }
    else
    {
        REG_SYSTEM_0x46 |= (1 << 30);
    }
}

void system_peri_clk_enable(SYS_PERI_CLK peries)
{
    REG_SYSTEM_0x02 &= ~(uint32_t)(peries >> 32);
    REG_SYSTEM_0x05 &= ~(uint32_t)(peries & 0xFFFFFFFF);
}

void system_peri_clk_disable(SYS_PERI_CLK peries)
{
    REG_SYSTEM_0x02 |= (uint32_t)(peries >> 32);
    REG_SYSTEM_0x05 |= (uint32_t)(peries & 0xFFFFFFFF);
}

void system_peri_clk_gating_enable(SYS_PERI_CLK_GATING peries)
{
    REG_SYSTEM_0x03 &= ~(uint32_t)(peries >> 32);
    REG_SYSTEM_0x06 &= ~(uint32_t)(peries & 0xFFFFFFFF);
}

void system_peri_clk_gating_disable(SYS_PERI_CLK_GATING peries)
{
    REG_SYSTEM_0x03 |= (uint32_t)(peries >> 32);
    REG_SYSTEM_0x06 |= (uint32_t)(peries & 0xFFFFFFFF);
}

void system_peri_mcu_irq_enable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x09 |= (uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x0A |= (uint32_t)(mask >> 32);
}

void system_peri_mcu_irq_disable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x09 &= ~(uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x0A &= ~(uint32_t)(mask >> 32);
}

void system_peri_dsp_irq_enable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x11 |= (uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x12 |= (uint32_t)(mask >> 32);
}

void system_peri_dsp_irq_disable(SYS_PERI_IRQ mask)
{
    REG_SYSTEM_0x11 &= ~(uint32_t)(mask & 0xFFFFFFFF);
    REG_SYSTEM_0x12 &= ~(uint32_t)(mask >> 32);
}

void system_gpio_peri_config(uint32_t idx, uint32_t mode)
{
    uint32_t reg;

    if(idx < 16)
    {
        idx *= 2;
        reg  = REG_SYSTEM_0x18;
        reg &= ~(3 << idx);
        reg |= (mode << idx);
        REG_SYSTEM_0x18 = reg;
    }
    else if(idx < 32)
    {
    	idx -= 16;
        idx *= 2;
        reg  = REG_SYSTEM_0x19;
        reg &= ~(3 << idx);
        reg |= (mode << idx);
        REG_SYSTEM_0x19 = reg;
    }
    else if(idx < 40)
    {
    	idx -= 32;
        idx *= 2;
        reg  = REG_SYSTEM_0x1A;
        reg &= ~(3 << idx);
        reg |= (mode << idx);
        REG_SYSTEM_0x1A = reg;
    }
}

void system_mem_clk_enable(SYS_MEM_CLK mems)
{
    REG_SYSTEM_0x21 &= ~(uint32_t)(mems >> 32);
    REG_SYSTEM_0x20 &= ~(uint32_t)(mems & 0xFFFFFFFF);
}

void system_mem_clk_disable(SYS_MEM_CLK mems)
{
    REG_SYSTEM_0x21 |= (uint32_t)(mems >> 32);
    REG_SYSTEM_0x20 |= (uint32_t)(mems & 0xFFFFFFFF);
}

void system_sleepmode_opt_select(void)
{
    REG_PMU_0x03 &= ~MSK_PMU_0x03_PWR_CTRL_SEL;
    REG_PMU_0x03 = (0x8d << SFT_PMU_0x03_PWR_CTRL_SEL); // 0x8d,optimalizing;
        
}

void system_lpo_clk_select(uint32_t sel)
{
    REG_PMU_0x00 &= ~(MSK_PMU_0x00_LPO_CLK_SEL);
    REG_PMU_0x00 |= (sel << SFT_PMU_0x00_LPO_CLK_SEL);
}

void system_wdt_reset_ana_dig(uint8_t ana,uint8_t dig)
{
    REG_PMU_0x00 |= MSK_PMU_0x00_MCHK_BPS;      // memory check bypass;
    REG_PMU_0x00 &= ~(MSK_PMU_0x00_WDT_ANA_PMT | MSK_PMU_0x00_WDT_DIG_PMT | MSK_PMU_0x00_WDT_SPI_PMT);
    REG_PMU_0x00 |= ((ana << SFT_PMU_0x00_WDT_ANA_PMT) | (dig << SFT_PMU_0x00_WDT_DIG_PMT) );
}

void system_mem_deep_sleep(void)
{
    //0x20[13]  ba22_dctag_ds    
    //0x20[15]  ba22_dcdat_ds    
    //0x20[17]  ba22_ictag_ds    
    //0x20[19]  ba22_icdat_ds    
    //0x20[22]  ba22_dram_ds     
    //0x20[23]  ba22_dram_ds 
}

void system_mem_light_sleep_bypass(void)
{
    //0x20[10]  ahbmem_ls_bps  
    //0x20[11]  ahbmem_ls_bps  
    //0x20[12]  ba22_dctag_ls_bps
    //0x20[14]  ba22_dcdat_ls_bps
    //0x20[16]  ba22_ictag_ls_bps
    //0x20[18]  ba22_icdat_ls_bps
    //0x20[20]  ba22_dram_ls_bps 
    //0x20[21]  ba22_dram_ls_bps 
}

int32_t system_ctrl(uint32_t cmd, uint32_t arg)
{
    uint32_t reg;
    int32_t  res = 0;

    switch(cmd)
    {
    case SYS_CTRL_CMD_AHB_CLK_DIV2:
        reg  = REG_SYSTEM_0x00;
        reg &= ~MSK_SYSTEM_0x00_AHB_DIV2;
        reg |= (arg & MAX_SYSTEM_0x00_AHB_DIV2) << SFT_SYSTEM_0x00_AHB_DIV2;
        REG_SYSTEM_0x00 = reg;
        break;
    case SYS_CTRL_CMD_CPU_CLK_DIV2:
        reg  = REG_SYSTEM_0x00;
        reg &= ~MSK_SYSTEM_0x00_CPU_DIV2;
        reg |= (arg & MAX_SYSTEM_0x00_CPU_DIV2) << SFT_SYSTEM_0x00_CPU_DIV2;
        REG_SYSTEM_0x00 = reg;
        system_cpu_frequency /= 2;
        sys_delay_calib_val  = system_cpu_frequency / 1000000;
        sys_delay_calib_val  = sys_delay_calib_val ? sys_delay_calib_val : 1;
        break;
    case SYS_CTRL_CMD_DSP_CLK_DIV2:
        reg  = REG_SYSTEM_0x00;
        reg &= ~MSK_SYSTEM_0x00_DSP_DIV2;
        reg |= (arg & MAX_SYSTEM_0x00_DSP_DIV2) << SFT_SYSTEM_0x00_DSP_DIV2;
        REG_SYSTEM_0x00 = reg;
        system_dsp_frequency /= 2;
        break;
    case SYS_CTRL_CMD_PWM0_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_PWMS0_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_PWMS0_SEL) << SFT_SYSTEM_0x01_PWMS0_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_PWM1_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_PWMS1_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_PWMS1_SEL) << SFT_SYSTEM_0x01_PWMS1_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_PWM2_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_PWMS2_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_PWMS2_SEL) << SFT_SYSTEM_0x01_PWMS2_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_PWM3_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_PWMS3_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_PWMS3_SEL) << SFT_SYSTEM_0x01_PWMS3_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_TIM1_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_TIM1_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_TIM1_SEL) << SFT_SYSTEM_0x01_TIM1_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_SADC_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_SADC_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_SADC_SEL) << SFT_SYSTEM_0x01_SADC_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_QSPI_CLK_SEL:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_QSPI_SEL;
        reg |= (arg & MAX_SYSTEM_0x01_QSPI_SEL) << SFT_SYSTEM_0x01_QSPI_SEL;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_SDIO_CLK_DIV:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_SDIO_DIV;
        reg |= (arg & MAX_SYSTEM_0x01_SDIO_DIV) << SFT_SYSTEM_0x01_SDIO_DIV;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_WDTS_CLK_DIV:
        reg  = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_WDTS_DIV;
        reg |= (arg & MAX_SYSTEM_0x01_WDTS_DIV) << SFT_SYSTEM_0x01_WDTS_DIV;
        REG_SYSTEM_0x01 = reg;
        break;
    case SYS_CTRL_CMD_GOTO_SLEEP:
        REG_SYSTEM_0x02 = !!arg;
        break;
    case SYS_CTRL_CMD_CPU_HALT:
        system_cpu_halt();
        break;
    case SYS_CTRL_CMD_DSP_HALT:
        REG_SYSTEM_0x10 = !!arg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_MUX:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_GPIO_DBGMUX;
        reg |= (arg & MAX_SYSTEM_0x1A_GPIO_DBGMUX) << SFT_SYSTEM_0x1A_GPIO_DBGMUX;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX0:
        reg  = REG_SYSTEM_0x28;
        reg &= ~MSK_SYSTEM_0x28_GPIO_DIAG_MUX0;
        reg |= (arg & MAX_SYSTEM_0x28_GPIO_DIAG_MUX0) << SFT_SYSTEM_0x28_GPIO_DIAG_MUX0;
        REG_SYSTEM_0x28 = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX1:
        reg  = REG_SYSTEM_0x28;
        reg &= ~MSK_SYSTEM_0x28_GPIO_DIAG_MUX1;
        reg |= (arg & MAX_SYSTEM_0x28_GPIO_DIAG_MUX1) << SFT_SYSTEM_0x28_GPIO_DIAG_MUX1;
        REG_SYSTEM_0x28 = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX2:
        reg  = REG_SYSTEM_0x28;
        reg &= ~MSK_SYSTEM_0x28_GPIO_DIAG_MUX2;
        reg |= (arg & MAX_SYSTEM_0x28_GPIO_DIAG_MUX2) << SFT_SYSTEM_0x28_GPIO_DIAG_MUX2;
        REG_SYSTEM_0x28 = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX3:
        reg  = REG_SYSTEM_0x28;
        reg &= ~MSK_SYSTEM_0x28_GPIO_DIAG_MUX3;
        reg |= (arg & MAX_SYSTEM_0x28_GPIO_DIAG_MUX3) << SFT_SYSTEM_0x28_GPIO_DIAG_MUX3;
        REG_SYSTEM_0x28 = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX4:
        reg  = REG_SYSTEM_0x28;
        reg &= ~MSK_SYSTEM_0x28_GPIO_DIAG_MUX4;
        reg |= (arg & MAX_SYSTEM_0x28_GPIO_DIAG_MUX4) << SFT_SYSTEM_0x28_GPIO_DIAG_MUX4;
        REG_SYSTEM_0x28 = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX5:
        reg  = REG_SYSTEM_0x29;
        reg &= ~MSK_SYSTEM_0x29_GPIO_DIAG_MUX5;
        reg |= (arg & MAX_SYSTEM_0x29_GPIO_DIAG_MUX5) << SFT_SYSTEM_0x29_GPIO_DIAG_MUX5;
        REG_SYSTEM_0x29 = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_DATA_GET:
        *(uint32_t*)arg = REG_SYSTEM_0x33;
        break;
    case SYS_CTRL_CMD_UART2_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_UART2_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_UART2_POS) << SFT_SYSTEM_0x1A_UART2_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_SPI3_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_SPI3_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_SPI3_POS) << SFT_SYSTEM_0x1A_SPI3_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_SDIO_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_SDIO_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_SDIO_POS) << SFT_SYSTEM_0x1A_SDIO_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_PWM6_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_PWM6_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_PWM6_POS) << SFT_SYSTEM_0x1A_PWM6_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_PWM7_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_PWM7_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_PWM7_POS) << SFT_SYSTEM_0x1A_PWM7_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_PWM8_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_PWM8_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_PWM8_POS) << SFT_SYSTEM_0x1A_PWM8_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_PWM9_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1A;
        reg &= ~MSK_SYSTEM_0x1A_PWM9_POS;
        reg |= (arg & MAX_SYSTEM_0x1A_PWM9_POS) << SFT_SYSTEM_0x1A_PWM9_POS;
        REG_SYSTEM_0x1A = reg;
        break;
    case SYS_CTRL_CMD_SPDIF_GPIO_MAPPING:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_SPDIF_POS;
        reg |= (arg & MAX_SYSTEM_0x1D_SPDIF_POS) << SFT_SYSTEM_0x1D_SPDIF_POS;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_MODE_EN:
        REG_SYSTEM_0x1B = arg;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_MSG_READ:
        *(uint32_t*)arg = REG_SYSTEM_0x1C;
        break;
    case SYS_CTRL_CMD_GPIO_DEBUG_MSG_WRITE:
        REG_SYSTEM_0x1C = arg;
        break;
    case SYS_CTRL_CMD_TOUCH_EN:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MAX_SYSTEM_0x1D_TOUCH_EN;
        reg |= (arg & MAX_SYSTEM_0x1D_TOUCH_EN) << SFT_SYSTEM_0x1D_TOUCH_EN;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_QSPI_OUT_PIN_MAPPING:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_QSPI_OUT_SEL;
        reg |= (arg & MAX_SYSTEM_0x1D_QSPI_OUT_SEL) << SFT_SYSTEM_0x1D_QSPI_OUT_SEL;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_JTAG_SEL:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_JTAG_SEL;
        reg |= (arg & MAX_SYSTEM_0x1D_JTAG_SEL) << SFT_SYSTEM_0x1D_JTAG_SEL;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_RW_CLK_BYPASS:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_RWCLK_BPS;
        reg |= (arg & MAX_SYSTEM_0x1D_RWCLK_BPS) << SFT_SYSTEM_0x1D_RWCLK_BPS;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_RW_ISO_EN:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_RW_ISO_EN;
        reg |= (arg & MAX_SYSTEM_0x1D_RW_ISO_EN) << SFT_SYSTEM_0x1D_RW_ISO_EN;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_BK24_EN:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_BK24_ENABLE;
        reg |= (arg & MAX_SYSTEM_0x1D_BK24_ENABLE) << SFT_SYSTEM_0x1D_BK24_ENABLE;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_SPI_FLASH_SEL:
        reg  = REG_SYSTEM_0x1D;
        reg &= ~MSK_SYSTEM_0x1D_SPI_FLA_SEL;
        reg |= (arg & MAX_SYSTEM_0x1D_SPI_FLA_SEL) << SFT_SYSTEM_0x1D_SPI_FLA_SEL;
        REG_SYSTEM_0x1D = reg;
        break;
    case SYS_CTRL_CMD_DEVICE_ID_GET:
        *(uint32_t*)arg = REG_SYSTEM_0x30;
        break;
    case SYS_CTRL_CMD_ANA_DETECT0_GET:
        *(uint32_t*)arg = REG_SYSTEM_0x31;
        break;
    case SYS_CTRL_CMD_ANA_DETECT1_GET:
        *(uint32_t*)arg = REG_SYSTEM_0x32;
        break;
    case SYS_CTRL_CMD_ANA_STATE2_GET:
        *(uint32_t*)arg = REG_SYSTEM_0x36;
        break;
    case SYS_CTRL_CMD_TOUCH_INT_SRC_GET:
        *(uint32_t*)arg = REG_SYSTEM_0x34;
        break;
    case SYS_CTRL_CMD_CPU_HALT_ACK_GET:
        *(uint32_t*)arg = (REG_SYSTEM_0x35 & MSK_SYSTEM_0x35_CPU_CLK_HALT) >> SFT_SYSTEM_0x35_CPU_CLK_HALT;
        break;
    case SYS_CTRL_CMD_DSP_HALT_ACK_GET:
        *(uint32_t*)arg = (REG_SYSTEM_0x35 & MSK_SYSTEM_0x35_DSP_HALT_ACK) >> SFT_SYSTEM_0x35_DSP_HALT_ACK;
        break;
    case SYS_CTRL_CMD_BOOT_SEL_GET:
        *(uint32_t*)arg = (REG_SYSTEM_0x35 & MSK_SYSTEM_0x35_BOOT_SEL) >> SFT_SYSTEM_0x35_BOOT_SEL;
        break;
    case SYS_CTRL_CMD_DSP_GP_OUT:
        *(uint32_t*)arg = REG_SYSTEM_0x37;
        break;
    case SYS_CTRL_AUDIO_CLK_OUTPUT:
        reg = REG_SYSTEM_0x01;
        reg &= ~MSK_SYSTEM_0x01_I2SMCLK_DIV;
        reg |= (arg & MAX_SYSTEM_0x01_I2SMCLK_DIV) << SFT_SYSTEM_0x01_I2SMCLK_DIV;
        reg &= ~MSK_SYSTEM_0x01_I2SMCLK_EN;
        reg |= (0x01 & MAX_SYSTEM_0x01_I2SMCLK_EN) << SFT_SYSTEM_0x01_I2SMCLK_EN;
        REG_SYSTEM_0x01 = reg;
        REG_SYSTEM_0x19 = 0x10000;
        break;
    default:
        res = -1;
        break;
    }

    return res;
}
