/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef CELLULAR_HAL_H
#define	CELLULAR_HAL_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "net_hal.h"
#include "inet_hal.h"
#include "system_tick_hal.h"
#include "cellular_hal_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Power on and initialize the cellular module,
 * if USART3 not initialized, will be done on first call.
 */
cellular_result_t  cellular_on(void* reserved);

cellular_result_t  cellular_init(void* reserved);

/**
 * Power off the cellular module.
 */
cellular_result_t  cellular_off(void* reserved);

/**
 * Wait for the cellular module to register on the GSM network.
 */
cellular_result_t  cellular_register(void* reserved);

/**
 * Activate the PDP context
 */
cellular_result_t  cellular_pdp_activate(CellularCredentials* connect, void* reserved);

/**
 * Deactivate the PDP context
 */
cellular_result_t  cellular_pdp_deactivate(void* reserved);

/**
 * Perform a GPRS attach.
 */
cellular_result_t  cellular_gprs_attach(CellularCredentials* connect, void* reserved);

/**
 * Perform a GPRS detach.
 */
cellular_result_t  cellular_gprs_detach(void* reserved);

/**
 * Register in the network and establish a PSD connection.
 */
cellular_result_t cellular_connect(void* reserved);

/**
 * Close the PSD connection.
 */
cellular_result_t cellular_disconnect(void* reserved);

/**
 * Fetch the ip configuration.
 */
cellular_result_t  cellular_fetch_ipconfig(CellularConfig* config, void* reserved);

/**
 * Retrieve cellular module info, must be initialized first.
 */
cellular_result_t cellular_device_info(CellularDevice* device, void* reserved);

/**
 * Set cellular connection parameters
 */
cellular_result_t cellular_credentials_set(const char* apn, const char* username, const char* password, void* reserved);

/**
 * Get cellular connection parameters
 */
CellularCredentials* cellular_credentials_get(void* reserved);

bool cellular_sim_ready(void* reserved);

/**
 * Attempts to stop/resume the cellular modem from performing AT operations.
 * Called from another thread or ISR context.
 *
 * @param cancel: true to cancel AT operations, false will resume AT operations.
 *        calledFromISR: true if called from ISR, false if called from main system thread.
 *        reserved: pass NULL. Allows future expansion.
 */
void cellular_cancel(bool cancel, bool calledFromISR, void* reserved);

/**
 * Retrieve cellular signal strength info
 */
cellular_result_t cellular_signal(CellularSignalHal* signal, cellular_signal_t* reserved);

/**
 * Send an AT command and wait for response, optionally specify a callback function to parse the results
 */
cellular_result_t cellular_command(_CALLBACKPTR_MDM cb, void* param,
                         system_tick_t timeout_ms, const char* format, ...);

/**
 * Set cellular data usage info
 */
cellular_result_t cellular_data_usage_set(CellularDataHal* data, void* reserved);

/**
 * Get cellular data usage info
 */
cellular_result_t cellular_data_usage_get(CellularDataHal* data, void* reserved);

/**
 * Set the SMS received callback handler
 */
cellular_result_t cellular_sms_received_handler_set(_CELLULAR_SMS_CB_MDM cb, void* data, void* reserved);

/**
 * Implementation of the USART3 IRQ handler exported via dynalib interface
 */
void HAL_USART3_Handler_Impl(void* reserved);

/**
 * Pauses cellular modem serial communication
 */
cellular_result_t cellular_pause(void* reserved);

/**
 * Resumes cellular modem serial communication
 */
cellular_result_t cellular_resume(void* reserved);

/**
 * Set the cellular network provider based on the IMSI of the SIM card inserted
 */
cellular_result_t cellular_imsi_to_network_provider(void* reserved);

/**
 * Function for getting the cellular network provider data currently set
 */
const CellularNetProvData cellular_network_provider_data_get(void* reserved);

/**
 * Acquires the modem lock.
 */
int cellular_lock(void* reserved);

/**
 * Releases the modem lock.
 */
void cellular_unlock(void* reserved);

/**
 * Sets the power mode used, bound to 0-3.
 *
 * mode is volatile and will default to 1 on system reset/boot.
 */
void cellular_set_power_mode(int mode, void* reserved);

#ifdef __cplusplus
}
#endif

#endif	/* CELLULAR_HAL_H */
