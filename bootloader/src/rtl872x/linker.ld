/* Memory Spaces Definitions */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld
  
    SRAM        (xrw) : ORIGIN = platform_bootloader_static_ram_start, LENGTH = platform_bootloader_static_ram_size
    SECURE_SRAM (xrw) : ORIGIN = platform_secure_ram_start,            LENGTH = platform_secure_ram_size - platform_bootloader_stack_size
    APP_FLASH   (rx)  : ORIGIN = platform_bootloader_flash_start,      LENGTH = platform_bootloader_flash_size
}

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /*----------------------------- START OF XIP PARTITION ------------------------------
     * Don't need to be copied to SRAM nor SECURE_SRAM:
     * - XIP partition header
     * - XIP code
     * - RAM partition header
     */
    .rtl_xip_image_header :
    {
        . = ALIGN(4);
        KEEP (*(.rtl_header_xip))
        . = ALIGN(4);
        link_rtl_xip_start = .;
    } > APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld

    .text :
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
    } > APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    .rtl_ram_image_header :
    {
        . = ALIGN(16); /* The image header must be 16-bytes aligned. */
        link_rtl_xip_end = .;
        KEEP (*(.rtl_header_ram))
        . = ALIGN(4);
    } > APP_FLASH

    PROVIDE (link_rtl_xip_size = link_rtl_xip_end - link_rtl_xip_start);
    /*----------------------------- END OF XIP PARTITION ------------------------------*/

    /*----------------------------- START OF RAM CODE PARTITION -------------------------
     * Needs to be copied to SECURE_SRAM. This is done by KM4 boot ROM according to the RAM partition header
     * - start table
     * - RAM code
     */
    .rtl_start_table : /* Realtek specific */
    {  
        link_rtl_ram_start = .;
        KEEP (*(.rtl_header_ram_start_table))
        KEEP (*(.rtl_header_ram_valid_pattern))
        KEEP (*(.rtl_header_ram_boot_export_table))
    } > SECURE_SRAM AT> APP_FLASH

    .ram_text : /* Realtek specific */
    {
        . = ALIGN(4);
        link_ram_code_start = .;
        *(.image2.ram.text)
        *(.image2.ram.text.*)
        *(.boot.ram.text)
        *(.boot.ram.text.*)
        *(.ram.text)
        *(.ram.text.*)
        link_ram_code_end = .;
        link_rtl_ram_end = .;
        . = ALIGN(4);
    } > SECURE_SRAM AT> APP_FLASH

    PROVIDE (link_rtl_ram_size = link_rtl_ram_end - link_rtl_ram_start);
    /*----------------------------- END OF RAM CODE PARTITION -------------------------*/
    
    /*----------------------------- START OF DATA PARTITION -----------------------------
     * Needs to be copied to SRAM. This is done by the startup file
     * - RAM data
     * - RAM bss initialization
     */
    /* Static RAM start */
    INCLUDE linker_static_ram_start.ld

    /* Non-zero initialised global and static variables */
    .data :
    {
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        *(.boot.ram.data) /* Realtek specific */
        *(.boot.ram.data.*) /* Realtek specific */
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
        *(.boot.ram.bss) /* Realtek specific */
        *(.boot.ram.bss*) /* Realtek specific */
        *(COMMON)
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_end = .;
    } > SRAM AT> SRAM
    
    /* Static RAM end */
    INCLUDE linker_static_ram_end.ld
    /*----------------------------- END OF DATA PARTITION -----------------------------*/

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
link_heap_location_end = link_heap_location + platform_heap_min_size;

/* Stack , used by startup*/
link_stack_size     = platform_bootloader_stack_size;
link_stack_location = platform_bootloader_stack_start;
link_stack_end      = platform_bootloader_stack_end;

INCLUDE linker_rtl872x_rom.ld
