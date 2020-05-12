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

#pragma once

#include "sdspi_hal_defs.h"
#include "spi_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO:
int hal_sdspi_init(HAL_SPI_Interface spi, uint32_t clock, pin_t csPin, pin_t intPin);
int hal_sdspi_uninit(HAL_SPI_Interface spi, pin_t csPin_, pin_t intPin);
int hal_sdspi_get_packet(spi_context_t* context, void* out_data, size_t size, size_t* out_length);
int hal_sdspi_send_packet(spi_context_t* context, const void* start, size_t length, uint32_t wait_ms);
size_t hal_sdspi_available_for_read(spi_context_t* context);
size_t hal_sdspi_available_for_write(spi_context_t* context);
int hal_sdspi_clear_intr(uint32_t intr_mask);
int hal_sdspi_get_intr(uint32_t* intr_raw);
int hal_sdspi_wait_intr(uint32_t wait_ms);

#ifdef __cplusplus
}
#endif
