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

#include "esp32_ncp_client.h"

#include "check.h"

namespace particle {

namespace {

} // unnamed

int Esp32NcpClient::init(const NcpClientConfig& conf) {
    return 0;
}

void Esp32NcpClient::destroy() {
}

int Esp32NcpClient::on() {
    return 0;
}

void Esp32NcpClient::off() {
}

NcpState Esp32NcpClient::ncpState() {
    return NcpState::OFF;
}

void Esp32NcpClient::disconnect() {
}

NcpConnectionState Esp32NcpClient::connectionState() {
    return NcpConnectionState::DISCONNECTED;
}

int Esp32NcpClient::getFirmwareVersionString(char* buf, size_t size) {
    return 0;
}

int Esp32NcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    return 0;
}

int Esp32NcpClient::updateFirmware(InputStream* file, size_t size) {
    return 0;
}

AtParser* Esp32NcpClient::atParser() {
    return &atParser_;
}

void Esp32NcpClient::lock() {
}

void Esp32NcpClient::unlock() {
}

int Esp32NcpClient::ncpId() const {
    return 0;
}

int Esp32NcpClient::connect(const char* ssid, const Bssid& bssid, WifiSecurity sec, const WifiCredentials& cred) {
    return 0;
}

int Esp32NcpClient::getNetworkInfo(WifiNetworkInfo* info) {
    return 0;
}

int Esp32NcpClient::scan(WifiScanCallback callback, void* data) {
    return 0;
}

} // particle
