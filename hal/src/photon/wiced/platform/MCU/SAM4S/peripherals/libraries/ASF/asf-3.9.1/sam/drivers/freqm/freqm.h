/**
 * \file
 *
 * \brief Frequency Meter driver for SAM4L.
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

#ifndef FREQM_H_INCLUDED
#define FREQM_H_INCLUDED

#include "compiler.h"
#include "status_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup sam_drivers_freqm_group Frequency Meter (FREQM)
 *
 * See \ref sam_freqm_quickstart.
 *
 * Driver for the Frequency Meter. This driver provides access to the main
 * features of the FREQM controller.
 *
 * @{
 */

/** Time out value (number of attempts) */
#define FREQM_NUM_OF_ATTEMPTS        1000000

/**
 * The default duration value of a measurement, given in number of
 * reference clock cycles.
 */
#define FREQM_DURATION_DEFAULT       128

/**
 * \brief Frequency Meter configuration structure.
 *
 * Configuration structure for a Frequency Meter instance. This
 * structure could be initialized by the \ref freqm_get_config_defaults()
 * function before being modified by the user application.
 */
struct freqm_config {
	/** Reference Clock Selection */
	uint8_t ref_clk;
	/** Measurement Clock Source Selection */
	uint8_t msr_clk;
	/**
	 * The duration of a measurement, given in number of
	 * reference clock cycles.
	 */
	uint8_t duration;
};

/**
 * \brief Frequency Meter driver software instance structure.
 *
 * Device instance structure for a Frequency Meter driver instance. This
 * structure should be initialized by the \ref freqm_init() function to
 * associate the instance with a particular hardware module of the device.
 */
struct freqm_dev_inst {
	/** Base address of the FREQM module. */
	Freqm *hw_dev;
	/** Pointer to FREQM configuration structure. */
	struct freqm_config  *freqm_cfg;
};

/** FREQM interrupt source type */
typedef enum freqm_interrupt_source {
	FREQM_INTERRUPT_MEASURMENT_READY = FREQM_IER_DONE,
	FREQM_INTERRUPT_REFERENCE_CLOCK_READY = FREQM_IER_RCLKRDY,
	FREQM_INTERRUPT_SOURCE_N
} freqm_interrupt_source_t;

typedef void (*freqm_callback_t)(void);

void freqm_get_config_defaults(struct freqm_config *const cfg);
enum status_code freqm_init(
		struct freqm_dev_inst *const dev_inst,
		Freqm *const freqm,
		struct freqm_config *const cfg);
enum status_code freqm_get_result_blocking(struct freqm_dev_inst *const dev_inst,
		uint32_t *p_result);
void freqm_enable(struct freqm_dev_inst *const dev_inst);
enum status_code freqm_disable(struct freqm_dev_inst *const dev_inst);
void freqm_set_callback(struct freqm_dev_inst *const dev_inst,
		freqm_interrupt_source_t source, freqm_callback_t callback,
		uint8_t irq_level);

/**
 * \brief Start a measurement
 *
 * \param dev_inst  Device structure pointer
 */
static inline void freqm_start_measure(struct freqm_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->FREQM_CTRL = FREQM_CTRL_START;
}

/**
 * \brief Enable refclk
 *
 * \param dev_inst  Device structure pointer
 */
static inline void freqm_enable_refclk(struct freqm_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->FREQM_MODE |= FREQM_MODE_REFCEN;
}

/**
 * \brief Disable refclk
 *
 * \param dev_inst  Device structure pointer
 */
static inline void freqm_disable_refclk(struct freqm_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->FREQM_MODE &= ~FREQM_MODE_REFCEN;
}

/**
 * \brief Get FREQM result value
 *
 * \param dev_inst  Device structure pointer
 *
 * \return Measurement result value
 */
static inline uint32_t freqm_get_result_value(
		struct freqm_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->FREQM_VALUE;
}

/**
 * \brief Get FREQM status
 *
 * \param dev_inst  Device structure pointer
 *
 * \return FREQM status value
 */
static inline uint32_t freqm_get_status(struct freqm_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->FREQM_STATUS;
}

/**
 * \brief Enable FREQM interrupt
 *
 * \param dev_inst  Device structure pointer
 * \param source  Interrupt source
 */
static inline void freqm_enable_interrupt(
		struct freqm_dev_inst *const dev_inst,
		freqm_interrupt_source_t source)
{
	dev_inst->hw_dev->FREQM_IER = (uint32_t)source;
}

/**
 * \brief Disable FREQM interrupt
 *
 * \param dev_inst  Device structure pointer
 * \param source  Interrupt source
 */
static inline void freqm_disable_interrupt(
		struct freqm_dev_inst *const dev_inst,
		freqm_interrupt_source_t source)
{
	dev_inst->hw_dev->FREQM_IDR = (uint32_t)source;
}

/**
 * \brief Get FREQM interrupt status.
 *
 * \param dev_inst  Device structure pointer
 *
 * \return Interrupt status value
 */
static inline uint32_t freqm_get_interrupt_status(
		struct freqm_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->FREQM_ISR;
}

/**
 * \brief Clear FREQM interrupt status.
 *
 * \param dev_inst  Device structure pointer.
 * \param source Interrupt source.
 *
 */
static inline void freqm_clear_interrupt_status(
		struct freqm_dev_inst *const dev_inst,
		freqm_interrupt_source_t source)
{
	dev_inst->hw_dev->FREQM_ICR = (uint32_t)source;
}

/**
 * \brief Get FREQM interrupt mask.
 *
 * \param dev_inst  Device structure pointer.
 *
 * \return Interrupt mask value
 */
static inline uint32_t freqm_get_interrupt_mask(
		struct freqm_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->FREQM_IMR;
}

//@}

#ifdef __cplusplus
}
#endif

/**
 * \page sam_freqm_quickstart Quickstart guide for SAM FREQM driver
 *
 * This is the quickstart guide for the \ref freqm_group "SAM FREQM driver",
 * with step-by-step instructions on how to configure and use the driver in a
 * selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section freqm_basic_use_case Basic use case
 * In this basic use case, the FREQM module are configured for:
 * - Select CLK32K as refclk
 * - Select CLK_CPU as msrclk.
 * - Duration of a measurement is 128
 *
 * \subsection sam_freqm_quickstart_prereq Prerequisites
 * -# \ref sysclk_group "System Clock Management (Sysclock)"
 *
 * \section freqm_basic_use_case_setup Setup steps
 * \subsection freqm_basic_use_case_setup_code Example code
 * Add to application C-file:
 * \code
 *    freqm_get_config_defaults(&g_freqm_cfg);
 *
 *    freqm_init(&g_freqm_inst, FREQM, &g_freqm_cfg);
 *
 *    freqm_enable(&g_freqm_inst);
 *
 *    freqm_start_measure(&g_freqm_inst);
 *
 *    freqm_get_result_blocking(&g_freqm_inst, &result);
 * \endcode
 *
 * \subsection freqm_basic_use_case_setup_flow Workflow
 * -# Initializes Frequency Meter configuration structure to defaults:
 *   - \code freqm_get_config_defaults(&g_freqm_cfg); \endcode
 * -# Configure FREQM line with specified mode:
 *   - \code freqm_set_configure(FREQM, 1, 0, 128); \endcode
 * -# Start Measurement.
 *   - \code freqm_start_measure(FREQM); \endcode
 * -# Get mesurement result:
 * \code
 * 	cpu_clk = (freqm_get_result_blocking(FREQM) / 128) * 32768;
 * \endcode
 */
#endif /* FREQM_H_INCLUDED */
