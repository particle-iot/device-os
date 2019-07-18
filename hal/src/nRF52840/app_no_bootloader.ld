/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*** REMOVED - WE ARE USING startup_stm32f2xx_electron.S
reset_handler = _start;

ENTRY( _start );
*/

/* Include memory map */
INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

/* default stack sizes.
 * These are used by the startup in order to allocate stacks for the different modes. */
_estack = _monolithic_firmware_stack_end;
__Stack_Size = _monolithic_firmware_stack_size;
__Stack_Init = _estack - __Stack_Size;

/* There will be a link error if there is not this amount of RAM free at the end. */
_Minimum_Stack_Size = 0x1400;

/* "PROVIDE" allows to easily override these values from an object file or the commmand line. */
PROVIDE ( _Stack_Size = __Stack_Size );
PROVIDE ( _Stack_Init = __Stack_Init );

/* Memory Spaces Definitions */
INCLUDE memory_no_bootloader.ld

/* TODO: Below sections are required by the Nordic SDK and were copy-pasted from a BLE example code.
 * Consider revising them, especially for modular builds */
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
    .vectors :
    {
        link_module_start = .;
        link_interrupt_vectors_location = .;
        KEEP(*(.isr_vector))            /* interrupt vector table */
        link_interrupt_vectors_location_end = .;
    }>APP_FLASH  AT> APP_FLASH

    interrupt_vectors_length = link_interrupt_vectors_location_end - link_interrupt_vectors_location;

    .module_info :
    {
        . = ALIGN(4);
        link_module_info_start = .;
        KEEP(*.o(.modinfo.module_info))
        link_module_info_end = .;
    }>APP_FLASH  AT> APP_FLASH

    .text :
    {
        . = ALIGN(4);

        *(.text)                   /* remaining code */
        *(.text.*)                   /* remaining code */
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)

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

        . = ALIGN(4);
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

    .ram_vectors : /* Ram VTOR table - must be 512 byte-aligned */
    {
        link_static_ram_start = .;
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
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_start = .;
        _sdata = . ;
        *(.data*)
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_end = .;
        _edata = . ;
    }> SRAM AT> APP_FLASH


    .bss : /* Zero initialised memory used for zero initialised variables */
    {
        . = ALIGN(4);
        _sbss = .;
        link_bss_location = .;
        __bss_start__ = _sbss;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_end = .;
        _ebss = . ;
        __bss_end__ = _ebss;
    }> SRAM AT> SRAM

    INCLUDE backup_ram_user.ld
    INCLUDE backup_ram_system.ld

    link_heap_location = end;
    link_heap_location_end = __Stack_Init;
    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );

    .stack __Stack_Init : /* Contains the initial stack */
    {
        link_stack_location = .;
        *(.stack)
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

    .module_info_suffix :
    {
        . = ALIGN(., 4) ;
        link_module_info_suffix_start = .;
        KEEP(*.o(.modinfo.module_info_suffix))
        link_module_info_suffix_end = .;
    }>APP_FLASH AT> APP_FLASH

    .module_end :
    {
        link_module_end = .;
    }> APP_FLASH AT>APP_FLASH

    .module_info_crc :
    {
        link_module_info_crc_start = .;
        KEEP(*.o(.modinfo.module_info_crc))
        link_module_info_crc_end = .;
    }>APP_FLASH AT> APP_FLASH
}

/* Referenced by SoftDevice driver. */
__data_start__ = link_static_ram_start;
__data_end__ = link_global_data_end;

/* Declare libc Heap to start at end of allocated RAM */
PROVIDE( _heap = link_stack_end );

/* End of the heap is top of RAM, aligned 8 byte */
PROVIDE( _eheap = ALIGN( ORIGIN( SRAM ) + LENGTH( SRAM ) - 8, 8 ) );

/* ThreadX aliases */
PROVIDE( __RAM_segment_used_end__ = link_stack_end );
PROVIDE( __tx_free_memory_start = link_stack_end );
PROVIDE( __tx_vectors = link_interrupt_vectors_location );

PROVIDE( VTOR_Length = interrupt_vectors_length );

ASSERT( link_ram_interrupt_vectors_location == ORIGIN( SRAM ), "RAM Interrupt table should be at start of RAM" );
ASSERT( ( link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location ) == VTOR_Length, "Expected RAM VTOR table to be same length as VTOR Flash" );

ASSERT ( link_module_info_start < link_module_info_end, "module info not linked" );
ASSERT ( link_module_info_suffix_start < link_module_info_suffix_end, "module info suffix not linked" );
ASSERT ( link_module_info_crc_start < link_module_info_crc_end, "module info crc not linked" );

ASSERT ( _monolithic_firmware_module_info_flash_start == link_module_info_start, "module info start not where expected" );
ASSERT ( _monolithic_firmware_module_info_flash_end == link_module_info_end, "module info end not where expected" );

ASSERT ( link_constructors_location % 4 == 0, "the constructor array should be at a 4-byte aligned address." );

/*ASSERT ( link_early_startup_begin != link_early_startup_end, "no early startup functions linked" );*/

ASSERT( ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end, "Static RAM region doesn't fit into RAM" );
