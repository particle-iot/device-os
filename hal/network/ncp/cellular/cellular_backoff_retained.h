/*
 ******************************************************************************
 *  Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#pragma once

namespace particle {

// The backoff stage held across a reset or a hibernate
// We cannot store the cooldown deadline, hibernate resets millis()

// Returns 0 if there is no retained stage, which means start at stage 1
unsigned cellularBackoffRetainedStage();

// Pass 0 to clear the retained stage
void cellularBackoffSetRetainedStage(unsigned stage);

} // namespace particle
