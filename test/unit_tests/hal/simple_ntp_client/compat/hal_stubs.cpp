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

#include "socket_hal_compat.h"
#include "inet_hal_compat.h"
#include "rng_hal.h"
#include "rtc_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"

static system_tick_t millisCounter = 0;

uint32_t HAL_RNG_GetRandomNumber() {
	return 0;
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    return 0;
}

uint64_t hal_timer_micros(void* reserved) {
    return 0;
}

int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, network_interface_t nif, void* reserved) {
    return -1;
}

sock_result_t socket_receivefrom_ex(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize, system_tick_t timeout, void* reserved) {
    return -1;
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size) {
    return -1;
}

uint8_t socket_handle_valid(sock_handle_t handle) {
    return handle >= 0;
}

sock_handle_t socket_handle_invalid() {
    return -1;
}

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif) {
    return -1;
}

sock_result_t socket_close(sock_handle_t sock) {
    return -1;
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void) {
    return millisCounter++;
}

void HAL_Delay_Milliseconds(uint32_t millis) {
    millisCounter += millis;
}
