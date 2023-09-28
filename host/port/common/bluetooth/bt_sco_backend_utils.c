/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <jos/jos_mbuf.h>
#include "bt_sco_backend_utils.h"
#include <bt_sco.h>
#include <audio_out_interface.h>
#include "app_beken_includes.h"
#include "beken_external.h"
#include "hw_lc.h"
#include "bt_hfp_hf.h"

#define SCO_BANDWIDTH        8000
#define SCO_WIDEBANDWIDTH    16000
#define SCO_MAX_LATENCY      0xFFFF // older value = 7,esco link/2EV3 packet not success!!!  ,>=0x0A
#define SCO_SAMPLE_SIZE      HCI_PCM_SAMPLE_SIZE_16BIT

extern void set_voice_recog_status(BOOL status);
extern uint32_t hfp_select_current_app_to_another(btaddr_t *btaddr);
extern hfp_hf_app_t * hfp_find_app_by_addr(btaddr_t *btaddr);

typedef struct {
    sco_h    ep;
    jtask_h  send_out_task;
    BOOL   connected;
    void *audio_out;
} sco_utils_t;

static void sco_utils_newconn(void *app_ctx, sco_link_type_t link_type, uint16_t pkt_type)
{
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    hfp_hf_app_t *hfp_app_ptr = NULL;
    sco_params_t sco_params;
    result_t rc;
    
    LOG_I(SCO,"sco_utils_newconn\r\n");

    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_LINK_QUALITY;

    /* Currently, application imposes no restrictions on the SCO packet type.
     * The actual packet type will be negotiated between the local controller
     * and the remote entity.
     */
    sco_params.pkt_type = HCI_PKT_ESCO_ALL;

    /* 2. Voice settings */
    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;

    hfp_app_ptr = (hfp_hf_app_t *)hfp_get_app_from_priv((sco_utils_h)app_ctx);
    if(hfp_app_ptr->freq == 16000)  /* mSBC */
    {
        sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_TRANSPARENT;
    }
    else
    {
        sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
    }

    rc = sco_accept(ctx->ep, link_type, &sco_params);

    DBG_RC_I(rc, DBT_TEST, ("%s: done, %s\n", FNAME, uwe_str(rc)));
    (void)rc;
}

static void sco_utils_connected(void *app_ctx)
{
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    btaddr_t *raddr;
    hfp_hf_app_t * app_ptr;

    LOG_I(SCO,"sco_utils_connected\r\n");
    
    jtask_stop(ctx->send_out_task);
    raddr = get_pcb_raddr(ctx->ep);
    
#if (CONFIG_CPU_CLK_OPTIMIZATION == 0)
    System_Config_MCU_For_eSCO();
#endif
    
    bt_app_entity_set_work_flag_by_addr(raddr, BT_WORK_SCO_CONNECTED);

    sbc_mem_free();
    if(!hfp_has_sco())
    {
#ifdef CONFIG_APP_AEC
       app_env_rf_pwr_set(1);
       if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AEC_ENABLE))
       {
            //sbc_mem_free();
            app_aec_init(get_current_hfp_freq());
        }
#endif
    }

    set_hf_flag_1toN(raddr, APP_FLAG_SCO_CONNECTION);
    if(!get_2hfps_sco_and_incoming_flag()) hf_exchange_sco_active_flag(raddr, SCO_CONNECTED);

    check_2hfps_sco_and_incoming_status();

    if(app_bt_flag1_get(APP_FLAG_AUTO_CONNECTION|APP_FLAG_RECONNCTION)) 
    {
        app_bt_flag2_set(APP_FLAG2_RECONN_AFTER_CALL, 1);
    }

    app_ptr = hfp_find_app_by_addr(raddr);
    hfp_update_current_app(app_ptr);
	
    //if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING)) aud_PAmute_delay_operation(1);//FIXME@liaixing

    //if(hfp_has_connection())
    {
        if(!get_2hfps_sco_and_incoming_flag()
            && !app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
        {
            hf_audio_restore();
#ifdef CONFIG_APP_HALFDUPLEX
            app_hfp_echo_erase_init();
#endif
        }
		aud_PAmute_delay_operation(0);
    }

    ctx->connected = 1;

#if CONFIG_CRTL_POWER_IN_BT_SNIFF_MODE
    uint16_t handle;
    handle = bt_sniff_get_handle_from_raddr(raddr);
    app_bt_write_sniff_link_policy(handle, 0);
#endif

#if(CONFIG_AUD_FADE_IN_OUT == 1)
    if(!app_bt_flag1_get(APP_FLAG_WAVE_PLAYING))
        set_aud_fade_in_out_state(AUD_FADE_IN);
#endif
}

static void sco_utils_disconnected(void *app_ctx, result_t err)
{
    app_handle_t app_h = app_get_sys_handler();
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    //uint8_t sco_active_flag, mac_win; 
    btaddr_t *raddr;

    app_h->mic_volume_store &= ~0x80;
    app_h->is_wechat_phone_call = 0;
#if (CONFIG_CPU_CLK_OPTIMIZATION == 0)
    System_Config_MCU_Restore();
#endif

    set_voice_recog_status(FALSE);//here should set voice_recog_status FALSE.

    jtask_stop(ctx->send_out_task);

    LOG_I(SCO,"sco_utils_disconnected\r\n");

    ctx->audio_out = NULL;
    ctx->connected = 0;

    set_current_hfp_flag(APP_HFP_PRIVATE_FLAG_TWC_WAIT_STATE, 0);

    raddr = get_pcb_raddr(ctx->ep);
    //sco_active_flag = get_hfp_flag_1toN(raddr, APP_SCO_PRIVATE_FLAG_CONNECTION_ACTIVE);
    
    bt_app_entity_clear_work_flag_by_addr(raddr, BT_WORK_SCO_CONNECTED);

    hf_exchange_sco_active_flag(raddr, SCO_DISCONNECTED);
    clear_hf_flag_1toN(raddr, APP_FLAG_SCO_CONNECTION);
    set_2hfps_sco_and_incoming_flag(0);

    //mac_win = bt_app_entity_get_mac_win_book(0)|bt_app_entity_get_mac_win_book(1);

    //if((mac_win) && !btaddr_same(&get_temp_hfp_app()->raddr,raddr))
    //{
    //    hfp_select_current_app_to_another(raddr);
    //}
	
    if(!hfp_has_sco())
    {
#ifdef CONFIG_APP_AEC
        app_env_rf_pwr_set(0);
        if(app_env_check_feature_flag(APP_ENV_FEATURE_FLAG_AEC_ENABLE))
            app_aec_uninit();
#endif
    }

    if(hfp_has_sco())
    { 
        if(!bt_audio_ch_busy() /*&& sco_active_flag*/)
        {
            hfp_select_current_app_to_another(raddr);
            hf_audio_restore();
		}
    }
    else
    {
        if(!a2dp_has_music() && !bt_audio_ch_busy())
        {
            aud_volume_mute(1);
        	aud_dac_close();
        }
        else
            app_audio_restore();

        aud_mic_open(0);
    }

	if(app_check_bt_mode(BT_MODE_1V2))
        app_bt_reconn_after_callEnd();

	if(app_check_bt_mode(BT_MODE_TWS|BT_MODE_DM_TWS))
	    BK3000_Ana_Decrease_Current(FALSE);
}

static void sco_utils_complete(void *app_ctx, int32_t count, void *arg)
{
}

static void sco_utils_input(void *app_ctx, struct mbuf *m)
{
    result_t rc=0;
    uint32_t length = m_length(m);
    sco_utils_t *ctx = (sco_utils_t *)app_ctx;
    DECLARE_FNAME("sco_utils_input");

    if(app_bt_flag1_get(APP_FLAG_HFP_CALLSETUP))
        rc = audio_out_write(ctx->audio_out, mtod(m, uint8_t *), length);
    DBG_RC_I(rc, DBT_SCO, ("%s: done, %s\n", FNAME, uwe_str(rc)));
    (void)rc;
}

static scoproto_t sco_utils_cbs = {
    sco_utils_connected,
    sco_utils_disconnected,
    sco_utils_newconn,
    sco_utils_complete,
    sco_utils_input
};

void util_sco_close(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    if(!priv)
    {
        return;
    }

    if(ctx->audio_out)
    {
        audio_out_close(&ctx->audio_out);
    }

    if(ctx->ep)
    {
        sco_close(ctx->ep);
    }

    if(ctx->send_out_task)
    {
        jtask_uninit(ctx->send_out_task);
        ctx->send_out_task = NULL;
    }

    jfree(ctx);
    ctx = NULL;
}

result_t util_sco_open(const btaddr_t *laddr, const btaddr_t *raddr, sco_utils_h *priv)
{
    result_t rc;
    sco_utils_t *ctx = NULL;

    ctx = jmalloc(sizeof(sco_utils_t), M_ZERO);
    rc = jtask_init(&ctx->send_out_task, J_TASK_PNP);
    if(rc)
        goto Error;

    rc = sco_open(laddr, raddr, &sco_utils_cbs, ctx, &ctx->ep);
    if(rc)
        goto Error;

    *priv = ctx;

    return UWE_OK;

 Error:
    util_sco_close(ctx);
    return rc;
}

#ifdef BQB_PROFILE  // PTS Ĭ�ϻᷢ��SCO����
result_t util_sco_connect(sco_utils_h priv)
{
    sco_params_t sco_params;
    sco_utils_t *ctx = (sco_utils_t *)priv;
    int ret = 0;

    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_POWER_CONSUMPTION;

    sco_params.pkt_type = HCI_PKT_SCO_ALL;

    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;
    sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;

    ret = sco_connect(ctx->ep, SCO_LINK_TYPE_SCO, &sco_params);
    return ret;
}
#else
result_t util_sco_connect(sco_utils_h priv)
{
    sco_params_t sco_params;
    sco_utils_t *ctx = (sco_utils_t *)priv;
    int ret = 0;
    
    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_LINK_QUALITY;

    sco_params.pkt_type = HCI_PKT_ESCO_ALL;

    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;
#if (CONFIG_HFP17_MSBC_SUPPORTED == 1)
    /* Reference to HFP spec 1.7.0, @Page.40 */

    /********************************************
    For all HF initiated audio connection establishments for which both sides
    support the Codec Negotiation feature, the HF shall trigger the AG to establish
    a Codec Connection. This is necessary because only the AG knows about the codec
    selection and settings of the network.
    *********************************************/
    /*********************************************
    AT+BCC (Bluetooth Codec Connection)
    Syntax: AT+BCC
    Description:
    This command is used by the HF to request the AG to start the codec connection procedure.
    *********************************************/
    {
        hfp_hf_app_t *hfp_app_ptr = NULL;
        hfp_app_ptr =(hfp_hf_app_t *) hfp_get_app_from_priv((sco_utils_h)ctx);
        if(hf_check_flag(hfp_app_ptr,APP_HFP_PRIVATE_FLAG_CODEC_NEGOTIATION))
        {
            ret = bt_hfp_hf_establishment_codec(hfp_app_ptr->session);
        }
        else
        {
            sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
            ret = sco_connect(ctx->ep, SCO_LINK_TYPE_ESCO, &sco_params);
        }
    }
#else
    sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;
    ret = sco_connect(ctx->ep, SCO_LINK_TYPE_ESCO, &sco_params);
#endif
    LOG_I(SCO,"===sco connect:%d\r\n",ret);
    return ret;
}
#endif

#if PTS_TESTING
/***********************************************************
* in pts testing, just can only use [sco] type connect.
***********************************************************/
result_t util_sco_connect_pts(sco_utils_h priv)
{
    sco_params_t sco_params;
    sco_utils_t *ctx = (sco_utils_t *)priv;
    int ret = 0;

    sco_params.tx_bandwidth = SCO_BANDWIDTH;
    sco_params.rx_bandwidth = SCO_BANDWIDTH;
    sco_params.max_latency = SCO_MAX_LATENCY;
    sco_params.retransmit_effort = SCO_RETRANSMIT_EFFORT_POWER_CONSUMPTION;

    sco_params.pkt_type = HCI_PKT_SCO_ALL;

    sco_params.pcm_settings.input_coding = HCI_PCM_INPUT_CODING_LINEAR;
    sco_params.pcm_settings.input_format = HCI_PCM_INPUT_FORMAT_2COMP;
    sco_params.pcm_settings.input_sample_size = SCO_SAMPLE_SIZE;
    sco_params.pcm_settings.linear_bit_position = 0;
    sco_params.pcm_settings.air_coding_format = HCI_PCM_AIR_FORMAT_CVSD;

    ret = sco_connect(ctx->ep, SCO_LINK_TYPE_SCO, &sco_params);
    LOG_I(SCO,"util_sco_connect_pts, only [sco] type, 0x%x, %d \r\n", priv, ret);
    return ret;
}
#endif

result_t util_sco_disconnect(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    return sco_disconnect(ctx->ep, 1);
}

result_t start_sco_test_call(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    if(ctx->connected == 0)
    {
        jtask_schedule(ctx->send_out_task,
                       200,
                       (jthread_func)util_sco_connect,
                       (void *)priv);
    }

    return 0;
}

void stop_sco_test_call(sco_utils_h priv)
{
    sco_utils_t *ctx = (sco_utils_t *)priv;

    app_button_type_set(BUTTON_TYPE_NON_HFP);

    jtask_stop(ctx->send_out_task);
}
// EOF
