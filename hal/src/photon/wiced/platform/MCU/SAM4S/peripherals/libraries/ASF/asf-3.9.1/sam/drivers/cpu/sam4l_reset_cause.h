/**
 * \file
 *
 * \brief Chip-specific reset cause functions
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
#ifndef SAM4L_RESET_CAUSE_H
#define SAM4L_RESET_CAUSE_H

#include "compiler.h"

/**
 * \defgroup group_sam4l_drivers_cpu SAM4L reset cause
 * \ingroup reset_cause_group
 *
 * See \ref reset_cause_quickstart
 *
 * @{
 */

/**
 * \brief Chip-specific reset cause type capable of holding all chip reset
 * causes. Typically reflects the size of the reset cause register.
 */
typedef uint32_t        reset_cause_t;

//! \internal \name Chip-specific reset causes
//@{

//! \internal External reset cause
# define CHIP_RESET_CAUSE_EXTRST        PM_RCAUSE_EXT

//! \internal Brown-out detected on CPU power domain reset cause
# define CHIP_RESET_CAUSE_BOD_CPU       PM_RCAUSE_BOD

//! \internal Brown-out detected on I/O power domain reset cause
# define CHIP_RESET_CAUSE_BOD_IO        PM_RCAUSE_BOD33

//! \internal On-chip debug system reset cause
# define CHIP_RESET_CAUSE_OCD           PM_RCAUSE_OCDRST

//! \internal Power-on-reset reset cause
# define CHIP_RESET_CAUSE_POR           PM_RCAUSE_POR

//! \internal Power-on-reset on I/O power domain reset cause
# define CHIP_RESET_CAUSE_POR_IO        PM_RCAUSE_POR33

//! \internal Watchdog timeout reset cause
# define CHIP_RESET_CAUSE_WDT           PM_RCAUSE_WDT

//@}

static inline reset_cause_t reset_cause_get_causes(void)
{
	return (reset_cause_t)PM->PM_RCAUSE;
}

static inline void reset_cause_clear_causes(reset_cause_t causes)
{
	/**
	 * \note Reset causes are not clearable on SAM4L.
	 */
	 UNUSED(causes);
}

static inline void reset_do_soft_reset(void)
{
	while (1) {
		NVIC_SystemReset();
	}
}

//! @}

#endif /* SAM4L_RESET_CAUSE_H */
