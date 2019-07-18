/* Stack */
_estack = _modular_firmware_stack_end;
__Stack_Size = _modular_firmware_stack_size;
__Stack_Init = _estack - __Stack_Size ;

PROVIDE ( _Stack_Size = __Stack_Size );
PROVIDE ( _Stack_Init = __Stack_Init );

/* Memory spaces definitions */
INCLUDE memory_user_part.ld

SECTIONS
{
    sram_start = ORIGIN(SRAM);
    sram_end = sram_start + LENGTH(SRAM);

    INCLUDE module_start.ld
    INCLUDE module_info.ld

    .dynalib :
    {
        expected_dynalib_start = ORIGIN(APP_FLASH) + _user_part_module_info_flash_size;
        link_dynalib_start = .;
        KEEP(*(*.user_part_module))
        link_dynalib_end = . ;
    } > APP_FLASH AT> APP_FLASH

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

    INCLUDE backup_ram_user.ld

    .data : /* Contains the non-zero initialised global variables */
    {
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        . = ALIGN(4);
        link_global_data_start = .;
        *(.data*)
        . = ALIGN(., 4);
        link_global_data_end = .;
    }> SRAM AT> APP_FLASH

    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        . = ALIGN(4);
        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(., 4);
        link_bss_end = .;
    }> SRAM AT> SRAM

    INCLUDE module_end.ld

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

PROVIDE ( end = system_heap_end );

/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld

/* Definitions for the static parts of the user module */
INCLUDE module_user_export.ld

ASSERT ( link_dynalib_start != link_dynalib_end , "Dynalib table not found" );
ASSERT ( link_dynalib_start == expected_dynalib_start, "Dynalib location not correct" );
ASSERT ( link_dynalib_start == _user_part_dynalib_table_flash_start, "Dynalib location not same as exported location" );

ASSERT ( USER_FIRMWARE_IMAGE_LOCATION == ORIGIN(APP_FLASH), "User firmware location and APP_FLASH origin differ" );
ASSERT ( USER_FIRMWARE_IMAGE_SIZE == LENGTH(APP_FLASH), "User firmware size and APP_FLASH length differ" );

ASSERT ( (link_bss_end - link_global_data_start) <= _user_part_static_ram_size, "Insufficient room for .data and .bss sections" )

ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );

ASSERT ( _user_part_module_info_flash_start == link_module_info_start, "module info start not where expected" );
ASSERT ( _user_part_module_info_flash_end == link_module_info_end, "module info end not where expected" );

ASSERT ( ( link_module_info_suffix_start - link_module_info_static_start ) == 4 || ( link_module_info_suffix_start - link_module_info_static_start ) == 8, "module info static details not linked" );

ASSERT( ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end, "Static RAM region doesn't fit into RAM" );
