/* 
 * Script for GNU linker.
 * Describes layout of sections, location of stack.
 *
 * In this case vectors are at location 0 (reset @ 0x08)
 *
 * +------------+  0x00400000
 * data         |
 * end
 * |(heap)      |
 * .            .
 * .            .
 * |(heap limit)|
 *
 * |- - - - - - |
 * stack bottom    64k
 * +------------+  
 *
 * +------------+  0x0000000
 * |vectors     |
 * |            |
 * |------------+
 * |text        |  0x0000300
 * |data        |
 * |            |  512k
 * +------------+
 */

/* memory:
 *  D-QMEM                     64kB (start at 00800000)
 *  IAHB-MEM EBI Flash         96kB (start at 00000000, mapped to 00000000)
 *  CACHES 2way x8K/way        16kB
 */

/* Split memory into area for vectors and ram */
MEMORY
{
    boot_section  (rx)  :  ORIGIN = 0x00000000,  LENGTH = 8k
    mcu_info      (rx)  :  ORIGIN = 0x00001E20,  LENGTH = 32
    flash         (rx)  :  ORIGIN = 0x00001E40,  LENGTH = 4M
    ram           (rw!x):  ORIGIN = 0x00800000,  LENGTH = 88k
    mem_pool      (rw!x):  ORIGIN = 0x00816000,  LENGTH = 40k
    sbc_mem       (rw!x):  ORIGIN = 0x01200000,  LENGTH = 16k
    rec_mem       (rw!x):  ORIGIN = 0x01204000,  LENGTH = 16k
    aud_rb_mem    (rw!x):  ORIGIN = 0x01208000,  LENGTH = 20k
    aud_dec_mem   (rw!x):  ORIGIN = 0x0120D000,  LENGTH = 24k
    aud_enc_mem   (rw!x):  ORIGIN = 0x01213000,  LENGTH = 20k
    ahbmem        (rw!x):  ORIGIN = 0x01208000,  LENGTH = 96k
    dsp_ahbmem    (rw!x):  ORIGIN = 0x01218000,  LENGTH = 32k
    dsp_ram       (rw!x):  ORIGIN = 0x03000000,  LENGTH = 512k
}

/* start execution at default reset vector location */
ENTRY(_reset_vector);
_reset_vector = 0x00000008;

SECTIONS
{
/* vectors go to vectors region */
.boot_vectors :
{
        *(.boot_vectors)
        . = ALIGN (8);
        *(boot_code boot_code.*) 
        . = ALIGN (8);           
    } > boot_section
    
    .mcu_info_code :
    {
        *(mcu_info_code mcu_info_code.*)
        . = ALIGN (8);
    } > mcu_info
    
    .vectors : 
    { 
        *(.vectors)
        . = ALIGN (8);
    } > flash
    
    . = ALIGN(0x8);

/* code, instructions    i=i+1; */
    .dram_code :
    {
        . = ALIGN (4); 
        *(dram_code dram_code.*) 
        . = ALIGN(0x8);	
    } > flash
    
    . = ALIGN(0x8);
    
    .flash_code : 
    {
        . = ALIGN (4);
        *(ram_code ram_code.*)
        
/*		. = ALIGN (4); */
/*		*(dram_code dram_code.*) */	
        
        . = ALIGN (4);
        /* *(mp3_code mp3_code.*) */
        /* . = ALIGN (4); */
        *(ram_test_code ram_test_code.*)
        . = ALIGN (4);
        *(flash_code flash_code.* )
        . = ALIGN(0x8);
    } > flash

/* code, instructions    i=i+1; */
    .text : 
    {
        *(.text .text.* ) 
    } > flash

/* read only data    const int rom_data[3]={1,2,3}; */
    .rodata ALIGN(8) : 
    {
        *(.rodata)
        . = ALIGN (4);
        *(.rodata.*)
        . = ALIGN (4);
        *(AAC_CONST_DATA)
    } > flash					/* in flash */

    . = ORIGIN(ram);

/* globals  int ram_data[3]={4,5,6}; */
    .data ALIGN(8) : AT( ALIGN( LOADADDR(.rodata)+SIZEOF(.rodata), 4 ) )		/* VMA in RAM, but keep LMA in flash */
    {
        *(.data .data.*)
        /* *(.rodata) */
        /* *(.rodata.*) */
    } >ram
    
    /* Loader will copy data from _flash_begin to _ram_begin..ram_end */
    _data_flash_begin = LOADADDR(.data);
    _data_ram_begin = ADDR(.data);
    _data_ram_end = .;

/* small read-write data section */
    .sdata ALIGN(8) : AT( ALIGN( LOADADDR(.data)+SIZEOF(.data), 4 ) )		/* VMA in RAM, but keep LMA in flash */
    { 
        *(.sdata) 
    } >ram

    /* Loader will copy sdata from _flash_begin to ram_begin..ram_end */
    _sdata_flash_begin = LOADADDR(.sdata);
    _sdata_ram_begin = ADDR(.sdata);
    _sdata_ram_end = .;


/* frame unside informatino for exception handling */
    /DISCARD/ :
    {
        *(.eh_frame)
    }


/* global pointer in middle of data, so as much as possible is accessible */
    . = ALIGN(4);  __gp = . + 0x1000;


/* uninitialized data section - global   int i; */
    .bss ALIGN(8):
    {
        _bss_start = .;
        . = ALIGN (4);
        *(.bss .bss.*)
        . = ALIGN (4);
        _bss_end = .;
    } > ram						/* in RAM */

    . = ALIGN (8);
    _empty_ram = .;

/* small uninitialized data section */
    .sbss ALIGN(8): 
    {
        _sbss_start = .;
        . = ALIGN (4);
        *(.scommon)
        . = ALIGN (4);
        *(.sbss)
        . = ALIGN (4);
        _sbss_end = .;
    } > ram

    RAM_END  = .; 
    STACK_BOTTOM = ORIGIN(ram) + LENGTH(ram) - 8k; 
    ASSERT(RAM_END < STACK_BOTTOM, "STACK OVERLAPPED")

/* This symbol defines end of code/data sections. Heap starts here. */
    PROVIDE(end    	  = .);

    .dsp_ram_data 0x03000000 (NOLOAD):
        {
        . = ALIGN (4);
        *(dsp_ram_section dsp_ram_section.*)
        . = ALIGN (4);
        } > dsp_ram

/* Define symbol for heap limit. Heap spans from end to here. When this is exhausted,
heap allocations will fail. If you don't use heap (malloc etc.) this is irrelevant.
This also defines the size of stack that will not be overwritten by dynamic
allocations. If you need more/less stack, modify below definition.
Default memory area reserved for stack is 8k here.
*/

/* _stack symbol defines initial stack bottom addres. Stack grows to lower addresses.
Typically you set this to be top of your RAM. Note: code never checks, if stack
grows into heap area!
*/

    PROVIDE(_heap_limit    = ORIGIN(ram) + LENGTH(ram) - 8k);
    PROVIDE(_stack         = ORIGIN(ram) + LENGTH(ram));
   	PROVIDE(_sbcmem_begin  = ORIGIN(sbc_mem));
	PROVIDE(_sbcmem_end    = ORIGIN(sbc_mem) + LENGTH(sbc_mem) - 4);
   	PROVIDE(_recmem_begin  = ORIGIN(rec_mem));
	PROVIDE(_recmem_end    = ORIGIN(rec_mem) + LENGTH(rec_mem) - 4);
    PROVIDE(_mempool_begin = ORIGIN(mem_pool));
    PROVIDE(_mempool_end   = ORIGIN(mem_pool) + LENGTH(mem_pool));
/*	PROVIDE(_ext_mem_begin = 8M + 122k);
	PROVIDE(_ext_mem_end   = 8M + 122k + 6k);*/
    PROVIDE(_aud_rb_begin  = ORIGIN(aud_rb_mem));
    PROVIDE(_aud_rb_end    = ORIGIN(aud_rb_mem) + LENGTH(aud_rb_mem));
    PROVIDE(_aud_dec_begin = ORIGIN(aud_dec_mem));
    PROVIDE(_aud_dec_end   = ORIGIN(aud_dec_mem) + LENGTH(aud_dec_mem));
    PROVIDE(_aud_enc_begin = ORIGIN(aud_enc_mem));
    PROVIDE(_aud_enc_end   = ORIGIN(aud_enc_mem) + LENGTH(aud_enc_mem));
    PROVIDE(_ahbmem_begin  = ORIGIN(ahbmem));
    PROVIDE(_ahbmem_end    = ORIGIN(ahbmem) + LENGTH(ahbmem));
}

