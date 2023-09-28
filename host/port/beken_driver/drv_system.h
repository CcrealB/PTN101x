/**
 **************************************************************************************
 * @file    drv_system.h
 * @brief   Driver API for SYSTEM
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_SYSTEM_H__
#define __DRV_SYSTEM_H__

#include <stdint.h>

/**
 * @brief System clock source definition
 */
typedef enum
{
    SYS_CLK_APLL = 0,
    SYS_CLK_XTAL = 1,
    SYS_CLK_DPLL = 2,
}SYS_CLK_SRC;

/**
 * @brief System audio PLL definition
 */
typedef enum
{
    SYS_APLL_98p3040_MHZ = 98304000,
    SYS_APLL_90p3168_MHZ = 90316800,
}SYS_APLL_FREQ;

/**
 * @brief System clock for peripherals definition
 */
typedef enum
{
    SYS_PERI_CLK_RWBT   = (1ULL << (32 + 8)),
    SYS_PERI_CLK_BK24   = (1ULL << (32 + 7)),
    SYS_PERI_CLK_USB1   = (1ULL << (32 + 6)),
    SYS_PERI_CLK_USB0   = (1ULL << (32 + 5)),
    SYS_PERI_CLK_QSPI   = (1ULL << (32 + 4)),
    SYS_PERI_CLK_DMA    = (1ULL << (32 + 1)),
    SYS_PERI_CLK_FLSPLL = (1ULL << (32 + 0)),
    SYS_PERI_CLK_DLP    = (1ULL << 26),
    SYS_PERI_CLK_PWMS3  = (1ULL << 25),
    SYS_PERI_CLK_PWMS2  = (1ULL << 24),
    SYS_PERI_CLK_ANC    = (1ULL << 23),
    SYS_PERI_CLK_PWMS1  = (1ULL << 22),
    SYS_PERI_CLK_CEC    = (1ULL << 21),
    SYS_PERI_CLK_I2S2   = (1ULL << 20),
    SYS_PERI_CLK_I2S1   = (1ULL << 19),
    SYS_PERI_CLK_EFUSE  = (1ULL << 17),
    SYS_PERI_CLK_SPDIF  = (1ULL << 16),
    SYS_PERI_CLK_CEVA   = (1ULL << 15),
    SYS_PERI_CLK_SDIO   = (1ULL << 14),
    SYS_PERI_CLK_SADC   = (1ULL << 13),
    SYS_PERI_CLK_IRDA   = (1ULL << 12),
    SYS_PERI_CLK_PWMS0  = (1ULL << 11),
    SYS_PERI_CLK_I2S0   = (1ULL << 10),
    SYS_PERI_CLK_TIM1   = (1ULL <<  9),
    SYS_PERI_CLK_TIM0   = (1ULL <<  8),
    SYS_PERI_CLK_I2C1   = (1ULL <<  7),
    SYS_PERI_CLK_I2C0   = (1ULL <<  6),
    SYS_PERI_CLK_SPI2   = (1ULL <<  5),
    SYS_PERI_CLK_SPI1   = (1ULL <<  4),
    SYS_PERI_CLK_SPI0   = (1ULL <<  3),
    SYS_PERI_CLK_UART2  = (1ULL <<  2),
    SYS_PERI_CLK_UART1  = (1ULL <<  1),
    SYS_PERI_CLK_UART0  = (1ULL <<  0),
}SYS_PERI_CLK;

/**
 * @brief System clock gating for peripherals definition
 */
typedef enum
{
    SYS_PERI_CLK_GATING_BK24   = (1ULL << (32 + 10)),
    SYS_PERI_CLK_GATING_DCACHE = (1ULL << (32 +  9)),
    SYS_PERI_CLK_GATING_RWBT   = (1ULL << (32 +  8)),
    SYS_PERI_CLK_GATING_USB1   = (1ULL << (32 +  7)),
    SYS_PERI_CLK_GATING_USB0   = (1ULL << (32 +  6)),
    SYS_PERI_CLK_GATING_QSPI   = (1ULL << (32 +  5)),
    SYS_PERI_CLK_GATING_SBC    = (1ULL << (32 +  4)),
    SYS_PERI_CLK_GATING_FFT    = (1ULL << (32 +  3)),
    SYS_PERI_CLK_GATING_GDMA   = (1ULL << (32 +  2)),
    SYS_PERI_CLK_GATING_FLASH  = (1ULL << (32 +  1)),
    SYS_PERI_CLK_GATING_SYS    = (1ULL << (32 +  0)),
    SYS_PERI_CLK_GATING_TRNG   = (1ULL << 25),
    SYS_PERI_CLK_GATING_CEC    = (1ULL << 24),
    SYS_PERI_CLK_GATING_I2S2   = (1ULL << 23),
    SYS_PERI_CLK_GATING_I2S1   = (1ULL << 22),
    SYS_PERI_CLK_GATING_ANC    = (1ULL << 21),
    SYS_PERI_CLK_GATING_XVR    = (1ULL << 20),
    SYS_PERI_CLK_GATING_CEVA   = (1ULL << 19),
    SYS_PERI_CLK_GATING_WDT    = (1ULL << 18),
    SYS_PERI_CLK_GATING_EFUSE  = (1ULL << 17),
    SYS_PERI_CLK_GATING_SPDIF  = (1ULL << 16),
    SYS_PERI_CLK_GATING_SDIO   = (1ULL << 15),
    SYS_PERI_CLK_GATING_SADC   = (1ULL << 14),
    SYS_PERI_CLK_GATING_IRDA   = (1ULL << 13),
    SYS_PERI_CLK_GATING_PWMS   = (1ULL << 12),
    SYS_PERI_CLK_GATING_I2S0   = (1ULL << 11),
    SYS_PERI_CLK_GATING_TIM1   = (1ULL << 10),
    SYS_PERI_CLK_GATING_TIM0   = (1ULL <<  9),
    SYS_PERI_CLK_GATING_I2C1   = (1ULL <<  8),
    SYS_PERI_CLK_GATING_I2C0   = (1ULL <<  7),
    SYS_PERI_CLK_GATING_SPI2   = (1ULL <<  6),
    SYS_PERI_CLK_GATING_SPI1   = (1ULL <<  5),
    SYS_PERI_CLK_GATING_SPI0   = (1ULL <<  4),
    SYS_PERI_CLK_GATING_UART2  = (1ULL <<  3),
    SYS_PERI_CLK_GATING_UART1  = (1ULL <<  2),
    SYS_PERI_CLK_GATING_UART0  = (1ULL <<  1),
    SYS_PERI_CLK_GATING_GPIO   = (1ULL <<  0),
}SYS_PERI_CLK_GATING;

/**
 * @brief System peripherals IRQ definition
 */
typedef enum
{
    SYS_PERI_IRQ_USBPLUG     = (1ULL << 0 ),
    SYS_PERI_IRQ_RTC         = (1ULL << 1 ),
    SYS_PERI_IRQ_ANC         = (1ULL << 2 ),
    SYS_PERI_IRQ_CEVA        = (1ULL << 3 ),
    SYS_PERI_IRQ_CEC         = (1ULL << 4 ),
    SYS_PERI_IRQ_I2S2        = (1ULL << 5 ),
    SYS_PERI_IRQ_I2S1        = (1ULL << 6 ),
    SYS_PERI_IRQ_SPDIF       = (1ULL << 7 ),
    SYS_PERI_IRQ_SDIO        = (1ULL << 8 ),
    SYS_PERI_IRQ_SADC        = (1ULL << 9 ),
    SYS_PERI_IRQ_IRDA        = (1ULL << 10),
    SYS_PERI_IRQ_PWM         = (1ULL << 11),
    SYS_PERI_IRQ_I2S0        = (1ULL << 12),
    SYS_PERI_IRQ_TIMER1      = (1ULL << 13),
    SYS_PERI_IRQ_TIMER0      = (1ULL << 14),
    SYS_PERI_IRQ_I2C1        = (1ULL << 15),
    SYS_PERI_IRQ_I2C0        = (1ULL << 16),
    SYS_PERI_IRQ_SPI2        = (1ULL << 17),
    SYS_PERI_IRQ_SPI1        = (1ULL << 18),
    SYS_PERI_IRQ_SPI0        = (1ULL << 19),
    SYS_PERI_IRQ_UART2       = (1ULL << 20),
    SYS_PERI_IRQ_UART1       = (1ULL << 21),
    SYS_PERI_IRQ_UART0       = (1ULL << 22),
    SYS_PERI_IRQ_GPIO        = (1ULL << 23),
    SYS_PERI_IRQ_RWBT0       = (1ULL << 24),
    SYS_PERI_IRQ_RWBT1       = (1ULL << 25),
    SYS_PERI_IRQ_RWBT2       = (1ULL << 26),
    SYS_PERI_IRQ_BK24        = (1ULL << 27),
    SYS_PERI_IRQ_USB1        = (1ULL << 28),
    SYS_PERI_IRQ_USB0        = (1ULL << 29),
    SYS_PERI_IRQ_QSPI        = (1ULL << 30),
    SYS_PERI_IRQ_SBC         = (1ULL << 31),
    SYS_PERI_IRQ_FFT         = (1ULL << 32),
    SYS_PERI_IRQ_GENER_DMA   = (1ULL << 33),
    SYS_PERI_IRQ_MBX_DSP2CPU = (1ULL << 34),
    SYS_PERI_IRQ_TOUCH       = (1ULL << 35),
    SYS_PERI_IRQ_DLP         = (1ULL << 36),
}SYS_PERI_IRQ;

/**
 * @brief System peripherals IRQ definition
 */
typedef enum
{
    SYS_MEM_CLK_FFT   = (1ULL <<  0),
    SYS_MEM_CLK_SBC   = (1ULL <<  1),
    SYS_MEM_CLK_QSPI  = (1ULL <<  2),
    SYS_MEM_CLK_USB0  = (1ULL <<  3),
    SYS_MEM_CLK_USB1  = (1ULL <<  4),
    SYS_MEM_CLK_BK24  = (1ULL <<  5),
    SYS_MEM_CLK_FLS   = (1ULL <<  7),
    SYS_MEM_CLK_AHB0  = (1ULL <<  8),
    SYS_MEM_CLK_AHB1  = (1ULL <<  9),
    SYS_MEM_CLK_DRAM  = (1ULL << 24), 
    SYS_MEM_CLK_RWBT0 = (1ULL << 25),
    SYS_MEM_CLK_RWBT1 = (1ULL << 26),
    SYS_MEM_CLK_UART0 = (1ULL << (32 +  0)),
    SYS_MEM_CLK_UART1 = (1ULL << (32 +  1)),
    SYS_MEM_CLK_UART2 = (1ULL << (32 +  2)),
    SYS_MEM_CLK_SPI0  = (1ULL << (32 +  3)),
    SYS_MEM_CLK_SPI1  = (1ULL << (32 +  4)),
    SYS_MEM_CLK_SPI2  = (1ULL << (32 +  5)),
    SYS_MEM_CLK_I2S0  = (1ULL << (32 +  6)),
    SYS_MEM_CLK_SDIO  = (1ULL << (32 +  7)),
    SYS_MEM_CLK_CEVA  = (1ULL << (32 +  8)),
    SYS_MEM_CLK_SPDIF = (1ULL << (32 +  9)),
    SYS_MEM_CLK_I2S1  = (1ULL << (32 + 10)),
    SYS_MEM_CLK_I2S2  = (1ULL << (32 + 11)),
    SYS_MEM_CLK_CEC   = (1ULL << (32 + 12)),
    SYS_MEM_CLK_ANC   = (1ULL << (32 + 13)),
    SYS_MEM_CLK_DLP   = (1ULL << (32 + 14)),
}SYS_MEM_CLK;

/**
 * @brief System control command definition
 */
typedef enum
{
    SYS_CTRL_CMD_NULL = 0,
    SYS_CTRL_CMD_AHB_CLK_DIV2,          //0:no,   1:yes
    SYS_CTRL_CMD_CPU_CLK_DIV2,          //0:no,   1:yes
    SYS_CTRL_CMD_DSP_CLK_DIV2,          //0:no,   1:yes
    SYS_CTRL_CMD_PWM0_CLK_SEL,          //0:32KD, 1:XTAL
    SYS_CTRL_CMD_PWM1_CLK_SEL,          //0:32KD, 1:XTAL
    SYS_CTRL_CMD_PWM2_CLK_SEL,          //0:32KD, 1:XTAL
    SYS_CTRL_CMD_PWM3_CLK_SEL,          //0:32KD, 1:XTAL
    SYS_CTRL_CMD_TIM1_CLK_SEL,          //0:32KD, 1:ROSC
    SYS_CTRL_CMD_SADC_CLK_SEL,          //0:XTAL, 1:ROSC
    SYS_CTRL_CMD_QSPI_CLK_SEL,          //0:ROSC, 1:XTAL, 2: DPLL
    SYS_CTRL_CMD_SDIO_CLK_DIV,          //CLK=XTAL, 0:div1, 1:div2, 2:div2, 3:div128
    SYS_CTRL_CMD_WDTS_CLK_DIV,          //CLK=XTAL, 0:div16, 1:div8, 2:div4, 3:div2
    SYS_CTRL_CMD_GOTO_SLEEP,
    SYS_CTRL_CMD_CPU_HALT,
    SYS_CTRL_CMD_DSP_HALT,
    SYS_CTRL_CMD_GPIO_DEBUG_MUX,        //0:DBG_MSG(REG0x26), 1:DIAG_MODEM, 2:DIAG_MAC, 3:DIAG_BT, 4:DIAG_DSP
    SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX0,  //0~63 //GPIO2
    SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX1,  //0~63 //GPIO3
    SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX2,  //0~63 //GPIO6
    SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX3,  //0~63 //GPIO7
    SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX4,  //0~63 //GPIO8
    SYS_CTRL_CMD_GPIO_DEBUG_DIAG_MUX5,  //0~63 //GPIO9
    SYS_CTRL_CMD_GPIO_DEBUG_DATA_GET,
    SYS_CTRL_CMD_UART2_GPIO_MAPPING,    //0:GPIO6~7, 1:GPIO16~17
    SYS_CTRL_CMD_SPI3_GPIO_MAPPING,     //0:GPIO30~33, 1:GPIO36~39
    SYS_CTRL_CMD_SDIO_GPIO_MAPPING,     //0:GPIO8~10+34~36, 1:GPIO34~39
    SYS_CTRL_CMD_PWM6_GPIO_MAPPING,     //0:GPIO18, 1:GPIO26, 2:GPIO30
    SYS_CTRL_CMD_PWM7_GPIO_MAPPING,     //0:GPIO19, 1:GPIO27, 2:GPIO31
    SYS_CTRL_CMD_PWM8_GPIO_MAPPING,     //0:GPIO20, 1:GPIO28, 2:GPIO32
    SYS_CTRL_CMD_PWM9_GPIO_MAPPING,     //0:GPIO21, 1:GPIO29, 2:GPIO33
    SYS_CTRL_CMD_SPDIF_GPIO_MAPPING,    //0:GPIO11, 1:GPIO12, 2:GPIO13
    SYS_CTRL_CMD_GPIO_DEBUG_MODE_EN,    //0:DEBUG mode, 1:Perial mode
    SYS_CTRL_CMD_GPIO_DEBUG_MSG_READ,
    SYS_CTRL_CMD_GPIO_DEBUG_MSG_WRITE,
    SYS_CTRL_CMD_TOUCH_EN,              //0:disable, 1:enable
    SYS_CTRL_CMD_QSPI_OUT_PIN_MAPPING,  //0:GPIO, 1:separate pin
    SYS_CTRL_CMD_JTAG_SEL,              //0:CPU, 1:DSP
    SYS_CTRL_CMD_RW_CLK_BYPASS,         //0:no, 1:yes
    SYS_CTRL_CMD_RW_ISO_EN,             //0:no, 1:yes
    SYS_CTRL_CMD_BK24_EN,               //0:no, 1:yes
    SYS_CTRL_CMD_SPI_FLASH_SEL,         //0:flash ctrl, 1:spi ctrl
    SYS_CTRL_CMD_DEVICE_ID_GET,
    SYS_CTRL_CMD_ANA_DETECT0_GET,
    SYS_CTRL_CMD_ANA_DETECT1_GET,
    SYS_CTRL_CMD_ANA_STATE2_GET,
    SYS_CTRL_CMD_TOUCH_INT_SRC_GET,
    SYS_CTRL_CMD_CPU_HALT_ACK_GET,
    SYS_CTRL_CMD_DSP_HALT_ACK_GET,
    SYS_CTRL_CMD_BOOT_SEL_GET,          //0:CPU, 1:DSP
    SYS_CTRL_CMD_DSP_GP_OUT,
    SYS_CTRL_AUDIO_CLK_OUTPUT,
}SYS_CTRL_CMD;
typedef enum
{
	debug_src_ceva_test0 = 48,
	debug_src_ceva_test1,
	debug_src_xvr_tx_bit,
	debug_src_xvr_rx_bit,
	debug_src_xvr_ref_clk,
	debug_src_xvr_slot_ctrl,
	debug_src_xvr_radio_on,
	debug_src_xvr_trx_mode,
	debug_src_edr_sync_find,
	debug_src_xvr_sync_find,
	debug_src_bb_restn,
	debug_src_xvr_dual_mode,
	debug_src_if_2m_sel,
	debug_src_gck_ena,
	debug_src_pll_tuning,
	debug_src_pll_unlock,

}debug_src;

/**
 * @brief System True Random Generator Enable
 */
void sys_trng_init(void); 

/**
 * @brief System True Random Generator Disable
 */
void sys_trng_uninit(void); 

/**
 * @brief System True Random Generator Random Data Get
 */
uint32_t sys_trng_random_get(void);

/**
 * @brief System delay for n cycles
 * @param n delay cycles
 */
void sys_delay_cycle(uint32_t n);

/**
 * @brief System delay for us in micro-second
 * @param us delay time in micro-second
 */
void sys_delay_us(uint32_t us);

/**
 * @brief System delay for ms in milli-second
 * @param ms delay time in milli-second
 */
void sys_delay_ms(uint32_t ms);

void system_freq_update(void);

/**
 * @brief System core clock setting
 * @param sel clock source, @see SYS_CLK_SRC
 * @param div clock divider, value range from 0 to 127, clk = clk / (div ? div : 1).
 */
void system_core_clk_set(SYS_CLK_SRC sel, uint32_t div);

/**
 * @brief Core frequency get
 * @return frequency of core clock in Hz
 */
uint32_t system_core_freq(void);

/**
 * @brief CPU frequency get
 * @return frequency of CPU clock in Hz
 */
uint32_t system_cpu_freq(void);

/**
 * @brief CPU halt operation
 */
void system_cpu_halt(void);

/**
 * @brief DSP frequency get
 * @return frequency of DSP clock in Hz
 */
uint32_t system_dsp_freq(void);

/**
 * @brief System analog registers initialization
 */
void system_analog_regs_init(void);

/**
 * @brief System audio PLL configuration
 * @param freq frequency in Hz, @see SYS_APLL_FREQ
 *
 * @note  for sample rate 8/12/16/24/32/48/64/96/128/192/256/384, set freq to 98.304MHz
 *        for sample rate 11.025/22.05/44.1/88.2/176.4/352.8, set freq to 90.3168MHz
 */
 
void system_apll_toggle(void);
void system_apll_config(uint32_t freq);

/**
 * @brief System audio PLL current frequency get
 * @return current frequency of audio PLL
 */
uint32_t system_apll_freq(void);

/**
 * @brief System audio PLL output to GPIO pin setting
 * @param enable output enable
 * @param div    output divider, 0: div1, 1: div2, 2: div4, 3: div8, 4-7: div16
 */
void system_apll_output(uint32_t enable, uint32_t div);

/**
 * @brief System digital PLL configuration
 * @param freq frequency in Hz
 */
void system_dpll_config(uint32_t freq);

/**
 * @brief System digital current frequency get
 * @return current frequency of digital PLL
 */
uint32_t system_dpll_freq(void);

void system_dpll_toggle(void);
uint8_t dpll_pwron_check(void);
void dpll_restart(void);

/**
 * @brief System USB PLL configuration
 * @note  USB PLL output frequency @ 48MHz
 */
void system_upll_config(void);

/**
 * @brief System analog BUCK enable
 * @param enable 0:disable, 1:enable
 */
void system_abuck_enable(uint32_t enable);

/**
 * @brief System digital BUCK enable
 * @param enable 0:disable, 1:enable
 */
void system_dbuck_enable(uint32_t enable);

/**
 * @brief System peripheral clock enable
 * @param peries peripheral, @see SYS_PERI_CLK
 */
void system_peri_clk_enable(SYS_PERI_CLK peries);

/**
 * @brief System peripheral clock disable
 * @param peries peripheral, @see SYS_PERI_CLK
 */
void system_peri_clk_disable(SYS_PERI_CLK peries);

/**
 * @brief System peripheral clock gating enable
 * @param peries peripheral, @see SYS_PERI_CLK_GATING
 */
void system_peri_clk_gating_enable(SYS_PERI_CLK_GATING peries);

/**
 * @brief System peripheral clock gating disable
 * @param peries peripheral, @see SYS_PERI_CLK_GATING
 */
void system_peri_clk_gating_disable(SYS_PERI_CLK_GATING peries);

/**
 * @brief System peripheral interrupt enable for MCU
 * @param peries peripheral interrupt mask, @see SYS_PERI_IRQ
 */
void system_peri_mcu_irq_enable(SYS_PERI_IRQ peries);

/**
 * @brief System peripheral interrupt disable for MCU
 * @param mask peripheral interrupt mask, @see SYS_PERI_IRQ
 */
void system_peri_mcu_irq_disable(SYS_PERI_IRQ peries);

/**
 * @brief System peripheral interrupt enable for DSP
 * @param mask peripheral interrupt mask, @see SYS_PERI_IRQ
 */
void system_peri_dsp_irq_enable(SYS_PERI_IRQ peries);

/**
 * @brief System peripheral interrupt disable for DSP
 * @param mask peripheral interrupt mask, @see SYS_PERI_IRQ
 */
void system_peri_dsp_irq_disable(SYS_PERI_IRQ peries);

/**
 * @brief System GPIO mapping to peripheral (function) mode configuration
 * @param idx  GPIO index, range from 0 to 39, @see GPIO_PIN
 * @param mode peripheral (function) mode, range from 0 to 3
 */
void system_gpio_peri_config(uint32_t idx, uint32_t mode);

/**
 * @brief System memory clock enable
 * @param mems system memory masks, @see SYS_MEM_CLK
 */
void system_mem_clk_enable(SYS_MEM_CLK mems);

/**
 * @brief System memory clock disable
 * @param mems system memory masks, @see SYS_MEM_CLK
 */
void system_mem_clk_disable(SYS_MEM_CLK mems);

/**
 * @brief System control
 * @param cmd control command, @see SYS_CTRL_CMD
 * @param arg control argument
 * @return control result, 0:SUCCESS, others:FAILURE
 */
int32_t system_ctrl(uint32_t cmd, uint32_t arg);

void system_sleepmode_opt_select(void);
void system_lpo_clk_select(uint32_t sel);
void system_wdt_reset_ana_dig(uint8_t ana,uint8_t dig);
void system_set_0x4d_reg(uint32_t reg_val);
uint8_t system_get_0x4d_reg(void);

#endif//__DRV_SYSTEM_H__
