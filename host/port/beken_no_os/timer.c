/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "bkreg.h"
#include "includes.h"
#include "app_beken_includes.h"
#include "bt_a2dp_mpeg_aac_decode.h"

#include "rec_api.h"

static os_timer_t *timers_head = NULL;
static __VOLATILE__ uint64_t tick_counter = 0;

volatile uint32_t flag_clr_wdt = 0;
volatile int16_t s_sys_idle_cnt = 0;

#if 1//def CONFIG_SYS_TICK_INT_1MS
static __VOLATILE__ uint32_t sys_tick = 0;//unit:1ms
__inline uint32_t sys_time_get(void)
{
    return sys_tick;
}
#endif
__inline uint32_t os_timer_get_current_tick(void)//unit:10ms
{
    return tick_counter;
}

/* Initialize new timer */
result_t os_timer_init(os_timer_h *timer_h)
{
    DECLARE_FNAME("os_timer_init");

    *timer_h = (os_timer_h)jmalloc(sizeof(os_timer_t), M_ZERO);

    return UWE_OK;
}

/* Un-Initialize timer */
void os_timer_uninit(os_timer_h timer_h)
{
    os_timer_t *node = (os_timer_t *)timer_h;
    DECLARE_FNAME("os_timer_uninit");

    if (node->is_pending)
        os_timer_stop(timer_h);

    if (timer_h)
    {
        jfree(timer_h);
        timer_h = NULL;
    }
}

int os_timer_is_pending(os_timer_h timer_h)
{
    os_timer_t *node = (os_timer_t *)timer_h;

    return (node->is_pending);
}

/* Reset/Start timer with specific timeout (in msec) */
// the last parameter "arg" need be global
result_t os_timer_reset(os_timer_h timer_h, uint32_t msec, jthread_func func,
                        void *arg)
{
    os_timer_t *node = (os_timer_t *)timer_h;
    os_timer_t **iter = &timers_head;
    DECLARE_FNAME("os_timer_reset");

    if (node->is_pending)
        os_timer_stop(timer_h);

    node->is_pending = 1;
    node->ticks = (msec + (MSEC_IN_TICK - 1))/MSEC_IN_TICK;
    node->ticks += tick_counter;
    node->f = func;
    node->arg = arg;

    while (*iter && ((*iter)->ticks < node->ticks))
        iter = &(*iter)->next;

    node->next = *iter;
    *iter = node;

    return UWE_OK;
}

/* Stop timer */
void os_timer_stop(os_timer_h timer_h)
{
    os_timer_t *node = (os_timer_t *)timer_h;
    os_timer_t **iter = &timers_head;
    DECLARE_FNAME("os_timer_stop");

    if (!node->is_pending)
        return;

    node->is_pending = 0;

    while (*iter && *iter != node)
        iter = &(*iter)->next;

    if (*iter)
        *iter = (*iter)->next;
}

/* Check if timer is waiting to run */
BOOL os_timer_pending(os_timer_h timer_h)
{
    DECLARE_FNAME("os_timer_pending");
    return ((os_timer_t *)timer_h)->is_pending;
}

//#pragma push
//#pragma O0
static float loops_per_usec;

/* Gauge execution rate per clock tick */
void compute_cpu_speed(void)
{
#define LOOPS_FOR_COMPUTE 1000000
    __VOLATILE__ uint64_t a;
    __VOLATILE__ uint64_t b;
    __VOLATILE__ uint32_t count = LOOPS_FOR_COMPUTE;

    a = tick_counter;
    while (count)
        count--;

    b = tick_counter;

    loops_per_usec = LOOPS_FOR_COMPUTE /((b - a) * MSEC_IN_TICK);
    loops_per_usec /= 1000;
}

/* Delay execution for N micro-seconds (don't lose context) */
void os_delay_us(uint32_t usec) {
    __VOLATILE__ uint32_t loops;
    /* loops = usec * loops_per_usec; */
    /* while (loops) */
    /*     loops--; */
    while (usec--) {
        loops=10;
        while (loops--);
    }
}
//#pragma pop

void os_delay_ms (uint32_t msec) {        /**< 78mHz */
    __VOLATILE__ uint32_t loops;
    while (msec--) {
        loops=9700;
        while (loops--);
    }
}

/* Get current time (real or from system boot) */
void os_get_time(os_time_t *time)
{
    uint64_t timer = tick_counter;
    uint32_t tick_in_sec = 1000 / MSEC_IN_TICK;

    time->tv_sec = timer / tick_in_sec;
    time->tv_usec = (timer % tick_in_sec) * MSEC_IN_TICK * 1000;
}

/* Get ticks-in-second */
uint32_t os_get_hz(void)
{
    return 1000 / MSEC_IN_TICK;
}
__INLINE__ void set_system_idle_cnt(int16_t cnt_10ms)
{
    s_sys_idle_cnt = cnt_10ms;
}
__INLINE__ int16_t get_system_idle_cnt(void)
{
    return s_sys_idle_cnt;
}

#if (CONFIG_APP_ESD == 1)
static void app_esd_process(void)
{
	uint32_t temp;
	static uint32_t s_cnt_for = 0;
	
    if(++s_cnt_for % 300 == 0)
    {
        temp = REG_SYSTEM_0x55;
        REG_SYSTEM_0x55 = temp;

        temp = REG_SYSTEM_0x56;
        REG_SYSTEM_0x56 = temp;

        temp = REG_SYSTEM_0x57;
        REG_SYSTEM_0x57 = temp;
    }	
}
#endif

extern volatile uint32_t syspwr_num_frames_to_sleep_bak;
extern uint8_t syspwr_cpu_halt;
void timer_timer0_pt0_isr(void)
{
#ifdef CONFIG_SYS_TICK_INT_1MS
    static int run_cnt = 0;
    sys_tick++;
    if(sys_tick & 0x1)//2ms
    {
        audio_swi_flag_set(SWI_FLAG_TIMER, 1);
    }
    extern void tick_task_1ms(void);
    tick_task_1ms();
    if((++run_cnt) < 10) return;
    run_cnt = 0;
#else
    sys_tick += 10;
#endif
    uint32_t step;
    extern uint8_t syspwr_cpu_halt;
    static uint64_t last_tick_counter = 0;
#if (CONFIG_OTHER_FAST_PLAY == 1)
	app_handle_t sys_hdl = app_get_sys_handler();
	if (sys_hdl->otherplay)
		sys_hdl->otherplay --;	
#endif	
    step = 1;
    sniff_tick_counter();
    tick_counter += step;
#ifndef SARADC_CRITICAL_CODE_PROTECT//++ by borg @230201
    /* Sometime,the saradc intr may be missing */
    saradc_set_chnl_busy(0);
#endif
    timer_polling_handler(tick_counter-last_tick_counter);
    last_tick_counter = tick_counter;

#ifdef CONFIG_DRV_USB_HOST
	///////////////////////////////////////////////////////////
    extern void usbhost_busreset_proc(/*void*bp*/);
	usbhost_busreset_proc();//usb host bus reset
#endif
#ifdef CONFIG_WAV_PLAY_IN_INTR
    // app_wave_file_play();
#endif
    if(sniffmode_wakeup_dly)
        --sniffmode_wakeup_dly;
    else
        app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 0);
    
    //syspwr_num_frames_to_sleep_bak = 0;
    syspwr_cpu_halt = 0;

    if (adcmute_cnt > 0)
   		adcmute_cnt --;
    if(s_sys_idle_cnt > 0) // for close dac in idle;
        s_sys_idle_cnt--;
	
#if (CONFIG_AUD_FADE_IN_OUT == 1)
    aud_fade_in_out_process();
#endif

    //if (!bt_flag2_is_set(APP_FLAG2_STEREO_WORK_MODE))
    {
        sbc_dac_clk_tracking(step);
    }

#if (CONFIG_APP_ESD == 1)
	app_esd_process();
#endif
    SYSirq_soft_trigger_interrupt(VIC_IDX_SWI_SYS);
}

void disable_timer0_pt0(void) 
{
    // REG_TIMG0_0x03 &= ~MSK_TIMG0_0x03_TIMER0_EN;
    timer_enable(TIMER0, TIMER_CH0, 0);
}

DRAM_CODE void enable_timer0_pt0(void) 
{
    // REG_TIMG0_0x00  = TIMER0_PT0_COUNT;
    // REG_TIMG0_0x03 |= MSK_TIMG0_0x03_TIMER0_EN;
    timer_config(TIMER0, TIMER_CH0, TIMER0_PT0_COUNT, timer_timer0_pt0_isr);
    timer_enable(TIMER0, TIMER_CH0, 1);
}

#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
DRAM_CODE void sniff_enable_timer0_pt0(void)
{
    //enable_timer0_pt0();
    if (sniffmode_wakeup_dly)
	    app_bt_flag2_set(APP_FLAG2_WAKEUP_DLY, 1);
    //timer_timer0_pt0_isr();

}

void sniff_tick_counter(void)
{
    uint32_t step = 0;
#ifdef CONFIG_PWM_NOT_SLEEP
    extern uint8_t g_sniff_flag;
    if ((syspwr_num_frames_to_sleep_bak) && (syspwr_cpu_halt) && (g_sniff_flag==2))
#else
    if ((syspwr_num_frames_to_sleep_bak) && (syspwr_cpu_halt) )
#endif
    {
        step = (2*1250*(syspwr_num_frames_to_sleep_bak))/10000;
        tick_counter += step;
        syspwr_num_frames_to_sleep_bak = 0;
    }
}
#else
void sniff_enable_timer0_pt0(void)
{

}
void sniff_tick_counter(void)
{

}
#endif

void timer_timer0_pt0_init(void)
{
    enable_timer0_pt0();
    /* WDT CLK 1/16 32K */
    REG_SYSTEM_0x01 &= ~(MSK_SYSTEM_0x01_WDTS_DIV);//wd timer select
    REG_SYSTEM_0x01 |= (0x01 << SFT_SYSTEM_0x01_WDTS_DIV);
}

void  RAM_CODE timer_timer0_pt0_swi(void)
{
    MSG_T msg;
    os_timer_t *tmp;

    while (timers_head && timers_head->ticks <= tick_counter)
    {
    	tmp = timers_head;
        timers_head = timers_head->next;
        tmp->is_pending = 0;
        msg.id = MSG_TIMER_PWM1_PT2_HDL;
        msg.hdl = (void *)tmp->f;
        msg.arg = (uint32_t)tmp->arg;

        if(msg_lush_put(&msg)==MSG_FAILURE_RET)
        {
            os_timer_t **iter = &timers_head;
            tmp->is_pending = 1;
            tmp->ticks += 100;
            while (*iter && ((*iter)->ticks < tmp->ticks))
                iter = &(*iter)->next;

             tmp->next = *iter;
            *iter = tmp;
        }
    }
}

uint64_t os_get_tick_counter(void)
{
    return tick_counter;
}

void os_tick_delay(uint32_t count)
{
    uint64_t t0 = os_get_tick_counter();

    while((t0 + (count)) > os_get_tick_counter())
    {
        os_printf("-");
    }
}

void timer_polling_handler(uint32_t step)
{
#if 1
    flag_clr_wdt += step;
#else
    CLEAR_WDT;
#endif
	sleep_tick += step;
    app_button_led_action(step);
    app_low_power_scanning(step);
#if (CONFIG_TEMPERATURE_DETECT == 1)
    app_temprature_scanning();
#endif
#if (CONFIG_EAR_IN == 1)
#if (CONFIG_DRIVER_PWM == 1)
    app_earin_pwm_handler(step);
#endif
    app_ear_in_scanning(step);
#endif
#if (CONFIG_CHARGE_EN == 1)
	app_charge_handler(step);
#endif
    /* app_linein_scanning(); */
    vusb_dlp_handler(step);

    /* sd_connect_scanning(); */
    /* button_scanning(); */
    //app_wave_file_play();
    timer_timer0_pt0_swi();
}
void timer_clear_watch_dog(void)
{
    if(flag_clr_wdt > 100) // clr wdt :1s
    {
        CLEAR_WDT;
        flag_clr_wdt = 0;
    }
}
#if A2DP_ROLE_SOURCE_CODE
extern void sbcShowAllStatus(void);
extern result_t RAM_CODE sbcDoEncode( void );
extern BOOL isA2dpSrcConnected(void);
extern uint8_t get_a2dp_role(void);

void timerPt1Start(uint32_t val)
{
	BK3000_PWM1_PT1_CONF = val<<sft_PWM_PT01_CNT_TO;
	BK3000_PWM1_PWM_CTRL = (1<<sft_PWM_PT1_ENABLE)|bit_PWM_PT1_INT_FLAG;
}

#endif
extern int hf_sco_handle_process(int oper);

void bt_audio_task(void)
{
    BT_DECODE_TASK_RUN(1);

#ifdef CONFIG_APP_AEC
    if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AEC_ENABLE))
        app_aec_swi();
#endif

#if A2DP_ROLE_SOURCE_CODE
    if((get_a2dp_role() == A2DP_ROLE_AS_SRC) && isA2dpSrcConnected() && app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE))
    {
        sbcDoEncode();
        //uint8_t err = sbcDoEncode();
        //os_printf(">>> sbcDoEncode,%d\r\n",err);
    }
    else if(get_a2dp_role() == A2DP_ROLE_AS_SNK)
    {
        sbc_do_decode();

#ifdef A2DP_VENDOR_DECODE
        vendor_do_decode();
#endif

    }

#else
    sbc_do_decode();

#ifdef A2DP_MPEG_AAC_DECODE
    bt_a2dp_aac_stream_decode();
#endif

#ifdef A2DP_VENDOR_DECODE
    vendor_do_decode();
#endif

#if 1
//for customers  who want BAT_DISPLAY,without HFP
    app_env_handle_t	env_h = app_env_get_handle();
    if((!(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_HFP))
                    &&(env_h->env_cfg.bt_para.bt_flag & APP_ENV_BT_FLAG_APP_BAT_DISPLAY)
                    )
    {
        if((get_current_hfp_flag(APP_FLAG_HFP_CALLSETUP|APP_FLAG_CALL_ESTABLISHED))
            &&(get_current_hfp_flag(APP_FLAG_SCO_CONNECTION)))
        {
            hf_sco_handle_process(1);
        }
    }
#endif
#endif
    BT_DECODE_TASK_RUN(0);
}

extern int fat_delay_flag;
DRAM_CODE void critical_path_handler(void)
{
    uint8_t mode;

    mode = get_app_mode();
    switch(mode)
    {
#if (CONFIG_APP_MP3PLAYER == 1)
    case SYS_WM_UDISK_MODE: //CONFIG_APP_USB_DISK
    case SYS_WM_SDCARD_MODE:
        if(!get_current_hfp_flag(APP_FLAG_CALL_ESTABLISHED|APP_FLAG_HFP_OUTGOING))
        {
            if(!fat_delay_flag) app_player_play_func();
        }
        break;
#endif
#ifndef CONFIG_BT_FUNC_INVALID
    // case SYS_WM_BT_MODE: bt_audio_task(); break;
#endif

    default:
        break;
    }
    #ifdef CONFIG_APP_RECORDER
    if(!fat_delay_flag) app_sound_record();
    #endif
}

#ifdef USER_KARAOK_MODE

static uint32_t s_swi_flag = 0;
uint32_t audio_swi_flag_get(uint32_t flag) { return s_swi_flag & flag; }
void audio_swi_flag_set(uint32_t flag, uint8_t en)
{
    if(en){
        s_swi_flag |= flag;
        SYSirq_soft_trigger_interrupt(VIC_IDX_SWI_AUDIO);
    }else{
        s_swi_flag &= ~flag;
        // SYSirq_soft_clear_interrupt(VIC_IDX_SWI_AUDIO);//clear in isr head
    }
}

// 10ms swi
DRAM_CODE void sys_task_swi(void)
{
    #ifdef CONFIG_WAV_PLAY_IN_INTR
        app_wave_file_play();
    #endif
    #ifndef CONFIG_BT_FUNC_INVALID
        if(get_app_mode() == SYS_WM_BT_MODE) bt_audio_task();
    #endif
    extern void usb_volume_schedule(void);
    usb_volume_schedule();
}

// 2ms swi, higher priority than sys_task_swi
DRAM_CODE void audio_task_swi(void)
{
    if(audio_swi_flag_get(SWI_FLAG_TIMER))
    {
        audio_swi_flag_set(SWI_FLAG_TIMER, 0);
    #ifdef CONFIG_APP_SPDIF
        // if(get_app_mode() == SYS_WM_SPDIF_MODE)
        {
            extern void app_spdif_task(void);
            app_spdif_task();
        }
    #endif
    #if !(defined(ASO_DEF_TRANS_BY_SHARE_RB) && defined(ASO_USB_TRANS_BY_SHARE_RB)) 
        audio_mcu2dsp_play();
    #endif
    #if !AUDIO_MCU2DSP_INVALID
        if(sys_time_get() > 500) audio_dsp2mcu_record();
    #endif
    }
#ifdef USB_AUDIO_TASK_SWI
    extern void app_usb_audio_proc(void);
    app_usb_audio_proc();
#endif
}
#endif

#if  CONFIG_TWS_TIMER1
void timer_timer0_pt1_start(uint32_t val)
{
    //INFO_PRT("timer_pt1_start:0x%x \r\n",  val);
    // REG_TIMG0_0x01  = val;
    // REG_TIMG0_0x03 |= MSK_TIMG0_0x03_TIMER1_EN;
    timer_config(TIMER0, TIMER_CH1, TIMER0_PT0_COUNT, timer_timer0_pt1_isr);
    timer_enable(TIMER0, TIMER_CH1, 1);
}

void timer_timer0_pt1_isr(void)
{
    aud_dac_open();
    app_bt_flag2_get(APP_FLAG2_STEREO_PLAYING_NO_USE, 1);
}


#endif


//#endif
/* end of file*/
