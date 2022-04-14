/**
 * \file
 *
 * \brief Peripheral DMA Controller (PDC) driver for SAM.
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

#ifndef PDC_H_INCLUDED
#define PDC_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/*! \brief PDC data packet for transfer */
typedef struct pdc_packet {
	/** \brief Address for PDC transfer packet.
	 *  The pointer to packet data start address. For pointer or next pointer
	 *  register (_PR).
	 */
	uint32_t ul_addr;
	/** \brief PDC transfer packet size.
	 *  Size for counter or next counter register (_CR). The max value is
	 *  0xffff.
	 *  The unit of size is based on peripheral data width, that is, data
	 *  width that each time the peripheral transfers.
	 *  E.g., size of PDC for USART is in number of bytes, but size of PDC
	 *  for 16 bit SSC is in number of 16 bit word.
	 */
	uint32_t ul_size;
} pdc_packet_t;

void pdc_tx_init(Pdc *p_pdc, pdc_packet_t *p_packet,
		pdc_packet_t *p_next_packet);
void pdc_rx_init(Pdc *p_pdc, pdc_packet_t *p_packet,
		pdc_packet_t *p_next_packet);
void pdc_rx_clear_cnt(Pdc *p_pdc);
void pdc_enable_transfer(Pdc *p_pdc, uint32_t ul_controls);
void pdc_disable_transfer(Pdc *p_pdc, uint32_t ul_controls);
uint32_t pdc_read_status(Pdc *p_pdc);
uint32_t pdc_read_rx_ptr(Pdc *p_pdc);
uint32_t pdc_read_rx_counter(Pdc *p_pdc);
uint32_t pdc_read_tx_ptr(Pdc *p_pdc);
uint32_t pdc_read_tx_counter(Pdc *p_pdc);
uint32_t pdc_read_rx_next_ptr(Pdc *p_pdc);
uint32_t pdc_read_rx_next_counter(Pdc *p_pdc);
uint32_t pdc_read_tx_next_ptr(Pdc *p_pdc);
uint32_t pdc_read_tx_next_counter(Pdc *p_pdc);

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

#endif /* PDC_H_INCLUDED */
