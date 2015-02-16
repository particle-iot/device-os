
INCLUDE module_system_part1_export.ld
INCLUDE module_system_part2_export.ld

MEMORY
{
    APP_FLASH                  (rx)  : ORIGIN = 0x080A0000,     LENGTH = 128K

    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    expected_dynalib_start = ORIGIN (APP_FLASH) ;
    .dynalib :
    {
        link_dynalib_start = . ;
        KEEP(*(*.user_part_module))
        link_dynalib_end = . ;
    }>APP_FLASH AT>APP_FLASH


    .text :
    {
        . = ALIGN(4);

        link_code_location = .;
        KEEP(*(.text.irq ))
        *(.text .text.* .gnu.linkonce.t.*)
        link_code_end = .;

        . = ALIGN(0x4);

        link_const_variable_data_location = .;
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        link_const_variable_data_end = .;

        . = ALIGN(0x4);

        link_constructors_location = .;
        KEEP(*(.preinit_array))
        KEEP(*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        link_constructors_end = .;

        . = ALIGN(0x4);

        link_destructors_location = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        link_destructors_end = .;

        . = ALIGN(16);

    }>APP_FLASH  AT> APP_FLASH

    /*
     * The .ARM.exidx and .ARM.extab sections are used for C++ exception handling.
     * It is located here for completeness. Bare-metal ARM projects
     * typically cannot afford the overhead associated with C++
     * exceptions handling.
     */
    .ARM.exidx :
    {
        __exidx_start = ALIGN(4);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > APP_FLASH  AT> APP_FLASH

    .ARM.extab :
    {
        __extab_start = ALIGN(4);
        *(.ARM.extab*)
        __extab_end = .;
    } > APP_FLASH  AT> APP_FLASH

    padding_length = ALIGN( system_static_ram_end - ORIGIN(SRAM), 4) ; 

    .padding : /* move location past system static RAM region */
    {
        link_padding_start = .;
        . = . + padding_length;
    }> SRAM
    
    .data : /* Contains the non-zero initialised global variables */
    {
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */        
        link_global_data_start = .;
        *(.data*)
        link_global_data_end = .;
        . = ALIGN(., 4);
    }> SRAM AT> APP_FLASH


    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        link_bss_location = ALIGN(., 4);
        *(.bss*)
        *(COMMON)
        link_bss_end = .;
        link_heap_start = .;
        . = ALIGN(., 4);
    }> SRAM AT> SRAM

    /* This is here just to validate that there is sufficient space available for the stack allocated by the system module. */
    .stack : /* Contains the initial stack */
    {
        link_stack_location_user = ALIGN(., 4);
        *(.stack)
        . = ALIGN(MAX(link_stack_location_user + stack_size , .), 4);
        link_stack_end = .;
    }> SRAM AT> SRAM

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

sram_end = ORIGIN(SRAM) + LENGTH(SRAM);

ASSERT( link_padding_start == ORIGIN( SRAM ), "Padding start should be at start of RAM" );

ASSERT ( link_dynalib_start != link_dynalib_end , "Dynalib table not found" );

ASSERT ( link_dynalib_start == expected_dynalib_start, "Dynalib location not correct" ); 

/* Include this modules static exports for validation */
INCLUDE module_user_export.ld

ASSERT ( link_dynalib_start == user_module_table, "Dynalib location not same as exported location" );

ASSERT ( USER_FIRMWARE_IMAGE_LOCATION == ORIGIN(APP_FLASH), "User firmware location and APP_FLASH origin differ" );

ASSERT ( USER_FIRMWARE_IMAGE_SIZE == LENGTH(APP_FLASH), "User firmware size and APP_FLASH length differ" );


ASSERT ( link_heap_start > ORIGIN(SRAM) && link_heap_start < sram_end, "link_heap_start is in the wrong memory space" );