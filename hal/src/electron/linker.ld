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

MEMORY
{
    DCT1_FLASH (rx)  : ORIGIN = _dct1_flash_start,                     LENGTH = _dct1_flash_size
    DCT2_FLASH (rx)  : ORIGIN = _dct2_flash_start,                     LENGTH = _dct2_flash_size
    APP_FLASH  (rx)  : ORIGIN = _monolithic_firmware_flash_start,      LENGTH = _monolithic_firmware_flash_size
    SRAM       (rwx) : ORIGIN = _monolithic_firmware_static_ram_start, LENGTH = _monolithic_firmware_static_ram_size
    
    INCLUDE memory_backup_ram.ld
}

SECTIONS
{
    /* Module start */
    INCLUDE linker_module_start.ld
    
    /* Interrupt vector table */
    .vectors :
    {
        . = ALIGN(0x4);
        link_interrupt_vectors_location = .;
        KEEP (*(.interrupt_vector_table))
        link_interrupt_vectors_location_end = .;
        . = ALIGN(0x4);
    } > APP_FLASH  AT> APP_FLASH

    /* Module info */
    INCLUDE linker_module_info.ld

    .text :
    {
        . = ALIGN(0x4);

        /* Code */
        link_code_start = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        link_code_end = .;
        
        . = ALIGN(0x4);
        
        /* Read-only data (constants) */
        link_const_data_start = .;
        *(.rodata)
        *(.rodata*)
        *(.gnu.linkonce.r.*)
        link_const_data_end = .;

        . = ALIGN(0x4);

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

        . = ALIGN(0x4);
    } > APP_FLASH  AT> APP_FLASH

    /* C++ exception handlers */
    INCLUDE linker_arm_exception.ld

    /* Ram interrupt vector table - must be 512 byte-aligned.
     * See Cortext-M4 Devices Generic User Guide, Section 4.3.4, Vector Table Offset Register. */
    .ram_vectors :
    {
        link_static_ram_start = .;
        link_ram_interrupt_vectors_location = .;
        . = . + _platform_vector_table_flash_size;
        link_ram_interrupt_vectors_location_end = .;
    } > SRAM

    /* This section contains code that is run from RAM after being loaded from flash.
     * functions can be put in this section with the C attribute: __attribute__ ((section (".fast"))) */
    .fast :
    {
        link_run_from_ram_code_flash_location = LOADADDR( .fast ); /* This is the location in flash of the code */
        link_run_from_ram_code_ram_location = .;
        *(.fast .fast.* .text.fastcode)
        link_run_from_ram_code_ram_end = .;
    } > SRAM AT> APP_FLASH

    /* Non-zero initialised global variables */
    .data :
    {
        /* This is the location in flash of the initial values of global variables */
        link_global_data_initial_values = LOADADDR( .data ); 
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_start = .;
        *(.data*)
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .data secion */
        link_global_data_end = .;
    } > SRAM AT> APP_FLASH

    /* Zero initialised memory used for zero initialised variables */
    .bss :
    {
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_location = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(0x4);
        /* This is used by the startup in order to initialize the .bss secion */
        link_bss_end = .;
        link_static_ram_end = .;
    } > SRAM AT> SRAM
    
    /* Heap location */
    link_heap_location = link_bss_end;
    link_heap_location_end = link_bss_end + _heap_min_size;
    
    .stack : /* Contains the initial stack */
    {
        link_stack_location = ALIGN(., 4);
        *(.stack)
        . = ALIGN(MAX(link_stack_location + __STACKSIZE__ , .), 4);
        link_stack_end = .;
    } > SRAM AT> SRAM

    /* User backup ram data */
    INCLUDE linker_backup_ram_user.ld
    
    /* System backup ram data */
    INCLUDE linker_backup_ram_system.ld
    
    /* Module end */
    INCLUDE linker_module_end.ld

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

/* Stack , used by startup*/
PROVIDE (__stack_size__ = _monolithic_firmware_stack_size);
PROVIDE (__stack_start__ = _monolithic_firmware_stack_start);
PROVIDE (__stack_end__ = __stack_start__ + __stack_size__);

PROVIDE (dct1_start_addr_loc = ORIGIN(DCT1_FLASH));
PROVIDE (dct1_size_loc       = LENGTH(DCT1_FLASH));
PROVIDE (dct2_start_addr_loc = ORIGIN(DCT2_FLASH));
PROVIDE (dct2_size_loc       = LENGTH(DCT2_FLASH));

ASSERT (
    (ORIGIN(SRAM) >= _ram_start && ORIGIN(SRAM) + LENGTH(SRAM) <= _ram_end) &&
    (ORIGIN(APP_FLASH) >= _flash_start && ORIGIN(APP_FLASH) + LENGTH(APP_FLASH) <= _flash_end) &&
    (link_module_start == ORIGIN(APP_FLASH)) &&
    (link_module_start == _monolithic_firmware_flash_start) &&
    (link_static_ram_start == ORIGIN(SRAM)) &&
    (link_static_ram_start == _monolithic_firmware_static_ram_start),
    "Invalid memory boundary!"
);
ASSERT (
    (link_interrupt_vectors_location == _monolithic_firmware_vector_table_flash_start) &&
    (link_interrupt_vectors_location_end == _monolithic_firmware_vector_table_flash_end) &&
    (link_interrupt_vectors_location_end - link_interrupt_vectors_location == _platform_vector_table_flash_size),
    "Invalid interrupt vector table!"
);
ASSERT (
    (link_module_info_start < link_module_info_end) &&
    (link_module_info_suffix_start - link_module_info_static_start == 4 || link_module_info_suffix_start - link_module_info_static_start == 8) &&
    (link_module_info_suffix_start < link_module_info_suffix_end) &&
    (link_module_info_crc_start < link_module_info_crc_end) &&
    (link_module_info_start == _monolithic_firmware_module_info_flash_start) &&
    (link_module_info_end == _monolithic_firmware_module_info_flash_end),
    "Invalid module info!"
);
ASSERT (
    (link_constructors_location % 4 == 0),
    "constructor array should be aligned to a 4-bytes!"
);
ASSERT (
    (link_ram_interrupt_vectors_location == _monolithic_firmware_static_ram_start) &&
    (link_ram_interrupt_vectors_location_end - link_ram_interrupt_vectors_location == _platform_vector_table_flash_size) &&
    (link_ram_interrupt_vectors_location % 512 == 0),
    "Invalid RAM interrupt vector table!"
);
ASSERT (
    (link_static_ram_end - link_static_ram_start <= _monolithic_firmware_static_ram_size),
    "Insufficient room for .data and .bss sections!"
);
ASSERT (
    (link_heap_location_end <= _monolithic_firmware_stack_start),
    "Insufficient room for heap!"
);
ASSERT (
    (__stack_end__ % 8 == 0),
    "Stack should be 8-byte aligned!"
);
