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

/** @file
 *  Defines functions to set and get the current time
 */

#pragma once

#include "wiced_result.h"
#include "wiced_utilities.h"
#include "RTOS/wwd_rtos_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                    Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/** @cond !ADDTHIS*/
#define MILLISECONDS      (1)
#define SECONDS           (1000)
#define MINUTES           (60 * SECONDS)
#define HOURS             (60 * MINUTES)
#define DAYS              (24 * HOURS)
/** @endcond */

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/


typedef uint32_t  wiced_time_t;        /**< Time value in milliseconds */
typedef uint32_t  wiced_utc_time_t;    /**< UTC Time in seconds        */
typedef uint64_t  wiced_utc_time_ms_t; /**< UTC Time in milliseconds   */

/******************************************************
 *                    Structures
 ******************************************************/

/** ISO8601 Time Structure
 */
#pragma pack(1)

typedef struct
{
    char year[4];        /**< Year         */
    char dash1;          /**< Dash1        */
    char month[2];       /**< Month        */
    char dash2;          /**< Dash2        */
    char day[2];         /**< Day          */
    char T;              /**< T            */
    char hour[2];        /**< Hour         */
    char colon1;         /**< Colon1       */
    char minute[2];      /**< Minute       */
    char colon2;         /**< Colon2       */
    char second[2];      /**< Second       */
    char decimal;        /**< Decimal      */
    char sub_second[6];  /**< Sub-second   */
    char Z;              /**< UTC timezone */
} wiced_iso8601_time_t;

#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/** @addtogroup time       Time management functions
 *
 * Functions to get and set the real-time-clock time.
 *
 *
 *  @{
 */
/*****************************************************************************/


/** Get the current system tick time in milliseconds
 *
 * @note The time will roll over every 49.7 days
 *
 * @param[out] time : A pointer to the variable which will receive the time value
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_time_get_time( wiced_time_t* time );


/** Set the current system tick time in milliseconds
 *
 * @param[in] time : the time value to set
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_time_set_time( const wiced_time_t* time );


/** Get the current UTC time in seconds
 *
 * This will only be accurate if the time has previously been set by using @ref wiced_time_set_utc_time_ms
 *
 * @param[out] utc_time : A pointer to the variable which will receive the time value
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_time_get_utc_time( wiced_utc_time_t* utc_time );


/** Get the current UTC time in milliseconds
 *
 * This will only be accurate if the time has previously been set by using @ref wiced_time_set_utc_time_ms
 *
 * @param[out] utc_time_ms : A pointer to the variable which will receive the time value
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_time_get_utc_time_ms( wiced_utc_time_ms_t* utc_time_ms );


/** Set the current UTC time in milliseconds
 *
 * @param[in] utc_time_ms : the time value to set
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_time_set_utc_time_ms( const wiced_utc_time_ms_t* utc_time_ms );


/** Get the current UTC time in iso 8601 format e.g. "2012-07-02T17:12:34.567890Z"
 *
 * @note The time will roll over every 49.7 days
 *
 * @param[out] iso8601_time : A pointer to the structure variable that
 *                            will receive the time value
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_time_get_iso8601_time( wiced_iso8601_time_t* iso8601_time );

/** @} */

/**
 * This function will return the value of time read from the nanosecond clock.
 * @return : number of nanoseconds passed since the function wiced_init_nanosecond_clock or wiced_reset_nanosecond_clock was called
 */
uint64_t wiced_get_nanosecond_clock_value( void );


/**
 * This function will deinitialize the nanosecond clock.
 */
void wiced_deinit_nanosecond_clock( void );


/**
 * This function will reset the nanosecond clock.
 */
void wiced_reset_nanosecond_clock( void );


/**
 * This function will initialize the nanosecond clock.
*/
void wiced_init_nanosecond_clock( void );

#ifdef __cplusplus
} /*extern "C" */
#endif
