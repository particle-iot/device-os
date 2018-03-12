/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
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
