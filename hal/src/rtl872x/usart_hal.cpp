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
#include "usart_hal.h"
#include "ringbuffer.h"
#include "pinmap_hal.h"
#include "gpio_hal.h"
#include <algorithm>
#include "hal_irq_flag.h"
#include "delay_hal.h"
#include "interrupts_hal.h"
#include "usart_hal_private.h"
#include "timer_hal.h"

int hal_usart_init_ex(hal_usart_interface_t serial, const hal_usart_buffer_config_t* config, void*) {
    return 0;
}

void hal_usart_init(hal_usart_interface_t serial, hal_usart_ring_buffer_t* rxBuffer, hal_usart_ring_buffer_t* txBuffer) {
    hal_usart_buffer_config_t conf = {
        .size = sizeof(hal_usart_buffer_config_t),
        .rx_buffer = rxBuffer->buffer,
        .rx_buffer_size = sizeof(rxBuffer->buffer),
        .tx_buffer = txBuffer->buffer,
        .tx_buffer_size = sizeof(txBuffer->buffer)
    };

    hal_usart_init_ex(serial, &conf, nullptr);
}

void hal_usart_begin_config(hal_usart_interface_t serial, uint32_t baud, uint32_t config, void*) {

}

void hal_usart_begin(hal_usart_interface_t serial, uint32_t baud) {
    hal_usart_begin_config(serial, baud, SERIAL_8N1, nullptr);
}

void hal_usart_end(hal_usart_interface_t serial) {
}

int32_t hal_usart_available_data_for_write(hal_usart_interface_t serial) {
    return 0;
}

int32_t hal_usart_available(hal_usart_interface_t serial) {
    return 0;
}

void hal_usart_flush(hal_usart_interface_t serial) {

}

bool hal_usart_is_enabled(hal_usart_interface_t serial) {
    return false;
}

ssize_t hal_usart_write_buffer(hal_usart_interface_t serial, const void* buffer, size_t size, size_t elementSize) {
    return 0;
}

ssize_t hal_usart_read_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize) {
    return 0;
}

ssize_t hal_usart_peek_buffer(hal_usart_interface_t serial, void* buffer, size_t size, size_t elementSize) {
    return 0;
}

int32_t hal_usart_read(hal_usart_interface_t serial) {
    return 0;
}

int32_t hal_usart_peek(hal_usart_interface_t serial) {
    return 0;
}

uint32_t hal_usart_write_nine_bits(hal_usart_interface_t serial, uint16_t data) {
    return hal_usart_write(serial, data);
}

uint32_t hal_usart_write(hal_usart_interface_t serial, uint8_t data) {
    return 0;
}

void hal_usart_send_break(hal_usart_interface_t serial, void* reserved) {
    // Unsupported
}

uint8_t hal_usart_break_detected(hal_usart_interface_t serial) {
    // Unsupported
    return SYSTEM_ERROR_NONE;
}

void hal_usart_half_duplex(hal_usart_interface_t serial, bool enable) {
    // Unsupported
}

int hal_usart_pvt_get_event_group_handle(hal_usart_interface_t serial, EventGroupHandle_t* handle) {
    return SYSTEM_ERROR_NONE;
}

int hal_usart_pvt_enable_event(hal_usart_interface_t serial, HAL_USART_Pvt_Events events) {
    return 0;
}

int hal_usart_pvt_disable_event(hal_usart_interface_t serial, HAL_USART_Pvt_Events events) {
    return 0;
}

int hal_usart_pvt_wait_event(hal_usart_interface_t serial, uint32_t events, system_tick_t timeout) {
    return 0;
}

int hal_usart_sleep(hal_usart_interface_t serial, bool sleep, void* reserved) {
    return 0;
}
