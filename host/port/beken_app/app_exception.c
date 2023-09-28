#include "driver_beken_includes.h"
#include "app_beken_includes.h"

#ifdef DEBUG_BA22_EXCEPTION
#define PRINT_BA22_EXCEPTION        1
#define ACTION_BA22_EXCEPTION       while(1)//do{ BK3000_wdt_reset();  while(1); }while(0)
/* ---[ 0x010: Instruction Bus Error exception ]------------------------- */
void instruction_bus_error(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("instruction_bus_error\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}

/* ---[ 0x018: Aligment exception ]-------------------------------------- */
void aligment_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("aligment_exception:%p\r\n", get_spr(SPR_EPCR_BASE));
    os_printf("aligment_exception:%p\r\n", get_spr(SPR_JPC));
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x020: Illegal insn exception ]---------------------------------- */
void illegal_insn_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("illegal_insn_exception:%p\r\n", get_spr(SPR_EPCR_BASE));
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x028: Trap exception ]------------------------------------------ */
void trap_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("trap_exception:%p\r\n", get_spr(SPR_EPCR_BASE));
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x030: Timer exception ]----------------------------------------- */
void timer_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("timer_exception\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x038: External interrupt exception ]---------------------------- */
void external_interrupt_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("external_interrupt_exception\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x040: Syscall exception ]--------------------------------------- */
void syscall_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("syscall_exception\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x048: Floating point exception ]-------------------------------- */
void floating_point_exception(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("floating_point_exception\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x060: Instruction Page Fault exception ]------------------------ */
void instruction_page_fault(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("instruction_page_fault\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x068: Data Page Fault exception ]-------------------------------- */
void data_page_fault(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("data_page_fault\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x070: Intruction TLB miss Vector exception ]----------------- */
void instruction_tlb_miss_vector(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("instruction_tlb_miss_vector\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
/* ---[ 0x078: Data TLB miss Vector exception ]---------------------- */
void data_tlb_miss_vector(void) {
#if (PRINT_BA22_EXCEPTION == 1)
    os_printf("data_tlb_miss_vector\r\n");
#endif
    ACTION_BA22_EXCEPTION;
}
#endif

// EOF
