#ifndef __DRIVER_VUSB_DLP_H__
#define __DRIVER_VUSB_DLP_H__
#include "types.h"
//#include "driver_vusb.h"
#define DLP_LOWER_BAUDRATE      	9600
#define DLP_LOWER_MEDIUM_BAUDRATE   19200
#define DLP_MEDIUM_BAUDRATE     	115200
#define DLP_HIGH_BAUDRATE       	1000000
#define DLP_ULTRA_BAUDRATE      	16000000

#define VUSB_DLP_MEMORY		0x04
#define VUSB_DLP_MATCH		0x20
#define VUSB_DLP_POP		0x21
#define VUSB_DLP_POLL		0x22
#define VUSB_DLP_PWR		0x23

#define VUSB_DLP_POLL_RSP_TWS	0x01
#define VUSB_DLP_POLL_RSP_PHONE	0x02

#define VUSB_DLP_HEADSET_EN_IN	0x01	
#define VUSB_DLP_HEADSET_R_IN	0x02
#define VUSB_DLP_HEADSET_ALL_IN	0x03
#define VUSB_DLP_HEADSET_POPUP_RAND	0x80

void vusb_dlp_clk_select(uint8 sel);
void vusb_dlp_init(uint32 baudrate);
void vusb_dlp_enable(void);
void vusb_dlp_disable(void);
void vusb_dlp_send(uint8 *buff,uint16 len);
void vusb_dlp_isr(void);
void vusb_dlp_handler(uint32 step);
#if 0
uint8 vusb_dlp_is_box_bat(void);
void vusb_dlp_popup_pairing(uint8 para);
uint8 vusb_dlp_popup_window(uint8 *param);
uint8 vusb_dlp_get_putin(void);
uint8 vusb_dlp_get_putin_popup_rand(void);
uint8 vusb_dlp_get_sec_putin(void);
void vusb_dlp_sec_putin_status(uint8 *param);
uint8 vusb_dlp_is_com_status(void);
#endif
#endif

