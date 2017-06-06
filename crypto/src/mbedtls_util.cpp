/**
 ******************************************************************************
 Copyright (c) 2013-2017 Particle Industries, Inc.  All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "mbedtls_util.h"
#include <cstring>

#include "rng_hal.h"

int mbedtls_default_rng(void*, unsigned char* data, size_t size) {
    while (size >= 4) {
        *((uint32_t*)data) = HAL_RNG_GetRandomNumber();
        data += 4;
        size -= 4;
    }
    while (size-- > 0) {
        *data++ = HAL_RNG_GetRandomNumber();
    }
    return 0;
}

#if defined(CRYPTO_PART1_SIZE_OPTIMIZATIONS) || PLATFORM_ID == 0
#define MBEDTLS_OVERRIDE_MD_LIST

#include "mbedtls/version.h"
#include "mbedtls/md.h"
#include "mbedtls/md_internal.h"

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_calloc    calloc
#define mbedtls_free       free
#endif


static mbedtls_callbacks_t s_callbacks = {0};

int mbedtls_set_callbacks(mbedtls_callbacks_t* callbacks, void* reserved)
{
    memcpy(&s_callbacks, callbacks, sizeof(mbedtls_callbacks_t));
    return 0;
}

extern "C"
{

// Size optimizations. Only enable SHA1

static const int supported_digests[] = {

#if defined(MBEDTLS_SHA1_C)
        MBEDTLS_MD_SHA1,
#endif

        MBEDTLS_MD_NONE
};

const int *mbedtls_md_list( void )
{
    if (s_callbacks.mbedtls_md_list) {
        return s_callbacks.mbedtls_md_list();
    }
    return( supported_digests );
}

const mbedtls_md_info_t *mbedtls_md_info_from_string( const char *md_name )
{
    if (s_callbacks.mbedtls_md_info_from_string) {
        return s_callbacks.mbedtls_md_info_from_string(md_name);
    }

    if( NULL == md_name )
        return( NULL );

    /* Get the appropriate digest information */
#if defined(MBEDTLS_SHA1_C)
    if( !strcmp( "SHA1", md_name ) || !strcmp( "SHA", md_name ) )
        return mbedtls_md_info_from_type( MBEDTLS_MD_SHA1 );
#endif
    return( NULL );
}

const mbedtls_md_info_t *mbedtls_md_info_from_type( mbedtls_md_type_t md_type )
{
    if (s_callbacks.mbedtls_md_info_from_type) {
        return s_callbacks.mbedtls_md_info_from_type(md_type);
    }

    switch( md_type )
    {
#if defined(MBEDTLS_SHA1_C)
        case MBEDTLS_MD_SHA1:
            return( &mbedtls_sha1_info );
#endif
        default:
            return( NULL );
    }
}

}
#else

__attribute__((weak)) int mbedtls_set_callbacks(mbedtls_callbacks_t* callbacks, void* reserved)
{
    return 0;
}

#endif // defined(CRYPTO_PART1_SIZE_OPTIMIZATIONS) || PLATFORM_ID == 0

