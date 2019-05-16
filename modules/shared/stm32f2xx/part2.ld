
SECTIONS
{
    INCLUDE module_start.ld

    .vectors :
    {
        link_interrupt_vectors_location = .;
        KEEP(*(*.interrupt_vector_table))
        link_interrupt_vectors_location_end = .;
    }>APP_FLASH  AT> APP_FLASH

    interrupt_vectors_length = link_interrupt_vectors_location_end - link_interrupt_vectors_location;

    INCLUDE module_info.ld

    /* 97*4=388 is vector table size. */
    expected_dynalib_start = ORIGIN (APP_FLASH) + 0x184 + 24;
    .dynalib :
    {
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
        chk_system_pre_init_start = .;
        KEEP (*(.module_pre_init))
        chk_system_pre_init_end = .;
        KEEP(*(.preinit_array))
        KEEP(*(.init_array))
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        chk_system_init_start = .;
        KEEP (*(.module_init))
        chk_system_init_end = .;
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

    /* Heap comes immediately after system-part1 */
    link_heap_location = ALIGN(system_part1_module_ram_end, 4);
    link_heap_location_end = system_heap_end;

    padding_length = ALIGN(system_part2_ram_start - ORIGIN(SRAM), 512);

    .padding : /* 512 byte aligned offset in SRAM */
    {
        link_padding_start = .;
        . = . + padding_length;
        link_static_ram_start = .;
    }> SRAM

    .ram_vectors : /* Ram VTOR table - must be 512 byte-aligned */
    {
        link_ram_interrupt_vectors_location = .;
        . = . + interrupt_vectors_length;
        link_ram_interrupt_vectors_location_end = .;
    }> SRAM

    INCLUDE backup_ram_system.ld

    .data : /* Contains the non-zero initialised global variables */
    {
        _sidata = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_start = .;
        _sdata = . ;
        *(.data*)
        link_global_data_end = .;
        _edata = . ;
        . = ALIGN(., 4);
    }> SRAM AT> APP_FLASH

    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        _sbss = ALIGN(., 4);
        link_bss_location = ALIGN(., 4);
        *(.bss*)
        *(COMMON)
        link_bss_end = .;
        _ebss = .;
        . = ALIGN(., 4);
        link_end_of_static_ram = .;
    }> SRAM AT> SRAM

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

    INCLUDE module_end.ld
}

/* Declare libc Heap to start at end of allocated RAM */

PROVIDE( _heap = link_heap_location );

/* End of the heap before static RAM, aligned 8 byte */

PROVIDE( _eheap = ALIGN( link_static_ram_start - 8, 8 ) );

/* ThreadX aliases */
PROVIDE( __RAM_segment_used_end__ = link_stack_end );
PROVIDE( __tx_free_memory_start = link_heap_location );
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


ASSERT ( link_interrupt_vectors_location == ORIGIN(APP_FLASH), "VTOR table should be at start of flash" )
ASSERT( link_padding_start == ORIGIN( SRAM ), "Padding start should be at start of RAM" );
ASSERT( ( link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location ) == VTOR_Length, "Expected RAM VTOR table to be same length as VTOR Flash" );

ASSERT ( link_dynalib_start != link_dynalib_end , "Dynalib table not found" );

ASSERT ( link_dynalib_start == expected_dynalib_start, "Dynalib location not correct" );

ASSERT ( link_dynalib_start == system_part2_module_table, "Dynalib location not same as exported location" );

ASSERT ( link_ram_interrupt_vectors_location >= system_part1_module_ram_end, "RAM vtor table should be at end of part1 static ram. " );
ASSERT ( ( link_ram_interrupt_vectors_location % 512 ) == 0, "RAM vtor table should be 512 byte aligned. " );

ASSERT ( chk_system_pre_init_start < chk_system_pre_init_end , "system_part2_pre_init function not included" );
ASSERT ( chk_system_init_start < chk_system_init_end , "system_part2_init function not included" );

expected_stack_end = ORIGIN(SRAM) + LENGTH(SRAM) ;

/* place stack at the end of SRAM, but not as a section, since this will increase the BSS region to ca. 128K */
/* Provide stack location to WICED linker symbols */
link_stack_location = stack_start;
link_stack_end = stack_end;
_estack = stack_end;

ASSERT ( link_stack_end - link_stack_location >= stack_size, "stack is too small" );

ASSERT ( link_stack_end == expected_stack_end, "stack should be at the top of SRAM" );

min_end_of_heap = link_heap_location + min_heap_size ;
ASSERT ( min_end_of_heap < link_static_ram_start, "Insufficient room for heap" );

ASSERT ( PLATFORM_DFU == ORIGIN(APP_FLASH), "Platform DFU and APP_FLASH origin differ" );

ASSERT ( system_part2_ram_start == link_static_ram_start, "exported SRAM start different from linked SRAM start." );
ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );
ASSERT ( system_part2_module_info_start == link_module_info_start, "part2 module info start not where expected" );
ASSERT ( system_part2_module_info_end == link_module_info_end, "part2 module info end not where expected" );

ASSERT ( ORIGIN(SRAM) == system_part2_ram_start, "ORIGIN of SRAM should be equal to system_part2_ram_start. Adjust by hand to obtain the correct address and length. ");
ASSERT ( LENGTH(SRAM) == ( system_part2_ram_size + stack_size ), "LENGTH of SRAM should be equal to system_static_ram_size plus stack_size. Adjust by hand to obtain the correct address and length. ");

ASSERT ( (link_bss_end - link_global_data_start) <= system_part2_ram_size, "Insufficient room for .data and .bss sections" )

ASSERT ( link_end_of_static_ram <= system_part2_ram_end, "static RAM use exceeds allocated space" );
