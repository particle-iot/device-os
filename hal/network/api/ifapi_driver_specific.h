/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct if_req_driver_specific {
    uint32_t type;
} if_req_driver_specific;

typedef enum if_req_driver_specific_type {
    IF_REQ_DRIVER_SPECIFIC_NONE = 0,
    IF_REQ_DRIVER_SPECIFIC_WIZNET_PIN_REMAP = 1,
    // FIXME: for compatibility
    IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP = IF_REQ_DRIVER_SPECIFIC_WIZNET_PIN_REMAP,
    IF_REQ_DRIVER_SPECIFIC_PPP_SERVER_UART_SETTINGS = 2
} if_req_driver_specific_type;

typedef struct if_wiznet_pin_remap {
    if_req_driver_specific base;
    uint16_t cs_pin;
    uint16_t reset_pin;
    uint16_t int_pin;
} if_wiznet_pin_remap;

typedef struct if_req_ppp_server_uart_settings {
    if_req_driver_specific base;
    uint8_t serial;
    uint8_t reserved[3];
    uint32_t baud;
    uint32_t config;
    uint32_t reserved1;
} if_req_ppp_server_uart_settings;

#ifdef __cplusplus
}
#endif /* __cplusplus */
