/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <bt_api.h>
#include <bt_a2dp_src.h>
#include <audio_out_interface.h>
#include "bt_app_internal.h"
#include "driver_beken_includes.h"
#include "app_beken_includes.h"
#include "beken_external.h"

#if A2DP_ROLE_SOURCE_CODE
#include "bt_sdp.h"
#include "bt_sdp_types.h"
#endif

#if A2DP_ROLE_SOURCE_CODE

extern void a2dp_codec_dump(const bt_a2dp_codec_t *codec);
extern result_t btA2dpSrcConnReconfig(a2dp_session_t *session, bt_a2dp_codec_t *codec);
extern result_t a2dpSrcStreamStart(a2dp_session_t *session);
extern result_t btA2dpSrcConnect(a2dp_session_t *session,
    const btaddr_t *laddr, const btaddr_t *raddr, bt_a2dp_endpoint_desc_t *ep);
extern result_t btA2dpSrcRegister(const bt_a2dp_src_cbs_t *cbs);
extern void btA2dpSrcUnregister(void);
extern result_t btA2dpSrcConnCreate(a2dp_session_t **handle,
    const bt_a2dp_src_conn_cbs_t *conn_cbs, bt_app_ctx_h app_ctx);
extern void btA2dpSrcConnDestroy(a2dp_session_t **handle);
extern result_t btA2dpSrcSetConfigurationRej(a2dp_session_t *session,
    int32_t local_ep_id, int32_t remote_ep_id);
extern result_t btA2dpSrcSetConfigurationRsp(a2dp_session_t *session,
    uint8_t method, int32_t local_ep_id,
    int32_t remote_ep_id);
extern result_t btA2dpSrcServerStart(a2dp_session_t **handle,
    const btaddr_t *laddr, bt_a2dp_src_newconn_cb_t newconn_cb,
    bt_app_ctx_h app_ctx);
extern void btA2dpSrcServerStop(a2dp_session_t **server);
extern result_t btA2dpSrcGetDefaultSbc(bt_a2dp_sbc_t *sbc);
extern result_t btA2dpSrcConnAccept(a2dp_session_t *session,
    bt_conn_req_h conn_req, bt_link_modes_mask_t modes,
    bt_a2dp_endpoint_desc_t *ep);
extern void btA2dpSrcConnReject(bt_conn_req_h conn_req);
extern result_t a2dpSrcStreamSend( a2dp_session_t *session, struct mbuf *m );
extern void appBtStereoProfileConnWrap(void);
extern result_t a2dpStreamSuspend(a2dp_session_t *session);
extern uint32_t getSrcL2capMbufNumDev(a2dp_session_t *session);

static sdp_a2dp_profile_t a2dpSrcProfile = {
    BT_SVC_TYPE_AUDIO_SOURCE,       /* Sink or Source */
    SDP_A2DP_PLAYER,				/* Supported features */
    "Audio Source"                  /* Service name */
};

static a2dp_backend_t gA2dpSrc;

bt_a2dp_codec_t *a2dpGetCodec(void)
{
    return &gA2dpSrc.codec;
}

inline void setFlag_APP_FLAG_A2DP_CONNECTION(void) {
    app_handle_t app_h = app_get_sys_handler();
    app_h->flag_sm1|=APP_FLAG_A2DP_CONNECTION;
}

inline void unsetFlag_APP_FLAG_A2DP_CONNECTION(void) {
    app_handle_t app_h = app_get_sys_handler();
    app_h->flag_sm1 &=(~APP_FLAG_A2DP_CONNECTION);
}

inline BOOL isSetFlag_APP_FLAG_A2DP_CONNECTION(void) {
    app_handle_t app_h = app_get_sys_handler();
    return app_h->flag_sm1 & APP_FLAG_A2DP_CONNECTION;
}

BOOL isA2dpSrcConnected(void) {
    //os_printf("isA2dpSrcConnected %d\r\n", isSetFlag_APP_FLAG_A2DP_CONNECTION());
    return isSetFlag_APP_FLAG_A2DP_CONNECTION();
}

BOOL isA2dpSrcWorking(void) {
    return isA2dpSrcConnected() && app_bt_flag2_get(APP_FLAG2_STEREO_WORK_MODE);
}

static void a2dpSrcConnected(bt_a2dp_sink_session_h session_h, bt_app_ctx_h app_ctx)
{
    os_printf("a2dpSrcConnected\r\n");

    appBtStereoProfileConnWrap();
    
}

static void a2dpSrcDisconnected(bt_a2dp_sink_session_h session_h,
    result_t status, bt_app_ctx_h app_ctx)
{
    os_printf("a2dpSrcDisconnected\r\n");
    gA2dpSrc.is_used = 0;
    unsetFlag_APP_FLAG_A2DP_CONNECTION();
}

static const bt_a2dp_src_conn_cbs_t connCbs = {
    a2dpSrcConnected,
    a2dpSrcDisconnected,
};

/***************************************************************
* The function getA2dpSrcRate() just only can be used for 
* [linein-adc-sampling] and [sbc-encode] in a2dp-src scene.
****************************************************************/
static uint32_t sA2dpSrcRate = 48000;//default use 48k rate.

void setA2dpSrcRate(uint32_t a2dpSrcRate) {
    sA2dpSrcRate = a2dpSrcRate;
}

uint32_t getA2dpSrcRate(void) {
    return sA2dpSrcRate;
}

static uint32_t a2dpSbcRate(const bt_a2dp_codec_t *codec)
{
    const bt_a2dp_sbc_t *sbc = &codec->u.sbc;

    if (sbc->octet0 & SBC_FREQ_48K)
        return 48000;
    else if (sbc->octet0 & SBC_FREQ_44K)
        return 44100;
    else if (sbc->octet0 & SBC_FREQ_32K)
        return 32000;
    else if (sbc->octet0 & SBC_FREQ_16K)
        return 16000;
    else
        return 0;
}

static uint32_t a2dpSbcBps(const bt_a2dp_codec_t *codec)
{
    const bt_a2dp_sbc_t *sbc = &codec->u.sbc;

    if (sbc->octet1 & SBC_BLOCK_LEN_4)
        return 4;
    else if (sbc->octet1 & SBC_BLOCK_LEN_8)
        return 8;
    else if (sbc->octet1 & SBC_BLOCK_LEN_12)
        return 12;
    else if (sbc->octet1 & SBC_BLOCK_LEN_16)
        return 16;
    else
        return 0;
}

static uint32_t a2dpSbcChannels(const bt_a2dp_codec_t *codec)
{
    return (codec->u.sbc.octet0 & SBC_CHANNEL_MONO ? 1 : 2);
}

static void a2dpSrcSetConfigurationCb(bt_a2dp_src_session_h session_h,
    const bt_a2dp_codec_t *codec, int32_t local_ep_id, int32_t remote_ep_id,
    bt_app_ctx_h app_ctx_h)
{
    result_t err;
    uint32_t rate, channels, bps;

    a2dp_codec_dump(codec);
    
    LOG_I(A2DP,"IB(%s)\r\n", "A2DP");

    rate     = a2dpSbcRate(codec);
    bps      = a2dpSbcBps(codec);
    channels = a2dpSbcChannels(codec);
    
    setA2dpSrcRate(rate);//for sync linein-adc/sbc-encode/a2dp rate.

    //os_printf("01 a2dpSrcSetConfigurationCb(),getA2dpSrcRate.%d, %d\r\n",getA2dpSrcRate(),channels);

    os_printf("a2dpSrcSetConfigurationCb(), rate=%d, bps=%d, channels=%d\r\n",rate,bps,channels);
    
    if (!rate || !bps || !channels)
    {
        err = UWE_INVAL;
        btA2dpSrcSetConfigurationRej(gA2dpSrc.svc, local_ep_id, remote_ep_id);
        goto Exit;
    }

    err = btA2dpSrcSetConfigurationRsp(gA2dpSrc.svc,
            A2DP_INPUT_METHOD_SBC_TO_PCM, local_ep_id, remote_ep_id);
Exit:
    os_printf("Exit: a2dpSrcSetConfigurationCb(), %d\r\n", err);
    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
}

static void a2dpSrcStreamStartCb(bt_a2dp_src_session_h session_h,
    bt_app_ctx_h app_ctx)
{
    os_printf("a2dpSrcStreamStartCb %lu \r\n", gA2dpSrc.svc_id);

    BK3000_set_clock(CPU_CLK_SEL, CPU_CLK_DIV);  
    
    sbc_stream_start_init();

}

static const bt_a2dp_src_cbs_t a2dpSrcCbs = {
    a2dpSrcSetConfigurationCb,
    a2dpSrcStreamStartCb,
};

static void a2dpSrcNewconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
    const btaddr_t *raddr, bt_link_modes_mask_t mode, bt_app_ctx_h app_ctx)
{
    result_t err = UWE_OK;
	os_printf("== a2dpSrcNewconn(), mode=%d, %d \r\n", mode, gA2dpSrc.is_used);
    
    if (!gA2dpSrc.is_used)
    {        
        bt_a2dp_endpoint_desc_t ep;

        ep.codecs_count = 1;
        ep.codecs[0].type = A2DP_CODEC_SBC;
        btA2dpSrcGetDefaultSbc(&ep.codecs[0].u.sbc);

        err = btA2dpSrcConnAccept(gA2dpSrc.svc, conn_req, mode, &ep);
        os_printf("== btA2dpSrcConnAccept(),err=%d\r\n",err);
        if (err)
            return;

        gA2dpSrc.is_used = 1; 
        return;
    }

    if (gA2dpSrc.is_used || gA2dpSrc.conn_req)
    {
        os_printf("== gA2dpSrc.is_used=%d, gA2dpSrc.conn_req=%d\r\n", gA2dpSrc.is_used, gA2dpSrc.conn_req);
        btA2dpSrcConnReject(conn_req);
        err = UWE_STATE;
        return;
    }

    gA2dpSrc.conn_req = conn_req;
    gA2dpSrc.conn_mode = mode;
}

void a2dpSrcBackendUninit(void)
{
    if (gA2dpSrc.svc)
        btA2dpSrcConnDestroy(&gA2dpSrc.svc);

    btA2dpSrcUnregister();
}

static result_t a2dpSrcServerStart(void)
{
    result_t err;
    bt_a2dp_src_session_h srv = NULL;

    err = btA2dpSrcServerStart(&srv, BTADDR_ANY, a2dpSrcNewconn, NULL);
    if (err)
        goto Exit;

    err = bt_sdp_service_register(BTADDR_ANY, BT_SVC_TYPE_AUDIO_SOURCE,
                &a2dpSrcProfile, sizeof(a2dpSrcProfile), &gA2dpSrc.sdp);
    if (err)
        goto Exit;

    gA2dpSrc.srv = srv;
    
Exit:
    if (err && srv)
        btA2dpSrcServerStop(&srv);

    return err;
}

result_t a2dpSrcBackendInit(void)
{
    result_t err;
    DECLARE_FNAME("a2dpSrcBackendInit");

    j_memset(&gA2dpSrc, 0, sizeof(gA2dpSrc));

    err = btA2dpSrcRegister(&a2dpSrcCbs);
    //os_printf("<<>>1 btA2dpSrcRegister(), err=%d\r\n",err);

    err = a2dpSrcServerStart();
    //os_printf("<<>>2 a2dpSrcServerStart(), err=%d\r\n",err);

    err = btA2dpSrcConnCreate(&gA2dpSrc.svc, &connCbs, NULL);

    if (err) {
        os_printf("a2dpSrcBackendInit fail, %d \r\n", err);
        a2dpSrcBackendUninit();
    } else {
        os_printf("a2dpSrcBackendInit success! \r\n");
    }

    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
    return err;
}

result_t a2dpSrcCmdReconfig(bt_a2dp_codec_t *codec)
{
    os_printf("a2dpSrcCmdReconfig, %d \r\n", gA2dpSrc.is_used);
	if(gA2dpSrc.is_used)
	    return btA2dpSrcConnReconfig(gA2dpSrc.svc, codec);
	else
		return UWE_OK;
}

result_t a2dpSrcCmdStreamStart(void)
{
    result_t err;
    os_printf("a2dpSrcCmdStreamStart, 0x%x \r\n", gA2dpSrc.svc);

    if (!gA2dpSrc.svc) {
        os_printf("a2dpSrcCmdStreamStart, gA2dpSrc.svc is NULL! error! \r\n");
        return UWE_NODEV;
    }
    
    err = a2dpSrcStreamStart(gA2dpSrc.svc);

    return err;
}

result_t a2dpSrcCmdConnect(char *params, unsigned int len)
{
    result_t err;
    btaddr_t laddr, raddr;
    int btaddr_tmp[6];
    uint32_t unit_id;
    bt_a2dp_endpoint_desc_t ep;
    os_printf("a2dpSrcCmdConnect, %d \r\n", gA2dpSrc.is_used);

    if (gA2dpSrc.is_used)
        return UWE_ALREADY;

    if (j_snscanf(params, NULL, "%u " BTADDR_FORMAT, &unit_id,
        BTADDR_TMP(btaddr_tmp)) != UW_ARRAY_SIZE(btaddr_tmp) + 1)
    {
        os_printf("a2dpSrcCmdConnect, UWE_INVAL \r\n");
        return UWE_INVAL;
    }

    TMP_TO_BTADDR(btaddr_tmp, &raddr);

    err = backend_unit_get_addr(unit_id, &laddr);
    if (err)
    {
        os_printf("backend_unit_get_addr, err = %d \r\n", err);
        goto Exit;
    }
    
    ep.codecs_count     = 1;
    ep.codecs[0].type   = A2DP_CODEC_SBC;
    btA2dpSrcGetDefaultSbc(&ep.codecs[0].u.sbc);

    err = btA2dpSrcConnect(gA2dpSrc.svc, &laddr, &raddr, &ep);
    if (err)
    {
        os_printf("btA2dpSrcConnect, err = %d \r\n", err);
        goto Exit;
    }
    
    gA2dpSrc.is_used = 1;
    bt_frontend_notification("a2dp connecting %lu", gA2dpSrc.svc_id);
    os_printf("IV(%s)\r\n", "A2DP");

Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s\n", FNAME, uwe_str(err)));
    return err;
}

void a2dpSrcConnectRemoteDevice(btaddr_t *remote_btaddr)
{
    if (NULL == remote_btaddr) {
        os_printf("a2dpSrcConnectRemoteDevice: remote_btaddr is NULL! error! \r\n");
        return;
    }
    
    char cmd[40];
    memset(cmd, 0 , 40);
    sprintf(cmd,"%u " BTADDR_FORMAT,
                0,
                remote_btaddr->b[5],
                remote_btaddr->b[4],
                remote_btaddr->b[3],
                remote_btaddr->b[2],
                remote_btaddr->b[1],
                remote_btaddr->b[0]);
    
    os_printf("a2dpSrcConnectRemoteDevice: " BTADDR_FORMAT " \r\n", BTADDR(remote_btaddr));

    a2dpSrcCmdConnect(cmd, sizeof(cmd));
}

result_t a2dpSrcCmdStreamSend(struct mbuf *m)
{
    return a2dpSrcStreamSend(gA2dpSrc.svc, m);
}

result_t a2dpSrcCmdStreamSuspend(void)
{
    result_t err;

    if (!gA2dpSrc.svc)
        return UWE_NODEV;

    os_printf("a2dpSrcCmdStreamSuspend, %d\r\n", isSetFlag_APP_FLAG_A2DP_CONNECTION());
    err = a2dpStreamSuspend(gA2dpSrc.svc);

    return err;
}

uint32_t getSrcL2capMbufNum(void)
{
    if (gA2dpSrc.svc)
        return getSrcL2capMbufNumDev(gA2dpSrc.svc);
    else
        return 0;
}

#endif


