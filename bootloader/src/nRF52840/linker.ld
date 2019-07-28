/* Memory Spaces Definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld
  
    SRAM      (xrw) : ORIGIN = _bootloader_static_ram_start, LENGTH = _bootloader_static_ram_size
    APP_FLASH (rx)  : ORIGIN = _bootloader_flash_start,      LENGTH = _bootloader_flash_size
}

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Interrupt vector table */
    .vectors :
    {
        link_interrupt_vectors_location = .;
        KEEP(*(.isr_vector))
        link_interrupt_vectors_location_end = .;
    } > APP_FLASH  AT> APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld

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

    /* User backup ram data */
    INCLUDE linker_backup_ram_user.ld
    
    /* System backup ram data */
    INCLUDE linker_backup_ram_system.ld
    
    /* Module end */
    INCLUDE linker_module_end.ld

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
}

/* Stack , used by startup*/
PROVIDE (__stack_size__ = _monolithic_firmware_stack_size);
PROVIDE (__stack_start__ = _monolithic_firmware_stack_start);
PROVIDE (__stack_end__ = __stack_start__ + __stack_size__);

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
    (link_bss_end - link_global_data_start <= _bootloader_static_ram_size),
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

