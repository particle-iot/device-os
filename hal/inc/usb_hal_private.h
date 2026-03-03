/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

// This header defines private USART HAL APIs that may be implemented only on certain platforms
// and are not intended to be exported.
#pragma once

#include "usb_hal.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include "system_tick_hal.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int hal_usb_cdc_pvt_get_event_group_handle(EventGroupHandle_t* handle);
int hal_usb_cdc_pvt_wait_event(uint32_t events, system_tick_t timeout);
int hal_usb_cdc_pvt_send_data(const char* data, size_t size);
int hal_usb_cdc_pvt_recv_data(char* data, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus
