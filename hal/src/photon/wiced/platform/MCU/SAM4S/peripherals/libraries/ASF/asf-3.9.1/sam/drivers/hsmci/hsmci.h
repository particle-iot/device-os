/**
 * \file
 *
 * \brief SAM HSMCI driver
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

#ifndef HSMCI_H_INCLUDED
#define HSMCI_H_INCLUDED

#include "compiler.h"
#include "sd_mmc_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup sam_drivers_hsmci High Speed MultiMedia Card Interface (HSMCI)
 *
 * This driver interfaces the HSMCI module.
 * It will add functions for SD/MMC card reading, writing and management.
 *
 * @{
 */

/** \brief Initializes the low level driver
 *
 * This enable the clock required and the hardware interface.
 */
void hsmci_init(void);

/** \brief Return the maximum bus width of a slot
 *
 * \param slot     Selected slot
 *
 * \return 1, 4 or 8 lines.
 */
uint8_t hsmci_get_bus_width(uint8_t slot);

/** \brief Return the high speed capability of the driver
 *
 * \return true, if the high speed is supported
 */
bool hsmci_is_high_speed_capable(void);

/**
 * \brief Select a slot and initialize it
 *
 * \param slot       Selected slot
 * \param clock      Maximum clock to use (Hz)
 * \param bus_width  Bus width to use (1, 4 or 8)
 * \param high_speed true, to enable high speed mode
 */
void hsmci_select_device(uint8_t slot, uint32_t clock, uint8_t bus_width,
		bool high_speed);

/**
 * \brief Deselect a slot
 *
 * \param slot       Selected slot
 */
void hsmci_deselect_device(uint8_t slot);

/** \brief Send 74 clock cycles on the line of selected slot
 * Note: It is required after card plug and before card install.
 */
void hsmci_send_clock(void);

/** \brief Send a command on the selected slot
 *
 * \param cmd        Command definition
 * \param arg        Argument of the command
 *
 * \return true if success, otherwise false
 */
bool hsmci_send_cmd(sdmmc_cmd_def_t cmd, uint32_t arg);

/** \brief Return the 32 bits response of the last command
 *
 * \return 32 bits response
 */
uint32_t hsmci_get_response(void);

/** \brief Return the 128 bits response of the last command
 *
 * \param response   Pointer on the array to fill with the 128 bits response
 */
void hsmci_get_response_128(uint8_t* response);

/** \brief Send an ADTC command on the selected slot
 * An ADTC (Addressed Data Transfer Commands) command is used
 * for read/write access.
 *
 * \param cmd          Command definition
 * \param arg          Argument of the command
 * \param block_size   Block size used for the transfer
 * \param nb_block     Total number of block for this transfer
 * \param access_block if true, the x_read_blocks() and x_write_blocks()
 * functions must be used after this function.
 * If false, the mci_read_word() and mci_write_word()
 * functions must be used after this function.
 *
 * \return true if success, otherwise false
 */
bool hsmci_adtc_start(sdmmc_cmd_def_t cmd, uint32_t arg, uint16_t block_size,
		uint16_t nb_block, bool access_block);

/** \brief Send a command to stop an ADTC command on the selected slot
 *
 * \param cmd        Command definition
 * \param arg        Argument of the command
 *
 * \return true if success, otherwise false
 */
bool hsmci_adtc_stop(sdmmc_cmd_def_t cmd, uint32_t arg);

/** \brief Read a word on the line
 *
 * \param value  Pointer on a word to fill
 *
 * \return true if success, otherwise false
 */
bool hsmci_read_word(uint32_t* value);

/** \brief Write a word on the line
 *
 * \param value  Word to send
 *
 * \return true if success, otherwise false
 */
bool hsmci_write_word(uint32_t value);

/** \brief Start a read blocks transfer on the line
 * Note: The driver will use the DMA available to speed up the transfer.
 *
 * \param dest       Pointer on the buffer to fill
 * \param nb_block   Number of block to transfer
 *
 * \return true if started, otherwise false
 */
bool hsmci_start_read_blocks(void *dest, uint16_t nb_block);

/** \brief Wait the end of transfer initiated by mci_start_read_blocks()
 *
 * \return true if success, otherwise false
 */
bool hsmci_wait_end_of_read_blocks(void);

/** \brief Start a write blocks transfer on the line
 * Note: The driver will use the DMA available to speed up the transfer.
 *
 * \param src        Pointer on the buffer to send
 * \param nb_block   Number of block to transfer
 *
 * \return true if started, otherwise false
 */
bool hsmci_start_write_blocks(const void *src, uint16_t nb_block);

/** \brief Wait the end of transfer initiated by mci_start_write_blocks()
 *
 * \return true if success, otherwise false
 */
bool hsmci_wait_end_of_write_blocks(void);

//! @}

#ifdef __cplusplus
}
#endif

#endif /* HSMCI_H_INCLUDED */
