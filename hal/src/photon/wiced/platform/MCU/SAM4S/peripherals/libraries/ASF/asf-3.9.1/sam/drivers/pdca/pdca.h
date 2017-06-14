/**
 * \file
 *
 * \brief PDCA driver for SAM4L.
 *
 * This file defines a useful set of functions for the PDCA interface on SAM4L
 * devices.
 *
 * Copyright (c) 2012-2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef PDCA_H_INCLUDED
#define PDCA_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/** Number of available PDCA channels, device dependent. */
#if SAM4L
#define PDCA_NUMBER_OF_CHANNELS                 PDCA_CHANNEL_LENGTH
#else
#error  'This device does not support PDCA driver'
#endif

/** PDCA channel number type */
typedef uint8_t pdca_channel_num_t;

/** PDCA channel interrupt mask type */
typedef uint32_t pdca_channel_interrupt_mask_t;

/** brief PDCA channel status */
enum pdca_channel_status {
	/** PDCA channel is disabled */
	PDCA_CH_FREE = 0,
	/** PDCA channel is enabled but transfer is on-going */
	PDCA_CH_BUSY,
	/** PDCA channel counter reload is zero */
	PDCA_CH_COUNTER_RELOAD_IS_ZERO,
	/** PDCA channel has completed a block transfer */
	PDCA_CH_TRANSFER_COMPLETED,
	/** PDCA channel failed to complete a block transfer */
	PDCA_CH_TRANSFER_ERROR,
};

/** PDCA channel options */
typedef struct {
	/** Memory address */
	volatile void *addr;
	/** Transfer counter */
	uint32_t size;
	/** Next memory address */
	volatile void *r_addr;
	/** Next transfer counter */
	uint32_t r_size;
	/** Select peripheral ID */
	uint32_t pid;
	/** Select the size of the transfer (byte, half-word or word) */
	uint32_t transfer_size;
	/**  Enable (\c true) or disable (\c false) the transfer upon event trigger. */
	bool etrig;
	/** Ring buffer function */
	bool ring;
} pdca_channel_config_t;

typedef void (*pdca_callback_t) (enum pdca_channel_status status);

void pdca_channel_set_callback(pdca_channel_num_t pdca_ch_number,
		pdca_callback_t callback, uint8_t irq_line, uint8_t irq_level,
		const pdca_channel_interrupt_mask_t pdca_channel_interrupt_mask);
volatile PdcaChannel *pdca_channel_get_handler(pdca_channel_num_t
		pdca_ch_number);
void pdca_disable(Pdca *pdca);
void pdca_enable(Pdca *pdca);
void pdca_channel_set_config(pdca_channel_num_t pdca_ch_number,
		const pdca_channel_config_t *cfg);
void pdca_channel_write_load(pdca_channel_num_t pdca_ch_number,
		volatile void *addr, uint32_t size);
void pdca_channel_write_reload(pdca_channel_num_t pdca_ch_number,
		volatile void *addr, uint32_t size);
uint32_t pdca_channel_read_load_size(pdca_channel_num_t pdca_ch_number);
uint32_t pdca_channel_read_reload_size(pdca_channel_num_t pdca_ch_number);
bool pdca_channel_is_enabled(pdca_channel_num_t pdca_ch_number);
enum pdca_channel_status
		pdca_get_channel_status(pdca_channel_num_t pdca_ch_number);
void pdca_channel_disable(pdca_channel_num_t pdca_ch_number);
void pdca_channel_enable(pdca_channel_num_t pdca_ch_number);
void pdca_channel_clear_error(pdca_channel_num_t pdca_ch_number);
void pdca_channel_disable_interrupt(pdca_channel_num_t pdca_ch_number,
		const pdca_channel_interrupt_mask_t pdca_channel_interrupt_mask);
void pdca_channel_enable_interrupt(pdca_channel_num_t pdca_ch_number,
		const pdca_channel_interrupt_mask_t pdca_channel_interrupt_mask);
pdca_channel_interrupt_mask_t
		pdca_channel_get_interrupt_mask(pdca_channel_num_t pdca_ch_number);

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \page sam_pdca_quickstart Quickstart guide for SAM PDCA driver
 *
 * This is the quickstart guide for the \ref sam_drivers_pdc_group "SAM PDCA driver",
 * with step-by-step instructions on how to configure and use the driver in a
 * selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section pdca_basic_use_case Basic use case
 * In this basic use case, the PDCA module and channel are configured for:
 * - Select USART2 as peripheral
 * - Interrupt-based handling
 *
 * \subsection sam_pdca_quickstart_prereq Prerequisites
 * -# \ref sysclk_group "System Clock Management (Sysclock)"
 *
 * \section pdca_basic_use_case_setup Setup steps
 * \subsection pdca_basic_use_case_setup_code Example code
 * Add to application C-file:
 * \code
 *   void pdca_callback(void)
 *   {
 *       //Get PDCA RX channel status and check if PDCA transfer complete
 *       if (status == PDCA_CH_TRANSFER_COMPLETED) {
 *           pdca_channel_write_load(PDCA_RX_CHANNEL, g_uc_pdc_buffer, BUFFER_SIZE);
 *           pdca_channel_write_load(PDCA_TX_CHANNEL, g_uc_pdc_buffer, BUFFER_SIZE);
 *       }
 *   }
 *   void pdca_setup(void)
 *   {
 *       pdca_enable(PDCA);
 *
 *       pdca_channel_write_config(PDCA_RX_CHANNEL, &PDCA_RX_CONFIGS);
 *       pdca_channel_write_config(PDCA_TX_CHANNEL, &PDCA_TX_CONFIGS);
 *
 *       pdca_channel_set_callback(PDCA_RX_CHANNEL, pdca_tranfer_done, PDCA_0_IRQn, 1, PDCA_IER_TRC);
 *
 *       pdca_channel_enable(PDCA_RX_CHANNEL);
 *       pdca_channel_enable(PDCA_TX_CHANNEL);
 *   }
 * \endcode
 *
 * \subsection pdca_basic_use_case_setup_flow Workflow
 * -# Define the interrupt callback function in the application:
 *   - \code
 *   void pdca_callback(void)
 *   {
 *       //Get PDCA RX channel status and check if PDCA transfer complete
 *       if (status == PDCA_CH_TRANSFER_COMPLETED) {
 *           pdca_channel_write_load(PDCA_RX_CHANNEL, g_uc_pdc_buffer, BUFFER_SIZE);
 *           pdca_channel_write_load(PDCA_TX_CHANNEL, g_uc_pdc_buffer, BUFFER_SIZE);
 *       }
 *   }
 * \endcode
 * -# Enable PDCA module:
 *   - \code pdca_enable(PDCA); \endcode
 *   - \note Including enable module clock and lock sleep mode.
 * -# Configure PDCA channel with specified mode:
 *   - \code pdca_channel_write_config(PDCA_RX_CHANNEL, &PDCA_RX_CONFIGS);
 *                pdca_channel_write_config(PDCA_TX_CHANNEL, &PDCA_TX_CONFIGS);
 *  \endcode
 * -# Set the PDCA callback function and enable PDCA interrupt.
 *   - \code pdca_channel_set_callback(PDCA_RX_CHANNEL, pdca_tranfer_done, PDCA_0_IRQn, 1, PDCA_IER_TRC); \endcode
 * -# Enable PDCA channel:
 *   - \code pdca_channel_enable(PDCA_RX_CHANNEL);
 *                pdca_channel_enable(PDCA_TX_CHANNEL);
 * \endcode
 */
#endif /* PDCA_H_INCLUDED */
