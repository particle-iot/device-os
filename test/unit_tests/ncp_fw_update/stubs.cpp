/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "system_mode.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "platform_ncp.h"
#include "system_network.h"
#include "cellular_hal.h"

uint32_t HAL_Timer_Get_Milli_Seconds() {
    static uint32_t millis = 0;
    return ++millis;
}

void HAL_Delay_Milliseconds(uint32_t ms) {
}

System_Mode_TypeDef system_mode() {
    return DEFAULT;
}

int system_reset(unsigned mode, unsigned reason, unsigned value, unsigned flags, void* reserved) {
    return 0;
}

PlatformNCPIdentifier platform_primary_ncp_identifier() {
    return PLATFORM_NCP_SARA_R510;
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
}

bool network_connecting(network_handle_t network, uint32_t param1, void* reserved) {
    return false;
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved) {
}

bool network_ready(network_handle_t network, uint32_t type, void* reserved) {
    return false;
}

void network_on(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
}

bool network_is_on(network_handle_t network, void* reserved) {
    return false;
}

bool network_is_off(network_handle_t network, void* reserved) {
    return false;
}

int cellular_start_ncp_firmware_update(bool update, void* reserved) {
    return 0;
}

int cellular_command(_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, ...) {
    return 0;
}

int sendCommandWithArgs(_CALLBACKPTR_MDM cb, void* param, system_tick_t timeout_ms, const char* format, va_list args) {
    return 0;
}

int cellular_get_ncp_firmware_version(uint32_t* version, void* reserved) {
    return 0;
}

int cellular_add_urc_handler(const char* prefix, hal_cellular_urc_callback_t cb, void* context) {
    return 0;
}

int cellular_remove_urc_handler(const char* prefix) {
    return 0;
}

int setupHTTPSProperties_impl() {
    return 0;
}

int cellular_lock(void* reserved) {
    return 0;
}

void cellular_unlock(void* reserved) {
}
