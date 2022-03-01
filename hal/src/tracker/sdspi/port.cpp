/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

// FIXME: pinmap definitions caused problems when nrf52840.h is included, so as a simple workaround include it here first
#include <nrf52840.h>
#include "esp32_sdio.h"
#include "sdspi_host.h"
#include "port.h"
#include "system_error.h"
#include "delay_hal.h"

namespace {

esp_err_t systemErrorToEsp(int err) {
    switch (err) {
        case SYSTEM_ERROR_NONE: {
            return ESP_OK;
        }
        case SYSTEM_ERROR_NO_MEMORY: {
            return ESP_ERR_NO_MEM;
        }
        case SYSTEM_ERROR_INVALID_ARGUMENT: {
            return ESP_ERR_INVALID_ARG;
        }
        case SYSTEM_ERROR_INVALID_STATE: {
            return ESP_ERR_INVALID_STATE;
        }
        case SYSTEM_ERROR_NOT_FOUND: {
            return ESP_ERR_NOT_FOUND;
        }
        case SYSTEM_ERROR_NOT_SUPPORTED: {
            return ESP_ERR_NOT_SUPPORTED;
        }
        case SYSTEM_ERROR_TIMEOUT: {
            return ESP_ERR_TIMEOUT;
        }
        case SYSTEM_ERROR_BAD_DATA: {
            return ESP_ERR_INVALID_RESPONSE;
        }
        default: {
            return SYSTEM_ERROR_UNKNOWN;
        }
    }
}

} // anonymous

using namespace particle;

/// Set CS high
void at_cs_high(void) {
    Esp32Sdio::instance()->chipSelect(false);
}

/// Set CS low
void at_cs_low(void) {
    Esp32Sdio::instance()->chipSelect(true);
}

esp_err_t at_spi_transmit(const void* tx_buff, void* rx_buff, uint32_t len) {
    return systemErrorToEsp(Esp32Sdio::instance()->spiTransmit(tx_buff, rx_buff, len));
}

esp_err_t at_spi_slot_init(void) {
    // We will handle low level initialization ourselves
    return ESP_OK;
}

void at_do_delay(uint32_t wait_ms) {
    HAL_Delay_Milliseconds(wait_ms);
}

AT_MUTEX_T at_mutex_init(void) {
    return (AT_MUTEX_T)Esp32Sdio::instance();
}

void at_mutex_lock(AT_MUTEX_T pxMutex) {
    auto instance = static_cast<Esp32Sdio*>(pxMutex);
    if (instance) {
        instance->lock();
    }
}

void at_mutex_unlock(AT_MUTEX_T pxMutex) {
    auto instance = static_cast<Esp32Sdio*>(pxMutex);
    if (instance) {
        instance->unlock();
    }
}

void at_mutex_free(AT_MUTEX_T pxMutex) {
    //
}
