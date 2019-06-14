
SECTIONS
{
  . = ALIGN(4);
  .svc_data :
  {
    PROVIDE(__start_svc_data = .);
    KEEP(*(.svc_data))
    PROVIDE(__stop_svc_data = .);
  } > SRAM
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > SRAM
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > SRAM
  .cli_sorted_cmd_ptrs :
  {
    PROVIDE(__start_cli_sorted_cmd_ptrs = .);
    KEEP(*(.cli_sorted_cmd_ptrs))
    PROVIDE(__stop_cli_sorted_cmd_ptrs = .);
  } > SRAM
} INSERT AFTER .data;

SECTIONS
{
    
    INCLUDE module_start.ld
    
    .vectors :
    {
        link_interrupt_vectors_location = .;
        KEEP(*(.isr_vector))            /* interrupt vector table */
        link_interrupt_vectors_location_end = .;
    }>APP_FLASH  AT> APP_FLASH

    interrupt_vectors_length = link_interrupt_vectors_location_end - link_interrupt_vectors_location;

    INCLUDE module_info.ld
    
    /* 0x200 is vector table size. */
    expected_dynalib_start = ORIGIN (APP_FLASH) + 0x200 + 24;
    .dynalib :
    {
        link_dynalib_start = . ;
        KEEP(*(*.system_part1_module))
        link_dynalib_end = . ;
    }>APP_FLASH AT>APP_FLASH

    .text :
    {
        . = ALIGN(4);

        *(.text)                   /* remaining code */
        *(.text.*)                   /* remaining code */
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)

        . = ALIGN(4);

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

        . = ALIGN(0x4);
        _etext = .;
        /* This is used by the startup in order to initialize the .data secion */
    }>APP_FLASH  AT> APP_FLASH

    .mem_section_dummy_rom :
    {
    }
    .sdh_soc_observers :
    {
      PROVIDE(__start_sdh_soc_observers = .);
      KEEP(*(SORT(.sdh_soc_observers*)))
      PROVIDE(__stop_sdh_soc_observers = .);
    } > APP_FLASH AT> APP_FLASH
    .sdh_ble_observers :
    {
      PROVIDE(__start_sdh_ble_observers = .);
      KEEP(*(SORT(.sdh_ble_observers*)))
      PROVIDE(__stop_sdh_ble_observers = .);
    } > APP_FLASH AT> APP_FLASH
    .pwr_mgmt_data :
    {
      PROVIDE(__start_pwr_mgmt_data = .);
      KEEP(*(SORT(.pwr_mgmt_data*)))
      PROVIDE(__stop_pwr_mgmt_data = .);
    } > APP_FLASH AT> APP_FLASH
    .log_const_data :
    {
      PROVIDE(__start_log_const_data = .);
      KEEP(*(SORT(.log_const_data*)))
      PROVIDE(__stop_log_const_data = .);
    } > APP_FLASH AT> APP_FLASH
      .nrf_balloc :
    {
      PROVIDE(__start_nrf_balloc = .);
      KEEP(*(.nrf_balloc))
      PROVIDE(__stop_nrf_balloc = .);
    } > APP_FLASH AT> APP_FLASH
    .sdh_state_observers :
    {
      PROVIDE(__start_sdh_state_observers = .);
      KEEP(*(SORT(.sdh_state_observers*)))
      PROVIDE(__stop_sdh_state_observers = .);
    } > APP_FLASH AT> APP_FLASH
    .sdh_stack_observers :
    {
      PROVIDE(__start_sdh_stack_observers = .);
      KEEP(*(SORT(.sdh_stack_observers*)))
      PROVIDE(__stop_sdh_stack_observers = .);
    } > APP_FLASH AT> APP_FLASH
    .sdh_req_observers :
    {
      PROVIDE(__start_sdh_req_observers = .);
      KEEP(*(SORT(.sdh_req_observers*)))
      PROVIDE(__stop_sdh_req_observers = .);
    } > APP_FLASH AT> APP_FLASH
      .nrf_queue :
    {
      PROVIDE(__start_nrf_queue = .);
      KEEP(*(.nrf_queue))
      PROVIDE(__stop_nrf_queue = .);
    } > APP_FLASH AT> APP_FLASH
      .cli_command :
    {
      PROVIDE(__start_cli_command = .);
      KEEP(*(.cli_command))
      PROVIDE(__stop_cli_command = .);
    } > APP_FLASH AT> APP_FLASH
    .crypto_data :
    {
      PROVIDE(__start_crypto_data = .);
      KEEP(*(SORT(.crypto_data*)))
      PROVIDE(__stop_crypto_data = .);
    } > APP_FLASH AT> APP_FLASH

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
    
    padding_length = ALIGN(system_part1_static_ram_start - ORIGIN(SRAM), 512);

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

    .fast : /* This section contains code that is run from RAM after being loaded from flash - functions can be put in this section with the C attribute: __attribute__ ((section (".fast"))) */
    {
        link_run_from_ram_code_flash_location = LOADADDR( .fast ); /* This is the location in flash of the code */
        link_run_from_ram_code_ram_location = .;
        *(.fast .fast.* .text.fastcode)
        link_run_from_ram_code_ram_end = .;
    }> SRAM AT> APP_FLASH

    .data : /* Contains the non-zero initialised global variables */
    {
        _sidata = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); /* This is the location in flash of the initial values of global variables */
        link_global_data_start = .;
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;
        __data_start__ = _sdata;
        *(.data*)
        link_global_data_end = .;
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _edata = . ;
        __data_end__ = _sdata;
    }> SRAM AT> APP_FLASH


    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        _sbss = ALIGN(., 4);
        link_bss_location = ALIGN(., 4);
        __bss_start__ = _sbss;
        *(.bss*)
        *(COMMON)
        link_bss_end = .;
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _ebss = . ;
        __bss_end__ = _ebss;
    }> SRAM AT> SRAM

    .stack __Stack_Init : /* Contains the initial stack */
    {
        link_stack_location = .;
        *(.stack)
        link_stack_end = .;
    }> SRAM AT> SRAM

    INCLUDE backup_ram_user.ld
    INCLUDE backup_ram_system.ld

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );
    
    link_heap_location = _ebss;
    link_heap_location_end = __Stack_Init;


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

/* End of the heap is top of RAM, aligned 8 byte */
PROVIDE( _eheap = ALIGN( link_heap_location_end - 7, 8 ) );

/* ThreadX aliases */
PROVIDE( __RAM_segment_used_end__ = link_stack_end );
PROVIDE( __tx_free_memory_start = link_stack_end );
PROVIDE( __tx_vectors = link_interrupt_vectors_location );

PROVIDE( VTOR_Length = interrupt_vectors_length );
ASSERT ( interrupt_vectors_length == 0x200, "vector table length invalid");

/* Dynalib assertion */
ASSERT ( link_dynalib_start != link_dynalib_end , "Dynalib table not found" );
ASSERT ( link_dynalib_start == expected_dynalib_start, "Dynalib location not correct" );
ASSERT ( link_dynalib_start == system_part1_module_table, "Dynalib location not same as exported location" );

/* RAM interrupt vector table assertion */
ASSERT( link_ram_interrupt_vectors_location == ORIGIN( SRAM ), "RAM Interrupt table should be at start of RAM" );
ASSERT( ( link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location ) == VTOR_Length, "Expected RAM VTOR table to be same length as VTOR Flash" );

/* System init assertion */
ASSERT ( chk_system_pre_init_start < chk_system_pre_init_end , "system_part1_pre_init function not included" );
ASSERT ( chk_system_init_start < chk_system_init_end , "system_part1_init function not included" );

/* Heap assertion*/
ASSERT ( link_heap_location + min_heap_size <= __Stack_Init, "Insufficient room for heap" );

ASSERT ( PLATFORM_DFU == ORIGIN(APP_FLASH), "Platform DFU and APP_FLASH origin differ" );

ASSERT ( system_part1_static_ram_start == link_static_ram_start, "exported SRAM start different from linked SRAM start." );
ASSERT ( system_part1_static_ram_start == ORIGIN(SRAM), "ORIGIN of SRAM should be equal to system_part1_static_ram_start. Adjust by hand to obtain the correct address and length. ");
ASSERT ( (link_bss_end - link_global_data_start) <= system_part1_static_ram_size, "Insufficient room for .data and .bss sections" )

ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );
ASSERT ( system_part1_module_info_start == link_module_info_start, "part1 module info start not where expected" );
ASSERT ( system_part1_module_info_end == link_module_info_end, "part1 module info end not where expected" );

ASSERT ( ( link_constructors_location % 4 ) == 0, "constructor array should be aligned to a 4-bytes" );

INCLUDE backup_ram_system_assert.ld
