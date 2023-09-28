#include <stdio.h>
#include <stddef.h>     // standard definition
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "sys_irq.h"
//#include "config.h"
//#include "app_beken_includes.h"
#include "..\beken_driver\bkreg.h"
#include "..\..\usb\usb.h"
#include "..\..\drv_usb.h"
#include "usb\usb_include.h"
#include "uw_errno.h"
#include "bkreg.h"
#include "msg_pub.h"
#include "..\..\..\beken_driver\drv_system.h"
#include "config.h"

#include "driver_audio.h"
#include "utils_audio.h"
#include "audio_hid.h"
#include "audio_sync_usb.h"
#if (CONFIG_AUD_SYNC_USBO == 1) || (CONFIG_AUD_SYNC_USBI == 1)
#include "audio_spc.h"
#endif
#ifdef USB_AUDIO_TASK_SWI
#include "timer.h"
#endif

extern int uac_get_featrue_unit(USBD_SetupReqTypedef *setup, uint8_t *pbuf, int *size);
extern int uac_set_feature_unit(USBD_SetupReqTypedef *setup, uint8_t *pbuf, int size);
extern void usb_in_volume_proc(int16_t *pcmi, int16_t *pcmo, int samples);
extern void usb_out_volume_proc(int16_t *pcmi, int16_t *pcmo, int samples);

static AUD_WAV_FORMAT_t wav_fmt_usbo;

/* ---------------------------------------------------------------------- */
#ifdef USB_DEV_DEBUG
static uint32_t g_usbd_log_flag = -1;
#else
static uint32_t g_usbd_log_flag = 0;
#endif
void usbd_log_flag_set(uint32_t flag, uint8_t en) { g_usbd_log_flag = en ? (g_usbd_log_flag | flag) : (g_usbd_log_flag & ~flag); }
uint32_t usbd_log_flag_get(uint32_t flag) { return (g_usbd_log_flag & flag); }

/* ---------------------------------------------------------------------- *///csm hid
/** 
 * @param idx
 * @param csm_cmd refer to HID_CMD_CSM_e
 * */
void csm_hid_key_send(uint8_t idx, uint8_t csm_cmd)
{
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    buff[idx] = csm_cmd;

    uint32_t interrupts_info, mask;
    SYSirq_Disable_Interrupts_Save_Flags(&interrupts_info, &mask);
    AplUsb_StartTx(((_t_drv_usb_device*)get_usb_dev_handle())->bp,USB_ENDPID_Hid_MSE, &buff[0], sizeof(buff)); 
    SYSirq_Interrupts_Restore_Flags(interrupts_info, mask);
}

/* ---------------------------------------------------------------------- *///dbg hid
void usbd_dbg_hid_tx(void* buf, int size)
{
    AplUsb_StartTx(((_t_drv_usb_device*)get_usb_dev_handle())->bp,USB_ENDPID_Hid_DBG_IN, buf, size);
}

__attribute__((weak))
void usbd_dbg_hid_rx(void* ptr, int size)
{
    uint8_t* buf = (uint8_t*)ptr;
    USBD_LOG_I("dbg hid rcv %d data:\n", size);
    for(int i = 0; i < size; i++) { std_printf("%02X ", buf[i]); }
    std_printf("\n");
    usbd_dbg_hid_tx(ptr, size);
}

/* ---------------------------------------------------------------------- *///usb audio
#if USB_AUDIO_FUNC_VALID

#if USB_AUDIO_STREAM_STATE_VALID//usb audio stream state process

static int s_usb_out_as_stat_updt = 0;
static int s_usb_out_as_state = 0;
static int s_usb_in_as_stat_updt = 0;
static int s_usb_in_as_state = 0;

#define USBO_IF_TRIG_BY_AS      0
#if USBO_IF_TRIG_BY_AS
jtask_h usb_spk_close_cbk = NULL;
void usb_aso_action(void* arg)
{
    int usb_aso_run = *((int*)arg);
    LOG_I(USB, "usb_aso_action:%d, %d.\n", usb_aso_run, audio_aso_type_get(ASO_TYPE_USB));
    if((!usb_aso_run) && audio_aso_type_get(ASO_TYPE_USB))
    {
        msg_put(MSG_USB_SPK_CLOSE);
    }
    else if(usb_aso_run && (!audio_aso_type_get(ASO_TYPE_USB)))
    {
        msg_put(MSG_USB_SPK_OPEN);
    }
    jtask_uninit(usb_spk_close_cbk);
    usb_spk_close_cbk = NULL;
}
#endif

//place in main loop(10ms/100ms)
void usb_io_as_state_update(void)
{
    if(s_usb_out_as_state != s_usb_out_as_stat_updt)
    {
        s_usb_out_as_state = s_usb_out_as_stat_updt;
        #if USBO_IF_TRIG_BY_AS
        if(usb_spk_close_cbk == NULL) jtask_init(&usb_spk_close_cbk, J_TASK_TIMEOUT);
        jtask_schedule(usb_spk_close_cbk, 500, usb_aso_action, &s_usb_out_as_state);
        #endif
    }
    s_usb_out_as_stat_updt = 0;
    s_usb_in_as_state = s_usb_in_as_stat_updt ? 1 : 0;
    s_usb_in_as_stat_updt = 0;
}

int usb_in_as_state_get(void){ return s_usb_in_as_state; }
int usb_out_as_state_get(void){ return s_usb_out_as_state; }
#else
void usb_io_as_state_update(void) { }
int usb_in_as_state_get(void) { }
int usb_out_as_state_get(void) { }
#endif


/**
 * @brief audio_usb_out_proc, USB OUT audio stream
 * */
void audio_usb_out_proc(void*ptr, int size)
{
#if USB_AUDIO_STREAM_STATE_VALID
    s_usb_out_as_stat_updt = 1;
#endif

    if(!audio_aso_type_get(ASO_TYPE_USB)) return;

#if CONFIG_AUD_SYNC_USBO
    int16_t* pcm_in = (int16_t*)ptr;
    int samples = USBO_Byte2Smp(size);
    int32_t pcm_out[(48 + 5) * 2];//48*2ch
    int fill_smps = USBO_Byte2Smp(usb_out_ringbuff_fill_get());
    int comp_num = usbo_aud_sync_proc(samples, fill_smps);

    #if CONFIG_AUD_SYNC_USBO == 1
    samples = audio_spc_exec(pcm_in, (int16_t*)pcm_out, samples, 16, 2, comp_num, 48);
    // if(comp_num) std_printf("%c", comp_num > 0 ? '+' : '-');
    #else
    memcpy(pcm_out, pcm_in, size); UNUSED(comp_num);
    #endif

    int free_smps = USBO_Byte2Smp(usb_out_ringbuff_free_get());
    if(free_smps >= samples){
        usb_out_volume_proc((int16_t*)pcm_out, (int16_t*)pcm_out, samples);
        usb_out_ringbuff_write((uint8_t*)pcm_out, USBO_Smp2Byte(samples));
    }else{
        USBD_LOG_W("usbo loss, free:%d < %d\n", free_smps, samples);
        usbo_aud_sync_reset();
        usb_out_ringbuff_clear();
        uint8_t pcm[AUDIO_USBO_BUF_SIZE / 2];
        memset(pcm, 0, sizeof(pcm));
        usb_out_ringbuff_write((uint8_t*)pcm, AUDIO_USBO_BUF_SIZE / 2);
    }
#else
    usb_out_ringbuff_write((uint8_t*)ptr, size);
#endif
}


/**
 * @brief audio_usb_in_proc, USB IN audio stream
 * */
void audio_usb_in_proc(void*ptr, int size)
{
    #define FRAME_SAMPLES       (AUDIO_MIC_SAMPRATE / 1000)

    int16_t pcm[(FRAME_SAMPLES + 5) * 2];
    uint16_t byte_per_smp = AUDIO_MIC_BR / 8;//1 channel
    int samples = FRAME_SAMPLES;

#if USB_AUDIO_STREAM_STATE_VALID
    s_usb_in_as_stat_updt = 1;
#endif
    if(!audio_asi_type_get(ASI_TYPE_USB)) return;

#if CONFIG_AUD_SYNC_USBI
    int fill_smps = USBI_Byte2Smp(usb_in_ringbuff_fill_get());
    // int free_smps = USBI_Byte2Smp(AUDIO_USBI_BUF_SIZE - 4) - fill_smps;
    int comp_num = usbi_aud_sync_proc(samples, fill_smps);
    samples -= comp_num;
    if(fill_smps >= samples) {
        usb_in_ringbuff_read((uint8_t*)pcm, USBI_Smp2Byte(samples));
    }else{
        // usbi_aud_sync_reset();
        USBD_LOG_W("usbi loss, fill:%d < %d\n", fill_smps, samples);
    }

    #if (AUDIO_MIC_CHNs == 1)
    for(int i = 0; i < samples; i++){ pcm[i] = pcm[2*i] + pcm[2*i+1]; }
    #endif

    #if CONFIG_AUD_SYNC_USBI == 1
    samples = audio_spc_exec(pcm, (int16_t*)pcm, samples, 16, 2, comp_num, 48);
    #else
    UNUSED(comp_num);
    #endif
#else
    usb_in_ringbuff_read((uint8_t*)pcm, samples * 2 * byte_per_smp);
    #if (AUDIO_MIC_CHNs == 1)
    for(int i = 0; i < samples; i++){ pcm[i] = pcm[2*i] + pcm[2*i+1]; }
    #endif
#endif
    usb_in_volume_proc((int16_t*)pcm, (int16_t*)pcm, samples);
    AplUsb_StartTx(((_t_drv_usb_device*)get_usb_dev_handle())->bp, USB_ENDPID_Audio_MIC, pcm, samples * AUDIO_MIC_CHNs * byte_per_smp);
}

#ifdef USB_AUDIO_TASK_SWI
static void* s_usbo_buff = NULL;
static int s_usbo_size = 0;
static void* s_usbi_buff = NULL;
static int s_usbi_size = 0;

void audio_usb_out_proc_v2(void*ptr, int size)
{
    s_usbo_buff = ptr;
    s_usbo_size = size;
    audio_swi_flag_set(SWI_FLAG_USB_OUT, 1);
}

void audio_usb_in_proc_v2(void*ptr, int size)
{
    s_usbi_buff = ptr;
    s_usbi_size = size;
    audio_swi_flag_set(SWI_FLAG_USB_IN, 1);
}

void app_usb_audio_proc(void)
{
    if(audio_swi_flag_get(SWI_FLAG_USB_OUT))
    {
        audio_swi_flag_set(SWI_FLAG_USB_OUT, 0);
	    // s_usbo_buff = (void*)AplUsb_GetRxBuf(USB_ENDPID_Audio_SPK);
        audio_usb_out_proc(s_usbo_buff, s_usbo_size);
    }
    if(audio_swi_flag_get(SWI_FLAG_USB_IN))
    {
        audio_swi_flag_set(SWI_FLAG_USB_IN, 0);
	    // s_usbi_buff = (void*)AplUsb_GetRxBuf(USB_ENDPID_Audio_MIC);
        audio_usb_in_proc(s_usbi_buff, s_usbi_size);
    }
}
#endif

void spk_set_interface(int idx,int altIntf)
{
    if(idx!=USB_INTFID_AudioAS_SPK) { return; }

    if(altIntf)
    {
        msg_put(MSG_USB_SPK_OPEN);
    }
    else
    {
        msg_put(MSG_USB_SPK_CLOSE);
    }
    USBD_LOG_I("spk_set:%d\n", altIntf);
}

void mic_set_interface(int idx, int altIntf)
{
    if (idx != USB_INTFID_AudioAS_MIC) { return; }

    if(altIntf)
    {
        msg_put(MSG_USB_MIC_OPEN);
    }
    else
    {
        msg_put(MSG_USB_MIC_CLOSE);
    }
    USBD_LOG_I("mic_set:%d\n", altIntf);
}
#else
void audio_usb_out_proc(void*ptr, int size){}
void audio_usb_in_proc(void*ptr, int size){}
void spk_set_interface(int idx,int altIntf){}
void mic_set_interface(int idx, int altIntf){}
#endif

void uac_usb_out_open(uint8_t en)
{
    if(en){
        audio_aso_config(AUDIO_SPK_SAMPRATE, AUDIO_SPK_CHNs, AUDIO_SPK_BR, ASO_TYPE_USB);
        audio_aso_open(ASO_TYPE_USB);
    }else{
        audio_aso_close(ASO_TYPE_USB);
    }
}

void uac_usb_in_open(uint8_t en)
{
    extern void HwUsb_EnableTxIE(void*bp, int endpn, int en_dis);
    _t_drv_usb_device* bp = ((_t_drv_usb_device*)get_usb_dev_handle())->bp;
    if(en){
        audio_asi_config(AUDIO_MIC_SAMPRATE, AUDIO_MIC_CHNs, AUDIO_MIC_BR, ASI_TYPE_USB);
        audio_asi_open(1, ASI_TYPE_USB);
        uint16_t buf[AUDIO_MIC_SAMPRATE/1000];
        memset(buf, 0, sizeof(buf));
        HwUsb_EnableTxIE(bp, USB_INTFID_AudioAS_MIC, 1);
        AplUsb_StartTx(((_t_drv_usb_device*)get_usb_dev_handle())->bp,USB_ENDPID_Audio_MIC, buf, sizeof(buf));
    }else{
        audio_asi_open(0, ASI_TYPE_USB);
        HwUsb_EnableTxIE(bp, USB_INTFID_AudioAS_MIC, 0);
    }
}

extern void usb0dev_mod_enable(int en_dis);
extern void usb0_mod_ie_enable(int en_dis);
static void usb_join_aud_hid()
{
    USBD_LOG_I("## %s() \n", __FUNCTION__);
	usb_init(USBDEV_BASE_PTR,usb0dev_mod_enable,usb0_mod_ie_enable);
    AplUsb_SetRxCbk(USB_ENDPID_Hid_DBG_OUT, (void *)usbd_dbg_hid_rx);
#ifndef USB_AUDIO_TASK_SWI
    AplUsb_SetTxCbk(USB_ENDPID_Audio_MIC, (void *)audio_usb_in_proc);
    AplUsb_SetRxCbk(USB_ENDPID_Audio_SPK, (void *)audio_usb_out_proc);
#else
    AplUsb_SetTxCbk(USB_ENDPID_Audio_MIC, (void *)audio_usb_in_proc_v2);
    AplUsb_SetRxCbk(USB_ENDPID_Audio_SPK, (void *)audio_usb_out_proc_v2);
#endif
    AplUsb_SetInterfaceCbk(USB_INTFID_AudioAS_SPK, spk_set_interface);
    AplUsb_SetInterfaceCbk(USB_INTFID_AudioAS_MIC, mic_set_interface);
    // audio init
    aud_wav_fmt_init(&wav_fmt_usbo, AUDIO_SPK_SAMPRATE, AUDIO_SPK_CHNs, AUDIO_SPK_BR);
    #if CONFIG_AUD_SYNC_USBO
    usbo_aud_sync_init(); 
    #endif
    #if CONFIG_AUD_SYNC_USBI
    usbi_aud_sync_init();
    #endif
}

static void usb_disconn_aud_hid()
{
    USBD_LOG_I("## %s() \n", __FUNCTION__);
	usb_deinit(USBDEV_BASE_PTR,usb0dev_mod_enable,usb0_mod_ie_enable);
}

extern void*aud_hid_GetDeviceDesc(void);
extern void*aud_hid_GetConfigDesc(void);
extern void*aud_hid_GetStringDesc(int idx);
extern void*aud_hid_GetHidRptDesc(int idx);

static int is_audio_hid_class_cmd(void*setup_pkt,int sz)
{
    USBD_SetupReqTypedef* setup = setup_pkt;
    // usb_dbg_setup_show(__func__, setup_pkt, sz);
    if((setup->bmRequest & 0x7F) != 0x21) goto RET;

    if((MUSB_MSB(setup->wIndex) == USBAUDIO_FU_SPK)
    || (MUSB_MSB(setup->wIndex) == USBAUDIO_FU_MIC))
    {
        return 1;
    }
RET:
	return 0;
}

//Dev->Host, Get
static void*get_audio_hid_class_in_db(void*setup_pkt,int sz)
{
    int ret = 0;
    extern void *AplUsbDev_GetDrv();
    CBufferBaseDesc *pdb = &((_t_usb_dev_drv *)AplUsbDev_GetDrv())->pipIntf[0].txBuf;

    ret = uac_get_featrue_unit((USBD_SetupReqTypedef*)setup_pkt, pdb->ptr, &pdb->sz);

    return ((ret == 0) ? pdb : NULL);
}

//Host->Dev, Set
static void*get_audio_hid_class_out_db(void*setup_pkt,int sz)
{
    USBD_SetupReqTypedef* setup = setup_pkt;
    usb_dbg_setup_show(__func__, setup, sz);
    extern void *AplUsbDev_GetDrv();
    CBufferBaseDesc *pdb = &((_t_usb_dev_drv *)AplUsbDev_GetDrv())->pipIntf[0].rxBuf;
    pdb->sz = setup->wLength;
    return pdb;
}

const _t_drv_usb_device cDrvAudio_hid={
	USBDEV_BASE_PTR,
	usb_join_aud_hid,
	usb_disconn_aud_hid,

	aud_hid_GetDeviceDesc,
	aud_hid_GetConfigDesc,
	aud_hid_GetStringDesc,
	aud_hid_GetHidRptDesc,

	is_audio_hid_class_cmd,
	get_audio_hid_class_in_db,
	get_audio_hid_class_out_db,

	NULL,
	NULL,
	NULL
};



