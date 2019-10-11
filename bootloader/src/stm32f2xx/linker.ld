/* Memory Spaces Definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    SRAM       (xrw) : ORIGIN = _bootloader_static_ram_start, LENGTH = _bootloader_static_ram_size
    APP_FLASH  (rx)  : ORIGIN = _bootloader_flash_start,      LENGTH = _bootloader_flash_size
    DCT1_FLASH (rx)  : ORIGIN = _dct1_flash_start,            LENGTH = _dct1_flash_size
    DCT2_FLASH (rx)  : ORIGIN = _dct2_flash_start,            LENGTH = _dct2_flash_size
  
    INCLUDE memory_backup_ram.ld
}

/* Sections Definitions */
SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Interrupt vector table */
    .isr_vector :
    {
        . = ALIGN(0x4);
        link_interrupt_vectors_location = .;
        KEEP (*(.interrupt_vector_table))
        link_interrupt_vectors_location_end = .;
        . = ALIGN(0x4);
    } > APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld

    /* the program code is stored in the .text section, which goes to Flash */
    .text :
    {
	    . = ALIGN(4);

        /* Code */
        link_code_start = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        link_code_end = .;
        
        . = ALIGN(0x4);
        
        /* Read-only data (constants) */
        link_const_data_start = .;
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.r.*)
        link_const_data_end = .;

        . = ALIGN(0x4);

        /* This is used by the startup in order to invoke C++ constructors */
        link_constructors_location = .;
        KEEP (*(.preinit_array))
        KEEP (*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        /* This is used by the startup in order to invoke C++ constructors */
        link_constructors_end = .;
        
        /* C++ destructors */
        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;
    } > APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Non-zero initialised global variables */
    .data :
    {
        link_static_ram_start = .;
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH
    
    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_end = .;
        link_static_ram_end = .;
    } > SRAM AT> SRAM
    
    /* Heap location */
    link_heap_location = link_bss_end;
    link_heap_location_end = link_bss_end + _heap_min_size;

    /* System backup ram data */
	INCLUDE linker_backup_ram_system.ld
    
    /* User backup ram data */
	INCLUDE linker_backup_ram_user.ld

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

    /* Module end */
    INCLUDE linker_module_end.ld
    
    /* remove the debugging information from the standard libraries */
    DISCARD :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}

/* Stack , used by startup*/
PROVIDE (__stack_size__ = _bootloader_stack_size);
PROVIDE (__stack_start__ = _bootloader_stack_start);
PROVIDE (__stack_end__ = __stack_start__ + __stack_size__);

PROVIDE (dct1_start_addr_loc = ORIGIN(DCT1_FLASH));
PROVIDE (dct1_size_loc       = LENGTH(DCT1_FLASH));
PROVIDE (dct2_start_addr_loc = ORIGIN(DCT2_FLASH));
PROVIDE (dct2_size_loc       = LENGTH(DCT2_FLASH));

ASSERT (
    (ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end) &&
    (ORIGIN(APP_FLASH) >= _flash_start && ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) <= _flash_end) &&
    (link_module_start == ORIGIN(APP_FLASH)) &&
    (link_module_start == _bootloader_flash_start) &&
    (link_static_ram_start == ORIGIN(SRAM)) &&
    (link_static_ram_start == _bootloader_static_ram_start),
    "Invalid memory boundary!"
);
ASSERT (
    (link_interrupt_vectors_location == _bootloader_vector_table_flash_start) &&
    (link_interrupt_vectors_location_end == _bootloader_vector_table_flash_end) &&
    (link_interrupt_vectors_location_end - link_interrupt_vectors_location == _platform_vector_table_flash_size),
    "Invalid interrupt vector table!"
);
ASSERT (
    (link_module_info_start < link_module_info_end) &&
    (link_module_info_suffix_start - link_module_info_static_start == 4 || link_module_info_suffix_start - link_module_info_static_start == 8) &&
    (link_module_info_suffix_start < link_module_info_suffix_end) &&
    (link_module_info_crc_start < link_module_info_crc_end) &&
    (link_module_info_start == _bootloader_module_info_flash_start) &&
    (link_module_info_end == _bootloader_module_info_flash_end),
    "Invalid module info!"
);
ASSERT (
    (link_constructors_location % 4 == 0),
    "constructor array should be aligned to a 4-bytes!"
);
ASSERT (
    (link_static_ram_end - link_static_ram_start <= _bootloader_static_ram_size),
    "Insufficient room for .data and .bss sections!"
);
ASSERT (
    (link_heap_location_end <= _bootloader_stack_start),
    "Insufficient room for heap!"
);
ASSERT (
    (__stack_end__ % 8 == 0),
    "Stack should be 8-byte aligned!"
);

