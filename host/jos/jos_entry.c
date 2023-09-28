/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "jos_internal.h"
#include "jos_driver.h"
#include "jinit.h"

#ifdef CONFIG_BLUETOOTH
#include <bt_init.h>
#endif

/* Stack's Initialization Options, supplied via uw_args */
static uw_init_opt_t init_opt = 0;

BOOL usbware_stopping = 0;
result_t uw_app_init(void);
void uw_app_uninit(void);

result_t jstart_stack(uw_args_t *args)
{
#define J_UW_INIT               (1 << 0)
#define J_MEDIACORE_INIT        (1 << 1)
#define J_BT_INIT               (1 << 2)
#define J_APP_INIT              (1 << 3)
#define J_DRV_INIT              (1 << 4)
#define J_HW_INIT               (1 << 5)
#define J_UW_ENABLE             (1 << 6)
#define J_USERSPACE_CLASS_INIT  (1 << 7)

    result_t rc = UWE_OK;
    int32_t init_status = 0;

    if(!args)
    {
        return UWE_INVAL;
    }

    /* Initialize JOS */
    rc = jos_init(args);
    if(rc)
    {
        return rc;
    }
    
    LOG_D(APP,"jstart_stack version "UWVER_STR"\r\n");

    /* Store initialization option */
    init_opt = args->init_opt;

#ifdef CONFIG_BLUETOOTH
    rc = bt_init();
    if (rc)
        goto Error;

    init_status |= J_BT_INIT;
#endif

    /* Initialize any supporting applications */
    rc = uw_app_init();
    if (rc)
        goto Error;
    init_status |= J_APP_INIT;

    rc = j_drivers_init(args);
    if(rc)
        goto Error;
    init_status |= J_DRV_INIT;

#ifdef CONFIG_JOS_BUS
    rc = jhw_init();
    if(rc)
    {
        goto Error;
    }
    init_status |= J_HW_INIT;
#endif

    jsafe_leave();

    return UWE_OK;

Error:
#ifdef CONFIG_JOS_BUS
    if(init_status & J_HW_INIT)
        jhw_uninit();
#endif

    if(init_status & J_DRV_INIT)
        j_drivers_uninit();

    if(init_status & J_APP_INIT)
        uw_app_uninit();
        
#ifdef CONFIG_BLUETOOTH
    if (init_status & J_BT_INIT)
        bt_uninit();
#endif

    jos_uninit();

    init_opt = 0;

    return rc;
}

void jstop_stack(void)
{
    jsafe_enter();
    usbware_stopping = 1;

#ifdef CONFIG_JOS_BUS
    /* Unload all controllers */
    jhw_uninit();
#endif

    j_drivers_uninit();

    uw_app_uninit();

#ifdef CONFIG_BLUETOOTH
    bt_uninit();
#endif

    jos_uninit();

    /* Print memory leaks */
    DBG_RUN(DJOS_INIT, DL_ERROR, mem_log_print());

    usbware_stopping = 0;
    init_opt = 0;
}

/* Retrieve initialization option */
uw_init_opt_t j_get_init_opt(void)
{
    return init_opt;
}

