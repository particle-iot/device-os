SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld
    
    /* Interrupt vector table */
    /* No interrupt vector table for user part. */
    
    /* Module info */
    INCLUDE linker_module_info.ld

    .text :
    {
        . = ALIGN(4);
        *(.xip.text)
        KEEP (*(*.dynalib_preinit))
        . = ALIGN(4);
    } > APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Dynalib table */
    .dynalib :
    {
        link_psram_code_flash_start = LOADADDR( .dynalib );
        link_dynalib_flash_start = LOADADDR( .dynalib );
        . = ALIGN(4);
        link_psram_code_start = .;
        link_dynalib_start = .;
        KEEP (*(*.user_part_module))
        link_dynalib_end = . ;
    } > PSRAM AT> APP_FLASH

    .psram_text :
    {
        . = ALIGN(4);

        /* Code */
        *(.flashtext) /* Realtek specific */
        *(.text)
        *(.text.*)

        /* Realtek specific */
        *(.image2.ram.text)
        *(.image2.ram.text.*)
        *(.boot.ram.text)
        *(.boot.ram.text.*)
        *(.ram.text)
        *(.ram.text.*)

        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)

        . = ALIGN(4);

        /* Read-only data (constants) */
        link_const_data_start = .;
        *(.rodata)
        *(.rodata*)
        *(.boot.rodata) /* Realtek specific */
        *(.boot.rodata*) /* Realtek specific */
        *(.gnu.linkonce.r.*)
        link_const_data_end = .;

        . = ALIGN(4);

        /* This is used by module_user_init() function in order to invoke C++ constructors */
        link_constructors_location = .;
        KEEP (*(.preinit_array))
        KEEP (*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        /* This is used by module_user_init() function in order to invoke C++ constructors */
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

    /* Non-zero initialised global and static variables */
    .data :
    {
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data );
        . = ALIGN(4);
        /* This is used by the user part preinit function in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        *(.boot.ram.data) /* Realtek specific */
        *(.boot.ram.data.*) /* Realtek specific */
        . = ALIGN(4);
        /* This is used by the user part preinit function in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(4);
        /* This is used by the user part preinit function in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(.boot.ram.bss) /* Realtek specific */
        *(.boot.ram.bss*) /* Realtek specific */
        *(COMMON)
        . = ALIGN(4);
        /* This is used by the user part preinit function in order to initialize the .bss secion */
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
    
    /* User backup ram data */
    INCLUDE linker_backup_ram_user.ld

    /* Module end */
    /* INCLUDE linker_module_end.ld */
    .module_info_suffix :
    {
        . = ALIGN(., 2);
        link_module_info_static_start = .;
        SHORT (0xffff)
        SHORT (0xffff)

        KEEP (*.o(.modinfo.product_id))
        KEEP (*.o(.modinfo.product_version))

        /* To make sure the following sections are located at the end of APP_FLASH */
        FILL (0xAA)
        . = DEFINED(platform_user_part_trimmed) ? ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) - 52 : .; /* FIXME: magic number */

        link_module_info_suffix_start = .;
        KEEP (*.o(.modinfo.module_info_suffix))
        link_module_info_suffix_end = .;
    } > APP_FLASH

    .module_end :
    {
        link_module_end = .;
    } > APP_FLASH

    .module_info_crc :
    {
        link_module_info_crc_start = .;
        KEEP (*.o(.modinfo.module_info_crc))
        link_module_info_crc_end = .;
    } > APP_FLASH

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

/* This can be used to calculate the OTA top boundary */
link_user_part_flash_start = platform_user_part_flash_start;

INCLUDE linker_rtl872x_rom.ld
