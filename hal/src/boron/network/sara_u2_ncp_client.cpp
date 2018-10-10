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

#include "sara_u2_ncp_client.h"

#include "serial_stream.h"

#include "check.h"

namespace particle {

SaraU2NcpClient::SaraU2NcpClient() {
}

SaraU2NcpClient::~SaraU2NcpClient() {
}

int SaraU2NcpClient::init(const NcpClientConfig& conf) {
    // const auto& c = static_cast<const CellularNcpClientConfig&>(conf);
    return 0;
}

void SaraU2NcpClient::destroy() {
}

int SaraU2NcpClient::on() {
    return 0;
}

void SaraU2NcpClient::off() {
}

NcpState SaraU2NcpClient::ncpState() {
    return NcpState::OFF;
}

int SaraU2NcpClient::disconnect() {
    return 0;
}

NcpConnectionState SaraU2NcpClient::connectionState() {
    return NcpConnectionState::DISCONNECTED;
}

int SaraU2NcpClient::getFirmwareVersionString(char* buf, size_t size) {
    return 0;
}

int SaraU2NcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return 0;
}

int SaraU2NcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraU2NcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    return 0;
}

void SaraU2NcpClient::processEvents() {
}

int SaraU2NcpClient::ncpId() const {
    return MeshNCPIdentifier::MESH_NCP_SARA_U201;
}

int SaraU2NcpClient::connect(const CellularNetworkConfig& conf) {
    return 0;
}

} // particle
