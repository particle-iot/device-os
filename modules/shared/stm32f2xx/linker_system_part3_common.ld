/* todo - this is mainly a copy/paste of part1.ld. Refactor to 
dedup common elements between these files. The primary difference is that
system part3 doesn't require a vector table. (including it does no harm, just wastes 384 bytes.) */

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Module info */
    INCLUDE linker_module_info.ld

    /* Dynalib table */
    .dynalib :
    {
        link_dynalib_start = . ;
        KEEP (*(*.system_part3_module))
        link_dynalib_end = . ;
    } > APP_FLASH  AT> APP_FLASH

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

        /* This is used by module_system_part3_init() function in order to invoke C++ constructors */
        link_constructors_location = .;
        KEEP (*(.preinit_array))
        KEEP (*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        /* This is used by module_system_part3_init() function in order to invoke C++ constructors */
        link_constructors_end = .;

        . = ALIGN(0x4);

        /* C++ destructors */
        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;

        . = ALIGN(16);
    } > APP_FLASH  AT> APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Non-zero initialised global variables */
    .data :
    {
        link_static_ram_start = .;
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(0x4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        . = ALIGN(0x4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(0x4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .bss secion */
        link_bss_end = .;
        link_static_ram_end = .;
    } > SRAM AT> SRAM
    
    /* Heap and stack is defined in system part 2. */
    
    /* Module end */
    INCLUDE linker_module_end.ld

    /DISCARD/ :
    {
        *(.ARM.attributes*)
        *(.comment)
        *(.init)
        *(.preinit)
        *(.fini)
        *(.fini_array)
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.gnu.linkonce.armextab.*)
        *(.v4_bx)
        *(.vfp11_veneer)
        *(.gcc_except_table)
        *(.eh_frame_hdr)
        *(.eh_frame)
        *(.glue*)
    }
}

ASSERT (
    (ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end) &&
    (ORIGIN(APP_FLASH) >= _flash_start && ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) <= _flash_end) &&
    (link_module_start == ORIGIN(APP_FLASH)) &&
    (link_module_start == _system_part3_flash_start) &&
    (link_static_ram_start == ORIGIN(SRAM)) &&
    (link_static_ram_start == _system_part3_static_ram_start) &&
    (PLATFORM_DFU == ORIGIN(APP_FLASH)),
    "Invalid memory boundary!"
);
ASSERT (
    (link_module_info_start < link_module_info_end) &&
    (link_module_info_suffix_start - link_module_info_static_start == 4 || link_module_info_suffix_start - link_module_info_static_start == 8) &&
    (link_module_info_suffix_start < link_module_info_suffix_end) &&
    (link_module_info_crc_start < link_module_info_crc_end) &&
    (link_module_info_start == _system_part3_module_info_flash_start) &&
    (link_module_info_end == _system_part3_module_info_flash_end),
    "Invalid module info!"
);
ASSERT (
    (link_dynalib_start != link_dynalib_end) &&
    (link_dynalib_start == _system_part3_dynalib_table_flash_start),
    "Invalid dynalib table!"
);
ASSERT (
    (link_constructors_location % 4 == 0),
    "constructor array should be aligned to a 4-bytes!"
);
ASSERT (
    (link_static_ram_end - link_static_ram_start <= _system_part3_static_ram_size),
    "Insufficient room for .data and .bss sections!"
);

