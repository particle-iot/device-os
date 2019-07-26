SECTIONS
{
    INCLUDE linker_module_start.ld

    .boot_jump : /* immitation vector table */
    {
        . = ALIGN(512);
        link_boot_table_start = .;
        KEEP(*(*.system_part1_boot_table))
        link_boot_table_end = .;
    } > APP_FLASH AT> APP_FLASH

    INCLUDE linker_module_info.ld

    .dynalib :
    {
        link_module_jump_table_start = . ;
        KEEP(*(*.system_part1_module))
        link_module_jump_table_end = . ;
    } > APP_FLASH  AT> APP_FLASH

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

    } > APP_FLASH  AT> APP_FLASH

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

    .data : /* Contains the non-zero initialised global variables */
    {
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_start = .;
        *(.data*)
        link_global_data_end = .;
        . = ALIGN(., 4);
    } > SRAM AT> APP_FLASH

    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        link_bss_location = ALIGN(., 4);
        *(.bss*)
        *(COMMON)
        link_bss_end = .;
        . = ALIGN(., 4);
        link_end_of_static_ram = .;
    } > SRAM AT> SRAM

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

    INCLUDE linker_module_end.ld
}

/**
 * These asserts verify that things are at their expected or exported location.
 * The export linker file defines symbols in absolute terms (a hard-coded address)
 * These asserts verify the exported symbol values match the actual values assigned during the link.
 */

ASSERT( link_boot_table_start == ORIGIN(APP_FLASH), "boot table should be at start of APP FLASH" );
ASSERT( link_module_jump_table_end != link_module_jump_table_start, "module master jump table not linked" );

ASSERT ( link_module_jump_table_start == _system_part1_dynalib_table_flash_start, "Export address for module table is incorrect" );
ASSERT ( link_end_of_static_ram <= _system_part1_static_ram_end, "static RAM use exceeds allocated space" );

ASSERT ( PLATFORM_DFU == ORIGIN(APP_FLASH), "Platform DFU and APP_FLASH origin differ" );

ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );

ASSERT ( link_module_info_start == _system_part1_module_info_flash_start, "module info start not where expected" );
ASSERT ( link_module_info_end == _system_part1_module_info_flash_end, "module info end not where expected" );

ASSERT ( link_boot_table_start == ORIGIN(APP_FLASH), "boot table should be at start of flash region" );
ASSERT ( link_boot_table_start == _system_part1_vector_table_flash_start, "boot table export address is not correct" );

link_boot_table_size = link_boot_table_end - link_boot_table_start ;
ASSERT ( link_boot_table_size == _system_part1_vector_table_flash_size, "boot table not linked or incorrect size" );

ASSERT( ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end, "Static RAM region doesn't fit into RAM" );
