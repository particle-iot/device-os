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

int HAL_disable_irq() {
    // We are blocking any interrupts with priorities >= 2, without
    // affecting SoftDevice interrupts which run with priorities 0 and 1.
    int st = __get_BASEPRI();
    __set_BASEPRI(APP_IRQ_PRIORITY_HIGHEST << (8 - __NVIC_PRIO_BITS));
    return st;
}

void HAL_enable_irq(int is) {
    __set_BASEPRI(is);
}
