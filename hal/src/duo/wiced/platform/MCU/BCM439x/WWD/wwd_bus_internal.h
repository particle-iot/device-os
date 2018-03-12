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
#include <stdint.h>
#include "wwd_bus_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DMA_DESCRIPTOR_ERROR      ( 1 << 10 )  /* descriptor error */
#define DMA_DATA_ERROR            ( 1 << 11 )  /* data error */
#define DMA_PROTOCOL_ERROR        ( 1 << 12 )  /* Descriptor protocol Error */
#define DMA_RECEIVE_UNDERFLOW     ( 1 << 13 )  /* Receive descriptor Underflow */
#define DMA_RECEIVE_OVERFLOW      ( 1 << 14 )  /* Receive fifo Overflow */
#define DMA_TRANSMIT_UNDERFLOW    ( 1 << 15 )  /* Transmit fifo Underflow */
#define DMA_RECEIVE_INTERRUPT     ( 1 << 16 )  /* Receive Interrupt */
#define DMA_TRANSMIT_INTERRUPT    ( 1 << 24 )  /* Transmit Interrupt */
#define DMA_ERROR_MASK            ( DMA_DESCRIPTOR_ERROR | DMA_DATA_ERROR | DMA_PROTOCOL_ERROR | DMA_RECEIVE_UNDERFLOW | DMA_RECEIVE_OVERFLOW | DMA_TRANSMIT_UNDERFLOW )  /* DMA Errors */

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern void     wwd_bus_prepare_firmware_download        ( void );
extern void     wwd_bus_write_reset_instruction          ( uint32_t reset_inst );
extern void     wwd_bus_reset_wlan_core                  ( void );
extern void     wwd_bus_reset_wlan_chip                  ( void );
extern void     wwd_bus_init_wlan_uart                   ( void );
extern void     wwd_bus_dma_init                         ( void );
extern void     wwd_bus_dma_deinit                       ( void );
extern void*    wwd_bus_dma_receive                      ( uint16_t** hwtag );
extern void     wwd_bus_dma_transmit                     ( void* data, uint32_t data_size );
extern void     wwd_bus_reclaim_dma_tx_packets           ( void );
extern void     wwd_bus_refill_dma_rx_buffer             ( void );
extern uint32_t wwd_bus_get_rx_packet_size               ( void );
extern uint16_t wwd_bus_get_available_dma_rx_buffer_space( void );
extern void     wwd_bus_enable_dma_interrupt             ( void );
extern void     wwd_bus_disable_dma_interrupt            ( void );
extern void     wwd_bus_mask_dma_interrupt               ( void );
extern void     wwd_bus_unmask_dma_interrupt             ( void );
extern void     wwd_bus_handle_dma_interrupt             ( void );
extern uint32_t wwd_bus_get_dma_interrupt_status         ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
