

/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_wifi_export.ld

reset_handler = _start;

ENTRY( _start );

/* todo - import defines for mem layout from system module 1 */

MEMORY
{
    DCT1_FLASH                 (rx)  : ORIGIN = 0x08004000,     LENGTH = 16K
    DCT2_FLASH                 (rx)  : ORIGIN = 0x08008000,     LENGTH = 16K
    APP_FLASH                  (rx)  : ORIGIN = 0x08020000,     LENGTH = 128K

    SRAM      (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}


SECTIONS
{
    .vectors :
    {
        link_interrupt_vectors_location = .;
        KEEP(*(*.interrupt_vector_table))
        link_interrupt_vectors_location_end = .;
    }>APP_FLASH  AT> APP_FLASH

    interrupt_vectors_length = link_interrupt_vectors_location_end - link_interrupt_vectors_location;
    /* 97*4=388 is vector table size. Add 3 additional entries. */
    expected_dynalib_start = ORIGIN (APP_FLASH) + 0x190 ;
    .dynalib :
    {
        /* additional space for a few extra ISRs */
        . = . + 12 ;
        link_dynalib_start = . ;
        KEEP(*(*.system_part2_module))
        link_dynalib_end = . ;
    }>APP_FLASH AT>APP_FLASH


    .text :
    {
        . = ALIGN(4);

        link_code_location = .;
        KEEP(*(.text.irq ))
        KEEP (*(EXCLUDE_FILE (*module_system_hal.o) .module))
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
        chk_system_init_start = .;
        KEEP (*module_system_hal.o(.module))
        chk_system_init_end = .;
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

    .padding : /* 512 byte offset in SRAM */
    {
        link_padding_start = .;
        . = . + 512;
    }> SRAM
    
    .ram_vectors : /* Ram VTOR table - must be 512 byte-aligned */
    {
        link_ram_interrupt_vectors_location = .;
        . = . + interrupt_vectors_length;
        link_ram_interrupt_vectors_location_end = .;
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
        . = ALIGN(., 4);
    }> SRAM AT> SRAM

    .stack : /* Contains the initial stack */
    {
        link_stack_location = ALIGN(., 4);
        *(.stack)
        . = ALIGN(MAX(link_stack_location + __STACKSIZE__ , .), 4);
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

/* Declare libc Heap to start at end of allocated RAM */

PROVIDE( _heap = link_stack_end );

/* End of the heap is top of RAM, aligned 8 byte */

PROVIDE( _eheap = ALIGN( ORIGIN( SRAM ) + LENGTH( SRAM ) - 8, 8 ) );

/* ThreadX aliases */
PROVIDE( __RAM_segment_used_end__ = link_stack_end );
PROVIDE( __tx_free_memory_start = link_stack_end );
PROVIDE( __tx_vectors = link_interrupt_vectors_location );

/* Some defines to keep WICED's _start() happy */
PROVIDE( link_run_from_ram_code_size = 0);
PROVIDE( link_run_from_ram_code_flash_location = 0);
PROVIDE( link_run_from_ram_code_ram_location = 0);
PROVIDE( link_run_from_ram_code_ram_end = 0);

PROVIDE( VTOR_Length = interrupt_vectors_length );

PROVIDE( dct1_start_addr_loc     = ORIGIN( DCT1_FLASH ));
PROVIDE( dct1_size_loc           = LENGTH( DCT1_FLASH ));
PROVIDE( dct2_start_addr_loc     = ORIGIN( DCT2_FLASH ));
PROVIDE( dct2_size_loc           = LENGTH( DCT2_FLASH ));


ASSERT( link_padding_start == ORIGIN( SRAM ), "Padding start should be at start of RAM" );
ASSERT( ( link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location ) == VTOR_Length, "Expected RAM VTOR table to be same length as VTOR Flash" );

ASSERT ( link_dynalib_start != link_dynalib_end , "Dynalib table not found" );

ASSERT ( link_dynalib_start == expected_dynalib_start, "Dynalib location not correct" ); 

/* Include this modules static exports for validation */
INCLUDE module_system_hal_export.ld

ASSERT ( link_dynalib_start == system_part2_module_table, "Dynalib location not same as exported location" );

ASSERT ( link_ram_interrupt_vectors_location == system_part1_module_ram_end, "RAM vtor table should be at end of part1 static ram. " );

ASSERT ( chk_system_init_start < chk_system_init_end , "system_part2_init function not included" );