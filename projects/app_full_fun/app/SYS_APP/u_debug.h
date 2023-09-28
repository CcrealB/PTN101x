
#ifndef _U_DEBUG_H_
#define _U_DEBUG_H_

#include "includes.h"

#define std_printf        os_printf
#define DBG_LOG_INFO(fmt,...)       std_printf("[I]"fmt, ##__VA_ARGS__)
#define DBG_LOG_FUNC(fmt,...)       std_printf("[F:%s()]"fmt, __FUNCTION__, ##__VA_ARGS__)
#define DBG_LOG_WARN(fmt,...)       std_printf("[W:%s():%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define DBG_LOG_ERR(fmt,...)        std_printf("[E:%s():%d]"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

extern int32_t os_printf(const char *fmt, ...);

#if 1 /* Tick Timer */

#include "bautil.h"
#include "spr_defs.h"

//open any one or both
#define TICK_TIMER_TIMESTAMP_EN     1
#define TICK_TIMER_EXTEND_EN        0

#if TICK_TIMER_TIMESTAMP_EN
#define HW_SYS_TICK_GETL()          get_spr(SPR_TTMTS_LO)
#define HW_SYS_TICK_GETH()          get_spr(SPR_TTMTS_HI)
#define HW_SYS_TICK_GET64b()        (((uint64_t)get_spr(SPR_TTMTS_HI) << 32) | get_spr(SPR_TTMTS_LO))
#endif

#if TICK_TIMER_EXTEND_EN
//get count
#define HW_SYS_TICK_GET()           get_spr(SPR_TTCR)
extern void hw_tick_timer_init(void);
#endif

#endif /* Tick Timer */


#define DBG_REG_GET()      (REG_SYSTEM_0x1C)
#define DBG_REG_SET(reg)   (REG_SYSTEM_0x1C = reg)

#endif /* _U_DEBUG_H_ */
