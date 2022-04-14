/**
 * \file
 *
 * \brief Pulse Width Modulation (PWM) driver for SAM.
 *
 * Copyright (c) 2011-2013 Atmel Corporation. All rights reserved.
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

#ifndef PWM_H_INCLUDED
#define PWM_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

#define PWM_INVALID_ARGUMENT  0xFFFF

/** Definitions for PWM channel number */
typedef enum _pwm_ch_t {
	PWM_CHANNEL_0 = 0,
	PWM_CHANNEL_1 = 1,
	PWM_CHANNEL_2 = 2,
	PWM_CHANNEL_3 = 3,
#if (SAM3XA)
	PWM_CHANNEL_4 = 4,
	PWM_CHANNEL_5 = 5,
	PWM_CHANNEL_6 = 6,
	PWM_CHANNEL_7 = 7
#endif /* (SAM3XA) */
} pwm_ch_t;

/** Definitions for PWM channel alignment */
typedef enum {
	PWM_ALIGN_LEFT = (0 << 8),   /* The period is left aligned. */
	PWM_ALIGN_CENTER = (1 << 8)  /* The period is center aligned. */
} pwm_align_t;

/** Definitions for PWM level */
typedef enum {
	PWM_LOW = LOW,     /* Low level */
	PWM_HIGH = HIGH,  /* High level */
#if SAM4E
	PWM_HIGHZ,  /* High Impedance */
#endif
} pwm_level_t;

/** Input parameters when initializing PWM */
typedef struct {
	/** Frequency of clock A in Hz (set 0 to turn it off) */
	uint32_t ul_clka;
	/** Frequency of clock B in Hz (set 0 to turn it off) */
	uint32_t ul_clkb;
	/** Frequency of master clock in Hz */
	uint32_t ul_mck;
} pwm_clock_t;

#if (SAM3U || SAM3S || SAM3XA || SAM4S || SAM4E)
/** Definitions for PWM channels used by motor stepper */
typedef enum {
	PWM_STEPPER_MOTOR_CH_0_1 = 0,  /* Channel 0 and 1 */
	PWM_STEPPER_MOTOR_CH_2_3 = 1,  /* Channel 2 and 3 */
#if (SAM3XA)
	PWM_STEPPER_MOTOR_CH_4_5 = 2,  /* Channel 4 and 5 */
	PWM_STEPPER_MOTOR_CH_6_7 = 3   /* Channel 6 and 7 */
#endif /* (SAM3XA) */
} pwm_stepper_motor_pair_t;

/** Definitions for PWM synchronous channels update mode */
typedef enum {
	PWM_SYNC_UPDATE_MODE_0 = PWM_SCM_UPDM_MODE0,
	PWM_SYNC_UPDATE_MODE_1 = PWM_SCM_UPDM_MODE1,
	PWM_SYNC_UPDATE_MODE_2 = PWM_SCM_UPDM_MODE2
} pwm_sync_update_mode_t;

/** Definitions for PWM event */
typedef enum {
	PWM_EVENT_PERIOD_END = (0 << 10),      /* The channel counter event occurs at the end of the PWM period. */
	PWM_EVENT_PERIOD_HALF_END = (1 << 10)  /* The channel counter event occurs at the half of the PWM period. */
} pwm_counter_event_t;

/** Definitions for PWM fault input ID */
typedef enum {
#if (SAM3U)
	PWM_FAULT_MAINOSC = (1 << 0),
	PWM_FAULT_PWMFI2 = (1 << 1),
	PWM_FAULT_PWMFI0 = (1 << 2),
	PWM_FAULT_PWMFI1 = (1 << 3)
#elif (SAM3S || SAM4S || SAM4E)
	PWM_FAULT_PWMFI1 = (1 << 0),
	PWM_FAULT_MAINOSC = (1 << 1),
	PWM_FAULT_ADC = (1 << 2),
	PWM_FAULT_ACC = (1 << 3),
	PWM_FAULT_TIMER_0 = (1 << 4),
	PWM_FAULT_TIMER_1 = (1 << 5)
#elif (SAM3XA)
	PWM_FAULT_PWMFI0 = (1 << 0),
	PWM_FAULT_PWMFI1 = (1 << 1),
	PWM_FAULT_PWMFI2 = (1 << 2),
	PWM_FAULT_MAINOSC = (1 << 3),
	PWM_FAULT_ADC = (1 << 4),
	PWM_FAULT_TIMER_0 = (1 << 5)
#endif
} pwm_fault_id_t;

/** Definitions of PWM register group */
typedef enum {
	PWM_GROUP_CLOCK = (1 << 0),
	PWM_GROUP_DISABLE = (1 << 1),
	PWM_GROUP_MODE = (1 << 2),
	PWM_GROUP_PERIOD = (1 << 3),
	PWM_GROUP_DEAD_TIME = (1 << 4),
	PWM_GROUP_FAULT = (1 << 5)
} pwm_protect_reg_group_t;

/** Definitions for PWM comparison interrupt */
typedef enum {
	PWM_CMP_MATCH = 8,   /* Comparison unit match */
	PWM_CMP_UPDATE = 16  /* Comparison unit update */
} pwm_cmp_interrupt_t;

/** Definitions for PWM comparison unit */
typedef enum {
	PWM_CMP_UNIT_0 = 0,
	PWM_CMP_UNIT_1 = 1,
	PWM_CMP_UNIT_2 = 2,
	PWM_CMP_UNIT_3 = 3,
	PWM_CMP_UNIT_4 = 4,
	PWM_CMP_UNIT_5 = 5,
	PWM_CMP_UNIT_6 = 6,
	PWM_CMP_UNIT_7 = 7
} pmc_cmp_unit_t;

/** Definitions for PWM PDC transfer request mode */
typedef enum {
	PWM_PDC_UPDATE_PERIOD_ELAPSED = (0 << 20),  /* PDC transfer request is set as soon as the update period elapses. */
	PWM_PDC_COMPARISON_MATCH = (1 << 20)  /* PDC transfer request is set as soon as the selected comparison matches. */
} pwm_pdc_request_mode_t;

/** Definitions for PWM PDC transfer interrupt */
typedef enum {
	PWM_PDC_TX_END = (1 << 1),   /* PDC Tx end */
	PWM_PDC_TX_EMPTY = (1 << 2)  /* PDC Tx buffer empty */
} pwm_pdc_interrupt_t;

/** Definitions for PWM synchronous channels interrupt */
typedef enum {
	PWM_SYNC_WRITE_READY = (1 << 0),  /* Write Ready for Synchronous Channels Update */
	PWM_SYNC_UNDERRUN = (1 << 3)      /* Synchronous Channels Update Underrun Error */
} pwm_sync_interrupt_t;

#if SAM4E
typedef enum {
	PWM_SPREAD_SPECTRUM_MODE_TRIANGULAR = 0,
	PWM_SPREAD_SPECTRUM_MODE_RANDOM
} pwm_spread_spectrum_mode_t;
typedef enum {
	PWM_ADDITIONAL_EDGE_MODE_INC = PWM_CAE_ADEDGM_INC,
	PWM_ADDITIONAL_EDGE_MODE_DEC = PWM_CAE_ADEDGM_DEC,
	PWM_ADDITIONAL_EDGE_MODE_BOTH = PWM_CAE_ADEDGM_BOTH,
} pwm_additional_edge_mode_t;
#endif

/** Configurations of a PWM channel output */
typedef struct {
	/** Boolean of using override output as PWMH */
	bool b_override_pwmh;
	/** Boolean of using override output as PWML */
	bool b_override_pwml;
	/** Level of override output for PWMH */
	pwm_level_t override_level_pwmh;
	/** Level of override output for PWML */
	pwm_level_t override_level_pwml;
} pwm_output_t;

/** Configurations of PWM comparison */
typedef struct {
	/** Comparison unit number */
	uint32_t unit;
	/** Boolean of comparison enable */
	bool b_enable;
	/** Comparison value */
	uint32_t ul_value;
	/** Comparison mode */
	bool b_is_decrementing;
	/** Comparison trigger value */
	uint32_t ul_trigger;
	/** Comparison period value */
	uint32_t ul_period;
	/** Comparison update period value */
	uint32_t ul_update_period;
	/** Boolean of generating a match pulse on PWM event line 0 */
	bool b_pulse_on_line_0;
	/** Boolean of generating a match pulse on PWM event line 1 */
	bool b_pulse_on_line_1;
} pwm_cmp_t;

/** Configuration of PWM fault input behaviors */
typedef struct {
	/** Fault ID */
	pwm_fault_id_t fault_id;
	/** Polarity of fault input */
	pwm_level_t polarity;
	/** Boolean of clearing fault flag */
	bool b_clear;
	/** Boolean of fault filtering */
	bool b_filtered;
} pwm_fault_t;

/** Structure of PWM write-protect information */
typedef struct {
	/** Bitmask of PWM register group for write protect hardware status */
	uint32_t ul_hw_status;
	/** Bitmask of PWM register group for write protect software status */
	uint32_t ul_sw_status;
	/** Offset address of PWM register in which a write access has been attempted */
	uint32_t ul_offset;
} pwm_protect_t;
#endif /* (SAM3U || SAM3S || SAM3XA) */

/** Input parameters when configuring a PWM channel mode */
typedef struct {
	/** Channel number */
	uint32_t channel;
	/** Channel prescaler */
	uint32_t ul_prescaler;
    /** Channel alignment */
	pwm_align_t alignment;
    /** Channel initial polarity */
	pwm_level_t polarity;
	/** Duty Cycle Value */
	uint32_t ul_duty;
	/** Period Cycle Value */
	uint32_t ul_period;

#if (SAM3U || SAM3S || SAM3XA || SAM4S || SAM4E)
    /** Channel counter event */
	pwm_counter_event_t counter_event;
    /** Boolean of channel dead-time generator */
	bool b_deadtime_generator;
    /** Boolean of channel dead-time PWMH output inverted */
	bool b_pwmh_output_inverted;
    /** Boolean of channel dead-time PWML output inverted */
	bool b_pwml_output_inverted;
	/** Dead-time Value for PWMH Output */
	uint16_t us_deadtime_pwmh;
	/** Dead-time Value for PWML Output */
	uint16_t us_deadtime_pwml;
	/** Channel output */
	pwm_output_t output_selection;
	/** Boolean of Synchronous Channel */
	bool b_sync_ch;
	/** Fault ID of the channel */
	pwm_fault_id_t fault_id;
	/** Channel PWMH output level in fault protection */
	pwm_level_t ul_fault_output_pwmh;
	/** Channel PWML output level in fault protection */
	pwm_level_t ul_fault_output_pwml;
#endif /* (SAM3U || SAM3S || SAM3XA || SAM4S || SAM4E) */
#if SAM4E
	/** Spread Spectrum Value */
	uint32_t ul_spread;
	/** Spread Spectrum Mode */
	pwm_spread_spectrum_mode_t spread_spectrum_mode;
	/** Additional Edge Value */
	uint32_t ul_additional_edge;
	/** Additional Edge Mode */
	pwm_additional_edge_mode_t additional_edge_mode;
#endif
} pwm_channel_t;


uint32_t pwm_init(Pwm *p_pwm, pwm_clock_t *clock_config);
uint32_t pwm_channel_init(Pwm *p_pwm, pwm_channel_t *p_channel);
uint32_t pwm_channel_update_period(Pwm *p_pwm, pwm_channel_t *p_channel,
		uint32_t ul_period);
uint32_t pwm_channel_update_duty(Pwm *p_pwm, pwm_channel_t *p_channel,
		uint32_t ul_duty);
uint32_t pwm_channel_get_counter(Pwm *p_pwm, pwm_channel_t *p_channel);
void pwm_channel_enable(Pwm *p_pwm, uint32_t ul_channel);
void pwm_channel_disable(Pwm *p_pwm, uint32_t ul_channel);
uint32_t pwm_channel_get_status(Pwm *p_pwm);
uint32_t pwm_channel_get_interrupt_status(Pwm *p_pwm);
uint32_t pwm_channel_get_interrupt_mask(Pwm *p_pwm);
void pwm_channel_enable_interrupt(Pwm *p_pwm, uint32_t ul_event,
		uint32_t ul_fault);
void pwm_channel_disable_interrupt(Pwm *p_pwm, uint32_t ul_event,
		uint32_t ul_fault);

#if (SAM3U || SAM3S || SAM3XA || SAM4S || SAM4E)
void pwm_channel_update_output(Pwm *p_pwm, pwm_channel_t *p_channel,
		pwm_output_t *p_output, bool b_sync);
void pwm_channel_update_dead_time(Pwm *p_pwm, pwm_channel_t *p_channel,
		uint16_t us_deadtime_pwmh, uint16_t us_deadtime_pwml);

uint32_t pwm_fault_init(Pwm *p_pwm, pwm_fault_t *p_fault);
uint32_t pwm_fault_get_status(Pwm *p_pwm);
pwm_level_t pwm_fault_get_input_level(Pwm *p_pwm, pwm_fault_id_t id);
void pwm_fault_clear_status(Pwm *p_pwm, pwm_fault_id_t id);

uint32_t pwm_cmp_init(Pwm *p_pwm, pwm_cmp_t *p_cmp);
uint32_t pwm_cmp_change_setting(Pwm *p_pwm, pwm_cmp_t *p_cmp);
uint32_t pwm_cmp_get_period_counter(Pwm *p_pwm, uint32_t ul_cmp_unit);
uint32_t pwm_cmp_get_update_counter(Pwm *p_pwm, uint32_t ul_cmp_unit);
void pwm_cmp_enable_interrupt(Pwm *p_pwm, uint32_t ul_sources,
		pwm_cmp_interrupt_t type);
void pwm_cmp_disable_interrupt(Pwm *p_pwm, uint32_t ul_sources,
		pwm_cmp_interrupt_t type);
void pwm_pdc_set_request_mode(Pwm *p_pwm, pwm_pdc_request_mode_t request_mode,
		uint32_t ul_cmp_unit);

void pwm_pdc_enable_interrupt(Pwm *p_pwm, uint32_t ul_sources);
void pwm_pdc_disable_interrupt(Pwm *p_pwm, uint32_t ul_sources);

uint32_t pwm_sync_init(Pwm *p_pwm, pwm_sync_update_mode_t mode,
		uint32_t ul_update_period);
void pwm_sync_unlock_update(Pwm *p_pwm);
void pwm_sync_change_period(Pwm *p_pwm, uint32_t ul_update_period);
uint32_t pwm_sync_get_period_counter(Pwm * p_pwm);
void pwm_sync_enable_interrupt(Pwm *p_pwm, uint32_t ul_sources);
void pwm_sync_disable_interrupt(Pwm *p_pwm, uint32_t ul_sources);

void pwm_enable_protect(Pwm *p_pwm, uint32_t ul_group, bool b_sw);
void pwm_disable_protect(Pwm *p_pwm, uint32_t ul_group);
bool pwm_get_protect_status(Pwm *p_pwm, pwm_protect_t * p_protect);

uint32_t pwm_get_interrupt_status(Pwm *p_pwm);
uint32_t pwm_get_interrupt_mask(Pwm *p_pwm);
#endif /* (SAM3U || SAM3S || SAM3XA || SAM4S || SAM4E) */

#if (SAM3S || SAM3XA || SAM4S || SAM4E)
void pwm_stepper_motor_init(Pwm *p_pwm, pwm_stepper_motor_pair_t pair,
		bool b_enable_gray, bool b_down);
#endif /* (SAM3S || SAM3XA || SAM4S || SAM4E) */

#if SAM4E
void pwm_channel_update_spread(Pwm *p_pwm, pwm_channel_t *p_channel,
		uint32_t ul_spread);
void pwm_channel_update_additional_edge(Pwm *p_pwm, pwm_channel_t *p_channel,
		uint32_t ul_additional_edge,
		pwm_additional_edge_mode_t additional_edge_mode);
void pwm_channel_update_polarity_mode(Pwm *p_pwm, pwm_channel_t *p_channel,
		bool polarity_inversion_flag, pwm_level_t polarity_value);
#endif

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \page sam_pwm_quickstart Quickstart guide for SAM PWM module
 *
 * This is the quickstart guide for the \ref sam_drivers_pwm_group "PWM module",
 * with step-by-step instructions on how to configure and use the drivers in a
 * selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section pwm_basic_use_case Basic use case
 * In this basic use case, the PWM module is configured to:
 * - Output a square wave on PWM channel 0
 * - The frequency of the square wave is 1KHz, 50% duty cycle
 * - Clock A as the source clock
 * - The output wave can be checked on the selected output pin
 *
 * \section pwm_basic_use_case_setup Setup steps
 *
 * \subsection pwm_basic_use_case_setup_prereq Prerequisites
 *  - \ref group_pmc "Power Management Controller driver"
 *  - \ref gpio_group "General Purpose I/O Management (gpio)"
 *
 * \subsection pwm_basic_use_case_setup_code Example code
 * Add this PWM initialization code at the beginning of the main function:
 * \code
 *    pwm_channel_t pwm_channel_instance;
 *
 *    pmc_enable_periph_clk(ID_PWM);
 *
 *    pwm_channel_disable(PWM, PWM_CHANNEL_0);
 *
 *    pwm_clock_t clock_setting = {
 *        .ul_clka = 1000 * 100,
 *        .ul_clkb = 0,
 *        .ul_mck = 48000000
 *    };
 *    pwm_init(PWM, &clock_setting);
 *
 *    pwm_channel_instance.ul_prescaler = PWM_CMR_CPRE_CLKA;
 *    pwm_channel_instance.ul_period = 100;
 *    pwm_channel_instance.ul_duty = 50;
 *    pwm_channel_instance.channel = PWM_CHANNEL_0;
 *    pwm_channel_init(PWM, &pwm_channel_instance);
 * \endcode
 *
 * \subsection pwm_basic_use_case_setup_flow Workflow
 * -# Define the PWM channel instance in order to configure channel 0:
 *   - \code pwm_channel_t pwm_channel_instance; \endcode
 * -# Enable the module clock for the PWM peripheral:
 *   - \code pmc_enable_periph_clk(ID_PWM); \endcode
 * -# Disable PWM channel 0:
 *   - \code pwm_channel_disable(PWM, PWM_CHANNEL_0); \endcode
 * -# Setup clock for PWM module:
 *   - \code
 *    pwm_clock_t clock_setting = {
 *        .ul_clka = 1000 * 100,
 *        .ul_clkb = 0,
 *        .ul_mck = 48000000
 *    };
 *    pwm_init(PWM, &clock_setting);
 *   \endcode
 *   - \note 1. Only Clock A is configured (clock B is not used).
 * 2. The expected frequency is 1KHz, system main clock is assumed to be 48MHz.
 * -# Initialize channel instance and configure PWM channel 0, selecting clock A
 * as its source clock and setting the duty cycle at 50%:
 *   - \code
 *    pwm_channel_instance.ul_prescaler = PWM_CMR_CPRE_CLKA;
 *    pwm_channel_instance.ul_period = 100;
 *    pwm_channel_instance.ul_duty = 50;
 *    pwm_channel_instance.channel = PWM_CHANNEL_0;
 *    pwm_channel_init(PWM, &pwm_channel_instance);
 *   \endcode
 *   - \note 1. Period is left-aligned and output waveform starts at a low level.
 * 2. The pwm_channel_instance can be re-used to configure other PWM channels
 * after setting the required parameters.
 *
 * \section pwm_basic_use_case_usage Usage steps
 *
 * \subsection pwm_basic_use_case_usage_code Example code
 * Add to, e.g., main loop in application C-file:
 * \code
 *    pwm_channel_enable(PWM, PWM_CHANNEL_0);
 * \endcode
 *
 * \subsection pwm_basic_use_case_usage_flow Workflow
 * -# Enable PWM channel 0 and output square wave on this channel:
 *   - \code pwm_channel_enable(PWM, PWM_CHANNEL_0); \endcode
 *
 * \section pwm_use_cases Advanced use cases
 * For more advanced use of the pwm driver, see the following use cases:
 * - \subpage pwm_use_case_1 : PWM channel 0 outputs square wave and duty cycle
 *  is updated in the PWM ISR.
 */

/**
 * \page pwm_use_case_1 Use case #1
 *
 * In this use case, the PWM module is configured to:
 * - Output a square wave on PWM channel 0
 * - The frequency of the square wave is 1KHz
 * - The duty cycle is changed in the PWM ISR
 * - Clock A as the source clock
 * - The output wave can be checked on the selected output pin
 *
 * \section pwm_use_case_1_setup Setup steps
 *
 * \subsection pwm_use_case_1_setup_prereq Prerequisites
 *  - \ref group_pmc "Power Management Controller driver"
 *  - \ref gpio_group "General Purpose I/O Management (gpio)"
 *
 * \subsection pwm_use_case_1_setup_code Example code
 * Add to application C-file:
 * \code
 *    pwm_channel_t pwm_channel_instance;
 * \endcode
 *
 * \code
 *    void PWM_Handler(void)
 *    {
 *        static uint32_t ul_duty = 0;
 *        uint32_t ul_status;
 *        static uint8_t uc_countn = 0;
 *        static uint8_t uc_flag = 1;
 *
 *        ul_status = pwm_channel_get_interrupt_status(PWM);
 *        if ((ul_status & PWM_CHANNEL_0) == PWM_CHANNEL_0) {
 *            uc_count++;
 *            if (uc_count == 10) {
 *                if (uc_flag) {
 *                    ul_duty++;
 *                    if (ul_duty == 100) {
 *                        uc_flag = 0;
 *                    }
 *                } else {
 *                    ul_duty--;
 *                    if (ul_duty == 0) {
 *                        uc_flag = 1;
 *                    }
 *                }
 *                uc_count = 0;
 *                pwm_channel_instance.channel = PWM_CHANNEL_0;
 *                pwm_channel_update_duty(PWM, &pwm_channel_instance, ul_duty);
 *            }
 *        }
 *    }
 * \endcode
 *
 * \code
 *    pmc_enable_periph_clk(ID_PWM);
 *
 *    pwm_channel_disable(PWM, PWM_CHANNEL_0);
 *
 *    pwm_clock_t clock_setting = {
 *        .ul_clka = 1000 * 100,
 *        .ul_clkb = 0,
 *        .ul_mck = 48000000
 *    };
 *    pwm_init(PWM, &clock_setting);
 *
 *    pwm_channel_instance.ul_prescaler = PWM_CMR_CPRE_CLKA;
 *    pwm_channel_instance.ul_period = 100;
 *    pwm_channel_instance.ul_duty = 0;
 *    pwm_channel_instance.channel = PWM_CHANNEL_0;
 *    pwm_channel_init(PWM, &pwm_channel_instance);
 *
 *    pwm_channel_enable_interrupt(PWM, PWM_CHANNEL_0, 0);
 * \endcode
 *
 * \subsection pwm_use_case_1_setup_flow Workflow
 * -# Define the PWM channel instance in order to configure channel 0:
 *   - \code pwm_channel_t pwm_channel_instance; \endcode
 * -# Define the PWM interrupt handler in the application:
 *   - \code void PWM_Handler(void); \endcode
 * -# In PWM_Handler(), get PWM interrupt status:
 *   - \code ul_status = pwm_channel_get_interrupt_status(PWM); \endcode
 * -# In PWM_Handler(), check whether the PWM channel 0 interrupt has occurred:
 *   - \code
 *    if ((ul_status & PWM_CHANNEL_0) == PWM_CHANNEL_0) {
 *    }
 *   \endcode
 * -# In PWM_Handler(), if the PWM channel 0 interrupt has occurred, update the ul_duty value:
 *   - \code
 *    uc_count++;
 *    if (uc_count == 10) {
 *        if (uc_flag) {
 *            ul_duty++;
 *            if (ul_duty >= 100) {
 *                uc_flag = 0;
 *            }
 *        } else {
 *            ul_duty--;
 *            if (ul_duty == 0) {
 *                uc_flag = 1;
 *            }
 *        }
 *    }
 *   \endcode
 * -# In PWM_Handler(), if the ul_duty value has been updated, change the square wave duty:
 *   - \code
 *    pwm_channel_instance.channel = PWM_CHANNEL_0;
 *    pwm_channel_update_duty(PWM, &pwm_channel_instance, ul_duty);
 *   \endcode
 * -# Enable the PWM clock:
 *   - \code pmc_enable_periph_clk(ID_PWM); \endcode
 * -# Disable PWM channel 0:
 *   - \code pwm_channel_disable(PWM, PWM_CHANNEL_0); \endcode
 * -# Setup clock for PWM module:
 *   - \code
 *    pwm_clock_t clock_setting = {
 *        .ul_clka = 1000 * 100,
 *        .ul_clkb = 0,
 *        .ul_mck = 48000000
 *    };
 *    pwm_init(PWM, &clock_setting);
 *   \endcode
 *   - \note 1. Only Clock A is configured (clock B is not used).
 * 2. The expected frequency is 1KHz, system main clock is assumed to be 48Mhz.
 * -# Initialize channel instance and configure PWM channel 0, selecting clock A
 * as its source clock and setting the initial ducy as 0%:
 *   - \code
 *    pwm_channel_instance.ul_prescaler = PWM_CMR_CPRE_CLKA;
 *    pwm_channel_instance.ul_period = 100;
 *    pwm_channel_instance.ul_duty = 0;
 *    pwm_channel_instance.channel = PWM_CHANNEL_0;
 *    pwm_channel_init(PWM, &pwm_channel_instance);
 *   \endcode
 *   - \note 1. Period is left-aligned and output waveform starts at a low level.
 * 2. The pwm_channel_instance can be re-used to configure other PWM channels
 * after setting the required parameters.
 * -# Enable channel 0 interrupt:
 *   - \code pwm_channel_enable_interrupt(PWM, PWM_CHANNEL_0, 0); \endcode
 *   - \note 1.In order to enable the PWM interrupt, the NVIC must be configured
 * to enable the PWM interrupt. 2. When the channel 0 counter reaches the channel
 * period, the interrupt (counter event) will occur.
 *
 * \section pwm_use_case_1_usage Usage steps
 *
 * \subsection pwm_use_case_1_usage_code Example code
 * \code
 *    pwm_channel_enable(PWM, PWM_CHANNEL_0);
 * \endcode
 *
 * \subsection pwn_use_case_1_usage_flow Workflow
 * -# Enable PWM channel 0 and output square wave on this channel:
 *   - \code pwm_channel_enable(PWM, PWM_CHANNEL_0); \endcode
 *
 */

#endif /* PWM_H_INCLUDED */
