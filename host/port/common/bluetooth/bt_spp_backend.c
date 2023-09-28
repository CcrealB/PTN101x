/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>
#include <bluetooth.h>
#include <bt_api.h>
#include <bt_spp.h>
#include <bt_sdp.h>
#include "bt_app_internal.h"
#include "bt_rfcomm_channel.h"
#include "app_beken_includes.h"

/* SPP Service-level Notifications */
static void spp_connected(bt_spp_session_h session, bt_app_ctx_h app_ctx);
static void spp_disconnected(bt_spp_session_h session, result_t status,
    bt_app_ctx_h app_ctx);

static bt_spp_conn_cbs_t spp_conn_cbs = {
    spp_connected,
    spp_disconnected
};

static void spp_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
    const btaddr_t *raddr, bt_link_modes_mask_t mode,
    bt_app_ctx_h app_server_ctx, uint8_t channel);

void spp_result_input(bt_spp_session_h session, char *result, uint16_t length);

result_t spp_send( char *buff, uint32_t len );

static const bt_spp_cbs_t spp_cbs = {
    spp_result_input
};

typedef struct {
    uint32_t                    svc_id;
    uint32_t                    server_sdp;
    bt_spp_server_h             server;
    bt_spp_session_h            session;

    btaddr_t                    laddr;
    btaddr_t                    raddr;
    bt_conn_req_h               conn_req;
    bt_link_modes_mask_t        conn_req_mode;

    BOOL                      is_used;
} spp_app_t;

/* XXX: Currently, only single service is supported by this sample */
static spp_app_t g_spp_app;
unsigned char spp_is_connected(void)
{
    spp_app_t *app = &g_spp_app;
    return app->is_used;
}

static void spp_connected(bt_spp_session_h session, bt_app_ctx_h app_ctx)
{
    //spp_app_t *app = &g_spp_app;
    
    LOG_I(SPP,"spp connected\r\n");
}

static void spp_disconnected(bt_spp_session_h session, result_t err, bt_app_ctx_h app_ctx)
{
    spp_app_t *app = &g_spp_app;

    LOG_I(SPP,"spp disconnected\r\n");

    app->svc_id++;
    app->is_used = 0;
}

static void spp_newconn(bt_conn_req_h conn_req, const btaddr_t *laddr,
                               const btaddr_t *raddr, bt_link_modes_mask_t mode,
                               bt_app_ctx_h app_server_ctx, uint8_t channel)
{
    spp_app_t *app = &g_spp_app;

    DECLARE_FNAME("spp_newconn");

    if(app->is_used || app->conn_req)
    {
        DBG_E(DBT_HFP, ("%s: already connected - rejecting\n", FNAME));
        bt_spp_conn_reject(conn_req);
        return;
    }

    j_memcpy((void *)&app->laddr, (void *)laddr, sizeof(btaddr_t));
    j_memcpy((void *)&app->raddr, (void *)raddr, sizeof(btaddr_t));

    if(bt_spp_conn_accept(app->session, conn_req, mode))
    {
        LOG_W(SPP,"spp connect %lu failed", app->svc_id);
        return;
    }

    app->is_used = 1;
    LOG_I(SPP,"spp connecting %lu\r\n", app->svc_id);
}

extern void uart_send (unsigned char *buff, unsigned int len);
extern void Beken_BTSPP_Rx(uint8_t* buffer, uint32_t length);
void spp_result_input(bt_spp_session_h session, char *result,uint16_t length)
{
    //uart_send((unsigned char *)&result[0], length);
    //spp_send(result, length);

#if (CONFIG_OTA_SPP == 1)
    #if (BEKEN_OTA == 1)
    beken_ota_spp_pkt_reframe((uint8_t*)&result[0], length);
    #endif
#endif

#if (CONFIG_DRIVER_OTA == 1)
    if(!driver_ota_is_ongoing()) 
#endif
        Beken_BTSPP_Rx((uint8_t*)result, length);
}

static enum {
    UNINIT = 0,
    SPP_REGISTER,
    SERVER_START,
    BACKEND_REGISTER,
    CONN_REGISTER
} spp_sample_init_stage = UNINIT;

void spp_backend_uninit(void);
result_t spp_backend_init(void)
{
    result_t err;
    
    LOG_I(SPP,"spp_backend_init\r\n");

    j_memset(&g_spp_app, 0, sizeof(g_spp_app));
    
    err = bt_spp_register(&spp_cbs);
    if(err)
        goto Exit;
    spp_sample_init_stage = SPP_REGISTER;

    err = bt_spp_server_start(&g_spp_app.server, BTADDR_ANY,
        RFCOMM_CHANNEL_SPP, spp_newconn, NULL);
    if(err)
        goto Exit;
    spp_sample_init_stage = SERVER_START;

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
    err = backend_section_register(&hf_section);
    if(err)
        goto Exit;
    spp_sample_init_stage = BACKEND_REGISTER;
#endif

    err = bt_spp_conn_create(&g_spp_app.session, &spp_conn_cbs,
        NULL);
    if(err)
        goto Exit;
    spp_sample_init_stage = CONN_REGISTER;

Exit:
    DBG_RC_I(err, DBT_APP, ("%s: done, %s, stage: %d\n", FNAME, uwe_str(err),
        spp_sample_init_stage));

    if(err)
        spp_backend_uninit();
    
    return err;
}

void spp_backend_uninit(void)
{
    if(spp_sample_init_stage == CONN_REGISTER)
    {
        if(g_spp_app.conn_req)
            (void)bt_spp_conn_reject(g_spp_app.conn_req);
        
        bt_spp_conn_destroy(&g_spp_app.session);
        spp_sample_init_stage = BACKEND_REGISTER;
    }

#ifdef CONFIG_BLUETOOTH_COMM_INTERFACE
    if(spp_sample_init_stage == BACKEND_REGISTER)
    {
        backend_section_unregister(&hf_section);
        spp_sample_init_stage = SERVER_START;
    }
#endif

    if(spp_sample_init_stage == SERVER_START)
    {
        bt_spp_server_stop(&g_spp_app.server);
        spp_sample_init_stage = SPP_REGISTER;
    }

    if(spp_sample_init_stage == SPP_REGISTER)
    {
        bt_spp_unregister();
        spp_sample_init_stage = UNINIT;
    }
}

result_t spp_connect_rfcomm(btaddr_t raddr, uint8_t channel)
{
	result_t err;
	btaddr_t laddr;
	uint32_t unit_id = 0;
	spp_app_t *app = &g_spp_app;
	
	err = backend_unit_get_addr(unit_id, &laddr);
	if(err)
	{
		err = UWE_NODEV;
		goto Exit;
	}
	
	app->laddr = laddr;
	app->raddr = raddr;
	err = bt_spp_conn_connect(app->session, &laddr, &raddr, channel);
	if(err)
		goto Exit;
	
	//bt_frontend_notification("spp connecting %lu", app->svc_id);
	
Exit:
	DBG_RC_I(err, DBT_APP, ("%s: Done (%s)\n", FNAME, uwe_str(err)));
	return err;
}

result_t spp_send(char *buff, uint32_t len)
{
    spp_app_t *app = &g_spp_app;

    return bt_spp_conn_send(app->session, buff, len);
}

result_t spp_slc_disconnect(void)
{
    spp_app_t *app = &g_spp_app;
    if(!app || !app->session)
    {
        LOG_W(SPP,"No spp connected\r\n");
        return UWE_NODEV;
    }

    return bt_spp_conn_disconnect(app->session);
}
