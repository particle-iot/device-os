/**
 * \file
 *
 * \brief TWIM driver for SAM.
 *
 * This file defines a useful set of functions for the TWIM on SAM4L devices.
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


#ifndef TWIM_H_INCLUDED
#define TWIM_H_INCLUDED

/**
 * \defgroup sam_drivers_twim_group TWIM - Two-Wire Master Interface
 *
 * Driver for the TWIM (Two-Wire Master Interface).
 * This driver provides access to the main features of the TWIM controller.
 * The TWIM interconnects components on a unique two-wire bus.
 * The TWIM is programmable as a master with sequential or single-byte access.
 * Multiple master capability is supported.
 *
 * \{
 */

#include "compiler.h"
#include "status_codes.h"

/** Enable TWIM Low Power Transfer in default */
#define TWIM_LOW_POWER_ENABLE 1

/* @{ */
/** TWI Standard Mode */
#define TWI_STD_MODE_SPEED         ( 100000 /* kbit/s */)
/** TWI Fast Mode */
#define TWI_FAST_MODE_SPEED        ( 400000 /* kbit/s */)
/** TWI Fast Mode Plus */
#define TWI_FAST_MODE_PLUS_SPEED   (1000000 /* kbit/s */)
/** TWI High Speed Mode */
#define TWI_HIGH_SPEED_MODE_SPEED  (3400000 /* kbit/s */)
/* @} */

/** Status Clear Register Mask for No Acknowledgements */
#define TWIM_SCR_NAK_MASK (TWIM_SCR_ANAK | TWIM_SCR_DNAK)
/** Status Register Mask for No Acknowledgements */
#define TWIM_SR_NAK_MASK  (TWIM_SR_ANAK | TWIM_SR_DNAK)
/** Interrupt Enable Register Mask for No Acknowledgements */
#define TWIM_IER_NAK_MASK (TWIM_IER_ANAK | TWIM_IER_DNAK)
/** Frequently used Interrupt Enable Register Mask */
#define TWIM_IER_STD_MASK (TWIM_IER_ANAK | TWIM_IER_ARBLST)
/** Frequently used Status Clear Register Mask */
#define TWIM_SR_STD_MASK  (TWIM_SR_ANAK | TWIM_SR_ARBLST)

/**
 * \brief Status Codes for TWI Transfer
 * @{
 */
enum twim_transfer_status {
	TWI_SUCCESS = 0,            /** \brief TWI Transaction Success */
	TWI_INVALID_ARGUMENT = -1,  /** \brief Invalid Argument Passed */
	TWI_ARBITRATION_LOST = -2,  /** \brief Bus Arbitration Lost */
	TWI_NO_CHIP_FOUND = -3,     /** \brief Slave Not Found */
	TWI_RECEIVE_NACK = -4,      /** \brief Data No Acknowledgement Received */
	TWI_SEND_NACK = -5,         /** \brief Data No Acknowledgement Send */
	TWI_INVALID_CLOCK_DIV = -6  /** \brief Invalid Clock Divider Value */
};

typedef enum twim_transfer_status twim_transfer_status_t;
/* @} */

/**
 * \brief Input parameters when initializing the TWIM module mode
 */
struct twim_config {
	/** The TWIM clock frequency */
	uint32_t twim_clk;
	/** The baudrate of the TWI bus */
	uint32_t speed;
	/** The baudrate of the TWI bus in high speed mode */
	uint32_t hsmode_speed;
	/** Clock cycles for data setup count */
	uint8_t data_setup_cycles;
	/** Clock cycles for data setup count in high speed mode */
	uint8_t hsmode_data_setup_cycles;
	/** SMBUS mode */
	bool smbus;
	/** Slew limit of the TWCK output buffer */
	uint8_t clock_slew_limit;
	/** Pull-down drive strength of the TWCK output buffer */
	uint8_t clock_drive_strength_low;
	/** Slew limit of the TWD output buffer */
	uint8_t data_slew_limit;
	/** Pull-down drive strength of the TWD output buffer */
	uint8_t data_drive_strength_low;
	/** Slew limit of the TWCK output buffer in high speed mode */
	uint8_t hs_clock_slew_limit;
	/** Pull-up drive strength of the TWCK output buffer in high speed mode */
	uint8_t hs_clock_drive_strength_high;
	/** Pull-down drive strength of the TWCK output buffer in high speed mode */
	uint8_t hs_clock_drive_strength_low;
	/** Slew limit of the TWD output buffer in high speed mode */
	uint8_t hs_data_slew_limit;
	/** Pull-down drive strength of the TWD output buffer in high speed mode */
	uint8_t hs_data_drive_strength_low;
};

/**
 * \brief Information concerning the data transmission
 */
struct twim_package {
	/** TWI chip address to communicate with. */
	uint32_t chip;
	/** TWI address/commands to issue to the other chip (node). */
	uint8_t addr[3];
	/** Length of the TWI data address segment (1-3 bytes). */
	uint8_t addr_length;
	/** Where to find the data to be written. */
	void *buffer;
	/** How many bytes do we want to write. */
	uint32_t length;
	/** Indicate if it is 10-bit addressing */
	bool ten_bit;
	/** Indicate if it is a high-speed transfer */
	bool high_speed;
	/** High speed mode master code, valid if high_speed is true */
	uint8_t high_speed_code;
};

/**
 * \name TWI Driver Compatibility
 * Codes for SAM devices using TWI modules can easily be ported
 * to SAM devices with TWIM module
 * @{
 */
#define twi_options_t struct twim_config
#define twi_package_t struct twim_package
#define twi_master_init twim_set_config
#define twi_probe twim_probe
/* @} */

/**
 * \brief Enable Master Mode of the TWI.
 *
 * \param twim   Base address of the TWIM instance.
 */
static inline void twim_enable(Twim *twim)
{
	twim->TWIM_CR = TWIM_CR_MEN;
}

/**
 * \brief Disable Master Mode of the TWI.
 *
 * \param twim   Base address of the TWIM instance.
 */
static inline void twim_disable(Twim *twim)
{
	twim->TWIM_CR = TWIM_CR_MDIS;
}

typedef void (*twim_callback_t)(Twim *);

status_code_t twim_set_config(Twim *twim, struct twim_config *config);
status_code_t twim_set_speed(Twim *twim, uint32_t speed, uint32_t clk,
	uint8_t cycles);
status_code_t twim_set_hsmode_speed(Twim *twim, uint32_t speed, uint32_t clk,
	uint8_t cycles);
status_code_t twim_probe(Twim *twim, uint32_t chip_addr);
status_code_t twi_master_read(Twim *twim, struct twim_package *package);
status_code_t twi_master_write(Twim *twim, struct twim_package *package);
void twim_enable_interrupt(Twim *twim, uint32_t interrupt_source);
void twim_disable_interrupt(Twim *twim, uint32_t interrupt_source);
uint32_t twim_get_interrupt_mask(Twim *twim);
void twim_default_callback(Twim *twim);
uint32_t twim_get_status(Twim *twim);
void twim_clear_status(Twim *twim, uint32_t clear_status);
void twim_set_callback(Twim *twim, uint32_t interrupt_source,
	twim_callback_t callback, uint8_t irq_level);

/**
 * \}
 */

/**
 * \page sam_twim_quick_start Quick Start Guide for the TWIM driver
 *
 * This is the quick start guide for the \ref sam_drivers_twim_group, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.The code examples can be copied into e.g the main
 * application loop or any other function that will need to control the
 * TWIM module.
 *
 * \section twim_use_cases Use cases
 * - \ref twim_basic
 *
 * \section twim_basic TWIM basic usage
 *
 * This use case will demonstrate how to initialize the TWIM module.
 *
 *
 * \section twim_basic_setup Setup steps
 *
 * \subsection twim_basic_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 *
 * \subsection twim_basic_setup_workflow
 *
 * -# Setup TWIM options, including TWIM clock frequency, the desired TWI bus
 * speed, the target chip slave address (optional) and being in SMBus mode or
 * not.
 *
 * - \code
 * struct twim_config twim_conf;
 * twim_conf.twim_clk = sysclk_get_cpu_hz();
 * twim_conf.speed = DESIRED_TWI_BUS_SPEED;
 * twim_conf.smbus = false;
 * twim_conf.hsmode_speed = 0;
 * twim_conf.data_setup_cycles = 0;
 * twim_conf.hsmode_data_setup_cycles = 0;
 * twim_set_config(TWIM0, &twim_conf);
 * \endcode
 *  - \note The TWIM driver supports I2C standard speed, fast speed, fast speed
 * plus and high speed.
 *
 * -# The default callback function must be set before calling read/write
 * functions.
 *  - \code
 * twim_set_callback(TWIM0, 0, twim_default_callback, 1);
 * \endcode
 *  - \note The read/write functions will enable and disable the corresponding
 * interrupt sources.
 *
 *
 * \section twim_basic_usage Usage steps
 *
 * \subsection twim_basic_usage_code
 *
 * We can send data to the target slave device. Firstly, the data package
 * should be prepared. In one data package, several items should be set, the
 * target slave address, the internal address (if needed), the length of the
 * internal address (if needed), the data buffer to be written and the length
 * of the data buffer.
 * \code
 * twi_package_t packet_tx;
 * packet_tx.chip = TARGET_SLAVE_ADDRESS;
 * packet_tx.addr[0] = (INTERNAL_ADDRESS >> 16) & 0xFF;
 * packet_tx.addr[1] = (INTERNAL_ADDRESS >> 8) & 0xFF;
 * packet_tx.addr_length = INTERNAL_ADDRESS_LENGTH;
 * packet_tx.buffer = (void *) data_buf_tx;
 * packet_tx.length = DATA_BUF_TX_LENGTH;
 * \endcode
 *  - \note The TWIM driver supports 1-3 bytes of internal address.
 *
 * After the data package is ready, we can call twi_master_write() to send the
 * package to the slave address. The callback set before will be handled in ISR.
 * \code
 * twi_master_write(TWIM1, &packet_tx);
 * \endcode
 *  - \note If the function returns STATUS_OK, the package has been sent to the
 * target slave device successfully. Otherwise, the transmission fails.
 *
 *
 * We can receive data from the target slave device. Firstly, the data package
 * should be prepared. In one data package, several items should be set, the
 * target slave address, the internal address (if needed), the length of the
 * internal address (if needed), the data buffer used to store received data
 * and the length of the data to be received.
 * \code
 * twi_package_t packet_rx;
 * packet_rx.chip = TARGET_SLAVE_ADDRESS;
 * packet_rx.addr[0] = (INTERNAL_ADDRESS >> 16) & 0xFF;
 * packet_rx.addr[1] = (INTERNAL_ADDRESS >> 8) & 0xFF;
 * packet_rx.addr_length = INTERNAL_ADDRESS_LENGTH;
 * packet_rx.buffer = (void *) data_buf_rx;
 * packet_rx.length = DATA_BUF_RX_LENGTH;
 * \endcode
 *  - \note The TWIM driver supports 1-3 bytes of internal address.
 *
 * After the data package is ready, we can call twi_master_read() to receive
 * the data package from the slave device. The callback set before will be
 * handled in ISR.
 * \code
 * twi_master_read(TWIM1, &packet_rx);
 * \endcode
 *  - \note If the function returns STATUS_OK, the package has been received
 * from the target slave device and the data has been stored in the data buffer
 * successfully. Otherwise, the transmission failed.
 */

#endif /* TWIM_H_INCLUDED */
