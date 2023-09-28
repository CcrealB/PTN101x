#ifndef _DRIVER_RTC_H_
#define _DRIVER_RTC_H_

#if(CONFIG_DRIVER_RTC==1)


typedef struct{
uint8_t hour;
uint8_t minute;
uint8_t second;
}rtc_watch_time;

typedef void (*rtc_interrupt_cb)(rtc_watch_time* time);

void rtc_band_calibration(void);
void rtc_lpoclk_sel(uint32 sel);
void rtc_initial(uint32 value, uint32 tick);
void rtc_initial_deepsleep(uint32 value, uint32 tick);
void clr_rtc_initial_norm(void);
void rtc_stop(void);
void rtc_isr(void);
uint8_t rtc_sleep(void);
uint32 rtc_get_value(void);
uint32  rtc_get_value1(void);
uint32 rtc_get_value2(void);
void rtc_rtc_cnt_reset(void);

void rtc_get_time(rtc_watch_time* time);
void rtc_register_cb(rtc_interrupt_cb cb);

#endif
#endif

