/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
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

// Just enough of SGP.22 to answer "is any profile enabled?" over AT+CSIM. The APDU sequence and the
// GetProfilesInfo tag list come from blueprint-m635e-ntn (lib/satellite), and the profileState
// decoding follows SGP.22 5.7.15 the way device-os-esim-manager does it, so this works on any
// eUICC rather than one particular vendor's. Expected to be deleted once the eSIM manager library
// is part of Device OS.

#include <cstddef>

namespace particle {

namespace esim {

// Sends a single APDU and returns 0 on success, negative on error. respSize is in/out: the size of
// resp on the way in, and the number of bytes written (data plus the two status bytes) on the way
// out.
typedef int (*SendApduFn)(void* ctx, const char* cmd, size_t cmdSize, char* resp, size_t& respSize);

// Returns 1 if the eUICC has at least one enabled profile, 0 if every profile is disabled or there
// are none at all, and a negative error code if we could not find out. Callers must treat an error
// as "we don't know" and carry on as usual, never as "no profile".
// If iccid is given, the enabled profile's ICCID is written to it on a return of 1, and it is left
// empty otherwise. iccidSize should be at least 21 to hold a 20 digit ICCID and its terminator.
int hasEnabledProfile(SendApduFn send, void* ctx, char* iccid = nullptr, size_t iccidSize = 0);

} // esim

} // particle
