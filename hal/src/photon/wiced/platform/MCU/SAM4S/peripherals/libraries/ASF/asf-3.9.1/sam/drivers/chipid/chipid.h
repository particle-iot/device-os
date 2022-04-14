/**
 * \file
 *
 * \brief Chip Identifier (CHIPID) driver for SAM.
 *
 * Copyright (c) 2011-2012 Atmel Corporation. All rights reserved.
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

#ifndef CHIPID_H_INCLUDED
#define CHIPID_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

//! Definition for chip id register data struct
typedef struct chipid_data {

	//! Version of the device
	uint32_t ul_version;
	//! Embedded processor
	uint32_t ul_eproc;
	//! Non-volatile program memory size
	uint32_t ul_nvpsiz;
	//! Second non-volatile program memory size
	uint32_t ul_nvpsiz2;
	//! Internal SRAM size
	uint32_t ul_sramsiz;
	//! Architecture identifier
	uint32_t ul_arch;
	//! Non-volatile program memory type
	uint32_t ul_nvptyp;
	//! Extension flag
	uint32_t ul_extflag;
	//! Chip ID extension
	uint32_t ul_extid;
} chipid_data_t;

uint32_t chipid_read(Chipid *p_chipid, chipid_data_t *p_chipid_data);
uint32_t chipid_read_version(Chipid *p_chipid);
uint32_t chipid_read_processor(Chipid *p_chipid);
uint32_t chipid_read_arch(Chipid *p_chipid);
uint32_t chipid_read_sramsize(Chipid *p_chipid);
uint32_t chipid_read_nvpmsize(Chipid *p_chipid);
uint32_t chipid_read_nvpm2size(Chipid *p_chipid);
uint32_t chipid_read_nvpmtype(Chipid *p_chipid);
uint32_t chipid_read_extchipid(Chipid *p_chipid);

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

#endif /* CHIPID_H_INCLUDED */
