/* (c) 2012 Jungo Ltd. All Rights Reserved. Jungo Confidential */
#include <jos.h>

#ifdef CONFIG_JHOST
# include <dev/usb/usbdi.h>
# include <dev/usb/usbdivar.h>
# include <dev/usb/usbdi_util.h>
#endif

typedef struct jos_event_handler_s {
    TAILQ_ENTRY(jos_event_handler_s)    next;
    j_event_handler_h                   handler;
} jos_event_handler_t;

static TAILQ_HEAD(handlers, jos_event_handler_s) event_handlers;

result_t j_event_init(void)
{
    DECLARE_FNAME("j_event_init");

    DBG_I(DJOS_EVENT, ("%s: Entered\n", FNAME));
    TAILQ_INIT(&event_handlers);

    return UWE_OK;
}

void j_event_uninit(void)
{
    jos_event_handler_t *curr;
    DECLARE_FNAME("j_event_uninit");

    DBG_I(DJOS_DRIVER, ("%s: Entered\n", FNAME));

    while ((curr = TAILQ_FIRST(&event_handlers)) != NULL)
    {
        DBG_I(DJOS_DRIVER, ("%s: Removing event handler %p\n", FNAME, curr));

        TAILQ_REMOVE_HEAD(&event_handlers, next);
        jfree(curr);
        curr = NULL;
    }
}

result_t j_event_dispatch(j_event_t msg, mdev_t dev, j_event_param_h param)
{
    jos_event_handler_t *cur_handler;
    result_t ret_rc = UWE_OK;
    result_t rc;
    DECLARE_FNAME("j_event_dispatch");

    TAILQ_FOREACH(cur_handler, &event_handlers, next)
    {
        rc = cur_handler->handler(msg, dev, param);
        if(rc)
        {
            /* TODO - If one handler returned error - do we need to continue? */
            DBG_E(DJOS_EVENT, ("%s: Msg %s failed - %s\n", FNAME,
                j_event_str(msg), uwe_str(rc)));
            if(!ret_rc)
                ret_rc = rc;
        }
    }

    return ret_rc;
}

result_t j_event_handler_register(j_event_handler_h handler)
{
    jos_event_handler_t *jhandler;
    DECLARE_FNAME("j_event_handler_register");

    DBG_I(DJOS_EVENT, ("%s: Entered, handler %p\n", FNAME, handler));
    KASSERT0(handler);
    if(!handler)
        return UWE_INVAL;

    jhandler = (jos_event_handler_t *)jmalloc(sizeof(jos_event_handler_t), M_ZERO);
    jhandler->handler = handler;
    TAILQ_INSERT_TAIL(&event_handlers, jhandler, next);

    return UWE_OK;
}

void j_event_handler_unregister(j_event_handler_h handler)
{
    jos_event_handler_t *jhandler;
    DECLARE_FNAME("j_event_handler_unregister");

    jhandler = TAILQ_FIRST(&event_handlers);

    TAILQ_FOREACH(jhandler, &event_handlers, next)
    {
        if(jhandler->handler == handler)
            break;
    }

    if(!jhandler)
    {
        DBG_E(DJOS_EVENT, ("%s: Handler %p is not registered\n", FNAME,
            handler));
        return;
    }

    DBG_I(DJOS_EVENT, ("%s: Unregistering handler %p\n", FNAME, handler));

    TAILQ_REMOVE(&event_handlers, jhandler, next);
    jfree(jhandler);
    jhandler = NULL;
}

#ifdef CONFIG_JHOST
result_t j_reset_device(mdev_t dev)
{
    usbd_device_handle udev = (usbd_device_handle)dev;
    DECLARE_FNAME("j_reset_device");

    KASSERT0(udev);

    DBG_V(DJOS_EVENT, ("%s: For Device %p (BDEV is %p)\n", FNAME, udev,
        udev->bdev));

    if(!udev->bdev)
    {
        DBG_E(DJOS_EVENT, ("%s: A device may be reset only after the initial "
            "enumeration ended successfully\n", FNAME));
        return UWE_NOTSUP;
    }

    return j_device_ioctl(j_device_get_parent(udev->bdev),
        DRV_IOCTL_PWR_DEVICE_REATTACH, udev);
}

result_t j_get_dev_desc(mdev_t mdev, j_dev_desc_t *desc)
{
    usbd_device_handle dev = (usbd_device_handle)mdev;
    DECLARE_FNAME("j_get_dev_desc");

    if(!desc)
        return UWE_INVAL;

    DBG_I(DJOS_EVENT, ("%s: Info for %p\n", FNAME, dev));

    j_memset(desc, 0, sizeof(j_dev_desc_t));

    desc->dev_class     = dev->ddesc.bDeviceClass;
    desc->subclass      = dev->ddesc.bDeviceSubClass;
    desc->protocol      = dev->ddesc.bDeviceProtocol;

    desc->index_manufacturer  = dev->ddesc.iManufacturer;
    desc->index_product       = dev->ddesc.iProduct;
    desc->index_serial_number = dev->ddesc.iSerialNumber;

    if(dev->ddesc.idVendor)
    {
        desc->vendor        = UGETW(dev->ddesc.idVendor);
        desc->product       = UGETW(dev->ddesc.idProduct);
        desc->version       = UGETW(dev->ddesc.bcdDevice);
    }

    desc->parent        = (mdev_t)dev->myhub;
    desc->port          = dev->myport;
    desc->address       = dev->address;
    desc->config        = dev->config;
    desc->depth         = dev->depth;
    desc->speed         = dev->speed;
    desc->self_powered  = dev->self_powered;
    desc->power         = dev->power;

    if(dev->bdev)
    {
        desc->name      = j_device_get_nameunit(dev->bdev);
        desc->desc      = j_device_get_desc(dev->bdev);
    }

    return UWE_OK;
}

/**
 * Function name:  j_get_string
 * Description:    Get a string from string index for a mdev_t handle
 * Parameters:
 *     @mdev: (IN) A mdev_t handle.
 *     @si:   (IN) USB String Index
 *
 * Return value:   String or NULL in case of failure
 *                 Caller must free the string.
 *
 * Scope:          Global
 **/
char *j_get_string(mdev_t mdev, uint8_t si)
{
    usbd_device_handle dev = (usbd_device_handle)mdev;
    char *str;
    DECLARE_FNAME("j_get_string");

    DBG_V(DJOS_EVENT, ("%s: String id %d\n", FNAME, si));

    str = (char *)jmalloc(USB_MAX_STRING_LEN, M_ZERO);
    if(!usbd_get_string(dev, si, str))
    {
        DBG_E(DJOS_EVENT, ("%s: usbd_get_string() failed\n", FNAME));
        jfree(str);
        str = NULL;
        
        return NULL;
    }

    return str;
}
#endif

#ifdef J_DEBUG
static const char *event_str[EVT_LAST + 1];

static void event_init_str(void)
{
    event_str[EVT_NEW_DEVICE] = "New device connected";
    event_str[EVT_NEW_DEVICE_READY] = "Device ready for operation";
    event_str[EVT_NEW_DEVICE_NO_DRIVER] = "No driver found for device";
    event_str[EVT_NEW_DEVICE_ATTACHING] = "Attaching new device";
    event_str[EVT_NEW_DEVICE_ATTACH_DENIED] = "Device attach denied by handler";
    event_str[EVT_DEVICE_DETACHING] = "Detaching device";
    event_str[EVT_DEVICE_DETACHED] = "Detached device";
    event_str[EVT_DEVICE_DISCONNECTED] = "Device disconnected";
    event_str[EVT_DEVICE_SUSPENDED] = "Device was suspended";
    event_str[EVT_DEVICE_RESUMED] = "Device was resumed";
    event_str[EVT_DEVICE_CONFIG_SETTING] = "Device setting new config";
    event_str[EVT_DEVICE_CONFIG_SET] = "Device new config set";

    event_str[EVT_UCOMP_ATTACH_SUBDEV] = "Attaching subdevice to ucompdev";
    event_str[EVT_UCOMP_DETACHING_SUBDEV] = "Starting subdevice detach";
    event_str[EVT_UCOMP_DETACHED_SUBDEV] = "Detached subdevice from ucompdev";

    event_str[EVT_NETWORK_IFC_ATTACH] = "Network device attached";
    event_str[EVT_NETWORK_IFC_DETACH] = "Network device detached";

    event_str[EVT_BLOCK_DEV_READY] = "Block Device Ready";
    event_str[EVT_BLOCK_DEV_REMOVED] = "Block Device Removed";

    event_str[EVT_ERROR_CANNOT_OPEN_EP0] = "Cannot open endpoint zero";
    event_str[EVT_ERROR_CANNOT_ALLOC_ADDR] = "Cannot find a free address";
    event_str[EVT_ERROR_CANNOT_SET_ADDR] = "Failed setting device address";
    event_str[EVT_ERROR_CANNOT_GET_FIRST_DESC] = "Failed getting first "
        "descriptor";
    event_str[EVT_ERROR_BAD_DEV_DESC] = "Invalid device descriptor";
    event_str[EVT_ERROR_BAD_BOS_DESC] = "Invalid BOS descriptor";
    event_str[EVT_ERROR_CANNOT_GET_CONFIG] = "Failed setting configuration";
    event_str[EVT_ERROR_CANNOT_ATTACH] = "Failed attaching";
    event_str[EVT_ERROR_CANNOT_SET_CONFIG] = "Failed setting configuration";
    event_str[EVT_ERROR_RESET_FAILED] = "Port reset failed";
    event_str[EVT_ERROR_OVERCURRENT] = "Over-current detected";

    event_str[EVT_ERROR_HUB_TOO_DEEP] = "New hub is too deep";
    event_str[EVT_ERROR_HUB_GET_DESC] = "Failed getting hub descriptor";
    event_str[EVT_ERROR_HUB_BUSPWR_ATTACHED_TO_BUSPWR] =
        "Bus powered hub attached to another bus powered hub";
    event_str[EVT_ERROR_HUB_NO_INTERFACE] = "No interface in hub descriptor";
    event_str[EVT_ERROR_HUB_NO_ENDPOINT] = "No endpoint in hub descriptor";
    event_str[EVT_ERROR_HUB_BAD_ENDPOINT] = "Invalid interrupt endpoint in hub";
    event_str[EVT_ERROR_HUB_CANNOT_OPEN_PIPE] = "Cannot open interrupt pipe";

    event_str[EVT_NOTIFY_HOST_CONNECT] = "Host is connected";
    event_str[EVT_NOTIFY_HOST_SUSPEND] = "Host is suspended";
    event_str[EVT_NOTIFY_HOST_RESUME] = "Host requesting resume";

    event_str[EVT_NOTIFY_DRIVE_VBUS_ON] = "Turn on vbus";
    event_str[EVT_NOTIFY_DRIVE_VBUS_OFF] = "Turn off vbus";

    event_str[EVT_NOTIFY_DEVICE_CONNECT] = "The Device is connected to a Host";
    event_str[EVT_NOTIFY_DEVICE_DISCONNECT] = "The Device is disconnected from "
        "a Host";
    event_str[EVT_NOTIFY_DEVICE_SUSPEND] = "The Device is suspended";
    event_str[EVT_NOTIFY_DEVICE_RESUME] = "The Device detected resume signal";
    event_str[EVT_NOTIFY_DEVICE_RESUME_COMPLETED] = "The Device is resumed";
    event_str[EVT_NOTIFY_DEVICE_REMOTE_WAKEUP] = "The Device is issuing a "
        "remote wakeup signal";
    event_str[EVT_NOTIFY_DEVICE_CONFIGURED] = "The Device is configured";
    event_str[EVT_NOTIFY_DEVICE_UNCONFIGURED] = "The Device is not configured";
    event_str[EVT_NOTIFY_DEVICE_RESET] = "The Device detected reset signal";

    event_str[EVT_NOTIFY_OTG_IDLE] = "OTG is idle";
    event_str[EVT_NOTIFY_OTG_BUSY] = "OTG requesting resume";

    event_str[EVT_ERROR_WIFI_CIPHER] = "Wi-Fi CIPHER error";

    event_str[EVT_PORT_VDK_MOUNT] = "VDK mount";
    event_str[EVT_PORT_VDK_UNMOUNT] = "VDK unmount";

    event_str[EVT_LAST] = "Invalid message !";
}

const char *j_event_str(j_event_t msg)
{
    if(!event_str[0])
        event_init_str();

    return event_str[msg];
}

#ifdef CONFIG_JHOST
void j_dump_dev_desc(mdev_t dev)
{
    j_dev_desc_t desc;
    result_t rc;

    rc = j_get_dev_desc(dev, &desc);
    if(rc)
        return;

    jprintf(" Device info %p (parent %p)\n", dev, desc.parent);

    if(desc.name)
        jprintf("   %s [%s]\n", desc.name, desc.desc ? desc.desc : "NULL");

    jprintf("   Class : %02x,   Sub-Class: %02x,   Protocol: %02x\n",
        desc.dev_class, desc.subclass, desc.protocol);

    jprintf("   Vendor: %04x, Product  : %04x, Version : %04x\n",
        desc.vendor, desc.product, desc.version);

    jprintf("   Addr: %02x, Config: %02x, Depth: %02d, Port: %02d, %s-speed, "
        "%s-powered (%dmA)\n",
        desc.address, desc.config, desc.depth, desc.port,
        (desc.speed == USB_SPEED_LOW ? "Low" :
        (desc.speed == USB_SPEED_FULL ? "Full" :
        (desc.speed == USB_SPEED_HIGH ? "High" : "Unknown"))),
        desc.self_powered ? "Self" : "Bus", desc.power);
}
#endif
#endif

