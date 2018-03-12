/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef _INCLUDED_CRC_H_
#define _INCLUDED_CRC_H_

#include <stdint.h>
#include "platform_toolchain.h"

#ifdef __cplusplus
extern "C" {
#endif

/* crc defines */
#define CRC8_INIT_VALUE  0xff       /* Initial CRC8 checksum value */
#define CRC16_INIT_VALUE 0xffff     /* Initial CRC16 checksum value */
#define CRC32_INIT_VALUE 0xffffffff /* Initial CRC32 checksum value */

#ifdef NO_CRC_FUNCTIONS
#define WICED_CRC_FUNCTION_NAME(function) wiced_##function
#else
#define WICED_CRC_FUNCTION_NAME(function) function
#endif

extern uint8_t WICED_CRC_FUNCTION_NAME( crc8 )
    (
        uint8_t* pdata,       /* pointer to array of data to process */
        unsigned int nbytes,  /* number of input data bytes to process */
        uint8_t crc           /* either CRC8_INIT_VALUE or previous return value */
    );

extern uint16_t WICED_CRC_FUNCTION_NAME( crc16 )
    (
        uint8_t* pdata,       /* pointer to array of data to process */
        unsigned int nbytes,  /* number of input data bytes to process */
        uint16_t crc          /* either CRC16_INIT_VALUE or previous return value */
    );

extern uint32_t WICED_CRC_FUNCTION_NAME( crc32 )
    (
        uint8_t* pdata,       /* pointer to array of data to process */
        unsigned int nbytes,  /* number of input data bytes to process */
        uint32_t crc          /* either CRC32_INIT_VALUE or previous return value */
    );

#ifdef NO_CRC_FUNCTIONS
static inline ALWAYS_INLINE uint8_t crc8( uint8_t* pdata, unsigned int nbytes, uint8_t crc )
{
    return wiced_crc8( pdata, nbytes, crc );
}

static inline ALWAYS_INLINE uint16_t crc16( uint8_t* pdata, unsigned int nbytes, uint16_t crc )
{
    return wiced_crc16( pdata, nbytes, crc );
}

static inline ALWAYS_INLINE uint32_t crc32( uint8_t* pdata, unsigned int nbytes, uint32_t crc )
{
    return wiced_crc32( pdata, nbytes, crc );
}
#endif

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef _INCLUDED_CRC_H_ */
