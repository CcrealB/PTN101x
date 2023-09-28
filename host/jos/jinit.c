/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "jos_internal.h"
#include "jinit.h"

BOOL bootverbose;
BOOL cold;

#define JOS_MEM_INIT            (1 << 0)
#define JOS_SAFE_INIT           (1 << 1)
#define JOS_PORT_INIT           (1 << 2)
#define JOS_SYNC_INIT           (1 << 3)
#define JOS_BUS_INIT            (1 << 4)
#define JOS_DRIVER_INIT         (1 << 5)
#define JOS_MBUF_INIT           (1 << 6)
#define JOS_SOCKET_INIT         (1 << 7)
#define JOS_TEST_JDRV_INIT      (1 << 9)
#define JOS_SELECT_INIT         (1 << 10)
#define JOS_DOMAIN_INIT         (1 << 11)
#define JOS_EVENT_INIT          (1 << 12)
#define JOS_LOAD_MODULE         (1 << 13)
#define JOS_KMEM_INIT           (1 << 14)
#define JOS_UMEM_INIT           (1 << 15)
static uint16_t jos_init_stage = 0;

result_t jos_init(uw_args_t *args)
{
    result_t rc;

    LOG_D(APP,"jos_init\r\n");

    rc = jos_mem_init(args);
    if(rc)
        goto Error;
#ifdef JMALLOC_STATISTICAL
    os_printf("JMALLOC_STATISTICAL: POOL INIT\r\n");
    memory_usage_show();
#endif

    jos_init_stage |= JOS_MEM_INIT;

#ifdef CONFIG_JOS_MBUF
    rc = mbuf_init();
    if(rc)
        goto Error;

    jos_init_stage |= JOS_MBUF_INIT;
#endif

    jsafe_enter();

    jos_init_stage |= JOS_SAFE_INIT;

    rc = j_event_init();
    if(rc)
        goto Error;

    jos_init_stage |= JOS_EVENT_INIT;

    rc = os_port_init();
    if(rc)
        goto Error;

    jos_init_stage |= JOS_PORT_INIT;

    rc = jos_sync_init();
    if(rc)
        goto Error;

    jos_init_stage |= JOS_SYNC_INIT;

#if defined(CONFIG_JOS_BUS)
    rc = j_bus_init(args);
    if(rc)
        goto Error;

    jos_init_stage |= JOS_BUS_INIT;
#elif defined(CONFIG_JOS_THREADS)
    rc = j_threads_init();
    if(rc)
        goto Error;

    jos_init_stage |= JOS_BUS_INIT;
#endif

    rc = jos_driver_init();
    if(rc)
        goto Error;

    jos_init_stage |= JOS_DRIVER_INIT;

    return UWE_OK;

Error:
    jos_uninit();
    return rc;
}

void jos_uninit(void)
{
    if(jos_init_stage & JOS_DRIVER_INIT)
        jos_driver_uninit();

#if defined(CONFIG_JOS_BUS)
    if(jos_init_stage & JOS_BUS_INIT)
        j_bus_uninit();
#endif

    if(jos_init_stage & JOS_SYNC_INIT)
        jos_sync_uninit();

    if(jos_init_stage & JOS_MEM_INIT)
        jdma_free_all();

    if(jos_init_stage & JOS_PORT_INIT)
        os_port_uninit();

    if(jos_init_stage & JOS_EVENT_INIT)
        j_event_uninit();

    if(jos_init_stage & JOS_SAFE_INIT)
    {
        jsafe_leave();
    }

#ifdef CONFIG_JOS_MBUF
    if(jos_init_stage & JOS_MBUF_INIT)
        mbuf_uninit();
#endif

    if(jos_init_stage & JOS_MEM_INIT)
        jos_mem_uninit();

    jos_init_stage = 0;
}

