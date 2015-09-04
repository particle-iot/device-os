/* 
 * File:   dct_hal.h
 * Author: mat
 *
 * Created on 11 November 2014, 09:32
 */

#ifndef DCT_HAL_H
#define	DCT_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "platform_system_flags.h"

#include <stdint.h>    
#include "platform_system_flags.h"  
#include "platform_flash_modules.h"
#include "static_assert.h"
#include "stddef.h"     // for offsetof in C

#define MAX_MODULES_SLOT    5 //Max modules
#define FAC_RESET_SLOT      0 //Factory reset module index
#define GEN_START_SLOT      1 //Generic module start index

/**
 * Custom extensions to the DCT data stored
 */    
typedef struct application_dct {    
    platform_system_flags_t system_flags;
    uint16_t version;
    uint8_t device_private_key[1216];   // sufficient for 2048 bits
    uint8_t device_public_key[384];     // sufficient for 2048 bits
    uint8_t unused_server_address[128];         // no longer used - write the server address to offset 
                                        // 0x180/384 in the server public key to emulate
    uint8_t claim_code[63];             // claim code. no terminating null.
    uint8_t claimed[1];                 // 0,0xFF, not claimed. 1 claimed.     
    uint8_t ssid_prefix[26];            // SSID prefix (25 chars max). First byte is length.
    uint8_t device_id[6];               // 6 suffix characters (not null terminated))
    uint8_t version_string[32];         // version string including date
    uint8_t reserved1[192];  
    uint8_t server_public_key[768];     // 4096 bits
    uint8_t padding[2];                 // align to 4 byte boundary
    platform_flash_modules_t flash_modules[MAX_MODULES_SLOT];//100 bytes
    uint16_t product_store[12];
    uint8_t reserved2[1282];    
    // safe to add more data here or use up some of the reserved space to keep the end where it is
    uint8_t end[0];
} application_dct_t;

#define DCT_SYSTEM_FLAGS_OFFSET  (offsetof(application_dct_t, system_flags)) 
#define DCT_DEVICE_PRIVATE_KEY_OFFSET (offsetof(application_dct_t, device_private_key)) 
#define DCT_DEVICE_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, device_public_key)) 
#define DCT_SERVER_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, server_public_key)) 
#define DCT_SERVER_ADDRESS_OFFSET ((DCT_SERVER_PUBLIC_KEY_OFFSET)+384)
#define DCT_CLAIM_CODE_OFFSET (offsetof(application_dct_t, claim_code)) 
#define DCT_SSID_PREFIX_OFFSET (offsetof(application_dct_t, ssid_prefix)) 
#define DCT_DEVICE_ID_OFFSET (offsetof(application_dct_t, device_id)) 
#define DCT_DEVICE_CLAIMED_OFFSET (offsetof(application_dct_t, claimed)) 
#define DCT_FLASH_MODULES_OFFSET (offsetof(application_dct_t, flash_modules))
#define DCT_PRODUCT_STORE_OFFSET (offsetof(application_dct_t, product_store))

#define DCT_SYSTEM_FLAGS_SIZE  (sizeof(application_dct_t::system_flags)) 
#define DCT_DEVICE_PRIVATE_KEY_SIZE  (sizeof(application_dct_t::device_private_key)) 
#define DCT_DEVICE_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::device_public_key)) 
#define DCT_SERVER_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::server_public_key)) 
#define DCT_SERVER_ADDRESS_SIZE  (128)
#define DCT_CLAIM_CODE_SIZE  (sizeof(application_dct_t::claim_code)) 
#define DCT_SSID_PREFIX_SIZE  (sizeof(application_dct_t::ssid_prefix)) 
#define DCT_DEVICE_ID_SIZE  (sizeof(application_dct_t::device_id)) 
#define DCT_DEVICE_CLAIMED_SIZE  (sizeof(application_dct_t::claimed)) 
#define DCT_FLASH_MODULES_SIZE  (sizeof(application_dct_t::flash_modules))
#define DCT_PRODUCT_STORE_SIZE  (sizeof(application_dct_t::product_store))

#define STATIC_ASSERT_DCT_OFFSET(field, expected) STATIC_ASSERT( dct_##field, offsetof(application_dct_t, field)==expected)
#define STATIC_ASSERT_FLAGS_OFFSET(field, expected) STATIC_ASSERT( dct_sysflag_##field, offsetof(platform_system_flags_t, field)==expected)

/**
 * Assert offsets. These ensure that the layout in flash isn't inadvertently changed.
 */
STATIC_ASSERT_DCT_OFFSET(system_flags, 0);
STATIC_ASSERT_DCT_OFFSET(version, 32);
STATIC_ASSERT_DCT_OFFSET(device_private_key, 34);
STATIC_ASSERT_DCT_OFFSET(device_public_key, 1250 /*34+1216*/);
STATIC_ASSERT_DCT_OFFSET(unused_server_address, 1634 /* 1250 + 384 */);
STATIC_ASSERT_DCT_OFFSET(claim_code, 1762 /* 1634 + 128 */);
STATIC_ASSERT_DCT_OFFSET(claimed, 1825 /* 1762 + 63 */ );
STATIC_ASSERT_DCT_OFFSET(ssid_prefix, 1826 /* 1825 + 1 */);
STATIC_ASSERT_DCT_OFFSET(device_id, 1852 /* 1826 + 26 */);
STATIC_ASSERT_DCT_OFFSET(version_string, 1858 /* 1852 + 6 */);
STATIC_ASSERT_DCT_OFFSET(reserved1, 1890 /* 1868 + 32 */);
STATIC_ASSERT_DCT_OFFSET(server_public_key, 2082 /* 1890 + 192 */);
STATIC_ASSERT_DCT_OFFSET(padding, 2850 /* 2082 + 768 */);
STATIC_ASSERT_DCT_OFFSET(flash_modules, 2852 /* 2850 + 2 */);
STATIC_ASSERT_DCT_OFFSET(product_store, 2952 /* 2852 + 100 */);
STATIC_ASSERT_DCT_OFFSET(reserved2, 2976 /* 2952 + 24 */);
STATIC_ASSERT_DCT_OFFSET(end, 4258 /* 2952 + 1282 */);

STATIC_ASSERT_FLAGS_OFFSET(Bootloader_Version_SysFlag, 4);
STATIC_ASSERT_FLAGS_OFFSET(NVMEM_SPARK_Reset_SysFlag, 6);
STATIC_ASSERT_FLAGS_OFFSET(FLASH_OTA_Update_SysFlag, 8);
STATIC_ASSERT_FLAGS_OFFSET(OTA_FLASHED_Status_SysFlag, 10);
STATIC_ASSERT_FLAGS_OFFSET(Factory_Reset_SysFlag, 12);
STATIC_ASSERT_FLAGS_OFFSET(IWDG_Enable_SysFlag, 14);
STATIC_ASSERT_FLAGS_OFFSET(dfu_on_no_firmware, 16);
STATIC_ASSERT_FLAGS_OFFSET(Factory_Reset_Done_SysFlag, 17);
STATIC_ASSERT_FLAGS_OFFSET(StartupMode_SysFlag, 18);
STATIC_ASSERT_FLAGS_OFFSET(FeaturesEnabled_SysFlag, 19);
STATIC_ASSERT_FLAGS_OFFSET(RCC_CSR_SysFlag, 20);
STATIC_ASSERT_FLAGS_OFFSET(reserved, 24);
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
    [DCT_INTERNAL_SECTION]    = 0
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
int wiced_write_dct             ( uint32_t data_start_offset, const void* data, uint32_t data_length, int8_t app_valid, void (*func)(void) );
char requires_erase             ( platform_dct_header_t* p_dct );
void wiced_erase_dct            ( platform_dct_header_t* p_dct );
    
typedef struct complete_dct {
    platform_dct_data_t system;
    uint8_t reserved[1024];   // just in case WICED decide to add more things in future, this won't invalidate existing data.
    application_dct_t application;     
} complete_dct_t;

// STATIC_ASSERT(offset_application_dct, (offsetof(complete_dct_t, application)==7548+1024) );

// STATIC_ASSERT(size_complete_dct, (sizeof(complete_dct_t)<16384));
        




#ifdef	__cplusplus
}
#endif

#endif	/* DCT_HAL_H */

