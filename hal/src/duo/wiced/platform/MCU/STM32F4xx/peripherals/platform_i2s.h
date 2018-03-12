/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced_platform.h"
#include "wiced_audio.h"


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

typedef enum
{
    WICED_I2S_READ,
    WICED_I2S_WRITE
} wiced_i2s_transfer_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef DMA_Stream_TypeDef dma_stream_registers_t;
typedef uint32_t           dma_channel_t;

typedef struct
{
    uint32_t sample_rate;
    uint16_t period_size;
    uint8_t bits_per_sample;
    uint8_t channels;
}wiced_i2s_params_t;

typedef struct
{
    void*    next;
    void*    buffer;
    uint32_t buffer_size;
} platform_i2s_transfer_t;

/* the callback will give a neww buffer pointer to the i2s device */
typedef void(*wiced_i2s_tx_callback_t)(uint8_t** buffer, uint16_t* size, void* context);

/* returned buffer and new buffer to receive more data */
typedef void(*wiced_i2s_rx_callback_t)( uint8_t* buffer, uint16_t read_size,  uint8_t**rx_buffer, uint16_t*rx_buf_size );

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_i2s_init                    ( wiced_audio_session_ref sh, wiced_i2s_t i2s, wiced_i2s_params_t* params, uint32_t* mclk);
wiced_result_t wiced_i2s_deinit                  ( wiced_i2s_t i2s);
wiced_result_t wiced_i2s_set_audio_buffer_details( wiced_i2s_t i2s, uint8_t* buffer_ptr, uint16_t size);
wiced_result_t wiced_i2s_start                   ( wiced_i2s_t i2s);
wiced_result_t wiced_i2s_stop                    ( wiced_i2s_t);
wiced_result_t wiced_i2s_get_current_hw_pointer  ( wiced_i2s_t i2s, uint32_t* hw_pointer );


#ifdef __cplusplus
} /* extern "C" */
#endif
