/* Memory Spaces Definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld
  
    SRAM       (xrw) : ORIGIN = platform_bootloader_ram_start,        LENGTH = platform_bootloader_ram_size
    SECURE_RAM (xrw) : ORIGIN = platform_secure_ram_start,            LENGTH = platform_secure_ram_size - platform_bootloader_stack_size
    APP_FLASH  (rx)  : ORIGIN = platform_bootloader_flash_start,      LENGTH = platform_bootloader_flash_size
    PSRAM      (rwx) : ORIGIN = platform_bootloader_psram_start,      LENGTH = platform_bootloader_psram_size
}

SECTIONS
{
    .psram (NOLOAD):
    {
        . = ALIGN(4);
        link_psram_location = .;
        *(.psram.*)
        . = ALIGN(4);
        link_psram_end = .;
    } > PSRAM AT> PSRAM

    /* Module start */
    INCLUDE linker_module_start.ld

    .rtl_xip_image_header :
    {
        . = ALIGN(4);
        KEEP (*(.rtl_header_xip))
        . = ALIGN(4);
    } > APP_FLASH

    /****************************** START OF XIP PARTITION ***********************************
     */
    .xip_start :
    {
        link_rtl_xip_start = .;
    } > APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Static RAM start */
    INCLUDE linker_static_ram_start.ld

    .text :
    {
        . = ALIGN(4);
        *(.xip.text)
        . = ALIGN(4);
    } > APP_FLASH

    INCLUDE linker_build_id.ld

    /* Non-zero initialised global and static variables */
    .data :
    {
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_start = .;

        link_ipc_data_start = .;
        *(.boot.ipc_data)
        FILL(0xff)
        . = ALIGN(16);
        link_ipc_data_end = .;

        /* Realtek specific RAM code */
        *(.image2.ram.text)
        *(.image2.ram.text.*)
        *(.boot.ram.text)
        *(.boot.ram.text.*)
        *(.ram.text)
        *(.ram.text.*)

        *(.data*)
        *(.boot.ram.data) /* Realtek specific */
        *(.boot.ram.data.*) /* Realtek specific */
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    .sram.text :
    {
        . = ALIGN(4);
        
        /* Code */
        link_code_start = .;
        *(.flashtext) /* Realtek specific */
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
        *(.boot.rodata) /* Realtek specific */
        *(.boot.rodata*) /* Realtek specific */
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
    } > SRAM AT> APP_FLASH
    
    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(.boot.ram.bss) /* Realtek specific */
        *(.boot.ram.bss*) /* Realtek specific */
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

    .xip_end :
    {
        . = ALIGN(16); /* The image end must be 16-bytes aligned. */
        link_rtl_xip_end = .;
        . = ALIGN(4);
    } > APP_FLASH

    PROVIDE (link_rtl_xip_size = link_rtl_xip_end - link_rtl_xip_start);
    /*
     ****************************** END OF XIP PARTITION *************************************/

    .rtl_ram_image_header :
    {
        . = ALIGN(16); /* The image header must be 16-bytes aligned. */
        KEEP (*(.rtl_header_ram))
        . = ALIGN(4);
    } > APP_FLASH

    /******************************* START OF RAM COPY PARTITION ******************************
     */
    .ram_copy_start :
    {
        link_rtl_ram_copy_start = .;
    } > SECURE_RAM

    .rtl_start_table : /* Realtek specific */
    {
        KEEP (*(.rtl_header_ram_start_table))
        KEEP (*(.rtl_header_ram_valid_pattern))
        KEEP (*(.rtl_header_ram_boot_export_table))
    } > SECURE_RAM AT> APP_FLASH

    .secure_ram_text : /* Realtek specific */
    {
        . = ALIGN(4);
        *(.secure.ram.text)
        . = ALIGN(4);
    } > SECURE_RAM AT> APP_FLASH

    .ram_copy_end :
    {
        link_rtl_ram_copy_end = .;
    } > SECURE_RAM


    PROVIDE (link_rtl_ram_copy_size = link_rtl_ram_copy_end - link_rtl_ram_copy_start);
    /*
     ****************************** END OF RAM COPY PARTITION ********************************/

    .rtl_flash_signature_footer :
    {
        link_rtl_secure_boot_footer = .;
        KEEP (*(.rtl_footer_signature))
    } > APP_FLASH

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

link_dynalib_flash_start = 0;
link_dynalib_start = 0;

/* To make the startup file happy */
PROVIDE (link_psram_code_flash_start = 0);
PROVIDE (link_psram_code_start = link_psram_code_flash_start);
PROVIDE (link_psram_code_end = link_psram_code_start);

PROVIDE (link_dynalib_flash_start = 0);
PROVIDE (link_dynalib_start = link_dynalib_flash_start);
PROVIDE (link_dynalib_end = link_dynalib_start);

/* Heap location */
link_heap_location     = link_bss_end;
link_heap_location_end = platform_ram_size - platform_backup_ram_all_size;

/* Stack , used by startup*/
link_stack_size     = platform_bootloader_stack_size;
link_stack_location = platform_bootloader_stack_start;
link_stack_end      = platform_bootloader_stack_end;

INCLUDE linker_rtl872x_rom_s.ld
