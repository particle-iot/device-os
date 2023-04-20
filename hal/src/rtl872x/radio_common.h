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
 * Load the BLE antenna setting from the DCT and select that antenna.
 *
 * @return 0 on success or a negative result code in case of an error.
 */
int initRadioAntenna();

/**
 * Disable the BLE antenna
 *
 * @return 0 on success or a negative result code in case of an error.
 */
int disableRadioAntenna();

/**
 * Enable the BLE antenna
 *
 * @return 0 on success or a negative result code in case of an error.
 */
int enableRadioAntenna();

/**
 * Select the BLE antenna and store the setting in the DCT.
 *
 * @param antenna Antenna type.
 * @return 0 on success or a negative result code in case of an error.
 */
int selectRadioAntenna(radio_antenna_type antenna);

/**
 * Return the currently configured antenna type from DCT.
 *
 * @param antenna Currently configured Antenna type.
 * @return 0 on success or a negative result code in case of an error.
 */
int getRadioAntenna(radio_antenna_type* antenna);

} // particle
