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

// Kore Super SIM multi-IMSI applet, two generations with different commands
// https://docs.korewireless.com/kore/supersim/supersim-multi-imsi-applet

#include "apdu.h"

namespace particle {

namespace kore {

enum class ImsiApplet {
    NONE = 0,
    CURRENT = 1,
    NEW = 2
};

// Which applet the SIM runs, ImsiApplet::NONE if neither answered, and whether it is switching IMSIs
// on its own. Returns 0 on success, negative on error.
int readImsiAppletStatus(SendApduFn send, void* ctx, ImsiApplet& applet, bool& automatic);

// Sends the rotation command. Returns 0 if it was sent, negative on error. Whatever the applet
// answers, only an IMSI check can tell whether it rotated.
int forceImsiRotation(SendApduFn send, void* ctx, ImsiApplet applet);

// Returns the applet to automatic switching. Returns 0 if it took the command, negative otherwise.
int setImsiAppletAutomatic(SendApduFn send, void* ctx, ImsiApplet applet);

} // kore

} // particle
