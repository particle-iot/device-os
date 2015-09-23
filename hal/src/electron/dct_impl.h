
#pragma once

#include "dct.h"

/**
 * Reads application data from the DCT area.
 * @param offset
 * @return
 */

// extern const void* dct_read_app_data(uint32_t offset);

// extern int dct_write_app_data( const void* data, uint32_t offset, uint32_t size );

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
//    DCT_WIFI_CONFIG_SECTION,
    DCT_INTERNAL_SECTION /* Do not use in apps */
//    DCT_ETHERNET_CONFIG_SECTION,
//    DCT_NETWORK_CONFIG_SECTION,
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

/* we don't need platform_dct_wifi_config_t but it really throws off the DCT r/w offsets */
typedef struct
{
    uint8_t padding[844];
} platform_dct_wifi_config_t;

typedef struct
{
    platform_dct_header_t          dct_header;
    platform_dct_mfg_info_t        mfg_info;
    platform_dct_security_t        security_credentials;
/* we don't need wifi_config but it really throws off the DCT r/w offsets */
    platform_dct_wifi_config_t     wifi_config_padding;
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
    [DCT_INTERNAL_SECTION]    = 0
};

/* Extern variables ---------------------------------------------------------*/

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;

/* Private function prototypes ----------------------------------------------*/

const void* dct_read_app_data			( uint32_t offset );
int dct_write_app_data			( const void* data, uint32_t offset, uint32_t size );
int platform_erase_flash		( uint16_t start_sector, uint16_t end_sector );
int platform_write_flash_chunk 	( uint32_t address, const void* data, uint32_t size );
int wiced_write_dct             ( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void (*func)(void) );
char requires_erase             ( platform_dct_header_t* p_dct );
void wiced_erase_dct            ( platform_dct_header_t* p_dct );

typedef struct complete_dct {
    platform_dct_data_t system;
    uint8_t reserved[1024];   // just in case WICED decide to add more things in future, this won't invalidate existing data.
    application_dct_t application;
} complete_dct_t;

/* Helper macro to look at the size of complex structures */
#define SHOW_ME_THE_SIZEOF(value_of_interest) template<int s> struct THE_SIZE_IS_; \
                                              struct _SO_I_HOPE_THATS_USEFUL { int a,b; }; \
                                              THE_SIZE_IS_<(value_of_interest)> _SO_I_HOPE_THATS_USEFUL;

//SHOW_ME_THE_SIZEOF(sizeof(complete_dct_t));
//SHOW_ME_THE_SIZEOF(sizeof(platform_dct_wifi_config_t));

/* before adding platform_dct_wifi_config_t back to platform_dct_data_t, this assert was passing 844 bytes shy of expected */
// STATIC_ASSERT(offset_application_dct, (offsetof(complete_dct_t, application)==6704+1024 /*new is 7728*/ /*should be 7548+1024*/) );

