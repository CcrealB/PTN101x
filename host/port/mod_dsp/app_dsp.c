#include "driver_beken_includes.h"
#include "app_prompt_wav.h"
#include "dsp_include.h"
#include "app_dsp.h"
#include "u_com.h"

static MsgPoolContext *p_dsp_cmd_msg = NULL;
static RingBufferContext *p_rb_log_ctx = NULL;
static uint32_t *p_dsp_run_time = NULL;
static uint32_t *p_dsp_run_tick = NULL;

//return: 0xYYYYMMDD
uint32_t dsp_drv_ver_get(void)
{
    uint32_t dsp_drv_ver = 0;
    uint32_t dsp2mcu_ack_data[4] = {0};
    int ret = mbx_mcu2dsp_transfer(USR_MBX_CMD_SYSTEM_VER_INFO | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    if((ret == 0) && (USR_MBX_CMD_SYSTEM_VER_INFO == dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD]))
    {
        dsp_drv_ver = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    }
    return dsp_drv_ver;
}

void dsp_performance_test(void)
{
    uint32_t cnt_max = 10000;
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_DSP_PERFORMENCE | MAILBOX_CMD_FAST_RSP_FLAG, 2, cnt_max, 0, &dsp2mcu_ack_data[0]);
    // int dsp_clk_KHz = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    // MDSP_LOG_I("%s: %dKHz\n", __FUNCTION__, dsp_clk_KHz);
}

/**
 * @brief get dsp audio process run time, Used to evaluate DSP load
 * @note p_dsp_run_time unsigned int 32bit data, effet run time saved in high 16bit, all audio procees time saved in low 16bit. [DSP utilization = high 16bit / low 16bit]
 * */
static int dsp_run_time_init(void)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    int ret = mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_AUD_RUN_TIME_GET | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    if((ret == 0) && (USR_MBX_CMD_SYS_AUD_RUN_TIME_GET == dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD]))
    {
        p_dsp_run_time = (uint32_t*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        p_dsp_run_tick = (uint32_t*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
        uint32_t tick_cur = (uint32_t)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
        MDSP_LOG_I("p_time:%p, p_tick:%p, tick_cur:%d\n", p_dsp_run_time, p_dsp_run_tick, tick_cur);
        return 0;
    }
    MDSP_LOG_E("%d, cmd:%d\n", ret, dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD]);
    return -1;
}

uint32_t dsp_run_time_eft_get(void) { return p_dsp_run_time ? ((*p_dsp_run_time) >> 16) : 0; }
uint32_t dsp_run_time_all_get(void) { return p_dsp_run_time ? ((*p_dsp_run_time & 0xFFFF)) : 0; }
void dsp_run_time_clear(void) { if(p_dsp_run_time) *p_dsp_run_time = 0; }

uint32_t dsp_run_tick_all_get(void) { return p_dsp_run_tick ? (*p_dsp_run_tick) : 0; }
void dsp_run_tick_clear(void) { if(p_dsp_run_tick) *p_dsp_run_tick = 0; }

/**
 * @param log_sel global log ctrl: 0:off 1:com, 2:uart0, 3:uart1, 4:uart2, 5:share rb(default)
 * @param loop_log_en loop debug log enable : 0:off 1:on
*/
void dsp_log_ctrl(int log_sel, int loop_log_en)
{
    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_DSP_LOG_CTRL | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)log_sel, (uint32_t)loop_log_en, 0, NULL);
}

/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
void app_dsp_cmd_msg_init(void)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_COM_CMD_MSG_INFO_SYNC | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);//NULL, 0, 0
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    if(ack_cmd == USR_MBX_CMD_COM_CMD_MSG_INFO_SYNC){
        p_dsp_cmd_msg = (MsgPoolContext*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        MDSP_LOG_I("%s, msg_ctx:0x%X, buf:0x%X, cnt:%u\n", __FUNCTION__, 
            (uint32_t)p_dsp_cmd_msg, p_dsp_cmd_msg->addr, p_dsp_cmd_msg->count);
    }else{
        MDSP_LOG_E("err ack_cmd:0x%X\n", ack_cmd);
    }
}

//cmd send to dsp by share ringbuffer context, 
int app_dsp_cmd_msg_push(MailBoxCmd *p_mbx)
{
    if(p_dsp_cmd_msg == NULL)
    {
        MDSP_LOG_E("p_dsp_cmd_msg is NULL:0x%p\n", p_dsp_cmd_msg);
        return -1;
    }
    if(msg_pool_get_free_msgs(p_dsp_cmd_msg))
    {
        msg_pool_inqueue(p_dsp_cmd_msg, p_mbx);
    }
    return msg_pool_get_free_msgs(p_dsp_cmd_msg);
}

/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */

//get dsp log ringbuf context addr, the addt can be set in the later(print log and reinit to other share mem addr)
static void app_dsp_log_init(void)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_COM_LOG_INFO_SYNC | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    if(ack_cmd == USR_MBX_CMD_COM_LOG_INFO_SYNC){
        p_rb_log_ctx = (RingBufferContext*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        MDSP_LOG_I("%s, rb:0x%X, buf:0x%X, size:%u\n", __FUNCTION__, 
            (uint32_t)p_rb_log_ctx, p_rb_log_ctx->address, p_rb_log_ctx->capacity);
    }else{
        MDSP_LOG_E("err ack_cmd:0x%X\n", ack_cmd);
    }
}

#if 0
void app_dsp_log_reinit(RingBufferContext *dsp_log_rb_st_new)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_COM_LOG_INFO_SYNC, (uint32_t)dsp_log_rb_st_new, 0, 0, &dsp2mcu_ack_data[0]);
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    uint32_t ack_rb = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    if(ack_cmd == USR_MBX_CMD_COM_LOG_INFO_SYNC && ack_rb == (uint32_t)dsp_log_rb_st_new){
        p_rb_log_ctx = ack_rb;
        MDSP_LOG_I("%s, rb:0x%X, buf:0x%X, size:%u\n", __FUNCTION__, 
            (uint32_t)p_rb_log_ctx, p_rb_log_ctx->address, p_rb_log_ctx->capacity);
    }else{
        MDSP_LOG_E("err ack_cmd:0x%X\n", ack_cmd);
    }
}
#endif

//return: log size printed
int app_dsp_log_proc(uint8_t log_en)
{
    int log_fill, log_out_sz = 0;

    if(p_rb_log_ctx == NULL)
    {
        MDSP_LOG_E("p_rb_log_ctx is NULL\n");
        goto RET_ERR;
    }

    log_fill = ring_buffer_get_fill_size(p_rb_log_ctx);
    if(log_fill > 0)
    {
        uint8_t buff[256];
        if(log_fill >= 128){
            uint8_t fifo_free = REG_UART0_FIFO_STATUS & 0xFF;//get data num in TX fifo
            log_out_sz = (log_fill > fifo_free) ? fifo_free : log_fill;
        }else{
            log_out_sz = log_fill;
        }
        ring_buffer_read(p_rb_log_ctx, (uint8_t*)buff, log_out_sz);
        // com_cmd_send_proc(buff, log_out_sz);
        if(log_en) dsp_log_out(buff, log_out_sz);
    }
// RET:
    return log_out_sz;
RET_ERR:
    return 0;
}


//as_sel: 1:m2d, 2:usb out, 3:usb in
void* app_dsp_spc_h_get(char* str, uint32_t as_sel)
{
    void *aud_spc_h = NULL;
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_SYNC_CTX_PTR | MAILBOX_CMD_FAST_RSP_FLAG, as_sel, (uint32_t)NULL, 0, &dsp2mcu_ack_data[0]);//1for m2d ch
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    uint32_t ack_as = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    void* ack_spc_h = (void*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    if(ack_cmd == USR_MBX_CMD_AUDIO_SYNC_CTX_PTR && (ack_as == as_sel) && (ack_spc_h != NULL)){
        aud_spc_h = (void*)ack_spc_h;
        MDSP_LOG_I("%s:%d, aud_spc_h:0x%08X\n", str, as_sel, aud_spc_h);
    }else{
        MDSP_LOG_E("%s: err ack_cmd:%d, ack_as:%d, ack_spc_h:0x%p\n", str, ack_cmd, ack_as, ack_spc_h);
    }
    return aud_spc_h;
}


/** @brief audio stream from mcu to dsp trans mode config. add @ 230317
 * @param as_id  m2d: 0x01:def play, 0x02:usb out, 0x03:wav;  d2m: 0x81:rec, 0x82:usb in
 * share_mem_rb_en: audio data trans by (1:share ringbuffer struct)/0:(mailbox intr in dsp)
 * */
void app_dsp_as_trans_mode_cfg(char* str, int as_id, RingBufferContext* p_rb_ctx)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AS_TRANS_MODE_CONFIG | MAILBOX_CMD_FAST_RSP_FLAG, as_id, (uint32_t)p_rb_ctx, 0, &dsp2mcu_ack_data[0]);
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    uint32_t ack_as_id = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
    void* ack_rb_ctx = (void*)dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
    if(ack_cmd == USR_MBX_CMD_AS_TRANS_MODE_CONFIG && (ack_as_id == as_id) && (ack_rb_ctx != NULL)){
        p_rb_ctx = (RingBufferContext*)ack_rb_ctx;
        MDSP_LOG_I("%s:0x%02X, rb:0x%X, buf:0x%X, size:%u\n", str, as_id, (uint32_t)p_rb_ctx, p_rb_ctx->address, p_rb_ctx->capacity);
    }else{
        MDSP_LOG_E("%s:0x%02X, err ack cmd:%d, as_id:0x%d, p_rb_ctx:0x%p\n", str, ack_cmd, ack_as_id, ack_rb_ctx);
    }
}

/** @brief check if dsp is ready(is waiting mcu trig)
 * dsp side: mailbox_dsp2mcu_resp(cmd, system_flag_get(SYS_FLAG_ALL_MASK), g_audio_init_cmp_fg, dsp_systick_value());
 * */
static int dsp_is_wait(void)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_INIT_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    if(ack_cmd == USR_MBX_CMD_AUDIO_INIT_CHECK){
        uint32_t dsp_system_flag = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        uint32_t dsp_aud_init_fg = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
        uint32_t dsp_systick_val = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
        MDSP_LOG_I("[%d]dsp stat, fg:0x%X, aud_ini:0x%X, tick:%u\n", __LINE__, dsp_system_flag, dsp_aud_init_fg, dsp_systick_val);
        return (dsp_system_flag & SYS_FLAG_WAIT_MCU_TRIG);
    }else{
        MDSP_LOG_E("err ack_cmd:0x%X\n", ack_cmd);
        return 0;
    }
}

//check if dsp effect is init ok
static int dsp_eft_init_ok_chk(void)
{
    uint32_t dsp2mcu_ack_data[4] = {0};
    mbx_mcu2dsp_transfer(USR_MBX_CMD_AUDIO_INIT_CHECK | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, &dsp2mcu_ack_data[0]);
    uint32_t ack_cmd = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_CMD];
    if(ack_cmd == USR_MBX_CMD_AUDIO_INIT_CHECK){
        uint32_t dsp_system_flag = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM0];
        uint32_t dsp_aud_init_fg = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM1];
        uint32_t dsp_systick_val = dsp2mcu_ack_data[MAILBOX_CMD_REG_IDX_PARAM2];
        MDSP_LOG_I("[%d]dsp stat, fg:0x%X, aud_ini:0x%X, tick:%u\n", __LINE__, dsp_system_flag, dsp_aud_init_fg, dsp_systick_val);
        return (dsp_system_flag & SYS_FLAG_SYSTEM_START);
    }else{
        MDSP_LOG_E("err ack_cmd:0x%X\n", ack_cmd);
        return 0;
    }
}

/* ******************************************************************** */
/* ******************************************************************** */
/* ******************************************************************** */
/**
 * @note step:
 * 1、dsp main init and wait mcu trig
 * 2、mcu trig by USR_MBX_CMD_SYS_DSP_MERGE_ADDR
 * 3、dsp audio init
 * ...
*/
void app_dsp_init(void)
{
    MDSP_LOG_I("app_dsp_init enter @ %dms, dsp_merge_addr:0x%X\n", sys_time_get(), (*((volatile unsigned int *)0x00000004)));
    int cnt = 1000; while(!dsp_is_wait() && (--cnt)) { sys_delay_us(100); }
    app_dsp_log_init();
    app_dsp_cmd_msg_init();
#ifdef ASO_DEF_TRANS_BY_SHARE_RB
    extern RingBufferContext def_aso_ringbuf;
    app_dsp_as_trans_mode_cfg("share_rb_def_aso", 0x01, &def_aso_ringbuf);
#endif
#ifdef ASI_DEF_TRANS_BY_SHARE_RB
    extern RingBufferContext def_asi_ringbuf;
    app_dsp_as_trans_mode_cfg("share_rb_def_asi", 0x81, &def_asi_ringbuf);
#endif
#ifdef ASO_USB_TRANS_BY_SHARE_RB
    extern RingBufferContext usb_aso_rb;
    app_dsp_as_trans_mode_cfg("share_rb_as_usbo", 0x02, &usb_aso_rb);
#endif
#ifdef ASI_USB_TRANS_BY_SHARE_RB
    extern RingBufferContext usb_asi_rb;
    app_dsp_as_trans_mode_cfg("share_rb_as_usbi", 0x82, &usb_asi_rb);
#endif
#if (BT_AUD_SYNC_ADJ_BY_SW > 0)
    audio_sync_sys_init();
#endif
    if(dsp_run_time_init() != 0) while(1);
    MDSP_LOG_I("dsp driver version:0x%X\n", dsp_drv_ver_get());
    extern void dsp_audio_init(void);
    dsp_audio_init();
    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_DSP_LIGHT_SLEEP_EN | MAILBOX_CMD_FAST_RSP_FLAG, 0, 0, 0, NULL);//enable, invalid, invalid
    //mcu trig 
    mbx_mcu2dsp_transfer(USR_MBX_CMD_SYS_DSP_MERGE_ADDR | MAILBOX_CMD_FAST_RSP_FLAG, (uint32_t)(*((volatile unsigned int *)0x00000004)), 0, 0, NULL);
    cnt = 50; while(!dsp_eft_init_ok_chk() && (cnt--)) { sys_delay_us(2000); }
    MDSP_LOG_I("app_dsp_init exit @%d ms\n", sys_time_get());
    while(app_dsp_log_proc(1));//read log fifo to empty
}

#ifdef DSP_EXCEPTION_CHECK
void dsp_exception_check_task(int loop_ms)
{
    static uint32_t t_mark = 0;
    // loop_ms = 10;//uncommit for debug
    if(sys_timeout(t_mark, loop_ms))// shoule 5~30s
    {
        t_mark = sys_time_get();
        if(dsp_is_working())
        {
            if(t_mark < 10000) return; //detect @ 10s after system power up
            if(dsp_run_time_all_get() == 0)
            {
                MDSP_LOG_W(" !!!!!!!!\n");
                extern void sys_debug_show(uint8_t redirect);
                sys_debug_show(0);
                uint32_t dsp_drv_ver = dsp_drv_ver_get();
                if(dsp_drv_ver == 0){
                    MDSP_LOG_E(" dsp_exception, ver:0x%X !!!!!!!!\n", dsp_drv_ver_get());
                    sys_delay_ms(10);
                    dsp_exception_callback();
                }
            }
            dsp_run_time_clear();
        }
    }
}

__attribute__((weak)) void dsp_exception_callback(void)
{
    #if 1
    extern void PlayWaveStop(uint16_t id);
    PlayWaveStop(APP_WAVE_FILE_ID_POWEROFF);
    #else
    //way 1
    watch_dog_start(30000);//30sec
    while(1);

    // // way 2
    // if(app_wave_playing()){
    //     app_wave_file_play_start(APP_WAVE_FILE_ID_POWEROFF);
    //     // app_wave_file_play_stop();
    // }
    // void app_powerdown(void);
    // app_powerdown();

    // //way 3
    // dsp_shutdown();
    // REG_SYSTEM_0x1D &= ~0x2;
    // system_wdt_reset_ana_dig(1, 1);
    // BK3000_wdt_reset();
    #endif
}
#endif
