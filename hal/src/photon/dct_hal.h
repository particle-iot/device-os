/*
 * File:   dct_hal.h
 * Author: mat
 *
 * Created on 11 November 2014, 09:32
 */

#ifndef DCT_HAL_H
#define DCT_HAL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "platform_dct.h"
#include "platform_system_flags.h"
#include "dct.h"

typedef struct complete_dct {
    // 7548 bytes
    platform_dct_data_t system;
    application_dct_t application;
    // 52 bytes
    platform_dct_data2_t system2;
} complete_dct_t;

STATIC_ASSERT(offset_application_dct, (offsetof(complete_dct_t, application)==7548) );

STATIC_ASSERT(size_complete_dct, (sizeof(complete_dct_t)<=16384));

#ifdef  __cplusplus
}
#endif

#endif  /* DCT_HAL_H */

