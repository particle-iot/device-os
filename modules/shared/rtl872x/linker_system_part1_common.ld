
SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Module info */
    INCLUDE linker_module_info.ld

    /* Entry point */
    .entry :
    {
        . = ALIGN(4);
        KEEP (*(.system.entry))
        . = ALIGN(4);
    } >APP_FLASH

    .text :
    {
        . = ALIGN(4);
        *(.xip.text)
        . = ALIGN(4);
    } > APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    INCLUDE linker_build_id.ld

    /* Dynalib table */
    .dynalib : ALIGN(8)
    {
        link_dynalib_flash_start = LOADADDR( .dynalib );

        link_dynalib_start = . ;
        KEEP (*(*.system_part1_module))
        . = ALIGN(4);
        link_dynalib_end = . ;
    } > PSRAM AT> APP_FLASH

    .psram_text : ALIGN(4)
    {
        link_psram_code_flash_start = LOADADDR( .psram_text );
        link_psram_code_start = .;

        /* Code */
        *(.flashtext) /* Realtek specific */
        *(.text)
        *(.text.*)

        /* Realtek specific */
        *(.image2.ram.text)
        *(.image2.ram.text.*)
        *(.image2.net.ram.text)
        *(.image2.net.ram.text.*)

        *(.ram.text)
        *(.ram.text.*)
        *(.BTTRACE)

        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)

        . = ALIGN(4);
        
        /* Read-only data (constants) */
        link_const_data_start = .;
        *(.rodata)
        *(.rodata*)
        *(.image2.net.ram.rodata*)
        *(.image2.ram.rodata*)
        *(.gnu.linkonce.r.*)
        *(.boot.rodata) /* Realtek specific */
        *(.boot.rodata*) /* Realtek specific */
        link_const_data_end = .;
        
        . = ALIGN(4);

        /* This is used by the startup in order to invoke C++ constructors */
        link_constructors_location = .;

        /* Particle specific preinit functions for modular build only */
        link_module_pre_init_start = .;
        KEEP (*(.module_pre_init))
        link_module_pre_init_end = .;

        KEEP (*(.preinit_array))
        KEEP (*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))

        /* Particle specific init functions for modular build only */
        link_module_init_start = .;
        KEEP (*(.module_init))
        link_module_init_end = .;

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
        link_psram_code_end = .;
    } > PSRAM AT> APP_FLASH

    .data_alt :
    {
        link_global_data_initial_values_alt = LOADADDR( .data_alt ); 
        . = ALIGN(4);
        link_global_data_start_alt = .;
        *(.psram.data*)
        . = ALIGN(4);
        link_global_data_end_alt = .;
    } > PSRAM AT> APP_FLASH

    .bss_alt :
    {
        . = ALIGN(4);
        link_bss_location_alt = .;
        *(.psram.bss*)
        . = ALIGN(4);
        link_bss_end_alt = .;
    } > PSRAM

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
        *(.image2.ram.data*)
        *(.image2.net.ram.data*)
        . = ALIGN(4);
        *(.boot.ipc_data)
        *(.ram.sleep*)
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    .lib_wlan.nocache.data :
    {
        . = ALIGN (32);
        __ram_nocache_start__ = .;
        *(.bdsram.data*)
        *(.image2.ram.bdsram.data*)
        *(.image2.net.ram.bdsram.data*)
        . = ALIGN (32);
        __ram_nocache_end__ = .;
    } > SRAM AT> APP_FLASH

    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_location = .;

        /* Coalesces FreeRTOS symbols into a single location for some tooling */
        INCLUDE linker_freertos_task_symbols.ld

        *(.bss*)
        *(.image2.ram.bss*)
        *(.image2.net.ram.bss*)
        *(COMMON)
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_end = .;
    } > SRAM AT> SRAM
    
    /* Static RAM end */
    INCLUDE linker_static_ram_end.ld

    .secure_ram_text :
    {
        link_secure_ram_code_flash_start = LOADADDR( .secure_ram_text ); 
        . = ALIGN(4);
        link_secure_ram_code_start = .;
        /* Reserved */
        link_secure_ram_code_end = .;
        . = ALIGN(4);
    } > SECURE_SRAM AT> APP_FLASH

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

link_user_part_flash_end = platform_user_part_flash_end;

/* Heap location */
link_heap_location     = link_bss_end;
link_heap_location_end = platform_system_backup_ram_start;

link_heap_location_alt     = link_bss_end_alt;
link_heap_location_end_alt = link_heap_location_alt + platform_heap_min_size;

/* Stack , used by startup*/
link_stack_size     = platform_modular_firmware_stack_size;
link_stack_location = platform_modular_firmware_stack_start;
link_stack_end      = platform_modular_firmware_stack_end;

link_secure_stack_size     = platform_modular_firmware_secure_stack_size;
link_secure_stack_location = platform_modular_firmware_secure_stack_start;
link_secure_stack_end      = platform_modular_firmware_secure_stack_end;

INCLUDE linker_rtl872x_rom_ns.ld
