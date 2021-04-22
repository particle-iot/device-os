
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

SECTIONS
{
    INCLUDE module_start.ld

    .rtl_header :
    {
        . = ALIGN(4);
        link_rtl_header_location = .;
        KEEP(*(.rtl_header))
        . = ALIGN(4);
        link_rtl_header_location_end = .;
    } >APP_FLASH

    /* for Cortex devices, the beginning of the startup code is stored in the .isr_vector section, which goes to FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        link_rtl_startup = .;
        KEEP(*(.isr_vector))            /* Startup code */
        . = ALIGN(4);
    } >APP_FLASH

    INCLUDE module_info.ld

    .flashtext :
    {
    . = ALIGN(4);
        *(.flashtext)            /* Startup code */
    . = ALIGN(4);
    } >APP_FLASH

    /* the program code is stored in the .text section, which goes to Flash */
    .text :
    {
        . = ALIGN(4);
        _etext = .;

        *(.text)                   /* remaining code */
        *(.text.*)                   /* remaining code */
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)

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
        *(COMMON)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _ebss = . ;
        __bss_end__ = _ebss;
    } >RAM

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

    .rtl_header_ram :
    {
        . = ALIGN(4);
        link_rtl_header_ram_location = .;
        KEEP(*(.rtl_header_ram))
        . = ALIGN(4);
        link_rtl_header_ram_location_end = .;
        KEEP(*(.rtl_header_ram_data))
    } >APP_FLASH

    PROVIDE(link_rtl_header_size = link_rtl_header_ram_location - link_rtl_header_location_end);

    INCLUDE module_end.ld

    PROVIDE(link_rtl_header_ram_size = link_module_info_crc_end - link_rtl_header_ram_location_end);
}

ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
/* ASSERT ( link_module_info_start - ORIGIN(APP_FLASH) == 0x20,  "module info not where expected" ) */
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );

ASSERT ( ORIGIN(RAM) >= _ram_start && ORIGIN(RAM) + LENGTH(RAM) <= _ram_end, "Static RAM region doesn't fit into RAM" );
ASSERT ( link_rtl_header_location_end - link_rtl_header_location == 0x20, "rtl872x header is invalid" );

SECTIONS {
    IS_FPGA_VERIF = 0x1000039c;
    CPU_ClkGet = 0x1010e515;
    irq_table_init = 0x10110795;
    irq_enable = 0x101109d1;
    irq_disable = 0x101109ed;
    irq_set_priority = 0x10110a11;
    irq_get_priority = 0x10110a3d;
    irq_set_pending = 0x10110a65;
    irq_get_pending = 0x10110a81;
    irq_clear_pending = 0x10110aa5;
    irq_register = 0x10110ac1;
    irq_unregister = 0x10110b0d;
    mpu_enable = 0x1010cbd5;
    mpu_disable = 0x1010cbed;
    mpu_init = 0x1010cc01;
    mpu_set_mem_attr = 0x1010cc3d;
    mpu_region_cfg = 0x1010ccb1;
    mpu_entry_free = 0x1010cdb5;
    mpu_entry_alloc = 0x1010cdc1;
    DiagPrintf = 0x1010a3f5;
}
