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
#pragma once

#include "wwd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef DEBUG
#define WICED_AUDIO_DEBUG_PRINT(x)
#define WICED_AUDIO_INFO_PRINT(x)     do { printf x; } while(0)
#define WICED_AUDIO_ERROR_PRINT(x)    do { printf x; } while(0)
#else
#define WICED_AUDIO_DEBUG_PRINT(x)
#define WICED_AUDIO_INFO_PRINT(x)
#define WICED_AUDIO_ERROR_PRINT(x)    do { printf x; } while(0)
#endif

#if defined(PLATFORM_L1_CACHE_SHIFT)
#define WICED_AUDIO_BUFFER_BYTE_ALIGNMENT_SHIFT     PLATFORM_L1_CACHE_SHIFT
#endif

#define WICED_AUDIO_BUFFER_RESERVED_BYTES   (4)

#if defined(WICED_AUDIO_BUFFER_BYTE_ALIGNMENT_SHIFT)
#define WICED_AUDIO_BUFFER_ALIGNMENT_BYTES  (1U << WICED_AUDIO_BUFFER_BYTE_ALIGNMENT_SHIFT)
#define WICED_AUDIO_BUFFER_ALIGNMENT_MASK   (WICED_AUDIO_BUFFER_ALIGNMENT_BYTES - 1)
#define WICED_AUDIO_BUFFER_ROUND_UP(a)      (((a) + WICED_AUDIO_BUFFER_ALIGNMENT_MASK) & ~(WICED_AUDIO_BUFFER_ALIGNMENT_MASK))
#define WICED_AUDIO_BUFFER_ROUND_DOWN(a)    ((a) & ~(WICED_AUDIO_BUFFER_ALIGNMENT_MASK))
#define WICED_AUDIO_BUFFER_PTR_ROUND_UP(p)  ((void*)WICED_AUDIO_BUFFER_ROUND_UP((uint32_t)(p)))
#define WICED_AUDIO_BUFFER_OFFSET(a)        ((uint32_t)(a) & (WICED_AUDIO_BUFFER_ALIGNMENT_MASK) )

#else
#define WICED_AUDIO_BUFFER_ALIGNMENT_BYTES  (0)
#define WICED_AUDIO_BUFFER_ALIGNMENT_MASK   (0)
#define WICED_AUDIO_BUFFER_ROUND_UP(a)      (a)
#define WICED_AUDIO_BUFFER_ROUND_DOWN(a)    (a)
#define WICED_AUDIO_BUFFER_PTR_ROUND_UP(p)  (p)
#define WICED_AUDIO_BUFFER_OFFSET(a)        (0)
#endif

#if defined(WICED_AUDIO_BUFFER_BYTE_ALIGNMENT_SHIFT)

#ifndef WICED_AUDIO_BUFFER_PERIOD_RESERVED_BYTES
#define WICED_AUDIO_BUFFER_PERIOD_RESERVED_BYTES (0)
#endif

#define WICED_AUDIO_BUFFER_HEADER_SIZEOF \
    WICED_AUDIO_BUFFER_ROUND_UP(sizeof(wiced_audio_buffer_header_t))

#define WICED_AUDIO_BUFFER_PERIOD_SIZEOF(_BYTES_PER_PERIOD_) \
    WICED_AUDIO_BUFFER_ROUND_UP((_BYTES_PER_PERIOD_) + WICED_AUDIO_BUFFER_RESERVED_BYTES)

#define WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(_BYTES_PER_PERIOD_) \
    (WICED_AUDIO_BUFFER_HEADER_SIZEOF + WICED_AUDIO_BUFFER_PERIOD_SIZEOF(_BYTES_PER_PERIOD_))

#define WICED_AUDIO_BUFFER_ARRAY_SIZEOF(_PERIODS_, _BYTES_PER_PERIOD_) \
    (WICED_AUDIO_BUFFER_ARRAY_ELEMENT_SIZEOF(_BYTES_PER_PERIOD_) * (_PERIODS_))

#define WICED_AUDIO_BUFFER_ARRAY_DIM_SIZEOF(_PERIODS_, _BYTES_PER_PERIOD_) \
    (WICED_AUDIO_BUFFER_ALIGNMENT_MASK + WICED_AUDIO_BUFFER_ARRAY_SIZEOF(_PERIODS_, _BYTES_PER_PERIOD_))

#define WICED_AUDIO_BUFFER_ARRAY_PTR(_PTR_) \
    WICED_AUDIO_BUFFER_PTR_ROUND_UP(_PTR_)

#else

#define WICED_AUDIO_BUFFER_ARRAY_DIM_SIZEOF(_PERIODS_, _BYTES_PER_PERIOD_) \
    ((_BYTES_PER_PERIOD_) * (_PERIODS_))

#define WICED_AUDIO_BUFFER_ARRAY_PTR(_PTR_) (_PTR_)

#endif /* defined(WICED_AUDIO_BUFFER_BYTE_ALIGNMENT_SHIFT) */

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_NUM_AUDIO_DEVICES (3)

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * WICED audio output type
 */
typedef enum
{
    SPEAKERS,
    HEADPHONES,
    SPEAKERS_AND_HEADPHONES
} wiced_audio_output_t;

/**
 * WICED audio device channel
 */
typedef enum
{
    WICED_PLAY_CHANNEL,
    WICED_RECORD_CHANNEL
} wiced_audio_device_channel_t;

/**
 * WICED audio event type
 */
typedef enum
{
    WICED_AUDIO_PERIOD_ELAPSED = 0,
    WICED_AUDIO_UNDERRUN       = 1,
    WICED_AUDIO_HW_ERROR       = 2,
} wiced_audio_platform_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void (*wiced_audio_data_provider_t)( uint8_t** buffer, uint16_t* size, void* context);
typedef struct wiced_audio_session_t* wiced_audio_session_ref;

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * WICED audio configuration
 */
typedef struct
{
    uint32_t sample_rate;
    uint8_t  bits_per_sample;
    uint8_t  channels;
    uint8_t  volume;
} wiced_audio_config_t;

/**
 * WICED audio device interface
 * @note : Only used by device drivers
 */
typedef struct
{
    const char*    name;
    void*          audio_device_driver_specific;
    wiced_result_t (*audio_device_init)           ( void* device_data );
    wiced_result_t (*audio_device_deinit)         ( void* device_data );
    wiced_result_t (*audio_device_configure)      ( void* device_data, wiced_audio_config_t* config, uint32_t* mclk );
    wiced_result_t (*audio_device_start_streaming)( void* device_data );
    wiced_result_t (*audio_device_pause)          ( void* device_data );
    wiced_result_t (*audio_device_resume)         ( void* device_data );
    wiced_result_t (*audio_device_set_volume)     ( void* device_data, double decibles );
    wiced_result_t (*audio_device_set_treble)     ( void* device_data, uint8_t percentage );
    wiced_result_t (*audio_device_set_bass)       ( void* device_data, uint8_t percentage );
    wiced_result_t (*audio_device_get_volume_range) ( void* device_data, double* min_volume_decibels, double* max_volume_decibels);
} wiced_audio_device_interface_t;

/**
 * @note : Only used by device drivers
 */
typedef struct wiced_audio_buffer_header
{
    struct wiced_audio_buffer_header* next;
    uint8_t*                          data_start;
    uint8_t*                          data_end;
} wiced_audio_buffer_header_t;

/**
 * WICED audio device class
 */
typedef struct
{
    wiced_audio_device_interface_t audio_devices[MAX_NUM_AUDIO_DEVICES];
    int                            device_count;
} audio_device_class_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_audio_init                        ( const char* device_name, wiced_audio_session_ref* sh, uint16_t period_size );
wiced_result_t wiced_audio_configure                   ( wiced_audio_session_ref sh, wiced_audio_config_t* config );
wiced_result_t wiced_audio_create_buffer               ( wiced_audio_session_ref sh, uint16_t size, uint8_t* buffer_ptr, void*(*allocator)(uint16_t size));
wiced_result_t wiced_audio_set_volume                  ( wiced_audio_session_ref sh, double volume_in_db );
wiced_result_t wiced_audio_get_volume_range            ( wiced_audio_session_ref sh, double *min_volume_in_db, double *max_volume_in_db );
wiced_result_t wiced_audio_deinit                      ( wiced_audio_session_ref sh );
wiced_result_t wiced_audio_get_buffer                  ( wiced_audio_session_ref sh, uint8_t** ptr, uint16_t* size);
wiced_result_t wiced_audio_get_current_hw_pointer      ( wiced_audio_session_ref sh, uint32_t* hw_pointer);
wiced_result_t wiced_audio_start                       ( wiced_audio_session_ref sh );
wiced_result_t wiced_audio_stop                        ( wiced_audio_session_ref sh );
wiced_result_t wiced_audio_release_buffer              ( wiced_audio_session_ref sh, uint16_t size);
wiced_result_t wiced_audio_buffer_platform_event       ( wiced_audio_session_ref sh, wiced_audio_platform_event_t event);
uint16_t       wiced_audio_buffer_platform_get_periods ( wiced_audio_session_ref sh );
wiced_result_t wiced_audio_wait_buffer                 ( wiced_audio_session_ref sh, uint16_t size, uint32_t timeout );
wiced_result_t wiced_audio_get_current_buffer_weight   ( wiced_audio_session_ref sh, uint32_t* weight );

/* Returns the latency of the audio system in audio frames */
wiced_result_t wiced_audio_get_latency                 ( wiced_audio_session_ref sh, uint32_t* latency );
wiced_result_t wiced_register_audio_device             ( const char* name, wiced_audio_device_interface_t* interface );


#ifdef __cplusplus
} /* extern "C" */
#endif
