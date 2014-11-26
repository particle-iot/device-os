/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_SPI_FLASH_API_H
#define INCLUDED_SPI_FLASH_API_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>



typedef enum
{
    SFLASH_WRITE_NOT_ALLOWED = 0,
    SFLASH_WRITE_ALLOWED     = 1,

} sflash_write_allowed_t;

typedef /*@abstract@*/ /*@immutable@*/ struct
{
    uint32_t device_id;
    void * platform_peripheral;
    sflash_write_allowed_t write_allowed;
} sflash_handle_t;


int init_sflash         ( /*@out@*/ sflash_handle_t* const handle, /*@shared@*/ void* peripheral_id, sflash_write_allowed_t write_allowed_in );
int sflash_read         ( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/  /*@dependent@*/ void* const data_addr, unsigned int size );
int sflash_write        ( const sflash_handle_t* const handle, unsigned long device_address,  /*@observer@*/ const void* const data_addr, unsigned int size );
int sflash_chip_erase   ( const sflash_handle_t* const handle );
int sflash_sector_erase ( const sflash_handle_t* const handle, unsigned long device_address );
int sflash_get_size     ( const sflash_handle_t* const handle, /*@out@*/ unsigned long* size );

#ifdef __cplusplus
}
#endif

#endif /* INCLUDED_SPI_FLASH_API_H */
