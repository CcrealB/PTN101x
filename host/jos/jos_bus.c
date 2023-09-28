/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include "jos_internal.h"

/* Minimal interval between non DSR (level 0) tasks */
#define TASK_EXEC_INTERVAL 2

/* Approximate time between the stack getting stuck and the developer
 * resetting the PC */
#define TASK_MAX_REASONABLE_RUN_TIME 33000
/* TODO - Refine values */
#define TASK_CTRL_REASONABLE_RUN_TIME TASK_MAX_REASONABLE_RUN_TIME
#define TASK_TO_REASONABLE_RUN_TIME   TASK_MAX_REASONABLE_RUN_TIME
#define TASK_PNP_REASONABLE_RUN_TIME  TASK_MAX_REASONABLE_RUN_TIME

typedef struct {
#ifdef DEBUG_SYNC
    const char  *locker_file;
    int32_t       locker_line;
    BOOL      initialized;
#endif
    BOOL      locked;
} jcritical_lock_t;

/* Internal JOS Critical Section lock */
static jcritical_lock_t jcritical_lock;


typedef struct {
    jthread_func        soft_intr;
   void               *dsr_arg;
} isr_data_t;

/**
 * Function name:  j_bus_init
 * Description:    Performs initialization of generic infrastructure of
 *                 controllers and interrupts
 * Parameters:
 *     @args: (IN)  Stack init arguments
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t j_bus_init(uw_args_t *args)
{
    DECLARE_FNAME("j_bus_init");

    DBG_V(DJOS_INIT, ("%s: Entered\n", FNAME));

    j_memset(&jcritical_lock, 0, sizeof(jcritical_lock_t));

#ifdef CONFIG_SINGLE_THREADED
    return UWE_OK;
#endif
}

/**
 * Function name:  j_bus_uninit
 * Description:    Performs uninitialization generic infrastructure of
 *                 controllers and interrupts
 * Return Value:   None
 * Scope:          Global
 **/
void j_bus_uninit(void)
{
    DECLARE_FNAME("j_bus_uninit");

    DBG_V(DJOS_INIT, ("%s: Entered\n", FNAME));

    KASSERT(!jcritical_lock.locked, ("%s: Still in critical section\n", FNAME));

    j_memset(&jcritical_lock, 0, sizeof(jcritical_lock_t));
}

void jos_critical_enter(void)
{
    os_critical_enter();
    jcritical_lock.locked = 1;
}

void jos_critical_leave(void)
{
    jcritical_lock.locked = 0;
    os_critical_leave();
}

BOOL jos_critical_check(void)
{
    return jcritical_lock.locked;
}

#ifdef DEBUG_SYNC
void jos_critical_enter_d(const char *file, int32_t line)
{
    DECLARE_FNAME("jos_critical_enter_d");

    if(!jcritical_lock.initialized)
    {
        jcritical_lock.locked = 0;
        jcritical_lock.initialized = TRUE;
        jcritical_lock.locker_file = NULL;
        jcritical_lock.locker_line = 0;
    }

    DBG_V(DJOS_SYNC, ("%s: Entering from [%s:%d].\n", FNAME, file, line));

    jos_critical_enter();

    jcritical_lock.locker_file = file;
    jcritical_lock.locker_line = line;
}

void jos_critical_leave_d(const char *file, int32_t line)
{
    int32_t locker_line = jcritical_lock.locker_line;
    const char *locker_file = jcritical_lock.locker_file;
    DECLARE_FNAME("jos_critical_leave_d");

    if(!jcritical_lock.initialized)
    {
        DBG_E(DJOS_SYNC, ("%s: Critical lock is not initialize, [%s,%d]\n",
            FNAME, file, line));
        return;
    }

    KASSERT0(jcritical_lock.locked);

    jcritical_lock.locker_line = 0;
    jcritical_lock.locker_file = NULL;

    jos_critical_leave();

    DBG_V(DJOS_SYNC, ("%s: Leaving from [%s:%d]. Last enter from [%s:%d]\n",
        FNAME, file, line, locker_file, locker_line));
}

uint32_t jos_critical_check_d(const char *file, int32_t line)
{
    DECLARE_FNAME("jos_critical_check_d");

    DBG_V(DJOS_SYNC, ("%s: Checking lock [%d] from [%s:%d]. "
        "Last enter from [%s:%d]\n", FNAME, jcritical_lock.locked, file, line,
        jcritical_lock.locker_file ? jcritical_lock.locker_file : "none",
        jcritical_lock.locker_line));

    return jos_critical_check();
}
#endif

/**
 * Function name:  j_generic_dsr
 * Description:    Manages context switch from DSR to software interrupt.
 * Parameters:
 *     @arg: (IN) Interrupt handler wrapper.
 * Return Value:   None
 * Scope:          Local
 **/
static void j_generic_dsr(void *arg)
{
    isr_data_t *data = (isr_data_t *)arg;
    KASSERT(arg, ("JBUS: No wrapper handle given to generic dsr\n"));

    /* If there is no software interrupt handler, just quit */
    if(!data->soft_intr)
        return;
#ifdef CONFIG_SINGLE_THREADED
    data->soft_intr(data->dsr_arg);
#endif
}

/**
 * Function name:  jos_interrupt_teardown
 * Description:    Removes an interrupt handler.
 * Parameters:
 *     @res:    (IN) Handle to the bus resource from which to unbind.
 *     @handle: (IN) Handle to JOS interrupt context, as received from
 *                   jos_interrupt_setup().
 *
 * Return value:   None
 * Scope:          Global
 *    */
void jos_interrupt_teardown(jbus_resource_h res, void *handle)
{
    isr_data_t *data = (isr_data_t *)handle;
    result_t rc;

    KASSERT(handle, ("JBUS: No wrapper handle given to interrupt teardown\n"));

    rc = os_interrupt_teardown(res);
    if(rc)
        DBG_E(DJOS_INIT, ("JBUS: Failed to tear down interrupt\n"));

    jfree(data);
    data = NULL;
}

/**
 * Function name:  jos_interrupt_setup
 * Description:    Sets up an interrupt handler with a wrapper to give correct
 *                 context to software interrupts.
 * Parameters:
 *     @res:      (IN) Handle to the bus resource to which to attach.
 *     @isr_func: (IN)  Hardware interrupt function to call
 *     @isr_arg:  (IN)  Argument to hardware interrupt
 *     @dsr_func: (IN)  Software interrupt function to call
 *     @dsr_arg:  (IN)  Argument to software interrupt
 *     @handle:   (OUT) Pointer to a handle to JOS interrupt context.
 *
 * Return value:  UWE_OK on success, error otherwise
 * Scope:         Global
 **/
result_t jos_interrupt_setup(jbus_resource_h res, interrupt_handler isr_func,
   void *isr_arg, jthread_func dsr_func, void *dsr_arg, void **handle)
{
    isr_data_t *data;
    result_t rc;
    DECLARE_FNAME("jos_interrupt_setup");

    /* Allocate data for interrupt wrapper */
    data = (isr_data_t *)jmalloc(sizeof(isr_data_t), M_ZERO);
    data->soft_intr = dsr_func;
    data->dsr_arg = dsr_arg;

    /* Set up actual interrupt handler with the bus */
    rc = os_interrupt_setup(res, isr_func, isr_arg, j_generic_dsr,
        (void *)data);
    if(rc)
    {
        DBG_E(DJOS_INIT, ("%s: Failed to register interrupt with the porting "
            "layer - %s\n", FNAME, uwe_str(rc)));
        goto Error;
    }

    *handle = data;

    return UWE_OK;

Error:
    if(data)
    {
        jfree(data);
        data = NULL;
    }

    *handle = NULL;

    return rc;
}

/* Controller related functions */
jbus_h j_controller_get_bus(j_device_h dev)
{
    KASSERT0(dev);
    KASSERT(j_device_get_aa(dev), ("Device %p (%s) attach argument is NULL!\n",
        dev, j_device_get_nameunit(dev)));
    KASSERT(dev->type == J_DEVICE_TYPE_CONTROLLER, ("Device %p type is not "
        "controller! (%d)\n", dev, dev->type));

    return ((controller_attach_arg_t *)j_device_get_aa(dev))->bus;
}

result_t add_generic_controller_ex(os_bus_h bus, int32_t id, void **handle,
   void *args)
{
    j_device_h dev;
    result_t rc;
    BOOL init = 1;
    uw_init_opt_t init_opt = j_get_init_opt();
    /* TODO: Create HCD/DCD_attach arguments in the porting layer and pass them
     * to the function. Currently we assume that both attach arguments
     * structures are the same (hence the malloc). */
    controller_attach_arg_t *caa;
    DECLARE_FNAME("add_generic_controller_ex");

    DBG_V(DJOS_INIT, ("%s: Entered, id 0x%x, bus %p\n", FNAME, id, bus));

    /* Adding controllers guide lines:
     * TC should be added at all times
     * OTG should be added at all times
     * HC should be added when init_opt is Host or HostDevice or OTG
     * DC should be added when init_opt is Device or HostDevice or OTG */
    switch (CONTROLLER_GET_TYPE(id))
    {
    case CONTROLLER_TYPE_TRANSCEIVER:
    case CONTROLLER_TYPE_OTG:
        break;
    case CONTROLLER_TYPE_HOST:
        if(!(init_opt & (UW_INIT_HOST | UW_INIT_OTG)))
            init = 0;
        break;
    case CONTROLLER_TYPE_DEVICE:
        if(!(init_opt & (UW_INIT_DEVICE | UW_INIT_OTG)))
            init = 0;
        break;
    default:
        DBG_E(DJOS_INIT, ("Controller type 0x%x (type 0x%x) not recognized\n",
            id, CONTROLLER_GET_TYPE(id)));
        init = 0;
        break;
    }
    if(!init)
    {
        DBG_E(DJOS_INIT, ("%s: Skipping add for controller 0x%x type 0x%x "
            "(Stack init_opt is 0x%x)\n", FNAME, id, CONTROLLER_GET_TYPE(id),
            init_opt));
        return UWE_OK;
    }

    caa = (controller_attach_arg_t *)jmalloc(sizeof(controller_attach_arg_t), M_ZERO);
    caa->bus = bus;
    caa->id = id;
    caa->ivars = args;

    dev = j_device_add(J_DEVICE_TYPE_CONTROLLER, NULL, (j_device_aa_h)caa,
        NULL, NULL);
    if(!dev)
    {
        rc = UWE_NOMEM;
        goto Error;
    }

    rc = UWE_INVAL;

    DBG_I(DJOS_INIT, ("Attaching Generic Controller\n"));

    rc = j_device_probe_and_attach(dev);
    if(rc)
    {
        DBG_E(DJOS_INIT, ("%s: Error in driver attach. rc %s\n", FNAME,
            uwe_str(rc)));

        goto Error;
    }

    *handle = dev;

    return rc;

Error:
    if(dev)
        j_device_delete(dev);
    jfree(caa);
    caa = NULL;

    return rc;
}

result_t add_generic_controller(os_bus_h bus, int32_t id, void **handle)
{
    result_t rc;

    CORE_SAFE_ENTER;
    rc = add_generic_controller_ex(bus, id, handle, NULL);
    CORE_SAFE_LEAVE;

    return rc;
}

result_t generic_controller_suspend(void *handle)
{
    result_t rc;
    DECLARE_FNAME("generic_controller_suspend");

    DBG_I(DJOS_INIT, ("%s: Entered, handle %p\n", FNAME, handle));
    CORE_SAFE_ENTER;
    rc = j_device_suspend((j_device_h)handle);
    CORE_SAFE_LEAVE;

    return rc;
}

result_t generic_controller_resume(void *handle)
{
    result_t rc;
    DECLARE_FNAME("generic_controller_resume");

    DBG_I(DJOS_INIT, ("%s: Entered, handle %p\n", FNAME, handle));
    CORE_SAFE_ENTER;
    rc = j_device_resume((j_device_h)handle);
    CORE_SAFE_LEAVE;

    return rc;
}

result_t del_generic_controller_ex(void *handle)
{
    j_device_h self = (j_device_h)handle;
    controller_attach_arg_t *caa =
        (controller_attach_arg_t *)j_device_get_aa(self);
    DECLARE_FNAME("del_generic_controller_ex");

    DBG_I(DJOS_INIT, ("%s: Entered, handle %p\n", FNAME, handle));

    j_device_delete(self);
    jfree(caa);
    caa = NULL;
    
    DBG_I(DJOS_INIT, ("%s: Finished\n", FNAME));

    return UWE_OK;
}

result_t del_generic_controller(void *handle)
{
    result_t rc;

    CORE_SAFE_ENTER;
    rc = del_generic_controller_ex(handle);
    CORE_SAFE_LEAVE;

    return rc;
}

// EOF
