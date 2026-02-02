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

#include "hal_platform.h"

namespace particle::control::config {

int getDeviceId(ctrl_request* req);
int getSerialNumber(ctrl_request* req);
int getSystemVersion(ctrl_request* req);
int getNcpFirmwareVersion(ctrl_request* req);
int getSystemCapabilities(ctrl_request* req);
int getDeviceMode(ctrl_request* req);
int setDeviceSetupDone(ctrl_request* req);
int isDeviceSetupDone(ctrl_request* req);
int setStartupMode(ctrl_request* req);
int getProtectedState(ctrl_request* req);
int setProtectedState(ctrl_request* req);
int setFeature(ctrl_request* req);
int getFeature(ctrl_request* req);
int echo(ctrl_request* req);

#if HAL_PLATFORM_ENV
int getEnv(ctrl_request* req);
int clearEnv(ctrl_request* req);
#endif

// Deprecated
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

} // particle::control::config
