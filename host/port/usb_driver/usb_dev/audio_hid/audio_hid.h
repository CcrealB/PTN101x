#ifndef _AUDIO_HID_H_
#define _AUDIO_HID_H_

#include <stdint.h>

/* -------------------------------------------------- *///debug
// #define USB_DEV_DEBUG

/* -------------------------------------------------- *///config

// #define USB_AUDIO_FUNC_VALID        0
#define USB_AUDIO_STREAM_STATE_VALID    1


// the macro below define @ config.h
// #define CONFIG_AUD_SYNC_USBO            2 // 0:disable/1:spc by mcu/2:spc by dsp
// #define CONFIG_AUD_SYNC_USBI            1 // 0:disable/1:spc by mcu/2:spc by dsp

/* -------------------------------------------------- */
#ifndef std_printf
#define std_printf                  os_printf
#endif
#define USBD_LOG_I(fmt,...)         std_printf("[USBD|I]"fmt, ##__VA_ARGS__)
#define USBD_LOG_W(fmt,...)         std_printf("[USBD|W:%d]"fmt, __LINE__, ##__VA_ARGS__)
#define USBD_LOG_E(fmt,...)         std_printf("[USBD|E:%s():%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define USBD_LOG_MSK_ASO            0x1
#define USBD_LOG_MSK_ASO_STD        0x2
#define USBD_LOG_MSK_ASI            0x4
#define USBD_LOG_MSK_ASI_STD        0x8
#define USBD_LOG_MSK_DBG            0x10
#define USBD_LOG_MSK_ALL            0xFF
#define USBO_LOG_AS(fmt,...)        do{ if(usbd_log_flag_get(USBD_LOG_MSK_ASO))      std_printf("[UACO] "fmt, ##__VA_ARGS__); }while(0)
#define USBO_LOG_AS_STD(fmt,...)    do{ if(usbd_log_flag_get(USBD_LOG_MSK_ASO_STD))  std_printf(fmt, ##__VA_ARGS__); }while(0)
#define USBI_LOG_AS(fmt,...)        do{ if(usbd_log_flag_get(USBD_LOG_MSK_ASI))      std_printf("[UACI] "fmt, ##__VA_ARGS__); }while(0)
#define USBI_LOG_AS_STD(fmt,...)    do{ if(usbd_log_flag_get(USBD_LOG_MSK_ASI_STD))  std_printf(fmt, ##__VA_ARGS__); }while(0)

#ifndef UNUSED
    #define UNUSED(v)   (void)v
#endif

#define USBI_Byte2Smp(size)  ((size) >> (2 + 0))/*16/32bit2ch*/
#define USBI_Smp2Byte(smps)  ((smps) << (2 + 0))
#define USBO_Byte2Smp(size)  ((size) >> (2 + 0))
#define USBO_Smp2Byte(smps)  ((smps) << (2 + 0))


/*define current USB App feature */
#define USB_MAX_CONFIG_NUM				1

#define _AUD_HID_VID_					0x1234
#define _AUD_HID_PID_					0x0202

enum{
	_Language_String_ID_=0,
	_Vendor_String_ID_,
	_Product_String_ID_,
	_Sn_String_ID_
};

enum {
	USB_INTFID_AudioAC=0,
	USB_INTFID_AudioAS_MIC,
	USB_INTFID_AudioAS_SPK,
	USB_INTFID_Hid_Mse,
	USB_INTFID_Hid_Dbg,
	USB_MAX_INTERFACE_NUM,
};

enum{
	USB_ENDPID_Default=0,
	USB_ENDPID_Audio_MIC,
	USB_ENDPID_Audio_SPK,

	USB_ENDPID_Hid_MSE,
	USB_ENDPID_Hid_DBG_OUT,
	USB_ENDPID_Hid_DBG_IN,
	USB_MAX_ENDPOINT_NUM,
};

enum{
	USB_AUDIO_SR_0=8000,
	USB_AUDIO_SR_1=16000,
	USB_AUDIO_SR_2=32000,
	USB_AUDIO_SR_3=44100,
	USB_AUDIO_SR_4=48000,
	USB_AUDIO_SR_MAX=USB_AUDIO_SR_4,
};
//note: 3435\3431Q have 5 usb endpoint(EP0~EP4),fifo size = 64Bytes, max value of TXMAXP/RXMAXP is 8
//note：PTN101x usb have 9 endpoint(EP0~EP8), EP1~EP4 have dual 256 bytes FIFO , EP5~EP8 have single 64 bytes FIFO
#define AUDIO_MIC_SAMPRATE			48000
#define AUDIO_MIC_CHNs				2
#define AUDIO_MIC_BR				16

#define AUDIO_SPK_SAMPRATE			48000
#define AUDIO_SPK_CHNs				2
#define AUDIO_SPK_BR				16
#define AUDIO_MAXP(chns,br,sr)		(((sr)/1000*1000)==(sr))?((sr/1000)*chns*br/8):((sr/1000+1)*chns*br/8)

//usb audio topology module id
#define USBAUDIO_IT_MIC				1
#define USBAUDIO_FU_MIC				2
#define USBAUDIO_OT_MIC				3
#define USBAUDIO_IT_SPK				4
#define USBAUDIO_FU_SPK				5
#define USBAUDIO_OT_SPK				6


/* --------------------------------------------------- */
//csm hid cmd, refer to hid desc
typedef enum _HID_CMD_CSM_e{
    CSM_HID_END = 0,
//first byte
    CSM_MUTE    = (1 << 0),
    CSM_VOL_P   = (1 << 1),
    CSM_VOL_N   = (1 << 2),
//second byte
    CSM_PLAY    = (1 << 0),
    CSM_STOP    = (1 << 1),
    CSM_PAUSE   = (1 << 2),
    CSM_NEXT    = (1 << 3),
    CSM_PREV    = (1 << 4),
    CSM_REC     = (1 << 5),
}HID_CMD_CSM_e;

/* --------------------------------------------------- */
void usbd_log_flag_set(uint32_t flag, uint8_t en);
uint32_t usbd_log_flag_get(uint32_t flag);

void usb_io_as_state_update(void);
int usb_in_as_state_get(void);
int usb_out_as_state_get(void);

void usbd_dbg_hid_tx(void* buf, int size);
void usbd_dbg_hid_rx(void* ptr, int size);
void csm_hid_key_send(uint8_t idx, uint8_t csm_cmd);


/* --------------------------------------------------- *///api, vol index
// float usb_in_vol_dB_max_get(void);
// float usb_in_vol_dB_min_get(void);
// float usb_in_vol_dB_res_get(void);
// float usb_out_vol_dB_max_get(void);
// float usb_out_vol_dB_min_get(void);
// float usb_out_vol_dB_res_get(void);

float usb_in_vol_dB_L_get(void);
float usb_in_vol_dB_R_get(void);
float usb_out_vol_dB_L_get(void);
float usb_out_vol_dB_R_get(void);

uint8_t usb_in_vol_idx_max_get(void);
uint8_t usb_in_vol_idx_L_get(void);
uint8_t usb_in_vol_idx_R_get(void);

uint8_t usb_out_vol_idx_max_get(void);
uint8_t usb_out_vol_idx_L_get(void);
uint8_t usb_out_vol_idx_R_get(void);

/* --------------------------------------------------- *///api, usb out vol control
void usb_out_vol_mute(void);
void usb_out_vol_inc(void);
void usb_out_vol_dec(void);
void usb_out_vol_end(void);

#endif /* _AUDIO_HID_H_ */
