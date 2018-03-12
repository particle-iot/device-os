/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/** @file
 */
#pragma once

#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

#define AUDIO_DEVICE_ID_NONE                    0x0000  /* generic no device on platform */
/* input devices */
#define AUDIO_DEVICE_ID_AK4954_ADC_LINE         0x0001
#define AUDIO_DEVICE_ID_AK4961_ADC_LINE         0x0002
#define AUDIO_DEVICE_ID_AK4961_ADC_DIGITAL_MIC  0x0003
#define AUDIO_DEVICE_ID_SPDIF_ADC               0x0004

#define AUDIO_DEVICE_ID_OUTPUT_NONE             0x0100    /* used to differentiate between input and output */
/* output devices */
#define AUDIO_DEVICE_ID_AK4954_DAC_LINE         0x0101
#define AUDIO_DEVICE_ID_AK4961_DAC_LINE         0x0102
#define AUDIO_DEVICE_ID_WM8533_DAC_LINE         0x0103
#define AUDIO_DEVICE_ID_SPDIF_DAC               0x0104
    /* must be last */
#define     AUDIO_DEVICE_ID_MAX                 0x0800  /* General testing for a device # that is out of bounds */
/* audio device ID type */
typedef uint16_t    platform_audio_device_id_t;


enum platform_audio_direction_e
{
    PLATFORM_AUDIO_DEVICE_INPUT = 0,
    PLATFORM_AUDIO_DEVICE_OUTPUT,
};
typedef uint8_t platform_audio_direction_t;     /* direction (see platform_audio_direction_e) */

enum platfrom_audio_port_type_e
{
    PLATFORM_AUDIO_LINE = 0,
    PLATFORM_AUDIO_DIFFERENTIAL_LINE,
    PLATFORM_AUDIO_MIC,
    PLATFORM_AUDIO_DIFFERENTIAL_MIC,
    PLATFORM_AUDIO_DIGITAL_MIC,
    PLATFORM_AUDIO_HEADPHONE,
    PLATFORM_AUDIO_I2S,
    PLATFORM_AUDIO_SPDIF,
};
typedef uint8_t platform_audio_port_type_t;     /* port type (see platfrom_audio_port_type_e) */

enum platform_audio_sample_sizes_e
{
    PLATFORM_AUDIO_SAMPLE_SIZE_8_BIT         = (1 <<  0),
    PLATFORM_AUDIO_SAMPLE_SIZE_10_BIT        = (1 <<  1),
    PLATFORM_AUDIO_SAMPLE_SIZE_16_BIT        = (1 <<  2),
    PLATFORM_AUDIO_SAMPLE_SIZE_20_BIT        = (1 <<  3),
    PLATFORM_AUDIO_SAMPLE_SIZE_24_BIT        = (1 <<  4),
    PLATFORM_AUDIO_SAMPLE_SIZE_32_BIT        = (1 <<  5),
};
typedef uint8_t    platform_audio_sample_sizes_t;  /* sample sizes (see platform_audio_sample_sizes_e) */

enum platform_audio_sample_rates_e
{
    PLATFORM_AUDIO_SAMPLE_RATE_8KHZ          = (1 <<  0),
    PLATFORM_AUDIO_SAMPLE_RATE_11_025KHZ     = (1 <<  1),
    PLATFORM_AUDIO_SAMPLE_RATE_12KHZ         = (1 <<  2),
    PLATFORM_AUDIO_SAMPLE_RATE_16KHZ         = (1 <<  3),
    PLATFORM_AUDIO_SAMPLE_RATE_22_05KHZ      = (1 <<  4),
    PLATFORM_AUDIO_SAMPLE_RATE_24KHZ         = (1 <<  5),
    PLATFORM_AUDIO_SAMPLE_RATE_32KHZ         = (1 <<  6),
    PLATFORM_AUDIO_SAMPLE_RATE_44_1KHZ       = (1 <<  7),
    PLATFORM_AUDIO_SAMPLE_RATE_48KHZ         = (1 <<  8),
    PLATFORM_AUDIO_SAMPLE_RATE_64KHZ         = (1 <<  9),
    PLATFORM_AUDIO_SAMPLE_RATE_88_2KHZ       = (1 << 10),
    PLATFORM_AUDIO_SAMPLE_RATE_96KHZ         = (1 << 11),
    PLATFORM_AUDIO_SAMPLE_RATE_128KHZ        = (1 << 12),
    PLATFORM_AUDIO_SAMPLE_RATE_176_4KHZ      = (1 << 13),
    PLATFORM_AUDIO_SAMPLE_RATE_192KHZ        = (1 << 14),
};
typedef uint16_t    platform_audio_sample_rates_t;  /* sample rates (see platform_audio_sample_rates_e) */

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct platform_audio_device_info_s
{
        platform_audio_device_id_t    device_id;
        char *                        device_name;    /* ex: "ak4961_dac"               */
        char *                        description;    /* ex: "4 connector 3.5mm @ J3"   */
        platform_audio_direction_t    direction;
        platform_audio_port_type_t    port_type;
        uint8_t                       num_channels;   /* number of channels device can handle [1-6] */
        platform_audio_sample_sizes_t sample_sizes;
        platform_audio_sample_rates_t sample_rates;
} platform_audio_device_info_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/* These are defined in each platform's platform_audio.c */
extern const platform_audio_device_info_t  platform_audio_input_devices[];
extern const platform_audio_device_info_t  platform_audio_output_devices[];

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wiced_result_t platform_init_audio( void );
extern wiced_result_t platform_deinit_audio( void );

/**
 * Enable audio timer
 *
 * @param[in]  audio_frame_count : audio timer interrupts period expressed in number of audio samples/frames
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_enable( uint32_t frame_count );


/**
 * Disable audio timer
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_disable( void );


/**
 * Wait for audio timer frame sync event
 *
 * @param[in]  timeout_msecs     : timeout value in msecs; WICED_NO_WAIT or WICED_WAIT_FOREVER otherwise.
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_get_frame_sync( uint32_t timeout_msecs );


/**
 * Read audio timer value (tick count)
 *
 * @param[out] time_hi           : upper 32-bit of 64-bit audio timer ticks
 * @param[out] time_lo           : lower 32-bit of 64-bit audio timer ticks
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_get_time( uint32_t *time_hi, uint32_t *time_lo );


/**
 * Get audio timer resolution (ticks per second)
 *
 * @param[in]  audio_sample_rate : audio sample rate
 * @param[out] ticks_per_sec     : returned audio timer resolution
 *
 * @return @ref wiced_result_t
 */
wiced_result_t platform_audio_timer_get_resolution( uint32_t audio_sample_rate, uint32_t *ticks_per_sec );

/** Get Audio Device Info.
 *
 * @param[in]       io_type : input / output Device type
 *                            platform_input_output_t
 * @param[in]       index   : index into list (max = platform_audio_device_count(io_type) - 1)
 *
 * @return  pointer to platform_audio_device_info_t
 *          NULL if bad argument(s)
 */
const platform_audio_device_info_t* platform_audio_device_get_info(platform_audio_direction_t io_type, uint16_t index);

/** Get Audio Device Type
 *
 * @param[in]       device_info   : pointer to partially filled out device information
 *                                  Required fields:
 *                                      direction
 *                                      port_type
 *                                  Optional fields: (checked if they are non-zero)
 *                                      num_channels
 *                                      sample_sizes
 *                                      sample_rates
 *                                  Ignored fields:
 *                                      device_id
 *                                      device_name
 *                                      description
 *
 * @return  platform_audio_device_id_t
 *          AUDIO_DEVICE_ID_NONE on error (no devices match requirements)
 */
platform_audio_device_id_t platform_audio_device_get_type(platform_audio_device_info_t* device_info);

/** Get Device Information
 *
 * @param[in]   device_id : platform_audio_device_id_t
 *
 * @return      pointer to platform_audio_device_info_t
 *              NULL if bad argument(s) or not defined for this platform
 */
const platform_audio_device_info_t* platform_audio_device_get_info_by_id(platform_audio_device_id_t device_id);

/** Get a string of the Port type for debug logging
 *
 * @param[in]   type    : platform_audio_port_type_t
 * @return      description string ("Unknown" on error)
 */
const char* platform_audio_device_get_port_string(platform_audio_port_type_t type);

/** Get a string of the sample rates for debug logging
 *
 * @param[in]   sample_rates  : platform_audio_sample_rates_t
 * @return      description string
 */
const char* platform_audio_device_get_sample_rates_string(platform_audio_sample_rates_t sample_rates);

/** Get a string of the sample sizes for debug logging
 *
 * @param[in]   sample_sizes  : platform_audio_sample_sizes_t
 * @return      description string
 */
const char* platform_audio_device_get_sample_sizes_string(platform_audio_sample_sizes_t sample_sizes);

/** Print audio device info
 *
 * @param audio_input_device_id     : current audio input device (AUDIO_DEVICE_ID_NONE - don't print input information)
 * @param in_dirty                  : !=0, show input device as dirty (not saved to Application DCT)
 * @param audio_output_device_id    : current audio output device (AUDIO_DEVICE_ID_NONE - don't print output information)
 * @param out_dirty                 : !=0, show output device as dirty (not saved to Application DCT)
 * @param verbose                   : print multi-line information
 *
 */void platform_audio_print_device_list(const platform_audio_device_id_t audio_input_device_id, uint8_t in_dirty,
                                      const platform_audio_device_id_t audio_output_device_id, uint8_t out_dirty,
                                      int verbose);

#ifdef __cplusplus
} /*extern "C" */
#endif
