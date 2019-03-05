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

#include "system_network.h"

void network_on(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
}

void network_off(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
}

void network_connect(network_handle_t network, uint32_t flags, uint32_t param1, void* reserved) {
}

bool network_connecting(network_handle_t network, uint32_t param1, void* reserved) {
    return false;
}

void network_disconnect(network_handle_t network, uint32_t reason, void* reserved) {
}

void network_listen(network_handle_t network, uint32_t flags, void* reserved) {
}

bool network_listening(network_handle_t network, uint32_t param1, void* reserved) {
    return false;
}

void network_set_listen_timeout(network_handle_t network, uint16_t timeout, void* reserved) {
}

uint16_t network_get_listen_timeout(network_handle_t network, uint32_t flags, void* reserved) {
    return 0;
}
