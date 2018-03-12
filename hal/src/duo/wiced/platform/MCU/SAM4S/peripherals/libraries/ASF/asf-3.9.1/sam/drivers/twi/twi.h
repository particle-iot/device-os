/**
 * \file
 *
 * \brief Two-Wire Interface (TWI) driver for SAM.
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

#ifndef TWI_H_INCLUDED
#define TWI_H_INCLUDED

#include "compiler.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \brief Return codes for TWI APIs.
 * @{
 */
#define TWI_SUCCESS              0
#define TWI_INVALID_ARGUMENT     1
#define TWI_ARBITRATION_LOST     2
#define TWI_NO_CHIP_FOUND        3
#define TWI_RECEIVE_OVERRUN      4
#define TWI_RECEIVE_NACK         5
#define TWI_SEND_OVERRUN         6
#define TWI_SEND_NACK            7
#define TWI_BUSY                 8
/**
 * @}
 */
 
/**
 * \brief Input parameters when initializing the TWI module mode.
 */
typedef struct twi_options {
	//! MCK for TWI.
	uint32_t master_clk;
	//! The baud rate of the TWI bus.
	uint32_t speed;
	//! The desired address.
	uint8_t chip;
	//! SMBUS mode (set 1 to use SMBUS quick command, otherwise don't).
	uint8_t smbus;
} twi_options_t;

/**
 * \brief Information concerning the data transmission.
 */
typedef struct twi_packet {
	//! TWI address/commands to issue to the other chip (node).
	uint8_t addr[3];
	//! Length of the TWI data address segment (1-3 bytes).
	uint32_t addr_length;
	//! Where to find the data to be transferred.
	void *buffer;
	//! How many bytes do we want to transfer.
	uint32_t length;
	//! TWI chip address to communicate with.
	uint8_t chip;
} twi_packet_t;

void twi_enable_master_mode(Twi *p_twi);
void twi_disable_master_mode(Twi *p_twi);
uint32_t twi_master_init(Twi *p_twi, const twi_options_t *p_opt);
uint32_t twi_set_speed(Twi *p_twi, uint32_t ul_speed, uint32_t ul_mck);
uint32_t twi_probe(Twi *p_twi, uint8_t uc_slave_addr);
uint32_t twi_master_read(Twi *p_twi, twi_packet_t *p_packet);
uint32_t twi_master_write(Twi *p_twi, twi_packet_t *p_packet);
void twi_enable_interrupt(Twi *p_twi, uint32_t ul_sources);
void twi_disable_interrupt(Twi *p_twi, uint32_t ul_sources);
uint32_t twi_get_interrupt_status(Twi *p_twi);
uint32_t twi_get_interrupt_mask(Twi *p_twi);
uint8_t twi_read_byte(Twi *p_twi);
void twi_write_byte(Twi *p_twi, uint8_t uc_byte);
void twi_enable_slave_mode(Twi *p_twi);
void twi_disable_slave_mode(Twi *p_twi);
void twi_slave_init(Twi *p_twi, uint32_t ul_device_addr);
void twi_set_slave_addr(Twi *p_twi, uint32_t ul_device_addr);
uint32_t twi_slave_read(Twi *p_twi, uint8_t *p_data);
uint32_t twi_slave_write(Twi *p_twi, uint8_t *p_data);
void twi_reset(Twi *p_twi);
Pdc *twi_get_pdc_base(Twi *p_twi);
#if SAM4E
void twi_set_write_protection(Twi *p_twi, bool flag);
void twi_read_write_protection_status(Twi *p_twi, uint32_t *p_status);
#endif

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

#endif /* TWI_H_INCLUDED */
