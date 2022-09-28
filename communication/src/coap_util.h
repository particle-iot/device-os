/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"

namespace particle::protocol {

/**
 * Log the contents of a CoAP message.
 *
 * @param level Logging level.
 * @param category Logging category.
 * @param data Message data.
 * @param size Message size.
 * @param logPayload Whether to log the payload data of the message.
 */
void logCoapMessage(LogLevel level, const char* category, const char* data, size_t size, bool logPayload = false);

} // namespace particle::protocol
