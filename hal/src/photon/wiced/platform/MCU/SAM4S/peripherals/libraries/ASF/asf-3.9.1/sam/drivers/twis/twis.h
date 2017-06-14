/**
 * \file
 *
 * \brief TWIS driver for SAM.
 *
 * This file defines a useful set of functions for the TWIS on SAM4L devices.
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
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


#ifndef TWIS_H_INCLUDED
#define TWIS_H_INCLUDED

/**
 * \defgroup sam_drivers_twis_group TWIS - Two-Wire Slave Interface
 *
 * See \ref sam_twis_quickstart.
 *
 * Driver for the TWIS (Two-Wire Slave Interface).
 * This driver provides access to the main features of the TWIS controller.
 * The TWIS interconnects components on a unique two-wire bus.
 * The TWIS is programmable as a slave with sequential or single-byte access.
 * High speed mode capability is supported.
 *
 * \{
 */

#include "compiler.h"
#include "status_codes.h"
#include "sysclk.h"

#define TWIS_WAIT_TIMEOUT 1000

/**
 * \brief Input parameters when initializing the TWIS module mode
 */
struct twis_config {
	/** Ten-bit addressing */
	bool ten_bit;
	/** The desired address. */
	uint32_t chip;
	/** SMBUS mode */
	bool smbus;
	/** Stretch clock on data byte reception */
	bool stretch_clk_data;
	/** Stretch clock on address match */
	bool stretch_clk_addr;
	/** Stretch clock if RHR is full or THR is empty */
	bool stretch_clk_hr;
	/** Acknowledge the general call address */
	bool ack_general_call;
	/** Acknowledge the specified slave address */
	bool ack_slave_addr;
	/** Enable packet error checking */
	bool enable_pec;
	/** Acknowledge the SMBus host header */
	bool ack_smbus_host_header;
	/** Acknowledge the SMBus default address */
	bool ack_smbus_default_addr;
	/** Data Setup Cycles */
	uint8_t sudat;
	/** Input Spike Filter Control in F/S mode*/
	uint8_t fs_filter;
	/** Data Slew Limit in F/S mode */
	uint8_t fs_daslew;
	/** Data Drive Strength Low in F/S mode */
	uint8_t fs_dadrivel;
	/** Data Hold Cycles */
	uint8_t hddat;
	/** Input Spike Filter Control in HS mode*/
	uint8_t hs_filter;
	/** Data Slew Limit in HS mode */
	uint8_t hs_daslew;
	/** Data Drive Strength Low in HS mode*/
	uint8_t hs_dadrivel;
	/** Clock Prescaler */
	uint8_t exp;
	/** SMBus TIMEOUT Cycles */
	uint8_t ttouts;
	/** SMBus Low:Sext Cycles */
	uint8_t tlows;
};

/**
 * \brief TWI slave driver software instance structure.
 *
 * Device instance structure for a TWI Slave driver instance. This
 * structure should be initialized by the \ref twis_init() function to
 * associate the instance with a particular hardware module of the device.
 */
struct twis_dev_inst {
	/** Base address of the TWIS module. */
	Twis *hw_dev;
	/** Pointer to TWIS configuration structure. */
	struct twis_config  *twis_cfg;
};

/** TWIS interrupt source type */
typedef enum twis_interrupt_source {
	TWIS_INTERRUPT_RX_BUFFER_READY     = TWIS_IER_RXRDY,
	TWIS_INTERRUPT_TX_BUFFER_READY     = TWIS_IER_TXRDY,
	TWIS_INTERRUPT_TRANS_COMP          = TWIS_IER_TCOMP,
	TWIS_INTERRUPT_UNDER_RUN           = TWIS_IER_URUN,
	TWIS_INTERRUPT_OVER_RUN            = TWIS_IER_ORUN,
	TWIS_INTERRUPT_NAK_RECEIVED        = TWIS_IER_NAK,
	TWIS_INTERRUPT_SMBUS_TIMEOUT       = TWIS_IER_SMBTOUT,
	TWIS_INTERRUPT_SMBUS_PEC_ERROR     = TWIS_IER_SMBPECERR,
	TWIS_INTERRUPT_BUS_ERROR           = TWIS_IER_BUSERR,
	TWIS_INTERRUPT_SLAVEADR_MATCH      = TWIS_IER_SAM,
	TWIS_INTERRUPT_GENCALL_MATCH       = TWIS_IER_GCM,
	TWIS_INTERRUPT_SMBUS_HHADR_MATCH   = TWIS_IER_SMBHHM,
	TWIS_INTERRUPT_SMBUS_DEFADR_MATCH  = TWIS_IER_SMBDAM,
	TWIS_INTERRUPT_STOP_RECEIVED       = TWIS_IER_STO,
	TWIS_INTERRUPT_RESTART_RECEIVED    = TWIS_IER_REP,
	TWIS_INTERRUPT_BYTE_TRANS_FINISHED = TWIS_IER_BTF,
	TWIS_INTERRUPT_ERRORS              = (TWIS_IER_BUSERR |
			TWIS_IER_SMBPECERR | TWIS_IER_SMBTOUT | TWIS_IER_ORUN |
			TWIS_IER_URUN),
	TWIS_INTERRUPT_ALL                 = ~0UL
} twis_interrupt_source_t;

/*
 * \brief Pointer on TWI slave user specific application routines
 */
typedef struct twis_callback
{
	/** Routine to receive data from TWI master */
	void (*rx) (uint8_t);
	/** Routine to transmit data to TWI master */
	uint8_t (*tx) (void);
	/** Routine to signal a TWI STOP */
	void (*stop) (void);
	/** Routine to handle bus error */
	void (*error) (void);
} twis_callback_t;

void twis_get_config_defaults(struct twis_config *const cfg);
enum status_code twis_init(struct twis_dev_inst *const dev_inst,
		Twis *const twis, struct twis_config *config);
void twis_set_callback(struct twis_dev_inst *const dev_inst,
		twis_interrupt_source_t source, twis_callback_t callback,
		uint8_t irq_level);
void twis_enable(struct twis_dev_inst *const dev_inst);
void twis_disable(struct twis_dev_inst *const dev_inst);

/**
 * \brief Get the last byte data received from TWI bus.
 *
 * \param dev_inst  Device structure pointer.
 * \param *data     The read data.
 *
 * \return Status of write operation.
 * \retval STATUS_OK The data is read correctly.
 * \retval STATUS_ERR_TIMEOUT The read operation aborted due to timeout.
 */
static inline enum status_code twis_read(struct twis_dev_inst *const dev_inst,
		uint8_t *data)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	uint32_t i = 0;
	while (i++ < TWIS_WAIT_TIMEOUT) {
		if (dev_inst->hw_dev->TWIS_SR & TWIS_SR_RXRDY) {
			break;
		}
	}

	if (i >= TWIS_WAIT_TIMEOUT) {
		return STATUS_ERR_TIMEOUT;
	} else {
		*data = dev_inst->hw_dev->TWIS_RHR;
	}

    return STATUS_OK;
}

/**
 * \brief Write one byte data to TWI bus.
 *
 * \param dev_inst  Device structure pointer.
 * \param byte      The byte data to write.
 *
 * \return Status of write operation.
 * \retval STATUS_OK The data is written correctly.
 * \retval STATUS_ERR_TIMEOUT The write operation aborted due to timeout.
 */
static inline enum status_code twis_write(struct twis_dev_inst *const dev_inst,
		uint8_t byte)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	uint32_t i = 0;
	while (i++ < TWIS_WAIT_TIMEOUT) {
		if (dev_inst->hw_dev->TWIS_SR & TWIS_SR_TXRDY) {
			break;
		}
	}

	if (i >= TWIS_WAIT_TIMEOUT) {
		return STATUS_ERR_TIMEOUT;
	} else {
		dev_inst->hw_dev->TWIS_THR = byte;
	}

    return STATUS_OK;
}

/**
 * \brief Enable NACK transfer in Slave Receiver Mode
 *
 * \param dev_inst  Device structure pointer.
 */
static inline void twis_send_data_nack(struct twis_dev_inst *const dev_inst)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);
	dev_inst->hw_dev->TWIS_CR |= TWIS_CR_ACK;
}

/**
 * \brief Enable ACK transfer in Slave Receiver Mode
 *
 * \param dev_inst  Device structure pointer.
 */
static inline void twis_send_data_ack(struct twis_dev_inst *const dev_inst)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	dev_inst->hw_dev->TWIS_CR &= ~TWIS_CR_ACK;
}

/**
 * \brief Get the calculated PEC value. Only for SMBus mode.
 *
 * \param dev_inst  Device structure pointer.
 *
 * \retval Calculated PEC value
 */
static inline uint8_t twis_get_smbus_pec(struct twis_dev_inst *const dev_inst)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	return (uint8_t)TWIS_PECR_PEC(dev_inst->hw_dev->TWIS_PECR);
}

/**
 * \brief Set the total number of data bytes in the transmission. Only for SMBus mode.
 *
 * \param dev_inst  Device structure pointer
 * \param nb         Total number of data bytes in the transmission
 * \param increment  Count up per byte transferred if true, otherwise count down
 */
static inline void twis_set_smbus_transfer_nb(
		struct twis_dev_inst *const dev_inst, uint8_t nb, bool increment)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	if (increment) {
		dev_inst->hw_dev->TWIS_CR |= TWIS_CR_CUP;
	} else {
		dev_inst->hw_dev->TWIS_CR &= ~TWIS_CR_CUP;
	}

	dev_inst->hw_dev->TWIS_NBYTES = nb;
}

/**
 * \brief Get the progress of the transfer in SMBus mode.
 *
 * \param dev_inst  Device structure pointer.
 *
 * \retval The left number of data bytes in the transmission.
 */
static inline uint8_t twis_get_smbus_transfer_nb(
		struct twis_dev_inst *const dev_inst)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	return (uint8_t)TWIS_NBYTES_NBYTES(dev_inst->hw_dev->TWIS_PECR);
}

/**
 * \brief Enable the TWIS interrupts
 *
 * \param dev_inst  Device structure pointer
 * \param interrupt_source  The TWIS interrupt source to be enabled
 */
static inline void twis_enable_interrupt(struct twis_dev_inst *const dev_inst,
		twis_interrupt_source_t interrupt_source)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	/* Enable the interrupt source */
	dev_inst->hw_dev->TWIS_IER = interrupt_source;
}

/**
 * \brief Disable the TWIS interrupts
 *
 * \param dev_inst  Device structure pointer.
 * \param interrupt_source  The TWIS interrupt to be disabled
 */
static inline void twis_disable_interrupt(struct twis_dev_inst *const dev_inst,
		twis_interrupt_source_t interrupt_source)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	/* Disable the interrupt source */
	dev_inst->hw_dev->TWIS_IDR = interrupt_source;
}

/**
 * \brief Get the TWIS interrupt mask
 *
 * \param dev_inst  Device structure pointer.
 *
 * \retval TWIS interrupt mask
 */
static inline uint32_t twis_get_interrupt_mask(
		struct twis_dev_inst *const dev_inst)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	return dev_inst->hw_dev->TWIS_IMR;
}

/**
 * \brief Information about the current status of the TWIS
 *
 * \param dev_inst  Device structure pointer.
 */
static inline uint32_t twis_get_status(struct twis_dev_inst *const dev_inst)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	return dev_inst->hw_dev->TWIS_SR;
}

/**
 * \brief Clear the current status of the TWIS
 *
 * \param dev_inst  Device structure pointer
 * \param clear_status  The TWIS status to be clear
 */
static inline void twis_clear_status(struct twis_dev_inst *const dev_inst,
		uint32_t clear_status)
{
	Assert(dev_inst);
	Assert(dev_inst->hw_dev);

	dev_inst->hw_dev->TWIS_SCR = clear_status;
}

/**
 * \}
 */

/**
 * \page sam_twis_quickstart Quickstart guide for SAM TWIS driver
 *
 * This is the quickstart guide for the \ref group_sam_drivers_twis
 * "SAM TWIS driver", with step-by-step instructions on how to
 * configure and use the driver in a selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section sam_twis_basic_use_case Basic use case
 * The basic use case demonstrate how to initialize the TWIS module and
 * configure it.
 *
 * \subsection sam_twis_quickstart_prereq Prerequisites
 * -# \ref sysclk_group "System Clock Management (Sysclock)"
 *
 * \section twis_basic_use_case_setup Setup steps
 * \subsection twis_basic_use_case_setup_code Example code
 * Enable the following macro in the conf_clock.h:
 * \code
 *  #define CONFIG_SYSCLK_SOURCE   SYSCLK_SRC_DFLL
 *  #define CONFIG_DFLL0_SOURCE    GENCLK_SRC_OSC0
 * \endcode
 *
 * Add the following code in the application C-file:
 * \code
 *  sysclk_init();
 * \endcode
 *
 * \subsection twis_basic_use_case_setup_flow Workflow
 * -# Set system clock source as DFLL:
 *   - \code #define CONFIG_SYSCLK_SOURCE   SYSCLK_SRC_DFLL \endcode
 * -# Set DFLL source as OSC0:
 *   - \code CONFIG_DFLL0_SOURCE     GENCLK_SRC_OSC0 \endcode
 * -# Initialize the system clock.
 *   - \code sysclk_init(); \endcode
 *
 * \section twis_basic_use_case_usage Usage steps
 * \subsection twis_basic_use_case_usage_code Example code
 * Add to, e.g., main loop in application C-file:
 * \code
 *  struct twis_dev_inst twis_device;
 *  struct twis_config config;
 *  twis_get_config_defaults(&config);
 *  config.chip = SLAVE_ADDRESS;
 *  twis_init(&twis_device, BOARD_BASE_TWI_SLAVE, &config);
 *
 *  twis_callback_t slave_callbacks;
 *  slave_callbacks.rx = TWIS_RX_HANDLER_POINTER;
 *  slave_callbacks.tx = TWIS_TX_HANDLER_POINTER;
 *  slave_callbacks.stop = TWIS_STOP_HANDLER_POINTER;
 *  slave_callbacks.error = TWIS_ERROR_HANDLER_POINTER;
 *  twis_set_callback(&twis_device, TWIS_INTERRUPT_SLAVEADR_MATCH, slave_callbacks, 1);
 *
 *  twis_enable(&twis_device);
 * \endcode
 *
 * \subsection twis_basic_use_case_usage_flow Workflow
 * -# Initialize the TWIS module with the given self slave address SLAVE_ADDRESS:
 * \code
 *  struct twis_dev_inst twis_device;
 *  struct twis_config config;
 *  twis_get_config_defaults(&config);
 *  config.chip = SLAVE_ADDRESS;
 *  twis_init(&twis_device, BOARD_BASE_TWI_SLAVE, &config);
 * \endcode
 * -# Set up the callback functions to handle reception, transmission, STOP
 * condition and errors. In the end, enable the slave address match interrupt:
 * \code
 *  twis_callback_t slave_callbacks;
 *  slave_callbacks.rx = TWIS_RX_HANDLER_POINTER;
 *  slave_callbacks.tx = TWIS_TX_HANDLER_POINTER;
 *  slave_callbacks.stop = TWIS_STOP_HANDLER_POINTER;
 *  slave_callbacks.error = TWIS_ERROR_HANDLER_POINTER;
 *  twis_set_callback(&twis_device, TWIS_INTERRUPT_SLAVEADR_MATCH, slave_callbacks, 1);
 * \endcode
 * -# Enable TWIS module:
 * \code
 *  twis_enable(&twis_device);
 * \endcode
 */
#endif /* TWIS_H_INCLUDED */
