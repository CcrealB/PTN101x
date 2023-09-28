/**
 * **************************************************************************************
 * @file    app_spdif.h
 * 
 * @author  Borg Xiao
 * @date    20230217
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */

#ifndef _APP_SPDIF_H_
#define _APP_SPDIF_H_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


void spdif_log_flag_set(uint32_t flag, uint8_t en);
uint32_t spdif_log_flag_get(uint32_t flag);

void app_spdif_ready_callback(void);
void app_spdif_lost_callback(void);
int app_spdif_scan(void);

int app_spdif_init(uint8_t enable);

int app_spdif_enter(void);
void app_spdif_exit(void);

int spdif_is_online(void);  //judge if spdif have valid audio signal

int app_spdif_ready(void);  //spdif detect cmp and ready to start audio stream
void app_spdif_start(uint8_t en);
int app_spdif_fs_get(void);
void app_spdif_task(void);


extern void spdif_aud_sync_init(void);
extern void spdif_aud_sync_reset(void);
extern int spdif_aud_sync_proc(int frame_smps, int lvl_cur);

#ifdef  __cplusplus
}
#endif//__cplusplus


#endif //_APP_SPDIF_H_
