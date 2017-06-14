/**
 * \file
 *
 * \brief PICOUART driver for SAM.
 *
 * Copyright (C) 2012-2013 Atmel Corporation. All rights reserved.
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

#ifndef _PICOUART_H_INCLUDED
#define _PICOUART_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup group_sam_drivers_picouart PICOUART - Picopower UART
 *
 * Driver for the PICOUART (Picopower UART).
 * Provides functions for configuring and operating the PICOUART.
 *
 * \{
 */

#include "compiler.h"
#include "status_codes.h"

enum picouart_actions_on_rx {
	/**< No action upon reception */
	PICOUART_ACTION_NONE                 = PICOUART_CFG_SOURCE(0),
	/**< Wake-up on start bit detection */
	PICOUART_ACTION_WAKEUP_ON_STARTBIT   = PICOUART_CFG_SOURCE(1),
	/**< Wake-up on full frame reception */
	PICOUART_ACTION_WAKEUP_ON_FULLFRAME  = PICOUART_CFG_SOURCE(2),
	/**< Wake up on character match */
	PICOUART_ACTION_WAKEUP_ON_MATCH      = PICOUART_CFG_SOURCE(3),
	/**< Event enable on start bit detection */
	PICOUART_ACTION_EVENT_ON_STARTBIT    = PICOUART_CFG_SOURCE(1)
			| PICOUART_CFG_ACTION,
	/**< Event enable on full frame reception */
	PICOUART_ACTION_EVENT_ON_FULLFRAME   = PICOUART_CFG_SOURCE(2)
			| PICOUART_CFG_ACTION,
	/**< Event enable on character match */
	PICOUART_ACTION_EVENT_ON_MATCH       = PICOUART_CFG_SOURCE(3)
			| PICOUART_CFG_ACTION
};

struct picouart_config {
	/* Action upon reception configuration */
	enum picouart_actions_on_rx action;
	/* Comparison character, used if the wake-up or the event generation 
	action is enabled upon character match */
	uint8_t                     match;
};

struct picouart_dev_inst {
	Picouart *dev_ptr;
};


/* Function Declarations */

/**
 * \brief Load a pre-defined configuration into the configuration structure
 *
 * The config struct is loaded with predefined values to allow quick startup.
 *
 * \param config pointer to the config struct
 */
void picouart_get_config_defaults(struct picouart_config *config);

/**
 * \brief Initializes the PICOUART module, based on a config struct
 *
 * \param dev_inst    pointer to the device struct
 * \param hw_dev      pointer to the register base address of the device
 * \param config      pointer to the config struct
 */
status_code_t picouart_init(struct picouart_dev_inst *const dev_inst, 
		Picouart *hw_dev, struct picouart_config *config);

/**
 * \brief Configures the PICOUART module, based on a config struct.
 *
 * \param dev_inst    pointer to the device struct
 * \param config      pointer to the config struct
 */
status_code_t picouart_set_config(struct picouart_dev_inst *const dev_inst,
		struct picouart_config *config);

/**
 * \brief Enable the PICOUART.
 *
 * \param dev_inst    pointer to the device struct
 */
void picouart_enable(struct picouart_dev_inst *const dev_inst);

/**
 * \brief Disable the PICOUART.
 *
 * \param dev_inst    pointer to the device struct
 */
void picouart_disable(struct picouart_dev_inst *const dev_inst);

/**
 * \brief Check the enable status of PICOUART
 *
 * \param dev_inst    pointer to the device struct
 *
 * \return true if PICOUART is enabled
 */
bool picouart_is_enabled(struct picouart_dev_inst *const dev_inst);

/**
 * \brief Check the data ready status of PICOUART
 *
 * \param dev_inst    pointer to the device struct
 *
 * \return true if data is ready
 */
bool picouart_is_data_ready(struct picouart_dev_inst *const dev_inst);

/**
 * \brief Get the current RHR value.
 *
 * \param dev_inst    pointer to the device struct
 * \param result      pointer to store result 
 *
 * \return The current RHR value.
 */
status_code_t picouart_read(struct picouart_dev_inst *const dev_inst, 
	uint8_t *result);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

/**
 * \page sam_picouart_quick_start Quick Start Guide for the PICOUART driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_picouart, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.The code examples can be copied into e.g the main
 * application loop or any other function that will need to control the
 * PICOUART module.
 *
 * \section picouart_qs_use_cases Use cases
 * - \ref picouart_basic
 *
 * \section picouart_basic PICOUART basic usage
 *
 * This use case will demonstrate how to initialize the PICOUART module to
 * wakeup system or generate event.
 *
 *
 * \section picouart_basic_setup Setup steps
 *
 * \subsection picouart_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref sysclk_group
 *
 * \subsection picouart_basic_setup_code
 *
 * Add this to the main loop or a setup function:
 * \code
 * if (!osc_is_ready(OSC_ID_OSC32)) {
 *	 osc_enable(OSC_ID_OSC32);
 *	 osc_wait_ready(OSC_ID_OSC32);
 * }
 * \endcode
 *
 * Or
 * \code
 * if (!osc_is_ready(OSC_ID_RC32K)) {
 *	 osc_enable(OSC_ID_RC32K);
 *	 osc_wait_ready(OSC_ID_RC32K);
 * }
 * \endcode
 * \subsection picouart_basic_setup_workflow
 * -# Init the PICOUART module
 * \code
 * struct picouart_dev_inst dev_inst;
 * struct picouart_config config;
 * picouart_get_config_defaults(&config);
 * picouart_init(&dev_inst, PICOUART, &config);
 * \endcode
 *
 * -# Enable the PICOUART module
 *  - \code picouart_enable(PICOUART); \endcode
 *
 * \section picouart_basic_usage Usage steps
 *
 * \subsection picouart_basic_usage_code
 * -# configurate the PICOUART to wakeup system
 *  - \code
 * case '0':
 *     printf("Enter backup mode with start bit wakeup.\r\n");
 *	 config.action = PICOUART_ACTION_WAKEUP_ON_STARTBIT;
 *     picouart_set_config(&dev_inst, &config);
 *     bpm_sleep(BPM, BPM_SM_BACKUP);
 *     break;
 * 
 * case '1':
 *     printf("Enter backup mode with full frame wakeup.\r\n");
 *     config.action = PICOUART_ACTION_WAKEUP_ON_FULLFRAME;
 *     picouart_set_config(&dev_inst, &config);
 *     bpm_sleep(BPM, BPM_SM_BACKUP);
 *     break;
 * 
 * case '2':
 *     printf("Enter backup mode with character match wakeup.\r\n");
 *     config.action = PICOUART_ACTION_WAKEUP_ON_MATCH;
 *     config.match = 'L';
 *     picouart_set_config(&dev_inst, &config);
 *     bpm_sleep(BPM, BPM_SM_BACKUP);
 *     break;
 * \endcode
 *
 * -# Or configurate the PICOUART to generate event
 *  - \code
 * case '0':
 *     config.action = PICOUART_ACTION_EVENT_ON_STARTBIT;
 *     picouart_set_config(&dev_inst, &config);
 *	 break;
 * 
 * case '1':
 *     config.action = PICOUART_ACTION_EVENT_ON_FULLFRAME;
 *     picouart_set_config(&dev_inst, &config);
 *	 break;
 * 
 * case '2':
 *     config.action = PICOUART_ACTION_EVENT_ON_MATCH;
 *     config.match = 'L';
 *     picouart_set_config(&dev_inst, &config);
 *	 break;
 * \endcode
 *
 * We can get the received value by
 * \code
 * while(!picouart_is_data_ready(&dev_inst)) {
 *     }
 * picouart_read(&dev_inst, &data);
 * \endcode
 *
 */

#endif  /* _PICOUART_H_INCLUDED */
