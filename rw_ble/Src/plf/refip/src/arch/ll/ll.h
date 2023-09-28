/**
 ****************************************************************************************
 *
 * @file ll.h
 *
 * @brief Declaration of low level functions.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */

#ifndef LL_H_
#define LL_H_


#include <stdint.h>


#include "compiler.h"
#include "bautil.h"			////
#include "user_config.h"		//// Debug
#include "drv_system.h"			////
#define KEIL_INTRINSICS_INTERRUPT
//#define __INLINE                 static __inline //__inline__		////inline

#define PTN101TEST_MCU_INT_DISABLE	0	////1	//// Cnacel INT Disable for debug
#define PTN101TEST_MCU_INT_WORKAROUND	1	////Only turn off RWIP, RWBT, RWLE

/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */

/* keil Compiler intrinsics for controlling IRQ and FIQ interrupts
*/
#if 0	////
	extern void __enable_irq(void);
	extern void __enable_fiq(void);
	extern int  __disable_irq(void);
	extern int  __disable_fiq(void);
#endif

#if 1	////TODO, FIXME
  #if(PTN101TEST_MCU_INT_DISABLE == 1)
	#define GLOBAL_INT_START()
  #else
  	#if(PTN101TEST_MCU_INT_WORKAROUND == 1)
		#define GLOBAL_INT_START()		system_peri_mcu_irq_enable(SYS_PERI_IRQ_RWBT0 | SYS_PERI_IRQ_RWBT1 | SYS_PERI_IRQ_RWBT2 )
	#else
		#define GLOBAL_INT_START()		cpu_set_interrupts_enabled(1)
	#endif
  #endif
#else
	#define GLOBAL_INT_START(); \
	do { \
			__enable_fiq(); \
			__enable_irq(); \
	} while(0);
#endif
/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */
#if 1	////
  #if(PTN101TEST_MCU_INT_DISABLE == 1)
	#define GLOBAL_INT_STOP()
  #else
	#if(PTN101TEST_MCU_INT_WORKAROUND == 1)
		#define GLOBAL_INT_STOP()		system_peri_mcu_irq_disable(SYS_PERI_IRQ_RWBT0 | SYS_PERI_IRQ_RWBT1 | SYS_PERI_IRQ_RWBT2 )
	#else
		#define GLOBAL_INT_STOP()		cpu_set_interrupts_enabled(0)
	#endif
  #endif
#else
	#define GLOBAL_INT_STOP();		\
	do { \
			__disable_fiq(); \
			__disable_irq(); \
	} while(0);
#endif

/* * @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#if 1	////
#define GLOBAL_INT_DISABLE(); \
  do { \
          uint32_t cpu_flags; \
          SYSirq_Disable_Interrupts_Except(&cpu_flags,(1<<VIC_IDX_CEVA));
#define GLOBAL_INT_RESTORE(); \
          SYSirq_Enable_All_Interrupts(cpu_flags); \
  } while(0) ; 
#else
	#define GLOBAL_INT_DISABLE(); \
	do { \
			uint32_t  fiq_tmp; \
			uint32_t  irq_tmp; \
			fiq_tmp = __disable_fiq(); \
			irq_tmp = __disable_irq();
	#define GLOBAL_INT_RESTORE(); \
				if(!fiq_tmp)           \
				{                      \
					__enable_fiq();    \
				}                      \
				if(!irq_tmp)           \
				{                      \
					__enable_irq();    \
				}                      \
	} while(0) ;
#endif

/** @brief Invoke the wait for interrupt procedure of the processor.
 *
 * @warning It is suggested that this macro is called while the interrupts are disabled
 * to have performed the checks necessary to decide to move to sleep mode.
 *
 */
//__INLINE void WFI(void)
static __inline void WFI(void)
{
#if 0	////TODO, FIXME
    uint32_t __l_rd;
#pragma arm
    __asm
    {
        MOV __l_rd, 0;
        MCR p15, 0, __l_rd, c7, c0, 4;
    }
#endif
}

#endif // LL_H_


