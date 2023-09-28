

#ifndef _APP_DSP_H_
#define _APP_DSP_H_

#include <stdint.h>
#include "drv_mailbox.h"

#define DSP_EXCEPTION_CHECK


uint32_t dsp_drv_ver_get(void);
void dsp_performance_test(void);

uint32_t dsp_run_time_eft_get(void);
uint32_t dsp_run_time_all_get(void);
void dsp_run_time_clear(void);
uint32_t dsp_run_tick_all_get(void);
void dsp_run_tick_clear(void);

void dsp_log_ctrl(int log_sel, int loop_log_en);

void app_dsp_cmd_msg_init(void);
int app_dsp_cmd_msg_push(MailBoxCmd *p_mbx);

void* app_dsp_spc_h_get(char* str, uint32_t as_sel);

int app_dsp_log_proc(uint8_t log_en);
void app_dsp_init(void);

#if 1//def DSP_EXCEPTION_CHECK
void dsp_exception_callback(void);
void dsp_exception_check_task(int loop_ms);
#endif

//dsp audio
extern void audio_interface_map_config(void);
extern void dsp_audio_init(void);

//dsp drv & sys
extern void dsp_boot(void);
extern void dsp_shutdown(void);
extern uint8_t dsp_is_working(void);

int app_sys_log_write(uint8_t *buf, int size);
// extern int dsp_log_out(uint8_t *buf, int size);
#define dsp_log_out     app_sys_log_write

#endif //_APP_DSP_H_
//EOF
