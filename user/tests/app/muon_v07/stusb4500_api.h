/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

#ifndef STUSB4500_API_H
#define STUSB4500_API_H

#include "Particle.h"
#include "SparkFun_STUSB4500.h"
#include "stusb4500_register_map.h"

int stusb4500Init(void);
void stusb4500SetDefaultParams(void);
void stusb4500LogParams(void);
void stusb4500LogStatus(void);
int stusb4500ReadDeviceId(uint8_t* deviceId);

#endif // STUSB4500_API_H