#ifndef MBEDTLS_COMMUNICATION_OPTIMIZE_INC
#define MBEDTLS_COMMUNICATION_OPTIMIZE_INC
#else
#error This file should only be included once
#endif

#include <string.h>
#include "mbedtls_util.h"

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

mbedtls_callbacks_t* mbedtls_get_callbacks(void* reserved)
{
    return &s_callbacks;
}

extern "C"
{

// Size optimizations. Only enable SHA1

static const int supported_digests[] = {

#if defined(MBEDTLS_SHA1_C)
        MBEDTLS_MD_SHA1,
#endif
#if defined(MBEDTLS_SHA256_C) && defined(PLATFORM_ID) && PLATFORM_ID == 10
        MBEDTLS_MD_SHA256,
        MBEDTLS_MD_SHA224,
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
#if defined(MBEDTLS_SHA256_C) && defined(PLATFORM_ID) && PLATFORM_ID == 10
    if( !strcmp( "SHA224", md_name ) )
        return mbedtls_md_info_from_type( MBEDTLS_MD_SHA224 );
    if( !strcmp( "SHA256", md_name ) )
        return mbedtls_md_info_from_type( MBEDTLS_MD_SHA256 );
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
#if defined(MBEDTLS_SHA256_C) && defined(PLATFORM_ID) && PLATFORM_ID == 10
        case MBEDTLS_MD_SHA224:
            return( &mbedtls_sha224_info );
        case MBEDTLS_MD_SHA256:
            return( &mbedtls_sha256_info );
#endif
        default:
            return( NULL );
    }
}

}

#endif
