#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"
#include "bt_mini_sched.h"
#include "bt_ctrl_app.h"
#include "sys_irq.h"
#include "hw_leds.h"
#include "diskio.h"
#include "bkreg.h"
#include "sys_power.h"
#include "PTN101_calibration.h"
//#include "driver_usb.h"
#include "../usb_driver/api_usb.h"
#include "app_fat.h"
#if (BT_DUALMODE_RW == 1)
#include "rwble.h"
#endif
#include "u_include.h"
#if (BT_DUALMODE_RW == 1) && (defined(CONFIG_USE_BLE))
#include "app_fff0.h"
#endif

volatile uint8 bUsb = 0;
volatile uint16_t adcmute_cnt = 0;
volatile uint32_t sleep_tick  = 0;
volatile uint32_t pwdown_tick = 0;
static uint32_t g_sys_loop_start = 0;

extern void dsp_boot(void);
extern void controller_init(void);
extern RAM_CODE int msg_get(MSG_T *msg_ptr);
extern uint32_t msg_get_pool_status(void);

uint32_t sys_loop_start_time_get() { return g_sys_loop_start; }
static void bsp_init(void)
{
    //sys_trng_init();
	
    #ifdef CONFIG_IRDA
    IrDA_init();
    #endif

    #ifdef CONFIG_DRIVER_I2C
    i2c_init(100000, 1);
    #endif
    #ifdef CONFIG_DRIVER_I2C1
    i2c1_init(I2C1_DEFAULT_SLAVE_ADDRESS, 0);
    #endif
    saradc_reset();

    #ifdef CONFIG_APP_SDCARD
    //sd_detect_init();
    #endif
}

static void host_init(void) 
{
#ifdef WROK_AROUND_DCACHE_BUG
    app_Dcache_initial();
#endif

    bsp_init();

#if(CONFIG_DRIVER_I2S)
	//i2s_init(I2S1, I2S_ROLE_SLAVE, 44100, 16, 16);
#endif

    app_init();

    j_stack_init(NULL);

    msg_init();
    msg_clear_all();

    aud_init();

    LOG_I(APP,"| SW Build Time: %s, %s\r\n",__DATE__,__TIME__);
    
    app_post_init();
    app_env_post_init();
    
#if POWER_ON_OUT_CHARGE_ENABLE
    if(GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO))
        BK3000_start_wdt(0x0FFF);
    else 
#endif
    BK3000_start_wdt(0xFFFF);	//wdt reset 0xc000:10s     /* WDT CLK 1/16 32K */

#ifndef CONFIG_BT_FUNC_INVALID
    bt_connection_init();
#endif
#if ((CONFIG_CHARGE_EN == 1))
	app_charge_init();
#endif

#ifdef CONFIG_LINEIN_FUNCTION
    app_linein_init();
#endif


#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_DSP || CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_DSP
    sys_delay_ms(10);
#endif

    audio_dac_dc_calib(AUDIO_DAC_CALIB_FINISH);// For audio dc calibration, the time between aud_init and this function need be more than 2ms
    app_env_power_on_check();

#if CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_DSP || CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_DSP || CONFIG_ANC_ENABLE
    system_core_clk_set(SYS_CLK_XTAL, 1);
    dsp_boot();
    system_core_clk_set(CPU_CLK_SEL, CPU_CLK_DIV);
#if (CONFIG_ANC_ENABLE == 1)
    app_anc_init();
#endif
#endif
#if (CONFIG_ANC_ENABLE == 1)
    app_anc_cali_data_init();
#endif

#ifdef USER_KARAOK_MODE

#ifdef CONFIG_APP_IR_RX
    app_ir_rx_init();
#endif

    sys_delay_ms(1);
    app_dsp_init();
    sys_delay_ms(1);

    app_fat_init();
    
#if defined(CONFIG_APP_SPDIF) && defined(SPDIF_GPIO)
    app_spdif_init(1);
#endif
#endif

#if ((CONFIG_CHARGE_EN == 1))
    if(!app_charge_is_powerdown())
#endif
    {
#ifdef CONFIG_LINEIN_FUNCTION
        if(app_linein_powerup_check())
        {
            app_set_led_event_action(LED_EVENT_LINEIN_PLAY);
            app_bt_flag2_set(APP_FLAG2_LED_LINEIN_INIT , 1 );
        }
        else
#endif
            app_set_led_event_action( LED_EVENT_POWER_ON );
            app_poweron_xtal_self_test();   // if xtal don't work,cpu will halt and LED_EVENT_POWER_ON is always ON.

    #if POWER_ON_OUT_CHARGE_ENABLE
        if (GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO))
        {
             #ifdef WAKEUP_GPIO_OUT_CHARGE
            if(app_env_get_pin_enable(PIN_ioDet))
            {
                gpio_config(app_env_get_pin_num(PIN_ioDet), 5);
                uint64_t wakeup_pin_mask = (1UL << app_env_get_pin_num(PIN_ioDet));
                REG_GPIO_0x34 &= ~(uint32_t)(wakeup_pin_mask >> 32);
                REG_GPIO_0x33 &= ~ (uint32_t)(wakeup_pin_mask & 0xFFFFFFFF);
            }
            #endif

            //UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHG_FINISHED_WITH_GPIO);
            app_wave_file_play_start(APP_WAVE_FILE_ID_START);
        }
        else
    #endif
            if((HW_PWR_KEY==app_env_check_pwrCtrl_mode()) 
			&& !GET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_SOFT_RESET))
        {
//yuan++            app_wave_file_play_start(APP_WAVE_FILE_ID_START);
        }
	#if (CONFIG_EAR_IN == 1)
	    app_ear_in_init();
	#endif
    }
    UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_SOFT_RESET);
    #if (REMOTE_RANGE_PROMPT == 1)
    UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_REMOTE_RANGE);  
    #endif
    temperature_saradc_enable();
#if CONFIG_PWM_LED
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_PWM);
    pwm_led_enable(1);
#endif
    app_env_reconfig_ana_reg(0,0);
}

static void _clear_AHB_memory(uint8_t zero)
{
    extern uint32_t _ahbmem_begin;
    extern uint32_t _ahbmem_end;
    volatile uint32_t *p_ahbmem_begin = (volatile uint32_t *)((uint32_t)&_ahbmem_begin  & (~3));
    volatile uint32_t *p_ahbmem_end = (volatile uint32_t *)((uint32_t)&_ahbmem_end  & (~3));
    uint32_t fill_data = 0xDEADBEEF*(!!zero);
    while(p_ahbmem_begin < p_ahbmem_end)
    {
        *p_ahbmem_begin++ = fill_data;
    }    
}

static void public_init(void)
{
#ifdef CONFIG_DRIVER_I2C1
    REG_SYSTEM_0x01 = (0x08 << 22); //Open APB&CEVA
#else
    REG_SYSTEM_0x01 = (0x0A << 22); //(0x0B << 22); // pwd:RW/CEVA/AHB/APB,only Ceva ON;
#endif
#if(CONFIG_DRIVER_UART1 == 1)
    REG_SYSTEM_0x01 &= ~(0x01 << 22);  //APB on
#endif

    REG_SYSTEM_0x03 = (1 << 0);     // bypass sys clk gating;
    REG_SYSTEM_0x06 = 0;            //(1 << 19) | (1 << 21);
    #if !(CONFIG_DAC_CTRL_MODE == DAC_CTRL_BY_DSP || CONFIG_ADC_CTRL_MODE == ADC_CTRL_BY_DSP || CONFIG_ANC_ENABLE || CONFIG_ENC_ENABLE)
    REG_SYSTEM_0x15 = 0x01;
    #endif

#if (BT_DUALMODE_RW == 1)
    REG_SYSTEM_0x01 &= ~(0x8 << 22);    // peri_cfgpwd, RWbt ON
    REG_SYSTEM_0x03 |= (1 << 8);        // bypass sys clk gating, RWbt_clkgat_dis ON
#endif
    hal_uart0_init(115200, 0, 0);//may inited in boot, close default
    bt_ext_wakeup_generate();	//yuan++
    app_charge_save_flag_before_Wakeup();

    system_wdt_reset_ana_dig(1,0);
    system_analog_regs_init();
    os_delay_ms(2);
#if (EFUSE_EN==1)  //get calibration data in eFuse
    eFuse_get_cali_data();
#endif
    BK3000_GPIO_Initial();
    app_sys_log_init(1);
#ifdef CONFIG_CHARGE_INNER
    app_charge_mini_sched();
#else
    LOG_I(CHARGE,"VBAT:%d,%d\r\n", app_charge_check_vbat(),vusb_is_charging());
#endif
#if defined(ZT_M184)
    while(gpio_input(GPIO15)==1);
#endif
    system_abuck_enable(0);
    system_dbuck_enable(0);
    app_bt_ldo_init();
    system_apll_config(SYS_APLL_98p3040_MHZ);
#if (CPU_CLK_SEL == CPU_CLK_DPLL)
    system_dpll_config(CPU_DPLL_CLK);
    system_ctrl(SYS_CTRL_CMD_AHB_CLK_DIV2, 1);
    system_ctrl(SYS_CTRL_CMD_CPU_CLK_DIV2, 1);
    system_ctrl(SYS_CTRL_CMD_DSP_CLK_DIV2, 0);
#endif
    // BK3000_GPIO_Initial();
    // app_sys_log_init(1);//reinit for cover by GPIO_Initial
    SYSirq_Initialise();
#ifndef CONFIG_BT_FUNC_INVALID
    system_mem_clk_enable(SYS_MEM_CLK_ANC | SYS_MEM_CLK_CEVA | SYS_MEM_CLK_AHB0 | SYS_MEM_CLK_AHB1 | SYS_MEM_CLK_DLP);
    system_peri_clk_enable(SYS_PERI_CLK_FLSPLL | SYS_PERI_CLK_DMA | SYS_PERI_CLK_ANC | SYS_PERI_CLK_CEVA | SYS_PERI_CLK_TIM0 /*| SYS_PERI_CLK_DLP */);
#else
    system_mem_clk_enable(SYS_MEM_CLK_ANC /*| SYS_MEM_CLK_CEVA*/ | SYS_MEM_CLK_AHB0 | SYS_MEM_CLK_AHB1 | SYS_MEM_CLK_DLP);
    system_peri_clk_enable(SYS_PERI_CLK_FLSPLL | SYS_PERI_CLK_DMA | SYS_PERI_CLK_ANC /*| SYS_PERI_CLK_CEVA*/ | SYS_PERI_CLK_TIM0 /*| SYS_PERI_CLK_DLP */);
#endif
    /* Warning: LPO clk must select XTAL-DIVID */
    system_lpo_clk_select(LPO_CLK_SEL);
    /* Warning:In sleep mode,some module clk will power down by hardware */
    system_sleepmode_opt_select();
    _clear_AHB_memory(1);
#ifndef CONFIG_BT_FUNC_INVALID
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_GPIO | SYS_PERI_IRQ_CEVA | SYS_PERI_IRQ_TIMER0 | SYS_PERI_IRQ_SBC);
#else
    system_peri_mcu_irq_enable(SYS_PERI_IRQ_GPIO/*| SYS_PERI_IRQ_CEVA*/ | SYS_PERI_IRQ_TIMER0 | SYS_PERI_IRQ_SBC);
#endif
#if (CPU_CLK_SEL == CPU_CLK_DPLL)
    if(!dpll_pwron_check())
    {
        LOG_W(DPLL, "DPLL_restart...\r\n");
        dpll_restart();
    }
#endif
    LOG_I(APP,"Welcome PTN101x(%08X)\r\n"/*,BK_SYSTEM_CHIP_ID*/, BK_SYSTEM_DEVICE_ID);
    if(!app_charge_is_usb_plug_in())  // USB plug out
    {
        UNSET_PWR_DOWN_FLAG(PWR_DOWN_FLAG_CHGING_WITH_DEEPSLEEP);    
    }
#ifdef CONFIG_DRIVER_PWM
    pwm_peri_init();
#endif
}
int fat_delay_flag = 0;
void bk_bt_run_immediately(const char *func_name,uint8_t main_flag)
{
    return;
    fat_delay_flag = 1;
    critical_path_handler();
#if BT_DUALMODE_RW == 1
    rw_ble_schedule();
#endif
#ifndef CONFIG_BT_FUNC_INVALID
    BTms_Sched(1);
#endif
    fat_delay_flag = 0;
}

void system_task(void)
{
    static uint32_t t_mark = 0;
    if(sys_timeout(t_mark, 10))//10ms task
    {
        t_mark = sys_time_get();
        #ifdef CONFIG_SDCARD_DETECT
        app_sd_scanning();	//yuan++
        #endif
        #if USB_AUDIO_FUNC_VALID
        usb_io_as_state_update();
        #endif
        #ifdef DSP_EXCEPTION_CHECK
        dsp_exception_check_task(5000);
        #endif
        #ifdef SYS_TIME_STAMP_LOG
        static uint32_t t_mark1 = 0;
        if(sys_timeout(t_mark1, 10000))//10s task
        {
            t_mark1 = sys_time_get();
            int t_min = t_mark1 / 60000;
            int t_h = t_min / 60;
            LOG_I(DBG, "time:%u ms [%dh %dmin]\n", t_mark1, t_h, t_min % 60);
        }
        #endif
    //===================================================
    #if (USB0_FUNC_SEL || USB1_FUNC_SEL)	//yuan++
        usb_conn_det_proc();
    #endif
    #ifdef CONFIG_AUD_AMP_DET_FFT
        extern void audio_det_fft_task(uint32_t(*cbk_systime_get)(void));
        audio_det_fft_task(sys_time_get);
    #endif
    #ifdef APP_SPDIF_DEBUG
        extern void app_spdif_debug_show();
        app_spdif_debug_show();
    #endif
    }

    extern uint8_t g_dsp_log_on_fg;
    app_dsp_log_proc(g_dsp_log_on_fg);
}

static void _Stack_Integrity_Check(void) 
{
    extern uint32_t _sbss_end;
    extern uint32_t _stack;
    volatile uint32_t *p_sbss = (volatile uint32_t *)((uint32_t)&_sbss_end  & (~3));
    volatile uint32_t *p_dram_code  = (volatile uint32_t *)((uint32_t) &_stack);
    
    //LOG_I(STACK,"===system stack size:%p,%p,%d\r\n",&_stack,&_sbss_end,(uint32_t)&_stack - (uint32_t)&_sbss_end);
#if 1
    if (p_sbss[0] != 0XDEADBEEF) 
    {
        LOG_E(STACK,"ShowStack:%p:%p\r\n",  &_sbss_end, &_stack);
        LOG_E(STACK,"sbss:%p,dram_c:%p\r\n",  p_sbss, p_dram_code);
        LOG_E(STACK,"Stack overflow!\r\n");
        while(1);
    }
#endif
#if 0
    if(p_dram_code[0] != 0xDEADBEEF)
    {
        LOG_E(STACK,"DRAM_CODE is polluted\r\n");
        while(1);
    }
#endif
}

static inline void _stack_show(void) 
{
    extern uint32_t _sbss_end;
    extern uint32_t _stack;
    LOG_I(STACK,"Stack info:%p,%p,%d\r\n",  &_stack, &_sbss_end, (uint32_t)&_stack - (uint32_t)&_sbss_end);
}
extern void app_bt_print_ber(void);
int OPTIMIZE_O0 main(void)
{
    MSG_T msg;
    public_init();
    user_init_prv();

    BK3000_set_clock(CPU_CLK_XTAL, CPU_CLK_DIV);
    flash_init();
    app_env_init();
    app_env_core_pm_init();
    os_delay_us(400);
    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);
#ifndef CONFIG_BT_FUNC_INVALID
    bt_controller_cb_register();
#endif
    controller_init();
    host_init();
    // system_peri_mcu_irq_enable(SYS_PERI_IRQ_USBPLUG);
	#ifdef JMALLOC_STATISTICAL
    os_printf("JMALLOC_STATISTICAL: POWER ON\r\n");
    memory_usage_show();
	#endif
    #if (BT_DUALMODE_RW == 1)
    rw_ble_init();
    #ifdef CONFIG_USE_BLE
    app_fff0_init();
    #endif
    #endif

    #if (CONFIG_DRIVER_OTA == 1) && (defined(CONFIG_OTA_APP))
    driver_ota_erase_flash();
    #endif
	
    _stack_show();

    user_init();
#if CONFIG_USE_USB
    usbh_udisk_auto_op_init();
	extern void app_usb_init(void);
	app_usb_init();
#endif
#if (CONFIG_APP_ASYNC_DATA_STREAM == 1)
    app_async_data_stream_init();
#endif

    system_freq_update();

    os_printf("[MCU]: apll @ %d Hz\r\n", system_apll_freq());
    os_printf("[MCU]: dpll @ %d Hz\r\n", system_dpll_freq());
    os_printf("[MCU]: Core @ %d Hz\r\n", system_core_freq());
    os_printf("[MCU]: DSP  @ %d Hz\r\n", system_dsp_freq());
    os_printf("[MCU]: CPU  @ %d Hz\r\n", system_cpu_freq());

#if (ADC_KEY_NUM)|| (GPIO_KEY_NUM)	//yuan++
    extern void key_function(KEY_CONTEXT_t *pKeyArray);
    user_key_init(key_function);
#endif

#if (KbonNUM)		//yuan++
	u_SaradcKbon_Init();	// 旋钮初始化
#endif
    user_init_post();
    g_sys_loop_start = sys_time_get();
    os_printf("loop start @ %d ms\r\n", sys_time_get());
    while(1)
    {
#ifdef CONFIG_DRV_USB_HOST
    	extern void bot_proc();
        bot_proc();
#endif
        if(MSG_FAILURE_RET == msg_get(&msg)) msg.id = MSG_NULL;
        system_task();
        user_loop();

        #ifndef CONFIG_BT_FUNC_INVALID
        if(app_is_bt_mode())//yuan++
        {
            bt_app_management_sched();
            bt_connect_scheduler();
        }
        #endif
        #ifndef CONFIG_WAV_PLAY_IN_INTR
        app_wave_file_play();
        #endif
        common_msg_handler(&msg);
        critical_path_handler();
        #if (BT_DUALMODE_RW == 1)
        rw_ble_schedule();
        #endif
        #ifndef CONFIG_BT_FUNC_INVALID
        BTms_Sched(2);
        #endif
        
        
        #if (CONFIG_DRIVER_OTA == 1)
        driver_ota_write_flash();
        #endif

		#if(CONFIG_DAC_CLOSE_IN_IDLE == 1)
        //aud_dac_close_in_idle();
		#endif

		#if (CONFIG_CPU_CLK_OPTIMIZATION == 1)
        /* In idle,set cpu clk = 26mHz/2 */
        BK3000_set_clock(CPU_OPT_CLK_SEL, CPU_OPT_CLK_DIV); 
        #else
        if(!SYSpwr_Is_Available_Halt_System() && msg_get_pool_status() == MSG_POOL_EMPTY && !hfp_has_sco())
        {
            system_cpu_halt();
        }
        #endif
        #ifndef CONFIG_BT_FUNC_INVALID
        app_bt_print_ber();
        #endif

        #if (CONFIG_APP_ASYNC_DATA_STREAM == 1)
        app_async_data_stream_handler();
        #endif

        timer_clear_watch_dog();
        _Stack_Integrity_Check();

    #if APP_ASYNC_SDCARD_ENABLE
        f_async_timming_call();
        disk_async_timming_call();
    #endif
    
    #ifdef TEST_FATFS_WRITE_SPEED
        extern void fatfs_rw_speed_test(void);
        fatfs_rw_speed_test();
    #endif
    }

    return 0;
}
