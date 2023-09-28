#include "bkreg.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "timer.h"
void disable_audio_ldo(void);

#if A2DP_ROLE_SOURCE_CODE
extern uint8_t get_a2dp_role(void);
extern uint32_t getA2dpSrcRate(void);
#endif

__inline void clear_bit(volatile uint32_t *p,uint32_t count,...)
{
    uint32_t val;
    uint8_t i = 0;
    int bit_no;
    va_list args;

    va_start(args, count);

    val = 0;
    for(;i < count;i++)
    {
        bit_no = va_arg(args,int);
        val |= (1 << bit_no);
    }

    (*p) &= (~(val));

    va_end(args);
}

__inline void set_bit(volatile uint32_t *p,uint32_t count,...)
{
    uint32_t val;
    uint8_t i = 0;
    int bit_no;
    va_list args;

    va_start(args, count);

    val = 0;
    for(;i < count;i++)
    {
        bit_no = va_arg(args,int);
        val |= (1 << bit_no);
    }

    (*p) |= val;

    va_end(args);
}

#if 0
void ldo_enable(void)
{
	BK3000_A3_CONFIG |= sft_GPIO_LDO_en;
}

void ldo_disable(void)
{
	BK3000_A3_CONFIG &= ~sft_GPIO_LDO_en;
}
#endif

static volatile uint32_t  b_26m_clock_closed     = FALSE;
static volatile uint32_t backup_pmu_peri_pwds_02 = 0XFFFFFFFF;
static volatile uint32_t backup_pmu_peri_pwds_05 = 0XFFFFFFFF;
static volatile uint32_t backup_ana_reg_40   = 0;
static volatile uint32_t backup_ana_reg_41   = 0;
static volatile uint32_t backup_ana_reg_42   = 0;
static volatile uint32_t backup_ana_reg_44   = 0;
static volatile uint32_t backup_ana_reg_46   = 0;
static volatile uint32_t backup_ana_reg_4c   = 0;
static volatile uint32_t backup_ana_reg_4e   = 0;
static volatile uint32_t backup_sys_mem_reg0x20 = 0;
static volatile uint32_t backup_sys_mem_reg0x21 = 0;

RAM_CODE void BK3000_Ana_Decrease_Current(uint8_t enable)
{
	//TODO
}

RAM_CODE void SYSpwr_Prepare_For_Sleep_Mode(void)
{
    //u_int32 cpu_flags, mask;
    u_int32 reg;
    if(b_26m_clock_closed) return;

    disable_timer0_pt0();
    //SYSirq_Disable_Interrupts_Save_Flags(&cpu_flags, &mask);
    //uart_gpio_disable();
    #if (CONFIG_UART_IN_SNIFF == 1)
	
    REG_UART0_CONF=0;
    REG_UART0_FIFO_CONF=0;
    REG_UART0_INT_ENABLE=0;
    REG_GPIO_0x35 |= (1 << 1);    
    REG_GPIO_0x30 &= (3 << 2);   
    REG_GPIO_0x33 |= (1 << 1);    
    REG_GPIO_0x01  = 0x3c;      // Uart Rx set 'input & pull up'
	#if(CONFIG_DRIVER_UART1 == 1)
	REG_GPIO_0x11  = 0x3C;      // Uart1 Rx set 'input & pull up'
	#endif
    #endif
    REG_GPIO_0x00 = 0x02;
    CLEAR_SLEEP_TICK;

    /* Firstly,backup some registers */
    backup_pmu_peri_pwds_02    = REG_SYSTEM_0x02;
    backup_pmu_peri_pwds_05    = REG_SYSTEM_0x05;
    backup_sys_mem_reg0x20     = REG_SYSTEM_0x20;
    backup_sys_mem_reg0x21     = REG_SYSTEM_0x21;
    backup_ana_reg_40          = REG_SYSTEM_0x40;
    backup_ana_reg_41          = REG_SYSTEM_0x41;
    backup_ana_reg_42          = REG_SYSTEM_0x42;
    backup_ana_reg_44          = REG_SYSTEM_0x44;
    backup_ana_reg_46          = REG_SYSTEM_0x46;
    backup_ana_reg_4c          = REG_SYSTEM_0x4C;
    backup_ana_reg_4e          = REG_SYSTEM_0x4E;

    reg = REG_PMU_0x11;
    reg &= ~(0x7f << 0);
    reg &= ~(0x7f << 7);
    reg |= ((0x01<<0) + (0x01<< 7));
    REG_PMU_0x11 = reg;

    /* Step 1: close BT xvr */
    //REG_SYSTEM_0x40 &= ~(1<<25);

    /* Step 2: close audio */
    #if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_MCU
    audio_dac_enable(0);
    #endif
    /* Step 3: set cpu clk = xtal 26M */
    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV);

    /* Step 4: set flash parameters */
    set_flash_clk(FLASH_CLK_26mHz);
    //flash_set_line_mode(FLASH_LINE_2);

    /* Step 5: Close APLL,DPLL,UPLL,XTAL BUF... */
    reg = REG_SYSTEM_0x40;
    /*
        bit30: XTAL ana BUF
        bit28: APLL
        bit27: DPLL
        bit18: random gen pwr down
        bit15: LCD PWRDOWN
        bit13: Touch pwrdown
        bit00: USB PLL
    */    
#if !CONFIG_AUDIO_DAC_ALWAYSOPEN
    reg |= ((1<<30) | (1<<28) | (1<<27) | (1<<18) | (1<<15) | (1<<13) | (1<<0));
#else
    reg |= ((1<<30) | (1<<27) | (1<<18) | (1<<15) | (1<<13) | (1<<0)); //DAC need APLL
#endif
    REG_SYSTEM_0x40 = reg;

    /* Step 6: ABB CB disable */
    sys_delay_cycle(6);
#if !CONFIG_AUDIO_DAC_ALWAYSOPEN
    REG_SYSTEM_0x40 &= ~(1<<20);  // ABB CB
#endif

    REG_SYSTEM_0x41 |= (1<<8);
    REG_SYSTEM_0x41 |= (1<<11);
    REG_SYSTEM_0x41 |= (1<<23);
    REG_SYSTEM_0x41 |= (1<<21);
    REG_SYSTEM_0x42 |= (1<<12);
    REG_SYSTEM_0x42 |= (1<<3);
    /* Step 7: Setting LDO and Disable buffer of 26M xtal */

    REG_SYSTEM_0x42 &= ~(1<<13); // ldo low current
    if(!(backup_ana_reg_46 & (1<<12)))  // if not auto lower voltage by hardware
    {
        reg = REG_SYSTEM_0x44;
        reg &= ~(0x7<<3);
        reg |= (0x04<<3);// dig core ldo = 0.75;
        REG_SYSTEM_0x44 = reg;
        sys_delay_cycle(6);
        /*
        REG_SYSTEM_0x44 &= ~(0x7<<0);// AON ldo = 0.65;
        sys_delay_cycle(6);
        REG_SYSTEM_0x44 &= ~(0x7<<3);// dig ldo = 0.65;
        sys_delay_cycle(6);
        */
    }
    REG_SYSTEM_0x44 &= ~(1<<31); // ldo low voltage 0.8V
    //REG_SYSTEM_0x46 |= (1<<12);  // core ldo 0.4~0.7
    REG_SYSTEM_0x4C &= ~(1<<18); // force the XTAL26M on
    //REG_SYSTEM_0x4E |= (1<<26);  // rc32k on
    sys_delay_cycle(6);
    REG_SYSTEM_0x4E &= ~(1<<31); // disable the high pssr mode in buff of 26M

    /* Step 8: power down peripheral clock */
    //REG_SYSTEM_0x02  = 0xffffffff;
    //REG_SYSTEM_0x05  = 0xffffffff;

    /* Step 9: power down memory,Warning: reg21.bit13(ANC) dont't shutdown */
    REG_SYSTEM_0x20 |= 0x3ff;  // 
    REG_SYSTEM_0x21 |= 0x5EFF; // 0x1fff:bit13(ANC) don't Shutdown;
    /* Step 10: Select PMU sleep mode*/
    reg = REG_PMU_0x02;
    reg &= ~(3<<16);
    reg |= (1<<16); // 00|11:normal mode,01:sleep mode,10:deepsleep mode;
    reg |= ((1<<19) | (1<<20)); // wakeup enable,bit19:CEVA;bit18:RW;bit20:GPIO
    reg |= (1<<0);  // for GPIO wakeup;
#if (BT_DUALMODE_RW == 1)
    reg |= (1<<18); // rwbt sleep wakeup enable
#else
    reg |= (1<<22); // rw sleep bypass enable, never wakeup
#endif
    REG_PMU_0x02 = reg;
    /* Step 11: Power option select */
    /* Warning: Flash don't power down */
    REG_PMU_0x03 = (0x81 << 11);
    
    /* Step 12: for DSP */
    //REG_SYSTEM_0x15 |= 1; // power down dsp
    REG_SYSTEM_0x10 |= MSK_SYSTEM_0x10_DSP_HALT; // close dsp clk

    /* Step 13: for mcu */    
    //REG_SYSTEM_0x0D |= 1;
    b_26m_clock_closed = TRUE;

    //SYSirq_Interrupts_Restore_Flags(cpu_flags, mask);
}            
RAM_CODE void SYSpwr_Wakeup_From_Sleep_Mode(void)
{
    return;
}
void SYSpwr_Wakeup_From_Sleep(void)
{

    if(!b_26m_clock_closed) 
        return;

    b_26m_clock_closed = FALSE;

    /* Step 1: Select PMU normal mode */
    //REG_SYSTEM_0x10 &= ~MSK_SYSTEM_0x10_DSP_HALT;
    REG_PMU_0x02 &= ~(3<<16);  // normal mode
    //REG_GPIO_0x00 = 0x70;
    /* Step 2: ABB CB enable */
    //REG_SYSTEM_0x40 |= (1<<20); // ABB CB

    /* Step 3: Open BT XVR */
    //REG_SYSTEM_0x40 |= (1<<25) ;  

    /* Step 4: Open APLL,DPLL,XTAL BUF...*/
    //reg = REG_SYSTEM_0x40;
    /*
        bit30: XTAL ana BUF
        bit28: APLL
        bit27: DPLL
        bit18: random gen pwr down
        bit15: LCD PWRDOWN
        bit13: Touch pwrdown
        bit00: USB PLL
    */    
    //reg &= ~((1<<30) | (1<<28) | (1<<27));
    //REG_SYSTEM_0x40 = reg; /* ??? */

    /* Step 5: Restore ana register */
    REG_SYSTEM_0x40 = backup_ana_reg_40;
    //delay_us(40);
    REG_SYSTEM_0x41 = backup_ana_reg_41;
    REG_SYSTEM_0x42 = backup_ana_reg_42;
    REG_SYSTEM_0x44 = backup_ana_reg_44;
    REG_SYSTEM_0x46 = backup_ana_reg_46;
    REG_SYSTEM_0x4C = backup_ana_reg_4c;
    REG_SYSTEM_0x4E = backup_ana_reg_4e;

    /* Step 6: Restore AHB/UART/ANC/CEVA... memory power on */
    REG_SYSTEM_0x20 = backup_sys_mem_reg0x20;
    REG_SYSTEM_0x21 = backup_sys_mem_reg0x21;

    /* Step 7: Restore peripheral clk enable */
    REG_SYSTEM_0x02 = backup_pmu_peri_pwds_02;
    REG_SYSTEM_0x05 = backup_pmu_peri_pwds_05; 

    /* Step 8: cpu power on */
    //REG_SYSTEM_0x0D &= ~(1<<0);// 

    /* Step 9: for dsp */
    //REG_SYSTEM_0x15 &= ~(1<<0); // power on dsp
    //REG_SYSTEM_0x10 &= ~(1<<0); // open dsp clk
    

    /* Step 10: set cpu clk */
    #if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    BK3000_set_clock(CPU_OPT_CLK_SEL, CPU_OPT_CLK_DIV);				/* 26M xtal */
    #else
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
    #endif

    /* Step 11 set flash */
    //flash_config();
    /* Warning: QE enable is too long time */
    //flash_set_line_mode(DEFAULT_LINE_MODE);
    set_flash_clk(FLASH_CLK_SEL);

    /* Step 12 set uart */
    #if (CONFIG_UART_IN_SNIFF == 1)
	REG_GPIO_0x35 |= (1 << 1);
    REG_GPIO_0x33 &= ~(1 << 1);
    REG_UART0_INT_STATUS=0xffff;
	if(app_env_system_uart_dbg_enable())
    {
        uart_initialise(UART_BAUDRATE_115200);
    }
    #else   
    if(app_env_system_uart_dbg_enable())
    {
        uart_gpio_enable();
    }
	#if(CONFIG_DRIVER_UART1 == 1)
	uart1_gpio_enable();
	#endif
    #endif  
#if (CONFIG_EAR_IN == 1) && (CONFIG_DRIVER_PWM == 0)
    earin_generate_start_from_sleep_mode(1);
#endif
	enable_timer0_pt0();
}


/*
    REG46[14:16] ana voltage: 0-7 <=> 1.3v-1.9v;
    REG44[0:2,3:5] dig voltage  0-7 <=> 0.65v-1.0v;
    AON:[0-2],must be 0x07;
*/
void BK3000_set_ana_dig_voltage(uint8_t ana,uint8_t dig)
{
    uint32_t reg;
    if(ana < 2 ) return;
    reg = REG_SYSTEM_0x46;
    reg &= ~(0x07 << 14);
    reg |= ((ana & 0x07) << 14);
    REG_SYSTEM_0x46 = reg;
    //always_on_ldo_change(7);// always on ldo 1.0V
    if(dig < 3) return;
    reg = REG_SYSTEM_0x44;
    reg &= ~(7<<3);
    reg |= (dig &0x07) << 3;  
    REG_SYSTEM_0x44 = reg;
#if 0
    if(ana > 5) 
        REG_SYSTEM_0x56 &= ~(1<<21);
    else
        REG_SYSTEM_0x56 |= (1<<21);
#endif            
}

void BK3000_set_ana_voltage(uint8 ana)
{
#if 0//CONFIG_TWS_SYNC_PHASE_OPT == 0
    uint32 reg = REG_SYSTEM_0x46;
    reg &= ~(0x07 << 14);
    reg |= ((ana & 0x07) << 14);
    REG_SYSTEM_0x46 = reg;
#endif
#if 0
    if(ana > 5) 
        REG_SYSTEM_0x56 &= ~(1<<21);
    else
        REG_SYSTEM_0x56 |= (1<<21);
#endif
}

void BK3000_set_dig_voltage(uint8 dig)
{

    uint32 reg;
    //always_on_ldo_change(7);// always on ldo 1.0V
    if(dig < 3) return;
    reg = REG_SYSTEM_0x44;
    reg &= ~(7<<3);
    reg |= (dig &0x07) << 3;  
    REG_SYSTEM_0x44 = reg;
}
void BK3000_set_AON_voltage(uint8_t aon)
{
    uint32_t reg;
    if(aon < 3) return;
    reg = REG_SYSTEM_0x44;
    reg &= ~(7<<0);
    reg |= (aon &0x07) << 0;
    REG_SYSTEM_0x44 = reg;
}
void BK3000_start_wdt( uint32_t val )
{
    os_delay_ms(2);
    REG_WDT_0x00 = 0x5A0000|val;
    REG_WDT_0x00 = 0xA50000|val;
}

void BK3000_wdt_power_on(void)
{
    REG_WDT_0x00 = 0x5A0000|0x3fff;
    REG_WDT_0x00 = 0xA50000|0x3fff;
}

void BK3000_stop_wdt(void )
{
    os_delay_ms(2);
    REG_WDT_0x00 = 0x5A0000;
    REG_WDT_0x00 = 0xA50000;
}

void BK3000_wdt_reset(void )
{
    system_peri_mcu_irq_disable(0xffffffffffff);    
    os_delay_ms(2);
    REG_WDT_0x00 = 0x5A0001;
    REG_WDT_0x00 = 0xA50001;
}

//ms:1 ~ 32767, wdt reset time
void watch_dog_start(uint16_t ms)
{
    os_delay_ms(2);
    uint16_t val = ms << 1;
    REG_WDT_0x00 = 0x5A0000 | val;
    REG_WDT_0x00 = 0xA50000 | val;
}

void watch_dog_clear(void)//清除看门狗计数器
{
    uint16_t val = REG_WDT_0x00 & 0xFFFF;
    REG_WDT_0x00 = 0x5A0000 | val;
    REG_WDT_0x00 = 0xA50000 | val;
}

void watch_dog_stop(void)
{
    os_delay_ms(2);
    REG_WDT_0x00 = 0x5A0000;
    REG_WDT_0x00 = 0xA50000;
}

void app_set_powerdown_gpio(void)
{
    uint8 i=0;
    app_handle_t app_h = app_get_sys_handler();
    app_env_handle_t env_h = app_env_get_handle();

    if((env_h->env_cfg.system_para.system_flag&APP_ENV_SYS_FLAG_LED_REVERSE) >> 9)
    {
        for(i = 0; i < LED_NUM; i++)
    	{
        	if(app_h->led_map[i] == 0xFF) continue;
			gpio_config(app_h->led_map[i],1);
			gpio_output(app_h->led_map[i],1);
		}
	}
	if(app_env_get_pin_enable(PIN_paMute))
	{
		gpio_config(app_env_get_pin_num(PIN_paMute),1);
		gpio_output(app_env_get_pin_num(PIN_paMute),app_env_get_pin_valid_level(PIN_paMute));
	}
}

extern uint8_t app_get_auto_powerdown(void);
static void BK3000_icu_deepsleep(uint8_t wakup_pin,t_powerdown_mode pwrdown_mode)
{
    int_t i;
    uint32_t reg;
    
    //for(i=0;i<0x1f;i++)
    //{
    //    os_printf("0x%x:0x%x\r\n",0x40+i,(*(volatile unsigned long*)(MDU_SYSTEM_BASE_ADDR + (0x40+i) * 4)));
    //}
    //sys_delay_ms(100);

    set_spr( SPR_VICMR(0), 0);
    cpu_set_interrupts_enabled(0);
    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_SEL); // xtal

    REG_PMU_0x02 &= ~MSK_PMU_0x02_DEEP_WAKEN;
    if((pwrdown_mode == POWERDOWN_CHG_DEEPSLEEP))
    {
        /* Wakeup Enable:USB PLUGIN/CHARGE FULL/1|2|4V Ready */
        REG_PMU_0x02 |= ((1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6));
        REG_PMU_0x03 = (0x87<<11);//  top power down need be always on, or charge 32k power down
        SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP);    
    } 
    else
    {
       /* Wakeup Enable:GPIO/USB PLUGIN/CHARGE FULL/1|2|4V Ready */
        REG_PMU_0x02 |= (1<<0) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) ;
        REG_PMU_0x03 = (0x87<<11);
        UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP);
    }
    INFO_PRT("icu_deepsleep:%d,%8x,%8x\r\n",wakup_pin,REG_PMU_0x01,REG_PMU_0x02);
    Delay(100);
#if 0
    for( i = 0; i <= GPIO_NUM; i++)
    {
        gpio_config(i,5);  // High impedance
    }
    gpio_config(wakup_pin,3);
#endif
    for( i = 0; i < wakup_pin; i++)
    {
        gpio_config(i,5);  // High impedance
    }
    for( i = wakup_pin+1; i < GPIO_NUM; i++)
    {
        gpio_config(i,5);  // High impedance
    }
    if((SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode())&&app_env_get_pin_valid_level(PIN_ioDet))
    {
        gpio_config(wakup_pin,0);
    }
    else
    {
        gpio_config(wakup_pin,3);
    }
    app_set_powerdown_gpio();

    if(pwrdown_mode == POWERDOWN_CHG_DEEPSLEEP)
    {//charging LED ON
    //app_set_led_charging_onoff(LED_ON);
    }
    Delay(100);

    /* Wakeup pin setting */
    /*
    * lvl setting 00:low lvl,01:high lvl,10:rising edge,11:falling edge
    * GPIO:00-15,REG0x30
    * GPIO:16-31,REG0x31
    * GPIO:32-40,REG0x32

    * enable int setting: 0:disable,1:enable;
    * GPIO:00-31,REG0x33
    * GPIO:32-40,REG0x34
    */
    REG_GPIO_0x30 = 0x55555550;
    REG_GPIO_0x31 = 0x55555555;
    REG_GPIO_0x32 = 0x00005555;
    REG_GPIO_0x33 = 0;
    REG_GPIO_0x34 = 0;

    if(wakup_pin < 16)
        REG_GPIO_0x30 &=  ~(0x03 << (wakup_pin*2));
    else if(wakup_pin <32)
        REG_GPIO_0x31 &=  ~(0x03 << ((wakup_pin-16)*2));
    else
        REG_GPIO_0x32 &=  ~(0x03 << ((wakup_pin-32)*2));

    if((SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode())&&app_env_get_pin_valid_level(PIN_ioDet))
    {
        if(wakup_pin < 16)
            REG_GPIO_0x30 |=  (0x01 << (wakup_pin*2));
        else if(wakup_pin <32)
            REG_GPIO_0x31 |=  (0x01 << ((wakup_pin-16)*2));
        else
            REG_GPIO_0x32 |=  (0x01 << ((wakup_pin-32)*2));
    }
    
    uint64_t wakeup_pin_mask = (1UL << wakup_pin);
    REG_GPIO_0x34 |= (uint32_t)(wakeup_pin_mask >> 32);
    REG_GPIO_0x33 |= (uint32_t)(wakeup_pin_mask & 0xFFFFFFFF);

    /* Deepsleep mode,setting ioldo/ana/dig */
    BK3000_set_ana_dig_voltage(6,4);
    REG_SYSTEM_0x44 |= (1<<26);
    REG_SYSTEM_0x45 &= ~(0x07<<0);
    //REG_SYSTEM_0x5C |= 0x3f;
    
    REG_PMU_0x02 &= ~MSK_PMU_0x02_DPSLP_SLP_SEL;
    REG_PMU_0x02 |= 2<<SFT_PMU_0x02_DPSLP_SLP_SEL; // deepsleep mode

    reg = REG_PMU_0x11;
    reg &= ~(0x7f << 0);
    reg &= ~(0x7f << 7);
    reg |= ((2<<0) + (10 << 7));
    REG_PMU_0x11 = reg;

    REG_SYSTEM_0x10 |= MSK_SYSTEM_0x10_DSP_HALT;
    system_cpu_halt();
    while(1);
}

void BK3000_rtc_init(void)
{
    //TODO@liaixing
#if 0
    os_printf("BK3000_rtc_init()\n");
#define PERI_PWDS           (*((volatile unsigned long *)    0x00800004))

    // s1
    PERI_PWDS &= ~(1<<28); //32k_divd open

    // s2
    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x1;

    os_printf ("rtc test start\n");
    os_delay_ms(2);

    // s3 reset RTC model
    BK3000_GPIO_58_CONFIG = 0;
    BK3000_GPIO_58_CONFIG |=   (1<<28) ;
    BK3000_GPIO_58_CONFIG &= (~(1<<28));
    os_delay_ms(2);
    BK3000_GPIO_58_CONFIG |= (1<<27);//rtc_supply_en open to check rtc status
    os_delay_ms(2);
    os_printf ("rtc first time\n");

    //0x3a[30:29]	rtc_clk_sel
    //0x3a[28]		rtc_latch_en
    //0x3a[27]		rtc_supply_en
    //0x3a[26]		RESERVED
    //0x3a[25]		rtc_clk_en
    //0x3a[24]		rtc_mode
    //0x3a[23]		rtc_int_clr


    // 1. loop mode
    /*
    BK3000_GPIO_61_CONFIG = 125;

    BK3000_GPIO_58_CONFIG |= ((0x1<<29) + (0x1<<25) + (0x1<<24));//(1<<27)already 1
    BK3000_GPIO_58_CONFIG |=   (1<<28) ;
    BK3000_GPIO_58_CONFIG &= (~(1<<28));
    */

    // 2. one-time mode

    // s4 how long
    BK3000_GPIO_61_CONFIG = 125 * 105; // about 60 seconds.

    // s5 config
    BK3000_GPIO_58_CONFIG |= ((0x1<<29) + (0x1<<25) + (0x0<<24));//(1<<27)already 1

    // s6 trigger RTC
    BK3000_GPIO_58_CONFIG |=   (1<<28) ;
    BK3000_GPIO_58_CONFIG &= (~(1<<28));
#endif
}

void BK3000_deepsleep_with_rtc(uint8 wakup_pin,t_powerdown_mode pwrdown_mode)
{
    //TODO@liaixing
#if 0
    int i, jitter_low_cnt = 0, jitter_high_cnt = 0;

    app_env_handle_t  env_h = app_env_get_handle();

    uint8 high_flag = (env_h->env_cfg.system_para.system_flag & APP_ENV_SYS_FLAG_PWRCTRL_HIGH) ? 1:0 ;

    //uint8 wakup_pin = env_h->env_cfg.system_para.wakup_pin+ GPIO0;

    //t_powerdown_mode pwrdown_mode = POWERDOWN_CHG_DEEPSLEEP;

    os_printf("BK3000_deepsleep_with_rtc: %d, %d\n",  wakup_pin, high_flag);

    BK3000_rtc_init();

    Delay(1000);
    set_spr(SPR_VICMR(0), 0);

    Delay(10);
    cpu_set_interrupts_enabled(0);

    BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL)
                          | (0 << sft_PMU_CPU_CLK_DIV
                          /* | PMU_AHB_CLK_ALWAYS_ON */);

    set_flash_clk(FLASH_CLK_26mHz);

    flash_set_line_mode(FLASH_LINE_2);

    if (pwrdown_mode == POWERDOWN_SELECT)
    {
        if (app_charge_is_usb_plug_in())
        {
            bt_flag2_operate(APP_FLAG2_CHARGE_POWERDOWN,0);
            BK3000_start_wdt(0x1ff);
            while(1);
            //return;
        }
    }

    /* Step 1: close RF cb and 52M pll */
    REG_XVR_0x09 = 0x2BFFF304;
    /* Step 2: close ABB CB */
    BK3000_A0_CONFIG &= ~(1<<15);
    /* Step 3: close DCO */
    BK3000_A0_CONFIG |= (1<<26);
    /* Step 4: ana buck pfm mode */
    BK3000_A1_CONFIG &= ~(1<<31);

    /* if system working in LDO MODE */
    /* Step 5: dig buck ramp compensation disable */
    /* Step 6: ana buck ramp compensation disable */
    if (env_h->env_cfg.used == 0x01)
    {
            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE))
            {
                BK3000_A1_CONFIG &= ~(1<<15);
            }

            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE))
            {
                BK3000_A2_CONFIG &= ~(1<<7);
            }
    }
    else
    {
        #if(SYS_CFG_BUCK_ON == 0)    // LDO MODE
            BK3000_A1_CONFIG &= ~(1<<15);
            BK3000_A2_CONFIG &= ~(1<<7);
        #endif
    }

    Delay(100);

    for (i = 0; i <= 26; i++)
    {
        gpio_config(i,0);  // High impedance
    }

    if (high_flag)
    	gpio_config(wakup_pin, 0);
    else
    	gpio_config(wakup_pin, 3);

    /* the elimination of GPIO jitter */
    if (pwrdown_mode == POWERDOWN_SELECT)
    {
        while (app_get_auto_powerdown())
        {
            os_delay_ms(20);
            if (gpio_input(wakup_pin) == (high_flag ^ 0x01))
            {
                jitter_high_cnt = 0;
                jitter_low_cnt++;
                if(jitter_low_cnt > 3) // if gpio detect is anti-high_flag level more than 3 times,then deep sleep!
                    break;
            }
            else
            {
                jitter_low_cnt = 0;
                jitter_high_cnt++;
                if(jitter_high_cnt > 0x0f) // if GPIO is high_flag level for a long time,then reset mcu;
                {
                  	BK3000_start_wdt(0xff);
                    while(1);
                }
            }
        }
    }

    if (gpio_input(wakup_pin) == high_flag)
    {
        gpio_config(wakup_pin, 5);
        //if (wakup_pin == 22)
         //   gpio_config(7, 5);
    }

    app_set_powerdown_gpio();
    Delay(10);

    BK3000_GPIO_DEEP_SLEEP_LATCH = 0x1;
//    BK3000_GPIO_PAD_CTRL |= (0x1f<<3);
    BK3000_GPIO_PAD_CTRL &= (~(0x1F << 3));
    BK3000_GPIO_PAD_CTRL |= (0x1d << 3);

    /*
    * Bit 29: wakeup enable;
    * Bit xx: gpio pin(0:26)wakeup;
    * Bit 27: USB plug in wakeup;
    * Bit 28: RTC wakeup;
    */
    BK3000_GPIO_WKUPEN  = ((1<<29) | (1<<wakup_pin) | (1<<28));
    if (POWERDOWN_SELECT == pwrdown_mode)
        BK3000_GPIO_WKUPEN |= (1<<27);

    //BK3000_GPIO_WKUPEN  = 0;
    //BK3000_GPIO_WKUPEN  |= ((1<<29) | (1<<wakup_pin) | (1<<28));

    BK3000_GPIO_DPSLP  = 0x3261;

    while(1);
#endif
}
void BK3000_setting_w4_reset(void)
{
    set_spr( SPR_VICMR(0), 0);
    cpu_set_interrupts_enabled(0);

    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV); // xtal
    set_flash_clk(FLASH_CLK_26mHz);      //FLASH:26MHz 
    flash_set_line_mode(FLASH_LINE_2);
    SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_SOFT_RESET);
#if 0
    app_env_handle_t  env_h = app_env_get_handle();
    set_spr(SPR_VICMR(0), 0);
    Delay(10);
    cpu_set_interrupts_enabled(0);
    BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL)
                                      | (0 << sft_PMU_CPU_CLK_DIV
                          /* | PMU_AHB_CLK_ALWAYS_ON */);

    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    /* Step 1: close RF cb and 52M pll */
    REG_XVR_0x09 = 0x2BFFF304;
    /* Step 2: close ABB CB */
    BK3000_A0_CONFIG &= ~(1 << 15);
    /* Step 3: close DCO */
    BK3000_A0_CONFIG |= (1 << 26);
    /* Step 4: ana buck pfm mode */
    BK3000_A1_CONFIG &= ~(1 << 31);

    /* if system working in LDO MODE */
    /* Step 5: dig buck ramp compensation disable */
    /* Step 6: ana buck ramp compensation disable */
    if (env_h->env_cfg.used == 0x01)
    {
            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_DIG_BUCK_ENABLE))
            {
                BK3000_A1_CONFIG &= ~(1 << 15);
            }

            if (!app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_ANA_BUCK_ENABLE))
            {
                BK3000_A2_CONFIG &= ~(1 << 7);
            }
    }
    else
    {
        #if(SYS_CFG_BUCK_ON == 0)    // LDO MODE
            BK3000_A1_CONFIG &= ~(1 << 15);
            BK3000_A2_CONFIG &= ~(1 << 7);
        #endif
    }
    SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_SOFT_RESET);
#endif
}

void BK3000_deepsleep_with_gpio(uint8_t wakup_pin,t_powerdown_mode pwrdown_mode)
{
    int i = 0;
#if POWER_ON_OUT_CHARGE_ENABLE
    int jitter_low_cnt = 0, jitter_high_cnt = 0;
#endif
    //app_env_handle_t  env_h = app_env_get_handle();
    uint32_t reg;
    INFO_PRT("icu_deepsleep_with_gpio:%d,%x,%x\r\n",wakup_pin,REG_PMU_0x01,REG_PMU_0x02);

    set_spr( SPR_VICMR(0), 0);
    cpu_set_interrupts_enabled(0);
    //BK3000_PMU_CONFIG = (1 << sft_PMU_CPU_CLK_SEL) | (0 << sft_PMU_CPU_CLK_DIV /* | PMU_AHB_CLK_ALWAYS_ON */);
    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_DIV); // xtal

    REG_PMU_0x02 &= ~MSK_PMU_0x02_DEEP_WAKEN;
    /* Wakeup Enable:GPIO/USB PLUGIN/CHARGE FULL */
    REG_PMU_0x02 |= (1<<0) | (1<<2) | (1<<3);
    REG_PMU_0x03 = (0x87<<11);
    Delay(1000);
#if 0    
    for( i = 0; i <= GPIO_NUM; i++)
    {
        gpio_config(i,5);  // High impedance
    }
#endif
    for( i = 0; i < wakup_pin; i++)
    {
        gpio_config(i,5);  // High impedance
    }
    for( i = wakup_pin+1; i < GPIO_NUM; i++)
    {
        gpio_config(i,5);  // High impedance
    }
    gpio_config(wakup_pin,3);

#if POWER_ON_OUT_CHARGE_ENABLE
	if (app_env_check_auto_pwr_on_enable())
	{
		SET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO);
	#ifdef WAKEUP_GPIO_OUT_CHARGE
	    gpio_config(WAKEUP_GPIO_OUT_CHARGE, 4);
	#else
	    gpio_config(wakup_pin, 4);
	#endif
	    while (1)/* the elimination of GPIO jitter */
	    {
	        os_delay_ms(20);
			#ifdef WAKEUP_GPIO_OUT_CHARGE
	        if (gpio_input(WAKEUP_GPIO_OUT_CHARGE) == 1)
			#else
	        if (gpio_input(wakup_pin) == app_env_get_pin_valid_level(PIN_ioDet))
			#endif
	        {
	            jitter_low_cnt = 0;
	            jitter_high_cnt++;
	            if (jitter_high_cnt > 20) // if gpio detect is anti-high_flag level more than 3 times, means detect the falling edge,  then enter into deep sleep!
	            {                          
	                if(get_Charge_state() == BATTERY_CHARGE_FULL_POWER)
	                    break;

	                BK3000_wdt_reset();
	                while(1);
	            }
	        }
	        else
	        {
	            jitter_high_cnt = 0;
	            jitter_low_cnt++;
	            if (jitter_low_cnt > 20) // if GPIO is high_flag level for a long time, then reset mcu;
	            {
	                //os_printf("reset wdt,%d,%d\n", jitter_low_cnt, jitter_high_cnt);
	                //os_delay_ms(50); //for printf

	                BK3000_wdt_reset();
	                while(1);
	            }
	        }
	    }
	}
#endif

    app_set_powerdown_gpio();
#if defined(CONFIG_FORCE_DRIVER_HW) || defined(CONFIG_FORCE_DRIVER_SOFT)
	#if (FORCE_DEVICE_TYPE == DEVICE_DF230)
    force_sensor_poweron();
    #endif
#endif
    Delay(10);

 
    //gpio_config(wakup_pin,3);
    /* Wakeup pin setting */
    /*
    * lvl setting 00:low lvl,01:high lvl,10:rising edge,11:falling edge
    * GPIO:00-15,REG0x30
    * GPIO:16-31,REG0x31
    * GPIO:32-40,REG0x32

    * enable int setting: 0:disable,1:enable;
    * GPIO:00-31,REG0x33
    * GPIO:32-40,REG0x34
    */
    REG_GPIO_0x30 = 0x55555550;
    REG_GPIO_0x31 = 0x55555555;
    REG_GPIO_0x32 = 0x00005555;
    REG_GPIO_0x33 = 0;
    REG_GPIO_0x34 = 0;
    if(wakup_pin < 16)
        REG_GPIO_0x30 &=  ~(0x03 << (wakup_pin*2));
    else if(wakup_pin <32)
        REG_GPIO_0x31 &=  ~(0x03 << ((wakup_pin-16)*2));
    else
        REG_GPIO_0x33 &=  ~(0x03 << ((wakup_pin-32)*2));

    uint64_t wakeup_pin_mask = (1UL << wakup_pin);
    REG_GPIO_0x34 |= (uint32_t)(wakeup_pin_mask >> 32);
    REG_GPIO_0x33 |= (uint32_t)(wakeup_pin_mask & 0xFFFFFFFF);

    /* Deepsleep mode,setting ioldo */
    REG_SYSTEM_0x44 |= (1<<26);
    REG_SYSTEM_0x45 &= ~(0x07<<0);
    REG_SYSTEM_0x5C |= 0x3f;

    REG_PMU_0x02 &= ~MSK_PMU_0x02_DPSLP_SLP_SEL;
    REG_PMU_0x02 |= 2<<SFT_PMU_0x02_DPSLP_SLP_SEL; // deepsleep mode

    reg = REG_PMU_0x11;
    reg &= ~(0x7f << 0);
    reg &= ~(0x7f << 7);
    reg |= ((2<<0) + (10 << 7));
    REG_PMU_0x11 = reg;

    REG_SYSTEM_0x10 |= MSK_SYSTEM_0x10_DSP_HALT;
    system_cpu_halt();
    while(1);
}

void BK3000_icu_shutdown( uint8_t  wakup_pin)
{
    set_spr( SPR_VICMR(0), 0);
    cpu_set_interrupts_enabled(0);
    Delay(10);
    set_flash_clk(FLASH_CLK_26mHz);
    flash_set_line_mode(FLASH_LINE_2);
    BK3000_set_clock(CPU_SLEEP_CLK_SEL, CPU_SLEEP_CLK_SEL); // xtal

    // wakeup pin set
    REG_SYSTEM_0x46 &= ~0xfff;
    sys_delay_cycle(6);
#if (CONFIG_SHUTDOWN_WAKEUP_IO==0)
    REG_SYSTEM_0x46 |= ((wakup_pin<<6) | (wakup_pin << 0));
#else
	gpio_config(SECOND_WAKEUP_IO,3);
	REG_SYSTEM_0x46 |= ((wakup_pin<<6) | (SECOND_WAKEUP_IO << 0));
#endif
    Delay(1000); // for print msg
    REG_SYSTEM_0x40 |= (1<<21);
}

void BK3000_icu_sw_powerdown(uint8_t  wakup_pin,t_powerdown_mode pwrdown_mode)
{
    LOG_I(PWR,"powerdown mode:%d,wakup_pin:%d\r\n",pwrdown_mode,wakup_pin);

#if (CONFIG_EAR_IN == 1)
    app_ear_in_uninit(1);
#endif

    switch(pwrdown_mode)
    {
    case POWERDOWN_SELECT:
        if ((SW_PWR_KEY_SWITCH==app_env_check_pwrCtrl_mode()) && app_env_get_pin_valid_level(PIN_ioDet))
        { //wakeup_pin = PIN_ioDet
            BK3000_icu_deepsleep(wakup_pin,POWERDOWN_SELECT);
        }
        else
        { //wakeup_pin = PIN_pwrBtn
            BK3000_icu_shutdown(wakup_pin);
        }
        break;
    case POWERDOWN_CHG_DEEPSLEEP:
        BK3000_icu_deepsleep(wakup_pin,POWERDOWN_CHG_DEEPSLEEP);
        break;
    case POWERDOWN_DEEPSLEEP_WITH_RTC:
        BK3000_deepsleep_with_rtc(wakup_pin, POWERDOWN_DEEPSLEEP_WITH_RTC);
        break;

    case POWERDOWN_DEEPSLEEP_WITH_GPIO:
        //wakup_pin = PIN_ioDet
        BK3000_deepsleep_with_gpio(wakup_pin, POWERDOWN_DEEPSLEEP_WITH_GPIO);
        break;

    case POWERDOWN_SHUTDOWN:
        BK3000_icu_shutdown(wakup_pin);
        break;
    default:
        BK3000_icu_deepsleep(wakup_pin,POWERDOWN_SELECT);
    }
}

void open_linein2aux_loop(void)
{
    //os_printf("open_linein2aux_loopn");
    REG_ANC_0x06  |=  ( 1 << SFT_ANC_0x06_ADC_LOOP_DAC1);
    REG_ANC_0x06  |=  ( 1 << SFT_ANC_0x06_ADC_LOOP_DAC0);
}

void close_linein2aux_loop(void)
{
    //os_printf("close_linein2aux_loop\n");
    REG_ANC_0x06  &= ~  ( 1 << SFT_ANC_0x06_ADC_LOOP_DAC1);
    REG_ANC_0x06  &= ~  ( 1 << SFT_ANC_0x06_ADC_LOOP_DAC0);
}

void BK3000_Ana_Line_enable( uint8_t enable )
{

}


void enable_audio_ldo(void)
{
    //GPIO_Ax.a3->pwdAudDigLDO = 0;    // Enable the LDO at Microphone LineIn and audio condition
    //BK3000_A3_CONFIG &= (~(1 << 0));
}

void enable_audio_ldo_for_music_files(void)
{
    //GPIO_Ax.a3->pwdAudDigLDO = 1;       // Disable the LDO at Microphone LineIn and audio condition for mp3&wav
    //BK3000_A3_CONFIG |= (1 << 0);
}

void clear_sco_connection(void)
{
    disable_audio_ldo();
}
void clear_wave_playing(void)
{
    app_bt_flag1_set(APP_FLAG_WAVE_PLAYING, 0);

    disable_audio_ldo();
}
void clear_music_play(void)
{
    app_bt_flag1_set(APP_FLAG_MUSIC_PLAY, 0);

    disable_audio_ldo();
}

void clear_line_in(void)
{
    app_bt_flag1_set(APP_FLAG_LINEIN, 0);

    disable_audio_ldo();
}

void disable_audio_ldo(void)
{
    return;
/*
    if(0 == (app_bt_flag1_get(APP_FLAG_LINEIN
                                | APP_FLAG_MUSIC_PLAY
                                | APP_FLAG_WAVE_PLAYING
                                | APP_FLAG_SCO_CONNECTION)
#if (CONFIG_APP_MP3PLAYER == 1)
             || player_get_play_status()
#endif
    	))
    {
        os_printf("disable_audio_ldo\r\n");
        //GPIO_Ax.a3->pwdAudDigLDO = 1;       // Enable the LDO at Microphone LineIn and audio condition
        //BK3000_A3_CONFIG |= (1 << 0);       // Enable the LDO at Microphone LineIn and audio condition
    }
*/
}

DRAM_CODE void BK3000_set_clock (int clock_sel, int div)
{
    uint32 reg;
    #if (CPU_CLK_SEL == CPU_CLK_DPLL)
    /*avoid clk changed by func pointer 'ctrl_sleep_cbs.bt_set_cpu_clk'(called in lib)
    whitch called by LSLCirq_IRQ_Handler(), TRAcodec_Dispatch_Pending_SCO(), TRAhcit_Dispatch_Pending_Data()*/
    if((clock_sel == 0) || (clock_sel == 1)) clock_sel += 2;//if is CPU_CLK_APLL and CPU_CLK_XTAL_APLL
    #endif
    #if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    if (b_26m_clock_closed)
    {
        return;
    }
    #endif
#if ((CONFIG_CHARGE_EN == 1)&&(CONFIG_CPU_CLK_OPTIMIZATION == 1))
/* ???????????????????????????????????? */
#if POWER_ON_OUT_CHARGE_ENABLE
    if(GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO)
        &&(app_bt_flag1_get(APP_AUDIO_FLAG_SET|APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION)))
    {
        clock_sel = CPU_CLK_SEL;
        div = CPU_CLK_DIV;
    }
#endif
/* ???????????????????????????????????? */
    if(app_charge_is_powerdown())
    {
        clock_sel = CPU_SLEEP_CLK_SEL;
        div = CPU_SLEEP_CLK_DIV;
    }
#endif
#if (CONFIG_CPU_CLK_OPTIMIZATION == 0)
#if (CONFIG_PRE_EQ == 1)
    app_env_handle_t env_h = app_env_get_handle();
    if (env_h->env_cfg.aud_eq.eq_enable 
    #if (CONFIG_EAR_IN == 1)
        && (get_earin_status()||!app_env_get_pin_enable(PIN_earInDet))
    #endif
        )
    {
        clock_sel = CPU_EQ_CLK_SEL;
        div = CPU_EQ_CLK_DIV;
    }
#endif
#if CONFIG_ANC_ENABLE == 1
    if((app_anc_status_get() > ANC_STATUS_IDLE)
    #if (CONFIG_EAR_IN == 1)
        && (get_earin_status()||!app_env_get_pin_enable(PIN_earInDet))
    #endif
    )
    {
        clock_sel = CPU_ANC_CLK_SEL;
        div = CPU_ANC_CLK_DIV;
    }
#endif
#endif

    #if CONFIG_ANC_ENABLE == 1
    if((app_anc_status_get()))
    {
        clock_sel = CPU_ANC_CLK_SEL;
        div = CPU_ANC_CLK_DIV;
    }
    #endif

#if (CONFIG_DRIVER_OTA == 1)
    if(driver_ota_is_ongoing())
    {
        if((clock_sel == CPU_OPT_CLK_SEL) || (div == CPU_OPT_CLK_DIV))
        {
            clock_sel = CPU_CLK_SEL;
            div = CPU_CLK_DIV;
        }
    }
#endif

    // for lpbg calibration
    if(app_bt_flag1_get(APP_FLAG_FCC_MODE_ENABLE))
    {
        clock_sel = CPU_CLK_XTAL;
        div = 0;
    }
#if (CONFIG_ENC_ENABLE == 1)
    if(app_bt_flag1_get(APP_FLAG_SCO_CONNECTION))
    {
        clock_sel = CPU_ENC_CLK_SEL;
        div = CPU_ENC_CLK_DIV;
        //if((clock_sel == CPU_CLK_XTAL) || (div == CPU_OPT_CLK_DIV))
        //{
        //    clock_sel = CPU_ENC_CLK_SEL;
        //    div = 2;
        //}
    }
#endif

#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
    u_int32 cpu_sel = 0;
    u_int32 div_sel = 0;
    //cpu_sel = (REG_SYSTEM_0x00 >> SFT_SYSTEM_0x00_CORE_SEL) & MSK_SYSTEM_0x00_CORE_SEL;
    //div_sel = (REG_SYSTEM_0x00 >> SFT_SYSTEM_0x00_CORE_DIV) & MSK_SYSTEM_0x00_CORE_DIV;
    reg = REG_SYSTEM_0x00;
    cpu_sel = (reg & MSK_SYSTEM_0x00_CORE_SEL) >> SFT_SYSTEM_0x00_CORE_SEL;
    div_sel = (reg & MSK_SYSTEM_0x00_CORE_DIV) >> SFT_SYSTEM_0x00_CORE_DIV;
    if((cpu_sel == clock_sel)&&(div_sel == div))
        return;
    if((clock_sel == CPU_CLK_XTAL) || (div == CPU_OPT_CLK_DIV))
    {
        reg = REG_SYSTEM_0x00;
        reg &= ~(MSK_SYSTEM_0x00_CORE_SEL | MSK_SYSTEM_0x00_CORE_DIV);
        REG_SYSTEM_0x00 = reg | (clock_sel << SFT_SYSTEM_0x00_CORE_SEL) | (div << SFT_SYSTEM_0x00_CORE_DIV);
        sys_delay_cycle(12);
        app_env_reconfig_dig_voltage(-1); // Decrease voltage of Digital core;
    }
    else
    {
        app_env_reconfig_dig_voltage(0);  // Restore voltage of Digital core;
        sys_delay_cycle(12);
        reg = REG_SYSTEM_0x00;
        reg &= ~(MSK_SYSTEM_0x00_CORE_SEL | MSK_SYSTEM_0x00_CORE_DIV);
        REG_SYSTEM_0x00 = reg | (clock_sel << SFT_SYSTEM_0x00_CORE_SEL) | (div << SFT_SYSTEM_0x00_CORE_DIV);
    }
#else
    reg = REG_SYSTEM_0x00;
    reg &= ~(MSK_SYSTEM_0x00_CORE_SEL | MSK_SYSTEM_0x00_CORE_DIV);
    REG_SYSTEM_0x00 = reg | (clock_sel << SFT_SYSTEM_0x00_CORE_SEL) | (div << SFT_SYSTEM_0x00_CORE_DIV);
#endif

    
}

#if (CONFIG_CPU_CLK_OPTIMIZATION == 0)
DRAM_CODE void System_Config_MCU_For_eSCO(void)
{
    app_env_reconfig_ana_reg(4,2);  // aana = 6,dig = 6;
    BK3000_set_clock(CPU_CLK_SEL,CPU_CLK_DIV_OF_SCO);
}
DRAM_CODE void System_Config_MCU_Restore(void)
{
    app_env_reconfig_ana_reg(0,0);
    BK3000_set_clock(CPU_CLK_SEL,CPU_CLK_DIV);
}
#endif
DRAM_CODE unsigned char BK3000_hfp_set_powercontrol(void)
{
    unsigned char set_power = 0;
    set_power = 0x18;
    set_power = set_power &0x1f;

    return set_power;
}

void VICMR_enable_intr_src(uint32_t mask)
{
    uint32_t int_mask;

    int_mask = get_spr(SPR_VICMR(0));
    int_mask |= mask;
    set_spr( SPR_VICMR(0), int_mask);
}

void VICMR_disable_intr_src(uint32_t mask)
{
    uint32_t int_mask;

    int_mask = get_spr(SPR_VICMR(0));
    int_mask &= (~mask);
    set_spr( SPR_VICMR(0), int_mask);
}

void VICMR_usb_chief_intr_enable(void)
{
    VICMR_enable_intr_src(1<<VIC_IDX_USB0);
}

void VICMR_usb_chief_intr_disable(void)
{
    VICMR_disable_intr_src(1<<VIC_IDX_USB0);
}

void ba22_disable_intr_exception(void)
{
    cpu_set_interrupts_enabled(0);
}

void ba22_enable_intr_exception(void)
{
    cpu_set_interrupts_enabled(1);
}

#ifdef	WROK_AROUND_DCACHE_BUG
inline void app_Dcache_disable(void)
{
    //disable dcache
	unsigned int sr ;
	sr = get_spr(SPR_SR);
	sr = sr & (~SPR_SR_DCE);
	set_spr(SPR_SR, sr);

	//gpio_output( 9, 1 );
	//os_printf("Dcache_disable\r\n");
}

inline void app_Dcache_enable(void)
{
	//enable dcache
	unsigned int sr ;
	sr = get_spr(SPR_SR);
	sr = sr | SPR_SR_DCE;
	set_spr(SPR_SR, sr);
	//gpio_output( 9, 0 );
	//os_printf("old\r\n");
}

inline void app_Dcache_initial(void)
{
	unsigned int sr ;

	//disable dcache
	sr = get_spr(SPR_SR);
	sr = sr & (~SPR_SR_DCE);
	set_spr(SPR_SR, sr);

	//initial dcache
	set_spr(SPR_RIR_MIN, 0);
	set_spr(SPR_RIR_MAX, 0xfffffff4);

	//enable dcache
	sr = get_spr(SPR_SR);
	sr = sr | SPR_SR_DCE;
	set_spr(SPR_SR, sr);

	//disable dcache
	sr = get_spr(SPR_SR);
	sr = sr & (~SPR_SR_DCE);
	set_spr(SPR_SR, sr);
}
#endif

void bt_ext_wakeup_generate(void)
{
    REG_PMU_0x02 &= ~(1<<21);
    REG_PMU_0x02 |= 1<<24;
    delay_us(100);
    REG_PMU_0x02 &= ~(1<<24);
}

#if (BT_DUALMODE_RW == 1)
void rw_ext_wakeup_generate(void)
{
    REG_PMU_0x02 &= ~(1<<22);
    REG_PMU_0x02 |= 1<<23;
    delay_us(100);
    REG_PMU_0x02 &= ~(1<<23);
}
#endif

#ifdef CONFIG_PRODUCT_TEST_INF
uint8_t rssi = 0;
uint8_t aver_rssi = 0;
int16_t freq_offset = 0,aver_offset=0;
#ifdef AUTO_CALIBRATE_FREQ_OFFSET
uint32_t test_flag = 0;
#endif

inline void get_freqoffset_rssi(void)
{
     rssi = REG_XVR_0x12 & 0xff;
     freq_offset = REG_XVR_0x14 & 0x1ff;
}

void average_freqoffset_rssi(void)
{
    static uint16_t sum_rssi=0;
    static int16_t sum_offset=0;
    static uint8_t i = 0;
    int16_t temp = freq_offset;
    sum_rssi += rssi;
    temp <<= 7;
    temp >>= 7;
    sum_offset += temp;
    i++;
    if (5 == i)
    {
        aver_rssi=sum_rssi/i;
        aver_offset = sum_offset/i;
        i = 0;
        sum_rssi = 0;
        sum_offset = 0;
#ifdef AUTO_CALIBRATE_FREQ_OFFSET
        app_auto_calibrate_freq_offset();
#endif
    }

}
#else
inline void get_freqoffset_rssi(void)
{

}
#endif
// end of file
