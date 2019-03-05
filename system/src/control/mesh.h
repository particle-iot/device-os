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

#pragma once

#include "system_control.h"

namespace particle {

namespace ctrl {

namespace mesh {

int auth(ctrl_request* req);
int createNetwork(ctrl_request* req);
int startCommissioner(ctrl_request* req);
int stopCommissioner(ctrl_request* req);
int prepareJoiner(ctrl_request* req);
int addJoiner(ctrl_request* req);
int removeJoiner(ctrl_request* req);
int joinNetwork(ctrl_request* req);
int leaveNetwork(ctrl_request* req);
int getNetworkInfo(ctrl_request* req);
int scanNetworks(ctrl_request* req);
int getNetworkDiagnostics(ctrl_request* req);

int test(ctrl_request* req); // FIXME

int notifyBorderRouter(bool active);

} // particle::ctrl::mesh

} // particle::ctrl

} // particle
