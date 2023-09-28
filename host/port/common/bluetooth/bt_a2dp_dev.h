#ifndef _BT_A2DP_DEV_H_
#define _BT_A2DP_DEV_H_
#include "bt_avdtp_proto.h"
#include "bt_a2dp_sink.h"
#include "../../../bluetooth/profile/a2dp/bt_sbc.h"
#include "app_vendor_decode.h" // for a2dp vendor decode

/* st flags */
#define ST_BEING_FREED          (1 << 0)
#define ST_DISCONNECTING        (1 << 1)

#if defined(A2DP_VENDOR_DECODE) || defined(A2DP_MPEG_AAC_DECODE)
#define AVDTP_SEID_NUM 2
#endif

/* A2DP session state */
typedef enum {
    A2DP_SESSION_DETACHED = 0,
    A2DP_SESSION_ATTACHED
} a2dp_session_state_t;

typedef struct {
    struct bt_a2dp_session_s     *session;
    /* TODO: change name to include avdtp */
    avdtp_stream_h               lower;
    uint8_t                      flags;
    uint8_t                      bt_sbc_flag;

    bt_a2dp_endpoint_desc_t      ep;
    bt_a2dp_sink_input_method_t  method;
   void                        *app_stream_ctx;
} stream_softc_t;

/* Remote stream endpoint */
typedef struct {
    sockaddr_avdtp_stream_t       soaddr;

    /* Service capabilities supported by the remote endpoint */
    avdtp_cap_list_t              caps;
} sep_remote_t;

typedef struct bt_a2dp_session_s {
    a2dp_session_state_t            state;

    bt_link_modes_mask_t            mode;        /* link mode */

    /* TODO: change name to include avdtp */
    avdtp_session_h                 session_h;

    /* TODO: move to server specific struct */
    bt_a2dp_sink_newconn_cb_t       newconn_cb;

    bt_a2dp_sink_conn_cbs_t         conn_cbs;
    bt_app_ctx_h                    app_ctx;

#if defined(A2DP_VENDOR_DECODE) || defined(A2DP_MPEG_AAC_DECODE)
    stream_softc_t                  *stream[AVDTP_SEID_NUM];     /* stream */
    stream_softc_t                  *stream_l[AVDTP_SEID_NUM];   /* stream listen */
#else
    stream_softc_t                  *stream;     /* stream */
    stream_softc_t                  *stream_l;   /* stream listen */
#endif

    /* An array of remote endpoints */
    sep_remote_t                    *rseps;
    uint8_t                         rseps_n;
    uint8_t                         get_caps_index;
    
    uint32_t                        record_handle; /* Server SDP record handle */

    LIST_ENTRY(bt_a2dp_session_s)   next;
} a2dp_session_t;

typedef struct {
    bt_a2dp_sink_cbs_t              cbs;

    LIST_HEAD(, bt_a2dp_session_s)  services;
    LIST_HEAD(, bt_a2dp_session_s)  servers;

    uint32_t                        register_flag;
} a2dp_softc_t;

#define SOCKADDR_INIT(soaddr) do {              \
    j_memset((soaddr), 0, sizeof(*(soaddr)));   \
    (soaddr)->len = sizeof(*(soaddr));          \
    (soaddr)->psm = L2CAP_PSM_AVDTP;            \
} while(0)

#ifndef A2DP_VENDOR_DECODE
uint32_t a2dp_stream_is_current_app(void *btproto_ctx);
#endif

#endif
// EOF
