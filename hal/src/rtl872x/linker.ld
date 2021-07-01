/* Default stack sizes. These are used by the startup in order to allocate stacks for the
   different modes. */
__Stack_Size = __STACKSIZE__;
__Stack_Init = 0x10004000;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1400;

/* Memory Spaces Definitions */
INCLUDE platform_ram.ld

bootloader_ram_origin = 0x10004000;

system_flash_length = 512K;
system_flash_origin = 0x08020000;


MEMORY
{
  INCLUDE backup_ram_memory.ld
  RAM (xrw) : ORIGIN = 0x10005000, LENGTH = 456K - (DEFINED(GCC_LD_BROKEN) ? 4K : LENGTH(BACKUPSRAM_ALL))
  APP_FLASH (rx) : ORIGIN = system_flash_origin, LENGTH = system_flash_length
}

PROVIDE ( _Stack_Size = __Stack_Size ) ;
/*"PROVIDE" allows to easily override these values from an object file or the commmand line.*/
PROVIDE ( _Stack_Init = __Stack_Init ) ;


/* include the memory spaces definitions sub-script */
/*
Linker subscript for STM32F205xx definitions with 1024K Flash and 128K RAM
*/

/* higher address of the user mode stack */
/* end of 256K SRAM - backup sram pages */
_estack = 0x20040000 - LENGTH(BACKUPSRAM_ALL);

/* include the sections management sub-script for FLASH mode */

/* Sections Definitions */

ENTRY(Reset_Handler)

SECTIONS
{
    INCLUDE module_start.ld

    .entry :
    {
        . = ALIGN(4);
        link_rtl_header_location = .;
        KEEP(*(.system.entry))
        . = ALIGN(4);
        link_rtl_header_location_end = .;
        link_rtl_startup = .;
    } >APP_FLASH

    /* .rtl_header :
    {
        . = ALIGN(4);
        link_rtl_header_location = .;
        KEEP(*(.rtl_header))
        . = ALIGN(4);
        link_rtl_header_location_end = .;
        link_rtl_startup = .;
    } >APP_FLASH

    .rtl_header_ram :
    {
        . = ALIGN(16);
        link_rtl_header_ram_location = .;
        KEEP(*(.rtl_header_ram))
        . = ALIGN(4);
        KEEP(*(.rtl_header_ram_start_table))
        KEEP(*(.rtl_header_ram_valid_pattern))
        KEEP(*(.rtl_header_ram_boot_export_table))
        link_rtl_header_ram_location_end = .;
    } >APP_FLASH

    PROVIDE(link_rtl_header_size = link_rtl_header_ram_location - link_rtl_header_location_end);

    PROVIDE(link_rtl_header_ram_size = link_rtl_header_ram_location_end - link_rtl_header_ram_location); */

    INCLUDE module_info.ld

    /* for Cortex devices, the beginning of the startup code is stored in the .isr_vector section, which goes to FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        link_interrupt_vectors_location = .;
        /* KEEP(*(.isr_vector)) */
        . = . + 512; /* reserved */
        link_interrupt_vectors_location_end = .;
        . = ALIGN(4);
    } >APP_FLASH

    interrupt_vectors_length = link_interrupt_vectors_location_end - link_interrupt_vectors_location;

    .flashtext :
    {
    . = ALIGN(4);
        *(.flashtext)
    . = ALIGN(4);
    } >APP_FLASH

    /* the program code is stored in the .text section, which goes to Flash */
    .text :
    {
        . = ALIGN(4);
        _etext = .;

        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.boot.ram.text)
        *(.boot.ram.text.*)
        *(.boot.rodata)
        *(.boot.rodata*)

        . = ALIGN(0x4);

        link_constructors_location = .;
        chk_system_pre_init_start = .;
        KEEP (*(.module_pre_init))
        chk_system_pre_init_end = .;
        KEEP(*(.preinit_array))
        KEEP(*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        chk_system_init_start = .;
        KEEP (*(.module_init))
        chk_system_init_end = .;
        link_constructors_end = .;

        . = ALIGN(4);
        __etext = .;
        /* This is used by the startup in order to initialize the .data secion */

    } >APP_FLASH

    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >APP_FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >APP_FLASH

    .ram_vectors : /* Ram VTOR table - must be 512 byte-aligned */
    {
        link_ram_interrupt_vectors_location = .;
        . = . + interrupt_vectors_length;
        link_ram_interrupt_vectors_location_end = .;
    }> RAM

    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH (inidata).
    It is one task of the startup to copy the initial values from FLASH to RAM. */
    .data  :
    {
        _sidata = LOADADDR(.data);
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;
        __data_start__ = _sdata;

        *(.data)
        *(.data.*)
        *(.boot.ram.data)
        *(.boot.ram.data.*)
        *(.image2.ram.text)
        *(.image2.ram.text.*)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _edata = . ;
    } >RAM AT>APP_FLASH

    INCLUDE backup_ram_system.ld
    INCLUDE backup_ram_user.ld

    /* This is the uninitialized data section */
    .bss :
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss = .;
        __bss_start__ = _sbss;

        *(.bss)
        *(.bss*)
        *(.boot.ram.bss)
        *(.boot.ram.bss*)
        *(COMMON)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _ebss = . ;
        __bss_end__ = _ebss;
    } >RAM

    link_heap_location = .;
    link_heap_location_end = link_heap_location + 128K; /* temp */

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );

    .stack __Stack_Init : /* Contains the initial stack */
    {
        link_stack_location = .;
        *(.stack)
        link_stack_end = .;
    }> RAM AT> RAM

    /* after that it's only debugging information. */

    /* remove the debugging information from the standard libraries */
    DISCARD :
    {
     libc.a ( * )
     libm.a ( * )
     libgcc.a ( * )
     }

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }

    INCLUDE module_end.ld
}

ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
/* ASSERT ( link_module_info_start - ORIGIN(APP_FLASH) == 0x20,  "module info not where expected" ) */
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );

ASSERT ( ORIGIN(RAM) >= _ram_start && ORIGIN(RAM) + LENGTH(RAM) <= _ram_end, "Static RAM region doesn't fit into RAM" );

INCLUDE linker_rtl872x_rom.ld
