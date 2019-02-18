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

INCLUDE softdevice.ld

MEMORY
{
    INCLUDE backup_ram_memory.ld

    /* FIXME: bootloader size should come from a common .ld */
    APP_FLASH (rx)  : ORIGIN = APP_CODE_BASE, LENGTH = 1M - APP_CODE_BASE - 48K

    SRAM      (rwx) : ORIGIN = 0x20000000 + APP_RAM_BASE, LENGTH = 256K - APP_RAM_BASE - LENGTH(BACKUPSRAM_ALL)
}

