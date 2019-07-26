/* nRF52840 specific sections */
INCLUDE linker_nrf52840_sdk_ram.ld

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld
    
    /* Interrupt vector table */
    .vectors :
    {
        link_interrupt_vectors_location = .;
        KEEP (*(.isr_vector))
        link_interrupt_vectors_location_end = .;
    } > APP_FLASH  AT> APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld
    
    /* Dynalib table */
    .dynalib :
    {
        link_dynalib_start = . ;
        KEEP (*(*.system_part1_module))
        link_dynalib_end = . ;
    } > APP_FLASH AT> APP_FLASH

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
        
        /* Const data */
        link_const_data_start = .;
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)
        *(.gnu.linkonce.r.*)
        link_const_data_end = .;
        
        . = ALIGN(4);

        /* C++ constructors */
        link_constructors_location = .;
        /* Particle specific preinit functions */
        link_module_pre_init_start = .;
        KEEP (*(.module_pre_init))
        link_module_pre_init_end = .;
        KEEP (*(.preinit_array))
        KEEP (*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        /* Particle specific init functions */
        link_module_init_start = .;
        KEEP (*(.module_init))
        link_module_init_end = .;
        link_constructors_end = .;

        . = ALIGN(0x4);

        /* C++ destructors */
        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;

        . = ALIGN(0x4);
    } > APP_FLASH  AT> APP_FLASH

    /* nRF52840 specific sections */
    INCLUDE linker_nrf52840_sdk_libs.ld

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Ram interrupt vector table - must be 512 byte-aligned.
     * See Cortext-M4 Devices Generic User Guide, Section 4.3.4, Vector Table Offset Register. */
    .ram_vectors :
    {
        link_static_ram_start = .;
        link_ram_interrupt_vectors_location = .;
        . = . + _platform_vector_table_flash_size;
        link_ram_interrupt_vectors_location_end = .;
    } > SRAM

    /* This section contains code that is run from RAM after being loaded from flash.
     * functions can be put in this section with the C attribute: __attribute__ ((section (".fast"))) */
    .fast :
    {
        link_run_from_ram_code_flash_location = LOADADDR( .fast ); /* This is the location in flash of the code */
        link_run_from_ram_code_ram_location = .;
        *(.fast .fast.* .text.fastcode)
        link_run_from_ram_code_ram_end = .;
    } > SRAM AT> APP_FLASH

    /* Non-zero initialised global variables */
    .data :
    {
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_end = .;
    } > SRAM AT> SRAM
    
    /* Heap location */
    link_heap_location = link_bss_end;
    link_heap_location_end = link_bss_end + _heap_min_size;
    
    /* Contains the initial stack */
    .stack __stack_start__ :
    {
        link_init_stack_start = .;
        *(.stack)
        link_init_stack_end = .;
    } > SRAM AT> SRAM

    /* System backup ram data */
    INCLUDE linker_backup_ram_system.ld

    /* Discarded sections */
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

    /* Module end */
    INCLUDE linker_module_end.ld
}

/* Referenced by SoftDevice driver. */
PROVIDE (__data_start__ = link_static_ram_start);
PROVIDE (__data_end__ = link_global_data_end);

/* Stack , used by startup*/
PROVIDE (__stack_size__ = _modular_firmware_stack_size);
PROVIDE (__stack_start__ = _modular_firmware_stack_start);
PROVIDE (__stack_end__ = __stack_start__ + __stack_size__);

ASSERT (
    (ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end) &&
    (ORIGIN(APP_FLASH) >= _flash_start && ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) <= _flash_end) &&
    (link_module_start == ORIGIN(APP_FLASH)) &&
    (link_module_start == _system_part1_flash_start) &&
    (link_static_ram_start == ORIGIN(SRAM)) &&
    (link_static_ram_start == _system_part1_static_ram_start) &&
    (PLATFORM_DFU == ORIGIN(APP_FLASH)),
    "Invalid memory boundary!"
);
ASSERT (
    (link_interrupt_vectors_location == _system_part1_vector_table_flash_start) &&
    (link_interrupt_vectors_location_end == _system_part1_vector_table_flash_end) &&
    (link_interrupt_vectors_location_end - link_interrupt_vectors_location == _platform_vector_table_flash_size),
    "Invalid interrupt vector table!"
);
ASSERT (
    (link_module_info_start < link_module_info_end) &&
    (link_module_info_suffix_start < link_module_info_suffix_end) &&
    (link_module_info_crc_start < link_module_info_crc_end) &&
    (link_module_info_start == _system_part1_module_info_flash_start) &&
    (link_module_info_end == _system_part1_module_info_flash_end),
    "Invalid Module info!"
);
ASSERT (
    (link_dynalib_start != link_dynalib_end) &&
    (link_dynalib_start == _system_part1_dynalib_table_flash_start),
    "Invalid dynalib table!"
);
ASSERT (
    (link_constructors_location % 4 == 0),
    "constructor array should be aligned to a 4-bytes!"
);
ASSERT (
    (link_module_pre_init_start < link_module_pre_init_end) &&
    (link_module_init_start < link_module_init_end),
    "Module preinit or init function is not included!"
);
ASSERT (
    (link_ram_interrupt_vectors_location == _system_part1_static_ram_start) &&
    (link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location == _platform_vector_table_flash_size) &&
    (link_ram_interrupt_vectors_location % 512 == 0),
    "Invalid RAM interrupt vector table!"
);
ASSERT (
    (link_bss_end - link_global_data_start <= _system_part1_static_ram_size),
    "Insufficient room for .data and .bss sections!"
);
ASSERT (
    (link_heap_location_end <= _user_part_static_ram_start),
    "Insufficient room for heap!"
);
ASSERT (
    (__stack_end__ % 8 == 0),
    "Stack should be 8-byte aligned!"
);
ASSERT (
    (link_global_backup_registers < link_global_backup_registers_end),
    "Backup registers not linked"
);
ASSERT (
    (link_global_retained_system_flags < link_global_retained_system_flags_end),
    "System flags not linked!"
);
