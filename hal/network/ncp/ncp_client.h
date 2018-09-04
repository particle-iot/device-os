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

#include "platform_ncp.h"

namespace particle {

class InputStream;

class NcpClient {
public:
    enum State {
        UNKNOWN = 0,
        READY = 1,
        OFF = 2
    };

    virtual ~NcpClient() = default;

    virtual int waitReady(unsigned timeout) = 0;
    virtual void reset() = 0;
    virtual void off() = 0;

    virtual State state() const = 0;

    virtual int updateFirmware(InputStream* file, size_t size) = 0;

    virtual int getFirmwareVersionString(char* buf, size_t size) = 0;
    virtual int getFirmwareModuleVersion(uint16_t* ver) = 0;

    virtual int ncpId() const = 0;

    static int create(int ncpId, NcpClient** ncpClient);
};

} // particle
