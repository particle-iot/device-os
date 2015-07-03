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

/* Private define -----------------------------------------------------------*/

#ifndef NULL
#define NULL ((void *)0)
#endif

#define BOOTLOADER_MAGIC_NUMBER     0x4d435242

#ifndef PRIVATE_KEY_SIZE
#define PRIVATE_KEY_SIZE  (2*1024)
#endif

#ifndef CERTIFICATE_SIZE
#define CERTIFICATE_SIZE  (4*1024)
#endif

// #ifndef CONFIG_AP_LIST_SIZE
// #define CONFIG_AP_LIST_SIZE   (5)
// #endif

// #ifndef HOSTNAME_SIZE
// #define HOSTNAME_SIZE       (32)
// #endif

#ifndef COOEE_KEY_SIZE
#define COOEE_KEY_SIZE   (16)
#endif

#ifndef SECURITY_KEY_SIZE
#define SECURITY_KEY_SIZE    (64)
#endif


#define CONFIG_VALIDITY_VALUE        0xCA1BDF58

#define DCT_FR_APP_INDEX            ( 0 )
#define DCT_DCT_IMAGE_INDEX         ( 1 )
#define DCT_OTA_APP_INDEX           ( 2 )
#define DCT_FILESYSTEM_IMAGE_INDEX  ( 3 )
#define DCT_WIFI_FIRMWARE_INDEX     ( 4 )
#define DCT_APP0_INDEX              ( 5 )
#define DCT_APP1_INDEX              ( 6 )
#define DCT_APP2_INDEX              ( 7 )

#define DCT_MAX_APP_COUNT      ( 8 )

#define DCT_APP_LOCATION_OF(APP_INDEX) (uint32_t )((uint8_t *)&((platform_dct_header_t *)0)->apps_locations + sizeof(image_location_t) * ( APP_INDEX ))
//#define DCT_APP_LOCATION_OF(APP_INDEX) (uint32_t )(OFFSETOF(platform_dct_header_t, apps_locations) + sizeof(image_location_t) * ( APP_INDEX ))

#define ERASE_VOLTAGE_RANGE ( VoltageRange_3 )
#define FLASH_WRITE_FUNC    ( FLASH_ProgramWord )
#define FLASH_WRITE_SIZE    ( 4 )

/* Private typedef ----------------------------------------------------------*/

typedef unsigned int	uint;
typedef unsigned int	uintptr;
typedef uint32_t 		flash_write_t;

typedef struct
{
        uint32_t location;
        uint32_t size;
} fixed_location_t;

typedef enum
{
    NONE,
    INTERNAL,
    EXTERNAL_FIXED_LOCATION,
    EXTERNAL_FILESYSTEM_FILE,
} image_location_id_t;

typedef struct
{
    image_location_id_t id;
    union
    {
        fixed_location_t internal_fixed;
        fixed_location_t external_fixed;
        char             filesystem_filename[32];
    } detail;
} image_location_t;

typedef struct
{
        image_location_t source;
        image_location_t destination;
        char load_once;
        char valid;
} load_details_t;

typedef struct
{
        load_details_t load_details;

        uint32_t entry_point;
} boot_detail_t;

/* DCT section */
typedef enum
{
    DCT_APP_SECTION,
    DCT_SECURITY_SECTION,
    DCT_MFG_INFO_SECTION,
    DCT_WIFI_CONFIG_SECTION,
    DCT_INTERNAL_SECTION, /* Do not use in apps */
    DCT_ETHERNET_CONFIG_SECTION,
    DCT_NETWORK_CONFIG_SECTION,
// #ifdef WICED_DCT_INCLUDE_BT_CONFIG
//     DCT_BT_CONFIG_SECTION
// #endif
} dct_section_t;

/* DCT header */
typedef struct
{
        unsigned long full_size;
        unsigned long used_size;
        char write_incomplete;
        char is_current_dct;
        char app_valid;
        char mfg_info_programmed;
        unsigned long magic_number;
        boot_detail_t boot_detail;
        image_location_t apps_locations[ DCT_MAX_APP_COUNT ];
        void (*load_app_func)( void ); /* WARNING: TEMPORARY */
#ifdef  DCT_HEADER_ALIGN_SIZE
        uint8_t padding[DCT_HEADER_ALIGN_SIZE - sizeof(struct platform_dct_header_s)];
#endif
} platform_dct_header_t;

typedef struct
{
    char manufacturer[ 32 ];
    char product_name[ 32 ];
    char BOM_name[24];
    char BOM_rev[8];
    char serial_number[20];
    char manufacture_date_time[20];
    char manufacture_location[12];
    char bootloader_version[8];
} platform_dct_mfg_info_t;

typedef struct
{
    char    private_key[ PRIVATE_KEY_SIZE ];
    char    certificate[ CERTIFICATE_SIZE ];
    uint8_t cooee_key  [ COOEE_KEY_SIZE ];
} platform_dct_security_t;

// typedef struct
// {
//     wiced_bool_t              device_configured;
//     wiced_config_ap_entry_t   stored_ap_list[CONFIG_AP_LIST_SIZE];
//     wiced_config_soft_ap_t    soft_ap_settings;
//     wiced_config_soft_ap_t    config_ap_settings;
//     wiced_country_code_t      country_code;
//     wiced_mac_t               mac_address;
//     uint8_t                   padding[2];  /* to ensure 32bit aligned size */
// } platform_dct_wifi_config_t;

typedef struct
{
    platform_dct_header_t          dct_header;
    platform_dct_mfg_info_t        mfg_info;
    platform_dct_security_t        security_credentials;
//    platform_dct_wifi_config_t     wifi_config;
//    platform_dct_ethernet_config_t ethernet_config;
//    platform_dct_network_config_t  network_config;
// #ifdef WICED_DCT_INCLUDE_BT_CONFIG
//     platform_dct_bt_config_t       bt_config;
// #endif
} platform_dct_data_t;

/* Private macro ------------------------------------------------------------*/

/**
 * Macros
 */
#define DCT1_START_ADDR  ((uint32_t)&dct1_start_addr_loc)
#define DCT1_SIZE        ((uint32_t)&dct1_size_loc)
#define DCT2_START_ADDR  ((uint32_t)&dct2_start_addr_loc)
#define DCT2_SIZE        ((uint32_t)&dct2_size_loc)

#define PLATFORM_DCT_COPY1_START_SECTOR      ( FLASH_Sector_1  )
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( DCT1_START_ADDR )
#define PLATFORM_DCT_COPY1_END_SECTOR        ( FLASH_Sector_1 )
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE )
#define PLATFORM_DCT_COPY2_START_SECTOR      ( FLASH_Sector_2  )
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( DCT2_START_ADDR )
#define PLATFORM_DCT_COPY2_END_SECTOR        ( FLASH_Sector_2 )
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE )

#define ERASE_DCT_1()  platform_erase_flash(PLATFORM_DCT_COPY1_START_SECTOR, PLATFORM_DCT_COPY1_END_SECTOR)
#define ERASE_DCT_2()  platform_erase_flash(PLATFORM_DCT_COPY2_START_SECTOR, PLATFORM_DCT_COPY2_END_SECTOR)

#ifndef OFFSETOF
#define	OFFSETOF(type, member)	((uint)(uintptr)&((type *)0)->member)
#endif /* OFFSETOF */

/* Private variables --------------------------------------------------------*/

static const uint32_t DCT_section_offsets[] =
{
    [DCT_APP_SECTION]         = sizeof( platform_dct_data_t ),
    [DCT_SECURITY_SECTION]    = OFFSETOF( platform_dct_data_t, security_credentials ),
    [DCT_MFG_INFO_SECTION]    = OFFSETOF( platform_dct_data_t, mfg_info ),
//    [DCT_WIFI_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, wifi_config ),
//    [DCT_ETHERNET_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ethernet_config ),
//    [DCT_NETWORK_CONFIG_SECTION]  = OFFSETOF( platform_dct_data_t, network_config ),
// #ifdef WICED_DCT_INCLUDE_BT_CONFIG
//     [DCT_BT_CONFIG_SECTION]   = OFFSETOF( platform_dct_data_t, bt_config ),
// #endif
    [DCT_INTERNAL_SECTION]    = 0,
};

/* Extern variables ---------------------------------------------------------*/

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;

/* Private function prototypes ----------------------------------------------*/

void* dct_read_app_data			( uint32_t offset );
int dct_write_app_data			( const void* data, uint32_t offset, uint32_t size );
int platform_erase_flash		( uint16_t start_sector, uint16_t end_sector );
int platform_write_flash_chunk 	( uint32_t address, const void* data, uint32_t size );
static int wiced_write_dct      ( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void (*func)(void) );
static char requires_erase      ( platform_dct_header_t* p_dct );
static void wiced_erase_dct     ( platform_dct_header_t* p_dct );

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

static char requires_erase(platform_dct_header_t* p_dct)
{
    unsigned* p = (unsigned*)p_dct;
    for (unsigned i=0; i<4096; i++) {
        if ((p[i])!=0xFFFFFFFFU) 
            return 1; 
    }
    return 0;
}

static void wiced_erase_dct( platform_dct_header_t* p_dct )
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

static int wiced_write_dct( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void (*func)(void) )
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

void* dct_read_app_data(uint32_t offset) {
    return ((char*)wiced_dct_get_current_address(0))+offset;
}

int dct_write_app_data( const void* data, uint32_t offset, uint32_t size ) {
    // first, let's try just writing the data
    void* dct_start = dct_read_app_data(offset);
    if (platform_write_flash_chunk((uint32_t)dct_start, data, size) != 0)
        return wiced_write_dct(DCT_section_offsets[0]+offset, data, size, 1, NULL);
    else
        return 0;
}
