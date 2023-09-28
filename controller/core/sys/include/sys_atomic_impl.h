#ifndef _PARTHUS_SYS_ATOMIC_IMPL_H
#define _PARTHUS_SYS_ATOMIC_IMPL_H

/**************************************************************************
 * MODULE NAME:    sys_atomic_impl.h
 * PROJECT CODE:    BlueStream
 * DESCRIPTION:    Atomic operations
 * MAINTAINER:     John Sheehy
 * DATE:           8 March 2000
 *
 * SOURCE CONTROL: $Id: sys_atomic_impl.h,v 1.7 2004/07/07 14:22:28 namarad Exp $
 *
 * LICENSE:
 *     This source code is copyright (c) 2000-2004 Ceva Inc.
 *     All rights reserved.
 *
 * REVISION HISTORY:
 *
 **************************************************************************/

#if (PRAGMA_INLINE==1)
#pragma inline(SYSatomic_Increment_u_int8,\
SYSatomic_Decrement_u_int8,\
SYSatomic_Increment_u_int16,\
SYSatomic_Decrement_u_int16)
#endif

/**************************************************************************
 * SYSatomic_Increment_u_int8
 * 
 * parameters: var = pointer to 8 bit variable to increment
 * returns: void
 *
 * Atomically increments variable
 **************************************************************************/
__INLINE__ void SYSatomic_Increment_u_int8(uint8_t *var)
{
	//uint32_t flags, mask;
	
	//g_ctrl_irq_cbs.SYSirq_Disable_Interrupts_Save_Flags_C(&flags, &mask); 
	(*var)++;
	//g_ctrl_irq_cbs.SYSirq_Interrupts_Restore_Flags_C(flags, mask);
}

/**************************************************************************
 * SYSatomic_Increment_u_int16
 * 
 * parameters: var = pointer to 16 bit variable to increment
 * returns: void
 *
 * Atomically increments variable
 **************************************************************************/
__INLINE__ void SYSatomic_Increment_u_int16(uint16_t *var)
{
	//uint32_t flags, mask;
	
	//g_ctrl_irq_cbs.SYSirq_Disable_Interrupts_Save_Flags_C(&flags, &mask); 
	(*var)++;
	//g_ctrl_irq_cbs.SYSirq_Interrupts_Restore_Flags_C(flags, mask);
}


/**************************************************************************
 * SYSatomic_Decrement_u_int8
 * 
 * parameters: var = pointer to 8 bit variable to decrement 
 * returns: void
 *
 * Atomically decrements variable
 **************************************************************************/
__INLINE__ void SYSatomic_Decrement_u_int8(uint8_t *var)
{
	//uint32_t flags, mask;

	//g_ctrl_irq_cbs.SYSirq_Disable_Interrupts_Save_Flags_C(&flags,&mask);
	(*var)--;
	//g_ctrl_irq_cbs.SYSirq_Interrupts_Restore_Flags_C(flags, mask);
}

/**************************************************************************
 * SYSatomic_Decrement_u_int16
 * 
 * parameters: var = pointer to 16 bit variable to decrement 
 * returns: void
 *
 * Atomically decrements variable
 **************************************************************************/
__INLINE__ void SYSatomic_Decrement_u_int16(uint16_t *var)
{
	//uint32_t flags, mask;

	//g_ctrl_irq_cbs.SYSirq_Disable_Interrupts_Save_Flags_C(&flags, &mask);
	(*var)--;
	//g_ctrl_irq_cbs.SYSirq_Interrupts_Restore_Flags_C(flags, mask);
}



#endif /* _PARTHUS_SYS_ATOMIC_IMPL_H */
