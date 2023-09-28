/*
 * This module contains basic exception handlers for all BA22 exceptions.
 * (By default) these are located at 0x008, 0x010, ..., 0x070.
 * Each handler has 8 bytes available. If your handler is longer,
 * you need to use subroutines. You can use HANDLE_EXCEPTION macro
 * to call C subroutine that does the useful part of exception handling.
 *
 * Linker script used with this example reserves 0x1000 bytes for
 * vectors section, so there is some free space after trap handler
 * (0xe00+) and before reset handler (0x0..0xff).
 */

#include "excutil.h"
#include "spr_defs.h"
#include "config_system.h"

#define CFG_BA22_EXCEPTION_ENABLE   (1)

.section RAMCODE,"ax",%progbits

/* copy memory region
 *
 *   from:     src_beg
 *   to start:     dst_beg
 *   to end:     dst_end
 *
 * with registers t1,t2,t3,t4 as temporary
 */
#define COPY_MEMORY(src_beg,dst_beg,dst_end,t1,t2,t3,t4)    \
        b.ori   t1,r0,src_beg    ;\
        b.ori   t2,r0,dst_beg    ;\
        b.ori   t3,r0,dst_end    ;\
1:      b.bgeu  t2,t3,2f         ;\
        b.lwz   t4,0(t1)         ;\
        b.sw    0(t2),t4         ;\
        b.addi  t1,t1,4          ;\
        b.addi  t2,t2,4          ;\
        b.j     1b               ;\
2:

        .section .boot_vectors, "ax"

/* ---[ 0x008: Reset exception ]----------------------------------------- */
        .org 0x008 /* 000 */

        /* As this code goes directly into vectors section,
         * it needs to be smaller than 0x100 bytes.
         * You can of course have larger initialization in text
         * section and just jump there. */
        /* set up stack pointer
         * and make sure it's 8-byte aligned */

        b.j _reset_f

#if CFG_BA22_EXCEPTION_ENABLE

        //[ 0x010: Instruction Bus Error exception ]
        .org 0x010
        UNHANDLED_EXCEPTION_DEBUG(instruction_bus_error)

/* ---[ 0x018: Aligment exception ]-------------------------------------- */
        .org 0x018
        UNHANDLED_EXCEPTION_DEBUG(aligment_exception)

/* ---[ 0x020: Illegal insn exception ]---------------------------------- */
        .org 0x020
        UNHANDLED_EXCEPTION_DEBUG(illegal_insn_exception)

/* ---[ 0x028: Trap exception ]------------------------------------------ */
        .org 0x028
        UNHANDLED_EXCEPTION_DEBUG(trap_exception)

/* ---[ 0x030: Timer exception ]----------------------------------------- */
        .org 0x030
       	UNHANDLED_EXCEPTION_DEBUG(timer_exception)

/* ---[ 0x038: External interrupt exception ]---------------------------- */
        .org 0x038
		UNHANDLED_EXCEPTION_DEBUG(external_interrupt_exception)

/* ---[ 0x040: Syscall exception ]--------------------------------------- */
        .org 0x040
        UNHANDLED_EXCEPTION_DEBUG(syscall_exception)

/* ---[ 0x048: Floating point exception ]-------------------------------- */
        .org 0x048
        UNHANDLED_EXCEPTION_DEBUG(floating_point_exception)

/* ---[ 0x060: Instruction Page Fault exception ]------------------------ */
        .org 0x060
        UNHANDLED_EXCEPTION_DEBUG(instruction_page_fault)

/* ---[ 0x068: Data Page Fault exception ]-------------------------------- */
        .org 0x068
        UNHANDLED_EXCEPTION_DEBUG(data_page_fault)

/* ---[ 0x070: Intruction TLB miss Vector exception ]----------------- */
        .org 0x070
        UNHANDLED_EXCEPTION_DEBUG(instruction_tlb_miss_vector)

/* ---[ 0x078: Data TLB miss Vector exception ]---------------------- */
        .org 0x078
        UNHANDLED_EXCEPTION_DEBUG(data_tlb_miss_vector)
#else
/* ---[ 0x010: Instruction Bus Error exception ]------------------------- */
        .org 0x010
        UNHANDLED_EXCEPTION

/* ---[ 0x018: Aligment exception ]-------------------------------------- */
        .org 0x018
        UNHANDLED_EXCEPTION

/* ---[ 0x020: Illegal insn exception ]---------------------------------- */
        .org 0x020
        UNHANDLED_EXCEPTION

/* ---[ 0x028: Trap exception ]------------------------------------------ */
        .org 0x028
        UNHANDLED_EXCEPTION

/* ---[ 0x030: Timer exception ]----------------------------------------- */
        .org 0x030
        UNHANDLED_EXCEPTION

/* ---[ 0x038: External interrupt exception ]---------------------------- */
        .org 0x038
        UNHANDLED_EXCEPTION

/* ---[ 0x040: Syscall exception ]--------------------------------------- */
        .org 0x040
        UNHANDLED_EXCEPTION

/* ---[ 0x048: Floating point exception ]-------------------------------- */
        .org 0x048
        UNHANDLED_EXCEPTION

/* ---[ 0x060: Instruction Page Fault exception ]------------------------ */
        .org 0x060
        UNHANDLED_EXCEPTION

/* ---[ 0x068: Data Page Fault exception ]-------------------------------- */
        .org 0x068
        UNHANDLED_EXCEPTION

/* ---[ 0x070: Intruction TLB miss Vector exception ]----------------- */
        .org 0x070
        UNHANDLED_EXCEPTION

/* ---[ 0x078: Data TLB miss Vector exception ]---------------------- */
        .org 0x078
        UNHANDLED_EXCEPTION

#endif//CFG_BA22_EXCEPTION_ENABLE

        //[ 0x080: TLV Table Base Address ]
        .org 0x080
        .word 0x00000000
        .word 0x00000000

/* Only uart download successfully,write password 'PTN101',don't care flash crc check :) */
        .org 0x100
		.byte 'B'
		.byte 'K'
		.byte '3'
		.byte '2'
		.byte '8'
		.byte '8'
		.byte BOOT_WAIT_TIME
		.byte 0x41
		.byte CODE_VERSION
		.byte FLASH_INFO

        // 0x110 DSP_addr
        .word 0xFFFFFFFF
        .word 0xFFFFFFFF
        
        .org 0x200
_reset_f:
        b.ori r1,r0,_stack-4
        b.andi r1,r1,~7

        /* Set FLASH @ 26M 2-LINE mode */
        b.addi	r7,r0,0x04000018
        b.sw	0x0100101C(r0),r7

#ifndef BA22_DE
        /* Enable caches (if present).
         * This can be removed if your CPU does not have caches implemented.
         */
        b.jal _enable_dcache
        b.jal _enable_icache
#endif
        b.jal Boot_loader
        b.jal _program_entry

#ifndef BA22_DE
/**
 * This function enables data caches (if present in implementation)
 * Whole procedure is:
 *   -make sure they are disabled first
 *   -invalidate any data present in cache
 *      For this "range invalidation" (newer, faster) can be used,
 *      or block invalidation (supported in oldest BA22 implementations).
 *   -reenable cache
 * If your CPU does not have data cache implemented, you can remove this.
 */
_enable_dcache:
        b.mfspr r3,r0,SPR_DCCFGR
        b.beq r3,r0,a_enable_dcache     //cache not implemented=>nothing to do
        //disable data cache
        b.mfspr r4,r0,SPR_SR
        b.andi r4,r4,~SPR_SR_DCE
        b.mtspr r0,r4,SPR_SR
        //use best method supported to invalidate it
        b.andi r3,r3,SPR_DCCFGR_RIRI    //check for range invalidation
        b.beq r3,r0,o_enable_dcache     //present in all but very old implementations
        //use range invalidation (invalidate all addresses)
        b.mtspr r0,r0,SPR_RIR_MIN
        b.ori r3,r0,0xfffffff0+SPR_RIR_MAX_DC
        b.mtspr r0,r3,SPR_RIR_MAX       //this does it
        b.j x_enable_dcache
o_enable_dcache:
        //use block invalidate (if you know your implementations
        //have range invalidate you can remove this part down to x_enable)
        b.mfspr r5,r0,SPR_DCCFGR
        b.andi r6,r5,SPR_DCCFGR_CBS
        b.srli r6,r6,8
        b.addi r6,r6,2
        b.ori r7,r0,1
        b.sll r7,r7,r6 // r7 = cache line size
        b.andi r5,r5,SPR_DCCFGR_NCS
        b.srli r5,r5,4
        b.add r6,r5,r6
        b.ori r5,r0,1
        b.sll r5,r5,r6 // r5 = cache size
        b.ori r6,r0,0
1:      b.mtspr r0,r6,SPR_DCBIR
        b.add r6,r6,r7
        b.bleu r6,r5,1b
x_enable_dcache:
        //enable data cache
        b.ori r4,r4,SPR_SR_DCE
        b.mtspr r0,r4,SPR_SR
a_enable_dcache:
        b.return

/**
 * Enables instruction cache.
 * Practically the same as enable_dcache, but registers are different.
 * If your CPU does not have instruction cache implemented, you can remove this.
 */
_enable_icache:
        b.mfspr r3,r0,SPR_ICCFGR
        b.beq r3,r0,a_enable_icache     //cache not implemented=>nothing to do
        //disable data cache
        b.mfspr r4,r0,SPR_SR
        b.andi r4,r4,~SPR_SR_ICE
        b.mtspr r0,r4,SPR_SR
        //use best method supported to invalidate it
        b.andi r3,r3,SPR_ICCFGR_RIRI    //check for range invalidation
        b.beq r3,r0,o_enable_icache     //present in all but very old implementations
        //use range invalidation (invalidate all addresses)
        b.mtspr r0,r0,SPR_RIR_MIN
        b.ori r3,r0,0xfffffff0+SPR_RIR_MAX_IC
        b.mtspr r0,r3,SPR_RIR_MAX       //do it
        b.j x_enable_icache
o_enable_icache:
        //use block invalidate (if you know your implementations
        //have range invalidate you can remove this part down to x_enable)
        b.mfspr r5,r0,SPR_ICCFGR
        b.andi r6,r5,SPR_ICCFGR_CBS
        b.srli r6,r6,8
        b.addi r6,r6,2
        b.ori r7,r0,1
        b.sll r7,r7,r6 // r7 = cache line size
        b.andi r5,r5,SPR_ICCFGR_NCS
        b.srli r5,r5,4
        b.add r6,r5,r6
        b.ori r5,r0,1
        b.sll r5,r5,r6 // r5 = cache size
        b.ori r6,r0,0
1:      b.mtspr r0,r6,SPR_ICBIR
        b.add r6,r6,r7
        b.bleu r6,r5,1b
x_enable_icache:
        //enable instruction  cache
        b.ori r4,r4,SPR_SR_ICE
        b.mtspr r0,r4,SPR_SR
a_enable_icache:
        b.return
#endif//BA22_DE

        .section .vectors, "ax"

_program_entry:
        b.nop    1

_copy_data_to_ram:
        COPY_MEMORY(_data_flash_begin,_data_ram_begin,_data_ram_end,r3,r4,r5,r8)
        COPY_MEMORY(_sdata_flash_begin,_sdata_ram_begin,_sdata_ram_end,r3,r4,r5,r8)
        //COPY_MEMORY(_iqmem_start_lma,_iqmem_start_vma,_iqmem_end_vma,r3,r4,r5,r8)

        //b.ori    r3,r0,_ext_mem_begin
        //b.ori    r4,r0,_ext_mem_end
        //b.jal    _clear_memory

        b.ori    r3,r0,_sbcmem_begin
        b.ori    r4,r0,_sbcmem_end
        b.jal    _clear_memory

        b.ori    r3,r0,_mempool_begin
        b.ori    r4,r0,_mempool_end
        b.jal    _clear_memory

        /* clear BSS section */
        b.ori    r3,r0,_bss_start
        b.ori    r4,r0,_bss_end
        b.jal    _clear_memory

        /* clear SBSS section */
        b.ori    r3,r0,_sbss_start
        b.ori    r4,r0,_sbss_end
        b.jal    _clear_memory

        /* clear STACK section */
        b.ori    r3,r0,_heap_limit
        b.ori    r4,r0,_stack
        b.jal    _clear_memory

        /* clear heap and stack section */
        b.ori	r3,r0,_sbss_end
        b.ori	r4,r0,_stack-64
        b.jal	_fill_memory

        /* for check DRAM_CODE */
        //b.ori	r3,r0,_mempool_end
        //b.ori	r4,r0,_mempool_end+8
        //b.jal	_fill_memory

        /* set up global pointer */
        b.ori    r2,r0,__gp

        /* execute main */
        b.jal    main

_main_done:
        /* Main returns here.
         * In embedded programs this really should not happen,
         * but if it does... */

        /* nop (special one, that terminates simulations ) */
        b.nop   1
        /* If not run on simulator, just loop that nop,
         * rather than go and execute some random code */
        b.j _main_done

/**
 * This function fills (sets to 5a) memory from address stored
 * in r3 to (excluding) address stored in r4.
 */
_fill_memory:
        b.bgeu r3,r4,4f
1:      //first write word by word
        b.addi r3,r3,4
        b.addi r6,r0,0XDEADBEEF
        b.bgtu r3,r4,2f
        b.sw   -4(r3),r6
        b.j 1b
2:      //then the remainder by bytes
        b.addi r3,r3,-4
3:      b.bgeu r3,r4,4f
        b.sb 0(r3),r6
        b.addi r3,r3,1
        b.j 3b
4:      b.return

/**
 * This function clears (sets to 0) memory from address stored
 * in r3 to (excluding) address stored in r4.
 */
_clear_memory:
        b.bgeu r3,r4,4f
1:      //first write word by word
        b.addi r3,r3,4
        b.bgtu r3,r4,2f
        b.sw   -4(r3),r0
        b.j 1b
2:      //then the remainder by bytes
        b.addi r3,r3,-4
3:      b.bgeu r3,r4,4f
        b.sb 0(r3),r0
        b.addi r3,r3,1
        b.j 3b
4:      b.return

.globl vic_isr_usbplug
.globl vic_isr_rtc
.globl vic_isr_anc
.globl vic_isr_ceva
.globl vic_isr_cec
.globl vic_isr_i2s2
.globl vic_isr_i2s1
.globl vic_isr_spdif
.globl vic_isr_sdio
.globl vic_isr_sadc
.globl vic_isr_irda
.globl vic_isr_pwm
.globl vic_isr_i2s0
.globl vic_isr_timer1
.globl vic_isr_timer0
.globl vic_isr_i2c1
.globl vic_isr_i2c0
.globl vic_isr_spi2
.globl vic_isr_spi1
.globl vic_isr_spi0
.globl vic_isr_uart2
.globl vic_isr_uart1
.globl vic_isr_uart0
.globl vic_isr_gpio
.globl vic_isr_rwbt0
.globl vic_isr_rwbt1
.globl vic_isr_rwbt2
.globl vic_isr_bk24
.globl vic_isr_usb1
.globl vic_isr_usb0
.globl vic_isr_qspi
.globl vic_isr_sbc
.globl vic_isr_fft
.globl vic_isr_gener_dma
.globl vic_isr_mbx_dsp2cpu
.globl vic_isr_touch
.globl vic_isr_vusb_dlp

vic_isr_usbplug     : HANDLE_EXCEPTION(int_handler_usbplug    )
vic_isr_rtc         : HANDLE_EXCEPTION(int_handler_rtc        )
vic_isr_anc         : HANDLE_EXCEPTION(int_handler_anc        )
vic_isr_ceva        : HANDLE_EXCEPTION(int_handler_ceva       )
vic_isr_cec         : HANDLE_EXCEPTION(int_handler_cec        )
vic_isr_i2s2        : HANDLE_EXCEPTION(int_handler_i2s2       )
vic_isr_i2s1        : HANDLE_EXCEPTION(int_handler_i2s1       )
vic_isr_spdif       : HANDLE_EXCEPTION(int_handler_spdif      )
vic_isr_sdio        : HANDLE_EXCEPTION(int_handler_sdio       )
vic_isr_sadc        : HANDLE_EXCEPTION(int_handler_sadc       )
vic_isr_irda        : HANDLE_EXCEPTION(int_handler_irda       )
vic_isr_pwm         : HANDLE_EXCEPTION(int_handler_pwm        )
vic_isr_i2s0        : HANDLE_EXCEPTION(int_handler_i2s0       )
vic_isr_timer1      : HANDLE_EXCEPTION(int_handler_timer1     )
vic_isr_timer0      : HANDLE_EXCEPTION(int_handler_timer0     )
vic_isr_i2c1        : HANDLE_EXCEPTION(int_handler_i2c1       )
vic_isr_i2c0        : HANDLE_EXCEPTION(int_handler_i2c0       )
vic_isr_spi2        : HANDLE_EXCEPTION(int_handler_spi2       )
vic_isr_spi1        : HANDLE_EXCEPTION(int_handler_spi1       )
vic_isr_spi0        : HANDLE_EXCEPTION(int_handler_spi0       )
vic_isr_uart2       : HANDLE_EXCEPTION(int_handler_uart2      )
vic_isr_uart1       : HANDLE_EXCEPTION(int_handler_uart1      )
vic_isr_uart0       : HANDLE_EXCEPTION(int_handler_uart0      )
vic_isr_gpio        : HANDLE_EXCEPTION(int_handler_gpio       )
vic_isr_rwbt0       : HANDLE_EXCEPTION(int_handler_rwbt0      )
vic_isr_rwbt1       : HANDLE_EXCEPTION(int_handler_rwbt1      )
vic_isr_rwbt2       : HANDLE_EXCEPTION(int_handler_rwbt2      )
vic_isr_bk24        : HANDLE_EXCEPTION(int_handler_bk24       )
vic_isr_usb1        : HANDLE_EXCEPTION(int_handler_usb1       )
vic_isr_usb0        : HANDLE_EXCEPTION(int_handler_usb0       )
vic_isr_qspi        : HANDLE_EXCEPTION(int_handler_qspi       )
vic_isr_sbc         : HANDLE_EXCEPTION(int_handler_sbc        )
vic_isr_fft         : HANDLE_EXCEPTION(int_handler_fft        )
vic_isr_gener_dma   : HANDLE_EXCEPTION(int_handler_gener_dma  )
vic_isr_mbx_dsp2cpu : HANDLE_EXCEPTION(int_handler_mbx_dsp2cpu)
vic_isr_touch       : HANDLE_EXCEPTION(int_handler_touch      )
vic_isr_vusb_dlp    : HANDLE_EXCEPTION(int_handler_vusb_dlp   )

