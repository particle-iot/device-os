/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "radio_hal.h"

namespace particle {

/**
 * Load the mesh/BLE antenna setting from the DCT and select that antenna.
 *
 * @return 0 on success or a negative result code in case of an error.
 */
int initRadioAntenna();

/**
 * Select the mesh/BLE antenna and store the setting in the DCT.
 *
 * @param antenna Antenna type.
 * @return 0 on success or a negative result code in case of an error.
 */
int selectRadioAntenna(radio_antenna_type antenna);

} // particle
