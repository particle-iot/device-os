/**
 * \file
 *
 * \brief HMATRIX driver for SAM.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
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

#ifndef __HMATRIX_H_INCLUDED__
#define __HMATRIX_H_INCLUDED__

#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define hmatrix_prs HmatrixbPrs

/** \brief HMATRIX master: undefined length burst type */
enum hmatrix_burst_type {
	HMATRIX_BURST_INFINITE_LENGTH = HMATRIXB_MCFG_ULBT(0),
	HMATRIX_BURST_SINGLE_ACCESS   = HMATRIXB_MCFG_ULBT(1),
	HMATRIX_BURST_FOUR_BEAT       = HMATRIXB_MCFG_ULBT(2),
	HMATRIX_BURST_EIGHT_BEAT      = HMATRIXB_MCFG_ULBT(3),
	HMATRIX_BURST_SIXTEEN_BEAT    = HMATRIXB_MCFG_ULBT(4)
};

/** \brief HMATRIX slave: default master type */
enum hmatrix_def_master_type {
	HMATRIX_DEFAULT_MASTER_NONE     = HMATRIXB_SCFG_DEFMSTR_TYPE(0),
	HMATRIX_DEFAULT_MASTER_PREVIOUS = HMATRIXB_SCFG_DEFMSTR_TYPE(1),
	HMATRIX_DEFAULT_MASTER_FIXED    = HMATRIXB_SCFG_DEFMSTR_TYPE(2)
};

/** \brief HMATRIX slave: arbitration type */
enum hmatrix_arbitration_type {
	HMATRIX_ARBITRATION_ROUND_ROBIN    = HMATRIXB_SCFG_ARBT_ROUND_ROBIN,
	HMATRIX_ARBITRATION_FIXED_PRIORITY = HMATRIXB_SCFG_ARBT_FIXED_PRIORITY
};

/** \brief Master identifier */
enum hmatrix_master_id {
	HMATRIX_MASTER_ID_CPU_IDCORE = HMATRIX_MASTER_CPU_IDCODE,
	HMATRIX_MASTER_ID_CPU_SYS    = HMATRIX_MASTER_CPU_SYS,
	HMATRIX_MASTER_ID_SMAP       = HMATRIX_MASTER_SMAP,
	HMATRIX_MASTER_ID_PDC        = HMATRIX_MASTER_PDCA,
	HMATRIX_MASTER_ID_USB        = HMATRIX_MASTER_USBC_MASTER,
	HMATRIX_MASTER_ID_CRCCU      = HMATRIX_MASTER_CRCCU,
	HMATRIX_MASTER_ID_NUM            /* number of Masters */
};

/** \brief Slave identifier */
enum hmatrix_slave_id {
	HMATRIX_SLAVE_ID_FLASH   = HMATRIX_SLAVE_FLASH,
	HMATRIX_SLAVE_ID_PBA     = HMATRIX_SLAVE_HTOP0,
	HMATRIX_SLAVE_ID_PBB     = HMATRIX_SLAVE_HTOP1,
	HMATRIX_SLAVE_ID_PBC     = HMATRIX_SLAVE_HTOP2,
	HMATRIX_SLAVE_ID_PBD     = HMATRIX_SLAVE_HTOP3,
	HMATRIX_SLAVE_ID_HRAMC0  = HMATRIX_SLAVE_HRAMC0,
	HMATRIX_SLAVE_ID_HRAMC1  = HMATRIX_SLAVE_HRAMC1,
	HMATRIX_SLAVE_ID_AES     = HMATRIX_SLAVE_AESA,
	HMATRIX_SLAVE_ID_NUM            /* number of Slaves */
};

/** \brief HMatrix Master Channel configuration structure.
 *
 *  Configuration structure for an HMatrix Master channel. This structure
 *  should be initialized by the \ref hmatrix_master_ch_get_config_defaults()
 *  function before being modified by the user application.
 */
struct hmatrix_master_ch_conf {
	enum hmatrix_burst_type        burst_type;
};

/** \brief HMatrix Slave Channel configuration structure.
 *
 *  Configuration structure for an HMatrix Slave channel. This structure
 *  should be initialized by the \ref hmatrix_slave_ch_get_config_defaults()
 *  function before being modified by the user application.
 */
struct hmatrix_slave_ch_conf {
	enum hmatrix_arbitration_type  arbitration_type;
	enum hmatrix_master_id         fixed_def_master_number;
	enum hmatrix_def_master_type   def_master_type;
	uint8_t                        slot_cycle;
};

/** \brief Initializes an HMatrix Master configuration structure to defaults.
 *
 *  Initializes a given HMatrix Master channel configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li Master channel Undefined Length Burst type is split into a
 *       sixteen-beat burst, allowing re-arbitration at each sixteen-beat
 *       burst end
 *
 *  \param config    Configuration structure to initialize to default values
 */
static inline void hmatrix_master_ch_get_config_defaults(
		struct hmatrix_master_ch_conf *const config)
{
	/* Sanity check arguments */
	Assert(config);

	/* Default configuration values */
	config->burst_type      = HMATRIX_BURST_FOUR_BEAT;
}

/** \brief Initializes an HMatrix Slave configuration structure to defaults.
 *
 *  Initializes a given HMatrix Slave channel configuration structure to a
 *  set of known default values. This function should be called on all new
 *  instances of these configuration structures before being modified by the
 *  user application.
 *
 *  The default configuration is as follows:
 *   \li Slave channel arbitration is round robin
 *   \li Slave channel is configured on last default master
 *   \li Slave channel slot cycle is 16
 *
 *  \param config    Configuration structure to initialize to default values
 */
static inline void hmatrix_slave_ch_get_config_defaults(
		struct hmatrix_slave_ch_conf *const config)
{
	/* Sanity check arguments */
	Assert(config);

	/* Default configuration values */
	config->arbitration_type        = HMATRIX_ARBITRATION_ROUND_ROBIN;
	config->fixed_def_master_number = HMATRIX_MASTER_ID_CPU_IDCORE;
	config->def_master_type         = HMATRIX_DEFAULT_MASTER_PREVIOUS;
	config->slot_cycle              = 16;
}

/** \brief Writes an HMatrix Master channel configuration to the hardware module.
 *
 *  Writes out a given configuration of a HMatrix Master channel configuration to
 *  the hardware module.
 *
 *  \param[in] master_id  HMatrix Master channel to configure
 *  \param[in] config     Configuration settings for the HMatrix Master
 */
void hmatrix_master_ch_set_config(const enum hmatrix_master_id master_id,
		struct hmatrix_master_ch_conf *const config);

/** \brief Writes an HMatrix Slave channel configuration to the hardware module.
 *
 *  Writes out a given configuration of a HMatrix Slave channel configuration to
 *  the hardware module.
 *
 *  \param[in] slave_id  HMatrix Slave channel to configure
 *  \param[in] config    Configuration settings for the HMatrix Slave
 */
void hmatrix_slave_ch_set_config(const enum hmatrix_slave_id slave_id,
		struct hmatrix_slave_ch_conf *const config);

/** \brief Initializes the HMatrix.
 *
 *  Initializes the HMatrix ready for use. This setup the clock mask for the hMatrix.
 */
void hmatrix_init(void);

/** \brief Enables the HMatrix.
 *
 *  Setup the clock mask for the hMatrix.
 */
void hmatrix_enable(void);

/** \brief Disables the HMatrix.
 *
 *  Stop the clock mask for the hMatrix.
 */
void hmatrix_disable(void);

/**
 * \brief Set priority for the specified slave access.
 *
 * \param[in] slave_id Slave index.
 * \param[in] p_prio Pointer to the priority register sets.
 */
void hmatrix_set_slave_priority(enum hmatrix_slave_id slave_id,
		hmatrix_prs *p_prio);

/**
 * \brief Get priority for the specified slave access.
 *
 * \param[in] slave_id Slave index.
 * \param[in] p_prio Pointer to the priority register sets.
 */
void hmatrix_get_slave_priority(enum hmatrix_slave_id slave_id,
		hmatrix_prs *p_prio);

#ifdef __cplusplus
}
#endif

/**
 * \page sam_hmatrix_quick_start Quick Start Guide for the HMATRIX driver
 *
 * This is the quick start guide for the \ref sam_drivers_hmatrix_group, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.The code examples can be copied into e.g the main
 * application loop or any other function that will need to control the
 * HMATRIX module.
 *
 * \section hmatrix_qs_use_cases Use cases
 * - \ref hmatrix_basic
 *
 * \section hmatrix_basic HMATRIX basic usage
 *
 * This use case will demonstrate how to configurate the HMATRIX module to
 * set the slave with different default master type.
 *
 *
 * \section hmatrix_basic_setup Setup steps
 *
 * \subsection hmatrix_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 *
 * \subsection hmatrix_basic_setup_code
 *
 * Add this to the main loop or a setup function:
 *
 * \subsection hmatrix_basic_setup_workflow
 *
 * -# Enable the HMATRIX module
 *  - \code hmatrix_enable(); \endcode
 * -# Get the default slave channel setting
 *  - \code
 * struct hmatrix_slave_ch_conf config;
 * hmatrix_slave_ch_get_config_defaults(&config);
 * \endcode
 *
 * \section hmatrix_basic_usage Usage steps
 *
 * \subsection hmatrix_basic_usage_code
 *
 * -# We can set slave with Round-Robin arbitration and without default master
 *  - \code
 * config.def_master_type = HMATRIX_DEFAULT_MASTER_NONE;
 * hmatrix_slave_ch_set_config(ul_slave_id, &config);
 * \endcode
 * -# Or Set slave with Round-Robin arbitration and with last access master
 *  - \code
 * config.def_master_type = HMATRIX_DEFAULT_MASTER_PREVIOUS;
 * hmatrix_slave_ch_set_config(ul_slave_id, &config);
 * \endcode
 *
 * We can set priority of each master for one slave by
 * \code
 * HmatrixbPrs prio;
 * prio.HMATRIXB_PRAS = 0x00111111;
 * prio.HMATRIXB_PRBS = 0x00000000;
 * hmatrix_set_slave_priority(ul_slave_id, &prio);
 * \endcode
 *
 */

#endif /* __HMATRIX_H_INCLUDED__ */
