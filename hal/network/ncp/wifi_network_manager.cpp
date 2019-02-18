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

#include "wifi_network_manager.h"

#include "wifi_ncp_client.h"

#include "file_util.h"
#include "logging.h"
#include "scope_guard.h"
#include "check.h"

#include "spark_wiring_vector.h"

#include <algorithm>

// FIXME: Move nanopb utilities to a common header file
#include "../../../system/src/control/common.h"

// FIXME: Build internal protocol files in a separate directory
#include "../../../system/src/control/proto/internal.pb.h"

#define PB(_name) particle_firmware_##_name
#define PB_WIFI(_name) particle_ctrl_wifi_##_name

#define CONFIG_FILE "/sys/wifi_config.bin"

LOG_SOURCE_CATEGORY("ncp.mgr")

namespace particle {

namespace {

using namespace control::common;
using spark::Vector;

template<typename T>
void bssidToPb(const MacAddress& bssid, T* pbBssid) {
    if (bssid != INVALID_MAC_ADDRESS) {
        memcpy(pbBssid->bytes, &bssid, MAC_ADDRESS_SIZE);
        pbBssid->size = MAC_ADDRESS_SIZE;
    }
}

template<typename T>
void bssidFromPb(MacAddress* bssid, const T& pbBssid) {
    if (pbBssid.size == MAC_ADDRESS_SIZE) {
        memcpy(bssid, pbBssid.bytes, MAC_ADDRESS_SIZE);
    }
}

// TODO: Implement a couple functions to conveniently save/load a protobuf message to/from a file
int loadConfig(Vector<WifiNetworkConfig>* networks) {
    // Get filesystem instance
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    // Open configuration file
    lfs_file_t file = {};
    CHECK(openFile(&file, CONFIG_FILE, LFS_O_RDONLY));
    NAMED_SCOPE_GUARD(fileGuard, {
        lfs_file_close(&fs->instance, &file);
    });
    // Parse configuration
    PB(WifiConfig) pbConf = {};
    pbConf.networks.arg = networks;
    pbConf.networks.funcs.decode = [](pb_istream_t* strm, const pb_field_t* field, void** arg) {
        const auto networks = (Vector<WifiNetworkConfig>*)*arg;
        PB(WifiConfig_Network) pbConf = {};
        DecodedCString dSsid(&pbConf.ssid);
        DecodedCString dPwd(&pbConf.credentials.password);
        if (!pb_decode_noinit(strm, PB(WifiConfig_Network_fields), &pbConf)) {
            return false;
        }
        WifiCredentials cred;
        cred.type((WifiCredentials::Type)pbConf.credentials.type);
        if (pbConf.credentials.type == PB_WIFI(CredentialsType_PASSWORD)) {
            cred.password(dPwd.data);
        }
        MacAddress bssid = INVALID_MAC_ADDRESS;
        bssidFromPb(&bssid, pbConf.bssid);
        auto conf = WifiNetworkConfig()
            .ssid(dSsid.data)
            .bssid(bssid)
            .security((WifiSecurity)pbConf.security)
            .credentials(std::move(cred));
        if (!networks->append(std::move(conf))) {
            return false;
        }
        return true;
    };
    const int r = decodeMessageFromFile(&file, PB(WifiConfig_fields), &pbConf);
    if (r < 0) {
        LOG(ERROR, "Unable to parse network settings");
        networks->clear();
        LOG(WARN, "Removing file: %s", CONFIG_FILE);
        lfs_file_close(&fs->instance, &file);
        fileGuard.dismiss();
        lfs_remove(&fs->instance, CONFIG_FILE);
    }
    return 0;
}

int saveConfig(const Vector<WifiNetworkConfig>& networks) {
    // Get filesystem instance
    const auto fs = filesystem_get_instance(nullptr);
    CHECK_TRUE(fs, SYSTEM_ERROR_FILE);
    fs::FsLock lock(fs);
    CHECK(filesystem_mount(fs));
    // Open configuration file
    lfs_file_t file = {};
    CHECK(openFile(&file, CONFIG_FILE, LFS_O_WRONLY));
    SCOPE_GUARD({
        lfs_file_close(&fs->instance, &file);
    });
    int r = lfs_file_truncate(&fs->instance, &file, 0);
    CHECK_TRUE(r == LFS_ERR_OK, SYSTEM_ERROR_FILE);
    // Serialize configuration
    PB(WifiConfig) pbConf = {};
    pbConf.networks.arg = const_cast<Vector<WifiNetworkConfig>*>(&networks);
    pbConf.networks.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto networks = (const Vector<WifiNetworkConfig>*)*arg;
        for (const WifiNetworkConfig& conf: *networks) {
            PB(WifiConfig_Network) pbConf = {};
            EncodedString eSsid(&pbConf.ssid, conf.ssid(), strlen(conf.ssid()));
            EncodedString ePwd(&pbConf.credentials.password);
            bssidToPb(conf.bssid(), &pbConf.bssid);
            pbConf.security = (PB_WIFI(Security))conf.security();
            pbConf.credentials.type = (PB_WIFI(CredentialsType))conf.credentials().type();
            if (conf.credentials().type() == WifiCredentials::PASSWORD) {
                const auto s = conf.credentials().password();
                ePwd.data = s;
                ePwd.size = strlen(s);
            }
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(WifiConfig_Network_fields), &pbConf)) {
                return false;
            }
        }
        return true;
    };
    CHECK(encodeMessageToFile(&file, PB(WifiConfig_fields), &pbConf));
    LOG(TRACE, "Updated file: %s", CONFIG_FILE);
    return 0;
}

int networkIndexForSsid(const char* ssid, const Vector<WifiNetworkConfig>& networks) {
    for (int i = 0; i < networks.size(); ++i) {
        if (strcmp(ssid, networks.at(i).ssid()) == 0) {
            return i;
        }
    }
    return -1;
}

void sortByRssi(Vector<WifiScanResult>* scanResults) {
    std::sort(scanResults->begin(), scanResults->end(), [](const WifiScanResult& ap1, const WifiScanResult& ap2) {
        return (ap1.rssi() > ap2.rssi()); // In descending order
    });
}

} // unnamed

WifiNetworkManager::WifiNetworkManager(WifiNcpClient* client) :
        client_(client) {
}

WifiNetworkManager::~WifiNetworkManager() {
}

int WifiNetworkManager::connect(const char* ssid) {
    // Get known networks
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    int index = 0;
    if (ssid) {
        // Find network with the given SSID
        for (; index < networks.size(); ++index) {
            if (strcmp(networks.at(index).ssid(), ssid) == 0) {
                break;
            }
        }
    }
    if (index == networks.size()) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    // Connect to the network
    bool updateConfig = false;
    auto network = &networks.at(index);
    int r = client_->connect(network->ssid(), network->bssid(), network->security(), network->credentials());
    if (r < 0) {
        // Perform network scan
        Vector<WifiScanResult> scanResults;
        CHECK_TRUE(scanResults.reserve(10), SYSTEM_ERROR_NO_MEMORY);
        CHECK(client_->scan([](WifiScanResult result, void* data) -> int {
            auto scanResults = (Vector<WifiScanResult>*)data;
            CHECK_TRUE(scanResults->append(std::move(result)), SYSTEM_ERROR_NO_MEMORY);
            return 0;
        }, &scanResults));
        // Sort discovered networks by RSSI
        sortByRssi(&scanResults);
        // Try to connect to any known network among the discovered ones
        bool connected = false;
        for (const auto& ap: scanResults) {
            if (!ssid) {
                index = 0;
                for (; index < networks.size(); ++index) {
                    network = &networks.at(index);
                    if (strcmp(network->ssid(), ap.ssid()) == 0) {
                        break;
                    }
                }
                if (index == networks.size()) {
                    continue;
                }
            } else if (strcmp(ssid, ap.ssid()) != 0) {
                continue;
            }
            r = client_->connect(network->ssid(), ap.bssid(), network->security(), network->credentials());
            if (r == 0) {
                if (network->bssid() != ap.bssid()) {
                    // Update BSSID
                    network->bssid(ap.bssid());
                    updateConfig = true;
                }
                connected = true;
                break;
            }
        }
        if (!connected) {
            return SYSTEM_ERROR_NOT_FOUND;
        }
    } else if (network->bssid() == INVALID_MAC_ADDRESS) {
        // Update BSSID
        WifiNetworkInfo info;
        r = client_->getNetworkInfo(&info);
        if (r == 0) {
            network->bssid(info.bssid());
            updateConfig = true;
        }
    }
    if (index != 0) {
        // Move the network to the beginning of the list
        auto network = networks.takeAt(index);
        networks.prepend(std::move(network));
        updateConfig = true;
    }
    if (updateConfig) {
        saveConfig(networks);
    }
    return 0;
}

int WifiNetworkManager::setNetworkConfig(WifiNetworkConfig conf) {
    CHECK_TRUE(conf.ssid(), SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    int index = networkIndexForSsid(conf.ssid(), networks);
    if (index < 0) {
        // Add a new network or replace the last network in the list
        if (networks.size() < (int)MAX_CONFIGURED_WIFI_NETWORK_COUNT) {
            CHECK_TRUE(networks.resize(networks.size() + 1), SYSTEM_ERROR_NO_MEMORY);
        }
        index = networks.size() - 1;
    }
    networks[index] = std::move(conf);
    CHECK(saveConfig(networks));
    return 0;
}

int WifiNetworkManager::getNetworkConfig(const char* ssid, WifiNetworkConfig* conf) {
    // TODO: Cache the list of networks
    CHECK_TRUE(ssid, SYSTEM_ERROR_INVALID_ARGUMENT);
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    const int index = networkIndexForSsid(ssid, networks);
    if (index < 0) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    *conf = std::move(networks[index]);
    return 0;
}

int WifiNetworkManager::getNetworkConfig(GetNetworkConfigCallback callback, void* data) {
    Vector<WifiNetworkConfig> networks;
    CHECK(loadConfig(&networks));
    for (int i = 0; i < networks.size(); ++i) {
        const int ret = callback(std::move(networks[i]), data);
        if (ret < 0) {
            return ret;
        }
    }
    return 0;
}

void WifiNetworkManager::removeNetworkConfig(const char* ssid) {
    Vector<WifiNetworkConfig> networks;
    if (loadConfig(&networks) < 0) {
        return;
    }
    const int index = networkIndexForSsid(ssid, networks);
    if (index < 0) {
        return;
    }
    networks.removeAt(index);
    saveConfig(networks);
}

void WifiNetworkManager::clearNetworkConfig() {
    Vector<WifiNetworkConfig> networks;
    saveConfig(networks);
}

bool WifiNetworkManager::hasNetworkConfig() {
    Vector<WifiNetworkConfig> networks;
    const int r = loadConfig(&networks);
    if (r < 0) {
        return false;
    }
    return !networks.isEmpty();
}

} // particle
