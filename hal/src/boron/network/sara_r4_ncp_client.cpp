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

#include "sara_r4_ncp_client.h"

#include "serial_stream.h"

#include "check.h"

namespace particle {

SaraR4NcpClient::SaraR4NcpClient() {
}

SaraR4NcpClient::~SaraR4NcpClient() {
}

int SaraR4NcpClient::init(const NcpClientConfig& conf) {
    // const auto& c = static_cast<const CellularNcpClientConfig&>(conf);
    return 0;
}

void SaraR4NcpClient::destroy() {
}

int SaraR4NcpClient::on() {
    return 0;
}

void SaraR4NcpClient::off() {
}

NcpState SaraR4NcpClient::ncpState() {
    return NcpState::OFF;
}

int SaraR4NcpClient::disconnect() {
    return 0;
}

NcpConnectionState SaraR4NcpClient::connectionState() {
    return NcpConnectionState::DISCONNECTED;
}

int SaraR4NcpClient::getFirmwareVersionString(char* buf, size_t size) {
    return 0;
}

int SaraR4NcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return 0;
}

int SaraR4NcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int SaraR4NcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
    return 0;
}

void SaraR4NcpClient::processEvents() {
}

int SaraR4NcpClient::ncpId() const {
    return MeshNCPIdentifier::MESH_NCP_SARA_U201;
}

int SaraR4NcpClient::connect(const CellularNetworkConfig& conf) {
    return 0;
}

int SaraR4NcpClient::getIccid(char* buf, size_t size) {
    return 0;
}

} // particle
