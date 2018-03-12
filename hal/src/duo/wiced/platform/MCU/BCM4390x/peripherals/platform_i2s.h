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

/* Maximum number of data bytes per DMA descriptor. */
#define I2S_DMA_DATA_BYTES_MAX      (4096)

#define I2S_MAX_CHANNELS            (2)
#define I2S_MAX_BYTES_PER_SAMPLE    (4)
#define I2S_MAX_FRAME_BYTES         (I2S_MAX_CHANNELS * I2S_MAX_BYTES_PER_SAMPLE)

/* I2S RX BufCount + RcvControl.RxOffset == 4KB will cause the DMA engine
 * to repeat previous 32-bytes at the 4KB boundary.  This is likely due to
 * the RxOffset mis-aligning the first frame.
*/
/* Maximum number of bytes of real audio payload data. */
#define I2S_TX_PERIOD_BYTES_MAX     (I2S_DMA_DATA_BYTES_MAX)
#define I2S_RX_PERIOD_BYTES_MAX     ((I2S_DMA_DATA_BYTES_MAX - I2S_DMA_RXOFS_BYTES) / I2S_MAX_FRAME_BYTES * I2S_MAX_FRAME_BYTES)
#define I2S_PERIOD_BYTES_MAX        (I2S_TX_PERIOD_BYTES_MAX > I2S_RX_PERIOD_BYTES_MAX ? I2S_TX_PERIOD_BYTES_MAX : I2S_RX_PERIOD_BYTES_MAX)

#define I2S_DMA_TXOFS_BYTES         (0)
/* RX direction always has a 4-byte offset. */
#define I2S_DMA_RXOFS_BYTES         (4)

#define I2S_DMA_OFS_BYTES           (I2S_DMA_TXOFS_BYTES > I2S_DMA_RXOFS_BYTES ? I2S_DMA_TXOFS_BYTES : I2S_DMA_RXOFS_BYTES)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    WICED_I2S_READ,
    WICED_I2S_WRITE
} wiced_i2s_transfer_t;

typedef enum
{
    WICED_I2S_SPDIF_MODE_OFF,
    WICED_I2S_SPDIF_MODE_ON,
} wiced_i2s_spdif_mode_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

//typedef DMA_Stream_TypeDef dma_stream_registers_t;
typedef uint32_t           dma_channel_t;

typedef struct
{
    uint32_t sample_rate;
    uint16_t period_size;
    uint8_t bits_per_sample;
    uint8_t channels;
    wiced_i2s_spdif_mode_t i2s_spdif_mode;
} wiced_i2s_params_t;

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

wiced_result_t wiced_i2s_init                      ( wiced_audio_session_ref sh, wiced_i2s_t i2s, wiced_i2s_params_t* params, uint32_t* mclk);
wiced_result_t wiced_i2s_deinit                    ( wiced_i2s_t i2s);
wiced_result_t wiced_i2s_set_audio_buffer_details  ( wiced_i2s_t i2s, wiced_audio_buffer_header_t *audio_buffer_ptr);
wiced_result_t wiced_i2s_start                     ( wiced_i2s_t i2s);
wiced_result_t wiced_i2s_stop                      ( wiced_i2s_t i2s);
wiced_result_t wiced_i2s_get_current_hw_pointer    ( wiced_i2s_t i2s, uint32_t* hw_pointer );
wiced_result_t wiced_i2s_pll_set_fractional_divider( wiced_i2s_t i2s, float value );


#ifdef __cplusplus
} /* extern "C" */
#endif
