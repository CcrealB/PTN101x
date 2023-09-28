2021-11-17：
一、usb device驱动文件结构修改
        usb_driver-----------------------------------------------usb-------------------------------------------------usb_dev-------------------------------------------------------usb_host------                                                                        
          |-------api_usb.c(应用层接口)                           |--------usb.c(usb device列举封装)                  |+++++audio_hid																									|----AplUsbHost.c(udisk app-dirver)
          |-------drv_usb.c(应用层驱动接口)                       |--------usbclass.c(usb device类处理封装)           |      |-----audio_hid.c(audio&hid class驱动)
          |-------driver_usb.c(usb core硬件驱动)                                                                      |      |-----audio_hid_desc.c(audio&hid描述符驱动)
          |-------drv_ll_usb.c(usb硬件驱动)                                                                       |
          |-------drv_usbhost.c(usb host core驱动)                                                                    |+++++udisk
                                                                                                                      |      |++++inc
                                                                                                                      |      |------bot.c(mass storage class驱动)
                                                                                                                      |      |------bot_desc.c(mass storage描述符驱动)

二、usb device驱动功能修改
	1、将udisk和audio&hid的驱动整合
	2、将驱动动态化，根据需要加载相应的usb device驱动
	3、需要添加新的device的话，可以在usb_dev下仿照已有的udisk或者audio_hid class创建新的device
	
三、usb_host驱动
	1、drv_usbhost.c:usb host core driver，实现usb host的枚举、配置以及用户层ctrl endpoint API
	2、AplUsbHost.c:usb host udisk app-driver，实现udisk的协议、scsi命令传输API、udisk读写API