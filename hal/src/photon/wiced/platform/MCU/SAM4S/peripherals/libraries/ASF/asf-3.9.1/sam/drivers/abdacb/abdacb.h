/**
 * \file
 *
 * \brief ABDAC driver for SAM.
 *
 * Copyright (C) 2013 Atmel Corporation. All rights reserved.
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

#ifndef _ABDAC_H_INCLUDED
#define _ABDAC_H_INCLUDED

/**
 * \defgroup group_sam_drivers_abdacb ABDAC - Audio Bitstream DAC
 *
 * Audio Bitstream DAC (Digital to Analog Converter) provides functions to
 * convert a sample value to a digital bitstream.
 *
 * \{
 */

#include "compiler.h"
#include "status_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABDAC_BUSY_TIMEOUT 10000

/** Supported sample rate */
enum abdac_sample_rate {
	ABDAC_SAMPLE_RATE_8000 = 0,
	ABDAC_SAMPLE_RATE_11025,
	ABDAC_SAMPLE_RATE_12000,
	ABDAC_SAMPLE_RATE_16000,
	ABDAC_SAMPLE_RATE_22050,
	ABDAC_SAMPLE_RATE_24000,
	ABDAC_SAMPLE_RATE_32000,
	ABDAC_SAMPLE_RATE_44100,
	ABDAC_SAMPLE_RATE_48000,
};

/** Data format of the SDR register value */
enum abdac_data_format {
	ABDAC_DATE_32BIT = 0,
	ABDAC_DATE_20BIT,
	ABDAC_DATE_18BIT,
	ABDAC_DATE_16BIT,
	ABDAC_DATE_16BIT_COMPACT,
	ABDAC_DATE_8BIT,
	ABDAC_DATE_8BIT_COMPACT,
};

/** Configuration setting structure */
struct abdac_config {
	/* Sample rate. */
	enum abdac_sample_rate sample_rate_hz;
	/* Number of bit per sample. */
	enum abdac_data_format data_word_format;
	/* 1 for Mono, 0 for stereo. */
	bool mono;
	/* 1 for enable, 0 for disable. */
	bool cmoc;
};

/** Interrupt sources */
typedef enum abdac_interrupt_source {
	ABDAC_INTERRUPT_TXRDY = 0,
	ABDAC_INTERRUPT_TXUR,
} abdac_interrupt_source_t;

/**
 * \brief ABDAC driver software instance structure.
 *
 * Device instance structure for a ABDAC driver instance. This
 * structure should be initialized by the \ref abdac_init() function to
 * associate the instance with a particular hardware module of the device.
 */
struct abdac_dev_inst {
	/** Base address of the ABDAC module. */
	Abdacb *hw_dev;
	/** Pointer to ABDAC configuration structure. */
	struct abdac_config  *cfg;
};

/**
 * \brief Interrupt callback function type for ABDAC.
 *
 * The interrupt handler can be configured to do a function callback,
 * the callback function must match the abdac_callback_t type.
 *
 */
typedef void (*abdac_callback_t)(void);

/**
 * \brief Initialize a ABDAC configuration structure to defaults.
 *
 *  Initialize a given ABDAC configuration structure to a set of
 *  known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *  - sample rate with 8000hz.
 *  - 16 bit data format.
 *  - stereo mode.
 *  - Common mode adjustment is disabled.
 *
 *  \param cfg    Configuration structure to initialize to default values.
 */
void abdac_get_config_defaults(struct abdac_config *const cfg);

/**
 * \brief Initialize the ABDAC module.
 *
 * \param dev_inst    Device structure pointer.
 * \param abdac         Base address of the ABDAC instance.
 * \param cfg         Pointer to ABDAC configuration.
 *
 * \return Status code.
 */
status_code_t abdac_init(struct abdac_dev_inst *const dev_inst,
		Abdacb *const abdac, struct abdac_config *const cfg);

/**
 * \brief Check the busy status of ABDAC.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return true if ABDAC is busy, else it will return false.
 */
static inline bool abdac_is_busy(struct abdac_dev_inst *const dev_inst)
{
	return (dev_inst->hw_dev->ABDACB_SR & ABDACB_SR_BUSY) != 0;
}

/**
 * \brief Check the transmit ready status of ABDAC.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return true if ABDAC is ready to receive a new data in SDR,
 * else it will return false.
 */
static inline bool abdac_is_tx_ready(struct abdac_dev_inst *const dev_inst)
{
	return (dev_inst->hw_dev->ABDACB_SR & ABDACB_SR_TXRDY) != 0;
}

/**
 * \brief Check the transmit underrun status of ABDAC.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return true if at least one underrun has occurred since the last time
 * this bit was cleared, else it will return false.
 */
static inline bool abdac_is_tx_underrun(
		struct abdac_dev_inst *const dev_inst)
{
	return (dev_inst->hw_dev->ABDACB_SR & ABDACB_SR_TXUR) != 0;
}

/**
 * \brief Return the ABDAC interrupts mask value.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return Interrupt mask value
 */
static inline uint32_t abdac_read_interrupt_mask(
		struct abdac_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->ABDACB_IMR;
}

/**
 * \brief Enable the ABDAC module.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return Status code.
 */
status_code_t abdac_enable(struct abdac_dev_inst *const dev_inst);

/**
 * \brief Disable the ABDAC module.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return Status code.
 */
status_code_t abdac_disable(struct abdac_dev_inst *const dev_inst);

/**
 * \brief Configure the ABDAC module.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return Status code.
 */
status_code_t abdac_set_config(struct abdac_dev_inst *const dev_inst);

/**
 * \brief Software reset the ABDAC module.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return Status code.
 */
status_code_t abdac_sw_reset(struct abdac_dev_inst *const dev_inst);

/**
 * \brief Swap the ABDAC channel output.
 *
 * \param dev_inst Device structure pointer..
 *
 * \return Status code.
 */
status_code_t abdac_swap_channels(struct abdac_dev_inst *const dev_inst);

/**
 * \brief Writes the data to SDR0.
 *
 * \param dev_inst Device structure pointer..
 * \param data Data value to write to SDR0.
 *
 * \return Status code.
 */
status_code_t abdac_write_data0(struct abdac_dev_inst *const dev_inst,
		uint32_t data);

/**
 * \brief Writes the data to SDR1.
 *
 * \param dev_inst Device structure pointer..
 * \param data Data value to write to SDR1.
 */
status_code_t abdac_write_data1(struct abdac_dev_inst *const dev_inst,
		uint32_t data);

/**
 * \brief Set the volume of channel 0.
 *
 * \param dev_inst Device structure pointer..
 * \param mute Flag if set the channel mute
 * \param volume Value of volume
 *
 * \return Status code.
 */
void abdac_set_volume0(struct abdac_dev_inst *const dev_inst, bool mute,
		uint32_t volume);

/**
 * \brief Set the volume of channel 1.
 *
 * \param dev_inst Device structure pointer..
 * \param mute Flag if set the channel mute
 * \param volume Value of volume
 *
 * \return Status code.
 */
void abdac_set_volume1(struct abdac_dev_inst *const dev_inst, bool mute,
		uint32_t volume);

/**
 * \brief Enable the interrupt.
 *
 * \param dev_inst Device structure pointer..
 * \param source Interrupt to be enabled
 */
void abdac_enable_interrupt(struct abdac_dev_inst *const dev_inst,
		abdac_interrupt_source_t source);

/**
 * \brief Disable the interrupt.
 *
 * \param dev_inst Device structure pointer..
 * \param source Interrupt to be disabled
 */
void abdac_disable_interrupt(struct abdac_dev_inst *const dev_inst,
		abdac_interrupt_source_t source);

/**
 * \brief Clear the interrupt status.
 *
 * \param dev_inst Device structure pointer..
 * \param source Interrupt status to be cleared
 */
void abdac_clear_interrupt_flag(struct abdac_dev_inst *const dev_inst,
		abdac_interrupt_source_t source);

/**
 * \brief Set callback for ABDAC
 *
 * \param dev_inst Device structure pointer.
 * \param source Interrupt source.
 * \param callback callback function pointer.
 * \param irq_level interrupt level.
 */
void abdac_set_callback(struct abdac_dev_inst *const dev_inst,
		abdac_interrupt_source_t source, abdac_callback_t callback,
		uint8_t irq_level);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

/**
 * \page sam_abdac_quick_start Quick Start Guide for the ABDAC driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_abdac, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.The code examples can be copied into e.g the main
 * application loop or any other function that will need to control the
 * ABDAC module.
 *
 * \section abdac_qs_use_cases Use cases
 * - \ref abdac_basic
 *
 * \section abdac_basic ABDAC basic usage
 *
 * This use case will demonstrate how to initialize the ABDAC module to
 * output audio data.
 *
 *
 * \section abdac_basic_setup Setup steps
 *
 * \subsection abdac_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 *
 * \subsection abdac_basic_setup_code
 *
 * Add this to the main loop or a setup function:
 * \code
 * struct abdac_dev_inst g_abdac_inst;
 * struct abdac_config   g_abdac_cfg;
 * abdac_get_config_defaults(&g_abdac_cfg);
 * g_abdac_cfg.sample_rate_hz = ABDAC_SAMPLE_RATE_11025;
 * g_abdac_cfg.data_word_format = ABDAC_DATE_16BIT;
 * g_abdac_cfg.mono = false;
 * g_abdac_cfg.cmoc = false;
 * abdac_init(&g_abdac_inst, ABDACB, &g_abdac_cfg);
 * abdac_enable(&g_abdac_inst);
 * \endcode
 *
 * \subsection abdac_basic_setup_workflow
 *
 * -# Initialize the ABDAC module
 * -# Enable the ABDAC mode
 *
 *  - \note The syste clock may changed after setting the ABDAC module.
 *
 *
 * \section abdac_basic_usage Usage steps
 *
 * \subsection abdac_basic_usage_code
 *
 * We can set the volume by
 * \code
 * abdac_set_volume0(&g_abdac_inst, false, 0x7FFF);
 * abdac_set_volume1(&g_abdac_inst, false, 0x7FFF);
 * \endcode
 * Or we can mute the volume by
 * \code
 * abdac_set_volume0(&g_abdac_inst, true, 0x7FFF);
 * abdac_set_volume1(&g_abdac_inst, true, 0x7FFF);
 * \endcode
 *
 * We can output the data without PDC by
 * \code
 * abdac_write_data0(&g_abdac_inst, data);
 * abdac_write_data0(&g_abdac_inst, data);
 * \endcode
 *
 * And we can set the interrupt by
 * \code
 * abdac_set_callback(&g_abdac_inst, ABDAC_INTERRUPT_TXRDY, callback, 3)
 * \endcode
 */

#endif  /* _ABDAC_H_INCLUDED */

