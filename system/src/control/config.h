/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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
namespace control {
namespace config {

int handleSetClaimCodeRequest(ctrl_request* req);
int handleIsClaimedRequest(ctrl_request* req);
int handleSetSecurityKeyRequest(ctrl_request* req);
int handleGetSecurityKeyRequest(ctrl_request* req);
int handleSetServerAddressRequest(ctrl_request* req);
int handleGetServerAddressRequest(ctrl_request* req);
int handleSetServerProtocolRequest(ctrl_request* req);
int handleGetServerProtocolRequest(ctrl_request* req);
int handleStartNyanRequest(ctrl_request* req);
int handleStopNyanRequest(ctrl_request* req);
int handleSetSoftapSsidRequest(ctrl_request* req);

} } } /* namespace particle::control::config */
