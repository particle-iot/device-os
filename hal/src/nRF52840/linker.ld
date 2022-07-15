/* Include memory map */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld

    APP_FLASH (rx)  : ORIGIN = platform_monolithic_firmware_flash_start,      LENGTH = platform_monolithic_firmware_flash_size
    SRAM      (rwx) : ORIGIN = platform_monolithic_firmware_static_ram_start, LENGTH = platform_monolithic_firmware_static_ram_size
}

/* TODO: Below sections are required by the Nordic SDK and were copy-pasted from a BLE example code.
 * Consider revising them, especially for modular builds */
INCLUDE linker_nrf52840_sdk_ram.ld

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Interrupt vector table */
    .vectors :
    {
        . = ALIGN(4);
        link_interrupt_vectors_location = .;
        KEEP (*(.isr_vector))
        link_interrupt_vectors_location_end = .;
        . = ALIGN(4);
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
        
        . = ALIGN(4);
        
        /* Read-only data (constants) */
        link_const_data_start = .;
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.r.*)
        link_const_data_end = .;

        . = ALIGN(4);

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

        . = ALIGN(4);
    } > APP_FLASH  AT> APP_FLASH

    /* nRF52840 specific sections */
    INCLUDE linker_nrf52840_sdk_libs.ld

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    INCLUDE linker_build_id.ld
    
    /* Static RAM start */
    INCLUDE linker_static_ram_start.ld

    /* Ram interrupt vector table - must be 512 byte-aligned.
     * See Cortext-M4 Devices Generic User Guide, Section 4.3.4, Vector Table Offset Register. */
    .ram_vectors :
    {
        link_ram_interrupt_vectors_location = .;
        . = . + platform_vector_table_flash_size;
        link_ram_interrupt_vectors_location_end = .;
    } > SRAM

    /* Non-zero initialised global and static variables */
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
    
    /* Static RAM end */
    INCLUDE linker_static_ram_end.ld

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

/* Heap location */
link_heap_location     = link_bss_end;
link_heap_location_end = platform_monolithic_firmware_stack_start;

/* Stack , used by startup*/
link_stack_size     = platform_monolithic_firmware_stack_size;
link_stack_location = platform_monolithic_firmware_stack_start;
link_stack_end      = platform_monolithic_firmware_stack_end;

/* Referenced by SoftDevice driver. */
__data_start__ = link_static_ram_start;
__data_end__   = link_global_data_end;


/* Basic sanity checks */
ASSERT (
    (ORIGIN(SRAM) >= platform_ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= platform_ram_end) &&
    (ORIGIN(APP_FLASH) >= platform_flash_start && ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) <= platform_flash_end) &&
    (link_module_start == ORIGIN(APP_FLASH)) &&
    (link_module_start == platform_monolithic_firmware_flash_start) &&
    (link_static_ram_start == ORIGIN(SRAM)) &&
    (link_static_ram_start == platform_monolithic_firmware_static_ram_start),
    "Invalid memory boundary!"
);
ASSERT (
    (link_interrupt_vectors_location == platform_monolithic_firmware_vector_table_flash_start) &&
    (link_interrupt_vectors_location_end == platform_monolithic_firmware_vector_table_flash_end) &&
    (link_interrupt_vectors_location_end - link_interrupt_vectors_location == platform_vector_table_flash_size),
    "Invalid interrupt vector table!"
);
ASSERT (
    (link_module_info_start < link_module_info_end) &&
    (link_module_info_suffix_start - link_module_info_static_start == 4 || link_module_info_suffix_start - link_module_info_static_start == 6) &&
    (link_module_info_suffix_start < link_module_info_suffix_end) &&
    (link_module_info_crc_start < link_module_info_crc_end) &&
    (link_module_info_start == platform_monolithic_firmware_module_info_flash_start) &&
    (link_module_info_end == platform_monolithic_firmware_module_info_flash_end),
    "Invalid module info!"
);
ASSERT (
    (link_constructors_location % 4 == 0),
    "constructor array should be aligned to a 4-bytes!"
);
ASSERT (
    (link_ram_interrupt_vectors_location == platform_monolithic_firmware_static_ram_start) &&
    (link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location == platform_vector_table_flash_size) &&
    (link_ram_interrupt_vectors_location % 512 == 0),
    "Invalid RAM interrupt vector table!"
);
ASSERT (
    (link_static_ram_end - link_static_ram_start <= platform_monolithic_firmware_static_ram_size),
    "Insufficient room for .data and .bss sections!"
);
ASSERT (
    (link_heap_location_end > link_heap_location) &&
    (link_heap_location_end <= platform_monolithic_firmware_stack_start),
    "Insufficient room for heap!"
);
ASSERT (
    (link_stack_end % 8 == 0),
    "Stack should be 8-byte aligned!"
);
