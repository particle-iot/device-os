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

/* s140_nrf52_6.0.0 */
SOFTDEVICE_MINIMUM_RAM_BASE = 0x1628;
SOFTDEVICE_MINIMUM_CODE_BASE = 0x26000;

/* Reserving 16K in total */
APP_RAM_BASE = 16K;
/* Reserving 192K in total */
APP_CODE_BASE = 192K;

ASSERT ( APP_RAM_BASE >= SOFTDEVICE_MINIMUM_RAM_BASE, "APP_RAM_BASE needs to be adjusted" );
ASSERT ( APP_CODE_BASE >= SOFTDEVICE_MINIMUM_CODE_BASE, "APP_CODE_BASE needs to be adjusted" );
