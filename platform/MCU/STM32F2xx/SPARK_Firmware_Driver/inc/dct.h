/* 
 * File:   dct.h
 * Author: mat
 *
 * Created on 12 November 2014, 04:41
 */

#ifndef DCT_H
#define	DCT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>    
#include "platform_system_flags.h"  
#include "static_assert.h"
#include "stddef.h"     // for offsetof in C

/**
 * Custom extensions to the DCT data stored
 */    
typedef struct application_dct {    
    platform_system_flags_t system_flags;
    uint16_t version;
    uint8_t device_private_key[1216];   // sufficient for 2048 bits
    uint8_t device_public_key[384];     // sufficient for 2048 bits
    uint8_t server_address[128];
    uint8_t claim_code[64];             // claim code. no terminating null.
    uint8_t ssid_prefix[26];            // SSID prefix (25 chars max). First byte is length.
    uint8_t device_id[6];               // 6 suffix characters (not null terminated))
    uint8_t claimed[1];                 // 0,0xFF, not claimed. 1 claimed. 
    uint8_t reserved1[223];             
    uint8_t server_public_key[768];     // 4096 bits
    uint8_t reserved2[1280+128];    
    // safe to add more data here
    uint8_t end[0];
} application_dct_t;

#define DCT_SYSTEM_FLAGS_OFFSET  (offsetof(application_dct_t, system_flags)) 
#define DCT_DEVICE_PRIVATE_KEY_OFFSET (offsetof(application_dct_t, device_private_key)) 
#define DCT_DEVICE_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, device_public_key)) 
#define DCT_SERVER_PUBLIC_KEY_OFFSET (offsetof(application_dct_t, server_public_key)) 
#define DCT_SERVER_ADDRESS_OFFSET (offsetof(application_dct_t, server_address)) 
#define DCT_CLAIM_CODE_OFFSET (offsetof(application_dct_t, claim_code)) 
#define DCT_SSID_PREFIX_OFFSET (offsetof(application_dct_t, ssid_prefix)) 
#define DCT_DEVICE_ID_OFFSET (offsetof(application_dct_t, device_id)) 
#define DCT_DEVICE_CLAIMED_OFFSET (offsetof(application_dct_t, claimed)) 

#define DCT_SYSTEM_FLAGS_SIZE  (sizeof(application_dct_t::system_flags)) 
#define DCT_DEVICE_PRIVATE_KEY_SIZE  (sizeof(application_dct_t::device_private_key)) 
#define DCT_DEVICE_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::device_public_key)) 
#define DCT_SERVER_PUBLIC_KEY_SIZE  (sizeof(application_dct_t::server_public_key)) 
#define DCT_SERVER_ADDRESS_SIZE  (sizeof(application_dct_t::server_address)) 
#define DCT_CLAIM_CODE_SIZE  (sizeof(application_dct_t::claim_code)) 
#define DCT_SSID_PREFIX_SIZE  (sizeof(application_dct_t::ssid_prefix)) 
#define DCT_DEVICE_ID_SIZE  (sizeof(application_dct_t::device_id)) 
#define DCT_DEVICE_CLAIMED_SIZE  (sizeof(application_dct_t::claimed)) 


#define STATIC_ASSERT_DCT_OFFSET(field, expected) STATIC_ASSERT( dct_##field, offsetof(application_dct_t, field)==expected)

/**
 * Assert offsets. These ensure that the layout in flash isn't inadvertently changed.
 */
STATIC_ASSERT_DCT_OFFSET(system_flags, 0);
STATIC_ASSERT_DCT_OFFSET(version, 32);
STATIC_ASSERT_DCT_OFFSET(device_private_key, 34);
STATIC_ASSERT_DCT_OFFSET(device_public_key, 1250 /*34+1216*/);
STATIC_ASSERT_DCT_OFFSET(server_address, 1634 /* 1250 + 384 */);
STATIC_ASSERT_DCT_OFFSET(claim_code, 1762 /* 1634 + 128 */);
STATIC_ASSERT_DCT_OFFSET(ssid_prefix, 1826 /* 1762 + 64 */);
STATIC_ASSERT_DCT_OFFSET(device_id, 1852 /* 1826 + 26 */);
STATIC_ASSERT_DCT_OFFSET(claimed, 1858 /* 1852 + 6 */ );
STATIC_ASSERT_DCT_OFFSET(reserved1, 1859 /* 1858 + 1 */);
STATIC_ASSERT_DCT_OFFSET(server_public_key, 2082 /* 1858 + 223 */);
STATIC_ASSERT_DCT_OFFSET(reserved2, 2850 /* 2082 + 768 */);
STATIC_ASSERT_DCT_OFFSET(end, 4258 /* 2850 + 1280 +128 */);


/**
 * Reads application data from the DCT area.
 * @param offset
 * @return 
 */

extern const void* dct_read_app_data(uint32_t offset);

extern int dct_write_app_data( const void* data, uint32_t offset, uint32_t size );
    

#ifdef	__cplusplus
}
#endif

#endif	/* DCT_H */

