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

#include "system_cloud.h"

void Spark_Abort() {
}

void cloud_disconnect(unsigned flags, cloud_disconnect_reason cloudReason, network_disconnect_reason networkReason,
		System_Reset_Reason resetReason, unsigned sleepDuration) {
}

void spark_cloud_flag_disconnect() {
}

bool spark_cloud_flag_connected() {
	return false;
}

String spark_deviceID(void) {
    return "_THIS_IS_STUB_DEVICE_ID_";
}
