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
#ifndef INCLUDED_ELF_H_
#define INCLUDED_ELF_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
    struct
    {
        uint32_t magic;  /* should be 0x7f 'E' 'L' 'F' */
        uint8_t  elfclass;
        uint8_t  data;
        uint8_t  version;
        uint8_t  reserved[9];
    } ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t program_header_offset; /* from start of file */
    uint32_t section_header_table_offset; /* from start of file */
    uint32_t flags;
    uint16_t elf_header_size;
    uint16_t program_header_entry_size;
    uint16_t program_header_entry_count;
    uint16_t section_header_entry_size;
    uint16_t section_header_entry_count;
    uint16_t string_table_index; /* index in the section header table */
} elf_header_t;

typedef struct {
    uint32_t type;
    uint32_t data_offset;
    uint32_t virtual_address;
    uint32_t physical_address;
    uint32_t data_size_in_file;
    uint32_t data_size_in_memory;
    uint32_t flags;
    uint32_t alignment;
} elf_program_header_t;


typedef struct
{
    uint32_t name_index_in_string_table;
    uint32_t type;
    uint32_t flags;
    uint32_t dest_addr;
    uint32_t data_offset; /* from start of file */
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;
} elf_section_header_t;

#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_ELF_H_ */
