/**
 * \file
 *
 * \brief Cyclic Redundancy Check Calculation Unit (CRCCU) driver for SAM.
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

#ifndef CRCCU_H_INCLUDED
#define CRCCU_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/* CRCCU transfer control width register offset */
#define CRCCU_TR_CTRL_TRWIDTH_POS  (24)
/* CRCCU transfer control width register byte */
#define CRCCU_TR_CTRL_TRWIDTH_BYTE (0 << CRCCU_TR_CTRL_TRWIDTH_POS)
/* CRCCU transfer control width register halfword */
#define CRCCU_TR_CTRL_TRWIDTH_HALFWORD (1 << CRCCU_TR_CTRL_TRWIDTH_POS)
/* CRCCU transfer control width register word */
#define CRCCU_TR_CTRL_TRWIDTH_WORD (2 << CRCCU_TR_CTRL_TRWIDTH_POS)

/* CRCCU context done interrupt enable offset */
#define CRCCU_TR_CTRL_IEN_OFFSET   (27)
/*The transfer done status bit is set at the end of the transfer.*/
#define CRCCU_TR_CTRL_IEN_ENABLE   (0 << CRCCU_TR_CTRL_IEN_OFFSET)
/*The transfer done status bit is not set at the end of the transfer.*/
#define CRCCU_TR_CTRL_IEN_DISABLE  (1 << CRCCU_TR_CTRL_IEN_OFFSET)

/** CRCCU descriptor type */
typedef struct crccu_dscr_type {
	uint32_t ul_tr_addr;	/* TR_ADDR */
	uint32_t ul_tr_ctrl;	/* TR_CTRL */
#if (SAM3SD8 || SAM4S || SAM4L)
	uint32_t ul_reserved[2];	/* Reserved register */
#elif SAM3S
	uint32_t ul_reserved[52];	/* TR_CRC begins at offset 0xE0 */
#endif
	uint32_t ul_tr_crc;	/* TR_CRC */
} crccu_dscr_type_t;

void crccu_configure_descriptor(Crccu *p_crccu, uint32_t ul_crc_dscr_addr);
void crccu_configure_mode(Crccu *p_crccu, uint32_t ul_mode);
void crccu_enable_dma(Crccu *p_crccu);
void crccu_disable_dma(Crccu *p_crccu);
void crccu_reset(Crccu *p_crccu);
uint32_t crccu_get_dma_status(Crccu *p_crccu);
void crccu_enable_dma_interrupt(Crccu *p_crccu);
void crccu_disable_dma_interrupt(Crccu *p_crccu);
uint32_t crccu_get_dma_interrupt_status(Crccu *p_crccu);
uint32_t crccu_get_dma_interrupt_mask(Crccu *p_crccu);
uint32_t crccu_read_crc_value(Crccu *p_crccu);
void crccu_enable_error_interrupt(Crccu *p_crccu);
void crccu_disable_error_interrupt(Crccu *p_crccu);
uint32_t crccu_get_error_interrupt_status(Crccu *p_crccu);
uint32_t crccu_get_error_interrupt_mask(Crccu *p_crccu);

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

#endif /* CRCCU_H_INCLUDED */
