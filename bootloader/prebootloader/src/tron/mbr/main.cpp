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

#include <cstdlib>
#include "rtl_support.h"
extern "C" {
#include "rtl8721d.h"
}
#include "bootloader_update.h"
#include "km4_misc.h"

extern "C" int main() {
    /*
     * FIXME: Do NOT allocate memory from heap in MBR, since the heap start address is incorrect!
     * As a workaround, we can export run time APIs in part1.
     */

    rtlLowLevelInit();
    rtlPmuInit();
    rtlIpcInit();

    rtlPowerOnBigCore();

    bootloaderUpdateInit(); // IPC channel should be initialized after KM4 is powered on
    km4MiscInit();

    while (true) {
        __WFE();
        __WFE(); // clear event

        bootloaderUpdateProcess();
        km4MiscProcess();
    }

    return 0;
}
