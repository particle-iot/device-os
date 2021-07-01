/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
LOG_SOURCE_CATEGORY("hal.usb.cdc");

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "usb_hal_cdc.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "hal_platform.h"
#include "system_error.h"
#include "interrupts_hal.h"

int usb_hal_init(void) {
    return 0;
}

int usb_uart_init(uint8_t *rx_buf, uint16_t rx_buf_size, uint8_t *tx_buf, uint16_t tx_buf_size) {
    return 0;
}

int usb_uart_send(uint8_t data[], uint16_t size) {
    return 0;
}

void usb_uart_set_baudrate(uint32_t baudrate) {

}

uint32_t usb_uart_get_baudrate(void) {
    return 0;
}

void usb_hal_attach(void) {

}

void usb_hal_detach(void) {

}

int usb_uart_available_rx_data(void) {
    return 0;
}

uint8_t usb_uart_get_rx_data(void) {
    return 0;
}

uint8_t usb_uart_peek_rx_data(uint8_t index) {
    return 0;
}

void usb_uart_flush_rx_data(void) {
}

void usb_uart_flush_tx_data(void) {

}

int usb_uart_available_tx_data(void) {
    return -1;
}

bool usb_hal_is_enabled(void) {
    return false;
}

bool usb_hal_is_connected(void) {
    return false;
}

void usb_hal_set_bit_rate_changed_handler(void (*handler)(uint32_t bitRate)) {

}

HAL_USB_State usb_hal_get_state() {
    return HAL_USB_STATE_NONE;
}

int usb_hal_set_state_change_callback(HAL_USB_State_Callback cb, void* context, void* reserved) {

    return SYSTEM_ERROR_NO_MEMORY;
}
