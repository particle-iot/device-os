/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Code can be tested on a P1 module */

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

/* Includes -----------------------------------------------------------------*/
#include "stm32f2xx.h"
#include <string.h>
#include "watchdog_hal.h"
#include "dct_hal.h"

int platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t i;

    /* Unlock the STM32 Flash */
    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );

    HAL_Notify_WDT(); // platform_watchdog_kick( );

    for ( i = start_sector; i <= end_sector; i += 8 )
    {
        if ( FLASH_EraseSector( i, ERASE_VOLTAGE_RANGE ) != FLASH_COMPLETE )
        {
            /* Error occurred during erase. */
            /* TODO: Handle error */
            while ( 1 )
            {
            }
        }
        HAL_Notify_WDT();
    }

    FLASH_Lock( );

    return 0;	// PLATFORM_SUCCESS;
}

int platform_write_flash_chunk( uint32_t address, const void* data, uint32_t size )
{
    int result = 0;
    uint32_t write_address   = address;
    flash_write_t* data_ptr  = (flash_write_t*) data;
    flash_write_t* end_ptr   = (flash_write_t*) &((uint8_t*)data)[ size ];

    FLASH_Unlock( );

    /* Clear any error flags */
    FLASH_ClearFlag( FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );

    /* Write data to STM32 flash memory */
    while ( data_ptr < end_ptr )
    {
        FLASH_Status status;

        if ( ( ( ( (uint32_t) write_address ) & 0x03 ) == 0 ) && ( end_ptr - data_ptr >= FLASH_WRITE_SIZE ) )
        {
            int tries = 0;
            /* enough data available to write as the largest size allowed by supply voltage */
            while ( ( FLASH_COMPLETE != ( status = FLASH_WRITE_FUNC( write_address, *data_ptr ) ) ) && ( tries < 10 ) )
            {
                tries++ ;
            }
            if ( FLASH_COMPLETE != status )
            {
                /* TODO: Handle error properly */
                //wiced_assert("Error during write", 0 != 0 );
            }
            write_address += FLASH_WRITE_SIZE;
            data_ptr++ ;
        }
        else
        {
            int tries = 0;
            /* Limited data available - write in bytes */
            while ( ( FLASH_COMPLETE != ( status = FLASH_ProgramByte( write_address, *((uint8_t*)data_ptr) ) ) ) && ( tries < 10 ) )
            {
                tries++ ;
            }
            if ( FLASH_COMPLETE != status )
            {
                /* TODO: Handle error properly */
                //wiced_assert("Error during write", 0 != 0 );
            }
            write_address++ ;
            data_ptr = (flash_write_t*) ( (uint32_t) data_ptr + 1 );
        }

    }
    if ( memcmp( (void*)address, (void*)data, size) != 0 )
    {
        result = -1; // PLATFORM_ERROR
    }
    FLASH_Lock( );

    return result;
}

void* wiced_dct_get_current_address( dct_section_t section )
{
    static const platform_dct_header_t hdr =
    {
        .write_incomplete    = 0,
        .is_current_dct      = 1,
        .app_valid           = 1,
        .mfg_info_programmed = 0,
        .magic_number        = BOOTLOADER_MAGIC_NUMBER,
        .load_app_func       = NULL
    };

    platform_dct_header_t* dct1 = ((platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS);
    platform_dct_header_t* dct2 = ((platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS);

    if ( ( dct1->is_current_dct == 1 ) &&
         ( dct1->write_incomplete == 0 ) &&
         ( dct1->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return &((char*)dct1)[ DCT_section_offsets[section] ];
    }

    if ( ( dct2->is_current_dct == 1 ) &&
         ( dct2->write_incomplete == 0 ) &&
         ( dct2->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return &((char*)dct2)[ DCT_section_offsets[section] ];
    }

    /* No valid DCT! */
    /* Erase the first DCT and init it. */

    //wiced_assert("BOTH DCTs ARE INVALID!", 0 != 0 );

    ERASE_DCT_1();

    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS, &hdr, sizeof(hdr) );

    return &((char*)dct1)[ DCT_section_offsets[section] ];
}

char requires_erase(platform_dct_header_t* p_dct)
{
    unsigned* p = (unsigned*)p_dct;
    for (unsigned i=0; i<4096; i++) {
        if ((p[i])!=0xFFFFFFFFU)
            return 1;
    }
    return 0;
}

void wiced_erase_dct( platform_dct_header_t* p_dct )
{
    /* Erase the non-current DCT */
    if ( p_dct == ( (platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        if ( requires_erase(p_dct) )
            ERASE_DCT_1();
    }
    else if ( p_dct == ( (platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS ) )
    {
        if ( requires_erase(p_dct) )
            ERASE_DCT_2();
    }
}

int wiced_write_dct( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void (*func)(void) )
{
    platform_dct_header_t* new_dct;
    uint32_t               bytes_after_data;
    uint8_t*               new_app_data_addr;
    uint8_t*               curr_app_data_addr;
    platform_dct_header_t* curr_dct  = &((platform_dct_data_t*)wiced_dct_get_current_address( DCT_INTERNAL_SECTION ))->dct_header;
    platform_dct_header_t  hdr =
    {
        .write_incomplete = 0,
        .is_current_dct   = 1,
        .magic_number     = BOOTLOADER_MAGIC_NUMBER
    };

    /* Check if the data is too big to write */
    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        return -1;
    }

    /* Erase the non-current DCT */
    if ( curr_dct == ((platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS) )
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY2_START_ADDRESS;
    }
    else
    {
        new_dct = (platform_dct_header_t*)PLATFORM_DCT_COPY1_START_ADDRESS;
    }

    wiced_erase_dct( new_dct );

    data_start_offset -= sizeof( platform_dct_header_t );

    /* Write the mfg data and initial part of app data before provided data */
    if ( platform_write_flash_chunk( ((uint32_t) &new_dct[1]), &curr_dct[1], data_start_offset ) != 0 )
    {
        return -2;
    }

    /* Write the app data */
    new_app_data_addr  = (uint8_t*)new_dct  + sizeof(platform_dct_header_t) + data_start_offset;
    curr_app_data_addr = (uint8_t*)curr_dct + sizeof(platform_dct_header_t)+ data_start_offset;

    if ( platform_write_flash_chunk( (uint32_t) new_app_data_addr, data, data_length ) != 0 )
    {
        return -3;
    }

    bytes_after_data = ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) - (sizeof(platform_dct_header_t) + data_start_offset + data_length );

    if ( bytes_after_data != 0 )
    {
        new_app_data_addr += data_length;
        curr_app_data_addr += data_length;

        if (platform_write_flash_chunk( (uint32_t)new_app_data_addr, curr_app_data_addr, bytes_after_data ) != 0 )
        {
            /* Error writing app data */
            return -4;
        }
    }

    hdr.app_valid           = (char) (( app_valid == -1 )? curr_dct->app_valid : app_valid);
    hdr.load_app_func       = func;
    hdr.mfg_info_programmed = curr_dct->mfg_info_programmed;
    memcpy( &hdr.boot_detail, &curr_dct->boot_detail, sizeof(boot_detail_t));
    memcpy( hdr.apps_locations, curr_dct->apps_locations, sizeof( image_location_t )* DCT_MAX_APP_COUNT );

    /* Write the header data */
    if ( platform_write_flash_chunk( (uint32_t)new_dct, &hdr, sizeof(hdr) ) != 0 )
    {
        /* Error writing header data */
        wiced_erase_dct( new_dct );
        return -5;
    }

    /* Erase the non-current DCT */
    wiced_erase_dct( curr_dct );
    return 0;
}

const void* dct_read_app_data(uint32_t offset) {
    return ((char*)wiced_dct_get_current_address(0))+offset;
}

int dct_write_app_data( const void* data, uint32_t offset, uint32_t size ) {
    // first, let's try just writing the data
    const void* dct_start = dct_read_app_data(offset);
    if (platform_write_flash_chunk((uint32_t)dct_start, data, size) != 0)
        return wiced_write_dct(DCT_section_offsets[0]+offset, data, size, 1, NULL);
    else
        return 0;
}
