/* Memory Spaces Definitions */
INCLUDE platform_ram_m23.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld
  
    SRAM          (xrw) : ORIGIN = platform_km0_part1_ram_start,   LENGTH = platform_km0_part1_ram_size
    APP_FLASH     (rx)  : ORIGIN = platform_km0_part1_flash_start, LENGTH = platform_km0_part1_flash_size
    RETENTION_RAM (xrw) : ORIGIN = platform_retention_ram_start,   LENGTH = platform_retention_ram_size
}

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld

    /* Module info */
    INCLUDE linker_module_info.ld

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Dynalib table */
    .dynalib : ALIGN(8)
    {
        link_dynalib_flash_start = LOADADDR( .dynalib );
        . = ALIGN(4);
        link_dynalib_start = . ;
        KEEP (*(*.bootloader_part1_module))
        link_dynalib_end = . ;
    } > SRAM AT> APP_FLASH

    .text :
    {
        . = ALIGN(4);
        KEEP (*(.xip.text))
        KEEP (*(*.dynalib_part1_preinit))
        . = ALIGN(4);
    } > APP_FLASH

    INCLUDE linker_build_id.ld

    .ram_text :
    {
        link_ram_copy_flash_start = LOADADDR( .ram_text );
        . = ALIGN(4);
        link_ram_copy_start = .;
        
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
        link_ram_copy_end = .;
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

    .ram_retention.entry : 
	{
        link_retention_ram_start = .;
		KEEP(*(SORT(.retention.entry.data*)))
        link_retention_ram_end = .;
	} > RETENTION_RAM
    
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
link_heap_location_end = platform_km0_ram_end - platform_km0_bootloader_stack_size;

/* Stack , used by startup*/
link_stack_size     = platform_km0_bootloader_stack_size;
link_stack_location = platform_km0_bootloader_stack_start;
link_stack_end      = platform_km0_bootloader_stack_end;

INCLUDE linker_rtl872x_rom_m23.ld
