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
#include "logging.h"
#include "spi_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "gpio_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "delay_hal.h"
#include "check.h"

#include <memory>
#include <cstdlib>

#define DEFAULT_SPI_MODE        SPI_MODE_MASTER
#define DEFAULT_DATA_MODE       SPI_MODE3
#define DEFAULT_BIT_ORDER       MSBFIRST
#define DEFAULT_SPI_CLOCK       SPI_CLOCK_DIV256
#define SPI_SYSTEM_CLOCK        (64000000UL)

#define SPI_BUFFER_LENGTH       32

void hal_spi_init(hal_spi_interface_t spi) {
   
}

void hal_spi_begin(hal_spi_interface_t spi, uint16_t pin) {
    // Default to Master mode
    hal_spi_begin_ext(spi, SPI_MODE_MASTER, pin, nullptr);
}

void hal_spi_begin_ext(hal_spi_interface_t spi, hal_spi_mode_t mode, uint16_t pin, void* reserved) {
    if (spi >= HAL_PLATFORM_SPI_NUM) {
        return;
    }
}

void hal_spi_end(hal_spi_interface_t spi) {

}

void hal_spi_set_bit_order(hal_spi_interface_t spi, uint8_t order) {

}

void hal_spi_set_data_mode(hal_spi_interface_t spi, uint8_t mode) {

}

void hal_spi_set_clock_divider(hal_spi_interface_t spi, uint8_t rate) {

}

uint16_t hal_spi_transfer(hal_spi_interface_t spi, uint16_t data) {
    return 0;
}

bool hal_spi_is_enabled(hal_spi_interface_t spi) {
    return false;
}

bool hal_spi_is_enabled_deprecated(void) {
    return false;
}

void hal_spi_info(hal_spi_interface_t spi, hal_spi_info_t* info, void* reserved) {

}

void hal_spi_set_callback_on_selected(hal_spi_interface_t spi, hal_spi_select_user_callback cb, void* reserved) {

}

void hal_spi_transfer_dma(hal_spi_interface_t spi, const void* tx_buffer, void* rx_buffer, uint32_t length, hal_spi_dma_user_callback userCallback) {

}

void hal_spi_transfer_dma_cancel(hal_spi_interface_t spi) {

}

int32_t hal_spi_transfer_dma_status(hal_spi_interface_t spi, hal_spi_transfer_status_t* st) {
    return 0;
}

int32_t hal_spi_set_settings(hal_spi_interface_t spi, uint8_t set_default, uint8_t clockdiv, uint8_t order, uint8_t mode, void* reserved) {
    return 0;
}

int hal_spi_sleep(hal_spi_interface_t spi, bool sleep, void* reserved) {
    return SYSTEM_ERROR_NONE;
}

int32_t hal_spi_acquire(hal_spi_interface_t spi, const hal_spi_acquire_config_t* conf) {
    return -1;
}

int32_t hal_spi_release(hal_spi_interface_t spi, void* reserved) {
    return -1;
}

int hal_spi_get_clock_divider(hal_spi_interface_t spi, uint32_t clock, void* reserved) {
    return SYSTEM_ERROR_UNKNOWN;
}
