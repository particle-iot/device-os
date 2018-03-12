/**
 * \file
 *
 * \brief Analog Comparator Interface Driver for SAM4L.
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

#ifndef AC_H_INCLUDED
#define AC_H_INCLUDED

#include "compiler.h"
#include "status_codes.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond


/** AC Number of channel */
#define AC_NB_OF_CH           8

/** AC Number of Window */
#define AC_NB_OF_WIN          (AC_NB_OF_CH >> 1)

/** AC Window Interrupt Offset */
#define AC_WIN_INT_OFFSET     24

/** AC configuration */
struct ac_config {
	/** Peripheral Event Trigger Enable */
	bool event_trigger;
};

/* Prototype for device instance. */
struct ac_dev_inst;

typedef void (*ac_callback_t)(void);

/** AC driver hardware instance */
struct ac_dev_inst {
	/** Base address of the AC module. */
	Acifc *hw_dev;
	/** Pointer to AC configuration structure. */
	struct ac_config  *cfg;
};

/** Hysteresis Voltage */
enum ac_hysteresis_voltage {
	AC_HYSTERESIS_0_MV = 0,
	AC_HYSTERESIS_25_MV,
	AC_HYSTERESIS_50_MV,
	AC_HYSTERESIS_75_MV
};

/** Negative input */
enum ac_negative_input{
	AC_NEGTIVE_INPUT_EXTERNAL = 0,
	AC_NEGTIVE_INPUT_RESERVED1,
	AC_NEGTIVE_INPUT_RESERVED2,
	AC_NEGTIVE_INPUT_RESERVED3
};

/** Comparator mode */
enum ac_comparator_mode {
	AC_COMPARATOR_OFF = 0,
	AC_COMPARATOR_CONTINUOUS,
	AC_COMPARATOR_USER_TRIGGERED,
	AC_COMPARATOR_EVENT_TRIGGERED
};

/** Channel interrupt setting */
enum ac_ch_interrupt_setting {
	AC_CH_IS_VINP_GT_VINN = 0,
	AC_CH_IS_VINP_LT_VINN,
	AC_CH_IS_OUTPUT_TGL,
	AC_CH_IS_COMP_DONE
};

/** AC channel configuration */
struct ac_ch_config {
	/** Hysteresis value */
	enum ac_hysteresis_voltage hysteresis_voltage;
	/** Always on enable */
	bool always_on;
	/** Fast mode enable */
	bool fast_mode;
	/** Output event when ACOUT is zero */
	bool event_negative;
	/** Output event when ACOUT is one */
	bool event_positive;
	/** Set the negative input */
	enum ac_negative_input negative_input;
	/** Set the comparator mode */
	enum ac_comparator_mode comparator_mode;
	/** Interrupt settings */
	enum ac_ch_interrupt_setting interrupt_setting;
};

/** Window Event output configuration */
enum ac_win_event_source {
	AC_WIN_EVENT_ACWOUT_RISING_EDGE = 0,
	AC_WIN_EVENT_ACWOUT_FALLING_EDGE,
	AC_WIN_EVENT_ACWOUT_ON_ANY_EDGE,
	AC_WIN_EVENT_INSIDE_WINDOW,
	AC_WIN_EVENT_OUTSIDE_WINDOW,
	AC_WIN_EVENT_MEASURE_DONE,
	AC_WIN_EVENT_RESERVED1,
	AC_WIN_EVENT_RESERVED2
};

/** Window interrupt settings */
enum ac_win_interrupt_setting {
	AC_WIN_IS_VINP_INSIDE_WINDOW = 0,
	AC_WIN_IS_VINP_OUTSIDE_WINDOW,
	AC_WIN_IS_WINDOW_OUTPUT_TGL,
	AC_WIN_IS_WINDOW_COMP_DONE,
	AC_WIN_IS_VINP_ENTER_WINDOW,
	AC_WIN_IS_VINP_LEAVE_WINDOW,
	AC_WIN_IS_RESERVED1,
	AC_WIN_IS_RESERVED2,
};

/** AC Window configuration */
struct ac_win_config {
	/** Window Mode Enable/Disable */
	bool enable;
	/** Window Event from ACWOUT Enable/Disable */
	bool event_enable;
	/** Window Event output configuration */
	enum ac_win_event_source event_source;
	/** Interrupt settings */
	enum ac_win_interrupt_setting interrupt_setting;
};

/** AC Interrupt configuration */
typedef enum ac_interrupt_source {
	AC_INTERRUPT_CONVERSION_COMPLETED_0 = 0,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_0,
	AC_INTERRUPT_CONVERSION_COMPLETED_1,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_1,
	AC_INTERRUPT_CONVERSION_COMPLETED_2,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_2,
	AC_INTERRUPT_CONVERSION_COMPLETED_3,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_3,
	AC_INTERRUPT_CONVERSION_COMPLETED_4,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_4,
	AC_INTERRUPT_CONVERSION_COMPLETED_5,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_5,
	AC_INTERRUPT_CONVERSION_COMPLETED_6,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_6,
	AC_INTERRUPT_CONVERSION_COMPLETED_7,
	AC_INTERRUPT_STARTUP_TIME_ELAPSED_7,
	AC_INTERRUPT_WINDOW_0,
	AC_INTERRUPT_WINDOW_1,
	AC_INTERRUPT_WINDOW_2,
	AC_INTERRUPT_WINDOW_3,
	AC_INTERRUPT_NUM,
} ac_interrupt_source_t;

/**
 * \brief Initializes an Analog Comparator module configuration structure to
 * defaults.
 *
 *  Initializes a given Analog Comparator module configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li Peripheral event trigger is disabled
 *
 * \param cfg  AC module configuration structure to initialize to default value
 */
static inline void ac_get_config_defaults(struct ac_config *const cfg)
{
	/* Sanity check argument */
	Assert(cfg);

	cfg->event_trigger = false;
}

enum status_code ac_init(struct ac_dev_inst *const dev_inst, Acifc *const ac,
		struct ac_config *const cfg);

/**
 * \brief Initializes an Analog Comparator channel configuration structure to
 * defaults.
 *
 *  Initializes a given Analog Comparator channel configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li Hysteresis voltage = 0 mV
 *   \li AC is disabled between measurements
 *   \li Fast mode is disabled
 *   \li AC is disabled between measurements
 *   \li No output peripheral event when ACOUT is zero
 *   \li No output peripheral event when ACOUT is one
 *   \li Negative input from the external pin
 *   \li User triggered single measurement mode
 *   \li Interrupt is issued when the comparison is done
 *
 * \param cfg  AC channel configuration structure to initialize to default value
 */
static inline void ac_ch_get_config_defaults(struct ac_ch_config *const cfg)
{
	/* Sanity check argument */
	Assert(cfg);

	cfg->hysteresis_voltage = AC_HYSTERESIS_0_MV;
	cfg->always_on = false;
	cfg->fast_mode = false;
	cfg->event_negative = false;
	cfg->event_positive = false;
	cfg->negative_input = AC_NEGTIVE_INPUT_EXTERNAL;
	cfg->comparator_mode = AC_COMPARATOR_USER_TRIGGERED;
	cfg->interrupt_setting = AC_CH_IS_COMP_DONE;
}

void ac_ch_set_config(struct ac_dev_inst *const dev_inst, uint32_t channel,
		struct ac_ch_config *cfg);

/**
 * \brief Initializes an Analog Comparator window configuration structure to
 * defaults.
 *
 *  Initializes a given Analog Comparator window configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li Window mode is disabled
 *   \li Peripheral event from ACWOUT is disabled
 *   \li Generate the window peripheral event when measure is done
 *   \li Window interrupt is issued when evaluation of common input voltage is
 *       done
 *
 * \param cfg  AC window configuration structure to initialize to default value
 */
static inline void ac_win_get_config_defaults(struct ac_win_config *const cfg)
{
	/* Sanity check argument */
	Assert(cfg);

	cfg->enable = false;
	cfg->event_enable = false;
	cfg->event_source = AC_WIN_EVENT_MEASURE_DONE;
	cfg->interrupt_setting = AC_WIN_IS_WINDOW_COMP_DONE;
}

void ac_win_set_config(struct ac_dev_inst *const dev_inst,
		uint32_t window, struct ac_win_config *cfg);

void ac_set_callback(struct ac_dev_inst *const dev_inst,
		ac_interrupt_source_t source, ac_callback_t callback,
		uint8_t irq_level);

/**
 * \brief Get AC interrupt status.
 *
 * \param dev_inst Device structure pointer.
 *
 */
static inline uint32_t ac_get_interrupt_status(
		struct ac_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->ACIFC_ISR;
}

/**
 * \brief Get AC interrupt mask.
 *
 * \param dev_inst Device structure pointer.
 *
 */
static inline uint32_t ac_get_interrupt_mask(
		struct ac_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->ACIFC_IMR;
}

/**
 * \brief Clear AC interrupt status.
 *
 * \param dev_inst Device structure pointer.
 * \param status   Interrupt status to be clear.
 *
 */
static inline void ac_clear_interrupt_status(
		struct ac_dev_inst *const dev_inst, uint32_t status)
{
	dev_inst->hw_dev->ACIFC_ICR = status;
}

void ac_enable_interrupt(struct ac_dev_inst *const dev_inst,
		ac_interrupt_source_t source);
void ac_disable_interrupt(struct ac_dev_inst *const dev_inst,
		ac_interrupt_source_t source);

void ac_enable(struct ac_dev_inst *const dev_inst);
void ac_disable(struct ac_dev_inst *const dev_inst);

/**
 * \brief User starts a single comparison.
 *
 * \param dev_inst Device structure pointer.
 */
static inline void ac_user_trigger_single_comparison(
		struct ac_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->ACIFC_CTRL |= ACIFC_CTRL_USTART;
}

/**
 * \brief Check if the comparison is done.
 *
 * \param dev_inst Device structure pointer.
 */
static inline bool ac_is_comparison_done(struct ac_dev_inst *const dev_inst)
{
	return (dev_inst->hw_dev->ACIFC_CTRL & ACIFC_CTRL_USTART !=
			ACIFC_CTRL_USTART);
}

/**
 * \brief Get AC status.
 *
 * \param dev_inst Device structure pointer.
 *
 */
static inline uint32_t ac_get_status(struct ac_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->ACIFC_SR;
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \page sam_acifc_quickstart Quickstart guide for SAM4L Analog Comparator 
  * Interface Driver
 *
 * This is the quickstart guide for the \ref group_sam_drivers_acifc
 * "SAM4L Analog Comparator Interface driver", with step-by-step instructions
 * on how to configure and use the driver in a selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section acifc_basic_use_case Basic use case
 * In this basic use case, AC channel 0 will be configured to compare the
 * inputs from ACAP0 and ACAN0.
 *
 * \subsection sam_acifc_quickstart_prereq Prerequisites
 * -# \ref sysclk_group "System Clock Management (Sysclock)"
 *
 * \section acifc_basic_use_case_setup Setup steps
 * by default.
 * \subsection acifc_basic_use_case_setup_code Example code
 * Enable the following macro in the conf_clock.h:
 * \code
 *  #define CONFIG_SYSCLK_SOURCE       SYSCLK_SRC_DFLL
 *  #define CONFIG_DFLL0_SOURCE        GENCLK_SRC_OSC32K
 * \endcode
 *
 * Add the following code in the application C-file:
 * \code
 *  sysclk_init();
 * \endcode
 *
 * \subsection acifc_basic_use_case_setup_flow Workflow
 * -# Set system clock source as DFLL:
 * \code #define CONFIG_SYSCLK_SOURCE       SYSCLK_SRC_DFLL \endcode
 * -# Set DFLL source as OSC32K:
 * \code #define CONFIG_DFLL0_SOURCE        GENCLK_SRC_OSC32K \endcode
 * -# Initialize the system clock.
 * \code sysclk_init(); \endcode
 *
 * \section acifc_basic_use_case_usage Usage steps
 * \subsection acifc_basic_use_case_usage_code Example code
 * Add to, e.g., main loop in application C-file:
 * \code
 *  struct ac_dev_inst ac_device;
 *  struct ac_config module_cfg;
 *  ac_get_config_defaults(&module_cfg);
 *  ac_init(&ac_device, ACIFC, &module_cfg);
 *  ac_enable(&ac_device);
 *  struct ac_ch_config ch_cfg;
 *  ac_ch_get_config_defaults(&ch_cfg);
 *  ch_cfg.always_on = true;
 *  ch_cfg.fast_mode = true;
 *  ac_ch_set_config(&ac_device, 0, &ch_cfg);
 *  while (!ac_is_comparison_done(&ac_device));
 * \endcode
 *
 * \subsection acifc_basic_use_case_usage_flow Workflow
 * -# Get the default confguration to initialize the module:
 * \code
 *  ac_configure(ACIFC, &acifc_opt);
 *  struct ac_config module_cfg;
 *  ac_get_config_defaults(&module_cfg);
 *  ac_init(&ac_device, ACIFC, &module_cfg);
 * \endcode
 * -# Enable the module:
 * \code  ac_enable(&ac_device); \endcode
 * -# Get the default confguration to initialize the channel 0:
 * \code
 *  struct ac_ch_config ch_cfg;
 *  ac_ch_get_config_defaults(&ch_cfg);
 *  ch_cfg.always_on = true;
 *  ch_cfg.fast_mode = true;
 *  ac_ch_set_config(&ac_device, 0, &ch_cfg);
 * \endcode
 * -# User starts a single comparison:
 * \code  ac_user_trigger_single_comparison(&ac_device); \endcode
 * -# Check if the comparison is done:
 * \code  while (!ac_is_comparison_done(&ac_device)); \endcode
 */

#endif /* AC_H_INCLUDED */
