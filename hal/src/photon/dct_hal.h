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

#include "platform_dct.h"
#include "platform_system_flags.h"
#include "dct.h"

typedef struct complete_dct {
    platform_dct_data_t system;
    uint8_t reserved[1024];   // just in case WICED decide to add more things in future, this won't invalidate existing data.
    application_dct_t application;
} complete_dct_t;

STATIC_ASSERT(offset_application_dct, (offsetof(complete_dct_t, application)==7548+1024) );

STATIC_ASSERT(size_complete_dct, (sizeof(complete_dct_t)<16384));





#ifdef	__cplusplus
}
#endif

#endif	/* DCT_HAL_H */

