SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Interrupt vector table */
    .vectors : 
    {
        . = ALIGN(512);
        link_interrupt_vectors_location = .;
        KEEP (*(*.system_part1_boot_table))
        link_interrupt_vectors_location_end = .;
    } > APP_FLASH AT> APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld

    /* Dynalib table */
    .dynalib :
    {
        link_dynalib_start = . ;
        KEEP (*(*.system_part1_module))
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

        . = ALIGN(4);

        /* Read-only data (constants) */
        link_const_data_start = .;
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.r.*)
        link_const_data_end = .;

        . = ALIGN(4);

        /* This is used by module_system_part1_init() function in order to invoke C++ constructors */
        link_constructors_location = .;
        KEEP (*(.preinit_array))
        KEEP (*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        /* This is used by module_system_part1_init() function in order to invoke C++ constructors */
        link_constructors_end = .;

        . = ALIGN(4);

        /* C++ destructors */
        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;

        . = ALIGN(4);
    } > APP_FLASH  AT> APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld
    
    /* Static RAM start */
    INCLUDE linker_static_ram_start.ld

    /* Non-zero initialised global and static variables */
    .data :
    {
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        . = ALIGN(4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        /* This is used by module_system_part1_pre_init() in order to initialize the .bss secion */
        link_bss_end = .;
    } > SRAM AT> SRAM
    
    /* Static RAM end */
    INCLUDE linker_static_ram_end.ld
    
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

/* Stack , used by system_part1_boot_table */
link_stack_size     = platform_modular_firmware_stack_size;
link_stack_location = platform_modular_firmware_stack_start;
link_stack_end      = platform_modular_firmware_stack_end;

ASSERT (
    (ORIGIN(SRAM) >= platform_ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= platform_ram_end) &&
    (ORIGIN(APP_FLASH) >= platform_flash_start && ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) <= platform_flash_end) &&
    (link_module_start == ORIGIN(APP_FLASH)) &&
    (link_module_start == platform_system_part1_flash_start) &&
    (link_static_ram_start == ORIGIN(SRAM)) &&
    (link_static_ram_start == platform_system_part1_static_ram_start) &&
    (PLATFORM_DFU == ORIGIN(APP_FLASH)),
    "Invalid memory boundary!"
);

ASSERT (
    (link_interrupt_vectors_location == platform_system_part1_vector_table_flash_start) &&
    (link_interrupt_vectors_location_end == platform_system_part1_vector_table_flash_end) &&
    (link_interrupt_vectors_location_end - link_interrupt_vectors_location == platform_vector_table_flash_size),
    "Invalid interrupt vector table!"
);
ASSERT (
    (link_module_info_start < link_module_info_end) &&
    (link_module_info_suffix_start - link_module_info_static_start == 4 || link_module_info_suffix_start - link_module_info_static_start == 8) &&
    (link_module_info_suffix_start < link_module_info_suffix_end) &&
    (link_module_info_crc_start < link_module_info_crc_end) &&
    (link_module_info_start == platform_system_part1_module_info_flash_start) &&
    (link_module_info_end == platform_system_part1_module_info_flash_end),
    "Invalid module info!"
);
ASSERT (
    (link_dynalib_start != link_dynalib_end) &&
    (link_dynalib_start == platform_system_part1_dynalib_table_flash_start),
    "Invalid dynalib table!"
);
ASSERT (
    (link_constructors_location % 4 == 0),
    "constructor array should be aligned to a 4-bytes!"
);
ASSERT (
    (link_static_ram_end - link_static_ram_start <= platform_system_part1_static_ram_size),
    "Insufficient room for .data and .bss sections!"
);
