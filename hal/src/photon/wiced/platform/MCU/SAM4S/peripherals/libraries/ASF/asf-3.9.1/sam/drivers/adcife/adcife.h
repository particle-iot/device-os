/**
 * \file
 *
 * \brief Analog-to-Digital Converter driver for SAM4L.
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

#ifndef ADC_H_INCLUDED
#define ADC_H_INCLUDED

#include "compiler.h"
#include "pdca.h"
#include "sysclk.h"
#include "sleepmgr.h"
#include "conf_adcife.h"
#include "status_codes.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/** Time out value (number of attempts) */
#define ADC_NUM_OF_ATTEMPTS                  10000

/** Definitions for ADC trigger */
enum adc_trigger_t {
	/* Starting a conversion is only possible by software. */
	ADC_TRIG_SW = 0,
	/* Internal Timer */
	ADC_TRIG_INTL_TIMER,
	/* Internal Trig Source */
	ADC_TRIG_INTL_TRIG_SRC,
	/* Countious Mode */
	ADC_TRIG_CON,
	/* External trigger pin rising edge */
	ADC_TRIG_EXT_PIN_RISE,
	/* External trigger pin falling edge */
	ADC_TRIG_EXT_PIN_FALL,
	/* External trigger pin both edges */
	ADC_TRIG_EXT_PIN_BOTH
};

/** Definitions for ADC gain */
enum adc_gain_t {
	ADC_GAIN_1X = 0,
	ADC_GAIN_2X,
	ADC_GAIN_4X,
	ADC_GAIN_8X,
	ADC_GAIN_16X,
	ADC_GAIN_32X,
	ADC_GAIN_64X,
	ADC_GAIN_HALF
};

/**
 * \name Prescaler Rate Slection
 * @{
 */
#define ADC_PRESCAL_DIV4      0
#define ADC_PRESCAL_DIV8      1
#define ADC_PRESCAL_DIV16     2
#define ADC_PRESCAL_DIV32     3
#define ADC_PRESCAL_DIV64     4
#define ADC_PRESCAL_DIV128    5
#define ADC_PRESCAL_DIV256    6
#define ADC_PRESCAL_DIV512    7
 /** @} */

/**
 * \name Clock Selection for sequencer/ADC cell
 * @{
 */
#define ADC_CLKSEL_GCLK      0
#define ADC_CLKSEL_APBCLK    1
 /** @} */

/**
 * \name ADC current reduction
 * @{
 */
#define ADC_SPEED_300K      0
#define ADC_SPEED_225K      1
#define ADC_SPEED_150K      2
#define ADC_SPEED_75K       3
 /** @} */

/**
 * \name ADC Reference selection
 * @{
 */
#define ADC_REFSEL_0      0
#define ADC_REFSEL_1      1
#define ADC_REFSEL_2      2
#define ADC_REFSEL_3      3
#define ADC_REFSEL_4      4
 /** @} */

/**
 * \name Zoom shift/unipolar reference source selection
 * @{
 */
#define ADC_ZOOMRANGE_0      0
#define ADC_ZOOMRANGE_1      1
#define ADC_ZOOMRANGE_2      2
#define ADC_ZOOMRANGE_3      3
#define ADC_ZOOMRANGE_4      4
 /** @} */

/**
 * \name MUX selection on Negative ADC input channel
 * @{
 */
#define ADC_MUXNEG_0      0
#define ADC_MUXNEG_1      1
#define ADC_MUXNEG_2      2
#define ADC_MUXNEG_3      3
#define ADC_MUXNEG_4      4
#define ADC_MUXNEG_5      5
#define ADC_MUXNEG_6      6
#define ADC_MUXNEG_7      7
 /** @} */

/**
 * \name MUX selection on Positive ADC input channel
 * @{
 */
#define ADC_MUXPOS_0      0
#define ADC_MUXPOS_1      1
#define ADC_MUXPOS_2      2
#define ADC_MUXPOS_3      3
#define ADC_MUXPOS_4      4
#define ADC_MUXPOS_5      5
#define ADC_MUXPOS_6      6
#define ADC_MUXPOS_7      7
#define ADC_MUXPOS_8      8
#define ADC_MUXPOS_9      9
#define ADC_MUXPOS_10    10
#define ADC_MUXPOS_11    11
#define ADC_MUXPOS_12    12
#define ADC_MUXPOS_13    13
#define ADC_MUXPOS_14    14
#define ADC_MUXPOS_15    15
 /** @} */

/**
 * \name Internal Voltage Sources Selection
 * @{
 */
#define ADC_INTERNAL_0      0
#define ADC_INTERNAL_1      1
#define ADC_INTERNAL_2      2
#define ADC_INTERNAL_3      3
 /** @} */

/**
 * \name Resolution
 * @{
 */
#define ADC_RES_8_BIT      1
#define ADC_RES_12_BIT     0
 /** @} */

/**
 * \name Gain Compensation
 * @{
 */
#define ADC_GCOMP_EN      1
#define ADC_GCOMP_DIS     0
 /** @} */

/**
 * \name Bipolar Mode
 * @{
 */
#define ADC_BIPOLAR_SINGLEENDED      0
#define ADC_BIPOLAR_DIFFERENTIAL     1
 /** @} */

/**
 * \name Half Word Left Adjust
 * @{
 */
#define ADC_HWLA_EN      1
#define ADC_HWLA_DIS     0
 /** @} */

/**
 * \name Window Monitor Mode
 * @{
 */
#define ADC_WM_OFF         0
#define ADC_WM_MODE_1      1
#define ADC_WM_MODE_2      2
#define ADC_WM_MODE_3      3
#define ADC_WM_MODE_4      4
 /** @} */

/** ADC interrupt source type */
typedef enum adc_interrupt_source {
	ADC_SEQ_SEOC = ADCIFE_IER_SEOC,
	ADC_SEQ_LOVR = ADCIFE_IER_LOVR,
	ADC_WINDOW_MONITOR = ADCIFE_IER_WM,
	ADC_SEQ_MTRG = ADCIFE_IER_SMTRG,
	ADC_TTO = ADCIFE_IER_TTO,
} adc_interrupt_source_t;

/** ADC Configuration structure. */
struct adc_config {
	/** Prescaler Rate Slection */
	uint8_t prescal;
	/** Clock Selection for sequencer/ADC cell */
	bool clksel;
	/** ADC current reduction */
	uint8_t speed;
	/** ADC Reference selection */
	uint8_t refsel;
	/** Startup time */
	uint8_t start_up;
};

/** Parameters for the configuration of the Sequencer. */
struct adc_seq_config {
	/** Half Word Left Adjust */
	uint32_t hwla:1;
	uint32_t reserved:1;
	/** conversion mode */
	uint32_t bipolar:1;
	uint32_t reserved2:1;
	/** gain value */
	uint32_t gain:3;
	/** Gain Compensation */
	uint32_t gcomp:1;
	/** Trigger selection. */
	uint32_t trgsel:3;
	uint32_t reserved3:1;
	/** Sequencer resolution. */
	uint32_t res:1;
	uint32_t reserved4:1;
	/** Internal Voltage Sources Selection */
	uint32_t internal:2;
	/** MUX selection on Positive ADC input channel. */
	uint32_t muxpos:4;
	/** MUX selection on Negative ADC input channel. */
	uint32_t muxneg:3;
	uint32_t reserved5:5;
	/** Zoom shift/unipolar reference source selection. */
	uint32_t zoomrange:3;
	uint32_t reserved6:1;
};

/** Parameters for the configuration of the Sequencer. */
struct adc_ch_config {
	struct adc_seq_config *seq_cfg;
	/** Internal Timer Max Counter */
	uint16_t internal_timer_max_count;
	/** Window Monitor Mode */
	uint8_t window_mode;
	/** LowThreshold value */
	uint16_t low_threshold;
	/** HighThreshold value */
	uint16_t high_threshold;
};

/** Parameters for the configuration of the First CDMA register. */
struct adc_cdma_first_config {
	/** Half Word Left Adjust */
	uint32_t hwla:1;
	uint32_t reserved:1;
	/** conversion mode */
	uint32_t bipolar:1;
	/** Software trigger */
	uint32_t strig:1;
	/** gain value */
	uint32_t gain:3;
	/** Gain Compensation */
	uint32_t gcomp:1;
	/** Enables the Startup Time */
	uint32_t enstup:1;
	uint32_t reserved2:3;
	/** Sequencer resolution. */
	uint32_t res:1;
	/** Internal Timer Start */
	uint32_t tss:1;
	/** Internal Voltage Sources Selection */
	uint32_t internal:2;
	/**MUX selection on Positive ADC input channel. */
	uint32_t muxpos:4;
	/**MUX selection on Negative ADC input channel. */
	uint32_t muxneg:3;
	uint32_t reserved3:5;
	/** Zoom shift/unipolar reference source selection. */
	uint32_t zoomrange:3;
	/** Double Word transmitting */
	uint32_t dw:1;
};

/** Parameters for the configuration of the Second CDMA register. */
struct adc_cdma_second_config {
	/** Low Threshold. */
	uint32_t lt:12;
	/** Window Monitor Mode. */
	uint32_t wm:3;
	uint32_t reserved:1;
	/** High Threshold. */
	uint32_t ht:12;
	uint32_t reserved2:3;
	/** Double Word transmitting */
	uint32_t dw:1;
};

/** Parameters for the configuration of the PDCA word. */
struct adc_cdma_config {
	/** Fisrt CDMA word */
	struct adc_cdma_first_config adc_cdma_first_cfg;
	/** Second CDMA word */
	struct adc_cdma_second_config adc_cdma_second_cfg;
};
/** Parameters for the configuration of the PDCA word. */
struct adc_pdca_config {
	/** Window Mode */
	bool wm;
	/** Number of channels */
	uint8_t nb_channels;
	/** ADC Buffer used for PDC transfer */
	uint16_t *buffer;
	/** ADC PDC RX Channel */
	uint8_t pdc_rx_channel;
	/** ADC PDC TX Channel */
	uint8_t pdc_tx_channel;
	/** CDMA word */
	void *cdma_cfg;
};

typedef void (*adc_callback_t) (void);

/**
 * \brief ADC driver software instance structure.
 *
 * Device instance structure for a ADC driver instance. This
 * structure should be initialized by the \ref adc_init() function to
 * associate the instance with a particular hardware module of the device.
 */
struct adc_dev_inst {
	/** Base address of the ADC module. */
	Adcife *hw_dev;
	/** Pointer to ADC configuration structure. */
	struct adc_config  *adc_cfg;
};

status_code_t adc_init(struct adc_dev_inst *const dev_inst, Adcife *const adc,
		struct adc_config *const cfg);

/**
 * \brief Initializes an ADC module configuration structure to
 * defaults.
 *
 *  Initializes a given ADC module configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li ADC Prescaler initialized to \ref ADC_PRESCAL_DIV16
 *   \li ADC Clock Sel initialized to \ref ADC_CLKSEL_APBCLK
 *   \li ADC Speed Clock initialized to \ref ADC_SPEED_150K
 *   \li ADC Reference Selection initialized to \ref ADC_REFSEL_1
 *   \li ADC Startup Value initialized to \ref CONFIG_ADC_STARTUP
 *
 * \param cfg  ADC module configuration structure to initialize to default value
 */
static inline void adc_get_config_defaults(struct adc_config *const cfg)
{
	/* Sanity check argument */
	Assert(cfg);

	cfg->prescal  = ADC_PRESCAL_DIV16;
	cfg->clksel   = ADC_CLKSEL_APBCLK;
	cfg->speed    = ADC_SPEED_150K;
	cfg->refsel   = ADC_REFSEL_1;
	cfg->start_up = CONFIG_ADC_STARTUP;
}

void adc_set_config(struct adc_dev_inst *const dev_inst,
		struct adc_config *cfg);

/**
 * \brief Initializes an ADC channel configuration structure to
 * defaults.
 *
 *  Initializes a given ADC channel configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li Hardware Left Adjustment Disabled
 *   \li Single Ended Mode Selected
 *   \li Gain x1 Selected
 *   \li Gain Compensation Disabled
 *   \li Software Trigger Selected
 *   \li 12-bits resolution Selected
 *   \li Enable the internal voltage sources
 *   \li Mus Pos 2 / Mux Neg 1 selected
 *   \li All modes except zoom and unipolar with hysteresis
 *
 * \param cfg  ADC channel configuration structure to initialize to default value
 */
static inline void adc_ch_get_config_defaults(struct adc_ch_config *const cfg)
{
	/* Sanity check argument */
	Assert(cfg);

	cfg->seq_cfg->hwla        = ADC_HWLA_DIS;
	cfg->seq_cfg->bipolar     = ADC_BIPOLAR_SINGLEENDED;
	cfg->seq_cfg->gain        = ADC_GAIN_1X;
	cfg->seq_cfg->gcomp       = ADC_GCOMP_DIS;
	cfg->seq_cfg->trgsel      = ADC_TRIG_SW;
	cfg->seq_cfg->res         = ADC_RES_12_BIT;
	cfg->seq_cfg->internal    = ADC_INTERNAL_3;
	cfg->seq_cfg->muxpos      = ADC_MUXPOS_2;
	cfg->seq_cfg->muxneg      = ADC_MUXNEG_1;
	cfg->seq_cfg->zoomrange   = ADC_ZOOMRANGE_0;
	cfg->internal_timer_max_count = 60;
	cfg->window_mode              = 0;
	cfg->low_threshold            = 0;
	cfg->high_threshold           = 0;
}

void adc_ch_set_config(struct adc_dev_inst *const dev_inst,
		struct adc_ch_config *cfg);

/**
 * \brief Initializes an ADC PDCA configuration structure to
 * defaults.
 *
 *  Initializes a given ADC PDCA configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 * \param cfg  ADC PDCA configuration structure to initialize to default value
 */
static inline void adc_pdca_get_config_defaults(struct adc_pdca_config *const cfg)
{
	/* Sanity check argument */
	Assert(cfg);

	cfg->wm                       = false;
	cfg->nb_channels              = 0;
	cfg->buffer                   = NULL;
	cfg->pdc_rx_channel           = CONFIG_ADC_PDCA_RX_CHANNEL;
	cfg->pdc_rx_channel           = CONFIG_ADC_PDCA_TX_CHANNEL;

	cfg->cdma_cfg                 = NULL;
}

void adc_pdca_set_config(struct adc_pdca_config *cfg);

void adc_set_callback(struct adc_dev_inst *const dev_inst,
		adc_interrupt_source_t source, adc_callback_t callback,
		uint8_t irq_line, uint8_t irq_level);

status_code_t adc_enable(struct adc_dev_inst *const dev_inst);
void adc_disable(struct adc_dev_inst *const dev_inst);

/**
 * \brief Configure ADC trigger source with specified value.
 *
 * \param dev_inst    Device structure pointer.
 * \param trigger trigger source selection.
 *
 */
static inline void adc_configure_trigger(struct adc_dev_inst *const dev_inst,
		const enum adc_trigger_t trigger)
{
	dev_inst->hw_dev->ADCIFE_SEQCFG |= ADCIFE_SEQCFG_TRGSEL(trigger);
}

/**
 * \brief Configure ADC gain with specified value.
 *
 * \param dev_inst    Device structure pointer.
 * \param gain gain value.
 *
 */
static inline void adc_configure_gain(struct adc_dev_inst *const dev_inst,
		const enum adc_gain_t gain)
{
	dev_inst->hw_dev->ADCIFE_SEQCFG |= ADCIFE_SEQCFG_GAIN(gain);
}

/**
 * \brief Start the ADC internal timer.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline void adc_start_itimer(struct adc_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->ADCIFE_CR = ADCIFE_CR_TSTART;
}

/**
 * \brief Stop the ADC internal timer.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline void adc_stop_itimer(struct adc_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->ADCIFE_CR = ADCIFE_CR_TSTOP;
}

/**
 * \brief Configure ADC internal timer with specified value.
 *
 * \param dev_inst    Device structure pointer.
 * \param period Internal Timer Max Counter
 *
 */
static inline void adc_configure_itimer_period(struct adc_dev_inst *const dev_inst,
		const uint32_t period)
{
	dev_inst->hw_dev->ADCIFE_ITIMER = ADCIFE_ITIMER_ITMC(period);
}

/**
 * \brief Check if internal timer is busy.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline bool adc_is_busy_itimer(struct adc_dev_inst *const dev_inst)
{
	return (dev_inst->hw_dev->ADCIFE_SR & ADCIFE_SR_TBUSY);
}

/**
 * \brief Configure ADC window monitor mode with specified value.
 *
 * \param dev_inst    Device structure pointer.
 * \param mode Window Monitor Mode.
 *
 */
static inline void adc_configure_wm_mode(struct adc_dev_inst *const dev_inst,
		const uint8_t mode)
{
	dev_inst->hw_dev->ADCIFE_WCFG = ADCIFE_WCFG_WM(mode);
}

/**
 * \brief Get ADC window monitor mode.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline uint8_t adc_get_wm_mode(struct adc_dev_inst *const dev_inst)
{
	return ((dev_inst->hw_dev->ADCIFE_WCFG & ADCIFE_WCFG_WM_Msk) >> ADCIFE_WCFG_WM_Pos);
}

/**
 * \brief Configure ADC window monitor threshold with specified value.
 *
 * \param dev_inst    Device structure pointer.
 * \param low_threshold Low Threshold value.
 * \param high_threshold HighThreshold value.
 *
 */
static inline void adc_configure_wm_threshold(struct adc_dev_inst *const dev_inst,
		const uint16_t low_threshold, const uint16_t high_threshold)
{
	dev_inst->hw_dev->ADCIFE_WTH = ADCIFE_WTH_LT(low_threshold) |
			ADCIFE_WTH_HT(high_threshold);
}

/**
 * \brief Configure ADC calibration mode with specified value.
 *
 * \param dev_inst    Device structure pointer.
 * \param fcd Flash Calibration Done.
 * \param biassel Select bias mode.
 * \param biascal Bias calibration.
 * \param calib Calibration value.
 *
 */
static inline void adc_configure_calibration(struct adc_dev_inst *const dev_inst,
		const bool fcd, const bool biassel, const uint8_t biascal,
		const uint8_t calib)
{
	dev_inst->hw_dev->ADCIFE_CALIB =
			ADCIFE_CALIB_CALIB(calib) |
			ADCIFE_CALIB_BIASCAL(biascal);
	if (fcd) {
		dev_inst->hw_dev->ADCIFE_CALIB |= ADCIFE_CALIB_FCD;
	}
	if (biassel) {
		dev_inst->hw_dev->ADCIFE_CALIB |= ADCIFE_CALIB_BIASSEL;
	}
}

/**
 * \brief Start ADC software trigger conversion.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline void adc_start_software_conversion(struct adc_dev_inst *const dev_inst)
{
	dev_inst->hw_dev->ADCIFE_CR |= ADCIFE_CR_STRIG;
}

/**
 * \brief Get last ADC converted value.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline uint16_t adc_get_last_conv_value(struct adc_dev_inst *const dev_inst)
{
	return (dev_inst->hw_dev->ADCIFE_LCV & ADCIFE_LCV_LCV_Msk);
}

/**
 * \brief Get last converted ADC negative channel.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline uint8_t adc_get_last_conv_nchannel(struct adc_dev_inst *const dev_inst)
{
	return ((dev_inst->hw_dev->ADCIFE_LCV & ADCIFE_LCV_LCNC_Msk) >>
			ADCIFE_LCV_LCNC_Pos);
}

/**
 * \brief Get last ADC converted positive channel.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline uint8_t adc_get_last_conv_pchannel(struct adc_dev_inst *const dev_inst)
{
	return ((dev_inst->hw_dev->ADCIFE_LCV & ADCIFE_LCV_LCPC_Msk) >> ADCIFE_LCV_LCPC_Pos);
}

/**
 * \brief Get ADC status.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline uint32_t adc_get_status(struct adc_dev_inst *const dev_inst)
{
	return dev_inst->hw_dev->ADCIFE_SR;
}

/**
 * \brief Clear ADC status.
 *
 * \param dev_inst    Device structure pointer.
 * \param status_flags status flag.
 *
 */
static inline void adc_clear_status(struct adc_dev_inst *const dev_inst,
		const uint32_t status_flags)
{
	dev_inst->hw_dev->ADCIFE_SCR = status_flags;
}

/**
 * \brief Enable ADC interrupt.
 *
 * \param dev_inst    Device structure pointer.
 * \param interrupt_source interrupt source.
 *
 */
static inline void adc_enable_interrupt(struct adc_dev_inst *const dev_inst,
		const adc_interrupt_source_t interrupt_source)
{
	dev_inst->hw_dev->ADCIFE_IER = interrupt_source;
}

/**
 * \brief Disable ADC interrupt.
 *
 * \param dev_inst    Device structure pointer.
 * \param interrupt_source interrupt source.
 *
 */
static inline void adc_disable_interrupt(struct adc_dev_inst *const dev_inst,
		const adc_interrupt_source_t interrupt_source)
{
	dev_inst->hw_dev->ADCIFE_IDR = interrupt_source;
}

/**
 * \brief Get ADC interrupt mask.
 *
 * \param dev_inst    Device structure pointer.
 *
 */
static inline adc_interrupt_source_t adc_get_interrupt_mask(struct adc_dev_inst *const dev_inst)
{
	return (adc_interrupt_source_t)dev_inst->hw_dev->ADCIFE_IMR;
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \page sam_adcife_quickstart Quickstart guide for SAM ADCIFE driver
 *
 * This is the quickstart guide for the \ref sam_drivers_adcife_group "SAM ADCIFE driver",
 * with step-by-step instructions on how to configure and use the driver in a
 * selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section adcife_basic_use_case Basic use case
 * In this basic use case, the ADCIFE module and single channel are configured for:
 * - 12-bit, unsigned conversions
 * - Internal bandgap as 3.3 V reference
 * - ADC clock rate of at most 1.8 MHz and maximum sample rate is 300 KHz
 * - Software triggering of conversions
 * - Interrupt-based conversion handling
 * - Single channel measurement
 * - ADC_CHANNEL_13 as positive input
 *
 * \subsection sam_adcife_quickstart_prereq Prerequisites
 * -# \ref sysclk_group "System Clock Management (Sysclock)"
 *
 * \section adcife_basic_use_case_setup Setup steps
 * \subsection adcife_basic_use_case_setup_code Example code
 * Add to application C-file:
 * \code
 *   void adcife_read_conv_result(void)
 *   {
 *       // Check the ADC conversion status
 *      if ((adc_get_status(&g_adc_inst) & ADCIFE_SR_SEOC) == ADCIFE_SR_SEOC){
 *           g_adc_sample_data[0] = adc_get_last_conv_value(&g_adc_inst);
 *           adc_clear_status(&g_adc_inst, ADCIFE_SCR_SEOC);
 *       }
 *   }
 *   void adc_setup(void)
 *   {
 *        adc_init(&g_adc_inst, ADCIFE, &adc_cfg);
 *        adc_enable(&g_adc_inst);
 *        adc_ch_set_config(&g_adc_inst, &adc_ch_cfg);
 *        adc_set_callback(&g_adc_inst, ADC_SEQ_SEOC, adcife_read_conv_result,
 *             ADCIFE_IRQn, 1);
 *   }
 * \endcode
 *
 * \subsection adcife_basic_use_case_setup_flow Workflow
 * -# Define the interrupt service handler in the application:
 *   - \code
 *   void adcife_read_conv_result(void)
 *   {
 *       // Check the ADC conversion status
 *      if ((adc_get_status(&g_adc_inst) & ADCIFE_SR_SEOC) == ADCIFE_SR_SEOC){
 *           g_adc_sample_data[0] = adc_get_last_conv_value(&g_adc_inst);
 *           adc_clear_status(&g_adc_inst, ADCIFE_SCR_SEOC);
 *       }
 *   }
 * \endcode
 *   - \note Get ADCIFE status and check if the conversion is finished. If done,
 *      read the last ADCIFE result data.
 * -# Initialize ADC Module:
 *   - \code adc_init(&g_adc_inst, ADCIFE, &adc_cfg); \endcode
 * -# Enable ADC Module:
 *   - \code adc_enable(&g_adc_inst); \endcode
 * -# Configure ADC single sequencer with specified value.:
 *   - \code  adc_ch_set_config(&g_adc_inst, &adc_ch_cfg); \endcode
 * -# Set callback for ADC:
 *   - \code  adc_set_callback(&g_adc_inst, ADC_SEQ_SEOC, adcife_read_conv_result,
 *                  ADCIFE_IRQn, 1); \endcode
 *
 * \section adcife_basic_use_case_usage Usage steps
 * \subsection adcife_basic_use_case_usage_code Example code
 * Add to, e.g., main loop in application C-file:
 * \code
 *    adc_start_software_conversion(&g_adc_inst);;
 * \endcode
 *
 * \subsection adcife_basic_use_case_usage_flow Workflow
 * -# Start ADC conversion on channel:
 *   - \code adc_start_software_conversion(&g_adc_inst); \endcode
 */
#endif /* ADC_H_INCLUDED */
