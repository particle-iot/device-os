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

#include <nrf52840.h>
#include <nrf_nvic.h>
#include <app_util_platform.h>

#ifdef SOFTDEVICE_PRESENT
/* Global nvic state instance, required by nrf_nvic.h */
nrf_nvic_state_t nrf_nvic_state = {};
#endif

int HAL_disable_irq() {
    // We are blocking any interrupts with priorities >= 2, without
    // affecting SoftDevice interrupts which run with priorities 0 and 1.
    // NOTE: SoftDevice SVC calls cannot be made!
    int st = __get_BASEPRI();
    __set_BASEPRI(APP_IRQ_PRIORITY_HIGHEST << (8 - __NVIC_PRIO_BITS));
    return st;
}

void HAL_enable_irq(int is) {
    __set_BASEPRI(is);
}

void app_util_critical_region_enter(uint8_t* nested) {
    // We are blocking any interrupts with priorities >= 2, without
    // affecting SoftDevice interrupts which run with priorities 0, 1 and 4.
    // NOTE: SoftDevice SVC calls can be made here!
    *nested = __get_BASEPRI();
    __set_BASEPRI(_PRIO_SD_LOWEST << (8 - __NVIC_PRIO_BITS));
}

void app_util_critical_region_exit(uint8_t nested) {
    __set_BASEPRI(nested);
}
