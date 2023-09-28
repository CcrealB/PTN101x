
#ifndef _TIMER_H_
#define _TIMER_H_

#include <uw_types.h>

#ifdef CONFIG_SYS_TICK_INT_1MS
    #define TIMER0_PT0_COUNT        1000
#else
    #define TIMER0_PT0_COUNT        10000     // 1MHz clock 10ms timer
#endif
#define os_timer_is_expired_for_time(timer,ref_timer) (((timer) < (ref_timer))? TRUE:FALSE)

__inline uint32_t sys_time_get(void);
#define sys_timeout(t_ref, ms)       ((uint32_t)(sys_time_get() - (t_ref)) >= (ms))


__inline uint32_t os_timer_get_current_tick(void);
extern void timer_timer0_pt0_init(void);
extern void RAM_CODE timer_timer0_pt0_swi(void);
extern uint64_t os_get_tick_counter(void);
DRAM_CODE extern void critical_path_handler(void);

typedef enum _AUD_SWI_FLAG_e{
    SWI_FLAG_TIMER   = 0x1,
    SWI_FLAG_USB_IN  = 0x2,
    SWI_FLAG_USB_OUT = 0x4,
    SWI_FLAG_ALL     = 0xFF,
}AUD_SWI_FLAG_e;

uint32_t audio_swi_flag_get(uint32_t flag);
void audio_swi_flag_set(uint32_t flag, uint8_t en);

void os_tick_delay(uint32_t count);
void compute_cpu_speed(void);
void timer_polling_handler(uint32_t step);
void timer_clear_watch_dog(void);
// int os_timer_is_pending(os_timer_h timer_h);
DRAM_CODE void sniff_enable_timer0_pt0(void);
void sniff_tick_counter(void);
void disable_timer0_pt0(void) ;
void enable_timer0_pt0(void) ;
#endif // _TIMER_H_

//EOF
