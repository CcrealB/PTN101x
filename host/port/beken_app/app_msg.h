#ifndef _APP_MSG_H_
#define _APP_MSG_H_

#define AMSG_DEBUG

#ifdef AMSG_DEBUG
    #define AMSG_PRT      os_printf
#else
    #define AMSG_PRT 
#endif

extern void common_msg_handler(MSG_T *msg_ptr);
extern void bt_msg_handler(MSG_T *msg_ptr);
extern void sd_music_msg_handler(MSG_T *msg_ptr);

#if (defined(CONFIG_APP_UDISK))
extern void udisk_msg_handler(MSG_T *msg_ptr);
#endif

extern void linein_msg_handler(MSG_T *msg_ptr);

#endif
