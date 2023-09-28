/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <uw_drivers.h>
#include "jos_driver.h"

/* TODO - Rename: driver APIs, driver context (j_driver_t), driver handle */
typedef struct j_driver_s {
    driver_methods_t            *methods;
    j_device_type_t             type;
    const char                  *name;
    uint8_t                     usage_count;
    uint8_t                     curr_unit;
    uint16_t                    softsize;
    TAILQ_ENTRY(j_driver_s)     next;
    TAILQ_HEAD(dev_list_s, j_device_s) dev_list;
} j_driver_t;

/* Lists of registered drivers */
static TAILQ_HEAD(, j_driver_s) driver_list;

static TAILQ_HEAD(, j_device_s) unmatched_dev_list;

static result_t jdriver_dev_attach(j_device_h device);

static struct {
    j_driver_type_t type;
    driver_init_t init_func;
} driver_load_list[] = 
{
#if defined CONFIG_BLUETOOTH_HCI_UART
    {DRIVER_TYPE_BT_UART, bt_uart_init},
#endif

    {DRIVER_TYPE_LAST, NULL}
};

void j_device_add_to_unmatched_list(j_device_h device)
{
    TAILQ_INSERT_TAIL(&unmatched_dev_list, device, next_in_drv_list);
}

result_t j_device_remove_from_unmatched_list(j_device_h device)
{
    uint8_t found;

    TAILQ_FIND_AND_REMOVE(&unmatched_dev_list, device, j_device_h,
        next_in_drv_list, found);
    if(!found)
    {
        DBG_E(DJOS_DRIVER, ("JDRV: device %p (%s) not in unmatched list\n",
            device, j_device_get_nameunit(device)));

        return UWE_INVAL;
    }

    return UWE_OK;
}

result_t jos_driver_init(void)
{
    /* Initialize the driver list. After this point driver's init function which
     * calls j_driver_register can be called */
    TAILQ_INIT(&driver_list);

    TAILQ_INIT(&unmatched_dev_list);

    return UWE_OK;
}

void jos_driver_uninit(void)
{
}

static BOOL find_driver(j_driver_type_t *driver_list, j_driver_type_t driver)
{
    int32_t i;

    for (i = 0; driver_list[i] < DRIVER_TYPE_LAST; i++)
    {
        if(driver_list[i] == driver)
            return 1;
    }

    DBG_IF(driver_list[i] > DRIVER_TYPE_LAST)
    {
        DBG_E(DJOS_DRIVER, ("JDRV: Invalid driver type, entry (%d) in list is "
            "%d (max value is %d)\n", i, driver_list[i], DRIVER_TYPE_LAST-1));
    }

    return 0;
}

/**
 * Function name:  j_drivers_init
 * Description:    Initializes all drivers in driver_load_list.
 * Parameters:     
 *     @args:   (IN)  Stack init arguments
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t j_drivers_init(uw_args_t *args)
{
    result_t rc = UWE_OK;
    int32_t i;

    CORE_SAFE_ENTER;

    for (i = 0; driver_load_list[i].type != DRIVER_TYPE_LAST; i++)
    {
        if(args->inactive_drivers &&
            find_driver(args->inactive_drivers, driver_load_list[i].type))
        {
            continue;
        }

        rc = driver_load_list[i].init_func();
        if(rc)
        {
            j_drivers_uninit();
            goto Exit;
        }
    }

Exit:
    CORE_SAFE_LEAVE;
    return rc;
}

/**
 * Function name:  j_drivers_uninit
 * Description:    Uninitializes all drivers in driver_list.
 * Parameters:     None
 * Return value:   None
 * Scope:          Global
 **/
void j_drivers_uninit(void)
{
    j_driver_t *curr;

    CORE_SAFE_ENTER;

    DBG_I(DJOS_DRIVER, ("JDRV: Starting driver unload\n"));
    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_driver_list());

    while ((curr = TAILQ_FIRST(&driver_list)) != NULL)
    {
        DBG_I(DJOS_DRIVER, ("JDRV: Removing driver %p (%s)\n", curr,
            curr->name));

        if(curr->usage_count)
        {
            DBG_E(DJOS_DRIVER, ("JDRV: Unloading driver [%s] type %d "
                "that is being used by %d devices !\n",
                curr->name, curr->type, curr->usage_count));
        }

        /* Run the driver's uninit function, if it exists; this function should
         * call j_driver_unregister(). */
        if(curr->methods->uninit)
            curr->methods->uninit();
        else
            j_driver_unregister(curr);
    }

    DBG_I(DJOS_DRIVER, ("JDRV: Finished driver unloading\n"));

    CORE_SAFE_LEAVE;
}

/**
 * Function name:  j_device_retry_attach
 * Description:    Attempts to match all devices in unmatched_dev_list to
 *                 registered drivers.
 * Parameters:     None
 * Return value:   None
 * Scope:          Global
 **/
void j_device_retry_attach(void)
{
    j_device_h next, curr = NULL;
    result_t rc;
    DECLARE_FNAME("j_device_retry_attach");

    DBG_V(DJOS_DRIVER, ("%s: Entered\n", FNAME));

    TAILQ_FOREACH_SAFE(curr, &unmatched_dev_list, next_in_drv_list, next)
    {
        DBG_X(DJOS_DRIVER, ("%s: device %p (%s), parent %s \n", FNAME, curr,
            j_device_get_nameunit(curr), j_device_get_nameunit(curr->parent)));

        /* If the device is in the unmatched list it is suspended - resume it */
        if(j_device_get_parent(curr))
        {
            rc = j_device_ioctl(j_device_get_parent(curr),
                DRV_IOCTL_PWR_CHILD_RESUME, (void *)curr);
            if(rc && rc != UWE_ALREADY)
            {
                DBG_E(DJOS_DRIVER, ("%s: Failed resuming device %p (%s) - %s\n",
                    FNAME, curr, j_device_get_nameunit(curr), uwe_str(rc)));
                continue;
            }
        }

        rc = jdriver_dev_attach(curr);
        if(rc && j_device_get_parent(curr))
        {
            DBG_X(DJOS_DRIVER, ("%s: Didn't find matching driver for "
                "device %p, suspending\n", FNAME, curr));
            j_device_ioctl(j_device_get_parent(curr),
                DRV_IOCTL_PWR_CHILD_SUSPEND, (void *)curr);
        }
    }

    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_free_devices());
}

/**
 * Function name:  j_driver_unregister
 * Description:    Unregisters a driver from JOS.
 * Parameters:
 *     @drv: (IN) Handle to the driver to unregister, received during
 *                registration (see j_driver_register() @drv_h)
 *
 * Return value:   None
 * Scope:          Global
 **/
void j_driver_unregister(j_driver_h drv)
{
#ifdef J_DEBUG
    uint8_t found;
    DECLARE_FNAME("j_driver_unregister");

    KASSERT0(drv);
    DBG_V(DJOS_DRIVER, ("%s: Entered, driver %p (%s)\n", FNAME, drv,
        drv->name ? drv->name : ""));
    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_driver_list());

    TAILQ_FIND_AND_REMOVE(&driver_list, drv, j_driver_t *, next, found);
    KASSERT(found, ("%s: Unregistered driver not in Driver List!!! (%p %s)\n",
        FNAME, drv, drv->name ? drv->name : ""));
#else
    /* Remove from driver_list */
    TAILQ_REMOVE(&driver_list, drv, next);
#endif

    if(drv->usage_count)
    {
        j_device_h dev;

        DBG_E(DJOS_DRIVER, ("%s: Unregister Driver with %d active devices\n",
            FNAME, drv->usage_count));

        while ((dev = TAILQ_FIRST(&drv->dev_list)) != NULL)
        {
            /* Assume running with Core Mutex taken */
            DBG_X(DJOS_DRIVER, ("%s: detaching dev %p %s\n", FNAME, dev,
                j_device_get_nameunit(dev)));
            j_device_detach(dev);
        }
    }

    DBG_V(DJOS_DRIVER, ("%s: Finished unregister driver %s\n", FNAME,
        drv->name));

    /* Free driver item */
    jfree(drv);
    drv = NULL;

    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_free_devices());
    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_driver_list());
}

/**
 * Function name:  j_driver_register
 * Description:    Registers a driver with JOS.
 * Parameters:
 *     @type:   (IN)  The type of devices that the driver can handle
 *     @methods:(IN)  Pointer to a driver-methods structure, to be used by JOS
 *     @name:   (IN)  Driver name
 *     @size:   (IN)  Size, in bytes, of driver's software context, to be
 *                    allocated by JOS
 *     @drv_h:  (OUT) Pointer to a driver handle, to be filled by JOS.
 *                    The driver should pass this handle to other JOS APIs.
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t j_driver_register(j_device_type_t type, driver_methods_t *methods,
    const char *name, uint16_t size, j_driver_h *drv_h)
{
    j_driver_t *new_driver;
    DECLARE_FNAME("j_driver_register");

    if(drv_h)
        *drv_h = NULL;
#ifdef J_DEBUG
    if(!name)
    {
        DBG_E(DJOS_DRIVER, ("%s: Cannot register, no name provided\n", FNAME));
        return UWE_INVAL;
    }

    if(!methods)
    {
        DBG_E(DJOS_DRIVER, ("%s: Cannot register %s, no methods\n", FNAME,
            name));
        return UWE_INVAL;
    }

    if(!methods->attach || !methods->detach || !methods->match)
    {
        DBG_E(DJOS_DRIVER, ("%s: Cannot register %s, some methods are "
            "missing\n", FNAME, name));
        return UWE_INVAL;
    }

    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_free_devices());
#endif

    new_driver = (j_driver_t *)jmalloc(sizeof(j_driver_t), M_ZERO);
    new_driver->type    = type;
    new_driver->methods = methods;
    new_driver->name    = name;
    new_driver->usage_count = 0;
    new_driver->softsize = size;
    new_driver->curr_unit = 0;
    TAILQ_INIT(&new_driver->dev_list);

    /* Insert into list */
    TAILQ_INSERT_TAIL(&driver_list, new_driver, next);

    if(drv_h)
        *drv_h = new_driver;

    DBG_I(DJOS_DRIVER, ("%s: Registered new driver %p type %d [%s:%d]\n",
        FNAME, new_driver, type, name, size));

    /* Try to find match in unmatched devices */
    if(TAILQ_FIRST(&unmatched_dev_list))
        j_device_retry_attach();

    return UWE_OK;
}

/**
 * Function name:  j_driver_detach
 * Description:    Detaches a device from a driver.
 * Parameters:
 *     @device: (IN) Handle to a JOS device
 *
 * Return value:   None
 * Scope:          Global
 **/
void j_driver_detach(j_device_h device)
{
    j_driver_t *drv;

    DBG_V(DJOS_DRIVER, ("JDRV: Starting detach for %p (%s)\n", device,
        j_device_get_nameunit(device)));
    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_children(device));

    LOCK_CHECK;
    CORE_SAFE_CHECK;
    DBG_ASSERT_CTX(J_CTX_PNP | J_CTX_OTHER);

    if(!j_device_is_owned(device))
    {
        DBG_E(DJOS_DRIVER,("JDRV: j_driver_detach: device %p (%s) has no driver"
            "\n", device, j_device_get_nameunit(device)));
        return;
    }

    /* Save the driver context, since the jdrv field may be NULL after detach */
    drv = device->jdrv;

    DBG_I(DJOS_DRIVER, ("JDRV: Detaching device %p (%s), parent %s from %p "
        "(%s)\n", device, j_device_get_nameunit(device),
        j_device_get_parent(device) ?
        j_device_get_nameunit(j_device_get_parent(device)) : "None",
        drv, drv->name));

    if(drv->methods->detach)
        drv->methods->detach(device);

    if(device->jdrv)
    {
        /* If the jdrv field is not NULL, owner didn't call disown during the
         * detach -- call it directly. This should be an error once all drivers
         * will implement the own and disown methods. */
        DBG_V(DJOS_DRIVER, ("JDRV: Detaching device %p (%s) from driver %s, "
            "calling disown\n", device, j_device_get_nameunit(device),
            drv->name));
        j_device_disown(device);
    }

    if(device->owner_ctx)
    {
        jfree(device->owner_ctx);
        device->owner_ctx = NULL;
    }

    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_free_devices());
}

static result_t jdriver_dev_attach(j_device_h device)
{
    static int32_t unknown_unit = 0;
    j_driver_t *curr;
    j_driver_t *best = NULL;
    umatch_t match_level, best_level = UMATCH_NONE;
    int32_t device_type;
    result_t rc;

    LOG_D(APP,"jdriver_dev_attach\r\n");

    device_type = j_device_get_type(device);

    DBG_I(DJOS_DRIVER, ("JDRV: Attach dev %p [%s] type %d\n", device,
        j_device_get_nameunit(device), device_type));

    /* Find the best suitable driver for this device */
    TAILQ_FOREACH(curr, &driver_list, next)
    {
        if(curr->type != device_type)
            continue;

        /* Allocate owner context, if the driver needs it */
        if(curr->softsize)
            device->owner_ctx = jmalloc(curr->softsize, M_ZERO);

        match_level = curr->methods->match(device);

        if(curr->softsize)
            jfree(device->owner_ctx);
        device->owner_ctx = NULL;

        if(match_level > best_level)
        {
            best = curr;
            best_level = match_level;
        }
    }

    if(!best)
    {
        rc = UWE_NOENT;
        goto Error;
    }

    /* Allocate owner context, if the driver needs it */
    if(best->softsize)
        device->owner_ctx = jmalloc(best->softsize, M_ZERO);

    /* Give it a nice name */
    j_device_set_nameunit(device, best->name, best->curr_unit++);

    rc = best->methods->attach(device);
    if(rc)
    {
        goto Error;
    }

    if(!j_device_is_owned(device))
    {
        /* If the jdrv field is NULL, owner didn't call j_device_own() during
         * the attach -- call it directly. This should be an error once all
         * drivers will implement the own and disown methods. */
        rc = j_device_own(device, best, NULL, NULL);
        if(rc)
        {
            DBG_E(DJOS_DRIVER, ("JDRV: Creator rejected own for device %p "
                "(%s) - %s\n", device, j_device_get_nameunit(device),
                uwe_str(rc)));

            j_device_detach(device);
            goto Error;
        }
    }

    return UWE_OK;

Error:
    if(device->owner_ctx)
    {
        jfree(device->owner_ctx);
        device->owner_ctx = NULL;
    }
    j_device_set_nameunit(device, "unknown", unknown_unit++);
    return rc;
}

/**
 * Function name:  j_driver_attach
 * Description:    Attempts to attach a device to a registered driver.
 * Parameters:
 *     @device: (IN) Handle to a JOS device
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t j_driver_attach(j_device_h device)
{
    result_t rc;

    LOCK_CHECK;
    CORE_SAFE_CHECK;
    DBG_ASSERT_CTX(J_CTX_PNP | J_CTX_OTHER);

    LOG_D(APP,"j_driver_attach\r\n");
    if(!device)
    {
        DBG_E(DJOS_DRIVER, ("JDRV: Device is NULL\n"));
        return UWE_INVAL;
    }

    DBG_I(DJOS_DRIVER, ("JDEV: Attach type %d, dev %p\n", device->type,
        device));

    rc = jdriver_dev_attach(device);
    if(rc)
    {
        DBG_I(DJOS_DRIVER, ("JDRV: Didn't find suitable driver for device "
            "%p\n", device));

        return rc;
    }

    DBG_RUN(DJOS_DRIVER, DL_VERBOSE, dump_free_devices());

    return UWE_OK;
}

result_t j_device_ioctl(j_device_h dev, drv_ioctl_t ioctl, void *arg)
{
    /* Check whether the device is not attached or the driver does not support
       IOCTLs */
    if(!dev->jdrv || !dev->jdrv->methods->ioctl)
        return UWE_INVAL;

    return dev->jdrv->methods->ioctl(dev, ioctl, arg);
}

result_t j_device_suspend(j_device_h dev)
{
    result_t rc = UWE_OK;

    /* If the device is attached and the driver supports device suspend --
       suspend the device */
    if(dev->jdrv && dev->jdrv->methods->suspend)
        rc = dev->jdrv->methods->suspend(dev);

    return rc;
}

result_t j_device_resume(j_device_h dev)
{
    result_t rc = UWE_OK;

    /* If the device is attached and the driver supports resume */
    if(dev->jdrv && dev->jdrv->methods->resume)
        rc = dev->jdrv->methods->resume(dev);

    return rc;
}

/**
 * Function name:  j_device_own
 * Description:    Claims ownership of a device.
 *                 This function should be called from the attach callback of
 *                 the driver that attempts to claim ownership of the device.
 * Parameters:
 *     @device:      (IN)  Handle to a JOS device
 *     @owner_drv_h: (IN)  Handle to the calling driver, received during
 *                         registration (see j_driver_register() @drv_h)
 *     @owner_cbs:   (IN)  Pointer to the calling driver's callbacks
 *                         structure
 *     @creator_methods: (OUT) Pointer to a handle to a creator-driver-methods
 *                         structure, to be filled by the creator
 *
 * Return value:   UWE_OK on success, otherwise an error code
 * Scope:          Global
 **/
result_t j_device_own(j_device_h device, j_driver_h owner_drv_h,
    owner_cbs_h owner_cbs, creator_methods_h *creator_methods)
{
    result_t rc;
    DECLARE_FNAME("j_device_own");

    KASSERT0(device);
    KASSERT0(owner_drv_h);

    /* TODO - Consider moving function to JOS device */
    /* Notify the creator of the device that a driver claims ownership on the
     * device */
    if(device->own)
    {
        rc = device->own(device, owner_cbs, creator_methods);
        if(rc)
        {
            DBG_E(DJOS_DRIVER, ("%s: Failed, device %p (%s) - %s\n", FNAME,
                device, j_device_get_nameunit(device), uwe_str(rc)));
            return rc;
        }
    }

    /* Device is owned by driver (device->jdrv) */
    device->jdrv = owner_drv_h;
    device->jdrv->usage_count++;
    j_device_remove_from_unmatched_list(device);
    TAILQ_INSERT_TAIL(&(device->jdrv->dev_list), device, next_in_drv_list);

    return UWE_OK;
}

/**
 * Function name:  j_device_disown
 * Description:    Relinquishes ownership of a device.
 *                 This function should be called from the detach callback of
 *                 the device's current owner.
 * Parameters:
 *     @device: (IN) Handle to a JOS device
 *
 * Return value:   None
 * Scope:          Global
 **/
void j_device_disown(j_device_h device)
{
    DECLARE_FNAME("j_device_disown");

    /* A disowned device must have no children */
    KASSERT(!j_device_get_first_child(device), ("%s: Device %p (%s) has "
        "children! First child %p (%s)\n", FNAME, device,
        j_device_get_nameunit(device), j_device_get_first_child(device),
        j_device_get_nameunit(j_device_get_first_child(device))));

    DBG_I(DJOS_DRIVER, ("%s: Entered, device %p (%s)\n", FNAME, device,
        j_device_get_nameunit(device)));

    /* Notify the creator of the device that the owner driver relinquishes
     * ownership on the device */
    if(device->disown)
    {
        DBG_X(DJOS_DRIVER, ("%s: Calling device %p (%s) disown method\n",
            FNAME, device, j_device_get_nameunit(device)));
        device->disown(device);
    }

    /* Remove the device from the driver's device list */
#ifdef J_DEBUG
    {
        uint8_t found;

        TAILQ_FIND_AND_REMOVE(&device->jdrv->dev_list, device, j_device_h,
            next_in_drv_list, found);
        KASSERT0(found);
    }
#else
    TAILQ_REMOVE(&device->jdrv->dev_list, device, next_in_drv_list);
#endif
    device->jdrv->usage_count--;

    /* Owner is no longer responsible for the device */
    device->jdrv = NULL;

    /* Add the device to the unmatched list */
    j_device_add_to_unmatched_list(device);
}

j_device_h j_driver_get_first_owned_device(j_driver_h drv)
{
    KASSERT0(drv);

    return TAILQ_FIRST(&drv->dev_list);
}

j_device_h j_driver_get_next_owned_device(j_device_h dev)
{
    KASSERT0(dev);

    return TAILQ_NEXT(dev, next_in_drv_list);
}

