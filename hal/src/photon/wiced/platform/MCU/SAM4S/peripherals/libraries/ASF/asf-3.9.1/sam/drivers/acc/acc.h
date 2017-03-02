/**
 * \file
 *
 * \brief Analog Comparator Controller (ACC) driver for SAM.
 *
 * Copyright (c) 2011 - 2013 Atmel Corporation. All rights reserved.
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

#ifndef ACC_H_INCLUDED
#define ACC_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

void acc_init(Acc *p_acc, uint32_t ul_select_plus, uint32_t ul_select_minus,
		uint32_t ul_edge_type, uint32_t ul_invert);
void acc_enable(Acc *p_acc);
void acc_disable(Acc *p_acc);
void acc_reset(Acc *p_acc);
void acc_set_input(Acc *p_acc, uint32_t ul_select_minus,
		uint32_t ul_select_plus);
void acc_set_output(Acc *p_acc, uint32_t ul_invert,
		uint32_t ul_fault_enable, uint32_t ul_fault_source);
uint32_t acc_get_comparison_result(Acc *p_acc);
void acc_enable_interrupt(Acc *p_acc);
void acc_disable_interrupt(Acc *p_acc);
uint32_t acc_get_interrupt_status(Acc *p_acc);
void acc_set_writeprotect(Acc *p_acc, uint32_t ul_enable);
uint32_t acc_get_writeprotect_status(Acc *p_acc);

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

#endif /* ACC_H_INCLUDED */
