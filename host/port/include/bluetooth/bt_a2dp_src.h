/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#ifndef _BT_A2DP_SRC_H_
#define _BT_A2DP_SRC_H_

/* ********************************************************************
 *  Bluetooth Advanced Audio Distribution Profile (A2DP) Sink Role API
 * ******************************************************************** */

#include <bt_a2dp_types.h>
#include <bt_avdtp.h>
#include <bt_api.h>
#include <bluetooth.h>

#if A2DP_ROLE_SOURCE_CODE
#include <bt_avdtp_proto.h>
#include "../../../bluetooth/profile/a2dp/bt_sbc.h"
#endif

#if A2DP_ROLE_SOURCE_CODE

/* Handle to a Bluetooth session object */
typedef struct bt_a2dp_session_s *bt_a2dp_src_session_h;
/* Handle to a local Bluetooth server */
typedef struct bt_a2dp_session_s *bt_a2dp_src_server_h;

/****************************************
 * A2DP Src Role-Specific Functionality
 ****************************************/

/**
 * Function type:  bt_a2dp_src_set_configuration_req_cb_t
 * Description:    Notifies that an AVDTP Set Configuration Command
 *                 (AVDTP_SET_CONFIGURATION_CMD) was received from the remote
 *                 device.
 *                 NOTE: The application must respond either by immediately
 *                 calling bt_a2dp_sink_set_configuration_rsp() to accept the
 *                 Set Configuration request, or by calling
 *                 bt_a2dp_sink_set_configuration_rej() to reject it.
 * Parameters:
 *     @session:      (IN) Handle to a Bluetooth session object
 *     @codec:        (IN) Pointer toa codec descriptor
 *     @local_ep_id:  (IN) Local endpoint ID
 *     @remote_ep_id: (IN) Remote endpoint ID
 *     @app_ctx_h:    (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_a2dp_src_set_configuration_req_cb_t)(
    bt_a2dp_src_session_h session_h, const bt_a2dp_codec_t *codec,
    int32_t local_ep_id, int32_t remote_ep_id, bt_app_ctx_h app_ctx_h);

/**
 * Function type:  bt_a2dp_sink_stream_start_cb_t
 * Description:    Notifies that an audio stream has been started.
 * Parameters:
 *     @session:     (IN) Handle to a Bluetooth session object
 *     @app_ctx_h:   (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_a2dp_src_stream_start_cb_t)(bt_a2dp_src_session_h session_h,
    bt_app_ctx_h app_ctx_h);
/* A2DP src role-specific application callback functions */
typedef struct {
    bt_a2dp_src_set_configuration_req_cb_t set_configuration_req_cb;
    bt_a2dp_src_stream_start_cb_t          stream_started_cb;
} bt_a2dp_src_cbs_t;

typedef void (*bt_a2dp_src_newconn_cb_t)(bt_conn_req_h conn_req_h,
    const btaddr_t *laddr, const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_ctx_h);


/**
 * Function type:  bt_a2dp_src_connected_cb_t
 * Description:    Notifies that a Bluetooth connection with a remote device
 *                 has been established.
 * Parameters:
 *     @session_h: (IN) Handle to a Bluetooth session object
 *     @app_ctx_h: (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_a2dp_src_connected_cb_t)(bt_a2dp_src_session_h session_h,
    bt_app_ctx_h app_ctx_h);

/**
 * Function type:  bt_a2dp_src_disconnected_cb_t
 * Description:    Notifies that a Bluetooth connection with a remote device
 *                 has been disconnected, or that a previous connection attempt
 *                 has failed.
 * Parameters:
 *     @session_h: (IN) Handle to a Bluetooth session object
 *     @status:    (IN) Disconnect status
 *     @app_ctx_h: (IN) Handle to a Bluetooth application context
 *
 * Return value:   None
 **/
typedef void (*bt_a2dp_src_disconnected_cb_t)(bt_a2dp_src_session_h session_h,
    result_t status, bt_app_ctx_h app_ctx_h);

/* Bluetooth connection-status application notification callback functions */
typedef struct {
    bt_a2dp_src_connected_cb_t    connected_cb;
    bt_a2dp_src_disconnected_cb_t disconnected_cb;
} bt_a2dp_src_conn_cbs_t;

/* Remote stream endpoint */
typedef struct {
    sockaddr_avdtp_stream_t       soaddr;

    /* Service capabilities supported by the remote endpoint */
    avdtp_cap_list_t              caps;
} sep_remote_t;

/* A2DP session state */
typedef enum {
    A2DP_SESSION_DETACHED = 0,
    A2DP_SESSION_ATTACHED
} a2dp_session_state_t;

typedef struct {
    struct bt_a2dp_session_s     *session;
    /* TODO: change name to include avdtp */
    avdtp_stream_h               lower;
    bt_sbc_h                     sbc_h;          /* SBC audio codec */
    uint8_t                      flags;

    bt_a2dp_endpoint_desc_t      ep;
    uint8_t                     method;
    void                         *app_stream_ctx;
} stream_softc_t;

typedef struct bt_a2dp_session_s {
    a2dp_session_state_t            state;

    bt_link_modes_mask_t            mode;        /* link mode */

    /* TODO: change name to include avdtp */
    avdtp_session_h                 session_h;
    bt_a2dp_src_newconn_cb_t       newconn_cb;

    bt_a2dp_src_conn_cbs_t         conn_cbs;
    bt_app_ctx_h                    app_ctx;

    stream_softc_t                  *stream;     /* stream */
    stream_softc_t                  *stream_l;   /* stream listen */

    /* An array of remote endpoints */
    sep_remote_t                    *rseps;
    uint8_t                         rseps_n;
    uint8_t                         get_caps_index;
    uint32_t                        record_handle; /* Server SDP record handle */

    LIST_ENTRY(bt_a2dp_session_s)   next;
} a2dp_session_t;

#endif


#endif
