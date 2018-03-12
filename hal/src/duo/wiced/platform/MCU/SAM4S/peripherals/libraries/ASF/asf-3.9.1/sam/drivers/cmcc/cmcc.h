/**
 *
 * \file
 *
 * \brief CMCC software driver for SAM.
 *
 * This file defines a useful set of functions for the CMCC on SAM devices.
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

#ifndef CMCC_H_INCLUDED
#define CMCC_H_INCLUDED

/**
 * \defgroup group_sam_drivers_cmcc CMCC - Cortex M Cache Controller module
 *
 * The Cortex M Cache Controller (CMCC) is a 4-way set associative unified
 * cache controller.
 * It integrates a controller, a tag directory, data memory, metadata memory
 * and a configuration interface.
 *
 * @{
 */

#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Cache Controller Monitor Counter Mode */
enum cmcc_monitor_mode {
	CMCC_CYCLE_COUNT_MODE = 0,
	CMCC_IHIT_COUNT_MODE,
	CMCC_DHIT_COUNT_MODE,
};

/** CMCC Configuration structure. */
struct cmcc_config {
	/* false = cache controller monitor disable, true = cache controller
	 *monitor enable */
	bool cmcc_monitor_enable;
	/* Cache Controller Monitor Counter Mode */
	enum cmcc_monitor_mode cmcc_mcfg_mode;
};

void cmcc_get_config_defaults(struct cmcc_config *const cfg);

bool cmcc_init(Cmcc *const p_cmcc, struct cmcc_config *const cfg);

void cmcc_enable(Cmcc *const p_cmcc);

void cmcc_invalidate_line(Cmcc *const p_cmcc, uint32_t cmcc_way,
		uint32_t cmcc_index);

/**
 * \brief Configure the CMCC.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 * \param cfg Pointer to CMCC configuration.
 */
static inline void cmcc_set_config(Cmcc *const p_cmcc,
		struct cmcc_config *const cfg)
{
	p_cmcc->CMCC_MCFG = cfg->cmcc_mcfg_mode;
}

/**
 * \brief Enable Clock gating.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_enable_clock_gating(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_CFG &= ~CMCC_CFG_GCLKDIS;
}

/**
 * \brief Disable Clock gating.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_disable_clock_gating(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_CFG |= CMCC_CFG_GCLKDIS;
}

/**
 * \brief Enable Cache Controller monitor.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_enable_monitor(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_MEN |= CMCC_MEN_MENABLE;
}

/**
 * \brief Disable Cache Controller monitor.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_disable_monitor(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_MEN &= ~CMCC_MEN_MENABLE;
}

/**
 * \brief Reset event counter register.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_reset_monitor(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_MCTRL = CMCC_MCTRL_SWRST;
}

/**
 * \brief Get the Cache Controller status.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 * \return the content of the status register.
 *
 */
static inline uint32_t cmcc_get_status(Cmcc *const p_cmcc)
{
	return p_cmcc->CMCC_SR;
}

/**
 * \brief Get the Cache Controller monitor status.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 * \return the content of the status register.
 *
 */
static inline uint32_t cmcc_get_monitor_cnt(Cmcc *const p_cmcc)
{
	return p_cmcc->CMCC_MSR;
}

/**
 * \brief Disable Cache Controller.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_disable(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_CTRL = 0;
}

/**
 * \brief Cache Controller Invalidate All.
 *
 * \param p_cmcc Pointer to an CMCC instance.
 *
 */
static inline void cmcc_invalidate_all(Cmcc *const p_cmcc)
{
	p_cmcc->CMCC_MAINT0 = CMCC_MAINT0_INVALL;
}

/** @} */

#ifdef __cplusplus
}
#endif

/**
 * \page sam_cmcc_quick_start Quick Start Guide for the CMCC driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_cmcc, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.The code examples can be copied into e.g the main
 * application loop or any other function that will need to control the
 * CMCC module.
 *
 * \section cmcc_qs_use_cases Use cases
 * - \ref cmcc_basic
 *
 * \section cmcc_basic CMCC basic usage
 *
 * This use case will demonstrate how to initialize the CMCC module.
 *
 *
 * \section cmcc_basic_setup Setup steps
 *
 * \subsection cmcc_basic_setup_code
 *
 * Add this to the main loop or a setup function:
 * \code
 *  struct cmcc_config   g_cmcc_cfg;
 *  cmcc_get_config_defaults(&g_cmcc_cfg);
 *  cmcc_init(CMCC, &g_cmcc_cfg);
 *  cmcc_enable(CMCC);
 * \endcode
 *
 * \subsection cmcc_basic_setup_workflow
 *
 * -# Enable the CMCC module
 * \code 
 *  cmcc_enable(CMCC); 
 * \endcode
 *
 * -# Initialize the CMCC to Data hit counter mode
 * \code
 *  g_cmcc_cfg->cmcc_mcfg_mode = CMCC_DHIT_COUNT_MODE;
 *  cmcc_set_config(CMCC,&g_cmcc_cfg);
 *  cmcc_enable_monitor(CMCC);
 * \endcode
 *
 * \section cmcc_basic_usage Usage steps
 *
 * \subsection cmcc_basic_usage_code
 *
 * We can then get the count of Cache Monitor Event by
 * \code
 *  cmcc_get_monitor_cnt(CMCC);
 * \endcode
 */

#endif  /* CMCC_H_INCLUDED */
