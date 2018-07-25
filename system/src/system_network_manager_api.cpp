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

#include "system_network_manager.h"

#if HAL_PLATFORM_IFAPI

#include "system_network.h"
#include "system_network_internal.h"

using namespace particle::system;

/* FIXME: */
uint32_t wlan_watchdog_base;
uint32_t wlan_watchdog_duration;

volatile uint8_t SPARK_WLAN_RESET;
volatile uint8_t SPARK_WLAN_SLEEP;
volatile uint8_t SPARK_WLAN_STARTED;

const void* network_config(network_handle_t network, uint32_t param, void* reserved) {
    return nullptr;
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved) {
}

bool network_ready(network_handle_t network, uint32_t param, void* reserved)
{
    return false;
}

bool network_connecting(network_handle_t network, uint32_t param, void* reserved) {
    return false;
}

int network_connect_cancel(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
    return 0;
}

void network_on(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
}

bool network_has_credentials(network_handle_t network, uint32_t param, void* reserved) {
    return false;
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param, void* reserved) {
}

void network_listen(network_handle_t network, uint32_t flags, void*) {
}

void network_set_listen_timeout(network_handle_t network, uint16_t timeout, void*) {
}

uint16_t network_get_listen_timeout(network_handle_t network, uint32_t flags, void*) {
    return 0;
}

bool network_listening(network_handle_t network, uint32_t, void*) {
    return false;
}

int network_set_credentials(network_handle_t network, uint32_t, NetworkCredentials* credentials, void*) {
    return -1;
}

bool network_clear_credentials(network_handle_t network, uint32_t, NetworkCredentials* creds, void*) {
    return false;
}

void network_setup(network_handle_t network, uint32_t flags, void* reserved) {
}

int network_set_hostname(network_handle_t network, uint32_t flags, const char* hostname, void* reserved) {
    return -1;
}

int network_get_hostname(network_handle_t network, uint32_t flags, char* buffer, size_t buffer_len, void* reserved) {
    return -1;
}

int network_clear_settings(network_handle_t network, uint32_t flags, void* reserved) {
    return -1;
}

/**
 * Reset or initialize the network connection as required.
 */
void manage_network_connection() {
}

void manage_smart_config() {
}

void manage_ip_config() {
}

#endif /* HAL_PLATFORM_IFAPI */
