#ifndef _DRIVER_ICU_H_
#define _DRIVER_ICU_H_

#include "config.h"
#include "sys_types.h"

#define CLEAR_BIT(reg,count,...) clear_bit(&(reg),count,##__VA_ARGS__)
#define SET_BIT(reg,count,...) set_bit(&(reg),count,##__VA_ARGS__)

typedef enum
{
    POWERDOWN_SELECT               = 0,
    POWERDOWN_CHG_DEEPSLEEP        = 1,
    POWERDOWN_DEEPSLEEP_WITH_RTC   = 2,
    POWERDOWN_DEEPSLEEP_WITH_GPIO  = 3,
    POWERDOWN_SHUTDOWN             = 4
}t_powerdown_mode;

#define LPO_CLK_SRC_X32K         0
#define LPO_CLK_SRC_ROSC         2
#define LPO_CLK_SRC_XTAL_DIV     3

#define LPO_CLK_SEL              LPO_CLK_SRC_XTAL_DIV

#define CPU_DPLL_CLK             CPU_CLK	//yuan++

#define CPU_CLK_APLL             0     /* 0:APLL,1:XTAL*/
#define CPU_CLK_DPLL             2     /* 2:DPLL,3:XTAL*/
#if (CONFIG_ENC_ENABLE == 1) || (CONFIG_ANC_ENABLE == 1)
#define CPU_CLK_SEL              CPU_CLK_DPLL //CPU_CLK_APLL     /* 2:DPLL,1:XTAL*/
#else
#define CPU_CLK_SEL              CPU_CLK_DPLL //CPU_CLK_APLL     /* 2:DPLL,1:XTAL*/
#endif
#if(CPU_CLK_SEL == CPU_CLK_DPLL)
#define CPU_CLK_XTAL             3     /* 3:XTAL*/
#elif (CPU_CLK_SEL == CPU_CLK_APLL)
#define CPU_CLK_XTAL             1     /* 1:XTAL*/
#else
#define CPU_CLK_XTAL             1     /* 1:XTAL*/
#endif


#if CONFIG_CPU_CLK_OPTIMIZATION == 1
#define CPU_CLK_DIV              1
#else
#define CPU_CLK_DIV              1
#define CPU_CLK_DIV_OF_SCO       1
#if (CONFIG_PRE_EQ == 1)
    #define CPU_EQ_CLK_SEL           CPU_CLK_SEL
    #define CPU_EQ_CLK_DIV           CPU_CLK_DIV
#endif
#endif

#if (CPU_CLK_SEL == CPU_CLK_DPLL)
#define CPU_OPT_CLK_SEL          CPU_CLK_DPLL    /* 2:DPLL,3:XTAL*/
#define CPU_OPT_CLK_DIV          4               /* DPLL/8 */
#else
#define CPU_OPT_CLK_SEL          CPU_CLK_APLL    /* 2:DPLL,3:XTAL*/
#define CPU_OPT_CLK_DIV          3               /* DPLL/8 */
#endif

#define CPU_SLEEP_CLK_SEL        CPU_CLK_XTAL   /* 2:DPLL,3:XTAL*/
#define CPU_SLEEP_CLK_DIV        1              /* XTAL/1 */

#if CONFIG_ANC_ENABLE
#define CPU_ANC_CLK_SEL          CPU_CLK_DPLL//CPU_CLK_APLL
#define CPU_ANC_CLK_DIV          1
#endif

#if (CONFIG_ENC_ENABLE == 1)
#define CPU_ENC_CLK_SEL     CPU_CLK_DPLL//CPU_CLK_APLL
#define CPU_ENC_CLK_DIV     1
#endif

RAM_CODE void BK3000_Ana_Decrease_Current(uint8_t enable);
RAM_CODE void SYSpwr_Prepare_For_Sleep_Mode(void);
RAM_CODE void SYSpwr_Wakeup_From_Sleep_Mode(void);
DRAM_CODE void BK3000_set_clock (int clock_sel, int div);
void BK3000_set_AON_voltage(uint8_t aon);
void BK3000_ICU_Initial(void);
void BK3000_setting_w4_reset(void);
void BK3000_icu_sw_powerdown(uint8_t  wakup_pin,t_powerdown_mode pwrdown_mode);
void BK3000_start_wdt(uint32_t val);
void BK3000_wdt_power_on(void);
void BK3000_stop_wdt(void);
void BK3000_wdt_reset(void);

void watch_dog_start(uint16_t ms);
void watch_dog_clear(void);
void watch_dog_stop(void);

void BK3000_set_ana_dig_voltage(uint8_t ana,uint8_t dig);
void BK3000_set_ana_voltage(uint8_t ana);
void BK3000_set_dig_voltage(uint8_t dig);
int  BK3000_wdt_flag( void );
void BK3000_Ana_Line_enable( uint8_t enable );
//void BK3000_Ana_Dac_clk_adjust( int mode );
void VICMR_usb_chief_intr_enable(void);
void VICMR_usb_chief_intr_disable(void);
void _audio_isr_dispatch(void);
void ba22_disable_intr_exception(void);
void ba22_enable_intr_exception(void);
#if (CONFIG_CPU_CLK_OPTIMIZATION == 0)
DRAM_CODE void System_Config_MCU_For_eSCO(void);
DRAM_CODE void System_Config_MCU_Restore(void);
#endif

void enable_audio_ldo(void);
void clear_sco_connection(void);
void clear_music_play(void);
//void ldo_enable(void);
//void ldo_disable(void);
void enable_audio_ldo_for_music_files(void);
void clear_wave_playing(void);
DRAM_CODE unsigned char BK3000_hfp_set_powercontrol(void);
#ifdef	WROK_AROUND_DCACHE_BUG
void app_Dcache_disable(void);
void app_Dcache_enable(void);
void app_Dcache_initial(void);
#endif

#ifdef CONFIG_PRODUCT_TEST_INF
extern uint8_t aver_rssi;
extern int16_t aver_offset;
inline void get_freqoffset_rssi(void);
void average_freqoffset_rssi(void);
#endif

void bt_ext_wakeup_generate(void);
#if (BT_DUALMODE_RW == 1)
void rw_ext_wakeup_generate(void);
#endif

#endif
