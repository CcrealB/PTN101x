
/**
 * **************************************************************************************
 * @file    irda_comm.h
 * 
 * @author  Borg Xiao
 * @date    20230906 create this file
 * 
 * @copyright 2023 Partner Co.,Ltd. All rights reserved.
 * 
 * **************************************************************************************
 * */

#ifndef _IRDA_COMM_H_
#define _IRDA_COMM_H_


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
#include "driver_irda.h"

/* ------------------------------------------------------------------------- */
extern int32_t os_printf(const char *fmt, ...);
#define IRDA_LOG_E(fmt,...)         os_printf("[IR|E:%d]"fmt, __LINE__, ##__VA_ARGS__)
#define IRDA_LOG_I(fmt,...)         os_printf("[IR|I]"fmt,##__VA_ARGS__)
#define IRDA_LOG_D(fmt,...)         //os_printf(fmt,##__VA_ARGS__)


/* ------------------------------------------------------------------------- */

extern void irda_rx_evt_set_rx_cmp(void);
extern void irda_rx_evt_set_repete(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* _IRDA_COMM_H_ */

