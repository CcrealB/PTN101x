/**
 * **************************************************************************************
 * @file    audio_sync_usb.c
 * 
 * @author  Borg Xiao
 * @date    20230315
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * **************************************************************************************
 * */
#ifndef _AUDIO_SYNC_USB_H_
#define _AUDIO_SYNC_USB_H_

#include <stdint.h>

void usbo_aud_sync_reset(void);
void usbo_aud_sync_init(void);
int usbo_aud_sync_proc(int frame_smps, int lvl_cur);

void usbi_aud_sync_reset(void);
void usbi_aud_sync_init(void);
int usbi_aud_sync_proc(int frame_smps, int lvl_cur);


#endif /* _AUDIO_SYNC_USBO_H_ */
