/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#define INCLUDE_DEVICE
#define INCLUDE_GENERAL
#include "jos_internal.h"
#include "jos_driver.h"

#define MAXINT_LEGTH                      10

j_device_h j_device_get_first_child(j_device_h device)
{
    KASSERT0(device);

    return TAILQ_FIRST(&device->children_list);
}

j_device_h j_device_get_next_child(j_device_h device, j_device_h curr_child)
{
    UNUSED_VAR(device);
    KASSERT0(device && curr_child);

    return TAILQ_NEXT(curr_child, next_child);
}

j_device_type_t j_device_get_type(j_device_h dev)
{
    KASSERT0(dev);
    if(!dev)
    {
        DBG_E(DJOS_DEVICE, ("JDRV: j_device_get_type - Invalid Device (NULL)"
            "\n"));
        return J_DEVICE_TYPE_INVALID;
    }

    return dev->type;
}

void j_device_set_creator_ctx(j_device_h dev, void *ctx)
{
    KASSERT0(dev);
    dev->creator_ctx = ctx;
}

void *j_device_get_creator_ctx(j_device_h dev)
{
    if(!dev)
    {
        DBG_E(DJOS_DEVICE, ("j_device_get_creator_ctx: NULL Device\n"));
        return NULL;
    }

    return dev->creator_ctx;
}

/* This function is only called from within this file. To enable calling it
 * from other files, add its forward declaration to jos_internal.h. */
static void j_device_set_aa(j_device_h dev, j_device_aa_h attach_args)
{
    dev->dev_aa = attach_args;
}

j_device_aa_h j_device_get_aa(j_device_h dev)
{
    if(!dev)
    {
        DBG_E(DJOS_DEVICE, ("JDRV: j_device_get_aa - Invalid Device (NULL)\n"));
        return NULL;
    }

    DBG_V(DJOS_DEVICE, ("JDEV: Getting attach arguments of %p\n", dev));

    return dev->dev_aa;
}

void *j_device_get_ivars(j_device_h dev)
{
    KASSERT(dev, ("device_get_ivars(NULL, ...)\n"));

    DBG_V(DJOS_DEVICE, ("JDEV: Getting ivars of %p\n", dev));

    return dev->ivars;
}

void j_device_set_ivars(j_device_h dev, void *ivars)
{
    KASSERT(dev, ("device_set_ivars(NULL, ...)\n"));

    DBG_V(DJOS_DEVICE, ("JDEV: Setting ivars of %p to %p\n", dev, ivars));

    dev->ivars = ivars;
}

const char *j_device_get_nameunit(j_device_h dev)
{
    if(!dev)
    {
        DBG_E(DJOS_DEVICE, ("j_device_get_nameunit: Invalid Device (NULL)\n"));
        return "";
    }

    DBG_V(DJOS_DEVICE, ("JDEV: Get nameunit of %p [%s]\n",
        dev, dev->nameunit ? dev->nameunit : "none"));

    return (dev->nameunit ? dev->nameunit : "");
}

void j_device_set_nameunit(j_device_h dev, const char *name, uint32_t unit)
{
    int32_t len;

    KASSERT(dev, ("device_set_nameunit(NULL, ...)\n"));

    DBG_V(DJOS_DEVICE, ("JDEV: Set nameunit of %p [%s] %ld\n",
        dev, name ? name : "none", unit));

    if (dev->nameunit)
    {
        jfree(dev->nameunit);
        dev->nameunit = NULL;
    }

    if(!name)
    {
        dev->nameunit = NULL;
        dev->unit = unit;
        return;
    }

    len = j_strlen(name) + MAXINT_LEGTH;
    dev->nameunit = (char *)jmalloc(len, M_ZERO);
    dev->unit = unit;
    j_snprintf(dev->nameunit, len, "%s%ld", name, unit);
}

j_device_h j_device_get_parent(j_device_h dev)
{
    KASSERT(dev, ("device_get_parent(NULL, ...)\n"));

    DBG_V(DJOS_DEVICE, ("JDEV: Get parent of %p\n", dev));

    return dev->parent;
}

void *j_device_get_owner_ctx(j_device_h dev)
{
    if(!dev)
    {
        DBG_E(DJOS_DEVICE, ("j_device_get_owner_ctx: Invalid Device (NULL)\n"));
        return NULL;
    }

    DBG_V(DJOS_DEVICE, ("JDEV: Get softc %p\n", dev));

    return dev->owner_ctx;
}

uint32_t j_device_get_unit(j_device_h dev)
{
    KASSERT(dev, ("device_get_unit(NULL, ...)\n"));

    DBG_V(DJOS_DEVICE, ("JDEV: Get unit %p\n", dev));

    return dev->unit;
}

BOOL j_device_is_owned(j_device_h dev)
{
    KASSERT(dev, ("device_is_owned(NULL, ...)\n"));

    return (dev->jdrv ? TRUE : FALSE);
}

j_driver_h j_device_get_owner(j_device_h device)
{
    if(device)
        return device->jdrv;
    return NULL;
}

/* Function name:  j_device_probe_and_attach
 * Description:    Attempts to attach a JOS device to a matching JOS driver.
 *                 The function probes all JOS-registered drivers that match the
 *                 device's type, and executes the device-attach method of the
 *                 best-matched driver (if found) -- which will become the
 *                 device's owner driver.
 * Parameters:
 *      @dev: (IN) Handle to a JOS device
 *
 * Return value:   UWE_OK on success, otherwise an error code
 */
result_t j_device_probe_and_attach(j_device_h dev)
{
    LOCK_CHECK;

    LOG_D(APP,"j_device_probe_and_attach\r\n");

    return j_driver_attach(dev);
}

void j_device_set_desc(j_device_h dev, const char *desc)
{
    KASSERT(dev, ("j_device_set_desc(NULL, ...)\n"));

    if(!desc)
        return;

    DBG_V(DJOS_DEVICE, ("JDEV: Setting desc for %p to [%s]\n", dev, desc));

    if(dev->desc)
    {
        jfree(dev->desc);
        dev->desc = NULL;
    }
    
    dev->desc = j_strdup(desc);
}

char *j_device_get_desc(j_device_h dev)
{
    return (dev ? dev->desc : "");
}

result_t j_device_detach(j_device_h dev)
{
    DBG_I(DJOS_DEVICE, ("JDEV: Detach %p (%s)\n", dev,
        j_device_get_nameunit(dev)));

    j_driver_detach(dev);

    /* Try re-attaching - consider moving to task */
    j_device_retry_attach();

    return UWE_OK;
}

#ifdef J_DEBUG
const char *j_device_type_str(j_device_type_t type)
{
    static j_code2str_t str[] = {
        { J_DEVICE_TYPE_CONTROLLER, "Controller" },
        { J_DEVICE_TYPE_USB, "USB" },
        { J_DEVICE_TYPE_TTY, "Terminal" },
        { J_DEVICE_TYPE_BLUETOOTH_UNIT, "BT Unit" },
        { J_DEVICE_TYPE_BLUETOOTH_DEV, "BT Device" },
        { J_DEVICE_TYPE_BLUETOOTH_SERVICE, "BT Service" },
        { J_DEVICE_TYPE_CLASS_VIDEO, "Video" },
        { J_DEVICE_TYPE_CLASS_MASS, "Mass" },
        { J_DEVICE_TYPE_CLASS_SERIAL, "Serial" },
        { J_DEVICE_TYPE_CLASS_USB_AUDIO, "USB Audio" },
        { J_DEVICE_TYPE_CLASS_AUDIO, "Audio" },
        { J_DEVICE_TYPE_TR_HID, "HID Transport" },
        { J_DEVICE_TYPE_TR_IPOD, "iPod Transport" },
        { J_DEVICE_TYPE_CLASS_HID, "HID Protocol" },
        { J_DEVICE_TYPE_CLASS_NCM, "NCM" },
        { J_CODE2STR_LAST, "Unknown device type" }
    };

    return j_code2str(type, str);
}

void j_device_assert_type(j_device_h device, j_device_type_t type, char *file,
    int32_t line)
{
    j_device_type_t devt = j_device_get_type(device);
    DECLARE_FNAME("j_device_assert_type");

    if(devt != type)
    {
        DBG_E(DJOS_DEVICE, ("%s: Device %p - wrong type 0x%x, expected 0x%x "
            "[%s:%d]\n", FNAME, device, devt, type, file, line));
        KASSERT0(0);
    }

    DBG_X(DJOS_DEVICE, ("%s: device %p type %s match [%s:%d]\n", FNAME, device,
        j_device_type_str(devt), file, line));
}
#endif

void j_device_delete(j_device_h dev)
{
    result_t rc;
    j_device_h parent;

    LOCK_CHECK;

    if(!dev)
    {
        DBG_E(DJOS_DEVICE, ("JDEV: delete called with NULL device\n"));
        return;
    }

    DBG_I(DJOS_DEVICE, ("JDEV: Delete %p (%s), type %s, desc %s\n", dev,
        j_device_get_nameunit(dev), j_device_type_str(dev->type),
        j_device_get_desc(dev)));

    if(j_device_is_owned(dev))
        j_driver_detach(dev);

    /* Remove device from parent's list */
    parent = j_device_get_parent(dev);
    if(parent)
    {
        uint8_t found;

        TAILQ_FIND_AND_REMOVE(&parent->children_list, dev, j_device_h,
            next_child, found);
        KASSERT(found, ("JDEV: Couldn't find device %p in parent %p list\n",
            dev, parent));
        UNUSED_VAR(found);
    }

    /* The device should be in unmatched list -- either because it was initially
     * not owned, or due to the call to j_driver_detach() above (resulting in
     * a call to j_device_disown(), which adds the device to the unmatched
     * devices list, after disowning it). */
    rc = j_device_remove_from_unmatched_list(dev);
    if(rc)
    {
        DBG_E(DJOS_DEVICE, ("JDEV: Failed removing device %p - %s\n", dev,
            uwe_str(rc)));
    }

    if(dev->nameunit)
    {
        jfree(dev->nameunit);
        dev->nameunit = NULL;
    }

    if(dev->desc)
    {
        jfree(dev->desc);
        dev->desc = NULL;
    }
    
    jfree(dev);
    dev = NULL;
}

j_device_h j_device_add(j_device_type_t type, j_device_h parent,
    j_device_aa_h attach_args, device_own_t own, device_disown_t disown)
{
    j_device_h dev;

    LOCK_CHECK;

    DBG_I(DJOS_DEVICE, ("JDEV: Add a new child to parent %p\n", parent));

    dev = (j_device_h)jmalloc(sizeof(struct j_device_s), M_ZERO);

    DBG_X(DJOS_DEVICE, ("JDEV: Added a new device %p, type %d to parent %p "
        "(%s)\n", dev, type, parent, j_device_get_nameunit(parent)));

    dev->parent = parent;
    dev->type = type;
    dev->own = own;
    dev->disown = disown;
    j_device_set_aa(dev, attach_args);
    TAILQ_INIT(&dev->children_list);

    /* Add the new child to the parent's children list */
    if(parent)
        TAILQ_INSERT_TAIL(&(parent->children_list), dev, next_child);

    /* Add new device to unmatched-list */
    j_device_add_to_unmatched_list(dev);

    return dev;
}

