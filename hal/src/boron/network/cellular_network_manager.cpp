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

#include "cellular_network_manager.h"

namespace particle {

CellularNetworkManager::CellularNetworkManager() :
        client_(nullptr) {
}

CellularNetworkManager::~CellularNetworkManager() {
}

int CellularNetworkManager::init() {
    return 0;
}

void CellularNetworkManager::destroy() {
}

int CellularNetworkManager::connect() {
    return 0;
}

int CellularNetworkManager::setNetworkConfig(SimType simType, const CellularNetworkConfig& conf) {
    return 0;
}

int CellularNetworkManager::getNetworkConfig(SimType simType, CellularNetworkConfig* conf) {
    return 0;
}

void CellularNetworkManager::clearNetworkConfig(SimType simType) {
}

void CellularNetworkManager::clearNetworkConfig() {
}

void CellularNetworkManager::setActiveSimType(SimType simType) {
}

int CellularNetworkManager::getActiveSimType(SimType* simType) {
    return 0;
}

} // particle
