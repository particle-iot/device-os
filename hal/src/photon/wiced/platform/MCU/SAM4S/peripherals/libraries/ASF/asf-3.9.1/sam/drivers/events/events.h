/**
 * \file
 *
 * \brief Peripheral Event Controller Driver for SAM.
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
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

#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup group_sam_drivers_events Peripheral Event Controller Driver for SAM
 *
 * This driver provides a unified interface for the configuration and
 * management of the event channels within the device.
 *
 * The peripheral event generators and users are interconnected by a network
 * known as the Peripheral Event System.
 *
 * The Peripheral Event System allows low latency peripheral-to-peripheral
 * signalling without CPU intervention, and without consuming system resources
 * such as bus or RAM bandwidth. This offloads the CPU and system resources
 * compared to a traditional interrupt-based software driven system.
 *
 * @{
 */

/** Maximum number for event channels (users) */
#define EVENT_CHANNEL_N      PEVC_TRIGOUT_BITS

/** Maximum number for event generator */
#define EVENT_GENERATOR_N    PEVC_EVIN_BITS

/**
 * \brief Input Glitch Filter divider configurations.
 *
 * Enumerate for the possible division ratio of Input Glitch Filter.
 */
enum events_igf_divider {
	EVENT_IGF_DIVIDER_1       = 0,
	EVENT_IGF_DIVIDER_2       = 1,
	EVENT_IGF_DIVIDER_4       = 2,
	EVENT_IGF_DIVIDER_8       = 3,
	EVENT_IGF_DIVIDER_16      = 4,
	EVENT_IGF_DIVIDER_32      = 5,
	EVENT_IGF_DIVIDER_64      = 6,
	EVENT_IGF_DIVIDER_128     = 7,
	EVENT_IGF_DIVIDER_256     = 8,
	EVENT_IGF_DIVIDER_512     = 9,
	EVENT_IGF_DIVIDER_1024    = 10,
	EVENT_IGF_DIVIDER_2048    = 11,
	EVENT_IGF_DIVIDER_4096    = 12,
	EVENT_IGF_DIVIDER_8192    = 13,
	EVENT_IGF_DIVIDER_16384   = 14,
	EVENT_IGF_DIVIDER_32768   = 15
};

/**
 * \brief Event Input Glitch Filter edge detection configurations.
 */
enum events_igf_edge {
	/** Input Glitch Filter is disabled */
	EVENT_IGF_EDGE_NONE    = 0,
	/** Event detection through Input Glitch Fiilter on rising edge. */
	EVENT_IGF_EDGE_RISING  = 1,
	/** Event detection through Input Glitch Fiilter on falling edge. */
	EVENT_IGF_EDGE_FALLING = 2,
	/** Event detection through Input Glitch Fiilter on both edges. */
	EVENT_IGF_EDGE_BOTH    = 3
};

/**
 * \brief Event configuration structure.
 *
 * Configuration structure for event module.
 */
struct events_conf {
	/** Input Glitch Filter divider */
	enum events_igf_divider igf_divider;
};

/**
 * \brief Event channel configuration structure.
 *
 * Configuration structure for an event channel.
 */
struct events_ch_conf {
	/** Channel to configure (user) */
	uint32_t channel_id;
	/** Event generator to connect to the channel */
	uint32_t generator_id;
	/** Enable event sharper or not */
	bool sharper_enable;
	/** Edge detection for event channels */
	enum events_igf_edge   igf_edge;
};

void events_get_config_defaults(struct events_conf *const config);

void events_init(struct events_conf *const config);

void events_enable(void);

void events_disable(void);

/**
 * \brief Set Input Glitch Filter Divider.
 *
 * \param  divider      Input Glitch Filter divider.
 *
 * \note As stated in the datasheet, there is one divider value for
 * all EVS instance.
 */
static inline void events_set_igf_divider(enum events_igf_divider divider)
{
	PEVC->PEVC_IGFDR = PEVC_IGFDR_IGFDR(divider);
}

void events_ch_get_config_defaults(struct events_ch_conf *const config);

void events_ch_configure(struct events_ch_conf *const config);

/**
 * \brief Enable an event channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_enable(uint32_t channel_id)
{
	PEVC->PEVC_CHER = PEVC_CHER_CHE(PEVC_CHER_CHE_1 << channel_id);
}

/**
 * \brief Disable an event channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_disable(uint32_t channel_id)
{
	PEVC->PEVC_CHDR = PEVC_CHDR_CHD(PEVC_CHDR_CHD_1 << channel_id);
}

/**
 * \brief Get status (enabled or disabled) of a channel.
 *
 * \param channel_id  Channel ID.
 *
 * \retval true  channel is enabled.
 * \retval false channel is disabled.
 */
static inline bool events_ch_is_enabled(uint32_t channel_id)
{
	if (PEVC->PEVC_CHSR & PEVC_CHSR_CHS(PEVC_CHSR_CHS_1 << channel_id)) {
		return true;
	} else {
		return false;
	}
}

/**
 * \brief Get the busy status of an event channel.
 *
 * \param channel_id  Channel ID.
 *
 *  \retval true  If the channel is ready to be used
 *  \retval false If the channel is currently busy
 */
static inline bool events_ch_is_ready(uint32_t channel_id)
{
	if (PEVC->PEVC_BUSY & PEVC_BUSY_BUSY(PEVC_BUSY_BUSY_1 << channel_id)) {
		return false;
	} else {
		return true;
	}
}

/**
 * \brief Enable software trigger for a channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_enable_software_trigger(uint32_t channel_id)
{
	PEVC->PEVC_CHMX[channel_id].PEVC_CHMX |= PEVC_CHMX_SMX;
}

/**
 * \brief Disable software trigger for a channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_disable_software_trigger(uint32_t channel_id)
{
	PEVC->PEVC_CHMX[channel_id].PEVC_CHMX &= (~PEVC_CHMX_SMX);
}

/**
 * \brief Trigger a Software Event for the corresponding channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_software_trigger(uint32_t channel_id)
{
	PEVC->PEVC_SEV = PEVC_SEV_SEV(PEVC_SEV_SEV_1 << channel_id);
}

/**
 * \brief Get the trigger status of an event channel.
 *
 * \param channel_id  Channel ID.
 *
 *  \retval true  A channel event has occurred
 *  \retval false A channel event has not occurred
 */
static inline bool events_ch_is_triggered(uint32_t channel_id)
{
	if (PEVC->PEVC_TRSR & PEVC_TRSR_TRS(PEVC_TRSR_TRS_1 << channel_id)) {
		return true;
	} else {
		return false;
	}
}

/**
 * \brief Clear the trigger status of an event channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_clear_trigger_status(uint32_t channel_id)
{
	PEVC->PEVC_TRSCR = PEVC_TRSCR_TRSC(PEVC_TRSCR_TRSC_1 << channel_id);
}

/**
 * \brief Get the overrun status of an event channel.
 *
 * \param channel_id  Channel ID.
 *
 *  \retval true  A channel overrun event has occurred
 *  \retval false A channel overrun event has not occurred
 */
static inline bool events_ch_is_overrun(uint32_t channel_id)
{
	if (PEVC->PEVC_OVSR & PEVC_OVSR_OVS(PEVC_OVSR_OVS_1 << channel_id)) {
		return true;
	} else {
		return false;
	}
}

/**
 * \brief Clear the overrun status of an event channel.
 *
 * \param channel_id  Channel ID.
 */
static inline void events_ch_clear_overrun_status(uint32_t channel_id)
{
	PEVC->PEVC_OVSCR = PEVC_OVSCR_OVSC(PEVC_OVSCR_OVSC_1 << channel_id);
}

/** @} */

#ifdef __cplusplus
}
#endif

/**
 * \page sam_events_quick_start Quick Start Guide for the Event System Driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_events, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section events_qs_use_cases Use Cases
 * - \ref event_basic_use_case
 *
 * \section event_basic_use_case Event Basic Use Case
 *
 * This use case will demonstrate how to use the Peripheral Event Controller
 * on SAM4L_EK. In this use case, one event channel is configured as:
 * - Configure AST periodic event 0 as a generator.
 * - Configure PDCA channel 0 as a user to transfer one word.
 * - Enable the event sharper for the generator.
 *
 * \section event_basic_setup Setup Steps
 *
 * \subsection event_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 *
 * \subsection event_basic_setup_code Setup Code Example
 *
 * Add this to the main loop or a setup function:
 * \code
 *   struct events_conf    events_config;
 *   struct events_ch_conf ch_config;
 *
 *   // Initialize AST as event generator
 *   init_ast();
 *
 *   // Initialize the PDCA as event user
 *   init_pdca();
 *
 *   // Initialize event module
 *   events_get_config_defaults(&events_config);
 *   events_init(&events_config);
 *   events_enable();
 *
 *   // Configure an event channel
 *   // - AST periodic event 0 --- Generator
 *   // - PDCA channel 0       --- User
 *   events_ch_get_config_defaults(&ch_config);
 *   ch_config.channel_id = PEVC_ID_USER_PDCA_0;
 *   ch_config.generator_id = PEVC_ID_GEN_AST_2;
 *   ch_config.sharper_enable = true;
 *   ch_config.igf_edge = EVENT_IGF_EDGE_NONE;
 *   events_ch_configure(&ch_config);
 *
 *   // Enable the channel
 *   events_ch_enable(PEVC_ID_USER_PDCA_0);
 * \endcode
 *
 * \subsection event_basic_setup_workflow Basic Setup Workflow
 *
 * -# Initialize AST to generate periodic event 0,
 *  see sam/drivers/events/example1 for detail.
 *  \code
 *   init_ast();
 *  \endcode
 * -# Initialize PDCA channel 0 to tranfer data to USART,
 *  see sam/drivers/events/example1 for detail.
 *  \code
 *   init_pdca();
 *  \endcode
 * -# Initialize the event module and enable it.
 *  \code
 *   struct events_conf    events_config;
 *
 *   // Initialize event module
 *   events_get_config_defaults(&events_config);
 *   events_init(&events_config);
 *   events_enable();
 *  \endcode
 * -# Initialize the event channel and enable it.
 *  \code
 *   struct events_ch_conf ch_config;
 *
 *   events_ch_get_config_defaults(&ch_config);
 *   ch_config.channel_id = PEVC_ID_USER_PDCA_0;
 *   ch_config.generator_id = PEVC_ID_GEN_AST_2;
 *   ch_config.sharper_enable = true;
 *   ch_config.igf_edge = EVENT_IGF_EDGE_NONE;
 *   events_ch_configure(&ch_config);
 *   events_ch_enable(PEVC_ID_USER_PDCA_0);
 *  \endcode
 *
 * \section event_basic_usage Event Basic Usage
 *
 * After the channel is configured correctly and enabled, each time a new
 * event from AST is coming, a character is sent to the USART via PDCA without
 * the use of the CPU.
 */

#endif /* EVENTS_H_INCLUDED */
